#pragma once

#ifndef _SYMUMASTER_TRACKEDUSER_
#define _SYMUMASTER_TRACKEDUSER_

#include "SymuMasterExports.h"

#include "PPath.h"

#include <vector>

#include <boost/date_time/posix_time/ptime.hpp>

namespace SymuCore {
    class Trip;
}

#pragma warning( push )
#pragma warning( disable : 4251 )

namespace SymuMaster {

    class SYMUMASTER_EXPORTS_DLL_DEF TrackedUser {

    public:
        TrackedUser();
        TrackedUser(SymuCore::Trip * pTrip, const std::vector<PPath> & subPaths);

        virtual ~TrackedUser();

        const PPath & GetCurrentPPath() const;
        bool GoToNextPPath(const boost::posix_time::ptime & nextPPathDepartureTime);

        const SymuCore::Trip * GetTrip() const;
        SymuCore::Trip * GetTrip();

        const boost::posix_time::ptime & GetCurrentPPathDepartureTime() const;

        std::vector<PPath>& GetListPPaths();

    protected:
        SymuCore::Trip * m_pTrip;

        // list of the Subpaths of the trip
        std::vector<PPath> m_PPaths;

        // current position of the user in the trip
        size_t m_iCurrentPPath;
        boost::posix_time::ptime m_dtCurrentPPathDepartureTime;
    };

}

#pragma warning( pop )

#endif // _SYMUMASTER_TRACKEDUSER_
