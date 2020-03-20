#pragma once

#ifndef _SYMUCORE_TRIP_
#define _SYMUCORE_TRIP_

#include "SymuCoreExports.h"

#include <Demand/Path.h>

#include <boost/date_time/posix_time/posix_time_types.hpp>

#pragma warning( push )  
#pragma warning( disable : 4251 )  

namespace SymuCore {

class Origin;
class Destination;
class Population;
class VehicleType;
class SubPopulation;

class SYMUCORE_DLL_DEF Trip {

public:

    Trip();
    Trip(int nID, const boost::posix_time::ptime & departureTime, Origin * pOrigin, Destination * pDest, SubPopulation* pSubPopulation, VehicleType * pVehicleType, double iSigmaConvenience = -1);

    virtual ~Trip();

    static bool DepartureTimeSorter(const Trip& lhs, const Trip& rhs);
    static bool DepartureTimePtrSorter(const Trip* lhs, const Trip* rhs);

    int GetID() const;
    const boost::posix_time::ptime & GetDepartureTime() const;
    Origin * GetOrigin() const;
    Destination * GetDestination() const;
    Population* GetPopulation() const;
    SubPopulation *GetSubPopulation() const;
    VehicleType * GetVehicleType() const;
	const Path& GetPath(int iInstance) const;
    Path& GetPath(int iInstance);
    bool isPredefinedPath() const;
    double GetSigmaConvenience(double dbDefaultValue) const;
	bool IsDeactivated() const { return m_bDeactivated; };
	void SetDeactivated(bool bDeactivated) { m_bDeactivated = bDeactivated; }

    void SetOrigin(Origin * pOrigin);
    void SetDestination(Destination * pDestination);
    void SetPath(int iInstance, const SymuCore::Path& path);
    void SetIsPredefinedPath(bool isPredefinedPath);

protected:

    int                         m_nID;
    boost::posix_time::ptime    m_DepartureTime;
    Origin*                     m_pOrigin;
    Destination*                m_pDestination;
    Population*                 m_pPopulation;
    SubPopulation*              m_pSubPopulation;
    VehicleType*                m_pVehicleType;
    std::vector<Path>           m_Paths;
    bool                        m_bIsPredefinedPath;
    double                      m_dbSigmaConvenience;
	bool						m_bDeactivated;			// true if the trip is deactivated (due to an empty path for example)
};

}

#pragma warning( pop )  

#endif // _SYMUCORE_TRIP_
