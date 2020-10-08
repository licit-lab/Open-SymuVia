#pragma once

#ifndef _SYMUMASTER_SYMUVIAINSTANCE_
#define _SYMUMASTER_SYMUVIAINSTANCE_

#include "SymuMasterExports.h"

#include "Simulation/Simulators/TraficSimulatorInstance.h"

namespace SymuMaster {

    class SYMUMASTER_EXPORTS_DLL_DEF SymuViaInstance : public TraficSimulatorInstance {

    public:
        SymuViaInstance(TraficSimulatorHandler * pParentHandler, int iInstance);

        virtual ~SymuViaInstance();

        virtual const std::string & GetSimulatorName() const;

        virtual bool Initialize(const SimulationConfiguration & config, SymuCore::IUserHandler * pUserHandler, TraficSimulatorHandler * pMasterSimulator = NULL);
        virtual bool BuildSimulationGraph(SymuCore::MultiLayersGraph *pParentGraph, SymuCore::MultiLayersGraph *pSimulatorGraph, bool bIsPrimaryGraph);
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

        virtual bool GetOffers(const std::vector<const AbstractSimulatorInterface*> & interfacesToGetOffer, std::map<const AbstractSimulatorInterface*, double> & offerValues);
        virtual bool SetCapacities(const std::vector<const AbstractSimulatorInterface*> & upstreamInterfaces, const AbstractSimulatorInterface* pDownstreamInterface, double dbCapacityValue);

		bool ForbidLinks(const std::vector<std::string> & linkNames);

    protected:
        virtual AbstractSimulatorInterface* MakeInterface(const std::string & strName, bool bIsUpstreamInterface);

        bool ActivateDrivingPPath(const PPath & currentPPath, const boost::posix_time::ptime & startTimeStepTime, const boost::posix_time::ptime & departureTime);
        bool ActivatePublicTransportPPath(const PPath & currentPPath, const boost::posix_time::ptime & startTimeStepTime, const boost::posix_time::ptime & departureTime);

    protected:

        int m_iNetworkID;

        bool m_bHasStartPeriodSnapshot;

        bool m_bHasTrajectoriesOutput;
    };

}

#endif // _SYMUMASTER_SYMUVIAINSTANCE_
