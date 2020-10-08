#include "TraficSimulators.h"

#include "TraficSimulatorHandler.h"
#include "SimulationDescriptor.h"
#include "Simulation/Config/SimulationConfiguration.h"

#include "Simulation/Users/UserEngine.h"

#include "IO/NetworkInterfacesFileHandler.h"

#include <Demand/Trip.h>
#include <Demand/Population.h>
#include <Graph/Model/Node.h>
#include <Graph/Model/Link.h>
#include <Demand/Origin.h>
#include <Demand/Destination.h>
#include <Graph/Model/Pattern.h>
#include <Graph/Model/AbstractPenalty.h>
#include <Graph/Model/MultiLayersGraph.h>

#include <boost/log/trivial.hpp>
#include <boost/date_time/posix_time/posix_time_io.hpp>

#include <set>

using namespace SymuMaster;
using namespace SymuCore;

TraficSimulators::TraficSimulators() :
    TraficSimulatorHandler(NULL, NULL)
{
}

TraficSimulators::~TraficSimulators()
{
    for (size_t i = 0; i < m_lstTraficSimulators.size(); i++)
    {
        delete m_lstTraficSimulators[i];
    }
    delete m_SimulationDescriptor.GetGraph();
}

const std::string & TraficSimulators::GetSimulatorName() const
{
    static const std::string combinedName = "CombinedSimulator";
    return combinedName;
}

void TraficSimulators::AddSimulatorHandler(TraficSimulatorHandler * pSimulatorhandler)
{
    m_lstTraficSimulators.push_back(pSimulatorhandler);
}

bool TraficSimulators::Empty() const
{
    return m_lstTraficSimulators.empty();
}

const std::vector<TraficSimulatorHandler*> & TraficSimulators::GetSimulators() const
{
    return m_lstTraficSimulators;
}

bool TraficSimulators::Initialize(const SimulationConfiguration & config, UserEngines & userEngines, TraficSimulatorHandler * pNullMasterSimulator)
{
    bool bOk = true;

    // First, get the "master" simulator if any. For now, the master simulator is SymuVia. The implication is the following :
    // the timestep used for "slaves" simulators is the one of the master simulator. An error occurs if a different value is chosen
    // for MFD simulator in acc based mode. In trip based mode, There is no "real" timestep so we can just take the one of SymuVia.
    m_pMasterSimulator = NULL;
    for (size_t i = 0; i < m_lstTraficSimulators.size() && bOk; i++)
    {
        TraficSimulatorHandler * pSimulator = m_lstTraficSimulators[i];
        if (!m_pMasterSimulator && pSimulator->GetSimulatorConfiguration()->GetSimulatorName() == ST_SymuVia)
        {
            m_pMasterSimulator = pSimulator;
            break;
        }
    }
    if (!m_pMasterSimulator && !m_lstTraficSimulators.empty())
    {
        m_pMasterSimulator = m_lstTraficSimulators.front();
    }

    // create the secondary graphs
    for (size_t i = 0; i < m_lstTraficSimulators.front()->GetSimulatorInstances().size(); i++)
    {
        if (i > 0)
        {
            m_lstSecondaryGraphs.push_back(new MultiLayersGraph());
        }
    }

    std::vector<TraficSimulatorHandler*> simulatorsToInitialize(1, m_pMasterSimulator);
    for (size_t i = 0; i < m_lstTraficSimulators.size() && bOk; i++)
    {
        if (m_lstTraficSimulators[i] != m_pMasterSimulator)
        {
            simulatorsToInitialize.push_back(m_lstTraficSimulators[i]);
        }
    }
    for (size_t i = 0; i < simulatorsToInitialize.size() && bOk; i++)
    {
        TraficSimulatorHandler * pSimulator = simulatorsToInitialize[i];
        bOk = pSimulator->Initialize(config, userEngines, pSimulator == m_pMasterSimulator ? NULL : m_pMasterSimulator);
        if (!bOk)
        {
            BOOST_LOG_TRIVIAL(error) << "Initialize failed for simulator " << simulatorsToInitialize[i]->GetSimulatorName();
        }
    }

    if (bOk)
    {
        if (m_lstTraficSimulators.empty())
        {
            bOk = false;
            BOOST_LOG_TRIVIAL(error) << "No simulator defined...";
        }
        else
        {
            std::set<boost::posix_time::time_duration> timeSteps;
            // first, define simulation temporal definition
            for (size_t iSimulator = 0; iSimulator < m_lstTraficSimulators.size() && bOk; iSimulator++)
            {
                TraficSimulatorHandler * pSimulator = m_lstTraficSimulators[iSimulator];

                SimulationDescriptor & simDescr = pSimulator->GetSimulationDescriptor();              

                timeSteps.insert(simDescr.GetTimeStepDuration());

                if (iSimulator == 0)
                {
                    m_SimulationDescriptor.SetStartTime(simDescr.GetStartTime());
                    m_SimulationDescriptor.SetEndTime(simDescr.GetEndTime());

                    m_minTimeStep = simDescr.GetTimeStepDuration();
                }
                else
                {
                    bOk = m_SimulationDescriptor.GetStartTime() == simDescr.GetStartTime()
                        && m_SimulationDescriptor.GetEndTime() == simDescr.GetEndTime();

                    m_minTimeStep = std::min<boost::posix_time::time_duration>(m_minTimeStep, simDescr.GetTimeStepDuration());

                    if (!bOk)
                    {
                        BOOST_LOG_TRIVIAL(error) << "Incompatible temporal definitions of the simulation between the simulators :";
                        BOOST_LOG_TRIVIAL(error) << "Expected start time : " << m_SimulationDescriptor.GetStartTime();
                        BOOST_LOG_TRIVIAL(error) << "Simulator start time : " << simDescr.GetStartTime();
                        BOOST_LOG_TRIVIAL(error) << "Expected end time : " << m_SimulationDescriptor.GetEndTime();
                        BOOST_LOG_TRIVIAL(error) << "Simulator end time : " << simDescr.GetEndTime();
                        break;
                    }
                }
            }

            // cheking timesteps
            if (timeSteps.size() == 1)
            {
                m_SimulationDescriptor.SetTimeStepDuration(*timeSteps.begin());
            }
            else
            {
                m_SimulationDescriptor.SetTimeStepDuration(*timeSteps.rbegin());

                // ensure that timesteps are multiple of each other
                for (std::set<boost::posix_time::time_duration>::const_reverse_iterator iter = timeSteps.rbegin(); iter != timeSteps.rend() && bOk; ++iter)
                {
                    for (std::set<boost::posix_time::time_duration>::const_reverse_iterator iter2 = iter; iter2 != timeSteps.rend() && bOk; ++iter2)
                    {
                        if (iter->ticks() % iter2->ticks() != 0)
                        {
                            BOOST_LOG_TRIVIAL(error) << "Incompatible timestep duration values for each simulator. They should be multiple of each other.";
                            bOk = false;
                        }
                    }
                }
            }
        }
    }
    return bOk;
}

bool TraficSimulators::BuildSimulationGraph(MultiLayersGraph *pParentGraph, MultiLayersGraph *pSimulatorGraph)
{
    bool bOk = true;

	NetworkInterfacesFileHandler networksInterfacesFileHandler;
	if (!m_pSimulationConfig->GetNetworksInterfacesFilePath().empty())
	{
		bOk = networksInterfacesFileHandler.Load(m_pSimulationConfig->GetNetworksInterfacesFilePath(), m_SimulationDescriptor);
	}

    for (size_t iSimulator = 0; iSimulator < m_lstTraficSimulators.size() && bOk; iSimulator++)
    {
        TraficSimulatorHandler * pSimulator = m_lstTraficSimulators[iSimulator];

        // associate simulator's graph to the root graph
        m_SimulationDescriptor.GetGraph()->AddLayer(pSimulator->GetSimulationDescriptor().GetGraph());

        // build simulator's graph
        bOk = pSimulator->BuildSimulationGraph(m_SimulationDescriptor.GetGraph(), pSimulator->GetSimulationDescriptor().GetGraph());
        if (!bOk)
        {
            BOOST_LOG_TRIVIAL(error) << "Building graph failed for simulator " << pSimulator->GetSimulatorName();
            break;
        }
    }

    if (bOk && !m_pSimulationConfig->GetNetworksInterfacesFilePath().empty())
    {
        bOk = networksInterfacesFileHandler.Apply(m_SimulationDescriptor, m_lstTraficSimulators);

        // Checking timesteps equality
        if (bOk)
        {
            for (size_t iSimulator = 1; iSimulator < m_lstTraficSimulators.size() && bOk; iSimulator++)
            {
                TraficSimulatorHandler * pSimulator = m_lstTraficSimulators[iSimulator];

                if (pSimulator->GetSimulationDescriptor().GetTimeStepDuration() != m_lstTraficSimulators.front()->GetSimulationDescriptor().GetTimeStepDuration())
                {
                    BOOST_LOG_TRIVIAL(error) << "When using a networks interfaces file for hybridation, timestep durations of all simulators must be equal";
                    bOk = false;
                }
            }
        }
    }

    if(bOk)
    {
        FillMapIndexedServiceTypes();
    }

    return bOk;
}

bool TraficSimulators::Step(const boost::posix_time::ptime & startTimeStep, const boost::posix_time::time_duration & longestTimeStepDuration, int iInstance, UserEngine * pUserEngine)
{
	bool bOk = true;

	// order the simulators bya timestep value
	std::map<boost::posix_time::time_duration::tick_type, std::vector<TraficSimulatorHandler*> > mapSimulatorByTimestep;
	for (size_t i = 0; i < m_lstTraficSimulators.size() && bOk; i++)
	{
		TraficSimulatorHandler * pSimulator = m_lstTraficSimulators[i];
		// put the master simulator first (for hybridation, we want SymuVia to run first. Maybe it is not needed anymore since we chose to force all simulators to have the same
		// timestep for hybridation ?)
		if (pSimulator == m_pMasterSimulator)
		{
			mapSimulatorByTimestep[pSimulator->GetSimulationDescriptor().GetTimeStepDuration().ticks()].insert(mapSimulatorByTimestep[pSimulator->GetSimulationDescriptor().GetTimeStepDuration().ticks()].begin(), pSimulator);
		}
		else
		{
			mapSimulatorByTimestep[pSimulator->GetSimulationDescriptor().GetTimeStepDuration().ticks()].push_back(pSimulator);
		}
	}

	assert(mapSimulatorByTimestep.size() <= 2); // Dealing with 3 different timesteps would greatly complexify the UserEngine "Run" method for hybridation. With only 2 different timesteps,
	// we can just run the shortest timesteps simulators and then the longest timestep ones.

	std::map<boost::posix_time::time_duration::tick_type, std::vector<TraficSimulatorHandler*> >::const_iterator iterTimestep = mapSimulatorByTimestep.begin();
	std::vector<TraficSimulatorHandler*> minTimeStepSimulators = iterTimestep->second;
	iterTimestep++;
	std::vector<TraficSimulatorHandler*> maxTimeStepSimulators;
	if (iterTimestep != mapSimulatorByTimestep.end())
	{
		maxTimeStepSimulators = iterTimestep->second;
	}

	// we start by the short/master timestep (so all the trips for the long timestep (MFD) are activated before runnning it)
	boost::posix_time::ptime currentTime = startTimeStep;
	while (bOk && currentTime < startTimeStep + longestTimeStepDuration)
	{
		bOk = pUserEngine->Run(GetSimulationDescriptor().GetPopulations(), currentTime, m_minTimeStep);

		for (size_t iSimulator = 0; iSimulator < minTimeStepSimulators.size() && bOk; iSimulator++)
		{
			TraficSimulatorHandler * pSimulator = minTimeStepSimulators[iSimulator];

			bOk = pSimulator->Step(currentTime, m_minTimeStep, iInstance);

			if (!bOk)
			{
				BOOST_LOG_TRIVIAL(error) << "Step failed for simulator " << pSimulator->GetSimulatorName();
			}
			else
			{
				// update the interfaces
				for (size_t i = 0; i < m_lstTraficSimulators.size() && bOk; i++)
				{
					TraficSimulatorHandler * pOtherSimulator = m_lstTraficSimulators[i];
					if (pOtherSimulator != pSimulator)
					{
						bOk = UpdateInterfacesCapacities(pSimulator, pOtherSimulator, iInstance);
					}
				}
			}
		}

		currentTime += m_minTimeStep;
	}

	for (size_t iSimulator = 0; iSimulator < maxTimeStepSimulators.size() && bOk; iSimulator++)
	{
		TraficSimulatorHandler * pSimulator = maxTimeStepSimulators[iSimulator];

		bOk = pSimulator->Step(startTimeStep, pSimulator->GetSimulationDescriptor().GetTimeStepDuration(), iInstance);

		if (!bOk)
		{
			BOOST_LOG_TRIVIAL(error) << "Step failed for simulator " << pSimulator->GetSimulatorName();
		}
		else
		{
			for (size_t i = 0; i < m_lstTraficSimulators.size() && bOk; i++)
			{
				TraficSimulatorHandler * pOtherSimulator = m_lstTraficSimulators[i];
				if (pOtherSimulator != pSimulator)
				{
					bOk = UpdateInterfacesCapacities(pSimulator, pOtherSimulator, iInstance);
				}
			}
		}
	}

	return bOk;
}

bool TraficSimulators::UpdateInterfacesCapacities(TraficSimulatorHandler * pDownstreamSimulator, TraficSimulatorHandler * pUpstreamSimulator, int iInstance)
{
    // For efficiency reasons (calls to matlab are costly), we process all downstream interface points at a time :
    std::map<const AbstractSimulatorInterface*, std::vector<const AbstractSimulatorInterface*> > mapInterfacesByDownstreamInterface;
    const std::vector<SimulatorsInterface> & interfaces = m_SimulationDescriptor.GetSimulatorsInterfaces()[pDownstreamSimulator][pUpstreamSimulator];
    for (size_t iInterface = 0; iInterface < interfaces.size(); iInterface++)
    {
        const SimulatorsInterface & interfaceRef = interfaces[iInterface];
		if (interfaceRef.HasToAdaptCapacityAndOffer()) {
			mapInterfacesByDownstreamInterface[interfaceRef.GetDownstreamInterface()].push_back(interfaceRef.GetUpstreamInterface());
		}
    }

    bool bOk = true;
    if (!mapInterfacesByDownstreamInterface.empty())
    {
        // Get the offer of the downstream simulator's entry points :
        std::vector<const AbstractSimulatorInterface*> interfacesToGetOffer;
        for (std::map<const AbstractSimulatorInterface*, std::vector<const AbstractSimulatorInterface*> >::const_iterator iter = mapInterfacesByDownstreamInterface.begin(); iter != mapInterfacesByDownstreamInterface.end(); ++iter)
        {
            interfacesToGetOffer.push_back(iter->first);
        }
        std::map<const AbstractSimulatorInterface*, double> offerValues;
		bOk = pDownstreamSimulator->GetOffers(iInstance, interfacesToGetOffer, offerValues);

        // Update the capacity of the upstream simulator exit points :
        for (std::map<const AbstractSimulatorInterface*, std::vector<const AbstractSimulatorInterface*> >::const_iterator iter = mapInterfacesByDownstreamInterface.begin(); bOk && iter != mapInterfacesByDownstreamInterface.end(); ++iter)
        {
            assert(!iter->second.empty()); // if there is a vector in the map, there should be something inside it, so an if() test is unnecessary to avoid useless calls to the simulators (can be costly for matlab simulators)

            bOk = pUpstreamSimulator->SetCapacities(iInstance, iter->second, iter->first, offerValues.at(iter->first));
        }
    }

    return bOk;
}

Origin * TraficSimulators::GetParentOrigin(Origin * pChildOrigin)
{
    assert(false); // should not be called for TraficSimulators
    return NULL;
}

Destination * TraficSimulators::GetParentDestination(Destination * pChildDestination)
{
    assert(false); // should not be called for TraficSimulators
    return NULL;
}

bool TraficSimulators::FillEstimatedCostsForNewPeriod(const boost::posix_time::ptime & startSimulationTime, const boost::posix_time::ptime & startPeriodTime, const boost::posix_time::ptime & endPeriodTime, const boost::posix_time::time_duration & travelTimesUpdatePeriod, const std::map<SubPopulation*, CostFunction> & mapCostFunctions, int nbInstancesNumber)
{
    bool bOk = true;
    for (size_t i = 0; i < m_lstTraficSimulators.size(); i++)
    {
        bOk = m_lstTraficSimulators[i]->ComputeNewPeriodCosts(mapCostFunctions);
        if (!bOk)
        {
            return false;
        }
    }

    double dbStartPeriodTime = (double)((startPeriodTime - startSimulationTime).total_microseconds()) / 1000000;
    double dbEndPeriodTime = (double)((endPeriodTime - startSimulationTime).total_microseconds()) / 1000000;
    double dbTravelTimesUpdatePeriod = (double)travelTimesUpdatePeriod.total_microseconds() / 1000000;
    Link * pLink;
    Node * pNode;
    for (int iInstance = 0; iInstance < nbInstancesNumber; iInstance++)
    {
        MultiLayersGraph * pGraph = iInstance == 0 ? m_SimulationDescriptor.GetGraph() : m_lstSecondaryGraphs[iInstance - 1];

        for (size_t iLink = 0; iLink < pGraph->GetListLinks().size(); iLink++)
        {
            pLink = pGraph->GetListLinks()[iLink];
            for (size_t iPattern = 0; iPattern < pLink->getListPatterns().size(); iPattern++)
            {
                Pattern * pPattern = pLink->getListPatterns()[iPattern];
                pPattern->prepareTimeFrames(dbStartPeriodTime, dbEndPeriodTime, dbTravelTimesUpdatePeriod, m_SimulationDescriptor.GetSubPopulationsByServiceType()[pPattern->getLink()->getParent()->GetServiceType()], mapCostFunctions, nbInstancesNumber, iInstance);
            }
        }

        for (size_t iNode = 0; iNode < pGraph->GetListNodes().size(); iNode++)
        {
            pNode = pGraph->GetListNodes()[iNode];

            std::map<Pattern*, std::map<Pattern*, AbstractPenalty*, ComparePattern>, ComparePattern>::iterator iter;
            for (iter = pNode->getMapPenalties().begin(); iter != pNode->getMapPenalties().end(); ++iter)
            {
                for (std::map<Pattern*, AbstractPenalty*, ComparePattern>::iterator iterDown = iter->second.begin(); iterDown != iter->second.end(); ++iterDown)
                {
                    iterDown->second->prepareTimeFrames(dbStartPeriodTime, dbEndPeriodTime, dbTravelTimesUpdatePeriod, m_SimulationDescriptor.GetSubPopulationsByServiceType()[pNode->getParent()->GetServiceType()], mapCostFunctions, nbInstancesNumber, iInstance);
                }
            }
        }
    }

    return true;
}

bool TraficSimulators::FillTripsDescriptor(MultiLayersGraph * pGraph, Populations & populations,
    const boost::posix_time::ptime & startSimulationTime, const boost::posix_time::ptime & endSimulationTime, bool bIgnoreSubAreas,
    std::vector<SymuCore::Trip> & placeHolder)
{
    bool bOk = true;

    std::vector<SymuCore::Trip> lstTrips;
    for (size_t i = 0; i < m_lstTraficSimulators.size() && bOk; i++)
    {
        TraficSimulatorHandler * pSimulator = m_lstTraficSimulators[i];

        bOk = pSimulator->FillTripsDescriptor(pGraph, populations, startSimulationTime, endSimulationTime, bIgnoreSubAreas, lstTrips);
        if (!bOk)
        {
            BOOST_LOG_TRIVIAL(error) << "FillTripsDescriptor failed for simulator " << m_lstTraficSimulators[i]->GetSimulatorName();
        }
    }

    if (bOk)
    {
        // tri par ordre de departure time
        std::sort(lstTrips.begin(), lstTrips.end(), SymuCore::Trip::DepartureTimeSorter);

        // Création de Trips dans les populations associées
        int tripIDCounter = 1;
        for (size_t iTrip = 0; iTrip < lstTrips.size(); iTrip++)
        {
            const SymuCore::Trip & trip = lstTrips[iTrip];
            trip.GetPopulation()->AddTrip(tripIDCounter++, trip.GetDepartureTime(), trip.GetOrigin(), trip.GetDestination(), trip.GetSubPopulation(), trip.GetVehicleType());
        }
    }

    return bOk;
}

bool TraficSimulators::ComputeCosts(int iTravelTimesUpdatePeriodIndex, boost::posix_time::ptime startTravelTimesUpdatePeriodTime, boost::posix_time::ptime endTravelTimesUpdatePeriodTime, const std::map<SymuCore::SubPopulation*, SymuCore::CostFunction> & mapCostFunctions, int iSimulationInstance)
{
    bool bOk = true;

    // tell Simulators to prepare computing costs for travel times update period
    for (size_t i = 0; i < m_lstTraficSimulators.size() && bOk; i++)
    {
        TraficSimulatorHandler * pSimulator = m_lstTraficSimulators[i];

        bOk = pSimulator->ComputeCosts(iTravelTimesUpdatePeriodIndex, startTravelTimesUpdatePeriodTime, endTravelTimesUpdatePeriodTime, mapCostFunctions, iSimulationInstance);
        if (!bOk)
        {
            BOOST_LOG_TRIVIAL(error) << "ComputeCosts failed for simulator " << m_lstTraficSimulators[i]->GetSimulatorName();
        }
    }

    // fill patterns and penalties with computed costs
    if (bOk)
    {
        MultiLayersGraph * pGraph = iSimulationInstance == 0 ? m_SimulationDescriptor.GetGraph() : m_lstSecondaryGraphs[iSimulationInstance - 1];
        Link * pLink;
        for (size_t iLink = 0; iLink < pGraph->GetListLinks().size(); iLink++)
        {
            pLink = pGraph->GetListLinks()[iLink];
            for (size_t iPattern = 0; iPattern < pLink->getListPatterns().size(); iPattern++)
            {
                Pattern * pPattern = pLink->getListPatterns()[iPattern];
                pPattern->fillMeasuredCostsForTravelTimesUpdatePeriod(iTravelTimesUpdatePeriodIndex, m_SimulationDescriptor.GetSubPopulationsByServiceType()[pPattern->getLink()->getParent()->GetServiceType()], mapCostFunctions);
            }
        }

        Node * pNode;
        for (size_t iNode = 0; iNode < pGraph->GetListNodes().size(); iNode++)
        {
            pNode = pGraph->GetListNodes()[iNode];
            std::map<Pattern*, std::map<Pattern*, AbstractPenalty*, ComparePattern>, ComparePattern>::iterator iter;
            for (iter = pNode->getMapPenalties().begin(); iter != pNode->getMapPenalties().end(); ++iter)
            {
                for (std::map<Pattern*, AbstractPenalty*, ComparePattern>::iterator iterDown = iter->second.begin(); iterDown != iter->second.end(); ++iterDown)
                {
                    iterDown->second->fillMeasuredCostsForTravelTimesUpdatePeriod(iTravelTimesUpdatePeriodIndex, m_SimulationDescriptor.GetSubPopulationsByServiceType()[pNode->getParent()->GetServiceType()], mapCostFunctions);
                }
            }
        }
    }

    return bOk;
}

void TraficSimulators::PostProcessCosts(int iSimulationInstance, const std::map<SymuCore::SubPopulation*, SymuCore::CostFunction> & mapCostFunctions)
{
    MultiLayersGraph * pGraph = iSimulationInstance == 0 ? m_SimulationDescriptor.GetGraph() : m_lstSecondaryGraphs[iSimulationInstance - 1];

    Link * pLink;
    for (size_t iLink = 0; iLink < pGraph->GetListLinks().size(); iLink++)
    {
        pLink = pGraph->GetListLinks()[iLink];
        for (size_t iPattern = 0; iPattern < pLink->getListPatterns().size(); iPattern++)
        {
            Pattern * pPattern = pLink->getListPatterns()[iPattern];
            pPattern->postProcessCosts(m_SimulationDescriptor.GetSubPopulationsByServiceType()[pPattern->getLink()->getParent()->GetServiceType()], mapCostFunctions);
        }
    }

    Node * pNode;
    for (size_t iNode = 0; iNode < pGraph->GetListNodes().size(); iNode++)
    {
        pNode = pGraph->GetListNodes()[iNode];
        std::map<Pattern*, std::map<Pattern*, AbstractPenalty*, ComparePattern>, ComparePattern>::iterator iter;
        for (iter = pNode->getMapPenalties().begin(); iter != pNode->getMapPenalties().end(); ++iter)
        {
            for (std::map<Pattern*, AbstractPenalty*, ComparePattern>::iterator iterDown = iter->second.begin(); iterDown != iter->second.end(); ++iterDown)
            {
                iterDown->second->postProcessCosts(m_SimulationDescriptor.GetSubPopulationsByServiceType()[pNode->getParent()->GetServiceType()], mapCostFunctions);
            }
        }
    }
}


void TraficSimulators::MergeCostsForSecondaryInstance(int iInstance)
{
    // rmk. target graph has more patterns and penalties, but those are nullpatterns and nullpenalties (links/penalties between global origins/destinations and layer origins/destinations),
    // so it does not matter if we don't merge those.
    MultiLayersGraph * pTargetGraph = m_SimulationDescriptor.GetGraph();
    MultiLayersGraph * pOriginGraph = m_lstSecondaryGraphs[iInstance - 1];

    Link * pLink, * pOriginalLink;
    for (size_t iLink = 0; iLink < pOriginGraph->GetListLinks().size(); iLink++)
    {
        pLink = pTargetGraph->GetListLinks()[iLink];
        pOriginalLink = pOriginGraph->GetListLinks()[iLink];
        for (size_t iPattern = 0; iPattern < pLink->getListPatterns().size(); iPattern++)
        {
            Pattern * pTargetPattern = pLink->getListPatterns()[iPattern];
            Pattern * pOriginalPattern = pOriginalLink->getListPatterns()[iPattern];

            pTargetPattern->fillFromSecondaryInstance(pOriginalPattern, iInstance);
        }
    }

    Node * pNode, * pOriginalNode;
    for (size_t iNode = 0; iNode < pOriginGraph->GetListNodes().size(); iNode++)
    {
        pNode = pTargetGraph->GetListNodes()[iNode];
        pOriginalNode = pOriginGraph->GetListNodes()[iNode];
        std::map<Pattern*, std::map<Pattern*, AbstractPenalty*, ComparePattern>, ComparePattern>::iterator iter, iterOriginal;
        for (iter = pNode->getMapPenalties().begin(), iterOriginal = pOriginalNode->getMapPenalties().begin(); iter != pNode->getMapPenalties().end() && iterOriginal != pOriginalNode->getMapPenalties().end(); ++iter, ++iterOriginal)
        {
            std::map<Pattern*, AbstractPenalty*, ComparePattern>::iterator iterDownOriginal;
            for (std::map<Pattern*, AbstractPenalty*, ComparePattern>::iterator iterDown = iter->second.begin(), iterDownOriginal = iterOriginal->second.begin(); iterDown != iter->second.end(); ++iterDown, ++iterDownOriginal)
            {
                iterDown->second->fillFromSecondaryInstance(iterDownOriginal->second, iInstance);
            }
        }
    }
}

bool TraficSimulators::ProcessPathsAssignment(const std::vector<SymuCore::Trip*> & assignedTrips, const boost::posix_time::time_duration & predictionPeriodDuration, const boost::posix_time::ptime & startTime, const boost::posix_time::ptime & endTime, int iSimulationInstance)
{
    bool bOk = true;
    for (size_t i = 0; i < m_lstTraficSimulators.size() && bOk; i++)
    {
        bOk = m_lstTraficSimulators[i]->ProcessPathsAssignment(assignedTrips, predictionPeriodDuration, startTime, endTime, iSimulationInstance);
        if (!bOk)
        {
            BOOST_LOG_TRIVIAL(error) << "ProcessPathsAssignment failed for simulator " << m_lstTraficSimulators[i]->GetSimulatorName();
        }
    }

    return bOk;
}

bool TraficSimulators::ActivatePPath(const PPath & currentPPath, const boost::posix_time::ptime & startTimeStepTime, const boost::posix_time::ptime & departureTime)
{
    assert(false); // must not be called for TraficSimulators
    return false;
}

bool TraficSimulators::Terminate()
{
    bool bOk = true;
    for (size_t i = 0; i < m_lstTraficSimulators.size(); i++)
    {
        if (!m_lstTraficSimulators[i]->Terminate())
        {
            bOk = false;
            // no break here to release simulators resources no matter what
            if (!bOk)
            {
                BOOST_LOG_TRIVIAL(error) << "Terminate failed for simulator " << m_lstTraficSimulators[i]->GetSimulatorName();
            }
        }
    }
    return bOk;
}

bool TraficSimulators::AssignmentPeriodStart(int iPeriod, bool bIsNotTheFinalIterationForPeriod, int nbMaxInstancesNb)
{
    bool bOk = true;
    for (size_t i = 0; i < m_lstTraficSimulators.size() && bOk; i++)
    {
        bOk = m_lstTraficSimulators[i]->AssignmentPeriodStart(iPeriod, bIsNotTheFinalIterationForPeriod, nbMaxInstancesNb);
        if (!bOk)
        {
            BOOST_LOG_TRIVIAL(error) << "AssignmentPeriodStartFailed failed for simulator " << m_lstTraficSimulators[i]->GetSimulatorName();
        }
    }

    return bOk;
}

bool TraficSimulators::AssignmentPeriodEnded(int iInstance)
{
    bool bOk = true;
    for (size_t i = 0; i < m_lstTraficSimulators.size() && bOk; i++)
    {
        bOk = m_lstTraficSimulators[i]->AssignmentPeriodEnded(iInstance);
        if (!bOk)
        {
            BOOST_LOG_TRIVIAL(error) << "AssignmentPeriodEnded failed for simulator " << m_lstTraficSimulators[i]->GetSimulatorName();
        }
    }

    return bOk;
}

bool TraficSimulators::AssignmentPeriodRollback(int nbMaxInstancesNb)
{
    bool bOk = true;

    for (size_t i = 0; i < m_lstTraficSimulators.size() && bOk; i++)
    {
        bOk = m_lstTraficSimulators[i]->AssignmentPeriodRollback(nbMaxInstancesNb);
        if (!bOk)
        {
            BOOST_LOG_TRIVIAL(error) << "AssignmentPeriodRollback failed for simulator " << m_lstTraficSimulators[i]->GetSimulatorName();
        }
    }

    return bOk;
}

bool TraficSimulators::AssignmentPeriodCommit(int nbMaxInstancesNb, bool bAssignmentPeriodEnded)
{
    bool bOk = true;
    for (size_t i = 0; i < m_lstTraficSimulators.size() && bOk; i++)
    {
        bOk = m_lstTraficSimulators[i]->AssignmentPeriodCommit(nbMaxInstancesNb, bAssignmentPeriodEnded);
        if (!bOk)
        {
            BOOST_LOG_TRIVIAL(error) << "AssignmentPeriodCommit failed for simulator " << m_lstTraficSimulators[i]->GetSimulatorName();
        }
    }

    return bOk;
}

bool TraficSimulators::IsExtraIterationToProduceOutputsRelevant()
{
    bool bIsRelevant = false;
    for (size_t i = 0; i < m_lstTraficSimulators.size() && !bIsRelevant; i++)
    {
        bIsRelevant = m_lstTraficSimulators[i]->IsExtraIterationToProduceOutputsRelevant();
    }
    return bIsRelevant;
}

const Populations &TraficSimulators::GetPopulations() const
{
    return m_SimulationDescriptor.GetPopulations();
}

Populations &TraficSimulators::GetPopulations()
{
    return m_SimulationDescriptor.GetPopulations();
}

void TraficSimulators::FillMapIndexedServiceTypes()
{
    std::map<ServiceType, std::vector<SubPopulation*> > indexedMap = GetSimulationDescriptor().GetSubPopulationsByServiceType();
    std::vector<Population*> listPopulation = GetPopulations().getListPopulations();
    Population* currentPopulation;
    ServiceType currentServiceType;
    bool find = false;

    for (size_t iPopulation = 0; iPopulation < listPopulation.size(); iPopulation++)
    {
        currentPopulation = listPopulation[iPopulation];
        for (size_t iServiceType = 0; iServiceType < currentPopulation->GetListEnabledServices().size(); iServiceType++)
        {
            currentServiceType = currentPopulation->GetListEnabledServices()[iServiceType];

            for (size_t iSubPop = 0; iSubPop < currentPopulation->GetListSubPopulations().size(); iSubPop++)
            {
                SubPopulation * pSubPop = currentPopulation->GetListSubPopulations()[iSubPop];
                find = false;

                //verify if the Macrotype already exist for the ServiceType
                for (size_t iSubPopulation = 0; iSubPopulation < indexedMap[currentServiceType].size(); iSubPopulation++)
                {
                    if (indexedMap[currentServiceType][iSubPopulation] == pSubPop)
                    {
                        find = true;
                        break;
                    }
                }
                if (!find){
                    indexedMap[currentServiceType].push_back(pSubPop);
                }
            }
        }
    }

    //All population who have access to ST_All, also have access to all other services
    std::vector<ServiceType> listAllServices;
    m_SimulationDescriptor.GetGraph()->GetListServiceType(listAllServices);
    for (int iEnum = 0; iEnum < listAllServices.size(); iEnum++)
    {
        currentServiceType = listAllServices[iEnum];

        for(size_t iST_All = 0; iST_All < indexedMap[ST_All].size(); iST_All++)
        {

            //verify if the Macrotype already exist for the ServiceType
            find = false;
            for(size_t iMacroEnum = 0; iMacroEnum < indexedMap[currentServiceType].size(); iMacroEnum++)
            {
                if(indexedMap[currentServiceType][iMacroEnum] == indexedMap[ST_All][iST_All])
                {
                    find = true;
                    break;
                }
            }
            if(!find){
                indexedMap[currentServiceType].push_back(indexedMap[ST_All][iST_All]);
            }
        }
    }

    //Add the map parts for other simulators
    for(size_t iSimulator = 0; iSimulator<m_lstTraficSimulators.size(); iSimulator++)
    {
        std::vector<ServiceType> listServiceType;
        m_lstTraficSimulators[iSimulator]->GetSimulationDescriptor().GetGraph()->GetListServiceType(listServiceType);
        for(size_t iServiceType = 0; iServiceType < listServiceType.size(); iServiceType++)
        {
            m_lstTraficSimulators[iSimulator]->GetSimulationDescriptor().GetSubPopulationsByServiceType()[listServiceType[iServiceType]] = indexedMap[listServiceType[iServiceType]];
        }
        m_lstTraficSimulators[iSimulator]->GetSimulationDescriptor().GetSubPopulationsByServiceType()[ST_All] = indexedMap[ST_All];
    }

    m_SimulationDescriptor.SetSubPopulationsByServiceType(indexedMap);
}

void TraficSimulators::PostFillDemand()
{
    // have to copy origins and destinations to secondary graphs so all graphs build the same way (with links with conditional creation with known origins/destinations, see SymuCoreDrivingGraphBuilder)
    for (size_t iSecondaryGraph = 0; iSecondaryGraph < m_lstSecondaryGraphs.size(); iSecondaryGraph++)
    {
        m_lstSecondaryGraphs[iSecondaryGraph]->SetListDestination(m_SimulationDescriptor.GetGraph()->GetListDestination());
        m_lstSecondaryGraphs[iSecondaryGraph]->SetListOrigin(m_SimulationDescriptor.GetGraph()->GetListOrigin());
		m_lstSecondaryGraphs[iSecondaryGraph]->SetListUpstreamInterfaces(m_SimulationDescriptor.GetGraph()->GetListUpstreamInterfaces());
		m_lstSecondaryGraphs[iSecondaryGraph]->SetListDownstreamInterfaces(m_SimulationDescriptor.GetGraph()->GetListDownstreamInterfaces());

        // have to put macro types also
        m_lstSecondaryGraphs[iSecondaryGraph]->SetListMacroTypes(m_SimulationDescriptor.GetGraph()->GetListMacroTypes());
    }
}

void TraficSimulators::MergeRelatedODs()
{
    Graph * pGraph = GetSimulationDescriptor().GetGraph();
    const std::vector<Origin*> originalOrigins = pGraph->GetListOrigin();
    for (size_t iOrigin = 0; iOrigin < originalOrigins.size(); iOrigin++)
    {
        Origin * pOrigin = originalOrigins[iOrigin];
        for (size_t iSimulator = 0; iSimulator < m_lstTraficSimulators.size(); iSimulator++)
        {
            TraficSimulatorHandler * pSimulator = m_lstTraficSimulators[iSimulator];

            //rmk : this creates the origin in the graph if necessary
            Origin * pParentOrigin = pSimulator->GetParentOrigin(pOrigin);

            if (pParentOrigin)
            {
                // 1 - replace in the demand the child origin by the parent one
                for (size_t iPop = 0; iPop < GetPopulations().getListPopulations().size(); iPop++)
                {
                    Population * pPop = GetPopulations().getListPopulations()[iPop];
                    for (size_t iTrip = 0; iTrip < pPop->GetListTrips().size(); iTrip++)
                    {
                        Trip * pTrip = pPop->GetListTrips()[iTrip];
                        if (pTrip->GetOrigin() == pOrigin)
                        {
                            pTrip->SetOrigin(pParentOrigin);
                        }
                    }
                }

                // 2 - remove the child origin for the graph
                pGraph->RemoveOrigin(pOrigin);
            }
        }
    }

    // same thing for Destinations
    const std::vector<Destination*> originalDestinations = pGraph->GetListDestination();
    for (size_t iDest = 0; iDest < originalDestinations.size(); iDest++)
    {
        Destination * pDestination = originalDestinations[iDest];
        for (size_t iSimulator = 0; iSimulator < m_lstTraficSimulators.size(); iSimulator++)
        {
            TraficSimulatorHandler * pSimulator = m_lstTraficSimulators[iSimulator];

            //rmk : this creates the destination in the graph if necessary
            Destination * pParentDestination = pSimulator->GetParentDestination(pDestination);

            if (pParentDestination)
            {
                // 1 - replace in the demand the child destination by the parent one
                for (size_t iPop = 0; iPop < GetPopulations().getListPopulations().size(); iPop++)
                {
                    Population * pPop = GetPopulations().getListPopulations()[iPop];
                    for (size_t iTrip = 0; iTrip < pPop->GetListTrips().size(); iTrip++)
                    {
                        Trip * pTrip = pPop->GetListTrips()[iTrip];
                        if (pTrip->GetDestination() == pDestination)
                        {
                            pTrip->SetDestination(pDestination);
                        }
                    }
                }

                // 2 - remove the child destination for the graph
                pGraph->RemoveDestination(pDestination);
            }
        }
    }
}

void TraficSimulators::SetSimulationConfig(const SimulationConfiguration * pSimulationConfig)
{
    m_pSimulationConfig = pSimulationConfig;
}

