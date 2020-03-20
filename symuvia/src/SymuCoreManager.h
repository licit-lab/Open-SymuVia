#pragma once
#ifndef SymuCoreManagerH
#define SymuCoreManagerH

#include "ReaderWriterLock.h"

#include <boost/thread/recursive_mutex.hpp>

#include <string>
#include <vector>
#include <map>
#include <deque>

namespace SymuCore {
    class MultiLayersGraph;
    class Node;
    class SubPopulation;
    class Origin;
    class Destination;
    class Pattern;
}

class SymuCoreGraphBuilder;
class SymuViaTripNode;
class Tuyau;
class Connexion;
class Reseau;
class TypeVehicule;
class TripNode;
class PublicTransportLine;
class Arret;

// Structure décrivant un itinéraire résultat
struct PathResult
{
    std::vector<Tuyau*> links;
    double dbCost;
    double dbPenalizedCost;
    double dbCommonalityFactor;
    bool bPredefined;
    std::string strName;
    Connexion* pJunction;
};

class SymuCoreManager
{
public:
    SymuCoreManager(Reseau * pNetwork);
    virtual ~SymuCoreManager();

    void ForceGraphRefresh();

    void UpdateCosts(const std::deque<TypeVehicule*> & lstTypes, bool bIsRecursiveCall);

    void Invalidate();

    void ComputePaths(TripNode* pOrigine, TripNode* pDestination, TypeVehicule * pTypeVeh, double dbInstant, int nbShortestPaths,
        int method, const std::vector<Tuyau*> & refPath, const std::vector<Tuyau*> & linksToAvoid,
        // Sorties
        std::vector<PathResult> & paths,
        std::map<std::vector<Tuyau*>, double> & MapFilteredItis, bool bRecursiveCall);

	void ComputePaths(Connexion * pOrigine, TripNode * pDestination, TypeVehicule * pTypeVeh, double dbInstant, int nbShortestPaths, int method, const std::vector<Tuyau*>& refPath, const std::vector<Tuyau*>& linksToAvoid, std::vector<PathResult>& paths, std::map<std::vector<Tuyau*>, double>& MapFilteredItis, bool bRecursiveCall);

    void ComputePaths(TripNode* pOrigine, const std::vector<TripNode*> &lstDestinations, TypeVehicule * pTypeVeh, double dbInstant, int nbShortestPaths,
        int method, const std::vector<Tuyau*> & refPath, const std::vector<Tuyau*> & linksToAvoid,
        // Sorties
        std::vector<std::vector<PathResult> > & paths,
        std::vector<std::map<std::vector<Tuyau*>, double> > & MapFilteredItis, bool bRecursiveCall);

    void ComputePaths(const std::vector<TripNode*> lstOrigins, TripNode* pDestination, TypeVehicule * pTypeVeh, double dbInstant, int nbShortestPaths,
        int method, const std::vector<Tuyau*> & refPath, const std::vector<Tuyau*> & linksToAvoid,
        // Sorties
        std::vector<std::vector<PathResult> > & paths,
        std::vector<std::map<std::vector<Tuyau*>, double> > & MapFilteredItis, bool bRecursiveCall);
    
    void ComputePaths(TripNode* pOrigine, TripNode* pDestination, TypeVehicule * pTypeVeh, double dbInstant, int nbShortestPaths,
        // Sorties
        std::vector<PathResult> & paths,
        std::map<std::vector<Tuyau*>, double> & MapFilteredItis, bool bRecursiveCall);

    void ComputePaths(TripNode* pOrigine, const std::vector<TripNode*> lstDestinations, TypeVehicule * pTypeVeh, double dbInstant, int nbShortestPaths,
        // Sorties
        std::vector<std::vector<PathResult> > & paths,
        std::vector<std::map<std::vector<Tuyau*>, double> > & MapFilteredItis, bool bRecursiveCall);

    void ComputePaths(const std::vector<TripNode*> lstOrigins, TripNode* pDestination, TypeVehicule * pTypeVeh, double dbInstant, int nbShortestPaths,
        // Sorties
        std::vector<std::vector<PathResult> > & paths,
        std::vector<std::map<std::vector<Tuyau*>, double> > & MapFilteredItis, bool bRecursiveCall);

    void ComputePaths(TripNode* pOrigine, Connexion* pDestination, TypeVehicule * pTypeVeh, double dbInstant, int nbShortestPaths,
        int method, const std::vector<Tuyau*> & refPath, const std::vector<Tuyau*> & linksToAvoid,
        // Sorties
        std::vector<PathResult> & paths,
        std::map<std::vector<Tuyau*>, double> & MapFilteredItis, bool bRecursiveCall);

    void ComputePaths(const std::vector<TripNode*> lstOrigins, Connexion* pDestination, TypeVehicule * pTypeVeh, double dbInstant, int nbShortestPaths,
        int method, const std::vector<Tuyau*> & refPath, const std::vector<Tuyau*> & linksToAvoid,
        // Sorties
        std::vector<std::vector<PathResult> > & paths,
        std::vector<std::map<std::vector<Tuyau*>, double> > & MapFilteredItis, bool bRecursiveCall);

    void ComputePaths(TripNode* pOrigine, Connexion* pDestination, TypeVehicule * pTypeVeh, double dbInstant, int nbShortestPaths,
        // Sorties
        std::vector<PathResult> & paths,
        std::map<std::vector<Tuyau*>, double> & MapFilteredItis, bool bRecursiveCall);

    void ComputePaths(const std::vector<TripNode*> lstOrigins, Connexion* pDestination, TypeVehicule * pTypeVeh, double dbInstant, int nbShortestPaths,
        // Sorties
        std::vector<std::vector<PathResult> > & paths,
        std::vector<std::map<std::vector<Tuyau*>, double> > & MapFilteredItis, bool bRecursiveCall);

    void ComputePaths(Connexion* pOrigine, TripNode* pDestination, TypeVehicule * pTypeVeh, double dbInstant, int nbShortestPaths, Tuyau * pTuyauAmont,
        // Sorties
        std::vector<PathResult> & paths,
        std::map<std::vector<Tuyau*>, double> & MapFilteredItis, bool bRecursiveCall);

    void ComputePaths(Connexion* pOrigine, const std::vector<TripNode*> & lstDestinations, TypeVehicule * pTypeVeh, double dbInstant, int nbShortestPaths, Tuyau * pTuyauAmont,
        // Sorties
        std::vector<std::vector<PathResult> > & paths,
        std::vector<std::map<std::vector<Tuyau*>, double> > & MapFilteredItis, bool bRecursiveCall);

    void ComputePaths(const std::vector<std::pair<Connexion*, Tuyau*> > & lstOrigines, TripNode* pDestination, TypeVehicule * pTypeVeh, double dbInstant, int nbShortestPaths,
        // Sorties
        std::vector<std::vector<PathResult> > & paths,
        std::vector<std::map<std::vector<Tuyau*>, double> > & MapFilteredItis, bool bRecursiveCall);

    void ComputePaths(Connexion* pOrigine, TripNode* pDestination, TypeVehicule * pTypeVeh, double dbInstant, int nbShortestPaths, Tuyau * pTuyauAmont,
        int method, const std::vector<Tuyau*> & refPath, const std::vector<Tuyau*> & linksToAvoid,
        // Sorties
        std::vector<PathResult> & paths,
        std::map<std::vector<Tuyau*>, double> & MapFilteredItis, bool bRecursiveCall);

	void ComputePaths(Connexion* pOrigine, Connexion* pDestination, TypeVehicule * pTypeVeh, double dbInstant, int nbShortestPaths, Tuyau * pTuyauAmont,
		int method, const std::vector<Tuyau*> & refPath, const std::vector<Tuyau*> & linksToAvoid,
		// Sorties
		std::vector<PathResult> & paths,
		std::map<std::vector<Tuyau*>, double> & MapFilteredItis, bool bRecursiveCall);

    void ComputePaths(Connexion* pOrigine, const std::vector<TripNode*> & lstDestinations, TypeVehicule * pTypeVeh, double dbInstant, int nbShortestPaths, Tuyau * pTuyauAmont,
        int method, const std::vector<Tuyau*> & refPath, const std::vector<Tuyau*> & linksToAvoid,
        // Sorties
        std::vector<std::vector<PathResult> > & paths,
        std::vector<std::map<std::vector<Tuyau*>, double> > & MapFilteredItis, bool bRecursiveCall);

    double ComputeCost(TypeVehicule* pTypeVehicule, const std::vector<Tuyau*> & path, bool bIsRecursiveCall);

    //////////////////////////////////////////////////////////////////////////////////////
    // Méthodes spécifiques aux calculs multimodaux (pour le simulation game par ex)
    //////////////////////////////////////////////////////////////////////////////////////
    void ComputePathsPublicTransport(Connexion* pOrigin, TripNode* pDestination, double dbInstant, int nbShortestPaths,
        // Sorties
        std::vector<std::pair<double, std::vector<std::string> > > & paths);

    void ComputePathsPublicTransport(Connexion* pOrigin, const std::vector<TripNode*> & lstDestinations, double dbInstant, int nbShortestPaths,
        // Sorties
        std::vector<std::vector<std::pair<double, std::vector<std::string> > > > & paths);

    void ComputePathsPublicTransport(Arret* pOrigin, PublicTransportLine* previousLine, PublicTransportLine* pNextLine, TripNode* pDEstination, double dbInstant, int nbShortestPaths,
        // Sorties
        std::vector<std::pair<double, std::vector<std::string> > > & paths);

private:

    void ClearSubPopulations();

    bool GraphExists();
    void Initialize();

    void CheckGraph(bool bIsRecursiveCall);

    void ComputePaths(const std::vector<SymuCore::Origin> & lstOrigins, const std::vector<SymuCore::Destination> & lstDestinations, TypeVehicule * pTypeVeh, double dbInstant, int nbShortestPaths,
        int method, const std::vector<Tuyau*> & refPath, const std::vector<Tuyau*> & linksToAvoid,
        // Sorties
        std::vector<std::vector<PathResult> > & paths,
        std::vector<std::map<std::vector<Tuyau*>, double> > & MapFilteredItis);

    void ComputePathsPublicTransport(SymuCore::Origin origin, const std::vector<SymuCore::Destination> & lstDestinations, const std::vector<SymuCore::Pattern*> & patternsToAvoid, double dbInstant, int nbShortestPaths,
        // Sorties
        std::vector<std::vector<std::pair<double, std::vector<std::string> > > > & paths);

    SymuCore::Node* GetNodeFromOrigin(TripNode * pOrigin);
    SymuCore::Node* GetNodeFromDestination(TripNode * pDest);

    void FillOriginFromTripNode(TripNode * pOrigin, SymuCore::Origin & origin);
    void FillDestinationFromTripNode(TripNode * pDest, SymuCore::Destination & destination);

private:

    // on gère un graphe par type de véhicule (SymuScript ne sait pas gérer différents types de véhicules)
    SymuCore::MultiLayersGraph * m_pGraph;

    // flag indiquant si le réseau d'affectation a changé et doit être reconstruit si nécessaire
    bool m_bInvalid;

    // Objet réseau associé
    Reseau *m_pNetwork;

    // Classe utilitaire pour la construction d'un graphe SymuCore
    SymuCoreGraphBuilder * m_pGraphBuilder;

    // Association entre types de véhicules et sous populations SymuCore
    std::map<TypeVehicule *, SymuCore::SubPopulation*> m_mapSubPopulations;

    // Sous population spécifique pour les demandes d'itinéraires en transports en commun uniquement
    SymuCore::SubPopulation * m_pPublicTransportOnlySubPopulation;

    // Association entre les tuyaux et les Patterns SymuCore pour accès rapide
    std::map<Tuyau*, SymuCore::Pattern*> m_MapTuyauxToPatterns;

    // Association entre les Nodes et les SymuViaTripNodes pour accès rapide
    std::map<SymuCore::Node*, SymuViaTripNode*> m_MapSymuViaTripNodeByNode;

    // Objets de synchro pour interdire les opérations parallèles incompatibles
    ReaderWriterLock m_Lock;
    boost::recursive_mutex m_Mutex;
    
};

#endif // SymuCoreManagerH