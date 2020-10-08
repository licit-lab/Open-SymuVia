#include "CostsWriting.h"

#include "Simulation/Config/SimulationConfiguration.h"

#include "Simulation/Simulators/SimulationDescriptor.h"

#include <Graph/Model/Graph.h>
#include <Graph/Model/Link.h>
#include <Graph/Model/Pattern.h>
#include <Graph/Model/Cost.h>
#include <Graph/Model/Node.h>
#include <Graph/Model/AbstractPenalty.h>

#include <Demand/SubPopulation.h>
#include <Demand/Populations.h>
#include <Demand/Population.h>

#include <boost/filesystem/fstream.hpp>
#include <boost/filesystem.hpp>

#include <boost/date_time/posix_time/posix_time_io.hpp>

#include <boost/log/trivial.hpp>

#include <fstream>

#define CSV_SEPARATOR ';'


using namespace SymuMaster;
using namespace SymuCore;

CostsWriting::CostsWriting(const SimulationConfiguration & config, const SimulationDescriptor & simulationDescriptor) :
m_SimulationDescriptor(simulationDescriptor)
{
    m_strOutpuDirectoryPath = config.GetStrOutpuDirectoryPath();
    m_strOutputPrefix = config.GetStrOutputPrefix();

    //create the file and empty it if already exist
    std::string costsFileName = m_strOutputPrefix + "_costs.csv";
    boost::filesystem::ofstream CostsOutputOFS;
    CostsOutputOFS.open(boost::filesystem::path(m_strOutpuDirectoryPath) / costsFileName, boost::filesystem::ofstream::trunc);
    CostsOutputOFS << "Period;Iteration;Instance;StartTime;EndTime;Pattern/Penalty;Subpopulation;TravelTime;TravelTimeCost;MarginalCost;Emission" << std::endl;
    CostsOutputOFS.close();
}

CostsWriting::~CostsWriting()
{
}

bool CostsWriting::WriteCosts(int iPeriod, int iIteration, int iInstance, int iInstanceInCurrentBatch, const boost::posix_time::ptime &startPeriodTime, const boost::posix_time::ptime &endPeriodTime, double dbMiddleTime, const Populations & populations, Graph * pGraph)
{
    boost::filesystem::ofstream costsOutputOFS;

    std::string costsFileName = m_strOutputPrefix + "_costs.csv";
    costsOutputOFS.open(boost::filesystem::path(m_strOutpuDirectoryPath) / costsFileName, boost::filesystem::ofstream::app);

    if (costsOutputOFS.fail())
    {
        BOOST_LOG_TRIVIAL(error) << "Unable to open the file '" << m_strOutpuDirectoryPath << "\\" << m_strOutputPrefix << "_costs.csv' !";
        return false;
    }

    for (size_t iLink = 0; iLink < pGraph->GetListLinks().size(); iLink++)
    {
        Link * pLink = pGraph->GetListLinks().at(iLink);

        const std::vector<SubPopulation*> & lstSubPops = m_SimulationDescriptor.GetSubPopulationsByServiceType().at(pLink->getParent()->GetServiceType());

        for (size_t iPattern = 0; iPattern < pLink->getListPatterns().size(); iPattern++)
        {
            Pattern * pPattern = pLink->getListPatterns().at(iPattern);

            for (size_t iSubPop = 0; iSubPop < lstSubPops.size(); iSubPop++)
            {
                SubPopulation * pSubPop = lstSubPops.at(iSubPop);
                Cost * pCost = pPattern->getPatternCost(iInstanceInCurrentBatch, dbMiddleTime, pSubPop);

				costsOutputOFS << iPeriod << CSV_SEPARATOR << iIteration << CSV_SEPARATOR << iInstance << CSV_SEPARATOR << startPeriodTime << CSV_SEPARATOR << endPeriodTime
					<< CSV_SEPARATOR << pPattern->toString() << CSV_SEPARATOR << pSubPop->GetStrName() << CSV_SEPARATOR << pCost->getTravelTime()
					<< CSV_SEPARATOR << pCost->getCostValue(CF_TravelTime) << CSV_SEPARATOR << pCost->getCostValue(CF_Marginals)
					<< CSV_SEPARATOR << pCost->getCostValue(CF_Emissions) << std::endl;
            }
        }
    }

    for (size_t iNode = 0; iNode < pGraph->GetListNodes().size(); iNode++)
    {
        Node * pNode = pGraph->GetListNodes().at(iNode);
        const std::vector<SubPopulation*> & lstSubPops = m_SimulationDescriptor.GetSubPopulationsByServiceType().at(pNode->getParent()->GetServiceType());

        const std::map<Pattern*, std::map<Pattern*, AbstractPenalty*, ComparePattern>, ComparePattern> & mapPenalties = pNode->getMapPenalties();
        for (std::map<Pattern*, std::map<Pattern*, AbstractPenalty*, ComparePattern>, ComparePattern>::const_iterator iter = mapPenalties.begin(); iter != mapPenalties.end(); ++iter)
        {
            for (std::map<Pattern*, AbstractPenalty*, ComparePattern>::const_iterator iter2 = iter->second.begin(); iter2 != iter->second.end(); ++iter2)
            {
                AbstractPenalty * pPenalty = iter2->second;

                for (size_t iSubPop = 0; iSubPop < lstSubPops.size(); iSubPop++)
                {
                    SubPopulation * pSubPop = lstSubPops.at(iSubPop);
                    Cost * pCost = pPenalty->getPenaltyCost(iInstanceInCurrentBatch, dbMiddleTime, pSubPop);
                    costsOutputOFS << iPeriod << CSV_SEPARATOR << iIteration << CSV_SEPARATOR << iInstance << CSV_SEPARATOR << startPeriodTime << CSV_SEPARATOR << endPeriodTime
                        << CSV_SEPARATOR << iter->first->toString() << "->" << iter2->first->toString() << CSV_SEPARATOR << pSubPop->GetStrName() << CSV_SEPARATOR << pCost->getTravelTime()
                        << CSV_SEPARATOR << pCost->getCostValue(CF_TravelTime) << CSV_SEPARATOR << pCost->getCostValue(CF_Marginals)
						<< CSV_SEPARATOR << pCost->getCostValue(CF_Emissions) << std::endl;
                }
            }
        }
    }

    costsOutputOFS.close();

    return true;
}
