#include "stdafx.h"
#include "SymuScriptManager.h"

#include "reseau.h"
#include "Connection.h"
#include "SystemUtil.h"
#include "tuyau.h"
#include "ZoneDeTerminaison.h"
#include "TronconDestination.h"
#include "TronconOrigine.h"
#include "ControleurDeFeux.h"
#include "Logger.h"

#include <graph.h>

using namespace estimation_od;

SymuScriptManager::SymuScriptManager(Reseau * pNetwork)
{
    m_pNetwork = pNetwork;
    m_bGraphsReinitialized = false;
}

SymuScriptManager::~SymuScriptManager()
{
    std::map<TypeVehicule*, estimation_od::graph*>::iterator iter;
    for(iter = m_Graphs.begin(); iter != m_Graphs.end(); ++iter)
    {
        delete iter->second;
    }
}

bool SymuScriptManager::GraphExists(TypeVehicule * pTypeVeh)
{
    return m_Graphs.find(pTypeVeh) != m_Graphs.end();
}

void SymuScriptManager::Initialize(TypeVehicule * pTypeVeh)
{
    std::map<estimation_od::link*, Tuyau*> & mapLinksToTuyaux = m_MapLinksToTuyaux[pTypeVeh];
    std::map<Tuyau*,estimation_od::link*> & mapTuyauxToLinks = m_MapTuyauxToLinks[pTypeVeh];
    std::map<Connexion*,estimation_od::node*> & mapConnexionsToNodes = m_MapConnexionsToNodes[pTypeVeh];
    std::map<estimation_od::node*, Connexion*> & mapNodesToConnexions = m_MapNodesToConnexions[pTypeVeh];
    std::map<ZoneDeTerminaison*,estimation_od::node*> & mapZonesToNodes = m_MapZonesToNodes[pTypeVeh];
    
    
    mapLinksToTuyaux.clear();
    mapTuyauxToLinks.clear();
    mapConnexionsToNodes.clear();
    mapNodesToConnexions.clear();
    mapZonesToNodes.clear();
    graph * pGraph = new graph();

    // Définition du graphe d'affectation
    estimation_od::link* pLink;
    estimation_od::node* pNode;
    Tuyau * pTuyau, * pTuyauAv;
    Connexion * pConnexion;
    for(size_t iNode = 0; iNode < m_pNetwork->m_LstUserCnxs.size(); iNode++)
    {
        pConnexion = m_pNetwork->m_LstUserCnxs[iNode];
        std::string connectionId = pConnexion->GetID();
        pNode = pGraph->AddNode(connectionId.c_str(), UNKNOWN); // rmq. : unknown car pas besoin de distinguer les types de noeuds pour l'utilisation faite de SYmuScript par SymuBruit
        mapConnexionsToNodes[pConnexion] = pNode;
        mapNodesToConnexions[pNode] = pConnexion;

        std::vector<Tuyau*> upstreamLinks, downstreamLinks;

        for(size_t iLink = 0; iLink < pConnexion->m_LstTuyAssAm.size(); iLink++)
        {
            pTuyau = pConnexion->m_LstTuyAssAm[iLink];
            if(!pTuyau->IsInterdit(pTypeVeh, m_pNetwork->m_dbInstSimu))
            {
                // on n'ajoute le tuyau que s'il n'a pas déjà été ajouté
                if(mapTuyauxToLinks.find(pTuyau) == mapTuyauxToLinks.end())
                {
					std::vector<double> x, y;
					pTuyau->BuildLineStringXY(x, y);
                    std::string linkId = pTuyau->GetLabel();
                    pLink = pGraph->AddLink(linkId.c_str(), 0, pTuyau->GetFunctionalClass(), x, y);
                    mapLinksToTuyaux[pLink] = pTuyau;
                    mapTuyauxToLinks[pTuyau] = pLink;
                }
                else
                {
                    pLink = mapTuyauxToLinks[pTuyau];
                }

                // Connexion du noeud au tuyau amont
                pGraph->ConnectInputLink(pNode, pLink);
                upstreamLinks.push_back(pTuyau);
            }
        }

        for(size_t iLink = 0; iLink < pConnexion->m_LstTuyAssAv.size(); iLink++)
        {
            pTuyau = pConnexion->m_LstTuyAssAv[iLink];

            if(!pTuyau->IsInterdit(pTypeVeh, m_pNetwork->m_dbInstSimu))
            {
                // on n'ajoute le tuyau que s'il n'a pas déjà été ajouté
                if(mapTuyauxToLinks.find(pTuyau) == mapTuyauxToLinks.end())
                {
					std::vector<double> x, y;
					pTuyau->BuildLineStringXY(x, y);
                    std::string linkId = pTuyau->GetLabel();
                    pLink = pGraph->AddLink(linkId.c_str(), 0, pTuyau->GetFunctionalClass(), x, y);
                    mapLinksToTuyaux[pLink] = pTuyau;
                    mapTuyauxToLinks[pTuyau] = pLink;
                }
                else
                {
                    pLink = mapTuyauxToLinks[pTuyau];
                }
                
                // Connexion du noeud au tuyau amont
                pGraph->ConnectOutputLink(pNode, pLink);
                downstreamLinks.push_back(pTuyau);
            }
        }

        // définition des mouvements interdits
        for(size_t iLink = 0; iLink < upstreamLinks.size(); iLink++)
        {
            pTuyau = upstreamLinks[iLink];
            for(size_t iLinkAv = 0; iLinkAv < downstreamLinks.size(); iLinkAv++)
            {
                pTuyauAv = downstreamLinks[iLinkAv];
                if(!pConnexion->IsMouvementAutorise(pTuyau, pTuyauAv, pTypeVeh, NULL))
                {
                    pGraph->AddForbiddenMove(pNode, mapTuyauxToLinks[pTuyau], mapTuyauxToLinks[pTuyauAv]);
                }
            }
        }
    }

    // Ajout de la définition des zones de terminaison
    ZoneDeTerminaison * pZone;
    std::map<Tuyau*, estimation_od::link*>::const_iterator iter;
    for(size_t iZone = 0; iZone < m_pNetwork->Liste_zones.size(); iZone++)
    {
        pZone = m_pNetwork->Liste_zones[iZone];
        std::string sID = pZone->GetOutputID();
        std::vector<estimation_od::link*> upstreamLinks;
        for(size_t iLink = 0; iLink < pZone->GetInputConnexion()->m_LstTuyAssAm.size(); iLink++)
        {
            iter = mapTuyauxToLinks.find(pZone->GetInputConnexion()->m_LstTuyAssAm[iLink]);
            if(iter != mapTuyauxToLinks.end())
            {
                upstreamLinks.push_back(iter->second);
            }
        }
        std::vector<estimation_od::link*> downstreamLinks;
        for(size_t iLink = 0; iLink < pZone->GetOutputConnexion()->m_LstTuyAssAv.size(); iLink++)
        {
            iter = mapTuyauxToLinks.find(pZone->GetOutputConnexion()->m_LstTuyAssAv[iLink]);
            if(iter != mapTuyauxToLinks.end())
            {
                downstreamLinks.push_back(iter->second);
            }
        }
        pNode = pGraph->AddZone(sID.c_str(), upstreamLinks, downstreamLinks);
        mapZonesToNodes[pZone] = pNode;
    }

    pGraph->SetDefaultTetaLogit(m_pNetwork->GetTetaLogit());
    assignment_mode mode = m_pNetwork->GetMode() == 1 ? ASSIGNMENT_MODE_LOGIT : ASSIGNMENT_MODE_WARDROP;
    pGraph->SetAssignmentMode(mode);
    pGraph->SetWardropTolerance(m_pNetwork->GetWardropTolerance());
    if (m_pNetwork->GetShortestPathHeuristic() == Reseau::HEURISTIC_NONE)
    {
        pGraph->SetShortestPathHeuristic(estimation_od::HEURISTIC_NONE);
    }
    else
    {
        pGraph->SetShortestPathHeuristic(estimation_od::HEURISTIC_EUCLIDIAN);
    }
    pGraph->SetHeuristicGamma(m_pNetwork->GetHeuristicGamma());
    pGraph->SetAssignmentAStarBeta(m_pNetwork->GetAStarBeta());
    pGraph->SetAssignmentAlpha(m_pNetwork->GetDijkstraAlpha());
    pGraph->SetAssignmentBeta(m_pNetwork->GetDijkstraBeta());
    pGraph->SetAssignmentGamma(m_pNetwork->GetDijkstraGamma());
    pGraph->SetAssignmentCommonalityFilter(m_pNetwork->GetCommonalityFilter());
    pGraph->SetDefaultAssignmentCommonalityParameters(m_pNetwork->GetCommonalityAlpha(), m_pNetwork->GetCommonalityBeta(), m_pNetwork->GetCommonalityGamma());

    // Définition des paramètres OD
    SymuViaTripNode * pOrigin;
    estimation_od::node* pNodeDest;
    double dbStartTime, dbEndTime;
    long startTimeMS, endTimeMS;
    for(size_t iOrigin = 0; iOrigin < m_pNetwork->Liste_origines.size(); iOrigin++)
    {
        pOrigin = m_pNetwork->Liste_origines[iOrigin];
        pNode = GetNodeFromOrigin(pOrigin, pTypeVeh);

        const std::deque<TimeVariation<SimMatOrigDest> > & lstCoeffDest = m_pNetwork->IsInitSimuTrafic() ? pOrigin->GetLstCoeffDest(pTypeVeh) : pOrigin->GetLstCoeffDestInit(pTypeVeh);

        // Pour chaque variante temporelle ...
        dbStartTime = 0;
        for(size_t iVar = 0; iVar < lstCoeffDest.size(); iVar++)
        {
            const TimeVariation<SimMatOrigDest> & coeffDestVar = lstCoeffDest[iVar];
            if(coeffDestVar.m_pData)
            {
                const std::deque<VectDest*> & coeffDest = coeffDestVar.m_pData->MatOrigDest;

                // définition de la plage temporelle correspondante au format SymuScript
                if(coeffDestVar.m_pPlage)
                {
                    dbStartTime = coeffDestVar.m_pPlage->m_Debut;
                    dbEndTime = coeffDestVar.m_pPlage->m_Fin;
                }
                else
                {
                    dbEndTime = dbStartTime + coeffDestVar.m_dbPeriod;
                    
                }
                startTimeMS = (long)(dbStartTime*1000.0+0.5);
                endTimeMS = (long)(dbEndTime*1000.0+0.5);

                // Pour chaque destination ...
                for(size_t iDest = 0; iDest < coeffDest.size(); iDest++)
                {
                    VectDest * pVectDest = coeffDest[iDest];
                    pNodeDest = GetNodeFromDestination(pVectDest->pDest, pTypeVeh);

                    // remarque : on ne définit pas le nombre de plus courts chemins demandés car on le passe à chaque appel à ComputePaths.

                    if(pVectDest->bHasTetaLogit)
                    {
                        if(pGraph->SetTetaLogit(pNode, pNodeDest, pVectDest->dbTetaLogit, startTimeMS, endTimeMS) != estimation_od::SUCCESS)
                        {
                            m_pNetwork->log() << Logger::Error << "Echec de la définition du teta logit pour le coupe OD " << pOrigin->GetOutputID() << " -> " << pVectDest->pDest->GetInputID() << ", les plages temporelles se chevauchent!";
                        }
                    }

                    if(pVectDest->bHasCustomCommonalityParameters)
                    {
                        if(pGraph->SetAssignmentCommonalityParameters(pNode, pNodeDest, pVectDest->dbCommonalityAlpha, pVectDest->dbCommonalityBeta, pVectDest->dbCommonalityGamma, startTimeMS, endTimeMS) != estimation_od::SUCCESS)
                        {
                            m_pNetwork->log() << Logger::Error << "Echec de la définition des paramètres du commonality filter pour le coupe OD " << pOrigin->GetOutputID() << " -> " << pVectDest->pDest->GetInputID() << ", les plages temporelles se chevauchent!";
                        }
                    }
                    
                    for(size_t iPath = 0; iPath < pVectDest->lstItineraires.size(); iPath++)
                    {
                        const std::pair<double, std::pair<std::pair<std::vector<Tuyau*>, Connexion*>, std::string> > & predefinedPath = pVectDest->lstItineraires[iPath];
                        std::vector<estimation_od::link*> links;
                        for(size_t iLink = 0; iLink < predefinedPath.second.first.first.size(); iLink++)
                        {
                            links.push_back(mapTuyauxToLinks.at(predefinedPath.second.first.first[iLink]));
                        }
                        estimation_od::node * pJunction = NULL;
                        if (predefinedPath.second.first.second != NULL)
                        {
                            pJunction = mapConnexionsToNodes.at(predefinedPath.second.first.second);
                        }
                        if (pGraph->DefineODPredefinedPath(pNode, pNodeDest, links, pJunction, predefinedPath.first, predefinedPath.second.second, startTimeMS, endTimeMS) != estimation_od::SUCCESS)
                        {
                            m_pNetwork->log() << Logger::Error << "Echec de la définition des itinéraires prédéfinis pour couple OD " << pOrigin->GetOutputID() << " -> " << pVectDest->pDest->GetInputID() << ", les plages temporelles se chevauchent!";
                        }
                    }
                }
                dbStartTime = dbEndTime;
            }
        }
    }

    // Nettoyage de l'ancienne définition du réseau
    if(GraphExists(pTypeVeh))
    {
        delete m_Graphs[pTypeVeh];
    }

    m_Graphs[pTypeVeh] = pGraph;
    m_bInvalid[pTypeVeh] = false;

    // Calcul des couts et pénalisations
    std::deque<TypeVehicule*> lstTypes(1, pTypeVeh);
    UpdateCosts(lstTypes);
}

void SymuScriptManager::ReinitGraphs()
{
    std::map<TypeVehicule*, estimation_od::graph*>::iterator iter;
    for(iter = m_Graphs.begin(); iter != m_Graphs.end(); ++iter)
    {
        Initialize(iter->first);
    }
    m_bGraphsReinitialized = true;
}

void SymuScriptManager::UpdateCosts(const std::deque<TypeVehicule*> & lstTypes)
{
    Tuyau * pLink;
    TypeVehicule * pTV;
    double dbCost;
    std::map<Tuyau*, estimation_od::link*>::const_iterator iterLink, iterLinkAval;
    for(size_t iTV = 0; iTV < lstTypes.size(); iTV++)
    {
        pTV = lstTypes[iTV];

        if(!GraphExists(pTV))
        {
            Initialize(pTV);
        }

        graph * pGraph = m_Graphs[pTV];

        const std::map<Tuyau*, estimation_od::link*> & mapTuyauxToLinks = m_MapTuyauxToLinks[pTV];
        
        for(iterLink = mapTuyauxToLinks.begin(); iterLink != mapTuyauxToLinks.end(); ++iterLink)
        {
            pLink = iterLink->first;
            dbCost = pLink->ComputeCost(m_pNetwork->m_dbInstSimu, pTV, true);
            pGraph->SetCost(iterLink->second, dbCost);
        }

        std::map<Connexion*,estimation_od::node*> & mapConnexionsToNodes = m_MapConnexionsToNodes[pTV];
        std::map<Connexion*,estimation_od::node*>::const_iterator iterNodes;
        Connexion * pConnexion;
        for(iterNodes = mapConnexionsToNodes.begin(); iterNodes != mapConnexionsToNodes.end(); ++iterNodes)
        {
            pConnexion = iterNodes->first;
            for(size_t iLink = 0; iLink < pConnexion->m_LstTuyAssAm.size(); iLink++)
            {
                iterLink = mapTuyauxToLinks.find(pConnexion->m_LstTuyAssAm[iLink]);
                if(iterLink != mapTuyauxToLinks.end())
                {
                    for(size_t iLinkAval = 0; iLinkAval < pConnexion->m_LstTuyAssAv.size(); iLinkAval++)
                    {
                        iterLinkAval = mapTuyauxToLinks.find(pConnexion->m_LstTuyAssAv[iLinkAval]);
                        if(iterLinkAval != mapTuyauxToLinks.end())
                        {
                            double dbPenalty = pConnexion->ComputeCost(pTV, iterLink->first, iterLinkAval->first);

                            // Pas la peine de polluer avec des pénalités non nulles
                            if(dbPenalty > 0)
                            {
                                pGraph->SetPenalty(iterNodes->second, iterLink->second, iterLinkAval->second, dbPenalty);
                            }
                        }
                    }
                }
            }
        }
    }

    m_pNetwork->InvalidatePathRelatedCaches();
}

void SymuScriptManager::Invalidate(TypeVehicule* pTypeVehicule)
{
    m_bInvalid[pTypeVehicule] = true;
}

void SymuScriptManager::CheckGraph(TypeVehicule* pTypeVehicule)
{
    if(!GraphExists(pTypeVehicule))
    {
        // Premier appel : il faut initialiser le graphe
        Initialize(pTypeVehicule);
    }
    else if(m_bInvalid[pTypeVehicule])
    {
        // Dans ce cas, on demande à calculer des itinéraires alors qu'une modification du réseau a eu lieu, impactant le graphe mais ne déclenchant pas automatiquement de recalcul de l'affectation (modification
        // des mouvements autorisés). On recalcul le nouveau graphe en conservant les couts actuels des différents liens avant de procéder au calcul d'itinéraire.
        graph * pGraph = m_Graphs[pTypeVehicule];
        const std::map<Tuyau*, estimation_od::link*> & mapTuyauxToLinks = m_MapTuyauxToLinks[pTypeVehicule];

        // On note les couts définis dans le graphe précédent
        std::map<Tuyau*, double> costs;
        std::map<Tuyau*, estimation_od::link*>::const_iterator iter;
        for(iter = mapTuyauxToLinks.begin(); iter != mapTuyauxToLinks.end(); ++iter)
        {
            costs[iter->first] = pGraph->GetCost(iter->second);
        }

        Initialize(pTypeVehicule);

        // On restaure les couts dans le nouveau graphe
        pGraph = m_Graphs[pTypeVehicule];
        std::map<Tuyau*, double>::const_iterator iterCost, iterCostEnd = costs.end();
        for(iter = mapTuyauxToLinks.begin(); iter != mapTuyauxToLinks.end(); ++iter)
        {
            iterCost = costs.find(iter->first);
            if(iterCost != iterCostEnd)
            {
                pGraph->SetCost(iter->second, iterCost->second);
            }
            else
            {
                // Le tuyau était "interdit" avant la MAJ du graphe, on n'a donc pas de cout déjà calculé, donc on le calcule.
                pGraph->SetCost(iter->second, iter->first->ComputeCost(m_pNetwork->m_dbInstSimu, pTypeVehicule, true));
            }
        }

        m_pNetwork->InvalidatePathRelatedCaches();
    }
}

void SymuScriptManager::ComputePaths(TripNode * pOrigine, TripNode * pDestination, TypeVehicule * pTypeVeh, double dbInstant, int nbShortestPaths,
    int method, const std::vector<Tuyau*> & refPath,
    std::vector<PathResult> & paths, std::map<std::vector<Tuyau*>, double> & MapFilteredItis)
{
    CheckGraph(pTypeVeh);

    estimation_od::origin myOrigin;
    if (pOrigine->GetOutputPosition().IsLink())
    {
        myOrigin.pLink = m_MapTuyauxToLinks[pTypeVeh].at(pOrigine->GetOutputPosition().GetLink());
    }
    else
    {
        myOrigin.pNode = GetNodeFromOrigin(pOrigine, pTypeVeh);
    }


    estimation_od::destination myDest;
    if (pDestination->GetInputPosition().IsLink())
    {
        myDest.pLink = m_MapTuyauxToLinks[pTypeVeh].at(pDestination->GetInputPosition().GetLink());
    }
    else
    {
        myDest.pNode = GetNodeFromDestination(pDestination, pTypeVeh);
    }
    ComputePaths(myOrigin, myDest, pTypeVeh, dbInstant, nbShortestPaths, NULL, method, refPath, paths, MapFilteredItis);
}

void SymuScriptManager::ComputePaths(TripNode * pOrigine, TripNode * pDestination, TypeVehicule * pTypeVeh, double dbInstant, int nbShortestPaths,
    std::vector<PathResult> & paths, std::map<std::vector<Tuyau*>, double> & MapFilteredItis)
{
    std::vector<Tuyau*> placeHolder;
    ComputePaths(pOrigine, pDestination, pTypeVeh, dbInstant, nbShortestPaths, 0, placeHolder, paths, MapFilteredItis);
}

void SymuScriptManager::ComputePaths(TripNode * pOrigine, Connexion * pDestination, TypeVehicule * pTypeVeh, double dbInstant, int nbShortestPaths,
    int method, const std::vector<Tuyau*> & refPath,
    std::vector<PathResult> & paths, std::map<std::vector<Tuyau*>, double> & MapFilteredItis)
{
    CheckGraph(pTypeVeh);

    estimation_od::origin myOrigin;
    if (pOrigine->GetOutputPosition().IsLink())
    {
        myOrigin.pLink = m_MapTuyauxToLinks[pTypeVeh].at(pOrigine->GetOutputPosition().GetLink());
    }
    else
    {
        myOrigin.pNode = GetNodeFromOrigin(pOrigine, pTypeVeh);
    }

    estimation_od::destination myDest;
    myDest.pNode = m_MapConnexionsToNodes[pTypeVeh].at(pDestination);

    ComputePaths(myOrigin, myDest, pTypeVeh, dbInstant, nbShortestPaths, NULL, method, refPath, paths, MapFilteredItis);
}

void SymuScriptManager::ComputePaths(TripNode * pOrigine, Connexion * pDestination, TypeVehicule * pTypeVeh, double dbInstant, int nbShortestPaths,
    std::vector<PathResult> & paths, std::map<std::vector<Tuyau*>, double> & MapFilteredItis)
{
    std::vector<Tuyau*> placeHolder;
    ComputePaths(pOrigine, pDestination, pTypeVeh, dbInstant, nbShortestPaths, 0, placeHolder, paths, MapFilteredItis);
}

void SymuScriptManager::ComputePaths(Connexion * pOrigine, TripNode * pDestination, TypeVehicule * pTypeVeh, double dbInstant, int nbShortestPaths,
    Tuyau * pTuyauAmont,
    std::vector<PathResult> & paths, std::map<std::vector<Tuyau*>, double> & MapFilteredItis)
{
    std::vector<Tuyau*> placeHolder;
    ComputePaths(pOrigine, pDestination, pTypeVeh, dbInstant, nbShortestPaths, pTuyauAmont, 0, placeHolder, paths, MapFilteredItis);
}


void SymuScriptManager::ComputePaths(Connexion * pOrigine, TripNode * pDestination, TypeVehicule * pTypeVeh, double dbInstant, int nbShortestPaths,
    Tuyau * pTuyauAmont, int method, const std::vector<Tuyau*> & refPath,
    std::vector<PathResult> & paths, std::map<std::vector<Tuyau*>, double> & MapFilteredItis)
{
    CheckGraph(pTypeVeh);

    estimation_od::origin myOrigin;
    myOrigin.pNode = m_MapConnexionsToNodes[pTypeVeh].at(pOrigine);
    
    estimation_od::destination myDest;
    if(pDestination->GetInputPosition().IsLink())
    {
		if( m_MapTuyauxToLinks[pTypeVeh].find( pDestination->GetInputPosition().GetLink()) == m_MapTuyauxToLinks[pTypeVeh].end() )
		{
			m_pNetwork->log() << Logger::Error << "Destination non présente dans le graphe d'affectation " << pDestination->GetInputPosition().GetLink()->GetLabel() << std::endl;
            m_pNetwork->log() << Logger::Info; // rebascule en mode INFO pour ne pas avoir à reprendre tous les appels aux log en précisant que c'est des INFO. à supprimer si on reprend tous les appels au log.
			return;
		}

        myDest.pLink = m_MapTuyauxToLinks[pTypeVeh].at(pDestination->GetInputPosition().GetLink());
    }
    else
    {
        myDest.pNode = GetNodeFromDestination(pDestination, pTypeVeh);
    }

    estimation_od::link * pUpstreamLink;
    if(pTuyauAmont)
    {
        pUpstreamLink = m_MapTuyauxToLinks.at(pTypeVeh).at(pTuyauAmont);
    }
    else
    {
        pUpstreamLink = NULL;
    }

    ComputePaths(myOrigin, myDest, pTypeVeh, dbInstant, nbShortestPaths, pUpstreamLink, method, refPath, paths, MapFilteredItis);
}


void SymuScriptManager::ComputePaths(estimation_od::origin myOrigin, estimation_od::destination myDest, TypeVehicule * pTypeVeh, double dbInstant, int nbShortestPaths,
    estimation_od::link * pUpstreamLink, int method, const std::vector<Tuyau*> & refPath,
    std::vector<PathResult> & paths, std::map<std::vector<Tuyau*>, double> & MapFilteredItis)
{
    const std::map<estimation_od::link*,Tuyau*> & mapLinksToTuyaux = m_MapLinksToTuyaux[pTypeVeh];
    const std::map<Tuyau*,estimation_od::link*> & mapTuyauxToLinks = m_MapTuyauxToLinks[pTypeVeh];
    const std::map<estimation_od::node*, Connexion*> & mapNodesToConnexions = m_MapNodesToConnexions[pTypeVeh];

    std::vector<estimation_od::link*> refPathSymuScript;
    for(size_t i = 0; i < refPath.size(); i++)
    {
        refPathSymuScript.push_back(mapTuyauxToLinks.at(refPath[i]));
    }

    std::map<std::vector<estimation_od::link*>, double> MapFilteredItisTmp;
    std::vector<estimation_od::path> computedPaths = m_Graphs[pTypeVeh]->ComputePaths((long)(dbInstant*1000.0+0.5), myOrigin, myDest, nbShortestPaths, pUpstreamLink, method, refPathSymuScript, MapFilteredItisTmp);
    for(size_t i = 0; i < computedPaths.size(); i++)
    {
        const estimation_od::path & myPath = computedPaths[i];
        PathResult path;
        std::vector<Tuyau*> lstTuyaux(myPath.links.size());
        for(size_t iLink = 0; iLink < lstTuyaux.size(); iLink++)
        {
            path.links.push_back(mapLinksToTuyaux.at(myPath.links[iLink]));
        }
        path.dbCoeff = myPath.coeff;
        path.dbCost = myPath.cost;
        path.dbPenalizedCost = myPath.penalizedCost;
        path.dbCommonalityFactor = myPath.commonalityFactor;
        path.bPredefined = myPath.predefined;
        path.strName = myPath.name;
        path.pJunction = myPath.pJunction == NULL ? NULL : mapNodesToConnexions.at(myPath.pJunction);
        paths.push_back(path);
    }
    std::map<std::vector<estimation_od::link*>, double>::const_iterator iter;
    for(iter = MapFilteredItisTmp.begin(); iter != MapFilteredItisTmp.end(); ++iter)
    {
        std::vector<Tuyau*> lstTuyaux(iter->first.size());
        for(size_t iLink = 0; iLink < iter->first.size(); iLink++)
        {
            lstTuyaux[iLink] = mapLinksToTuyaux.at(iter->first[iLink]);
        }
        MapFilteredItis[lstTuyaux] = iter->second;
    }
}

double SymuScriptManager::ComputeCost(TypeVehicule* pTypeVehicule, const std::vector<Tuyau*> & path)
{
    assert(GraphExists(pTypeVehicule));
    graph * pGraph = m_Graphs[pTypeVehicule];
    const std::map<Tuyau*,estimation_od::link*> & mapTuyauxToLinks = m_MapTuyauxToLinks[pTypeVehicule];
    std::vector<estimation_od::link*> links;
    for(size_t i = 0; i < path.size(); i++)
    {
        links.push_back(mapTuyauxToLinks.at(path.at(i)));
    }
    return pGraph->ComputeCost(links);
}

double SymuScriptManager::SetCost(TypeVehicule* pTypeVehicule, Tuyau * pLink, double dbNewCost)
{
    // modification arbitraire du cout d'un arc (utilisé pour feinter le calculer d'itinéraire
    // et éviter un arc par exemple, sans modifier le graphe découlement associé)
    assert(GraphExists(pTypeVehicule));
    graph * pGraph = m_Graphs[pTypeVehicule];
    const std::map<Tuyau*, estimation_od::link*> & mapTuyauxToLinks = m_MapTuyauxToLinks[pTypeVehicule];
    estimation_od::link * pGraphLink = mapTuyauxToLinks.at(pLink);
    double dbOldCost = pGraph->GetCost(pGraphLink);
    pGraph->SetCost(pGraphLink, dbNewCost);
    return dbOldCost;
    
}

estimation_od::node* SymuScriptManager::GetNodeFromOrigin(TripNode * pOrigin, TypeVehicule * pTypeVeh)
{
    estimation_od::node* result;
    ZoneDeTerminaison * pZone = dynamic_cast<ZoneDeTerminaison*>(pOrigin);
    if(pZone)
    {
        result = m_MapZonesToNodes[pTypeVeh][pZone];
    }
    else
    {
        result = m_MapConnexionsToNodes[pTypeVeh][pOrigin->GetOutputPosition().GetConnection()];
    }
    return result;
}

estimation_od::node* SymuScriptManager::GetNodeFromDestination(TripNode * pDest, TypeVehicule * pTypeVeh)
{
    estimation_od::node* result;
    ZoneDeTerminaison * pZone = dynamic_cast<ZoneDeTerminaison*>(pDest);
    if(pZone)
    {
        result = m_MapZonesToNodes[pTypeVeh][pZone];
    }
    else
    {
        result = m_MapConnexionsToNodes[pTypeVeh][pDest->GetInputPosition().GetConnection()];
    }
    return result;
}

bool SymuScriptManager::IsReinitialized()
{
    return m_bGraphsReinitialized;
}

void SymuScriptManager::ResetReinitialized()
{
    m_bGraphsReinitialized = false;
}

