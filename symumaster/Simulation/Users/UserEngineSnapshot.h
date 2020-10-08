#pragma once

#ifndef _SYMUMASTER_USERENGINE_SNAPSHOT_
#define _SYMUMASTER_USERENGINE_SNAPSHOT_

#include "SymuMasterExports.h"

#include "TrackedUser.h"

#include <vector>

namespace SymuCore {
    class Population;
}

#pragma warning( push )
#pragma warning( disable : 4251 )

namespace SymuMaster {

    class SYMUMASTER_EXPORTS_DLL_DEF UserEngineSnapshot {

    public:
        UserEngineSnapshot();

        virtual ~UserEngineSnapshot();

        std::vector<int> & GetOwnedUsers();
        std::vector<TrackedUser> & GetAllUsers();
        std::map<SymuCore::Population*, size_t> & GetNextTripsToActivate();

    protected:
        std::vector<int> m_OwnedUsers;
        std::vector<TrackedUser> m_AllUsers;
        std::map<SymuCore::Population*, size_t> m_NextTripsToActivate;
    };

}

#pragma warning( pop )

#endif // _SYMUMASTER_USERENGINE_SNAPSHOT_
