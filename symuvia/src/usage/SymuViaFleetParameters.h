#pragma once
#ifndef SymuViaFleetParametersH
#define SymuViaFleetParametersH

#include "AbstractFleetParameters.h"

#include <vector>
#include <string>
#include <set>

class Parking;
class Tuyau;
class CMotifCoeff;
class CPlaque;
class Connexion;
class ZoneDeTerminaison;

class SymuViaFleetParameters : public AbstractFleetParameters
{

public:
    SymuViaFleetParameters();
    virtual ~SymuViaFleetParameters();

    // Itinéraire prédéfini
    void                        SetRouteId(const std::string & routeId);
    const std::string &         GetRouteId();
    
    void                        SetIsTerminaison(bool bTerminaison);
    bool                        IsTerminaison();
    void                        SetIsTerminaisonResidentielle(bool bTerminaisonResidentielle);
    bool                        IsTerminaisonResidentielle();
    void                        SetIsTerminaisonSurfacique(bool bTerminaisonSurfacique);
    bool                        IsTerminaisonSurfacique();
    void                        SetIsRechercheStationnement(bool bIsRechercheStationnement, double dbInstant);
    bool                        IsRechercheStationnement();
    void                        SetDoReaffecteVehicule(bool bValue);
    bool                        DoReaffecteVehicule();

    void                        UpdateGoalDistance(double dbGoalDistance, double dbInstant);
    double                      GetGoalDistance();
    double                      GetDistanceRechercheParcourue();
    bool                        IsGoalDistanceUpdateNeeded(double dbInstant, ZoneDeTerminaison * pZone);
    std::vector<Tuyau*> &       GetAccessibleLinksForParking();

    bool                        IsMaxSurfaciqueDureeReached(double dbInstant, ZoneDeTerminaison * pZone);

    void                        SetForcePerimetreEtendu(bool bValue);

    bool                        IsPerimetreElargi(double dbInstant, ZoneDeTerminaison * pZone);
    bool                        IsRearbitrageNeeded(double dbInstant, ZoneDeTerminaison * pZone);

    void                        InitializeZoneTravel(double dbInstant, const std::string & zoneId, double dbDistance, Connexion * pInputJunction);
    double                      GetInstantEntreeZone();
    void                        IncDistanceParcourue(double dbDistance);
    void                        SetPlaqueOrigin(CPlaque * pPlaqueOrigin);
    void                        SetLastLinkInOriginZone(Tuyau * pLink);
    void                        SetPlaqueDestination(CPlaque * pPlaqueDestination);
    std::set<Tuyau*>            &GetLstTuyauxDejaParcourus();
    CMotifCoeff*                GetMotif();
    void                        SetMotif(CMotifCoeff * pMotif);
    const std::string &         GetZoneId();
    double                      GetDistanceParcourue();
    Connexion*                  GetZoneDestinationInputJunction();
    void                        SetZoneDestinationInputJunction(Connexion* pJunction);
    CPlaque*                    GetPlaqueOrigin();
    Tuyau*                      GetLastLinkInOriginZone();
    CPlaque*                    GetPlaqueDestination();


    virtual AbstractFleetParameters * Clone();
    
protected:

    // Identifiant de l'itinéraire prédéfini éventuel
    std::string                 m_RouteId;

    // Gestion de la terminaison en zone
    bool                        m_bIsTerminaison;
    bool                        m_bIsTerminaisonResidentielle;
    bool                        m_bIsTerminaisonSurfacique;
    bool                        m_bIsRechercheStationnement;
    double                      m_dbInstantDebutRechercheStationnement;
    double                      m_dbInstantLastArbitrage;
    double                      m_dbInstantLastMAJGoalDistance;
    bool                        m_bForcePerimetreEtendu;
    std::set<Tuyau*>            m_LstTuyauxDejaParcourus;    // Liste des tronçons de la zone déjà parcourus à la recherche de stationnement
    bool                        m_bDoReaffecteVehicule;     // Indique si on doit réarbitrer le stationnement final

    CMotifCoeff*                m_pMotif;                   // Motif de déplacement éventuel du véhicule

    double                      m_dbInstantEntreeZone;      // Instant de l'entrée en zone
    std::string                 m_zoneId;                   // Identifiant de la zone cible
    double                      m_dbDistanceParcourue;      // Pour sortie de la distance parcourue depuis la rentrée en zone
    double                      m_dbDistanceRechercheParcourue; // Distance parcourue en recherche de stationnement (pour le mode de stationnement basé sur la distance à parcourir)
    double                      m_dbGoalDistance;           // Distance à parcourir avant de trouver une place de stationnement (mode de stationnement surfacique simplifié)
    Connexion*                  m_pInputJunction;           // Point de jonction d'entrée dans la zone de destination

    CPlaque*                    m_pPlaqueOrigin;            // Plaque d'origine affectée au véhicule.
    Tuyau*                      m_pLastLinkInOriginZone;    // Dernier tuyau de l'itinéraire initial du véhicule dans sa zone d'origine (pour calcul des temps de départ en zone pour SymuPlayer)
    CPlaque*                    m_pPlaqueDestination;       // Plaque de destination affectée au véhicule (cas où SymuMaster l'impose, SymuVia n'a plus à effectuer de tirage)

    std::vector<Tuyau*>         m_LstAccessibleLinksForParking; // Liste des liens accessibles pour le parking surfacique ne mode simplifié

////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Sérialisation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////
private:
	friend class boost::serialization::access;
	template<class Archive>
	void serialize(Archive& ar, const unsigned int version);
};

#endif // SymuViaFleetParametersH
