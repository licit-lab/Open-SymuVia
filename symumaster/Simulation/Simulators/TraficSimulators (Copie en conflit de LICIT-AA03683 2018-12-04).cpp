#include "TraficSimulators.h"

#include "TraficSimulatorHandler.h"
#include "SimulationDescriptor.h"
#include "Simulation/Config/SimulationConfiguration.h"

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

using namespace SymuMaster;
using namespace SymuCore;

TraficSimulators::TraficSimulators() :
    TraficSimulatorHandler(NULL)
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

bool TraficSimulators::Initialize(const SimulationConfiguration & config, SymuCore::IUserHandler * pUserHandler)
{
    bool bOk = true;

    for (size_t i = 0; i < m_lstTraficSimulators.size() && bOk; i++)
    {
        TraficSimulatorHandler * pSimulator = m_lstTraficSimulators[i];
        bOk = pSimulator->Initialize(config, pUserHandler);
        if (!bOk)
        {
            BOOST_LOG_TRIVIAL(error) << "Initialize failed for simulator " << m_lstTraficSimulators[i]->GetSimulatorName();
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
            // first, define simulation temporal definition
            for (size_t iSimulator = 0; iSimulator < m_lstTraficSimulators.size() && bOk; iSimulator++)
            {
                TraficSimulatorHandler * pSimulator = m_lstTraficSimulators[iSimulator];

                SimulationDescriptor & simDescr = pSimulator->GetSimulationDescriptor();

                // For now, we just make sure that all simulators use the same StartTime, EndTime, and TimeStepDuration.
                // We can refine this if needed later.

                if (iSimulator == 0)
                {
                    m_SimulationDescriptor.SetStartTime(simDescr.GetStartTime());
                    m_SimulationDescriptor.SetEndTime(simDescr.GetEndTime());
                    m_SimulationDescriptor.SetTimeStepDuration(simDescr.GetTimeStepDuration());
                }
                else
                {
                    bOk = m_SimulationDescriptor.GetStartTime() == simDescr.GetStartTime()
                        && m_SimulationDescriptor.GetEndTime() == simDescr.GetEndTime()
                        && m_SimulationDescriptor.GetTimeStepDuration() == simDescr.GetTimeStepDuration();

                    if (!bOk)
                    {
                        BOOST_LOG_TRIVIAL(error) << "Incompatible temporal definitions of the simulation between the simulators";
                        break;
                    }
                }
            }
        }
    }
    return bOk;
}

bool TraficSimulators::BuildSimulationGraph(MultiLayersGraph *pParentGraph)
{
    bool bOk = true;

    for (size_t iSimulator = 0; iSimulator < m_lstTraficSimulators.size() && bOk; iSimulator++)
    {
        TraficSimulatorHandler * pSimulator = m_lstTraficSimulators[iSimulator];

        // associate simulator's graph to the root graph
        m_SimulationDescriptor.GetGraph()->AddLayer(pSimulator->GetSimulationDescriptor().GetGraph());

        // build simulator's graph
        bOk = pSimulator->BuildSimulationGraph(m_SimulationDescriptor.GetGraph());
        if (!bOk)
        {
            BOOST_LOG_TRIVIAL(error) << "Building graph failed for simulator " << pSimulator->GetSimulatorName();
            break;
        }
    }

    if(bOk)
    {
        FillMapIndexedServiceTypes();
    }

    return bOk;
}

bool TraficSimulators::Step(const boost::posix_time::ptime & startTimeStep, const boost::posix_time::time_duration & timeStepDuration)
{
    bool bOk = true;
    for (size_t i = 0; i < m_lstTraficSimulators.size() && bOk; i++)
    {
        bOk = m_lstTraficSimulators[i]->Step(startTimeStep, timeStepDuration);
        if (!bOk)
        {
            BOOST_LOG_TRIVIAL(error) << "Step failed for simulator " << m_lstTraficSimulators[i]->GetSimulatorName();
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

bool TraficSimulators::FillEstimatedCostsForNewPeriod(const boost::posix_time::ptime & startSimulationTime, const boost::posix_time::ptime & startPeriodTime, const boost::posix_time::ptime & endPeriodTime, const boost::posix_time::time_duration & travelTimesUpdatePeriod, const std::map<SubPopulation*, CostFunction> & mapCostFunctions)
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
    for (size_t iLink = 0; iLink < m_SimulationDescriptor.GetGraph()->GetListLinks().size(); iLink++)
    {
        pLink = m_SimulationDescriptor.GetGraph()->GetListLinks()[iLink];
        for (size_t iPattern = 0; iPattern < pLink->getListPatterns().size(); iPattern++)
        {
            Pattern * pPattern = pLink->getListPatterns()[iPattern];
            pPattern->prepareTimeFrames(dbStartPeriodTime, dbEndPeriodTime, dbTravelTimesUpdatePeriod, m_SimulationDescriptor.GetSubPopulationsByServiceType()[pPattern->getLink()->getParent()->GetServiceType()], mapCostFunctions);
        }
    }

    Node * pNode;
    for (size_t iNode = 0; iNode < m_SimulationDescriptor.GetGraph()->GetListNodes().size(); iNode++)
    {
        pNode = m_SimulationDescriptor.GetGraph()->GetListNodes()[iNode];

        std::map<Pattern*, std::map<Pattern*, AbstractPenalty*> >::iterator iter;
        for (iter = pNode->getMapPenalties().begin(); iter != pNode->getMapPenalties().end(); ++iter)
        {
            for (std::map<Pattern*, AbstractPenalty*>::iterator iterDown = iter->second.begin(); iterDown != iter->second.end(); ++iterDown)
            {
                iterDown->second->prepareTimeFrames(dbStartPeriodTime, dbEndPeriodTime, dbTravelTimesUpdatePeriod, m_SimulationDescriptor.GetSubPopulationsByServiceType()[pNode->getParent()->GetServiceType()], mapCostFunctions);
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

bool TraficSimulators::ComputeCosts(int iTravelTimesUpdatePeriodIndex, boost::posix_time::ptime startTravelTimesUpdatePeriodTime, boost::posix_time::ptime endTravelTimesUpdatePeriodTime, const std::map<SymuCore::SubPopulation*, SymuCore::CostFunction> & mapCostFunctions)
{
    bool bOk = true;

    // tell Simulators to prepare computing costs for travel times update period
    for (size_t i = 0; i < m_lstTraficSimulators.size() && bOk; i++)
    {
        TraficSimulatorHandler * pSimulator = m_lstTraficSimulators[i];

        bOk = pSimulator->ComputeCosts(iTravelTimesUpdatePeriodIndex, startTravelTimesUpdatePeriodTime, endTravelTimesUpdatePeriodTime, mapCostFunctions);
        if (!bOk)
        {
            BOOST_LOG_TRIVIAL(error) << "ComputeCosts failed for simulator " << m_lstTraficSimulators[i]->GetSimulatorName();
        }
    }

    // fill patterns and penalties with computed costs
    if (bOk)
    {
        Link * pLink;
        for (size_t iLink = 0; iLink < m_SimulationDescriptor.GetGraph()->GetListLinks().size(); iLink++)
        {
            pLink = m_SimulationDescriptor.GetGraph()->GetListLinks()[iLink];
            for (size_t iPattern = 0; iPattern < pLink->getListPatterns().size(); iPattern++)
            {
                Pattern * pPattern = pLink->getListPatterns()[iPattern];
                pPattern->fillMeasuredCostsForTravelTimesUpdatePeriod(iTravelTimesUpdatePeriodIndex, m_SimulationDescriptor.GetSubPopulationsByServiceType()[pPattern->getLink()->getParent()->GetServiceType()], mapCostFunctions);
            }
        }

        Node * pNode;
        for (size_t iNode = 0; iNode < m_SimulationDescriptor.GetGraph()->GetListNodes().size(); iNode++)
        {
            pNode = m_SimulationDescriptor.GetGraph()->GetListNodes()[iNode];
            std::map<Pattern*, std::map<Pattern*, AbstractPenalty*> >::iterator iter;
            for (iter = pNode->getMapPenalties().begin(); iter != pNode->getMapPenalties().end(); ++iter)
            {
                for (std::map<Pattern*, AbstractPenalty*>::iterator iterDown = iter->second.begin(); iterDown != iter->second.end(); ++iterDown)
                {
                    iterDown->second->fillMeasuredCostsForTravelTimesUpdatePeriod(iTravelTimesUpdatePeriodIndex, m_SimulationDescriptor.GetSubPopulationsByServiceType()[pNode->getParent()->GetServiceType()], mapCostFunctions);
                }
            }
        }
    }

    return bOk;
}

void TraficSimulators::PostProcessCosts(const std::map<SymuCore::SubPopulation*, SymuCore::CostFunction> & mapCostFunctions)
{
    Link * pLink;
    for (size_t iLink = 0; iLink < m_SimulationDescriptor.GetGraph()->GetListLinks().size(); iLink++)
    {
        pLink = m_SimulationDescriptor.GetGraph()->GetListLinks()[iLink];
        for (size_t iPattern = 0; iPattern < pLink->getListPatterns().size(); iPattern++)
        {
            Pattern * pPattern = pLink->getListPatterns()[iPattern];
            pPattern->postProcessCosts(m_SimulationDescriptor.GetSubPopulationsByServiceType()[pPattern->getLink()->getParent()->GetServiceType()], mapCostFunctions);
        }
    }

    Node * pNode;
    for (size_t iNode = 0; iNode < m_SimulationDescriptor.GetGraph()->GetListNodes().size(); iNode++)
    {
        pNode = m_SimulationDescriptor.GetGraph()->GetListNodes()[iNode];
        std::map<Pattern*, std::map<Pattern*, AbstractPenalty*> >::iterator iter;
        for (iter = pNode->getMapPenalties().begin(); iter != pNode->getMapPenalties().end(); ++iter)
        {
            for (std::map<Pattern*, AbstractPenalty*>::iterator iterDown = iter->second.begin(); iterDown != iter->second.end(); ++iterDown)
            {
                iterDown->second->postProcessCosts(m_SimulationDescriptor.GetSubPopulationsByServiceType()[pNode->getParent()->GetServiceType()], mapCostFunctions);
            }
        }
    }
}

bool TraficSimulators::ProcessPathsAssignment(const std::vector<SymuCore::Trip*> & assignedTrips, const boost::posix_time::time_duration & predictionPeriodDuration, const boost::posix_time::ptime & startTime, const boost::posix_time::ptime & endTime)
{
    bool bOk = true;
    for (size_t i = 0; i < m_lstTraficSimulators.size() && bOk; i++)
    {
        bOk = m_lstTraficSimulators[i]->ProcessPathsAssignment(assignedTrips, predictionPeriodDuration, startTime, endTime);
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

bool TraficSimulators::AssignmentPeriodStart(int iPeriod, bool bIsNotTheFinalIterationForPeriod)
{
    bool bOk = true;
    for (size_t i = 0; i < m_lstTraficSimulators.size() && bOk; i++)
    {
        bOk = m_lstTraficSimulators[i]->AssignmentPeriodStart(iPeriod, bIsNotTheFinalIterationForPeriod);
        if (!bOk)
        {
            BOOST_LOG_TRIVIAL(error) << "AssignmentPeriodStartFailed failed for simulator " << m_lstTraficSimulators[i]->GetSimulatorName();
        }
    }

    return bOk;
}

bool TraficSimulators::AssignmentPeriodEnded()
{
    bool bOk = true;
    for (size_t i = 0; i < m_lstTraficSimulators.size() && bOk; i++)
    {
        bOk = m_lstTraficSimulators[i]->AssignmentPeriodEnded();
        if (!bOk)
        {
            BOOST_LOG_TRIVIAL(error) << "AssignmentPeriodEnded failed for simulator " << m_lstTraficSimulators[i]->GetSimulatorName();
        }
    }

    return bOk;
}

bool TraficSimulators::AssignmentPeriodRollback()
{
    bool bOk = true;

    for (size_t i = 0; i < m_lstTraficSimulators.size() && bOk; i++)
    {
        bOk = m_lstTraficSimulators[i]->AssignmentPeriodRollback();
        if (!bOk)
        {
            BOOST_LOG_TRIVIAL(error) << "AssignmentPeriodRollback failed for simulator " << m_lstTraficSimulators[i]->GetSimulatorName();
        }
    }

    return bOk;
}

bool TraficSimulators::AssignmentPeriodCommit(bool bAssignmentPeriodEnded)
{
    bool bOk = true;
    for (size_t i = 0; i < m_lstTraficSimulators.size() && bOk; i++)
    {
        bOk = m_lstTraficSimulators[i]->AssignmentPeriodCommit(bAssignmentPeriodEnded);
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
