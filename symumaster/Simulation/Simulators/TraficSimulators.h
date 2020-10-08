#pragma once

#ifndef _SYMUMASTER_TRAFICSIMULATORS_
#define _SYMUMASTER_TRAFICSIMULATORS_

#include "TraficSimulatorHandler.h"

#include "SimulatorsInterface.h"

#include "SymuMasterExports.h"

#include <vector>
#include <map>

#pragma warning( push )  
#pragma warning( disable : 4251 )  

namespace SymuCore {
    class Node;
}

namespace SymuMaster {

    class TraficSimulatorHandler;
    class UserEngine;

    class SYMUMASTER_EXPORTS_DLL_DEF TraficSimulators : public TraficSimulatorHandler {

    public:
        TraficSimulators();

        virtual ~TraficSimulators();

        virtual const std::string & GetSimulatorName() const;

        virtual void AddSimulatorHandler(TraficSimulatorHandler * pSimulatorhandler);

        virtual bool Empty() const;

        const std::vector<TraficSimulatorHandler*> & GetSimulators() const;

        /////////////////////////////////////////////////////////////
        /// TraficSimulatorHandler superclass implementation
        /////////////////////////////////////////////////////////////

        virtual bool Initialize(const SimulationConfiguration & config, UserEngines & userEngines, TraficSimulatorHandler * pMasterSimulator = NULL);
        virtual bool BuildSimulationGraph(SymuCore::MultiLayersGraph *pParentGraph, SymuCore::MultiLayersGraph *pSimulatorGraph);
        bool Step(const boost::posix_time::ptime & startTimeStep, const boost::posix_time::time_duration & timeStepDuration, int iInstance, UserEngine * pUserEngine);

        virtual SymuCore::Origin * GetParentOrigin(SymuCore::Origin * pChildOrigin);
        virtual SymuCore::Destination * GetParentDestination(SymuCore::Destination * pChildDestination);

        virtual bool FillTripsDescriptor(SymuCore::MultiLayersGraph * pGraph, SymuCore::Populations & populations,
            const boost::posix_time::ptime & startSimulationTime, const boost::posix_time::ptime & endSimulationTime, bool bIgnoreSubAreas,
            std::vector<SymuCore::Trip> & lstTrips);

        virtual bool ComputeCosts(int iTravelTimesUpdatePeriodIndex, boost::posix_time::ptime startTravelTimesUpdatePeriodTime, boost::posix_time::ptime endTravelTimesUpdatePeriodTime, const std::map<SymuCore::SubPopulation*, SymuCore::CostFunction> & mapCostFunctions, int iInstance);

        virtual bool ActivatePPath(const PPath & currentPPath, const boost::posix_time::ptime & startTimeStepTime, const boost::posix_time::ptime & departureTime);

        virtual bool Terminate();

        virtual bool AssignmentPeriodStart(int iPeriod, bool bIsNotTheFinalIterationForPeriod, int nbMaxInstancesNb);
        virtual bool AssignmentPeriodEnded(int iInstance);
        virtual bool AssignmentPeriodRollback(int nbMaxInstancesNb);
        virtual bool AssignmentPeriodCommit(int nbMaxInstancesNb, bool bAssignmentPeriodEnded);

        virtual bool IsExtraIterationToProduceOutputsRelevant();

        virtual bool ProcessPathsAssignment(const std::vector<SymuCore::Trip*> & assignedTrips, const boost::posix_time::time_duration & predictionPeriodDuration, const boost::posix_time::ptime & startTime, const boost::posix_time::ptime & endTime, int iSimulationInstance);

        /////////////////////////////////////////////////////////////
        /// End of TraficSimulatorHandler superclass implementation
        /////////////////////////////////////////////////////////////

        virtual bool FillEstimatedCostsForNewPeriod(const boost::posix_time::ptime & startSimulationTime, const boost::posix_time::ptime & startPeriodTime, const boost::posix_time::ptime & endPeriodTime, const boost::posix_time::time_duration & travelTimesUpdatePeriod, const std::map<SymuCore::SubPopulation*, SymuCore::CostFunction> & mapCostFunctions, int nbInstancesNumber);

        virtual void PostProcessCosts(int iSimulationInstance, const std::map<SymuCore::SubPopulation*, SymuCore::CostFunction> & mapCostFunctions);
        virtual void MergeCostsForSecondaryInstance(int iInstance);

        const SymuCore::Populations& GetPopulations() const;
        SymuCore::Populations& GetPopulations();

        void FillMapIndexedServiceTypes();

        void PostFillDemand();
        void MergeRelatedODs();

        const std::vector<SymuCore::MultiLayersGraph*> & GetSecondaryGraph() const { return m_lstSecondaryGraphs; }

        void SetSimulationConfig(const SimulationConfiguration * pSimulationConfig);

    private:
        bool UpdateInterfacesCapacities(TraficSimulatorHandler * pDownstreamSimulator, TraficSimulatorHandler * pUpstreamSimulator, int iInstance);

    protected:

        std::vector<TraficSimulatorHandler*> m_lstTraficSimulators;

        std::vector<SymuCore::MultiLayersGraph*> m_lstSecondaryGraphs;

        const SimulationConfiguration * m_pSimulationConfig;

        boost::posix_time::time_duration m_minTimeStep;
        TraficSimulatorHandler * m_pMasterSimulator;
    };

}

#pragma warning( pop ) 

#endif // _SYMUMASTER_SIMULATIONDESCRIPTOR_
