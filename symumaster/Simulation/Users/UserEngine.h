#pragma once

#ifndef _SYMUMASTER_USERENGINE_
#define _SYMUMASTER_USERENGINE_

#include "SymuMasterExports.h"

#include "TrackedUser.h"

#include <Users/IUserHandler.h>

#include <map>

namespace SymuCore {
    class Populations;
    class Population;
}

namespace boost {
    namespace posix_time {
        class ptime;
        class time_duration;
    }
}

#pragma warning( push )
#pragma warning( disable : 4251 )

namespace SymuMaster {

    class UserEngineSnapshot;
    class UserPathWriting;
    class SimulationConfiguration;

    class SYMUMASTER_EXPORTS_DLL_DEF UserEngine : public SymuCore::IUserHandler {

    public:
        UserEngine(int iInstance);

        virtual ~UserEngine();

        void setUserPathWriter(UserPathWriting *pUserPathWriter);
        UserPathWriting * GetUserPathWriter() const;

        /**
         * inherited from SymuCore::IUserHandler
         */
        virtual void OnPPathCompleted(int tripID, double dbNextPPathDepartureTimeFromCurrentTimeStep);

        void Initialize(const SimulationConfiguration & config, const std::vector<TraficSimulatorInstance*> & lstTraficSimulators);

        virtual bool Run(const SymuCore::Populations & populations, const boost::posix_time::ptime & startTimeStepTime, const boost::posix_time::time_duration & timeStepDuration);

        // handling commits/rollbacks
        virtual void AssignmentPeriodStart();
        virtual void AssignmentPeriodEnded();
        virtual void AssignmentPeriodRollback();
        virtual void AssignmentPeriodCommit();

    protected:

        TrackedUser * CreateTrackedUserFromTrip(SymuCore::Trip * pTrip) const;

        UserEngineSnapshot * TakeSnapshot();
        void RestoreSnapshot(UserEngineSnapshot* pSnapshot);

    protected:

        int m_iInstance;

        std::map<int, TrackedUser*> m_OwnedUsers;
        std::map<int, TrackedUser*> m_AllUsers;

        std::map<SymuCore::Population*, size_t> m_NextTripsToActivate;

        std::vector<TraficSimulatorInstance*>   m_lstTraficSimulators;
        TraficSimulatorInstance*                m_pWalkSimulator;

        boost::posix_time::ptime                m_dtCurrentTimeStepStartTime;

        std::vector<UserEngineSnapshot*>        m_lstUserEngineSnapshot;

        UserPathWriting *                       m_pUserPathWriter;
    };

}

#pragma warning( pop )

#endif // _SYMUMASTER_USERENGINE_
