#pragma once
#ifndef ZoneDeTerminaisonH
#define ZoneDeTerminaisonH

#include "usage/SymuViaTripNode.h"

#ifdef USE_SYMUCORE
#include "SymuCoreAreaHelper.h"
#endif // USE_SYMUCORE

class Connexion;
class Parking;
class DocTrafic;
class TronconOrigine;
class TronconDestination;
class CPlaque;
class CSection;
class LessSectionPtr;
class LessSection;
class TripLeg;
class MFDSensor;
class SymuViaFleetParameters;
class ParkingParameters;


class ZoneDeTerminaison : public SymuViaTripNode
{
public:
    ZoneDeTerminaison();
    ZoneDeTerminaison(std::string sID, Reseau * pReseau);
    virtual ~ZoneDeTerminaison();

    // *******************************************
    // Traitements public
    // *******************************************

    // Traitement sur entrée d'un véhicule dans le TripNode. Ne fait rien : la sortie éventuelle du véhicule est gérée dans le SymuViaFleet::OnCurrentLegFinished()
	virtual void VehiculeEnter(boost::shared_ptr<Vehicule> pVehicle, VoieMicro * pDestinationEnterLane, double dbInstDestinationReached, double dbInstant, double dbTimeStep, bool bForcedOutside = false);
	

    // Initialisation de la zone.
    void SetZonePosition(const std::vector<Tuyau*> & tuyaux);

    // Renvoie la liste des tuyaux avec capacité de stationnement non nulle
    std::vector<Tuyau*> GetTuyauxStationnement();

    // Définition d'un parking dans la zone de terminaison
    void AddParking(Parking * pParking) { m_LstParkings.push_back(pParking); }

    // Définition d'une plaque dans la zone de terminaison
    void AddPlaque(CPlaque * pPlaque) { m_LstPlaques.push_back(pPlaque); }

    // Traite le cas du véhicule qui entre dans sa zone de destination
    void ReaffecteVehicule(Vehicule * pVehicule, double dbInstant);

    // Traite le réarbitrage d'un véhicule vers un stationnement de destination
    void ReaffecteVehiculeStationnementDestination(Vehicule * pVehicule, double dbInstant, bool bRearbitrage);

    // Traite le cas de la réaffectation du véhicule vers un tronçon interne de la zone de stationnement
    void ReaffecteVehiculeTroncon(Vehicule * pVehicule, double dbInstant);

    // Sorties à produire pour la zone
    void SortieTrafic(DocTrafic *pXMLDocTrafic);

    // Gère l'incrémentation du stock de véhicules stationnés en mode simplifié
    void IncStockStationnement(Vehicule * pVehicle);

    // Tire une plaque d'origine
    Tuyau * TiragePlaqueOrigine(double dbInstant, CPlaque * pPlaqueOrigin, TypeVehicule * pTypeVehicule, CMotifCoeff * pMotifCoeff, SymuViaTripNode * pDestination, Tuyau * pTuy, TripLeg * pTripLeg, bool bForceNonResidential, double & dbCreationPosition, CPlaque* & pPlaqueResult, bool & bDontCreateVehicle, Parking* & pParkingOrigin, bool & bSortieStationnement);

    // Tire une plaque de destination
    void TiragePlaqueDestination(double dbInstant, CMotifCoeff * pMotifCoeff, Vehicule * pVehicule, SymuViaTripNode * pOrigin, TypeVehicule * pVehiculeType, CPlaque * & pPlaqueResult, Tuyau * & pCurrentTuy, Tuyau * & pPrevTuyau);

	// Random selection of a link from the area
	Tuyau* LinkRandomSelection(double dbInstant, CPlaque * pPlaque, TypeVehicule * pTypeVehicule, bool bPerimetreElargi, SymuViaTripNode * pMultiClassDestination, bool bIsResidentiel);

	// Construit le début du parcours pour sortir de la zone d'origine en fonction du motif et des plaques
    Tuyau * AddInitialPathToLeg(double dbInstant, SymuViaFleetParameters * pFleetParams, CPlaque * pPlaqueOrigin, CMotifCoeff * pMotifCoeff, Tuyau * pTuy, TypeVehicule * pTypeVehicule, TripLeg * pTripLeg, bool bForceNonResidential, double & dbCreationPosition, Parking* & pParkingOrigin, bool & bSortieStationnement);

    // Vérification de la bonne définition des coefficients des motifs pour les plaques
    bool CheckCoeffs(Logger & logger);

    // Récupère le mode surfacique pour l'arrivée en zone
    bool IsModeSurfaciqueSimplifie(CPlaque * pPlaque, double dbInstant);

    // Récupère le mode de demande autogénérée par le stationnement
    bool IsModeDemandeStationnement(CPlaque * pPlaque, double dbInstant);

    // Met à jour la distance avant stationnement (mode simplifié), en se basant sur les tuyaux accessibles déjà utilisés lors du calcul initial
    void UpdateGoalDistance(double dbInstant, SymuViaFleetParameters * pParams);

    // Gestion de la demande autogénérée pour le stationnement
    void GestionDemandeStationnement(CPlaque * pPlaque, double dbInstant);
   
    // *******************************************
    // Accesseurs
    // *******************************************
    void SetMaxDistanceToJunction(double dbMaxDistanceToJunction) { m_dbMaxDistanceToJunction = dbMaxDistanceToJunction; }
    double GetMaxDistanceToJunction() { return m_dbMaxDistanceToJunction; }

    void SetParkingParameters(ParkingParameters * pParams, bool bOwnsParkingParams) { m_pParkingParameters = pParams; m_bOwnsParkingParams = bOwnsParkingParams; }
    ParkingParameters * GetParkingParameters() { return m_pParkingParameters; }

    const std::vector<CPlaque*> & GetLstPlaques() { return m_LstPlaques; }

    std::map<Connexion*, std::vector<Tuyau*>, LessConnexionPtr<Connexion> > GetInputJunctions();
    std::map<Connexion*, std::vector<Tuyau*>, LessConnexionPtr<Connexion> > GetOutputJunctions();

    void ClearPathCaches();

#ifdef USE_SYMUCORE
    void CalculTempsParcours(double dbInstFinPeriode, SymuCore::MacroType * pMacroType, double dbPeriodDuration);
    void ComputeMeanSpeed(double dbInstFinPeriode, SymuCore::MacroType * pMacroType, double dbPeriodDuration);
    void ComputeConcentration(double dbPeriodDuration);
    double GetEmptyMeanSpeed(double dbInstFinPeriode, SymuCore::MacroType * pMacroType);

    MFDSensor* m_pMFDSensor;  // Capteur MFD utile pour les calculs de vitesse moyenne en zone pour le calcul des temps de parcours pour SymuMaster par méthode spatiale
    std::map<SymuCore::MacroType*, double> m_dbMeanSpeed;
    double m_dbConcentration;

    SymuCoreAreaHelper m_AreaHelper;
#endif // USE_SYMUCORE

private:

    // Tire si la terminaison du trajet en zone est de type résidentielle ou non
    bool TirageTypeDestinationResidentielle(CPlaque * pPlaque, double dbInstant);

    // Tire si le départ du trajet en zone est de type résidentielle ou non
    bool TirageTypeOrigineResidentielle(CPlaque * pPlaque, double dbInstant);

    // Vérifie le critère de radius pour la longueur d'un itinéraire de départ en zone
    bool CheckRadiusForOriginOK(const std::vector<Tuyau*> & path, Tuyau * pPreviousLink);

    // Vérifie le critère de radius pour la longueur d'un itinéraire d'arrivée en zone
    bool CheckRadiusForDestinationOK(const std::vector<Tuyau*> & path);

    // Vérifie si les tronçons respectent le le critère de radius autour d'un point de jonction
    std::vector<Tuyau*> CheckRadiusForDestinationOK(TypeVehicule * pTypeVeh, Connexion * pJunction, const std::map<Tuyau*, std::vector<Tuyau*>, LessPtr<Tuyau> > & pCandidates);

    // Récupère la liste des parkings de la zone liés aux tuyaux considérés
    std::set<Parking*> GetParkingsFromLinks(const std::vector<Tuyau*> & links, TypeVehicule * pTypeVehicule);

    // Récupère la liste des tronçons permettant de ralier le point de jonction désiré
    const std::vector<Tuyau*> & GetAccessibleLinksToStart(CPlaque * pPlaque, bool bPerimetreElargi, TypeVehicule * pTypeVeh, SymuViaTripNode * pJunctionLinkOrDestination);

    // Récupère la liste des tronçons accessibles depuis l'origine passé en paramètre (soit origine, soit couple de tuyau courant + précédent)
    const std::vector<Tuyau*> & GetAccessibleLinksToEnd(CPlaque * pPlaque, bool bPerimetreElargi, TypeVehicule * pTypeVeh, Connexion * pJunction, SymuViaTripNode * pOrigin, Tuyau * pCurrentTuy, Tuyau * pPrevTuyau);

    // Tire le type de stationnement entre surfacique et hors-sol pour le départ en zone.
    bool TirageTypeParkingSurfaciqueOrigin(double dbInstant, CPlaque * pPlaque, bool bPerimetreElargi, TypeVehicule * pTypeVeh, SymuViaTripNode * pJunctionLink, bool & bFound);

    // Tire le type de stationnement entre surfacique et hors-sol en fonction des taux d'occupation et des couts pour l'arrivée en zone.
    bool TirageTypeParkingSurfaciqueDestination(double dbInstant, CPlaque * pPlaque, bool bPerimetreElargi, TypeVehicule * pTypeVeh, Connexion * pJunction, Tuyau * pCurrentTuy, Tuyau * pPrevTuyau);

    // Calcule le taux d'occupation en stationnement surfacique
    double GetTauxOccupationSurfacique(const std::vector<Tuyau*> & links, bool & bHasSurfacicStock, bool & bHasSurfacicRoom);

    // Calcule le taux d'occupation en stationnement hors sol
    double GetTauxOccupationHorsSol(const std::vector<Tuyau*> & links, TypeVehicule * pTypeVeh, bool & bHasHSStock, bool & bHasHSRoom);

    // Construit la map pour le tirage d'une section de la zone ou plaque de départ en fonction du type de stationnement initial
    void FillCandidateSectionsMapOrigin(CPlaque * pPlaque, TypeVehicule * pTypeVehicule, bool bPerimetreElargi, SymuViaTripNode * pJunctionLink, bool bIsResidentiel, std::map<CSection, double, LessSection> & sectionsCandidates);

    // Construit la map pour le tirage d'une section de la zone ou plaque de destination en fonction du type de stationnement final
    void FillCandidateSectionsMapDestination(CPlaque * pPlaque, TypeVehicule * pTypeVehicule, bool bPerimetreElargi, Connexion * pJunction, SymuViaTripNode * pOrigin, Tuyau * pCurrentTuy, Tuyau * pPrevTuyau, std::map<CSection, double, LessSection> & sectionsCandidates);

    // fonction de construction de la map à partir d'un ensemble de longueurs et de sections
    void FillCandidateSectionsFromLength(const std::vector<CSection> & sections, TypeVehicule * pTypeVehicule, std::map<CSection, double, LessSection> & sectionsCandidates);

    // Tirage d'un parking d'origine
    Parking * TirageParkingOrigin(CPlaque * pPlaque, TypeVehicule * pTypeVehicule, bool bPerimetreElargi, SymuViaTripNode * pMultiClassDestination);

    // Tirage d'un parking de destination
    Parking * TirageParkingDestination(CPlaque * pPlaque, TypeVehicule * pTypeVehicule, bool bPerimetreElargi, Connexion * pJunction, SymuViaTripNode * pOrigin, Tuyau * pCurrentTuy, Tuyau * pPrevTuyau);

    // Tirage du type de stationnement en zone ou plaque au départ d'un véhicule
    void TiragesStationnementOrigin(double dbInstant, CPlaque * pPlaque, TypeVehicule * pTypeVehicule, SymuViaTripNode * pMultiClassDestination, bool bForceNonResidential, bool & bIsResidentiel, bool & bIsParkingSurfacique, bool & bPerimetreElargi, bool & bDontCreateVehicle);

    // Tirage du type de stationnement en zone ou plaque à l'arrivée d'un véhicule
    void TiragesStationnementDestination(double dbInstant, CPlaque * pPlaque, TypeVehicule * pTypeVehicule, Connexion * pJunction, Tuyau * pCurrentTuy, Tuyau * pPrevTuyau, bool bPerimetreElargi, bool & bIsResidentiel, bool & bIsParkingSurfacique);

    // Ajoute l'itinéraire entre le parking d'origine et le point de jonction au TripLeg passé en paramètre
    bool AddInitialPathFromParking(TripLeg * pTripLeg, Parking * pParkingOrigin, SymuViaTripNode * pMultiClassDestination, TypeVehicule * pTypeVehicule, double dbInstant);

    // Ajoute l'itinéraire manquant jusqu'au parking passé en paramètre
    bool AddFinalPathToParking(Vehicule * pVehicle, Parking * pParkingDestination, Connexion * pJunction, Tuyau * pCurrentTuy, Tuyau * pPrevTuyau, TypeVehicule * pTypeVehicule, double dbInstant);

    // Ajoute l'itinéraire entre la section d'origine et le point de jonction au TripLeg passé en paramètre
    Tuyau * AddInitialPathFromSection(TripLeg * pTripLeg, double dbInstant, CPlaque * pPlaque, TypeVehicule * pTypeVehicule, bool bPerimetreElargi, SymuViaTripNode * pMultiClassDestination, bool bIsResidentiel);

    // Ajoute la fin de l'itinéraire poru terminaison en zone
    bool AddFinalPathToSection(Vehicule * pVehicle, double dbInstant, CPlaque * pPlaque, TypeVehicule * pTypeVehicule, SymuViaTripNode * pOrigin, Tuyau * pCurrentTuy, Tuyau * pPrevTuy, bool bPerimetreElargi);

    // Calcule et assigne la distance avant stationnement (mode simplifié)
    void UpdateGoalDistance(double dbInstant, SymuViaFleetParameters * pParams, TypeVehicule * pTypeVeh, Tuyau * pCurrentTuy, Tuyau * pPrevTuyau);

private:
    // List of car parks
    std::vector<Parking*> m_LstParkings;

    // List of sub-area
    std::vector<CPlaque*> m_LstPlaques;

    // Longueur max pour le tirage des chemins en zone
    double m_dbMaxDistanceToJunction;

    // Cache de TronconDestination pour la zone
    std::map<std::pair<Tuyau*,double>, TronconDestination*> m_CacheTronconsDestination;

    // Cache pour les tronçons possibles au départ en zone pour un point de jonction donné
    std::map<TypeVehicule*, std::map<CPlaque*, std::map<bool, std::map<SymuViaTripNode*, std::vector<Tuyau*> > > > > m_CacheAvailableOriginLinks;
    std::map<TypeVehicule*, std::map<CPlaque*, std::map<bool, std::map<SymuViaTripNode*, std::vector<Tuyau*> > > > > m_CacheAvailableDestinationLinksFromOrigin;
    std::map<TypeVehicule*, std::map<CPlaque*, std::map<Connexion*, std::map<std::pair<Tuyau*, Tuyau*>, std::vector<Tuyau*> > > > > m_CacheAvailableDestinationLinksFromLink;
    std::map<TypeVehicule*, std::map<CPlaque*, std::map<std::pair<Tuyau*, Tuyau*>, std::vector<Tuyau*> > > > m_CacheExtendedAvailableDestinationLinksFromLink;

    std::map<TypeVehicule*, std::map<Connexion*, std::map<Tuyau*, bool> > > m_CacheLinksInRadius;

    ParkingParameters * m_pParkingParameters;
    bool m_bOwnsParkingParams;

////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Sérialisation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////
private:

	friend class boost::serialization::access;
	template<class Archive>
	void serialize(Archive & ar, const unsigned int version);
};
#endif // ZoneDeTerminaisonH

