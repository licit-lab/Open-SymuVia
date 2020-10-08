#include "WalkSimulator.h"

#include "Simulation/Users/PPath.h"

#include <Users/IUserHandler.h>
#include <Demand/Trip.h>
#include <Graph/Model/Pattern.h>
#include <Graph/Model/Cost.h>
#include <Graph/Model/MultiLayersGraph.h>

using namespace SymuMaster;
using namespace SymuCore;

WalkSimulator::WalkSimulator(const SimulatorConfiguration * pConfig, int iInstance) :
TraficSimulatorInstance(NULL, iInstance)
{
}

WalkSimulator::~WalkSimulator()
{
}

const std::string & WalkSimulator::GetSimulatorName() const
{
    static const std::string walkSimulatorName = "Walk Simulator";
    return walkSimulatorName;
}

bool WalkSimulator::Initialize(const SimulationConfiguration & config, IUserHandler * pUserHandler, TraficSimulatorHandler * pMasterSimulator)
{
    m_pUserHandler = pUserHandler;
    return true;
}

bool WalkSimulator::BuildSimulationGraph(SymuCore::MultiLayersGraph *pParentGraph, SymuCore::MultiLayersGraph *pSimulatorGraph, bool bIsPrimaryGraph)
{
    assert(false); // should not go there...
    return false;
}

bool WalkSimulator::Step(const boost::posix_time::ptime & startTimeStep, const boost::posix_time::time_duration & timeStepDuration)
{
    assert(false);
    return false;
}

Origin * WalkSimulator::GetParentOrigin(Origin * pChildOrigin)
{
    assert(false); // should not go there...
    return NULL;
}

Destination * WalkSimulator::GetParentDestination(Destination * pChildDestination)
{
    assert(false); // should not go there...
    return NULL;
}

bool WalkSimulator::FillTripsDescriptor(MultiLayersGraph * pGraph, Populations & populations,
    const boost::posix_time::ptime & startSimulationTime, const boost::posix_time::ptime & endSimulationTim, bool bIgnoreSubArease,
    std::vector<SymuCore::Trip> & lstTrips)
{
    assert(false); // should not go there...
    return false;
}

bool WalkSimulator::ComputeCosts(int iTravelTimesUpdatePeriodIndex, boost::posix_time::ptime startTravelTimesUpdatePeriodTime, boost::posix_time::ptime endTravelTimesUpdatePeriodTime, const std::map<SymuCore::SubPopulation*, SymuCore::CostFunction> & mapCostFunctions)
{
    assert(false); // should not go there...
    return false;
}

bool WalkSimulator::ActivatePPath(const PPath & currentPPath, const boost::posix_time::ptime & startTimeStepTime, const boost::posix_time::ptime & departureTime)
{
    // compute the total walk time ot the PPath (should be walk only) :
    Cost totalWalkCost;
    const Path & tripPath = currentPPath.GetTrip()->GetPath(m_iInstance);
    Pattern * pPrevPattern = NULL;
    for (size_t iPattern = currentPPath.GetFirstPatternIndex(); iPattern <= currentPPath.GetLastPatternIndex(); iPattern++)
    {
        Pattern * pPattern = tripPath.GetlistPattern()[iPattern];
        // rmq. : assuming no penalties inter-walking patterns, and constant walk pattern costs.
        totalWalkCost.plus(pPattern->getPatternCost(0, 0, currentPPath.GetTrip()->GetSubPopulation()));
    }

    // the release time is then :
    boost::posix_time::ptime releaseTime = departureTime + boost::posix_time::microseconds((int64_t)round(totalWalkCost.getTravelTime()*1000000.0));

    double dbEndWalkTimeFromStartTimeStep = (double)((releaseTime - startTimeStepTime).total_microseconds()) / 1000000.0;
    m_pUserHandler->OnPPathCompleted(currentPPath.GetTrip()->GetID(), dbEndWalkTimeFromStartTimeStep);

    return true;
}

bool WalkSimulator::Terminate()
{
    assert(false); // should not go there...
    return true;
}

bool WalkSimulator::AssignmentPeriodStart(int iPeriod, bool bIsNotTheFinalIterationForPeriod)
{
    assert(false); // should not go there...
    return true;
}

bool WalkSimulator::AssignmentPeriodEnded()
{
    assert(false); // should not go there...
    return true;
}

bool WalkSimulator::AssignmentPeriodRollback()
{
    assert(false); // should not go there...
    return true;
}

bool WalkSimulator::AssignmentPeriodCommit(bool bAssignmentPeriodEnded)
{
    assert(false); // should not go there...
    return true;
}

bool WalkSimulator::IsExtraIterationToProduceOutputsRelevant()
{
    assert(false); // should not go there...
    return false;
}


