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
class CarrefourAFeuxEx;
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

// Structure permettant le stockage des variables de simulation d'une entr�e � un instant donn�
struct stVarSimEntree
{
	std::map<Tuyau*, std::map<int, std::deque<boost::shared_ptr<Vehicule>> > >  mapVehEnAttente;    // Map de la liste des v�hicules en attente par voie (num�ro de voie, map des v�hicules)
	std::map<Tuyau*, std::map<int, int > >						                mapVehPret;			// Map du v�hicule pret � partir par voie (num�ro de voie, identifiant du v�hicule)
	double                             					                        dbCritCreationVeh;	// Crit�re de cr�ation des v�hicule pour l'instant courant
																		// le crit�re peut s'exprimer sous forme d'un instant � atteindre ou d'un nombre (<=1) de v�hicule 
    std::deque<CreationVehicule>                                                lstVehiculesACreer; // Liste des v�hicules � cr�er
    std::vector<double>                                                         lstInstantsSortieStationnement; // Liste des instants de cr�ation dues � une sortie de stationnement

	std::map<TypeVehicule*, std::map<SymuViaTripNode*, std::vector< std::pair<double, std::vector<Tuyau*> > > > > mapDrivenPaths; // Chemins pilot�s de fa�on externe via SymSetODPaths()

////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// S�rialisation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////
private:
	friend class boost::serialization::access;
	template<class Archive>
	void serialize(Archive & ar, const unsigned int version);
};

// Structure permettant le stockage des variables de simulation d'un controleur de feux � un instant donn�
struct stVarSimCDF
{
    stVarSimCDF() {pPDFActif = NULL; pSeqPrePriorite = NULL; pSeqRetour1 = NULL; pSeqRetour2 = NULL;}

    PlanDeFeux*                                         pPDFActif;                  // Plan de feux actif (si pilotage)
    char                                                cModeFct;                   // Mode de fonctionnement du contr�leur (N : normal, O : pr�-priorit�, P : priorit�, Q : post-priorit�)
    std::deque<std::pair<int,LGP*> >                    dqBus;                      // Liste des bus prioritaires actuellement pris en compte par les LGP du contr�leur
    boost::shared_ptr<PlanDeFeux>                       pPDFEx;                     // Plan de feu actif durant une phase de VGP prioritaire
    double                                              dbDebPDFEx;                 // D�but de l'application du plan de feux de la phase VGP prioritaire
    double                                              dbFinPDFEx;                 // Fin de l'application du plan de feux de la phase VGP prioritaire (rteour au plan de feux normal)
    CDFSequence                                           *pSeqPrePriorite;            // S�quence � appliquer
    CDFSequence                                           *pSeqRetour1;                // S�quence de retour 1 apr�s la sortie du CDF du VGP
    CDFSequence                                           *pSeqRetour2;                // S�quence de retour 2 apr�s la sortie du CDF du VGP
    int                                                 nTypeGestionPrePriorite;    // Type de gestion ( 1: 'respect temps min', 2: 's�quence actuelle �court�e', 3 : 'prolongement s�quence actuelle')

////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// S�rialisation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////
private:
	friend class boost::serialization::access;
	template<class Archive>
	void serialize(Archive & ar, const unsigned int version);
};

// Structure permettant le stockage des variables de simulation d'un tuyau micro � un instant donn�
struct stVarSimTuy
{
    int                                 nResolution;                            // Type meso ou micro (� sauvegarder/restituer car peut changer � cause du mode d'hybridation dynamique)
	std::map<int, std::pair< TypeVehicule*, std::pair<double, double>> > mapVeh;// Map des v�hicules sortant du tuyau au cours de la p�riode d'affectation courante
																				// avec instant exact d'entr�e et de sortie du tuyau
	std::map<TypeVehicule*, std::pair<double, double>> mapDernierVehSortant;	// Map du dernier v�hicule sorti par type de v�hicule (lors de la p�riode d'affectation pr�c�dente) avec instant exact d'entr�e et de sorti

	std::map<TypeVehicule*, int>		mapNbVehEntre;							// Map du nombre cumul� de v�hicule entr� sur le tron�on depuis le d�but de la simulation
	std::map<TypeVehicule*, int>		mapNbVehSorti;							// Map du nombre cumul� de v�hicule sorti du tron�on depuis le d�but de la simulation
	std::map<TypeVehicule*, int>		mapNbVehSortiPrev;						// Map du nombre cumul� de v�hicule sorti sur le tron�on depuis le d�but de la simulation jusqu'� la pr�c�dente p�riode d'affectation
	std::map<TypeVehicule*, int>		mapNbVehEntrePrev;						// Map du nombre cumul� de v�hicule entr� sur le tron�on depuis le d�but de la simulation jusqu'� la pr�c�dente p�riode d'affectation

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
// S�rialisation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////
private:
	friend class boost::serialization::access;
	template<class Archive>
	void serialize(Archive & ar, const unsigned int version);
};

// Structure permettant le stockage des variables de simulation d'un tuyau meso � un instant donn�
struct stVarSimTuyMeso
{
    std::deque<int>                     currentVehicules;
	double                              dCurrentMeanW;          //! Moyenne courante des w des vehicules du troncon
	double                              dCurrentMeanK;          //! Moyenne courante des k des vehicules du troncons
    std::list< std::pair<double, int> > arrivals;               //! Temps d'arriv� des v�hicules sur le troncon et le v�hicule concern�
    std::pair<double, int>              nextArrival;            //! Prochain temps d'arriv� des v�hicules sur le troncon et le v�hicule concern�
    std::deque<double >                 upstreamPassingTimes;   //! Temps de passages depuis la connexion amont 
    std::deque<double>                  downstreamPassingTimes; //! Temps de passage depuis la connexion avale
    double                              dSupplyTime;            //! Temps de fourniture suivant au troncon

	bool								bIsForbidden;			//! Le tuyau est interdit ou non

////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// S�rialisation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////
private:
	friend class boost::serialization::access;
	template<class Archive>
	void serialize(Archive & ar, const unsigned int version);
};

// Structure permettant le stockage des variables de simulation d'un noeud meso� un instant donn�
struct stVarSimMesoNode
{
    std::map<Tuyau*,std::list< std::pair<double, int > > >                                  arrivals;               //! Liste des couples vehicules; instants d'arriv�s depuis une entr�e
    std::map<Tuyau*, std::deque<double >>                                                   upstreamPassingTimes;   //! Liste des temps de passage amont ordonn�es dans l'ordre de passage d'apr�s le tuyau d'origine
    std::map<Tuyau*, std::deque<double >>                                                   downstreamPassingTimes; //! Liste des temps de passage aval ordonn�es dans l'ordre de passage
    std::map<Tuyau*, std::pair<double, int > >                                              nextArrivals;           //! Couples vehicules; instants de la prochaines arriv�e depuis une entr�e
    std::map< Tuyau*, int>                                                                  mapRanksIncomming;      //! Nombre de v�hicules entrant sur les troncons de la connexion (si tuyau = null il s'agit d'une entr�e )
    std::map< Tuyau*, int>                                                                  mapRanksOutGoing;       //! Nombre de v�hicules sortant sur les troncons de la connexion (si tuyau = null il s'agit d'une sortie )
    Tuyau*                                                                                  pNextEventTuyauEntry;   //! Tuyau d'entr�e du prochain �v�nement
    double                                                                                  dNextEventTime;
    std::map<Tuyau*,  double>                                                               dNextSupplyTime;

////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// S�rialisation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////
private:
	friend class boost::serialization::access;
	template<class Archive>
	void serialize(Archive & ar, const unsigned int version);
};


// Structure permettant le stockage des variables de simulation d'une brique de connexion � un instant donn�
struct stVarSimBrique
{
	std::map<std::pair<Tuyau*,Tuyau*>, std::map<TypeVehicule*, double> >  mapCoutsMesures; // temps mesur� pour chaque mouvement de la brique
    std::map<std::pair<Tuyau*,Tuyau*>, std::map<int, std::pair< TypeVehicule*, std::pair<double,double> > > > mapVeh;	// Map des v�hicules traversant le mouvement autoris� au cours de la p�riode d'affectation courante
																					                                    // avec instant de sortie et dur�e de travers�e du mouvement
#ifdef USE_SYMUCORE
    std::map<std::pair<Tuyau*, Tuyau*>, std::map<SymuCore::MacroType*, double> > mapTTByMacroType;
    std::map<std::pair<Tuyau*, Tuyau*>, std::map<SymuCore::MacroType*, double> > mapMarginalByMacroType;
    std::map<std::pair<Tuyau*, Tuyau*>, std::map<SymuCore::MacroType*, double> > dbMeanNbVehiclesForTravelTimeUpdatePeriod;
    std::map<std::pair<Tuyau*, Tuyau*>, double>                                  dbTTForAllMacroTypes;
	std::map<std::pair<Tuyau*, Tuyau*>, double>                                  dbEmissionsForAllMacroTypes;
#endif // USE_SYMUCORE

////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// S�rialisation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////
private:
	friend class boost::serialization::access;
	template<class Archive>
	void serialize(Archive & ar, const unsigned int version);
};

// Structure permettant le stockage des variables de simulation pour une plaque � un instant donn�
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

    // rmq. : pas de s�rialisation n�cessaire tant qu'on ne se sert de ces classes que pour SymuMaster (qui ne permet pas la s�rialisation)
};


// Structure permettant le stockage des variables de simulation d'une zone � un instant donn�
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
    // S�rialisation
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////
private:
    friend class boost::serialization::access;
    template<class Archive>
    void serialize(Archive & ar, const unsigned int version);
};


// Structure permettant le stockage des variables de simulation d'un voie micro � un instant donn�
struct stVarSimVoie
{
	std::map<std::string, double> dbNextInstSortie;      // Instant de sortie du v�hicule suivant

////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// S�rialisation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////
private:
	friend class boost::serialization::access;
	template<class Archive>
	void serialize(Archive & ar, const unsigned int version);
};

// Structure permettant le stockage des variables de simulation d'un convergent � un instant donn�
struct stVarSimConvergent
{
	std::deque<double> dbInstLastPassage;      // Instant du dernier passage au point de convergence

////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// S�rialisation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////
private:
	friend class boost::serialization::access;
	template<class Archive>
	void serialize(Archive & ar, const unsigned int version);
};

// Structure permettant le stockage des variables de simulation d'un carrefour à feux à un instant donn�
struct stVarSimCrossroads
{
	std::deque<double> dbInstLastCrossing;      // Instant du dernier passage au point de convergence

////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// S�rialisation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////
private:
	friend class boost::serialization::access;
	template<class Archive>
	void serialize(Archive & ar, const unsigned int version);
};

// Structure permettant le stockage des variables de simulation d'une brique de r�gulation � un instant donn�
struct stVarSimRegulationBrique
{
	std::deque<boost::python::dict> lstContextes;      // Liste des contextes des �l�ments de la brique de r�gulation

////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// S�rialisation
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

	// Variables de stockage d'une capture d'une simu � un instant donn�
	double	m_dbInstSvg;						// Instant sauvegard� dans le cas d'une affectation convergente
	int		m_nInstSvg;							// Indice de l'instant sauvegard� dans le cas d'une affectation convergente

	int		m_nNbVehCumSvg;						// Cumul sauvegard� du nombre de v�hicules pr�sents sur le r�seau � chaque pas de temps

    unsigned int m_nRandCountSvg;               // Nombre de tirages al�atoire effectu�s depuis le d�but de la simulation
	
	std::deque<boost::shared_ptr<Vehicule>> m_lstVehSvg;			// Liste des v�hicules pr�sents sur le r�seau � l'instant 0 de la p�riode d'affectation courante
	int		m_nLastIdVehSvg;

	std::map<AbstractFleet*, AbstractFleetSnapshot*> m_mapFleet;	// Sauvegarde des variables de simulation utiles pour la gestion des flottes de v�hicules
    std::map<ControleurDeFeux*, stVarSimCDF> m_mapCDF;		        // Sauvegarde des variables de simulation utiles pour la gestion des CDF
	std::map<TuyauMicro*, stVarSimTuy> m_mapTuy;		            // Sauvegarde des variables de simulation utiles pour la gestion des tuyaux micro
    std::map<CTuyauMeso*, stVarSimTuyMeso> m_mapTuyMeso;	        // Sauvegarde des variables de simulation utiles pour la gestion des tuyaux meso
	std::map<VoieMicro*, stVarSimVoie> m_mapVoie;		            // Sauvegarde des variables de simulation utiles pour la gestion des voies
	std::map<SymuViaTripNode*, stVarSimEntree> m_mapOrigine;		        // Sauvegarde des variables de simulation utiles pour la gestion des origines
	std::map<AbstractSensor*, AbstractSensorSnapshot*> m_mapCapteur;	    // Sauvegarde des variables de simulation utiles pour la gestion des capteurs
    std::map<Convergent*, stVarSimConvergent>               m_mapConvergents;   // Sauvegarde des variables de simulation utiles pour la gestion des convergents
    std::map<CarrefourAFeuxEx*, stVarSimCrossroads>           m_mapCrossroads;   // Sauvegarde des variables de simulation utiles pour la gestion des convergents
    std::map<RegulationBrique*, stVarSimRegulationBrique>   m_mapRegulations;   // Sauvegarde du contexte d'ex�cution des briques
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
    TraceDocTrafic * m_XmlDocTraficTmp;				// Document sauvegarde des instants de simulation de la p�riode d'affectation (avant la convergence)

    std::ofstream *m_pTrajsOFSTMP;
    std::ofstream *m_pLinkChangesOFSTMP;
    std::ofstream *m_pSensorsOFSTMP;

    std::string    m_strTrajsOFSTMP;
    std::string    m_strLinkChangesOFSTMP;
    std::string    m_strSensorsOFSTMP;

    rapidjson::Document* m_TravelTimesTmp; // document JSON temporaire ajout� dans le principal uniquement en cas de validation de la p�riode d'agr�gation

#ifdef USE_SYMUCOM
    DOMLSSerializerSymu* m_pSymuComWriter; // fichier temporaire ajout� dans le principal uniquement en cas de validation de la p�riode d'agr�gation
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
// S�rialisation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    friend class boost::serialization::access;
    template<class Archive>
    void serialize(Archive & ar, const unsigned int version);
};

#endif // SimulationSnapshotH
