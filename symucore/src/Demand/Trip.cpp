#include "Trip.h"

#include <Demand/Population.h>
#include <Demand/SubPopulation.h>

using namespace SymuCore;

Trip::Trip()
{

}

Trip::Trip(int nID, const boost::posix_time::ptime & departureTime, Origin * pOrigin, Destination * pDest, SubPopulation *pSubPopulation, VehicleType * pVehicleType, double iSigmaConvenience) :
m_nID(nID),
m_DepartureTime(departureTime),
m_pOrigin(pOrigin),
m_pDestination(pDest),
m_pSubPopulation(pSubPopulation),
m_pVehicleType(pVehicleType),
m_dbSigmaConvenience(iSigmaConvenience)
{
    m_bIsPredefinedPath = false;
    m_pPopulation = pSubPopulation->GetPopulation();
	m_bDeactivated = false;
}

Trip::~Trip()
{

}

bool Trip::DepartureTimeSorter(const Trip& lhs, const Trip& rhs)
{
    return lhs.m_DepartureTime < rhs.m_DepartureTime;
}

bool Trip::DepartureTimePtrSorter(const Trip* lhs, const Trip* rhs)
{
    return lhs->m_DepartureTime < rhs->m_DepartureTime;
}

const boost::posix_time::ptime & Trip::GetDepartureTime() const
{
    return m_DepartureTime;
}

int Trip::GetID() const
{
    return m_nID;
}

Origin * Trip::GetOrigin() const
{
    return m_pOrigin;
}

Destination * Trip::GetDestination() const
{
    return m_pDestination;
}

Population *Trip::GetPopulation() const
{
    return m_pPopulation;
}

VehicleType * Trip::GetVehicleType() const
{
    return m_pVehicleType;
}

const Path & Trip::GetPath(int iInstance) const
{
    assert((size_t)iInstance < m_Paths.size()); // can't resize as in the other Get/SetPath functions since it is a const method. If it crashes here, a solution would be to have a static empty path to return in this case

    return m_Paths.at(iInstance);
}

Path & Trip::GetPath(int iInstance)
{
    if (m_Paths.size() <= (size_t)iInstance)
    {
        m_Paths.resize(iInstance + 1);
    }

	return m_Paths[iInstance];
}

void Trip::SetOrigin(Origin * pOrigin)
{
    m_pOrigin = pOrigin;
}

void Trip::SetDestination(Destination * pDestination)
{
    m_pDestination = pDestination;
}

void Trip::SetPath(int iInstance, const SymuCore::Path& path)
{
    if (m_Paths.size() <= (size_t)iInstance)
    {
        m_Paths.resize(iInstance + 1);
    }

    m_Paths[iInstance] = path;
}

bool Trip::isPredefinedPath() const
{
    return m_bIsPredefinedPath;
}

void Trip::SetIsPredefinedPath(bool isPredefinedPath)
{
    m_bIsPredefinedPath = isPredefinedPath;
}

SubPopulation *Trip::GetSubPopulation() const
{
    return m_pSubPopulation;
}

double Trip::GetSigmaConvenience(double dbDefaultValue) const
{
    //(-1) represent no value defined
    if(m_dbSigmaConvenience == -1)
        return dbDefaultValue;

    return m_dbSigmaConvenience;
}

