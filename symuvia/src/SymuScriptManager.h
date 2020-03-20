#pragma once
#ifndef SymuScriptManagerH
#define SymuScriptManagerH

class Reseau;
class TypeVehicule;
class Connexion;
class Tuyau;
class ZoneDeTerminaison;
class TripNode;

namespace estimation_od {
    class graph;
    class link;
    class node;
    struct graphPosition;
    typedef graphPosition origin;
    typedef graphPosition destination;
}

#include <vector>
#include <deque>
#include <map>

// Structure décrivant un itinéraire résultat
struct PathResult
{
    std::vector<Tuyau*> links;
    double dbCoeff;
    double dbCost;
    double dbPenalizedCost;
    double dbCommonalityFactor;
    bool bPredefined;
    std::string strName;
    Connexion* pJunction;
};

class SymuScriptManager
{
public:
    SymuScriptManager(Reseau * pNetwork);
    virtual ~SymuScriptManager();

    void ReinitGraphs();
    bool IsReinitialized();
    void ResetReinitialized();

    void UpdateCosts(const std::deque<TypeVehicule*> & lstTypes);

    void Invalidate(TypeVehicule* pTypeVehicule);

    void ComputePaths(TripNode* pOrigine, TripNode* pDestination, TypeVehicule * pTypeVeh, double dbInstant, int nbShortestPaths,
        int method, const std::vector<Tuyau*> & refPath,
        // Sorties
        std::vector<PathResult> & paths,
        std::map<std::vector<Tuyau*>, double> & MapFilteredItis);
    
    void ComputePaths(TripNode* pOrigine, TripNode* pDestination, TypeVehicule * pTypeVeh, double dbInstant, int nbShortestPaths,
        // Sorties
        std::vector<PathResult> & paths,
        std::map<std::vector<Tuyau*>, double> & MapFilteredItis);

    void ComputePaths(TripNode* pOrigine, Connexion* pDestination, TypeVehicule * pTypeVeh, double dbInstant, int nbShortestPaths,
        int method, const std::vector<Tuyau*> & refPath,
        // Sorties
        std::vector<PathResult> & paths,
        std::map<std::vector<Tuyau*>, double> & MapFilteredItis);

    void ComputePaths(TripNode* pOrigine, Connexion* pDestination, TypeVehicule * pTypeVeh, double dbInstant, int nbShortestPaths,
        // Sorties
        std::vector<PathResult> & paths,
        std::map<std::vector<Tuyau*>, double> & MapFilteredItis);

    void ComputePaths(Connexion* pOrigine, TripNode* pDestination, TypeVehicule * pTypeVeh, double dbInstant, int nbShortestPaths, Tuyau * pTuyauAmont,
        // Sorties
        std::vector<PathResult> & paths,
        std::map<std::vector<Tuyau*>, double> & MapFilteredItis);

    void ComputePaths(Connexion* pOrigine, TripNode* pDestination, TypeVehicule * pTypeVeh, double dbInstant, int nbShortestPaths, Tuyau * pTuyauAmont,
        int method, const std::vector<Tuyau*> & refPath,
        // Sorties
        std::vector<PathResult> & paths,
        std::map<std::vector<Tuyau*>, double> & MapFilteredItis);

    double ComputeCost(TypeVehicule* pTypeVehicule, const std::vector<Tuyau*> & path);

    double SetCost(TypeVehicule* pTypeVehicule, Tuyau * pLink, double dbNewCost);

private:

    bool GraphExists(TypeVehicule * pTypeVeh);
    void Initialize(TypeVehicule * pTypeVeh);

    void CheckGraph(TypeVehicule* pTypeVehicule);

    void ComputePaths(estimation_od::origin myOrigin, estimation_od::destination myDest, TypeVehicule * pTypeVeh, double dbInstant, int nbShortestPaths, estimation_od::link * pUpstreamLink,
        int method, const std::vector<Tuyau*> & refPath,
        // Sorties
        std::vector<PathResult> & paths,
        std::map<std::vector<Tuyau*>, double> & MapFilteredItis);

    estimation_od::node* GetNodeFromOrigin(TripNode * pOrigin, TypeVehicule * pTypeVeh);
    estimation_od::node* GetNodeFromDestination(TripNode * pDest, TypeVehicule * pTypeVeh);

private:

    // on gère un graphe par type de véhicule (SymuScript ne sait pas gérer différents types de véhicules)
    std::map<TypeVehicule*, estimation_od::graph*> m_Graphs;

    // flag indiquant si le réseau d'affectation a changé et doit être reconstruit si nécessaire
    std::map<TypeVehicule*, bool> m_bInvalid;

    // flag indiquant qu'un ReinitGraphs a été appelé.
    bool    m_bGraphsReinitialized;

    // Objet réseau associé
    Reseau *m_pNetwork;

    // Map permettant de faire le lien entre les tronçons de SymuScript et les tronçons SymuVia
    std::map<TypeVehicule*, std::map<estimation_od::link*, Tuyau*> > m_MapLinksToTuyaux;
    // Et en sens inverse
    std::map<TypeVehicule*, std::map<Tuyau*, estimation_od::link*> > m_MapTuyauxToLinks;
    // Idem pour les noeuds
    std::map<TypeVehicule*, std::map<Connexion*, estimation_od::node*> > m_MapConnexionsToNodes;
    // Et en sens inverse
    std::map<TypeVehicule*, std::map<estimation_od::node*, Connexion*> > m_MapNodesToConnexions;
    // Idem pour les zones de terminaison
    std::map<TypeVehicule*, std::map<ZoneDeTerminaison*, estimation_od::node*> > m_MapZonesToNodes;
    
};

#endif // SymuScriptManagerH