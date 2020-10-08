#pragma once 

#ifndef _SYMUMASTER_TRAFICSIMULATORHANDLER_
#define _SYMUMASTER_TRAFICSIMULATORHANDLER_

#include "SymuMasterExports.h"

#include "Simulation/Simulators/SimulationDescriptor.h"

#include <boost/date_time/posix_time/posix_time_types.hpp>

#include <vector>
#include <map>

#pragma warning( push )  
#pragma warning( disable : 4251 )  

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
    class Node;
}

namespace SymuMaster {

    class SimulatorConfiguration;
    class SimulationConfiguration;
    class SimulationDescriptor;
    class TraficSimulatorInstance;
    class PPath;
    class UserEngines;
    class TraficSimulators;
    class AbstractSimulatorInterface;

    class SYMUMASTER_EXPORTS_DLL_DEF TraficSimulatorHandler {

    public:
        TraficSimulatorHandler(const SimulatorConfiguration * pConfig, TraficSimulators * pParentTraficSimulators);

        virtual ~TraficSimulatorHandler();

        virtual const std::string & GetSimulatorName() const;

        virtual bool Initialize(const SimulationConfiguration & config, UserEngines & userEngines, TraficSimulatorHandler * pMasterSimulator = NULL);
        virtual bool BuildSimulationGraph(SymuCore::MultiLayersGraph *pParentGraph, SymuCore::MultiLayersGraph *pSimulatorGraph);
        virtual bool Step(const boost::posix_time::ptime & startTimeStep, const boost::posix_time::time_duration & timeStepDuration, int iInstance);

        virtual SymuCore::Origin * GetParentOrigin(SymuCore::Origin * pChildOrigin);
        virtual SymuCore::Destination * GetParentDestination(SymuCore::Destination * pChildDestination);

        virtual bool FillTripsDescriptor(SymuCore::MultiLayersGraph * pGraph, SymuCore::Populations & populations,
            const boost::posix_time::ptime & startSimulationTime, const boost::posix_time::ptime & endSimulationTim, bool bIgnoreSubArease,
            std::vector<SymuCore::Trip> & lstTrips);

        virtual bool ComputeCosts(int iTravelTimesUpdatePeriodIndex, boost::posix_time::ptime startTravelTimesUpdatePeriodTime, boost::posix_time::ptime endTravelTimesUpdatePeriodTime, const std::map<SymuCore::SubPopulation*, SymuCore::CostFunction> & mapCostFunctions, int iInstance);

        virtual bool Terminate();

        virtual bool AssignmentPeriodStart(int iPeriod, bool bIsNotTheFinalIterationForPeriod, int nbMaxInstancesNb);
        virtual bool AssignmentPeriodEnded(int iInstance);
        virtual bool AssignmentPeriodRollback(int nbMaxInstancesNb);
        virtual bool AssignmentPeriodCommit(int nbMaxInstancesNb, bool bAssignmentPeriodEnded);

        virtual bool IsExtraIterationToProduceOutputsRelevant();

        virtual bool ComputeNewPeriodCosts(const std::map<SymuCore::SubPopulation*, SymuCore::CostFunction> & mapCostFunctions);

        virtual bool ProcessPathsAssignment(const std::vector<SymuCore::Trip*> & assignedTrips, const boost::posix_time::time_duration & predictionPeriodDuration, const boost::posix_time::ptime & startTime, const boost::posix_time::ptime & endTime, int iSimulationInstance);

        SimulationDescriptor & GetSimulationDescriptor();
        const SimulationDescriptor & GetSimulationDescriptor() const;

        const SimulatorConfiguration * GetSimulatorConfiguration() const;

        virtual void AddSimulatorInstance(TraficSimulatorInstance * pSimInstance);

        const std::vector<TraficSimulatorInstance*> & GetSimulatorInstances() const;

        AbstractSimulatorInterface* GetOrCreateInterface(const std::string & strName, bool bIsUpstreamInterface);
        bool CheckInterfaces(const std::map<TraficSimulatorHandler*, std::map<TraficSimulatorHandler*, std::vector<SimulatorsInterface> > > & interfaces);
        bool GetOffers(int iInstance, const std::vector<const AbstractSimulatorInterface*> & interfacesToGetOffer, std::map<const AbstractSimulatorInterface*, double> & offerValues);
        bool SetCapacities(int iInstance, const std::vector<const AbstractSimulatorInterface*> & upstreamInterfaces, const AbstractSimulatorInterface* pDownstreamInterface, double dbCapacityValue);

    protected:
        const SimulatorConfiguration * m_pConfig;

        TraficSimulators * m_pParentTraficSimulators;

        SimulationDescriptor m_SimulationDescriptor;

        std::vector<TraficSimulatorInstance*> m_lstInstances;
    };

}

#pragma warning( pop )  

#endif // _SYMUMASTER_TRAFICSIMULATORHANDLER_
