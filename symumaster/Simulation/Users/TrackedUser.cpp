#include "TrackedUser.h"

#include "PPath.h"

#include <Demand/Trip.h>

using namespace SymuMaster;

TrackedUser::TrackedUser()
{
    m_pTrip = nullptr;
}

TrackedUser::TrackedUser(SymuCore::Trip * pTrip, const std::vector<PPath> & subPaths)
{
    m_pTrip = pTrip;
    m_PPaths = subPaths;
    m_iCurrentPPath = 0;
    m_dtCurrentPPathDepartureTime = pTrip->GetDepartureTime();
}

TrackedUser::~TrackedUser()
{
}

const PPath & TrackedUser::GetCurrentPPath() const
{
    return m_PPaths[m_iCurrentPPath];
}

const boost::posix_time::ptime & TrackedUser::GetCurrentPPathDepartureTime() const
{
    return m_dtCurrentPPathDepartureTime;
}

std::vector<PPath>& TrackedUser::GetListPPaths()
{
    return m_PPaths;
}

const SymuCore::Trip * TrackedUser::GetTrip() const
{
    return m_pTrip;
}

SymuCore::Trip * TrackedUser::GetTrip()
{
    return m_pTrip;
}

bool TrackedUser::GoToNextPPath(const boost::posix_time::ptime & nextPPathDepartureTime)
{
    m_PPaths[m_iCurrentPPath].SetArrivalTime(nextPPathDepartureTime);
    m_iCurrentPPath++;
    m_dtCurrentPPathDepartureTime = nextPPathDepartureTime;
    return m_iCurrentPPath < m_PPaths.size();
}


