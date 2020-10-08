#include "PPath.h"

using namespace SymuMaster;

PPath::PPath()
{
}

PPath::PPath(SymuCore::Trip * pTrip, SymuCore::ServiceType serviceType, size_t iFirstPatternIndex, TraficSimulatorInstance* pPPathSimulator)
{
    m_pTrip = pTrip;
    m_eServiceType = serviceType;
    m_iFirstPatternIndex = iFirstPatternIndex;
    m_iLastPatternIndex = iFirstPatternIndex;
    m_pSimulator = pPPathSimulator;
}

PPath::~PPath()
{
}

const SymuCore::Trip * PPath::GetTrip() const
{
    return m_pTrip;
}

SymuCore::ServiceType PPath::GetServiceType() const
{
    return m_eServiceType;
}

TraficSimulatorInstance * PPath::GetSimulator() const
{
    return m_pSimulator;
}

size_t PPath::GetFirstPatternIndex() const
{
    return m_iFirstPatternIndex;
}

size_t PPath::GetLastPatternIndex() const
{
    return m_iLastPatternIndex;
}

void PPath::SetLastPatternIndex(size_t iLastPatternIndex)
{
    m_iLastPatternIndex = iLastPatternIndex;
}

bool PPath::operator==(const PPath &otherPath) const
{
    return ((otherPath.m_pTrip == m_pTrip) && (otherPath.m_iFirstPatternIndex == m_iFirstPatternIndex) && (otherPath.m_iLastPatternIndex == m_iLastPatternIndex));
}

const boost::posix_time::ptime & PPath::GetArrivalTime() const
{
    return m_dtArrivalTime;
}

void PPath::SetArrivalTime(const boost::posix_time::ptime &dtArrivalTime)
{
    m_dtArrivalTime = dtArrivalTime;
}


