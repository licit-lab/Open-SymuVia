#pragma once 

#ifndef _SYMUMASTER_TRAFICSIMULATORHANDLER_
#define _SYMUMASTER_TRAFICSIMULATORHANDLER_

#include "SymuMasterExports.h"

#include "Simulation/Simulators/SimulationDescriptor.h"

#include <boost/date_time/posix_time/posix_time_types.hpp>

#include <vector>
#include <map>

namespace SymuCore {
    class Pattern;
    class AbstractPenalty;
    class Trip;
    class MultiLayersGraph;
    class Populations;
    class Populations;
    class Origin;
    class Destination;
    class IUserHandler;
}

namespace SymuMaster {

    class SimulatorConfiguration;
    class SimulationConfiguration;
    class SimulationDescriptor;
    class PPath;

    class SYMUMASTER_EXPORTS_DLL_DEF TraficSimulatorHandler {

    public:
        TraficSimulatorHandler(const SimulatorConfiguration * pConfig);

        virtual ~TraficSimulatorHandler();

        virtual const std::string & GetSimulatorName() const = 0;

        virtual bool Initialize(const SimulationConfiguration & config, SymuCore::IUserHandler * pUserHandler) = 0;
        virtual bool BuildSimulationGraph(SymuCore::MultiLayersGraph *pParentGraph) = 0;
        virtual bool Step(const boost::posix_time::ptime & startTimeStep, const boost::posix_time::time_duration & timeStepDuration) = 0;

        virtual SymuCore::Origin * GetParentOrigin(SymuCore::Origin * pChildOrigin) = 0;
        virtual SymuCore::Destination * GetParentDestination(SymuCore::Destination * pChildDestination) = 0;

        virtual bool FillTripsDescriptor(SymuCore::MultiLayersGraph * pGraph, SymuCore::Populations & populations,
            const boost::posix_time::ptime & startSimulationTime, const boost::posix_time::ptime & endSimulationTim, bool bIgnoreSubArease,
            std::vector<SymuCore::Trip> & lstTrips) = 0;

        virtual bool ComputeCosts(int iTravelTimesUpdatePeriodIndex, boost::posix_time::ptime startTravelTimesUpdatePeriodTime, boost::posix_time::ptime endTravelTimesUpdatePeriodTime, const std::map<SymuCore::SubPopulation*, SymuCore::CostFunction> & mapCostFunctions) = 0;

        virtual bool ActivatePPath(const PPath & currentPPath, const boost::posix_time::ptime & startTimeStepTime, const boost::posix_time::ptime & departureTime) = 0;

        virtual bool Terminate() = 0;

        virtual bool AssignmentPeriodStart(int iPeriod, bool bIsNotTheFinalIterationForPeriod) = 0;
        virtual bool AssignmentPeriodEnded() = 0;
        virtual bool AssignmentPeriodRollback() = 0;
        virtual bool AssignmentPeriodCommit(bool bAssignmentPeriodEnded) = 0;

        virtual bool IsExtraIterationToProduceOutputsRelevant() = 0;

        virtual bool ComputeNewPeriodCosts(const std::map<SymuCore::SubPopulation*, SymuCore::CostFunction> & mapCostFunctions) { return true; }

        virtual bool ProcessPathsAssignment(const std::vector<SymuCore::Trip*> & assignedTrips, const boost::posix_time::time_duration & predictionPeriodDuration, const boost::posix_time::ptime & startTime, const boost::posix_time::ptime & endTime) { return true; }

        SimulationDescriptor & GetSimulationDescriptor();
        const SimulationDescriptor & GetSimulationDescriptor() const;

        const SimulatorConfiguration * GetSimulatorConfiguration() const;

    protected:
        const SimulatorConfiguration * m_pConfig;

        SimulationDescriptor m_SimulationDescriptor;

    };

}

#endif // _SYMUMASTER_TRAFICSIMULATORHANDLER_
