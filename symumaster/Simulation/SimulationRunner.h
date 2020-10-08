#pragma once

#ifndef _SYMUMASTER_SIMULATIONRUNNER_
#define _SYMUMASTER_SIMULATIONRUNNER_

#include "SymuMasterExports.h"

#include "Simulators/TraficSimulators.h"
#include "Users/UserEngines.h"

#include <vector>

#pragma warning( push )
#pragma warning( disable : 4251 )

namespace SymuCore {
    class Origin;
}

namespace SymuMaster {

    class SimulationConfiguration;
    class AssignmentModel;
    class DemandFileHandler;
	class CostsWriting;
	class UserPathWritings;

	/**
	 The SimulationRunner objects handle a simulation run. A SimulationRunner can be run just once : please instanciate a 
	 new SimulationRunner object for each simulation run.
	 */
	class SYMUMASTER_EXPORTS_DLL_DEF SimulationRunner {

    public:
        SimulationRunner(const SimulationConfiguration &config);

        virtual ~SimulationRunner();

        virtual bool Run();

        void SetAssignmentModel(AssignmentModel *pAssignmentModel);

		virtual bool Initialize();

		bool RunNextAssignmentPeriod(bool & bEnd);

		bool Terminate();

		// External simulation control methods :
		bool ForbidSymuviaLinks(const std::vector<std::string> & linkNames);

    protected:

        virtual bool BuildSimulationGraph();

        virtual bool LoadDemand(DemandFileHandler& demandFileHandler);

        virtual bool Execute();

		bool IsEndReached();

        void SortTrip(const SymuCore::Populations &listPopulations, boost::posix_time::ptime startTime, boost::posix_time::time_duration assignmentPeriod,
            int nbPeriods, boost::posix_time::time_duration predictionPeriod,
            std::vector<std::vector<SymuCore::Trip*> > & listResultTrips,
            std::vector<SymuCore::Trip*> & listResultTripsNoOptimizer);

        bool AssignAllToShortestPath(const std::vector<SymuCore::Trip*> & trips, int nbInstances) const;

    protected:

        const SimulationConfiguration & m_Config;

        TraficSimulators m_Simulators;

        AssignmentModel* m_pAssignmentModel;

        UserEngines m_UserEngines;

		// Working variables
		int m_iAssignmentPeriodIndex;
		int m_nbAssignmentPeriod;
		int m_nbTravelTimeUpdatePeriodsByPeriod;
		boost::posix_time::ptime m_currentTime;
		std::vector<std::vector<SymuCore::Trip*> > m_listSortedTrips;
		std::map<SymuCore::SubPopulation*, SymuCore::CostFunction> m_mapCostFunctions;
		bool m_bRedoLastIteration;
		CostsWriting * m_pCostsWriting;
		UserPathWritings * m_pTripOutputWritings;
    };

}

#pragma warning( pop )

#endif // _SYMUMASTER_SIMULATIONRUNNER_
