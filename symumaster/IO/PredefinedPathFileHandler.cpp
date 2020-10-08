#include "PredefinedPathFileHandler.h"

#include "Demand/Populations.h"
#include "Demand/Population.h"
#include "Demand/Path.h"
#include "Demand/Origin.h"
#include "Demand/Destination.h"
#include "Graph/Model/Node.h"
#include "Graph/Model/Link.h"
#include "Graph/Model/Pattern.h"
#include "Graph/Model/Graph.h"
#include "CSVStringUtils.h"
#include <boost/log/trivial.hpp>

#include <iostream>
#include <fstream>
#include <set>

const char s_CSV_SEPARATOR = ';';
const int s_POPULATION_COLUMN = 0;
const int s_SUB_POPULATION_COLUMN = 1;
const int s_ORIGIN_COLUMN = 2;
const int s_DESTINATION_COLUMN = 3;
const int s_PREDEFINED_PATH_COLUMN = 4;
const int s_FIRST_PERIOD_AFFECTATION_COLUMN = 5;

using namespace SymuMaster;
using namespace SymuCore;
using namespace std;
using namespace boost::posix_time;

PredefinedPathFileHandler::PredefinedPathFileHandler()
{

}

PredefinedPathFileHandler::~PredefinedPathFileHandler()
{

}

bool PredefinedPathFileHandler::FillPredefinedPaths(const string &strCSVFilePath, Graph* pGraph, Populations &pPopulations,
                                std::map<SymuCore::SubPopulation *, std::map<SymuCore::Origin *, std::map<SymuCore::Destination *, std::map<SymuCore::Path, std::vector<double> >, AssignmentModel::SortDestination >, AssignmentModel::SortOrigin >, AssignmentModel::SortSubPopulation > &mapPredefinedPaths)
{
    bool bOk = true;
    std::vector<Population*> allPopulations = pPopulations.getListPopulations();
    std::vector<SubPopulation*> listSubPopulations;
    Origin * pOrigin;
    Destination * pDestination;

    std::set<std::string> ignoredOrigins, ignoredDestinations;

    string line;
    vector<string> values;
    ifstream myfile (strCSVFilePath.c_str());
    if (myfile.is_open())
    {
        while (getline(myfile,line) && bOk) // line should be [population_name];[sub_population_name];origin_name;destination_name;pattern_name_1/downstream_node_name_1/.../downstream_node_name_n
        {
            values = CSVStringUtils::split(line, s_CSV_SEPARATOR);
            if(values.size() < 5)
            {
                BOOST_LOG_TRIVIAL(error) << "Wrong format for the line : " << line << " !";
                bOk = false;
                break;
            }

            //verify Population
            if(values[s_POPULATION_COLUMN] == "")
            {
                listSubPopulations.clear();
                //verify SubPopulation
                for(size_t i = 0; i < allPopulations.size(); i++)
                {
                    Population* currentPopulation = allPopulations[i];
                    if(values[s_SUB_POPULATION_COLUMN] == "")
                    {
                        listSubPopulations.insert(listSubPopulations.end(), currentPopulation->GetListSubPopulations().begin(), currentPopulation->GetListSubPopulations().end());
                    }else{
                        SubPopulation* currentSubPopulation = currentPopulation->hasSubPopulation(values[s_SUB_POPULATION_COLUMN]);
                        if(!currentPopulation)
                        {
                            BOOST_LOG_TRIVIAL(error) << "SubPopulation '" << values[s_SUB_POPULATION_COLUMN] << "' doesn't exist in the Population !";
                            bOk = false;
                            break;
                        }
                        listSubPopulations.push_back(currentSubPopulation);
                    }
                }
            }else{
                Population* currentPopulation = pPopulations.getPopulation(values[s_POPULATION_COLUMN]);
                if(!currentPopulation)
                {
                    BOOST_LOG_TRIVIAL(error) << "Population '" << values[s_POPULATION_COLUMN] << "' doesn't exist in the network !";
                    bOk = false;
                    break;
                }
                //verify SubPopulation
                if(values[s_SUB_POPULATION_COLUMN] == "")
                {
                    listSubPopulations = currentPopulation->GetListSubPopulations();
                }else{
                    SubPopulation* currentSubPopulation = currentPopulation->hasSubPopulation(values[s_SUB_POPULATION_COLUMN]);
                    if(!currentPopulation)
                    {
                        BOOST_LOG_TRIVIAL(error) << "SubPopulation '" << values[s_SUB_POPULATION_COLUMN] << "' doesn't exist in the Population !";
                        bOk = false;
                        break;
                    }
                    listSubPopulations.clear();
                    listSubPopulations.push_back(currentSubPopulation);
                }
            }

            //verify Origin
			pOrigin = pGraph->getOrigin(values[s_ORIGIN_COLUMN]);
			if (!pOrigin)
            {
                // to avoid spamming this warning for the same origin
                if (ignoredOrigins.find(values[s_ORIGIN_COLUMN]) == ignoredOrigins.end())
                {
                    BOOST_LOG_TRIVIAL(warning) << "Origin '" << values[s_ORIGIN_COLUMN] << "' doesn't exist in the network or in the demand : ignoring path.";
                    ignoredOrigins.insert(values[s_ORIGIN_COLUMN]);
                }
                continue;
            }

            //verify Destination
			pDestination = pGraph->getDestination(values[s_DESTINATION_COLUMN]);
			if (!pDestination)
            {
                // to avoid spamming this warning for the same destination
                if (ignoredDestinations.find(values[s_DESTINATION_COLUMN]) == ignoredDestinations.end())
                {
                    BOOST_LOG_TRIVIAL(warning) << "Destination '" << values[s_DESTINATION_COLUMN] << "' doesn't exist in the network or in the demand : igoring path.";
                    ignoredDestinations.insert(values[s_DESTINATION_COLUMN]);
                }
                continue;
            }

            vector<string> strPath = CSVStringUtils::split(values[s_PREDEFINED_PATH_COLUMN], '/');

            if((strPath.size() %2) != 1) // path alternate pattern and node and end with a pattern, so it should be an odd number
            {
                BOOST_LOG_TRIVIAL(error) << "Predefined path from '" << pOrigin->getStrNodeName() << "' to '" << pDestination->getStrNodeName() << "' is not correct !";
                bOk = false;
                break;
            }

            Path newPath;
            if (!CSVStringUtils::verifyPath(strPath, pOrigin, pDestination, newPath))
			{
				BOOST_LOG_TRIVIAL(error) << "Predefined path from '" << pOrigin->getStrNodeName() << "' to '" << pDestination->getStrNodeName() << "' is not correct !";
				bOk = false;
                break;
			}

            //add the affectation
            vector<double> affectationPercent;
            string strAffectationValue;
            double dbAffectationValue;
            for(int iPeriod = 0; iPeriod < ((int)values.size() - s_FIRST_PERIOD_AFFECTATION_COLUMN); iPeriod++)
            {
                strAffectationValue = values[iPeriod + s_FIRST_PERIOD_AFFECTATION_COLUMN];
                if(strAffectationValue == "")
                {
                    dbAffectationValue = -1; // -1 mean no value found
                }else{
                    try {
                        dbAffectationValue = stod(strAffectationValue);
                    }
                    catch (std::invalid_argument ia) {
                        BOOST_LOG_TRIVIAL(error) << "The " << iPeriod << "ieth value of predefined path from '" << pOrigin->getStrNodeName() << "' to '" << pDestination->getStrNodeName() << "' is not correct !";
                        bOk = false;
                        break;
                    }
                }
                affectationPercent.push_back(dbAffectationValue);
            }

            //add the predefined path
            SubPopulation* pSubPopulation;
            for(size_t j=0; j < listSubPopulations.size(); j++)
            {
                pSubPopulation = listSubPopulations[j];
                mapPredefinedPaths[pSubPopulation][pOrigin][pDestination][newPath] = affectationPercent;
            }
        }

        //verify if the affectation for periods are correct
        vector<double> sumAffectation;
        vector<double> currentPathAffectation;
        for (map<SubPopulation*, map<Origin*, map<Destination*, map<Path, vector<double> >, AssignmentModel::SortDestination >, AssignmentModel::SortOrigin >, AssignmentModel::SortSubPopulation >::iterator iterSubPopulation = mapPredefinedPaths.begin(); iterSubPopulation != mapPredefinedPaths.end() && bOk; ++iterSubPopulation)
        {
            for (map<Origin*, map<Destination*, map<Path, vector<double> >, AssignmentModel::SortDestination >, AssignmentModel::SortOrigin >::iterator iterOrigin = iterSubPopulation->second.begin(); iterOrigin != iterSubPopulation->second.end() && bOk; ++iterOrigin)
            {
                for (map<Destination*, map<Path, vector<double> >, AssignmentModel::SortDestination >::iterator iterDestination = iterOrigin->second.begin(); iterDestination != iterOrigin->second.end() && bOk; ++iterDestination)
                {
                    sumAffectation.clear();
                    for (map<Path, vector<double> >::iterator iterPath = iterDestination->second.begin(); iterPath != iterDestination->second.end() && bOk; ++iterPath)
                    {
                        currentPathAffectation = iterPath->second;
                        if(sumAffectation.size() != 0 && currentPathAffectation.size() != sumAffectation.size())
                        {
                            BOOST_LOG_TRIVIAL(error) << "Not all paths have the same number of affectation for paths from '" << iterOrigin->first->getStrNodeName() << "' to '" << iterDestination->first->getStrNodeName() << "' !";
                            bOk = false;
                            break;
                        }

						if (sumAffectation.size() == 0)
						{
							sumAffectation = currentPathAffectation;
						}
						else{
							for (size_t iPeriod = 0; iPeriod < currentPathAffectation.size(); iPeriod++)
							{
								sumAffectation[iPeriod] += currentPathAffectation[iPeriod];
							}
						}
                    }

                    for(size_t iPeriod = 0; iPeriod < sumAffectation.size(); iPeriod++)
                    {
                        // don't bother about -1 values
                        if (sumAffectation[iPeriod] != (double)(sumAffectation.size() * -1.0))
                        {
                            // we accept a sum slightly different than 1 and renormalize automatically in this case(same as what we do in SymuVia in the same case) :
                            double dbTolerance = iterDestination->second.size() * std::numeric_limits<double>::epsilon();
                            if (fabs(sumAffectation[iPeriod] - 1.0) > dbTolerance && fabs(sumAffectation[iPeriod] - 1.0) < 0.0001)
                            {
                                // renormalisation
                                for (map<Path, vector<double> >::iterator iterPath = iterDestination->second.begin(); iterPath != iterDestination->second.end(); ++iterPath)
                                {
                                    vector<double> & currentPathAffectation = iterPath->second;
                                    currentPathAffectation[iPeriod] /= sumAffectation[iPeriod];
                                }
                                sumAffectation[iPeriod] = 1.0;
                            }
                            if (fabs(sumAffectation[iPeriod] - 1.0) > dbTolerance)
                            {
                                BOOST_LOG_TRIVIAL(error) << "The sum of all affectation for paths from '" << iterOrigin->first->getStrNodeName() << "' to '" << iterDestination->first->getStrNodeName() << "' is not equal to 1.0 !";
                                bOk = false;
                                break;
                            }
                        }
                    }
                }
            }
        }

        myfile.close();

    }else{

        BOOST_LOG_TRIVIAL(error) << "Error when opening PredefinedPaths file -> " << strCSVFilePath;
        bOk = false;
    }

    return bOk;
}
