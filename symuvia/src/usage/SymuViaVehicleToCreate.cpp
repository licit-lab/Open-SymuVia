#include "stdafx.h"
#include "SymuViaVehicleToCreate.h"

#include "vehicule.h"
#include "SymuViaTripNode.h"
#include "tuyau.h"
#include "Plaque.h"

#include <boost/serialization/shared_ptr.hpp>
#include <boost/serialization/vector.hpp>

SymuViaVehicleToCreate::SymuViaVehicleToCreate() :
    VehicleToCreate()
{
    m_pOrigine = NULL;
    m_pPlaqueOrigin = NULL;
    m_pPlaqueDestination = NULL;
    m_pTypeVeh = NULL;
    m_nVoie = 0;
    m_dbt = 0;
    m_pDest = NULL;
    m_pJunction = NULL;
    m_ExternalID = -1;
}

SymuViaVehicleToCreate::SymuViaVehicleToCreate(int vehId, AbstractFleet * pFleet) :
    VehicleToCreate(vehId, pFleet)
{
    m_pOrigine = NULL;
    m_pPlaqueOrigin = NULL;
    m_pPlaqueDestination = NULL;
    m_pTypeVeh = NULL;
    m_nVoie = 0;
    m_dbt = 0;
    m_pDest = NULL;
    m_pJunction = NULL;
    m_ExternalID = -1;
}

SymuViaVehicleToCreate::~SymuViaVehicleToCreate()
{
}

void SymuViaVehicleToCreate::SetOrigin(SymuViaTripNode * pOrigin)
{
    m_pOrigine = pOrigin;
}
SymuViaTripNode * SymuViaVehicleToCreate::GetOrigin()
{
    return m_pOrigine;
}

void SymuViaVehicleToCreate::SetPlaqueOrigin(CPlaque * pPlaqueOrigin)
{
    m_pPlaqueOrigin = pPlaqueOrigin;
}
CPlaque * SymuViaVehicleToCreate::GetPlaqueOrigin()
{
    return m_pPlaqueOrigin;
}

void SymuViaVehicleToCreate::SetPlaqueDestination(CPlaque * pPlaqueDestination)
{
    m_pPlaqueDestination = pPlaqueDestination;
}

CPlaque * SymuViaVehicleToCreate::GetPlaqueDestination()
{
    return m_pPlaqueDestination;
}

void SymuViaVehicleToCreate::SetType(TypeVehicule * pTypeVeh)
{
    m_pTypeVeh = pTypeVeh;
}

TypeVehicule * SymuViaVehicleToCreate::GetType()
{
    return m_pTypeVeh;
}

void SymuViaVehicleToCreate::SetNumVoie(int nVoie)
{
    m_nVoie = nVoie;
}

int SymuViaVehicleToCreate::GetNumVoie()
{
    return m_nVoie;
}

void SymuViaVehicleToCreate::SetTimeFraction(double dbt)
{
    m_dbt = dbt;
}

double SymuViaVehicleToCreate::GetTimeFraction()
{
    return m_dbt;
}

void SymuViaVehicleToCreate::SetDestination(SymuViaTripNode * pDest)
{
    m_pDest = pDest;
}

SymuViaTripNode * SymuViaVehicleToCreate::GetDestination()
{
    return m_pDest;
}

void SymuViaVehicleToCreate::SetItinerary(boost::shared_ptr<std::vector<Tuyau*> > pIti)
{
    m_pItinerary = pIti;
}

boost::shared_ptr<std::vector<Tuyau*> > SymuViaVehicleToCreate::GetItinerary()
{
    return m_pItinerary;
}

void SymuViaVehicleToCreate::SetJunction(Connexion * pJunction)
{
    m_pJunction = pJunction;
}

Connexion * SymuViaVehicleToCreate::GetJunction()
{
    return m_pJunction;
}

void SymuViaVehicleToCreate::SetExternalID(int externalID)
{
    m_ExternalID = externalID;
}

int SymuViaVehicleToCreate::GetExternalID()
{
    return m_ExternalID;
}

void SymuViaVehicleToCreate::SetNextRouteID(const std::string & nextRouteID)
{
	m_strNextRouteID = nextRouteID;
}

const std::string & SymuViaVehicleToCreate::GetNextRouteID()
{
    return m_strNextRouteID;
}

template void SymuViaVehicleToCreate::serialize(boost::archive::xml_woarchive & ar, const unsigned int version);
template void SymuViaVehicleToCreate::serialize(boost::archive::xml_wiarchive & ar, const unsigned int version);

template<class Archive>
void SymuViaVehicleToCreate::serialize(Archive& ar, const unsigned int version)
{
    ar & BOOST_SERIALIZATION_BASE_OBJECT_NVP(VehicleToCreate);

    ar & BOOST_SERIALIZATION_NVP(m_pOrigine);
    ar & BOOST_SERIALIZATION_NVP(m_pPlaqueOrigin);
    ar & BOOST_SERIALIZATION_NVP(m_pPlaqueDestination);
	ar & BOOST_SERIALIZATION_NVP(m_pTypeVeh);
    ar & BOOST_SERIALIZATION_NVP(m_nVoie);
	ar & BOOST_SERIALIZATION_NVP(m_dbt);
    ar & BOOST_SERIALIZATION_NVP(m_pDest);
    ar & BOOST_SERIALIZATION_NVP(m_pItinerary);
    ar & BOOST_SERIALIZATION_NVP(m_pJunction);
    ar & BOOST_SERIALIZATION_NVP(m_ExternalID);
    ar & BOOST_SERIALIZATION_NVP(m_strNextRouteID);
}