#pragma once

#ifndef _SYMUMASTER_PPATH_
#define _SYMUMASTER_PPATH_

#include "SymuMasterExports.h"

#include <Utils/SymuCoreConstants.h>

#include <boost/date_time/posix_time/ptime.hpp>

#include <cstddef>

namespace SymuCore {
    class Trip;
}

#pragma warning( push )
#pragma warning( disable : 4251 )

namespace SymuMaster {

    class TraficSimulatorInstance;

    class SYMUMASTER_EXPORTS_DLL_DEF PPath {

    public:
        PPath();
        PPath(SymuCore::Trip * pTrip, SymuCore::ServiceType serviceType, size_t iFirstPatternIndex, TraficSimulatorInstance * pPPathSimulator);

        virtual ~PPath();

        const SymuCore::Trip * GetTrip() const;
        SymuCore::ServiceType GetServiceType() const;
        TraficSimulatorInstance * GetSimulator() const;
        size_t GetFirstPatternIndex() const;
        size_t GetLastPatternIndex() const;

        void SetLastPatternIndex(size_t iLastPatternIndex);

        bool operator==(const PPath & otherPath) const;

        const boost::posix_time::ptime & GetArrivalTime() const;
        void SetArrivalTime(const boost::posix_time::ptime &dtArrivalTime);

    protected:
        SymuCore::Trip * m_pTrip;
        SymuCore::ServiceType m_eServiceType;
        size_t m_iFirstPatternIndex;
        size_t m_iLastPatternIndex;
        boost::posix_time::ptime m_dtArrivalTime;

        TraficSimulatorInstance * m_pSimulator;
    };

}

#pragma warning( pop )

#endif // _SYMUMASTER_PPATH_
