#include "Population.h"

#include "MacroType.h"
#include "Trip.h"
#include "SubPopulation.h"
#include "Graph/Model/MultiLayersGraph.h"

#include <stddef.h>

using namespace SymuCore;

Population::Population()
{
    m_pMacroType = NULL;
    m_dbInitialWalkSpeed = 1.2;
    m_dbIntermediateWalkSpeed = 1.2;
    m_dbInitialWalkRadius = 100;
    m_dbIntermediateWalkRadius = 100;
    m_bDisableOptimization = false;
}

Population::Population(const std::string& strName)
{
    m_strName = strName;
    m_pMacroType = NULL;
    m_dbInitialWalkSpeed = 1.2;
    m_dbIntermediateWalkSpeed = 1.2;
    m_dbIntermediateWalkRadius = 100;
    m_bDisableOptimization = false;
}

Population::~Population()
{
    Clear();
}

void Population::DeepCopy(const Population & source, MultiLayersGraph * pGraph)
{
    m_strName = source.m_strName;
    if (source.m_pMacroType)
    {
        m_pMacroType = new MacroType();
        m_pMacroType->DeepCopy(*source.m_pMacroType);
        pGraph->AddMacroType(m_pMacroType);
    }
    else
    {
        m_pMacroType = NULL;
    }

    m_dbInitialWalkSpeed = source.m_dbInitialWalkSpeed;
    m_dbIntermediateWalkSpeed = source.m_dbIntermediateWalkSpeed;
    m_dbInitialWalkRadius = source.m_dbInitialWalkRadius;
    m_dbIntermediateWalkRadius = source.m_dbIntermediateWalkRadius;
    m_bDisableOptimization = source.m_bDisableOptimization;

    for(std::vector<SubPopulation*>::const_iterator itSubPopulation = source.GetListSubPopulations().begin(); itSubPopulation != source.GetListSubPopulations().end(); itSubPopulation++)
    {
        SubPopulation* pSubPopulation = new SubPopulation(**itSubPopulation);
        pSubPopulation->SetPopulation(this);
        m_listSubPopulations.push_back(pSubPopulation);
    }

    m_listEnabledServices = source.m_listEnabledServices;
}


void Population::Clear()
{
    for (size_t i = 0; i < m_listTrips.size(); i++)
    {
        delete m_listTrips[i];
    }
    m_listTrips.clear();

    for (size_t i = 0; i < m_listSubPopulations.size(); i++)
    {
        delete m_listSubPopulations[i];
    }
    m_listSubPopulations.clear();
}

Trip* Population::AddTrip(int nID, const boost::posix_time::ptime &departureTime, Origin *pOrigin, Destination *pDest, SubPopulation* pSubPopulation, VehicleType * pVehicleType)
{
    Trip* newTrip = new Trip(nID, departureTime, pOrigin, pDest, pSubPopulation, pVehicleType);
    pSubPopulation->GetListUsers().push_back(newTrip);
    m_listTrips.push_back(newTrip);
    return newTrip;
}

MacroType *Population::GetMacroType() const
{
    return m_pMacroType;
}

const std::vector<ServiceType>& Population::GetListEnabledServices() const
{
    return m_listEnabledServices;
}

void Population::SetMacroType(MacroType *pMacroType)
{
    m_pMacroType = pMacroType;
}

void Population::SetListEnabledServices(const std::vector<ServiceType> &listEnabledServices)
{
    m_listEnabledServices = listEnabledServices;
}

void Population::addServiceType(ServiceType eServiceType)
{
    m_listEnabledServices.push_back(eServiceType);
}

void Population::addSubPopulation(SubPopulation *pSubPopulation)
{
    m_listSubPopulations.push_back(pSubPopulation);
    pSubPopulation->SetPopulation(this);
}

bool Population::hasAccess(ServiceType eServiceType)
{
    // don't consider ST_Undefined as forbidden to allow users to go on the interlayer nodes
    if (eServiceType == ST_Undefined)
    {
        return true;
    }
    else
    {
        for (std::vector<ServiceType>::iterator itServiceType = m_listEnabledServices.begin(); itServiceType != m_listEnabledServices.end(); itServiceType++)
        {
            if ((*itServiceType) == ST_All || (*itServiceType) == eServiceType)
            {
                return true;
            }
        }

        return false;
    }
}

SubPopulation* Population::hasSubPopulation(const std::string &strName)
{
    SubPopulation* pSubPopulation = NULL;
    for(size_t i = 0; i < m_listSubPopulations.size(); i++)
    {
        if(m_listSubPopulations[i]->GetStrName() == strName)
        {
            pSubPopulation = m_listSubPopulations[i];
            break;
        }
    }

    return pSubPopulation;
}

double Population::GetInitialWalkSpeed() const
{
    return m_dbInitialWalkSpeed;
}

double Population::GetIntermediateWalkSpeed() const
{
    return m_dbIntermediateWalkSpeed;
}

double Population::GetInitialWalkRadius() const
{
    return m_dbInitialWalkRadius;
}

double Population::GetIntermediateWalkRadius() const
{
    return m_dbIntermediateWalkRadius;
}

bool Population::GetDisableOptimization() const
{
    return m_bDisableOptimization;
}

void Population::SetListSubPopulations(const std::vector<SubPopulation *> &listSubPopulations)
{
    m_listSubPopulations = listSubPopulations;
}

const std::vector<SubPopulation *> &Population::GetListSubPopulations() const
{
    return m_listSubPopulations;
}

std::vector<SubPopulation *> &Population::GetListSubPopulations()
{
    return m_listSubPopulations;
}

std::string Population::GetStrName() const
{
    return m_strName;
}

void Population::SetStrName(const std::string &strName)
{
    m_strName = strName;
}

const std::vector<Trip *> & Population::GetListTrips() const
{
    return m_listTrips;
}

void Population::SetListTrips(const std::vector<Trip *> &listTrips)
{
    m_listTrips = listTrips;
}

void Population::SetInitialWalkSpeed(double dbWalkSpeed)
{
    m_dbInitialWalkSpeed = dbWalkSpeed;
}

void Population::SetIntermediateWalkSpeed(double dbWalkSpeed)
{
    m_dbIntermediateWalkSpeed = dbWalkSpeed;
}

void Population::SetInitialWalkRadius(double dbWalkRadius)
{
    m_dbInitialWalkRadius = dbWalkRadius;
}

void Population::SetIntermediateWalkRadius(double dbWalkRadius)
{
    m_dbIntermediateWalkRadius = dbWalkRadius;
}

void Population::SetDisableOptimization(bool bDisableOptimization)
{
    m_bDisableOptimization = bDisableOptimization;
}
