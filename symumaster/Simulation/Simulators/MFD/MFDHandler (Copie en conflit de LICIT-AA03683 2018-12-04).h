#pragma once

#ifndef _SYMUMASTER_MFDHANDLER_
#define _SYMUMASTER_MFDHANDLER_

#include "SymuMasterExports.h"

#include "Simulation/Simulators/TraficSimulatorHandler.h"

#include <Graph/Model/Cost.h>

namespace matlab {
    namespace engine {
        class MATLABEngine;
    }
}

#pragma warning(push)
#pragma warning(disable : 4251)

namespace SymuMaster {

    class SYMUMASTER_EXPORTS_DLL_DEF MFDHandler : public TraficSimulatorHandler {

    public:
        MFDHandler(const SimulatorConfiguration * pConfig);

        virtual ~MFDHandler();

        virtual const std::string & GetSimulatorName() const;


        virtual bool Initialize(const SimulationConfiguration & config, SymuCore::IUserHandler * pUserHandler);
        virtual bool BuildSimulationGraph(SymuCore::MultiLayersGraph *pParentGraph);
        virtual bool Step(const boost::posix_time::ptime & startTimeStep, const boost::posix_time::time_duration & timeStepDuration);

        virtual SymuCore::Origin * GetParentOrigin(SymuCore::Origin * pChildOrigin);
        virtual SymuCore::Destination * GetParentDestination(SymuCore::Destination * pChildDestination);

        virtual bool FillTripsDescriptor(SymuCore::MultiLayersGraph * pGraph, SymuCore::Populations & populations,
            const boost::posix_time::ptime & startSimulationTime, const boost::posix_time::ptime & endSimulationTime, bool bIgnoreSubAreas,
            std::vector<SymuCore::Trip> & lstTrips);

        virtual bool ComputeCosts(int iTravelTimesUpdatePeriodIndex, boost::posix_time::ptime startTravelTimesUpdatePeriodTime, boost::posix_time::ptime endTravelTimesUpdatePeriodTime, const std::map<SymuCore::SubPopulation*, SymuCore::CostFunction> & mapCostFunctions);

        virtual bool ActivatePPath(const PPath & currentPPath, const boost::posix_time::ptime & startTimeStepTime, const boost::posix_time::ptime & departureTime);

        virtual bool Terminate();

        virtual bool AssignmentPeriodStart(int iPeriod, bool bIsNotTheFinalIterationForPeriod);
        virtual bool AssignmentPeriodEnded();
        virtual bool AssignmentPeriodRollback();
        virtual bool AssignmentPeriodCommit(bool bAssignmentPeriodEnded);

        virtual bool IsExtraIterationToProduceOutputsRelevant();

        virtual bool ComputeNewPeriodCosts(const std::map<SymuCore::SubPopulation*, SymuCore::CostFunction> & mapCostFunctions);

        virtual bool ProcessPathsAssignment(const std::vector<SymuCore::Trip*> & assignedTrips, const boost::posix_time::time_duration & predictionPeriodDuration, const boost::posix_time::ptime & startTime, const boost::posix_time::ptime & endTime);

    private:
        void updateMapCosts(const std::map<SymuCore::SubPopulation*, SymuCore::CostFunction> & mapCostFunctions);

    private:
        std::unique_ptr<matlab::engine::MATLABEngine> m_pMatlabEngine;

        std::map<int, std::map<SymuCore::CostFunction, SymuCore::Cost> > m_mapCostByRoute;

        bool m_bHasStartPeriodSnapshot;

        SymuCore::IUserHandler * m_pUserHandler;

        bool m_bAccBased;
    };

}

#pragma warning(pop)

#endif // _SYMUMASTER_MFDHANDLER_
