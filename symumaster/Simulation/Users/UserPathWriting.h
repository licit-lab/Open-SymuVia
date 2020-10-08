#pragma once

#ifndef _SYMUMASTER_UserPathWriting_
#define _SYMUMASTER_UserPathWriting_

#include "SymuMasterExports.h"
#include "Demand/Trip.h"

#include <vector>

#include <boost/date_time/posix_time/ptime.hpp>

namespace SymuCore {
    class Trip;
}

#pragma warning( push )
#pragma warning( disable : 4251 )

namespace SymuMaster {

    class SimulationConfiguration;
    class UserEngine;
    class PPath;

    class SYMUMASTER_EXPORTS_DLL_DEF UserPathWriting {

	public:
		UserPathWriting();
        UserPathWriting(const SimulationConfiguration &config, UserEngine *pUserEngine);

        virtual ~UserPathWriting();

        bool WriteUserPaths(int iPeriod, int iIteration, int iInstanceOffset, int iInstance, const boost::posix_time::ptime &startTimeSimulation, const boost::posix_time::ptime &startTimePeriod, const boost::posix_time::time_duration & timeAssignmentPeriodDuration, const boost::posix_time::time_duration & timePredictionPeriodDuration, bool isFinalFile);

        void SetPPaths(SymuCore::Trip* pTrip, const std::vector<PPath>& newListPPaths);

        void CompletedPPath(const boost::posix_time::ptime & nextPPathDepartureTime, SymuCore::Trip* pTrip, const PPath &pPPath);

        // handling commits/rollbacks
        void AssignmentPeriodStart();
        void AssignmentPeriodEnded();
        void AssignmentPeriodRollback();
        void AssignmentPeriodCommit();

    private:

        struct TripCompare
        {
            bool operator()(const SymuCore::Trip* p1, const SymuCore::Trip* p2 ) const
            {
                if(p1->GetDepartureTime() != p2->GetDepartureTime())
                    return p1->GetDepartureTime() < p2->GetDepartureTime();

                return p1->GetID() < p2->GetID();
            }
        };

        std::string                         m_strOutpuDirectoryPath;

        std::string                         m_strOutputPrefix;

        std::map<SymuCore::Trip*, std::vector<PPath>, TripCompare >        m_lstAllPPath;

        std::vector<std::map<SymuCore::Trip*, std::vector<PPath>, TripCompare > >    m_lstUserPathWritingSnapshots;

    };

}

#pragma warning( pop )

#endif // _SYMUMASTER_UserPathWriting_
