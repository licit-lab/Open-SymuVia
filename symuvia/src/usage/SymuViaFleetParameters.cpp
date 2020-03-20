#include "stdafx.h"
#include "SymuViaFleetParameters.h"

#include "Parking.h"
#include "tuyau.h"
#include "RepMotif.h"
#include "Plaque.h"
#include "ParkingParameters.h"
#include "ZoneDeTerminaison.h"

#include <boost/serialization/vector.hpp>
#include <boost/serialization/set.hpp>

SymuViaFleetParameters::SymuViaFleetParameters()
{
    m_bIsTerminaison = false;
    m_bIsTerminaisonResidentielle = true;
    m_bIsTerminaisonSurfacique = true;
    m_bIsRechercheStationnement = false;
    m_dbInstantDebutRechercheStationnement = -1;
    m_dbInstantLastArbitrage = -1;
    m_dbInstantLastMAJGoalDistance = -1;
    m_bDoReaffecteVehicule = false;
    m_bForcePerimetreEtendu = false;
    m_dbInstantEntreeZone = -1;
    m_dbDistanceParcourue = 0;
    m_dbDistanceRechercheParcourue = 0;
    m_dbGoalDistance = DBL_MAX;
    m_pMotif = NULL;
    m_pPlaqueOrigin = NULL;
    m_pLastLinkInOriginZone = NULL;
    m_pPlaqueDestination = NULL;
    m_pInputJunction = NULL;
}


SymuViaFleetParameters::~SymuViaFleetParameters()
{
}

void SymuViaFleetParameters::SetRouteId(const std::string & routeId)
{
    m_RouteId = routeId;
}

const std::string & SymuViaFleetParameters::GetRouteId()
{
    return m_RouteId;
}

void SymuViaFleetParameters::SetIsTerminaison(bool bTerminaison)
{
    m_bIsTerminaison = bTerminaison;
}

bool SymuViaFleetParameters::IsTerminaison()
{
    return m_bIsTerminaison;
}

void SymuViaFleetParameters::SetIsTerminaisonResidentielle(bool bTerminaisonResidentielle)
{
    m_bIsTerminaisonResidentielle = bTerminaisonResidentielle;
}

bool SymuViaFleetParameters::IsTerminaisonResidentielle()
{
    return m_bIsTerminaisonResidentielle;
}

void SymuViaFleetParameters::SetIsTerminaisonSurfacique(bool bTerminaisonSurfacique)
{
    m_bIsTerminaisonSurfacique = bTerminaisonSurfacique;
}

bool SymuViaFleetParameters::IsTerminaisonSurfacique()
{
    return m_bIsTerminaisonSurfacique;
}

void SymuViaFleetParameters::SetIsRechercheStationnement(bool bIsRechercheStationnement, double dbInstant)
{
    if (!m_bIsRechercheStationnement)
    {
        m_dbInstantDebutRechercheStationnement = dbInstant;
        m_dbInstantLastArbitrage = dbInstant;
    }
    m_bIsRechercheStationnement = bIsRechercheStationnement;
}

bool SymuViaFleetParameters::IsMaxSurfaciqueDureeReached(double dbInstant, ZoneDeTerminaison * pZone)
{
    ParkingParametersVariation * pParams = GetPlaqueDestination() ? GetPlaqueDestination()->GetParkingParameters()->GetVariation(dbInstant) : pZone->GetParkingParameters()->GetVariation(dbInstant);
    return dbInstant - m_dbInstantDebutRechercheStationnement > pParams->GetMaxSurfacicSearchDuration();
}

bool SymuViaFleetParameters::IsRearbitrageNeeded(double dbInstant, ZoneDeTerminaison * pZone)
{
    bool bResult = false;
    ParkingParametersVariation * pParams = GetPlaqueDestination() ? GetPlaqueDestination()->GetParkingParameters()->GetVariation(dbInstant) : pZone->GetParkingParameters()->GetVariation(dbInstant);
    if (dbInstant >= m_dbInstantLastArbitrage + pParams->GetReroutingPeriod())
    {
        bResult = true;
        m_dbInstantLastArbitrage = dbInstant;
    }
    return bResult;
}

bool SymuViaFleetParameters::IsPerimetreElargi(double dbInstant, ZoneDeTerminaison * pZone)
{
    ParkingParametersVariation * pParams = GetPlaqueDestination() ? GetPlaqueDestination()->GetParkingParameters()->GetVariation(dbInstant) : pZone->GetParkingParameters()->GetVariation(dbInstant);
    return m_bForcePerimetreEtendu || (m_dbInstantDebutRechercheStationnement != -1 && dbInstant >= m_dbInstantDebutRechercheStationnement + pParams->GetWideningDelay());
}

void SymuViaFleetParameters::SetForcePerimetreEtendu(bool bValue)
{
    m_bForcePerimetreEtendu = bValue;
}

bool SymuViaFleetParameters::IsRechercheStationnement()
{
    return m_bIsRechercheStationnement;
}

void SymuViaFleetParameters::SetDoReaffecteVehicule(bool bValue)
{
    m_bDoReaffecteVehicule = bValue;
}

bool SymuViaFleetParameters::DoReaffecteVehicule()
{
    return m_bDoReaffecteVehicule;
}

void SymuViaFleetParameters::InitializeZoneTravel(double dbInstant, const std::string & zoneId, double dbDistance, Connexion * pInputJunction)
{
    m_dbInstantEntreeZone = dbInstant;
    m_zoneId = zoneId;
    m_dbDistanceParcourue = dbDistance;
    m_pInputJunction = pInputJunction;
}

double SymuViaFleetParameters::GetInstantEntreeZone()
{
    return m_dbInstantEntreeZone;
}

void SymuViaFleetParameters::IncDistanceParcourue(double dbDistance)
{
    m_dbDistanceParcourue += dbDistance;
    if (m_bIsRechercheStationnement)
    {
        // Distance parcourue en recherche de stationnement
        m_dbDistanceRechercheParcourue += dbDistance;
    }
}

void SymuViaFleetParameters::UpdateGoalDistance(double dbGoalDistance, double dbInstant)
{
    // Si première fois ou si division par 0 dans le cas d'une mise à jour (ne devrait pas arriver de toute façon mais bon)
    if (m_dbGoalDistance >= DBL_MAX || m_dbGoalDistance <= 0)
    {
        m_dbGoalDistance = dbGoalDistance;
    }
    else
    {
        // Mise à jour en prenant en compte l'objectif précédent
        m_dbGoalDistance = m_dbDistanceRechercheParcourue + dbGoalDistance * (m_dbGoalDistance - m_dbDistanceRechercheParcourue) / m_dbGoalDistance;
    }
    m_dbInstantLastMAJGoalDistance = dbInstant;
}

double SymuViaFleetParameters::GetGoalDistance()
{
    return m_dbGoalDistance;
}

double SymuViaFleetParameters::GetDistanceRechercheParcourue()
{
    return m_dbDistanceRechercheParcourue;
}

bool SymuViaFleetParameters::IsGoalDistanceUpdateNeeded(double dbInstant, ZoneDeTerminaison * pZone)
{
    bool bResult = false;
    ParkingParametersVariation * pParams = GetPlaqueDestination() ? GetPlaqueDestination()->GetParkingParameters()->GetVariation(dbInstant) : pZone->GetParkingParameters()->GetVariation(dbInstant);
    if (dbInstant >= m_dbInstantLastMAJGoalDistance + pParams->GetUpdateGoalDistancePeriod())
    {
        bResult = true;
    }
    return bResult;
}

std::vector<Tuyau*> & SymuViaFleetParameters::GetAccessibleLinksForParking()
{
    return m_LstAccessibleLinksForParking;
}

std::set<Tuyau*> & SymuViaFleetParameters::GetLstTuyauxDejaParcourus()
{
    return m_LstTuyauxDejaParcourus;
}

CMotifCoeff* SymuViaFleetParameters::GetMotif()
{
    return m_pMotif;
}

void SymuViaFleetParameters::SetMotif(CMotifCoeff * pMotif)
{
    m_pMotif = pMotif;
}

const std::string & SymuViaFleetParameters::GetZoneId()
{
    return m_zoneId;
}

double SymuViaFleetParameters::GetDistanceParcourue()
{
    return m_dbDistanceParcourue;
}

Connexion* SymuViaFleetParameters::GetZoneDestinationInputJunction()
{
    return m_pInputJunction;
}

void SymuViaFleetParameters::SetZoneDestinationInputJunction(Connexion* pJunction)
{
    m_pInputJunction = pJunction;
}


void SymuViaFleetParameters::SetPlaqueOrigin(CPlaque * pPlaqueOrigin)
{
    m_pPlaqueOrigin = pPlaqueOrigin;
}

CPlaque* SymuViaFleetParameters::GetPlaqueOrigin()
{
    return m_pPlaqueOrigin;
}

void SymuViaFleetParameters::SetLastLinkInOriginZone(Tuyau * pLink)
{
    m_pLastLinkInOriginZone = pLink;
}

Tuyau* SymuViaFleetParameters::GetLastLinkInOriginZone()
{
    return m_pLastLinkInOriginZone;
}

void SymuViaFleetParameters::SetPlaqueDestination(CPlaque * pPlaqueDestination)
{
    m_pPlaqueDestination = pPlaqueDestination;
}

CPlaque* SymuViaFleetParameters::GetPlaqueDestination()
{
    return m_pPlaqueDestination;
}

AbstractFleetParameters* SymuViaFleetParameters::Clone()
{
    SymuViaFleetParameters * pResult = new SymuViaFleetParameters();

    pResult->m_UsageParameters = m_UsageParameters;

    pResult->m_RouteId = m_RouteId;

    pResult->m_bIsTerminaison = m_bIsTerminaison;
    pResult->m_bIsTerminaisonResidentielle = m_bIsTerminaisonResidentielle;
    pResult->m_bIsTerminaisonSurfacique = m_bIsTerminaisonSurfacique;
    pResult->m_bIsRechercheStationnement = m_bIsRechercheStationnement;
    pResult->m_dbInstantDebutRechercheStationnement = m_dbInstantDebutRechercheStationnement;
    pResult->m_bDoReaffecteVehicule = m_bDoReaffecteVehicule;
    pResult->m_bForcePerimetreEtendu = m_bForcePerimetreEtendu;
    pResult->m_LstTuyauxDejaParcourus = m_LstTuyauxDejaParcourus;
    pResult->m_pMotif = m_pMotif;
    pResult->m_dbInstantEntreeZone = m_dbInstantEntreeZone;
    pResult->m_zoneId = m_zoneId;
    pResult->m_dbDistanceParcourue = m_dbDistanceParcourue;
    pResult->m_dbDistanceRechercheParcourue = m_dbDistanceRechercheParcourue;
    pResult->m_dbGoalDistance = m_dbGoalDistance;
    pResult->m_pPlaqueOrigin = m_pPlaqueOrigin;
    pResult->m_pLastLinkInOriginZone = m_pLastLinkInOriginZone;
    pResult->m_pPlaqueDestination = m_pPlaqueDestination;
    pResult->m_pInputJunction = m_pInputJunction;
    pResult->m_LstAccessibleLinksForParking = m_LstAccessibleLinksForParking;
    pResult->m_dbInstantLastMAJGoalDistance = m_dbInstantLastMAJGoalDistance;

    return pResult;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Sérialisation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////
template void SymuViaFleetParameters::serialize(boost::archive::xml_woarchive & ar, const unsigned int version);
template void SymuViaFleetParameters::serialize(boost::archive::xml_wiarchive & ar, const unsigned int version);

template<class Archive>
void SymuViaFleetParameters::serialize(Archive & ar, const unsigned int version)
{
    ar & BOOST_SERIALIZATION_BASE_OBJECT_NVP(AbstractFleetParameters);

    ar & BOOST_SERIALIZATION_NVP(m_RouteId);

    ar & BOOST_SERIALIZATION_NVP(m_bIsTerminaison);
    ar & BOOST_SERIALIZATION_NVP(m_bIsTerminaisonResidentielle);
    ar & BOOST_SERIALIZATION_NVP(m_bIsTerminaisonSurfacique);
    ar & BOOST_SERIALIZATION_NVP(m_bIsRechercheStationnement);
    ar & BOOST_SERIALIZATION_NVP(m_dbInstantDebutRechercheStationnement);
    ar & BOOST_SERIALIZATION_NVP(m_bDoReaffecteVehicule);
    ar & BOOST_SERIALIZATION_NVP(m_bForcePerimetreEtendu);
    ar & BOOST_SERIALIZATION_NVP(m_LstTuyauxDejaParcourus);
    ar & BOOST_SERIALIZATION_NVP(m_pMotif);
    ar & BOOST_SERIALIZATION_NVP(m_dbInstantEntreeZone);
    ar & BOOST_SERIALIZATION_NVP(m_zoneId);
    ar & BOOST_SERIALIZATION_NVP(m_dbDistanceParcourue);
    ar & BOOST_SERIALIZATION_NVP(m_dbDistanceRechercheParcourue);
    ar & BOOST_SERIALIZATION_NVP(m_dbGoalDistance);
    ar & BOOST_SERIALIZATION_NVP(m_pPlaqueOrigin);
    ar & BOOST_SERIALIZATION_NVP(m_pLastLinkInOriginZone);
    ar & BOOST_SERIALIZATION_NVP(m_pPlaqueDestination);
    ar & BOOST_SERIALIZATION_NVP(m_pInputJunction);
    ar & BOOST_SERIALIZATION_NVP(m_LstAccessibleLinksForParking);
    ar & BOOST_SERIALIZATION_NVP(m_dbInstantLastMAJGoalDistance);
}
