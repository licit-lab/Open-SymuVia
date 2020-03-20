#pragma once

#ifndef SYMUCORE_POPULATION_H
#define SYMUCORE_POPULATION_H

#include "SymuCoreExports.h"

#include "Utils/SymuCoreConstants.h"

#include <vector>
#include <string>

namespace boost {
    namespace posix_time {
        class ptime;
    }
}

#pragma warning(push)
#pragma warning(disable : 4251)

namespace SymuCore {

class MacroType;
class Trip;
class Origin;
class Destination;
class VehicleType;
class SubPopulation;
class MultiLayersGraph;

class SYMUCORE_DLL_DEF Population{

public:

    Population();
    Population(const std::string& strName);
    virtual ~Population();

    void DeepCopy(const Population & source, MultiLayersGraph * pGraph);

    void Clear();

    Trip* AddTrip(int nID, const boost::posix_time::ptime & departureTime, Origin * pOrigin, Destination * pDest, SubPopulation *pSubPopulation, VehicleType * pVehicleType);

    MacroType *GetMacroType() const;
    const std::vector<ServiceType>& GetListEnabledServices() const;
    const std::vector<Trip *> & GetListTrips() const;
    const std::vector<SubPopulation *>& GetListSubPopulations() const;
    std::vector<SubPopulation *> &GetListSubPopulations();
    std::string GetStrName() const;
    double GetInitialWalkSpeed() const;
    double GetIntermediateWalkSpeed() const;
    double GetInitialWalkRadius() const;
    double GetIntermediateWalkRadius() const;
    bool GetDisableOptimization() const;

    void SetMacroType(MacroType *pMacroType);
    void SetListEnabledServices(const std::vector<ServiceType> &listEnabledServices);
    void SetListTrips(const std::vector<Trip *> &listTrips);
    void SetListSubPopulations(const std::vector<SubPopulation *> &listSubPopulations);
    void SetStrName(const std::string &strName);
    void SetInitialWalkSpeed(double dbWalkSpeed);
    void SetIntermediateWalkSpeed(double dbWalkSpeed);
    void SetInitialWalkRadius(double dbWalkRadius);
    void SetIntermediateWalkRadius(double dbWalkRadius);
    void SetDisableOptimization(bool bDisableOptimization);

    void addServiceType(ServiceType eServiceType);
    void addSubPopulation(SubPopulation* pSubPopulation);
    bool hasAccess(ServiceType eServiceType);
    SubPopulation *hasSubPopulation(const std::string& strName);

protected:
    MacroType*                     m_pMacroType; //Macro type that the population can use
    std::string                    m_strName; // name of this macro
    std::vector<ServiceType>       m_listEnabledServices; //list of enabled services for this population
    std::vector<Trip*>             m_listTrips; //list of trips for this population
    std::vector<SubPopulation*>    m_listSubPopulations; //list of SubPopulation for this population
    double                         m_dbInitialWalkSpeed; //walking speed for initiation/termination of the trip
    double                         m_dbIntermediateWalkSpeed; //walking speed between PPaths inside of the trip
    double                         m_dbInitialWalkRadius;//walking radius for initial/termination patterns
    double                         m_dbIntermediateWalkRadius;//walking radius for intermediate patterns

    bool                           m_bDisableOptimization;//Disable the optimization for the population (all users assigned to shortest path)
};
}

#pragma warning(pop)

#endif // SYMUCORE_POPULATION_H
