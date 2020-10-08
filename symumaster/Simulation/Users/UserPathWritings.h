#pragma once

#ifndef _SYMUMASTER_UserPathWritings_
#define _SYMUMASTER_UserPathWritings_

#include "SymuMasterExports.h"

#include <vector>

#include <boost/date_time/posix_time/ptime.hpp>

#pragma warning( push )
#pragma warning( disable : 4251 )

namespace SymuMaster {

    class UserEngines;
    class UserPathWriting;
    class SimulationConfiguration;

    class SYMUMASTER_EXPORTS_DLL_DEF UserPathWritings {

	public:
        UserPathWritings();
        UserPathWritings(const SimulationConfiguration & config, const UserEngines & userEngines);

        virtual ~UserPathWritings();

        bool WriteUserPaths(int iPeriod, int iIteration, int iInstanceOffset, const boost::posix_time::ptime &startTimeSimulation, const boost::posix_time::ptime &startTimePeriod, const boost::posix_time::time_duration & timeAssignmentPeriodDuration, const boost::posix_time::time_duration & timePredictionPeriodDuration, bool isFinalFile);

        // handling commits/rollbacks
        void AssignmentPeriodStart(int nbMaxInstancesNb);
        void AssignmentPeriodRollback(int nbMaxInstancesNb);
        void AssignmentPeriodCommit();

    private:

        std::vector<UserPathWriting*> m_lstUserPathWritings;

    };

}

#pragma warning( pop )

#endif // _SYMUMASTER_UserPathWritings_
