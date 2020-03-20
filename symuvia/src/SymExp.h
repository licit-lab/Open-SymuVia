//------------------------------------------
// Fonctions exportées de la DLL SYMUVIA
//------------------------------------------

#include <string> 
#include "TrafficState.h"

#ifdef USE_SYMUCORE
#include <Utils/SymuCoreConstants.h>
namespace SymuCore {
    class MultiLayersGraph;
    class MacroType;
    class Node;
    class Pattern;
    class AbstractPenalty;
    class SubPopulation;
    class Populations;
    class Origin;
    class Destination;
    class IUserHandler;
}
namespace boost {
    namespace posix_time {
        class ptime;
        class time_duration;
    }
}
#endif

#ifdef WIN32
    #define SYMUBRUIT_EXPORT __declspec(dllimport)
    #define SYMUBRUIT_CDECL __cdecl
#else
    #define SYMUBRUIT_EXPORT
    #define SYMUBRUIT_CDECL 
#endif


// Chargement du scenario
SYMUBRUIT_EXPORT bool SYMUBRUIT_CDECL SymLoadNetwork(std::string sTmpXmlDataFile, std::string sScenarioID = "", std::string sOutdir = "");
SYMUBRUIT_EXPORT bool SYMUBRUIT_CDECL SymLoadNetwork(std::string sTmpXmlDataFile, eveShared::SimulationInfo * &pSInfo, eveShared::SymuviaNetwork * &pSNetwork, std::string sScenarioID = "", std::string sOutdir = "");
SYMUBRUIT_EXPORT int SYMUBRUIT_CDECL SymLoadNetworkPar(std::string sTmpXmlDataFile, std::string sScenarioID = "", std::string sOutdir = "");

// Sortie de la simulation
SYMUBRUIT_EXPORT int  SYMUBRUIT_CDECL SymQuit();

// ----- Simulation complète -----

// Exécution complète d'une simulation
SYMUBRUIT_EXPORT bool SYMUBRUIT_CDECL SymRun();

// Exécution complète d'une simulation de trafic
SYMUBRUIT_EXPORT bool SYMUBRUIT_CDECL SymRunTraffic();

// Exécution complète d'une simulation des émissions acoustiques
SYMUBRUIT_EXPORT bool SYMUBRUIT_CDECL SymRunAcousticEmissions();

// Exécution complète d'une simulation des émissions atmosphériques
SYMUBRUIT_EXPORT bool SYMUBRUIT_CDECL SymRunAirEmissions();


// ----- Simulation pas à pas -----

// Exécution d'un pas de temps d'une simulation (et sortie de l'état de la simulation)
SYMUBRUIT_EXPORT bool SYMUBRUIT_CDECL SymRunNextStep(std::string &sXmlFluxInstant, bool bTrace, bool &bNEnd);
SYMUBRUIT_EXPORT bool SYMUBRUIT_CDECL SymRunNextStep(eveShared::TrafficState * &pTrafficEVE, bool bTrace, bool &bNEnd);

SYMUBRUIT_EXPORT char* SYMUBRUIT_CDECL SymRunNextStepNode(bool bTrace, bool &bNEnd);

SYMUBRUIT_EXPORT bool SYMUBRUIT_CDECL SymRunNextStep(int networkId, std::string &sXmlFluxInstant, bool bTrace, bool &bNEnd);

// Exécution d'un pas de temps d'une simulation de trafic
SYMUBRUIT_EXPORT bool SYMUBRUIT_CDECL SymRunNextStepTraffic(std::string &sXmlFluxInstant, bool bTrace, bool &bNEnd);
SYMUBRUIT_EXPORT bool SYMUBRUIT_CDECL SymRunNextStepTraffic(eveShared::TrafficState * &pTrafficEVE, bool bTrace, bool &bNEnd);

// Exécution d'un pas de temps d'une simulation de trafic et d'acoustique
SYMUBRUIT_EXPORT bool SYMUBRUIT_CDECL SymRunNextStepTrafficAcoustic(std::string &sXmlFluxInstant, bool bCel, bool bSrc, bool bTrace, bool &bNEnd);
SYMUBRUIT_EXPORT bool SYMUBRUIT_CDECL SymRunNextStepTrafficAcoustic(eveShared::TrafficState * &pTrafficEVE, bool bCel, bool bSrc, bool bTrace, bool &bNEnd);

// Exécution d'un pas de temps d'une simulation de trafic et d'atmospherique
SYMUBRUIT_EXPORT bool SYMUBRUIT_CDECL SymRunNextStepTrafficAtmospheric(std::string &sXmlFluxInstant, bool bTrace, bool &bNEnd);

// Déplacement vers un pas de temps d'indice donné
SYMUBRUIT_EXPORT bool SYMUBRUIT_CDECL SymRunToStep(int nStep, std::string &sXmlFluxInstant, bool bTrace, bool &bNEnd);
SYMUBRUIT_EXPORT bool SYMUBRUIT_CDECL SymRunToStep(int nStep, eveShared::TrafficState * &pTrafficEVE, bool bTrace, bool &bNEnd);

// Réinitialisation de la simulation au premier instant
SYMUBRUIT_EXPORT int  SYMUBRUIT_CDECL SymReset();

#ifdef USE_SYMUCORE

// Chargement du réseau spécifique SymuMaster (désactive les calculs d'itinéraires, la génération des véhicules, ...)
SYMUBRUIT_EXPORT int SYMUBRUIT_CDECL SymLoadNetwork(SymuCore::IUserHandler * pUserHandler, std::string sTmpXmlDataFile, int marginalsDeltaN, bool bUseMarginalsMeanMethod,
	bool bUseMarginalsMedianMethod, int nbVehiclesForTravelTimes, int nbPeriodsForTravelTimes, bool bUseSpatialTTMethod, bool bUseClassicSpatialTTMethod, /*bool bUseSustainable,*/
	bool bEstimateTrafficLightsWaitTime, double dbMinSpeedForTravelTimes, double dbMaxMarginalsValue, bool bUseTravelTimesAsMarginalsInAreas,
	double dbConcentrationRatioForFreeFlowCriterion, int nNbClassRobustTTMethod, int nMaxNbPointsPerClassRobustTTMethod, int nMinNbPointsStatisticalFilterRobustTTMethod,
	bool bUseLastBusIfAnyForTravelTimes, bool bUseEmptyTravelTimesForBusLines, double dbMeanBusExitTime,
	int nbStopsToConnectOutsideOfSubAreas, double dbMaxInitialWalkRadius, double dbMaxIntermediateWalkRadius, bool bComputeAllCosts, double dbMinLengthForMarginals,
	std::string sRobustTravelTimesFile, std::string sRobustTravelSpeedsFile, bool bPollutantEmissionComputation,
	boost::posix_time::ptime & startTime, boost::posix_time::ptime & endTime, boost::posix_time::time_duration & timeStep, bool & bHasTrajectoriesOutput);

SYMUBRUIT_EXPORT bool SYMUBRUIT_CDECL SymDisableTrajectoriesOutput(int networkId, bool bDisableTrajectoriesOutput);

// Fonctions pour l'identifications des relations entre zones et sous-origines/destinations cotrespodnantes (plaques, parkings)
SYMUBRUIT_EXPORT SymuCore::Origin * SYMUBRUIT_CDECL SymGetParentOrigin(int networkId, SymuCore::Origin * pChildOrigin, SymuCore::MultiLayersGraph * pGraph);
SYMUBRUIT_EXPORT SymuCore::Destination * SYMUBRUIT_CDECL SymGetParentDestination(int networkId, SymuCore::Destination * pChildDestination, SymuCore::MultiLayersGraph * pGraph);

// Fonctions nécessaires au pilotage de l'affectation par SymuMaster
SYMUBRUIT_EXPORT bool SYMUBRUIT_CDECL SymTakeSimulationSnapshot(int networkId);
SYMUBRUIT_EXPORT bool SYMUBRUIT_CDECL SymSimulationRollback(int networkId, size_t iSnapshotIdx);
SYMUBRUIT_EXPORT bool SYMUBRUIT_CDECL SymSimulationCommit(int networkId);

// Génération d'un Graph SymuCore
SYMUBRUIT_EXPORT bool SYMUBRUIT_CDECL SymBuildGraph(int networkId, SymuCore::MultiLayersGraph * pGraph, bool bIsPrimaryGraph);

// Récupération de ODs utiles pour la période d'affectation
SYMUBRUIT_EXPORT bool SYMUBRUIT_CDECL SymFillPopulations(int iNetworkId, SymuCore::MultiLayersGraph * pGraph, SymuCore::Populations & populations,
    const boost::posix_time::ptime & startSimulationTime, const boost::posix_time::ptime & endSimulationTime, bool bIgnoreSubAreas,
    std::vector<SymuCore::Trip> & lstTrips);

// Met à jour les mesures des temps de parcours des liens et noeuds
SYMUBRUIT_EXPORT bool SYMUBRUIT_CDECL SymComputeCosts(int iNetworkId, const boost::posix_time::ptime & startTime, const boost::posix_time::ptime & endTime, const std::vector<SymuCore::MacroType*> & macroTypes, const std::map<SymuCore::SubPopulation*, SymuCore::CostFunction> & mapCostFunctions);

// Crée un utilisateur de transport en commun
SYMUBRUIT_EXPORT int SYMUBRUIT_CDECL SymCreatePublicTransportUser(int networkId, const std::string & startStop, const std::string & endStop, const std::string & lineName, double dbt, int externalUserID);

//récupère les informations pour le KShortestPath
SYMUBRUIT_EXPORT bool SYMUBRUIT_CDECL SymFillShortestPathParameters(bool bFillKParameters, bool bUseCommonalityFilter, double& dbAssignementAlpha, double& dbAssignementBeta, double& dbAssignementGamma,
                                                                    std::vector<double>& dbCommonalityFactorParameters, double& dbWardropTolerance,
                                                                    std::map<SymuCore::SubPopulation*, std::map<SymuCore::Origin*, std::map<SymuCore::Destination*, SymuCore::ListTimeFrame<double> > > >& KByOD);

//récupère les informations pour le Logit
SYMUBRUIT_EXPORT bool SYMUBRUIT_CDECL SymFillLogitParameters(std::map<SymuCore::SubPopulation*, std::map<SymuCore::Origin*, std::map<SymuCore::Destination*, SymuCore::ListTimeFrame<double> > > >& LogitByOD);

// récupère les informations d'offre aux entrées demandées (pour gestion des interfaces d'hybridation avec d'autres simulateurs)
SYMUBRUIT_EXPORT bool SYMUBRUIT_CDECL SymGetOffers(int networkID, const std::vector<std::string> & inputNames, std::vector<double> & offerValuesVect);

// met à jour la restriction de capacité d'une sortie pour un identifiant de route aval donné (hybridation avec SimuRes)
SYMUBRUIT_EXPORT bool SYMUBRUIT_CDECL SymSetCapacities(int networkID, const std::string & exitName, const std::string & downstreamRouteName , double dbCapacityValue);

#endif // USE_SYMUCORE

// Ménage
SYMUBRUIT_EXPORT bool SYMUBRUIT_CDECL SymRunDeleteTraffic(eveShared::TrafficState * &pTrafficEVE);

// ----- Mise à jour des scenari -----

// Mise à jour du scenario
SYMUBRUIT_EXPORT bool SYMUBRUIT_CDECL SymUpdateNetwork(std::string sXmlDataFile);

// Mise à jour d'un plan de feux du réseau
SYMUBRUIT_EXPORT int  SYMUBRUIT_CDECL SymSendSignalPlan(std::string sCDF, std::string sSP);

// Mise à jour d'une vitesse réglementaire d'un tronçon du réseau
SYMUBRUIT_EXPORT int  SYMUBRUIT_CDECL SymSendSpeedLimit(std::string sSection, std::string sVehType, double dbSpeedLimit);

// Affectation d'itinéraires pour une OD
SYMUBRUIT_EXPORT int SYMUBRUIT_CDECL SymSetRoutes(char* originId, char* destinationId, char* typeVeh, char** links[], double coeffs[], int iLength);

// ----- Pilotage des véhicules -----

// Création d'un véhicule
SYMUBRUIT_EXPORT int SYMUBRUIT_CDECL SymCreateVehicle(std::string sType, std::string sEntree, std::string sSortie, int nVoie, double dbt);
SYMUBRUIT_EXPORT int SYMUBRUIT_CDECL SymCreateVehicle(char* originId, char* destinationId, char* typeVeh, int nVoie, double dbt, const char*  links[]);
SYMUBRUIT_EXPORT int SYMUBRUIT_CDECL SymCreateVehicle(char* originId, char* destinationId, char* typeVeh, double dbt, const char*  links[]);
SYMUBRUIT_EXPORT int SYMUBRUIT_CDECL SymCreateVehicle(char* originId, char* destinationId, char* typeVeh, double dbt, const char*  links[], char * junctionName, int externalUserID);
SYMUBRUIT_EXPORT int SYMUBRUIT_CDECL SymCreateVehicle(int networkId, char* originId, char* destinationId, char* typeVeh, double dbt, const char*  links[], char * junctionName, int externalUserID, const char* strNextRouteId = NULL);

// Pilotage d'un véhicule
SYMUBRUIT_EXPORT int SYMUBRUIT_CDECL SymDriveVehicle(int nID, std::string sTroncon, int nVoie, double dbPos, bool bForce);

// Modification d'un itinéraire d'un véhicule
SYMUBRUIT_EXPORT int SYMUBRUIT_CDECL SymAlterRoute(int nIdVeh, char*  links[], int iLength);

// Retourne les itinéraires actuels d'une liste de véhicules
SYMUBRUIT_EXPORT char* SYMUBRUIT_CDECL SymGetVehiclesPaths(char*  vehiculeId[], int iLength);

// ----- Sorties complémentaires -----

// Génération du reseau EVE
SYMUBRUIT_EXPORT bool SYMUBRUIT_CDECL SymGenEveNetwork(eveShared::EveNetwork * &pNetwork);

// Génération de la liste des cellules acoustiques
SYMUBRUIT_EXPORT bool SYMUBRUIT_CDECL SymGenAcousticCells();

// ----- Sérialisation -----

// Sauvegarde de l'état courant vers un fichier XML
SYMUBRUIT_EXPORT void SYMUBRUIT_CDECL SymSaveState(char* sXmlDataFile);

// Chargement d'un état sauvegardé depuis un fichier XML
SYMUBRUIT_EXPORT void SYMUBRUIT_CDECL SymLoadState(char* sXmlDataFile);


// ----- Post-traitement -----

// Module de calcul de décélération
SYMUBRUIT_EXPORT bool SYMUBRUIT_CDECL SymDeceleration(double dbRate);

// Module de calcul des trajectories
SYMUBRUIT_EXPORT bool SYMUBRUIT_CDECL SymGenTrajectories();


// ----- JSON -------
SYMUBRUIT_EXPORT char* SYMUBRUIT_CDECL SymGetNetworkJSON();
SYMUBRUIT_EXPORT char* SYMUBRUIT_CDECL SymRunNextStepJSON(bool &bNEnd);
SYMUBRUIT_EXPORT char* SYMUBRUIT_CDECL SymGetVehiclePathJSON(int vehicleId);

SYMUBRUIT_EXPORT char* SYMUBRUIT_CDECL SymGetRemainingPathInfoJSON(int vehicleId);
SYMUBRUIT_EXPORT void SYMUBRUIT_CDECL SymRunDeleteStr(char* str);

// - Nouvelles interfaces pour la boucle de contrôle
SYMUBRUIT_EXPORT bool SYMUBRUIT_CDECL SymForbidLink(char* linkName);
SYMUBRUIT_EXPORT bool SYMUBRUIT_CDECL SymForbidLinks(char* linkNamesJSON);
SYMUBRUIT_EXPORT bool SYMUBRUIT_CDECL SymForbidZone(char* areaJSON);
// Pour facilité d'appel depuis du code C++ sans avoir à générer du JSON pour plusieurs liens
// (appeler plusieurs fois SymForbidLink serait a priori mauvais pour les perfs car on regenère le graphe à chaque appel).
// De plus, cette variante ne renvoie pas d'erreur si un lien n'est pas trouvé (permet d'éviter de rechercher dans SymuMaster
// en amont à quel simulateur appartient le lien dont le nom est donné).
SYMUBRUIT_EXPORT bool SYMUBRUIT_CDECL SymForbidLinks(int networkId, const std::vector<std::string> & linkNames);

SYMUBRUIT_EXPORT bool SYMUBRUIT_CDECL SymAllowLink(char* linkName);
SYMUBRUIT_EXPORT bool SYMUBRUIT_CDECL SymAllowLinks(char* linkNamesJSON);
SYMUBRUIT_EXPORT bool SYMUBRUIT_CDECL SymAllowZone(char* areaJSON);

SYMUBRUIT_EXPORT bool SYMUBRUIT_CDECL SymSetODPaths(char* pathsDefJSON);
