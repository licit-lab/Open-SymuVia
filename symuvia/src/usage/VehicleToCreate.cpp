#include "stdafx.h"
#include "VehicleToCreate.h"

#include "AbstractFleet.h"
#include "Trip.h"

VehicleToCreate::VehicleToCreate()
{
    m_VehicleId = -1;
    m_pFleet = NULL;
    m_pTrip = NULL;
}

VehicleToCreate::VehicleToCreate(int vehId, AbstractFleet * pFleet)
{
    m_VehicleId = vehId;
    m_pFleet = pFleet;
    m_pTrip = NULL;
}

VehicleToCreate::~VehicleToCreate()
{
}

int VehicleToCreate::GetVehicleID()
{
    return m_VehicleId;
}
    
AbstractFleet * VehicleToCreate::GetFleet()
{
    return m_pFleet;
}

Trip* VehicleToCreate::GetTrip()
{
    return m_pTrip;
}

void VehicleToCreate::SetTrip(Trip * pTrip)
{
    m_pTrip = pTrip;
}

template void VehicleToCreate::serialize(boost::archive::xml_woarchive & ar, const unsigned int version);
template void VehicleToCreate::serialize(boost::archive::xml_wiarchive & ar, const unsigned int version);

template<class Archive>
void VehicleToCreate::serialize(Archive& ar, const unsigned int version)
{
    ar & BOOST_SERIALIZATION_NVP(m_VehicleId);
    //ar & BOOST_SERIALIZATION_NVP(m_pFleet);
    //ar & BOOST_SERIALIZATION_NVP(m_pTrip);
}