#pragma once 

#ifndef _SYMUMASTER_WALKSIMULATOR_
#define _SYMUMASTER_WALKSIMULATOR_

#include "Simulation/Simulators/TraficSimulatorHandler.h"

namespace SymuMaster {

    class SYMUMASTER_EXPORTS_DLL_DEF WalkSimulator : public TraficSimulatorHandler {

    public:
        WalkSimulator(const SimulatorConfiguration * pConfig);

        virtual ~WalkSimulator();

        virtual const std::string & GetSimulatorName() const;

        virtual bool Initialize(const SimulationConfiguration & config, SymuCore::IUserHandler * pUserHandler);
        virtual bool BuildSimulationGraph(SymuCore::MultiLayersGraph *pParentGraph);
        virtual bool Step(const boost::posix_time::ptime & startTimeStep, const boost::posix_time::time_duration & timeStepDuration);

        virtual SymuCore::Origin * GetParentOrigin(SymuCore::Origin * pChildOrigin);
        virtual SymuCore::Destination * GetParentDestination(SymuCore::Destination * pChildDestination);

        virtual bool FillTripsDescriptor(SymuCore::MultiLayersGraph * pGraph, SymuCore::Populations & populations,
            const boost::posix_time::ptime & startSimulationTime, const boost::posix_time::ptime & endSimulationTim, bool bIgnoreSubArease,
            std::vector<SymuCore::Trip> & lstTrips);

        virtual bool ComputeCosts(int iTravelTimesUpdatePeriodIndex, boost::posix_time::ptime startTravelTimesUpdatePeriodTime, boost::posix_time::ptime endTravelTimesUpdatePeriodTime, const std::map<SymuCore::SubPopulation*, SymuCore::CostFunction> & mapCostFunctions);

        virtual bool ActivatePPath(const PPath & currentPPath, const boost::posix_time::ptime & startTimeStepTime, const boost::posix_time::ptime & departureTime);

        virtual bool Terminate();

        virtual bool AssignmentPeriodStart(int iPeriod, bool bIsNotTheFinalIterationForPeriod);
        virtual bool AssignmentPeriodEnded();
        virtual bool AssignmentPeriodRollback();
        virtual bool AssignmentPeriodCommit(bool bAssignmentPeriodEnded);

        virtual bool IsExtraIterationToProduceOutputsRelevant();

    protected:
        SymuCore::IUserHandler * m_pUserHandler;
    };

}

#endif // _SYMUMASTER_WALKSIMULATOR_
