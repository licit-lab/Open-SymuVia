#pragma once
#ifndef SimulationSnapshotH
#define SimulationSnapshotH

#include "usage/SymuViaTripNode.h"
#include "sensors/AbstractSensor.h"

#pragma warning(disable : 4003)
#include <rapidjson/document.h>
#pragma warning(default : 4003)

#include <boost/shared_ptr.hpp>

#include <map>
#include <deque>

class Tuyau;
class Vehicule;
class PlanDeFeux;
class CDFSequence;
class AbstractFleet;
class AbstractFleetSnapshot;
class ControleurDeFeux;
class Convergent;
class RegulationBrique;
class BriqueDeConnexion;
class ZoneDeTerminaison;
class TravelTimesOutputManagerSnapshot;
struct LGP;

#ifdef USE_SYMUCORE
namespace SymuCore {
    class MacroType;
}
#endif // USE_SYMUCORE

// Structure permettant le stockage des variables de simulation d'une entrée à un instant donné
struct stVarSimEntree
{
	std::map<Tuyau*, std::map<int, std::deque<boost::shared_ptr<Vehicule>> > >  mapVehEnAttente;    // Map de la liste des véhicules en attente par voie (numéro de voie, map des véhicules)
	std::map<Tuyau*, std::map<int, int > >						                mapVehPret;			// Map du véhicule pret à partir par voie (numéro de voie, identifiant du véhicule)
	double                             					                        dbCritCreationVeh;	// Critère de création des véhicule pour l'instant courant
																		// le critère peut s'exprimer sous forme d'un instant à atteindre ou d'un nombre (<=1) de véhicule 
    std::deque<CreationVehicule>                                                lstVehiculesACreer; // Liste des véhicules à créer
    std::vector<double>                                                         lstInstantsSortieStationnement; // Liste des instants de création dues à une sortie de stationnement

	std::map<TypeVehicule*, std::map<SymuViaTripNode*, std::vector< std::pair<double, std::vector<Tuyau*> > > > > mapDrivenPaths; // Chemins pilotés de façon externe via SymSetODPaths()

////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Sérialisation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////
private:
	friend class boost::serialization::access;
	template<class Archive>
	void serialize(Archive & ar, const unsigned int version);
};

// Structure permettant le stockage des variables de simulation d'un controleur de feux à un instant donné
struct stVarSimCDF
{
    stVarSimCDF() {pPDFActif = NULL; pSeqPrePriorite = NULL; pSeqRetour1 = NULL; pSeqRetour2 = NULL;}

    PlanDeFeux*                                         pPDFActif;                  // Plan de feux actif (si pilotage)
    char                                                cModeFct;                   // Mode de fonctionnement du contrôleur (N : normal, O : pré-priorité, P : priorité, Q : post-priorité)
    std::deque<std::pair<int,LGP*> >                    dqBus;                      // Liste des bus prioritaires actuellement pris en compte par les LGP du contrôleur
    boost::shared_ptr<PlanDeFeux>                       pPDFEx;                     // Plan de feu actif durant une phase de VGP prioritaire
    double                                              dbDebPDFEx;                 // Début de l'application du plan de feux de la phase VGP prioritaire
    double                                              dbFinPDFEx;                 // Fin de l'application du plan de feux de la phase VGP prioritaire (rteour au plan de feux normal)
    CDFSequence                                           *pSeqPrePriorite;            // Séquence à appliquer
    CDFSequence                                           *pSeqRetour1;                // Séquence de retour 1 après la sortie du CDF du VGP
    CDFSequence                                           *pSeqRetour2;                // Séquence de retour 2 après la sortie du CDF du VGP
    int                                                 nTypeGestionPrePriorite;    // Type de gestion ( 1: 'respect temps min', 2: 'séquence actuelle écourtée', 3 : 'prolongement séquence actuelle')

////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Sérialisation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////
private:
	friend class boost::serialization::access;
	template<class Archive>
	void serialize(Archive & ar, const unsigned int version);
};

// Structure permettant le stockage des variables de simulation d'un tuyau micro à un instant donné
struct stVarSimTuy
{
    int                                 nResolution;                            // Type meso ou micro (à sauvegarder/restituer car peut changer à cause du mode d'hybridation dynamique)
	std::map<int, std::pair< TypeVehicule*, std::pair<double, double>> > mapVeh;// Map des véhicules sortant du tuyau au cours de la période d'affectation courante
																				// avec instant exact d'entrée et de sortie du tuyau
	std::map<TypeVehicule*, std::pair<double, double>> mapDernierVehSortant;	// Map du dernier véhicule sorti par type de véhicule (lors de la période d'affectation précédente) avec instant exact d'entrée et de sorti

	std::map<TypeVehicule*, int>		mapNbVehEntre;							// Map du nombre cumulé de véhicule entré sur le tronçon depuis le début de la simulation
	std::map<TypeVehicule*, int>		mapNbVehSorti;							// Map du nombre cumulé de véhicule sorti du tronçon depuis le début de la simulation
	std::map<TypeVehicule*, int>		mapNbVehSortiPrev;						// Map du nombre cumulé de véhicule sorti sur le tronçon depuis le début de la simulation jusqu'à la précédente période d'affectation
	std::map<TypeVehicule*, int>		mapNbVehEntrePrev;						// Map du nombre cumulé de véhicule entré sur le tronçon depuis le début de la simulation jusqu'à la précédente période d'affectation

	bool								bIsForbidden;							// Le tuyau est interdit ou non

#ifdef USE_SYMUCORE
    std::map<SymuCore::MacroType*, double> mapTTByMacroType;
    std::map<SymuCore::MacroType*, double> mapMarginalByMacroType;
    double                                 dbTTForAllMacroTypes;
	double								   dbEmissionForAllMacroTypes;
    std::map<SymuCore::MacroType*, double> dbMeanNbVehiclesForTravelTimeUpdatePeriod;
    AbstractSensorSnapshot * sensorSnapshot;

    stVarSimTuy()
    {
        sensorSnapshot = NULL;
    }

    ~stVarSimTuy()
    {
        if (sensorSnapshot)
        {
            delete sensorSnapshot;
        }
    }
#endif

////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Sérialisation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////
private:
	friend class boost::serialization::access;
	template<class Archive>
	void serialize(Archive & ar, const unsigned int version);
};

// Structure permettant le stockage des variables de simulation d'un tuyau meso à un instant donné
struct stVarSimTuyMeso
{
    std::deque<int>                     currentVehicules;
	double                              dCurrentMeanW;          //! Moyenne courante des w des vehicules du troncon
	double                              dCurrentMeanK;          //! Moyenne courante des k des vehicules du troncons
    std::list< std::pair<double, int> > arrivals;               //! Temps d'arrivé des véhicules sur le troncon et le véhicule concerné
    std::pair<double, int>              nextArrival;            //! Prochain temps d'arrivé des véhicules sur le troncon et le véhicule concerné
    std::deque<double >                 upstreamPassingTimes;   //! Temps de passages depuis la connexion amont 
    std::deque<double>                  downstreamPassingTimes; //! Temps de passage depuis la connexion avale
    double                              dSupplyTime;            //! Temps de fourniture suivant au troncon

	bool								bIsForbidden;			//! Le tuyau est interdit ou non

////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Sérialisation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////
private:
	friend class boost::serialization::access;
	template<class Archive>
	void serialize(Archive & ar, const unsigned int version);
};

// Structure permettant le stockage des variables de simulation d'un noeud mesoà un instant donné
struct stVarSimMesoNode
{
    std::map<Tuyau*,std::list< std::pair<double, int > > >                                  arrivals;               //! Liste des couples vehicules; instants d'arrivés depuis une entrée
    std::map<Tuyau*, std::deque<double >>                                                   upstreamPassingTimes;   //! Liste des temps de passage amont ordonnées dans l'ordre de passage d'après le tuyau d'origine
    std::map<Tuyau*, std::deque<double >>                                                   downstreamPassingTimes; //! Liste des temps de passage aval ordonnées dans l'ordre de passage
    std::map<Tuyau*, std::pair<double, int > >                                              nextArrivals;           //! Couples vehicules; instants de la prochaines arrivée depuis une entrée
    std::map< Tuyau*, int>                                                                  mapRanksIncomming;      //! Nombre de véhicules entrant sur les troncons de la connexion (si tuyau = null il s'agit d'une entrée )
    std::map< Tuyau*, int>                                                                  mapRanksOutGoing;       //! Nombre de véhicules sortant sur les troncons de la connexion (si tuyau = null il s'agit d'une sortie )
    Tuyau*                                                                                  pNextEventTuyauEntry;   //! Tuyau d'entrée du prochain évênement
    double                                                                                  dNextEventTime;
    std::map<Tuyau*,  double>                                                               dNextSupplyTime;

////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Sérialisation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////
private:
	friend class boost::serialization::access;
	template<class Archive>
	void serialize(Archive & ar, const unsigned int version);
};


// Structure permettant le stockage des variables de simulation d'une brique de connexion à un instant donné
struct stVarSimBrique
{
	std::map<std::pair<Tuyau*,Tuyau*>, std::map<TypeVehicule*, double> >  mapCoutsMesures; // temps mesuré pour chaque mouvement de la brique
    std::map<std::pair<Tuyau*,Tuyau*>, std::map<int, std::pair< TypeVehicule*, std::pair<double,double> > > > mapVeh;	// Map des véhicules traversant le mouvement autorisé au cours de la période d'affectation courante
																					                                    // avec instant de sortie et durée de traversée du mouvement
#ifdef USE_SYMUCORE
    std::map<std::pair<Tuyau*, Tuyau*>, std::map<SymuCore::MacroType*, double> > mapTTByMacroType;
    std::map<std::pair<Tuyau*, Tuyau*>, std::map<SymuCore::MacroType*, double> > mapMarginalByMacroType;
    std::map<std::pair<Tuyau*, Tuyau*>, std::map<SymuCore::MacroType*, double> > dbMeanNbVehiclesForTravelTimeUpdatePeriod;
    std::map<std::pair<Tuyau*, Tuyau*>, double>                                  dbTTForAllMacroTypes;
	std::map<std::pair<Tuyau*, Tuyau*>, double>                                  dbEmissionsForAllMacroTypes;
#endif // USE_SYMUCORE

////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Sérialisation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////
private:
	friend class boost::serialization::access;
	template<class Archive>
	void serialize(Archive & ar, const unsigned int version);
};

// Structure permettant le stockage des variables de simulation pour une plaque à un instant donné
struct stVarSimPlaque
{
    std::map<Connexion*, std::map<int, std::pair< TypeVehicule*, std::pair<std::pair<double, double>, double> > > > mapDepartingVeh;
    std::map<Connexion*, std::map<int, std::pair< TypeVehicule*, std::pair<std::pair<double, double>, double> > > > mapArrivingVeh;

#ifdef USE_SYMUCORE
    std::map<Connexion*, std::map<SymuCore::MacroType*, double > > lstDepartingTravelTimes;
    std::map<Connexion*, std::map<SymuCore::MacroType*, double > > lstDepartingMarginals;
    std::map<Connexion*, std::map<SymuCore::MacroType*, double > > lstArrivingTravelTimes;
    std::map<Connexion*, std::map<SymuCore::MacroType*, double > > lstArrivingMarginals;
    std::map<Connexion*, std::map<SymuCore::MacroType*, double > > lstArrivingMeanNbVehicles;
    std::map<Connexion*, std::map<SymuCore::MacroType*, double > > lstDepartingMeanNbVehicles;
    std::map<Connexion*, double > lstArrivingTTForAllMacroTypes;
    std::map<Connexion*, double > lstDepartingTTForAllMacroTypes;
	std::map<Connexion*, double > lstArrivingEmissionForAllMacroTypes;
	std::map<Connexion*, double > lstDepartingEmissionForAllMacroTypes;
#endif // USE_SYMUCORE

    // rmq. : pas de sérialisation nécessaire tant qu'on ne se sert de ces classes que pour SymuMaster (qui ne permet pas la sérialisation)
};


// Structure permettant le stockage des variables de simulation d'une zone à un instant donné
struct stVarSimZone
{
#ifdef USE_SYMUCORE
    std::map<SymuCore::MacroType*, double> mapMeanSpeed;
    double dbConcentration;
#endif // USE_SYMUCORE

    stVarSimPlaque                     zoneData;
    std::map<CPlaque*, stVarSimPlaque> mapPlaques;

    AbstractSensorSnapshot * sensorSnapshot;

    stVarSimZone()
    {
        sensorSnapshot = NULL;
    }

    ~stVarSimZone()
    {
        if (sensorSnapshot)
        {
            delete sensorSnapshot;
        }
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    // Sérialisation
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////
private:
    friend class boost::serialization::access;
    template<class Archive>
    void serialize(Archive & ar, const unsigned int version);
};


// Structure permettant le stockage des variables de simulation d'un voie micro à un instant donné
struct stVarSimVoie
{
	std::map<std::string, double> dbNextInstSortie;      // Instant de sortie du véhicule suivant

////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Sérialisation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////
private:
	friend class boost::serialization::access;
	template<class Archive>
	void serialize(Archive & ar, const unsigned int version);
};

// Structure permettant le stockage des variables de simulation d'un convergent à un instant donné
struct stVarSimConvergent
{
	std::deque<double> dbInstLastPassage;      // Instant du dernier passage au point de convergence

////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Sérialisation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////
private:
	friend class boost::serialization::access;
	template<class Archive>
	void serialize(Archive & ar, const unsigned int version);
};


// Structure permettant le stockage des variables de simulation d'une brique de régulation à un instant donné
struct stVarSimRegulationBrique
{
	std::deque<boost::python::dict> lstContextes;      // Liste des contextes des éléments de la brique de régulation

////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Sérialisation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////
private:
	friend class boost::serialization::access;
	template<class Archive>
	void serialize(Archive & ar, const unsigned int version);
};

struct SimulatorSnapshot;
class DOMLSSerializerSymu;
// Classe d'implantation du module d'affectation
class SimulationSnapshot
{
private:

	// Variables de stockage d'une capture d'une simu à un instant donné
	double	m_dbInstSvg;						// Instant sauvegardé dans le cas d'une affectation convergente
	int		m_nInstSvg;							// Indice de l'instant sauvegardé dans le cas d'une affectation convergente

	int		m_nNbVehCumSvg;						// Cumul sauvegardé du nombre de véhicules présents sur le réseau à chaque pas de temps

    unsigned int m_nRandCountSvg;               // Nombre de tirages aléatoire effectués depuis le début de la simulation
	
	std::deque<boost::shared_ptr<Vehicule>> m_lstVehSvg;			// Liste des véhicules présents sur le réseau à l'instant 0 de la période d'affectation courante
	int		m_nLastIdVehSvg;

	std::map<AbstractFleet*, AbstractFleetSnapshot*> m_mapFleet;	// Sauvegarde des variables de simulation utiles pour la gestion des flottes de véhicules
    std::map<ControleurDeFeux*, stVarSimCDF> m_mapCDF;		        // Sauvegarde des variables de simulation utiles pour la gestion des CDF
	std::map<TuyauMicro*, stVarSimTuy> m_mapTuy;		            // Sauvegarde des variables de simulation utiles pour la gestion des tuyaux micro
    std::map<CTuyauMeso*, stVarSimTuyMeso> m_mapTuyMeso;	        // Sauvegarde des variables de simulation utiles pour la gestion des tuyaux meso
	std::map<VoieMicro*, stVarSimVoie> m_mapVoie;		            // Sauvegarde des variables de simulation utiles pour la gestion des voies
	std::map<SymuViaTripNode*, stVarSimEntree> m_mapOrigine;		        // Sauvegarde des variables de simulation utiles pour la gestion des origines
	std::map<AbstractSensor*, AbstractSensorSnapshot*> m_mapCapteur;	    // Sauvegarde des variables de simulation utiles pour la gestion des capteurs
    std::map<Convergent*, stVarSimConvergent>               m_mapConvergents;   // Sauvegarde des variables de simulation utiles pour la gestion des convergents
    std::map<RegulationBrique*, stVarSimRegulationBrique>   m_mapRegulations;   // Sauvegarde du contexte d'exécution des briques
    std::map<BriqueDeConnexion*, stVarSimBrique> m_mapBrique;		            // Sauvegarde des variables de simulation utiles pour la gestion des briques de connexion
    std::map<CMesoNode*, stVarSimMesoNode> m_mapMesoNode;		                // Sauvegarde des variables de simulation utiles pour la gestion des noeuds meso
    std::map<ZoneDeTerminaison*, stVarSimZone>  m_mapZones;                     // Sauvegarde des variables de simulation utiles pour la gestion des zones

    TravelTimesOutputManagerSnapshot * m_pTravelTimesOutputSnapshot;            // Sauvegarde des variables de simulation utiles pour la sortie JSON des temps de parcours

#ifdef USE_SYMUCOM
    SimulatorSnapshot * m_pITSSnapshot;                                         // Sauvegarde de ITS
#endif // USE_SYMUCOM

    std::vector<int>    m_nGestionCapteurPeriodes;

    // Pour gestion des fichiers temporaires :
public:
    TraceDocTrafic * m_XmlDocTraficTmp;				// Document sauvegarde des instants de simulation de la période d'affectation (avant la convergence)

    std::ofstream *m_pTrajsOFSTMP;
    std::ofstream *m_pLinkChangesOFSTMP;
    std::ofstream *m_pSensorsOFSTMP;

    std::string    m_strTrajsOFSTMP;
    std::string    m_strLinkChangesOFSTMP;
    std::string    m_strSensorsOFSTMP;

    rapidjson::Document* m_TravelTimesTmp; // document JSON temporaire ajouté dans le principal uniquement en cas de validation de la période d'agrégation

#ifdef USE_SYMUCOM
    DOMLSSerializerSymu* m_pSymuComWriter; // fichier temporaire ajouté dans le principal uniquement en cas de validation de la période d'agrégation
#endif // USE_SYMUCOM

public:

    SimulationSnapshot();
    ~SimulationSnapshot();

    void Backup(Reseau * pReseau);
    void Restore(Reseau * pReseau);

    double GetInstSvg() { return m_dbInstSvg; }

    void SwitchToTempFiles(Reseau * pReseau, size_t snapshotIdx);
    void DiscardTempFiles();
    void ValidateTempFiles(Reseau * pReseau);

private:

////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Sérialisation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    friend class boost::serialization::access;
    template<class Archive>
    void serialize(Archive & ar, const unsigned int version);
};

#endif // SimulationSnapshotH
