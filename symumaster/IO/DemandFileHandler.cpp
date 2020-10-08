#include "DemandFileHandler.h"

#include "Demand/Populations.h"
#include "Demand/Population.h"
#include "Demand/SubPopulation.h"
#include "Demand/VehicleType.h"
#include "Demand/MacroType.h"
#include "Demand/Trip.h"
#include "Demand/Origin.h"
#include "Demand/Destination.h"
#include "Graph/Model/Node.h"
#include "Graph/Model/Link.h"
#include "Graph/Model/Pattern.h"
#include "Graph/Model/Graph.h"
#include "Simulation/Simulators/SymuVia/SymuViaConfig.h"
#include "Simulation/Config/SimulationConfiguration.h"
#include "Simulation/Simulators/SimulationDescriptor.h"
#include "CSVStringUtils.h"

#include <boost/log/trivial.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/filesystem/convenience.hpp>

#include <iostream>
#include <fstream>

#include <set>

const char s_CSV_SEPARATOR = ';';
const int s_ID_COLUMN = 0;
const int s_POPULATION_COLUMN = 1;
const int s_SUB_POPULATION_COLUMN = 2;
const int s_VEHICLE_TYPE_COLUMN = 3;
const int s_DEPARTURE_TIME_COLUMN = 4;
const int s_ORIGIN_COLUMN = 5;
const int s_DESTINATION_COLUMN = 6;
const int s_PREDEFINED_PATH_COLUMN = 7;

using namespace SymuMaster;
using namespace SymuCore;
using namespace std;
using namespace boost::posix_time;

DemandFileHandler::DemandFileHandler()
{

}

DemandFileHandler::~DemandFileHandler()
{

}

bool DemandFileHandler::FillDemands(const std::string &strCSVFilePath, const ptime & startSimulationTime, const ptime & endSimulationTime,
                               Populations& pPopulations, Graph* pGraph)
{
    bool bOk = true;
    int nbIgnoredTrips = 0;
    bool generatePopulations = pPopulations.getListPopulations().empty();
    map<string, Population*> alreadySeen;
    Population* currentPopulation;
    SubPopulation* currentSubPopulation;
    VehicleType* currentVehicleType;
    ptime tripTime;
    ptime lastDepartureTime;

    string line;
    vector<string> values;
    std::set<int> IDValues;
    ifstream myfile (strCSVFilePath.c_str());
    if (myfile.is_open())
    {
        while (getline(myfile,line)) // line should be id;population_name;subPopulation_name;vehicleType_name;time(HH:MM:SS.mmm);origin_name;destination_name[;pattern_name_1/downstream_node_name_1/.../downstream_node_name_n]
        {
            values = CSVStringUtils::split(line, s_CSV_SEPARATOR);
            if (values.size() < s_PREDEFINED_PATH_COLUMN)
            {
                BOOST_LOG_TRIVIAL(error) << "Wrong format for the line : " << line << " !";
                bOk = false;
                break;
            }
            int nID;
            try
            {
                nID = boost::lexical_cast<int>(values[s_ID_COLUMN]);
                if (nID <= 0)
                {
                    BOOST_LOG_TRIVIAL(error) << "Trip identifier '" << values[s_ID_COLUMN] << "' must be a valid strictly positive integer !";
                    bOk = false;
                    break;
                }
            }
            catch (boost::bad_lexical_cast& )
            {
                BOOST_LOG_TRIVIAL(error) << "Trip identifier '" << values[s_ID_COLUMN] << "' must be a valid positive number !";
                bOk = false;
                break;
            }
            size_t oldIDNumber = IDValues.size();
            IDValues.insert(nID);
            if (IDValues.size() == oldIDNumber)
            {
                BOOST_LOG_TRIVIAL(error) << "Duplicate trip identifier '" << nID << "' !";
                bOk = false;
                break;
            }
            currentPopulation = pPopulations.getPopulation(values[s_POPULATION_COLUMN]);
            if(generatePopulations && !currentPopulation)
            {
                currentPopulation = new Population(values[s_POPULATION_COLUMN]);
                MacroType* pMacroType = new MacroType();
                currentPopulation->SetMacroType(pMacroType);
                //CPQ - TODO - peux etre qu'il ne faut pas ajouter la valeur ST_All mais un autre service à definir ?
                currentPopulation->addServiceType(ST_All);
                pGraph->AddMacroType(pMacroType);
                pPopulations.getListPopulations().push_back(currentPopulation);
            }

            if (currentPopulation)
            {
                currentSubPopulation = currentPopulation->hasSubPopulation(values[s_SUB_POPULATION_COLUMN]);
                if(generatePopulations && !currentSubPopulation)
                {
                    currentSubPopulation = new SubPopulation(values[s_SUB_POPULATION_COLUMN]);
                    currentPopulation->addSubPopulation(currentSubPopulation);
                }
                if(!currentSubPopulation)
                {
                    BOOST_LOG_TRIVIAL(error) << "SubPopulation '" << values[s_SUB_POPULATION_COLUMN] << "' doesn't exist in the Population !";
                    bOk = false;
                    break;
                }

                if(alreadySeen[values[s_VEHICLE_TYPE_COLUMN]] == NULL || alreadySeen[values[s_VEHICLE_TYPE_COLUMN]] == currentPopulation)
                {
                    alreadySeen[values[s_VEHICLE_TYPE_COLUMN]] = currentPopulation;
                }else{
                    BOOST_LOG_TRIVIAL(error) << "VehicleType '" << values[s_VEHICLE_TYPE_COLUMN] << "' can't belong to 2 different Populations !";
                    bOk = false;
                    break;
                }

                // we can have populations with no macro type (public transport only for example) :
                if (currentPopulation->GetMacroType())
                {
                    currentVehicleType = currentPopulation->GetMacroType()->getVehicleType(values[s_VEHICLE_TYPE_COLUMN]);
                    if (generatePopulations && !currentVehicleType)
                    {
                        currentVehicleType = new VehicleType(values[s_VEHICLE_TYPE_COLUMN]);
                        currentPopulation->GetMacroType()->addVehicleType(currentVehicleType);
                    }
                    if (!currentVehicleType)
                    {
                        BOOST_LOG_TRIVIAL(error) << "VehicleType '" << values[s_VEHICLE_TYPE_COLUMN] << "' doesn't belong to the MacroType of Population '" << values[s_POPULATION_COLUMN] << "' !";
                        bOk = false;
                        break;
                    }
                }
                else
                {
                    currentVehicleType = NULL;
                }

                try{
                    if(values[s_DEPARTURE_TIME_COLUMN].find(' ') == -1){//date is not specified
                        tripTime = ptime(startSimulationTime.date(), duration_from_string(values[s_DEPARTURE_TIME_COLUMN]));
                    }else{
                        tripTime = time_from_string(values[s_DEPARTURE_TIME_COLUMN]);
                    }
                }catch (const boost::bad_lexical_cast& ) {
                    BOOST_LOG_TRIVIAL(error) << "Wrong format for the date_time : '" << values[s_DEPARTURE_TIME_COLUMN] << "' !";
                    bOk = false;
                    break;
                }

                if(tripTime.is_not_a_date_time())
                {
                    BOOST_LOG_TRIVIAL(error) << "Wrong format for the date_time : '" << values[s_DEPARTURE_TIME_COLUMN] << "' !";
                    bOk = false;
                    break;
                }else{   //first trip read
                    lastDepartureTime = tripTime;
                }

                if(tripTime < startSimulationTime || tripTime > endSimulationTime){
                    nbIgnoredTrips++;
                    continue;
                }


                Trip* pTrip = currentPopulation->AddTrip(nID, tripTime, pGraph->GetOrCreateOrigin(values[s_ORIGIN_COLUMN]), pGraph->GetOrCreateDestination(values[s_DESTINATION_COLUMN]), currentSubPopulation, currentVehicleType);

                //add the predefined path
                if(values.size() > s_PREDEFINED_PATH_COLUMN)
                {
                    m_mapPaths[pTrip] = values[s_PREDEFINED_PATH_COLUMN];
                }
            }else{
                BOOST_LOG_TRIVIAL(error) << "Population '" << values[s_POPULATION_COLUMN] << "' doesn't exist !";
                bOk = false;
                break;
            }
        }

        myfile.close();
    }else{
        BOOST_LOG_TRIVIAL(error) << "Error when opening Demand file -> " << strCSVFilePath;
        bOk = false;
    }

    if(bOk && nbIgnoredTrips>0){
        BOOST_LOG_TRIVIAL(warning) << nbIgnoredTrips << " trip(s) have been ignored because they don't fit in the simulation";
    }

    return bOk;
}

bool DemandFileHandler::AddUsersPredefinedPaths(Graph *pGraph, int nbInstances)
{
    bool bOk = true;

    if(m_mapPaths.empty())
        return bOk;

    for(map<Trip*, string>::iterator itPath=m_mapPaths.begin(); itPath != m_mapPaths.end() && bOk; itPath++)
    {
        vector<string> strPath = CSVStringUtils::split(itPath->second, '/');
        if((strPath.size() %2) != 1) // path alternate pattern and node and end with a pattern, so it should be an odd number
        {
            BOOST_LOG_TRIVIAL(error) << "Predefined path from '" << itPath->first->GetOrigin()->getStrNodeName() << "' to '" << itPath->first->GetDestination()->getStrNodeName() << "' with vehicle type '" << itPath->first->GetVehicleType()->getStrName() << "' is not correct !";
            bOk = false;
            break;
        }

        Path newPath;
        Trip* pTrip = itPath->first;
        pTrip->SetIsPredefinedPath(true);

        bOk = CSVStringUtils::verifyPath(strPath, pTrip->GetOrigin(), pTrip->GetDestination(), newPath);

        if(!bOk)
            break;

        for (int iInstance = 0; iInstance < nbInstances; iInstance++)
        {
            itPath->first->SetPath(iInstance, newPath);
        }
    }

    m_mapPaths.clear();

    return bOk;
}

bool DemandFileHandler::CheckOriginAndDestinations(SymuCore::Graph *pGraph)
{
    bool bOk = true;
    for (vector<Origin*>::const_iterator itOrigin = pGraph->GetListOrigin().begin(); itOrigin != pGraph->GetListOrigin().end(); itOrigin++)
    {
        if (!(*itOrigin)->getNode())
        {
            BOOST_LOG_TRIVIAL(error) << "Origin '" << (*itOrigin)->getStrNodeName() << "' doesn't exist in the network !";
            bOk = false;
            break;
        }
    }

    for (vector<Destination*>::const_iterator itDestination = pGraph->GetListDestination().begin(); itDestination != pGraph->GetListDestination().end() && bOk; itDestination++)
    {
        if (!(*itDestination)->getNode())
        {
            BOOST_LOG_TRIVIAL(error) << "Destination '" << (*itDestination)->getStrNodeName() << "' doesn't exist in the network !";
            bOk = false;
            break;
        }
    }

    return bOk;
}

std::string DemandFileHandler::BuildAvailableDemandFileName(const SimulationConfiguration & config)
{
    bool bAvailableNameFound = false;
    std::string demandFilePath;
    int intSuffix = 0;
    while (!bAvailableNameFound)
    {
        std::string candidateDemandFilePath;
        if (!config.GetLoadedConfigurationFilePath().empty())
        {
            boost::filesystem::path configPath(config.GetLoadedConfigurationFilePath());
            candidateDemandFilePath = configPath.parent_path().operator/=(configPath.stem()).string();
        }
        else
        {
            // we use in this case the path of the input symuvia XML file
            bool bSymuViaFound = false;
            for (size_t iSim = 0; iSim < config.GetLstSimulatorConfigs().size() && !bSymuViaFound; iSim++)
            {
                SymuViaConfig * pSymuViaConfig = dynamic_cast<SymuViaConfig*>(config.GetLstSimulatorConfigs()[iSim]);
                if (pSymuViaConfig)
                {
                    boost::filesystem::path symuViaScenarioPath(pSymuViaConfig->GetXMLFilePath());
                    candidateDemandFilePath = symuViaScenarioPath.parent_path().operator/=(symuViaScenarioPath.stem()).string();
                    bSymuViaFound = true;
                }
            }

            assert(bSymuViaFound); // deal with the case of no SymuVia simulator nor XML configuration file if this can happen...
        }

        candidateDemandFilePath += "_demand";
        if (intSuffix > 0)
        {
            std::ostringstream oss;
            oss << "_" << intSuffix;
            candidateDemandFilePath += oss.str();
        }
        candidateDemandFilePath += ".csv";

        if (!boost::filesystem::exists(candidateDemandFilePath))
        {
            demandFilePath = candidateDemandFilePath;
            bAvailableNameFound = true;
        }
        else
        {
            intSuffix++;
        }
    }

    return demandFilePath;
}

bool DemandFileHandler::Write(const SimulationConfiguration & config, const SimulationDescriptor & pSimDesc)
{
    bool bOk = true;

    // definition of the demand file path do write to
    string demandFilePath = BuildAvailableDemandFileName(config);

    // write the file :
    ofstream myfile;
    myfile.open(demandFilePath.c_str(), std::ios::out);
    bOk = myfile.is_open() && myfile.good();

    if (bOk)
    {
        // sorting trips to write
        std::vector<Trip*> tripsToWrite;
        const std::vector<Population*> & populations = pSimDesc.GetPopulations().getListPopulations();
        for (size_t iPopulation = 0; iPopulation < populations.size(); iPopulation++)
        {
            tripsToWrite.insert(tripsToWrite.end(), populations[iPopulation]->GetListTrips().begin(), populations[iPopulation]->GetListTrips().end());
        }
        std::sort(tripsToWrite.begin(), tripsToWrite.end(), Trip::DepartureTimePtrSorter);

        for (size_t iTrip = 0; iTrip < tripsToWrite.size(); iTrip++)
        {
            Trip * pTrip = tripsToWrite[iTrip];
            myfile << pTrip->GetID() << s_CSV_SEPARATOR
                   << pTrip->GetPopulation()->GetStrName() << s_CSV_SEPARATOR
                   << pTrip->GetSubPopulation()->GetStrName() << s_CSV_SEPARATOR
                   << (pTrip->GetVehicleType() ? pTrip->GetVehicleType()->getStrName() : "") << s_CSV_SEPARATOR
                   << pTrip->GetDepartureTime() << s_CSV_SEPARATOR
                   << pTrip->GetOrigin()->getStrNodeName() << s_CSV_SEPARATOR
                   << pTrip->GetDestination()->getStrNodeName() << s_CSV_SEPARATOR
                   << std::endl;
        }
    }

    myfile.close();

    if (bOk)
    {
        bOk = !myfile.is_open() && myfile.good();
    }

    if (bOk)
    {
        BOOST_LOG_TRIVIAL(warning) << "No demand file has been speficied as input of SymuMaster. The demand file has been auto-generated from the simulation description and saved as '" << demandFilePath << "'.";
        BOOST_LOG_TRIVIAL(warning) << "You can use this file as the input demand file next time to save computational time and prevent generating a new file again.";
    }
    else
    {
        BOOST_LOG_TRIVIAL(error) << "Error writing demand file '" << demandFilePath << "'";
    }

    return bOk;
}
