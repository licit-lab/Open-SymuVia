#include "stdafx.h"
#include "SymuCoreManager.h"

#include "reseau.h"
#include "SymuCoreGraphBuilder.h"
#include "Logger.h"
#include "SymuViaDrivingPattern.h"
#include "ConnectionDrivingPenalty.h"
#include "TronconDrivingPattern.h"
#include "tuyau.h"
#include "TronconOrigine.h"
#include "Affectation.h"
#include "SymuViaPublicTransportPattern.h"
#include "ArretPublicTransportPenalty.h"
#include "arret.h"

#include <Demand/Origin.h>
#include <Demand/Destination.h>
#include <Graph/Model/MultiLayersGraph.h>
#include <Graph/Model/Link.h>
#include <Graph/Algorithms/KShortestPaths.h>
#include <Graph/Algorithms/Dijkstra.h>
#include <Demand/Population.h>
#include <Demand/SubPopulation.h>
#include <Demand/MacroType.h>
#include <Demand/VehicleType.h>
#include <Demand/ValuetedPath.h>
#include <Demand/PredefinedPath.h>
#include <Graph/Model/PublicTransport/PublicTransportLine.h>
#include <Graph/Model/PublicTransport/WalkingPattern.h>

#include <boost/log/trivial.hpp>


SymuCoreManager::SymuCoreManager(Reseau * pNetwork)
{
    m_pGraph = NULL;
    m_pNetwork = pNetwork;
    m_pGraphBuilder = NULL;
    m_pPublicTransportOnlySubPopulation = NULL;
}

SymuCoreManager::~SymuCoreManager()
{
    ClearSubPopulations();

    if (m_pGraph)
    {
        delete m_pGraph;
    }
    if (m_pGraphBuilder)
    {
        delete m_pGraphBuilder;
    }
}

bool SymuCoreManager::GraphExists()
{
    return m_pGraph != NULL;
}

void SymuCoreManager::ClearSubPopulations()
{
    // rmq. : les macrotypes sont détruits avec le Graph, donc on ne les détruit pas ici
    for (std::map<TypeVehicule *, SymuCore::SubPopulation*>::const_iterator iter = m_mapSubPopulations.begin(); iter != m_mapSubPopulations.end(); ++iter)
    {
        // les populations ont l'ownership des sous-populations, et on a une population par sous population :
        delete iter->second->GetPopulation();
    }
    m_mapSubPopulations.clear();

    if (m_pPublicTransportOnlySubPopulation)
    {
        delete m_pPublicTransportOnlySubPopulation->GetPopulation();
    }
}

void SymuCoreManager::Initialize()
{
    boost::unique_lock<boost::recursive_mutex> lock(m_Mutex);

    // Nettoyage de l'ancienne définition du réseau
    if (GraphExists())
    {
        delete m_pGraph;
    }

    m_pGraph = new SymuCore::MultiLayersGraph();

    if (m_pGraphBuilder)
    {
        delete m_pGraphBuilder;
    }
    m_pGraphBuilder = new SymuCoreGraphBuilder(m_pNetwork, false);
    if (!m_pGraphBuilder->Build(m_pGraph, true))
    {
        m_pNetwork->log() << Logger::Error << "Failure during SymuCore's graph creation !!!" << std::endl;
        m_pNetwork->log() << Logger::Info; // rebascule en mode INFO pour ne pas avoir à reprendre tous les appels aux log en précisant que c'est des INFO. à supprimer si on reprend tous les appels au log.
    }


    ClearSubPopulations();
    // Création d'une population pour chaque type de véhicule !
    for (size_t iTypeVeh = 0; iTypeVeh < m_pNetwork->m_LstTypesVehicule.size(); iTypeVeh++)
    {
        TypeVehicule * pTV = m_pNetwork->m_LstTypesVehicule.at(iTypeVeh);
        SymuCore::Population * currentPopulation = new SymuCore::Population(pTV->GetLabel());
        SymuCore::MacroType* pMacroType = new SymuCore::MacroType();

        SymuCore::VehicleType * currentVehicleType = new SymuCore::VehicleType(pTV->GetLabel());
        pMacroType->addVehicleType(currentVehicleType);

        currentPopulation->SetMacroType(pMacroType);
        currentPopulation->addServiceType(SymuCore::ST_Driving);
        m_pGraph->AddMacroType(pMacroType);
        
        SymuCore::SubPopulation * pSubPopulation = new SymuCore::SubPopulation(pTV->GetLabel());
        currentPopulation->addSubPopulation(pSubPopulation);

        m_mapSubPopulations[pTV] = pSubPopulation;
    }

    // Création de la sous population pour les calculs d'itinéraires en TEC uniquement
    std::vector<SymuCore::SubPopulation*> listPublicTransportSubPopulations;
    std::map<SymuCore::SubPopulation*, SymuCore::CostFunction> mapCostFunctions;
    if (m_pNetwork->IsWithPublicTransportGraph())
    {
        SymuCore::Population * pPublicTransportOnlyPop = new SymuCore::Population("PublicTransportOnly");
        pPublicTransportOnlyPop->addServiceType(SymuCore::ST_PublicTransport);
        m_pPublicTransportOnlySubPopulation = new SymuCore::SubPopulation("PublicTransportOnly");
        pPublicTransportOnlyPop->addSubPopulation(m_pPublicTransportOnlySubPopulation);
        listPublicTransportSubPopulations.push_back(m_pPublicTransportOnlySubPopulation);
        mapCostFunctions[m_pPublicTransportOnlySubPopulation] = SymuCore::CF_TravelTime;
    }
    

    m_MapTuyauxToPatterns.clear();
    SymuCore::Link * pLink;
    std::vector<SymuCore::Pattern*> listPatternsWithNoFuncClass;
    double dbMaxFuncClass = 0;
    for (size_t iLink = 0; iLink < m_pGraph->GetListLinks().size(); iLink++)
    {
        pLink = m_pGraph->GetListLinks()[iLink];
        for (size_t iPattern = 0; iPattern < pLink->getListPatterns().size(); iPattern++)
        {
            SymuCore::Pattern * pPattern = pLink->getListPatterns()[iPattern];
            // Construction de la map d'association entre les tuyaux routiers et les patterns correspondants
            SymuViaDrivingPattern * pSymuViaDrivingPattern = dynamic_cast<SymuViaDrivingPattern*>(pPattern);
            if (pSymuViaDrivingPattern)
            {
                pSymuViaDrivingPattern->prepareTimeFrame();
                TronconDrivingPattern * pTronconPattern = dynamic_cast<TronconDrivingPattern*>(pSymuViaDrivingPattern);
                if (pTronconPattern)
                {
                    m_MapTuyauxToPatterns[(Tuyau*)pTronconPattern->GetTuyau()] = pTronconPattern;

                    // Positionnement de la classe fonctionnelle
                    // rmq. : on laisse du coup à 0 le terme beta associé à la classe fonctionnelle pour les autres types de patterns... il faudra se poser
                    // la question quand on fera du multimodal et du A* de si on laisse comme ça : en l'état, on va pénaliser la route sur les TECs.
                    if (pTronconPattern->GetTuyau()->GetFunctionalClass() == 0)
                    {
                        listPatternsWithNoFuncClass.push_back(pTronconPattern);
                    }
                    else
                    {
                        pTronconPattern->setFuncClass(pTronconPattern->GetTuyau()->GetFunctionalClass());
                        dbMaxFuncClass = std::max<double>(dbMaxFuncClass, pTronconPattern->getFuncClass());
                    }
                }
            }
            else if (m_pNetwork->IsWithPublicTransportGraph())
            {
                SymuViaPublicTransportPattern * pPublicTransportPattern = dynamic_cast<SymuViaPublicTransportPattern*>(pPattern);
                if (pPublicTransportPattern)
                {
                    pPublicTransportPattern->addTimeFrame(0, m_pNetwork->GetDureeSimu());
                }
                else
                {
                    SymuCore::WalkingPattern * pWalkingPattern = dynamic_cast<SymuCore::WalkingPattern*>(pPattern);
                    if (pWalkingPattern)
                    {
                        SymuCore::Cost & walkingCost = pWalkingPattern->GetCostBySubPopulation()[m_pPublicTransportOnlySubPopulation];
                        double dbWalkTime = pWalkingPattern->getWalkLength() / m_pNetwork->GetWalkSpeed();
                        walkingCost.setUsedCostValue(dbWalkTime);
                        walkingCost.setTravelTime(dbWalkTime);
                    }
                }
            }
        }
    }

    // Positionnement de la classe fonctionnelle des tronçons routier pour lesquels elle n'est pas renseignée :
    for (size_t iPattern = 0; iPattern < listPatternsWithNoFuncClass.size(); iPattern++)
    {
        listPatternsWithNoFuncClass[iPattern]->setFuncClass(dbMaxFuncClass);
    }

    m_MapSymuViaTripNodeByNode = m_pGraphBuilder->GetDrivingGraphBuilder().GetMapNodesToExternalConnexions();
    for (std::map<ZoneDeTerminaison*, SymuCore::Node*, LessConnexionPtr<ZoneDeTerminaison> >::const_iterator iterZones = m_pGraphBuilder->GetDrivingGraphBuilder().GetMapZones().begin(); iterZones != m_pGraphBuilder->GetDrivingGraphBuilder().GetMapZones().end(); ++iterZones)
    {
        m_MapSymuViaTripNodeByNode[iterZones->second] = iterZones->first;
    }

    SymuCore::Node * pNode;
    for (size_t iNode = 0; iNode < m_pGraph->GetListNodes().size(); iNode++)
    {
        pNode = m_pGraph->GetListNodes()[iNode];
        std::map<SymuCore::Pattern*, std::map<SymuCore::Pattern*, SymuCore::AbstractPenalty*, SymuCore::ComparePattern>, SymuCore::ComparePattern>::iterator iter;
        for (iter = pNode->getMapPenalties().begin(); iter != pNode->getMapPenalties().end(); ++iter)
        {
            for (std::map<SymuCore::Pattern*, SymuCore::AbstractPenalty*, SymuCore::ComparePattern >::iterator iterDown = iter->second.begin(); iterDown != iter->second.end(); ++iterDown)
            {
                ConnectionDrivingPenalty * pPenalty = dynamic_cast<ConnectionDrivingPenalty*>(iterDown->second);
                // Pour filtrer les NullPenalty qui ne nous intéressent pas ici :
                if (pPenalty)
                {
                    pPenalty->prepareTimeFrame();
                }
                else if (m_pNetwork->IsWithPublicTransportGraph())
                {
                    ArretPublicTransportPenalty * pPublicTransportPenalty = dynamic_cast<ArretPublicTransportPenalty*>(iterDown->second);
                    if (pPublicTransportPenalty)
                    {
                        pPublicTransportPenalty->prepareTimeFrames(0, m_pNetwork->GetDureeSimu(), m_pNetwork->GetDureeSimu(), listPublicTransportSubPopulations, mapCostFunctions, 1, 0);
                    }
                }
            }
        }
    }

    // Calcul des couts et pénalisations
    UpdateCosts(m_pNetwork->m_LstTypesVehicule, true);

    m_bInvalid = false;
}

void SymuCoreManager::UpdateCosts(const std::deque<TypeVehicule*> & lstTypes, bool bIsRecursiveCall)
{
    boost::unique_lock<boost::recursive_mutex> lock(m_Mutex);

    if (!bIsRecursiveCall)
    {
        m_Lock.BeginWrite();
    }

    if (!GraphExists())
    {
        Initialize();
    }

    // Construction de la liste des sous-pops pour lesquels calculer les couts :
    std::map<TypeVehicule*, SymuCore::SubPopulation*> subpopulations;
    for (size_t iTypeVeh = 0; iTypeVeh < lstTypes.size(); iTypeVeh++)
    {
        TypeVehicule * pTV = lstTypes.at(iTypeVeh);
        subpopulations[pTV] = m_mapSubPopulations.at(pTV);
    }

    std::vector<SymuCore::SubPopulation*> listPublicTransportSubPopulations;
    listPublicTransportSubPopulations.push_back(m_pPublicTransportOnlySubPopulation);
    std::map<SymuCore::SubPopulation*, SymuCore::CostFunction> mapCostFunctions;
    mapCostFunctions[m_pPublicTransportOnlySubPopulation] = SymuCore::CF_TravelTime;

    SymuCore::Link * pLink;
    for (size_t iLink = 0; iLink < m_pGraph->GetListLinks().size(); iLink++)
    {
        pLink = m_pGraph->GetListLinks()[iLink];
        for (size_t iPattern = 0; iPattern < pLink->getListPatterns().size(); iPattern++)
        {
            SymuCore::Pattern * pPattern = pLink->getListPatterns()[iPattern];
            SymuViaDrivingPattern * pSymuViaDrivingPattern = dynamic_cast<SymuViaDrivingPattern*>(pPattern);
            if (pSymuViaDrivingPattern)
            {
                pSymuViaDrivingPattern->fillCost(subpopulations);
            }
            else if (m_pNetwork->IsWithPublicTransportGraph())
            {
                SymuViaPublicTransportPattern * pPublicTransportPattern = dynamic_cast<SymuViaPublicTransportPattern*>(pPattern);
                if (pPublicTransportPattern)
                {
                    pPublicTransportPattern->fillMeasuredCostsForTravelTimesUpdatePeriod(0, listPublicTransportSubPopulations, mapCostFunctions);
                }
                // rien à faire pour la marche à pieds : cout fixe déjà calculé à l'initialisation
            }
        }
    }

    SymuCore::Node * pNode;
    for (size_t iNode = 0; iNode < m_pGraph->GetListNodes().size(); iNode++)
    {
        pNode = m_pGraph->GetListNodes()[iNode];
        std::map<SymuCore::Pattern*, std::map<SymuCore::Pattern*, SymuCore::AbstractPenalty*, SymuCore::ComparePattern >, SymuCore::ComparePattern >::iterator iter;
        for (iter = pNode->getMapPenalties().begin(); iter != pNode->getMapPenalties().end(); ++iter)
        {
            for (std::map<SymuCore::Pattern*, SymuCore::AbstractPenalty*, SymuCore::ComparePattern >::iterator iterDown = iter->second.begin(); iterDown != iter->second.end(); ++iterDown)
            {
                ConnectionDrivingPenalty * pDrivingPenalty = dynamic_cast<ConnectionDrivingPenalty*>(iterDown->second);
                // Pour filtrer les NullPenalty qui ne nous intéressent pas ici :
                if (pDrivingPenalty)
                {
                    pDrivingPenalty->fillCost(subpopulations);
                }
                else if (m_pNetwork->IsWithPublicTransportGraph())
                {
                    ArretPublicTransportPenalty * pPublicTransportPenalty = dynamic_cast<ArretPublicTransportPenalty*>(iterDown->second);
                    if (pPublicTransportPenalty)
                    {
                        pPublicTransportPenalty->fillMeasuredCostsForTravelTimesUpdatePeriod(0, listPublicTransportSubPopulations, mapCostFunctions);
                    }
                }
            }
        }
    }

    m_pNetwork->InvalidatePathRelatedCaches();

    if (!bIsRecursiveCall)
    {
        m_Lock.EndWrite();
    }
}

void SymuCoreManager::Invalidate()
{
    m_bInvalid = true;
}

void SymuCoreManager::CheckGraph(bool bIsRecursiveCall)
{
    boost::unique_lock<boost::recursive_mutex> lock(m_Mutex);

    if(!GraphExists())
    {
        if (!bIsRecursiveCall)
        {
            m_Lock.BeginWrite();
        }

        // Premier appel : il faut initialiser le graphe
        Initialize();

        if (!bIsRecursiveCall)
        {
            m_Lock.EndWrite();
        }
    }
    else if(m_bInvalid)
    {
        if (!bIsRecursiveCall)
        {
            m_Lock.BeginWrite();
        }

        // Dans ce cas, on demande à calculer des itinéraires alors qu'une modification du réseau a eu lieu, impactant le graphe mais ne déclenchant pas automatiquement de recalcul de l'affectation (modification
        // des mouvements autorisés). On recalcul le nouveau graphe en conservant les couts actuels des différents liens avant de procéder au calcul d'itinéraire.

        // On note les couts définis dans le graphe précédent
        std::map<Tuyau*, std::map<SymuCore::SubPopulation*, SymuCore::Cost> > costs;
        std::map<Tuyau*, SymuCore::Pattern*>::const_iterator iter;
        for (iter = m_MapTuyauxToPatterns.begin(); iter != m_MapTuyauxToPatterns.end(); ++iter)
        {
            std::map<SymuCore::SubPopulation*, SymuCore::Cost> & costsForLink = costs[iter->first];
            for (std::map<TypeVehicule *, SymuCore::SubPopulation*>::const_iterator iterSubPop = m_mapSubPopulations.begin(); iterSubPop != m_mapSubPopulations.end(); ++iterSubPop)
            {
                costsForLink[iterSubPop->second] = *iter->second->getPatternCost(0, 0, iterSubPop->second);
            }
        }

        Initialize();

        // On restaure les couts notés dans le nouveau graphe. Les autres couts sont les couts calculés lors du Initialize.
        std::map<Tuyau*, std::map<SymuCore::SubPopulation*, SymuCore::Cost> >::const_iterator iterCost, iterCostEnd = costs.end();
        for (iter = m_MapTuyauxToPatterns.begin(); iter != m_MapTuyauxToPatterns.end(); ++iter)
        {
            iterCost = costs.find(iter->first);
            if(iterCost != iterCostEnd)
            {
                for (std::map<TypeVehicule *, SymuCore::SubPopulation*>::const_iterator iterSubPop = m_mapSubPopulations.begin(); iterSubPop != m_mapSubPopulations.end(); ++iterSubPop)
                {
                    *iter->second->getPatternCost(0, 0, iterSubPop->second) = iterCost->second.at(iterSubPop->second);
                }
            }
        }

        m_pNetwork->InvalidatePathRelatedCaches();

        if (!bIsRecursiveCall)
        {
            m_Lock.EndWrite();
        }
    }
}

void SymuCoreManager::ComputePaths(TripNode * pOrigine, TripNode * pDestination, TypeVehicule * pTypeVeh, double dbInstant, int nbShortestPaths,
    int method, const std::vector<Tuyau*> & refPath, const std::vector<Tuyau*> & linksToAvoid,
    std::vector<PathResult> & paths, std::map<std::vector<Tuyau*>, double> & MapFilteredItis, bool bRecursiveCall)
{
    CheckGraph(bRecursiveCall);

    if (!bRecursiveCall)
    {
        m_Lock.BeginRead();
    }

    SymuCore::Origin myOrigin;
    FillOriginFromTripNode(pOrigine, myOrigin);

    SymuCore::Destination myDest;
    FillDestinationFromTripNode(pDestination, myDest);

    std::vector<std::vector<PathResult> > pathsByOrigin;
    std::vector<std::map<std::vector<Tuyau*>, double> > filteredPathsByOrigin;
    std::vector<SymuCore::Origin> oneOriginVect(1, myOrigin);
    std::vector<SymuCore::Destination> oneDestinationVect(1, myDest);
    ComputePaths(oneOriginVect, oneDestinationVect, pTypeVeh, dbInstant, nbShortestPaths, method, refPath, linksToAvoid, pathsByOrigin, filteredPathsByOrigin);

    paths = pathsByOrigin.front();
    MapFilteredItis = filteredPathsByOrigin.front();

    if (!bRecursiveCall)
    {
        m_Lock.EndRead();
    }
}

/*void SymuCoreManager::ComputePaths(Connexion *pOrigine, TripNode * pDestination, TypeVehicule * pTypeVeh, double dbInstant, int nbShortestPaths,
	int method, const std::vector<Tuyau*> & refPath, const std::vector<Tuyau*> & linksToAvoid,
	std::vector<PathResult> & paths, std::map<std::vector<Tuyau*>, double> & MapFilteredItis, bool bRecursiveCall)
{
	SymuCore::Origin myOrigin;
	myOrigin.setSelfNode(m_pGraphBuilder->GetDrivingGraphBuilder().GetNodeFromConnexion(pOrigine));

	SymuCore::Destination myDest;
	FillDestinationFromTripNode(pDestination, myDest);

	std::vector<std::vector<PathResult> > pathsByOrigin;
	std::vector<std::map<std::vector<Tuyau*>, double> > filteredPathsByOrigin;
	std::vector<SymuCore::Origin> oneOriginVect(1, myOrigin);
	std::vector<SymuCore::Destination> oneDestinationVect(1, myDest);

	ComputePaths(myOrigin, pDestination, pTypeVeh, dbInstant, nbShortestPaths,
		method, refPath, linksToAvoid,
		paths, MapFilteredItis, bRecursiveCall),

}*/

void SymuCoreManager::ComputePaths(TripNode * pOrigine, const std::vector<TripNode*> & lstDestinations, TypeVehicule * pTypeVeh, double dbInstant, int nbShortestPaths,
    int method, const std::vector<Tuyau*> & refPath, const std::vector<Tuyau*> & linksToAvoid,
    std::vector<std::vector<PathResult> > & paths, std::vector<std::map<std::vector<Tuyau*>, double> > & MapFilteredItis, bool bRecursiveCall)
{
    CheckGraph(bRecursiveCall);

    if (!bRecursiveCall)
    {
        m_Lock.BeginRead();
    }

    SymuCore::Origin myOrigin;
    FillOriginFromTripNode(pOrigine, myOrigin);

    std::vector<SymuCore::Destination> myDestinations;
    for (size_t iDest = 0; iDest < lstDestinations.size(); iDest++)
    {
        SymuCore::Destination myDest;
        FillDestinationFromTripNode(lstDestinations.at(iDest), myDest);
        myDestinations.push_back(myDest);
    }

    std::vector<SymuCore::Origin> oneOriginVect(1, myOrigin);
    ComputePaths(oneOriginVect, myDestinations, pTypeVeh, dbInstant, nbShortestPaths, method, refPath, linksToAvoid, paths, MapFilteredItis);

    if (!bRecursiveCall)
    {
        m_Lock.EndRead();
    }
}

void SymuCoreManager::ComputePaths(const std::vector<TripNode*> lstOrigins, TripNode* pDestination, TypeVehicule * pTypeVeh, double dbInstant, int nbShortestPaths,
    int method, const std::vector<Tuyau*> & refPath, const std::vector<Tuyau*> & linksToAvoid,
    std::vector<std::vector<PathResult> > & paths,
    std::vector<std::map<std::vector<Tuyau*>, double> > & MapFilteredItis, bool bRecursiveCall)
{
    CheckGraph(bRecursiveCall);

    if (!bRecursiveCall)
    {
        m_Lock.BeginRead();
    }

    std::vector<SymuCore::Origin> lstSymuCoreOrigins;
    for (size_t iOrigin = 0; iOrigin < lstOrigins.size(); iOrigin++)
    {
        TripNode * pTripNodeOrigin = lstOrigins.at(iOrigin);

        SymuCore::Origin myOrigin;
        FillOriginFromTripNode(pTripNodeOrigin, myOrigin);
        lstSymuCoreOrigins.push_back(myOrigin);
    }

    SymuCore::Destination myDest;
    FillDestinationFromTripNode(pDestination, myDest);

    std::vector<SymuCore::Destination> oneDestinationVect(1, myDest);
    ComputePaths(lstSymuCoreOrigins, oneDestinationVect, pTypeVeh, dbInstant, nbShortestPaths, method, refPath, linksToAvoid, paths, MapFilteredItis);

    if (!bRecursiveCall)
    {
        m_Lock.EndRead();
    }
}

void SymuCoreManager::ComputePaths(TripNode * pOrigine, TripNode * pDestination, TypeVehicule * pTypeVeh, double dbInstant, int nbShortestPaths,
    std::vector<PathResult> & paths, std::map<std::vector<Tuyau*>, double> & MapFilteredItis, bool bRecursiveCall)
{
    std::vector<Tuyau*> placeHolder;
    ComputePaths(pOrigine, pDestination, pTypeVeh, dbInstant, nbShortestPaths, 0, placeHolder, placeHolder, paths, MapFilteredItis, bRecursiveCall);
}

void SymuCoreManager::ComputePaths(TripNode * pOrigine, const std::vector<TripNode*> lstDestinations, TypeVehicule * pTypeVeh, double dbInstant, int nbShortestPaths,
    std::vector<std::vector<PathResult> > & paths, std::vector<std::map<std::vector<Tuyau*>, double> > & MapFilteredItis, bool bRecursiveCall)
{
    std::vector<Tuyau*> placeHolder;
    ComputePaths(pOrigine, lstDestinations, pTypeVeh, dbInstant, nbShortestPaths, 0, placeHolder, placeHolder, paths, MapFilteredItis, bRecursiveCall);
}

void SymuCoreManager::ComputePaths(const std::vector<TripNode*> lstOrigins, TripNode* pDestination, TypeVehicule * pTypeVeh, double dbInstant, int nbShortestPaths,
    std::vector<std::vector<PathResult> > & paths,
    std::vector<std::map<std::vector<Tuyau*>, double> > & MapFilteredItis, bool bRecursiveCall)
{
    std::vector<Tuyau*> placeHolder;
    ComputePaths(lstOrigins, pDestination, pTypeVeh, dbInstant, nbShortestPaths, 0, placeHolder, placeHolder, paths, MapFilteredItis, bRecursiveCall);
}

void SymuCoreManager::ComputePaths(TripNode * pOrigine, Connexion * pDestination, TypeVehicule * pTypeVeh, double dbInstant, int nbShortestPaths,
    int method, const std::vector<Tuyau*> & refPath, const std::vector<Tuyau*> & linksToAvoid,
    std::vector<PathResult> & paths, std::map<std::vector<Tuyau*>, double> & MapFilteredItis, bool bRecursiveCall)
{
    CheckGraph(bRecursiveCall);

    if (!bRecursiveCall)
    {
        m_Lock.BeginRead();
    }

    SymuCore::Origin myOrigin;
    FillOriginFromTripNode(pOrigine, myOrigin);

    SymuCore::Destination myDest;
    myDest.setSelfNode(m_pGraphBuilder->GetDrivingGraphBuilder().GetNodeFromConnexion(pDestination));

    std::vector<std::vector<PathResult> > pathsByOrigin;
    std::vector<std::map<std::vector<Tuyau*>, double> > filteredPathsByOrigin;
    std::vector<SymuCore::Origin> oneOriginVect(1, myOrigin);
    std::vector<SymuCore::Destination> oneDestinationVect(1, myDest);
    ComputePaths(oneOriginVect, oneDestinationVect, pTypeVeh, dbInstant, nbShortestPaths, method, refPath, linksToAvoid, pathsByOrigin, filteredPathsByOrigin);

    paths = pathsByOrigin.front();
    MapFilteredItis = filteredPathsByOrigin.front();

    if (!bRecursiveCall)
    {
        m_Lock.EndRead();
    }
}

void SymuCoreManager::ComputePaths(const std::vector<TripNode*> lstOrigins, Connexion* pDestination, TypeVehicule * pTypeVeh, double dbInstant, int nbShortestPaths,
    int method, const std::vector<Tuyau*> & refPath, const std::vector<Tuyau*> & linksToAvoid,
    std::vector<std::vector<PathResult> > & paths,
    std::vector<std::map<std::vector<Tuyau*>, double> > & MapFilteredItis, bool bRecursiveCall)
{
    CheckGraph(bRecursiveCall);

    if (!bRecursiveCall)
    {
        m_Lock.BeginRead();
    }

    std::vector<SymuCore::Origin> lstSymuCoreOrigins;
    for (size_t iOrigin = 0; iOrigin < lstOrigins.size(); iOrigin++)
    {
        TripNode * pTripNodeOrigin = lstOrigins.at(iOrigin);

        SymuCore::Origin myOrigin;
        FillOriginFromTripNode(pTripNodeOrigin, myOrigin);
        lstSymuCoreOrigins.push_back(myOrigin);
    }

    SymuCore::Destination myDest;
    myDest.setSelfNode(m_pGraphBuilder->GetDrivingGraphBuilder().GetNodeFromConnexion(pDestination));

    std::vector<SymuCore::Destination> oneDestinationVect(1, myDest);
    ComputePaths(lstSymuCoreOrigins, oneDestinationVect, pTypeVeh, dbInstant, nbShortestPaths, method, refPath, linksToAvoid, paths, MapFilteredItis);

    if (!bRecursiveCall)
    {
        m_Lock.EndRead();
    }
}

void SymuCoreManager::ComputePaths(TripNode * pOrigine, Connexion * pDestination, TypeVehicule * pTypeVeh, double dbInstant, int nbShortestPaths,
    std::vector<PathResult> & paths, std::map<std::vector<Tuyau*>, double> & MapFilteredItis, bool bRecursiveCall)
{
    std::vector<Tuyau*> placeHolder;
    ComputePaths(pOrigine, pDestination, pTypeVeh, dbInstant, nbShortestPaths, 0, placeHolder, placeHolder, paths, MapFilteredItis, bRecursiveCall);
}

void SymuCoreManager::ComputePaths(const std::vector<TripNode*> lstOrigins, Connexion* pDestination, TypeVehicule * pTypeVeh, double dbInstant, int nbShortestPaths,
    std::vector<std::vector<PathResult> > & paths,
    std::vector<std::map<std::vector<Tuyau*>, double> > & MapFilteredItis, bool bRecursiveCall)
{
    std::vector<Tuyau*> placeHolder;
    ComputePaths(lstOrigins, pDestination, pTypeVeh, dbInstant, nbShortestPaths, 0, placeHolder, placeHolder, paths, MapFilteredItis, bRecursiveCall);
}

void SymuCoreManager::ComputePaths(Connexion* pOrigine, TripNode* pDestination, TypeVehicule * pTypeVeh, double dbInstant, int nbShortestPaths, Tuyau * pTuyauAmont,
    // Sorties
    std::vector<PathResult> & paths,
    std::map<std::vector<Tuyau*>, double> & MapFilteredItis, bool bRecursiveCall)
{
    std::vector<Tuyau*> placeHolder;
    ComputePaths(pOrigine, pDestination, pTypeVeh, dbInstant, nbShortestPaths, pTuyauAmont, 0, placeHolder, placeHolder, paths, MapFilteredItis, bRecursiveCall);
}

void SymuCoreManager::ComputePaths(Connexion* pOrigine, const std::vector<TripNode*> & lstDestinations, TypeVehicule * pTypeVeh, double dbInstant, int nbShortestPaths, Tuyau * pTuyauAmont,
    // Sorties
    std::vector<std::vector<PathResult> > & paths,
    std::vector<std::map<std::vector<Tuyau*>, double> > & MapFilteredItis, bool bRecursiveCall)
{
    std::vector<Tuyau*> placeHolder;
    ComputePaths(pOrigine, lstDestinations, pTypeVeh, dbInstant, nbShortestPaths, pTuyauAmont, 0, placeHolder, placeHolder, paths, MapFilteredItis, bRecursiveCall);
}

void SymuCoreManager::ComputePaths(Connexion* pOrigine, TripNode* pDestination, TypeVehicule * pTypeVeh, double dbInstant, int nbShortestPaths, Tuyau * pTuyauAmont,
    int method, const std::vector<Tuyau*> & refPath, const std::vector<Tuyau*> & linksToAvoid,
    // Sorties
    std::vector<PathResult> & paths,
    std::map<std::vector<Tuyau*>, double> & MapFilteredItis, bool bRecursiveCall)
{
    // convenience method pour ne pas avoir à changer les appels de partout dans SymuVia :
    if (pTuyauAmont == NULL)
    {
        CheckGraph(bRecursiveCall);

        if (!bRecursiveCall)
        {
            m_Lock.BeginRead();
        }

        SymuCore::Origin myOrigin;
        myOrigin.setSelfNode(m_pGraphBuilder->GetDrivingGraphBuilder().GetNodeFromConnexion(pOrigine));

        SymuCore::Destination myDest;
        FillDestinationFromTripNode(pDestination, myDest);

        std::vector<std::vector<PathResult> > pathsByOrigin;
        std::vector<std::map<std::vector<Tuyau*>, double> > filteredPathsByOrigin;
        std::vector<SymuCore::Origin> oneOriginVect(1, myOrigin);
        std::vector<SymuCore::Destination> oneDestinationVect(1, myDest);
        ComputePaths(oneOriginVect, oneDestinationVect, pTypeVeh, dbInstant, nbShortestPaths, method, refPath, linksToAvoid, pathsByOrigin, filteredPathsByOrigin);

        paths = pathsByOrigin.front();
        MapFilteredItis = filteredPathsByOrigin.front();

        if (!bRecursiveCall)
        {
            m_Lock.EndRead();
        }
    }
    else
    {
        TronconOrigine originLink(pTuyauAmont, NULL);
        ComputePaths(&originLink, pDestination, pTypeVeh, dbInstant, nbShortestPaths, method, refPath, linksToAvoid, paths, MapFilteredItis, bRecursiveCall);

        // Il faut retirer le tuyau amont de l'itinéraire résultat...
        for (size_t iPath = 0; iPath < paths.size(); iPath++)
        {
            PathResult & myPath = paths[iPath];
            if (!myPath.links.empty())
            {
                assert(myPath.links.front() == pTuyauAmont);

                myPath.links.erase(myPath.links.begin());
            }

            // rmq. : pour être rigoureux, il faudrait modifier aussi les itinéraires de MapFilteredItis et recalculer les couts
            // mais ils ne sont pas utilisés en cas d'appel à cette fonction avec un tuyau amont imposé, donc...
        }
    }
}

void SymuCoreManager::ComputePaths(Connexion* pOrigine, Connexion* pDestination, TypeVehicule * pTypeVeh, double dbInstant, int nbShortestPaths, Tuyau * pTuyauAmont,
	int method, const std::vector<Tuyau*> & refPath, const std::vector<Tuyau*> & linksToAvoid,
	// Sorties
	std::vector<PathResult> & paths,
	std::map<std::vector<Tuyau*>, double> & MapFilteredItis, bool bRecursiveCall)
{
	// convenience method pour ne pas avoir à changer les appels de partout dans SymuVia :
	if (pTuyauAmont == NULL)
	{
		CheckGraph(bRecursiveCall);

		if (!bRecursiveCall)
		{
			m_Lock.BeginRead();
		}

		SymuCore::Origin myOrigin;
		myOrigin.setSelfNode(m_pGraphBuilder->GetDrivingGraphBuilder().GetNodeFromConnexion(pOrigine));

		SymuCore::Destination myDest;
		myDest.setSelfNode(m_pGraphBuilder->GetDrivingGraphBuilder().GetNodeFromConnexion(pDestination));
		//FillDestinationFromTripNode(pDestination, myDest);

		std::vector<std::vector<PathResult> > pathsByOrigin;
		std::vector<std::map<std::vector<Tuyau*>, double> > filteredPathsByOrigin;
		std::vector<SymuCore::Origin> oneOriginVect(1, myOrigin);
		std::vector<SymuCore::Destination> oneDestinationVect(1, myDest);
		ComputePaths(oneOriginVect, oneDestinationVect, pTypeVeh, dbInstant, nbShortestPaths, method, refPath, linksToAvoid, pathsByOrigin, filteredPathsByOrigin);

		paths = pathsByOrigin.front();
		MapFilteredItis = filteredPathsByOrigin.front();

		if (!bRecursiveCall)
		{
			m_Lock.EndRead();
		}
	}
	else
	{
		TronconOrigine originLink(pTuyauAmont, NULL);

		SymuCore::Destination myDest;
		myDest.setSelfNode(m_pGraphBuilder->GetDrivingGraphBuilder().GetNodeFromConnexion(pDestination));

		ComputePaths(&originLink, pDestination, pTypeVeh, dbInstant, nbShortestPaths, method, refPath, linksToAvoid, paths, MapFilteredItis, bRecursiveCall);

		// Il faut retirer le tuyau amont de l'itinéraire résultat...
		for (size_t iPath = 0; iPath < paths.size(); iPath++)
		{
			PathResult & myPath = paths[iPath];
			if (!myPath.links.empty())
			{
				assert(myPath.links.front() == pTuyauAmont);

				myPath.links.erase(myPath.links.begin());
			}

			// rmq. : pour être rigoureux, il faudrait modifier aussi les itinéraires de MapFilteredItis et recalculer les couts
			// mais ils ne sont pas utilisés en cas d'appel à cette fonction avec un tuyau amont imposé, donc...
		}
	}
}

void SymuCoreManager::ComputePaths(Connexion* pOrigine, const std::vector<TripNode*> & lstDestinations, TypeVehicule * pTypeVeh, double dbInstant, int nbShortestPaths, Tuyau * pTuyauAmont,
    int method, const std::vector<Tuyau*> & refPath, const std::vector<Tuyau*> & linksToAvoid,
    // Sorties
    std::vector<std::vector<PathResult> > & paths,
    std::vector<std::map<std::vector<Tuyau*>, double> > & MapFilteredItis, bool bRecursiveCall)
{
    // convenience method pour ne pas avoir à changer les appels de partout dans SymuVia :
    if (pTuyauAmont == NULL)
    {
        CheckGraph(bRecursiveCall);

        if (!bRecursiveCall)
        {
            m_Lock.BeginRead();
        }

        SymuCore::Origin myOrigin;
        myOrigin.setSelfNode(m_pGraphBuilder->GetDrivingGraphBuilder().GetNodeFromConnexion(pOrigine));

        std::vector<SymuCore::Destination> myDestinations;
        for (size_t iDest = 0; iDest < lstDestinations.size(); iDest++)
        {
            SymuCore::Destination myDest;
            FillDestinationFromTripNode(lstDestinations.at(iDest), myDest);
            myDestinations.push_back(myDest);
        }

        std::vector<std::vector<PathResult> > pathsByDestination;
        std::vector<std::map<std::vector<Tuyau*>, double> > filteredPathsByDestination;
        std::vector<SymuCore::Origin> oneOriginVect(1, myOrigin);
        ComputePaths(oneOriginVect, myDestinations, pTypeVeh, dbInstant, nbShortestPaths, method, refPath, linksToAvoid, paths, MapFilteredItis);

        if (!bRecursiveCall)
        {
            m_Lock.EndRead();
        }
    }
    else
    {
        TronconOrigine originLink(pTuyauAmont, NULL);
        ComputePaths(&originLink, lstDestinations, pTypeVeh, dbInstant, nbShortestPaths, method, refPath, linksToAvoid, paths, MapFilteredItis, bRecursiveCall);

        // Il faut retirer le tuyau amont de l'itinéraire résultat...
        for (size_t iDest = 0; iDest < paths.size(); iDest++)
        {
            std::vector<PathResult> & pathsForDest = paths.at(iDest);
            for (size_t iPath = 0; iPath < pathsForDest.size(); iPath++)
            {
                PathResult & myPath = pathsForDest[iPath];
                if (!myPath.links.empty())
                {
                    assert(myPath.links.front() == pTuyauAmont);

                    myPath.links.erase(myPath.links.begin());
                }

                // rmq. : pour être rigoureux, il faudrait modifier aussi les itinéraires de MapFilteredItis et recalculer les couts
                // mais ils ne sont pas utilisés en cas d'appel à cette fonction avec un tuyau amont imposé, donc...
            }
        }
    }
}

void SymuCoreManager::ComputePaths(const std::vector<std::pair<Connexion*, Tuyau*> > & lstOrigines, TripNode* pDestination, TypeVehicule * pTypeVeh, double dbInstant, int nbShortestPaths,
    // Sorties
    std::vector<std::vector<PathResult> > & paths,
    std::vector<std::map<std::vector<Tuyau*>, double> > & MapFilteredItis, bool bRecursiveCall)
{
    CheckGraph(bRecursiveCall);

    if (!bRecursiveCall)
    {
        m_Lock.BeginRead();
    }

    std::vector<SymuCore::Origin> lstSymuCoreOrigins;
    for (size_t iOrigin = 0; iOrigin < lstOrigines.size(); iOrigin++)
    {
        const std::pair<Connexion*, Tuyau*> myOriginPair = lstOrigines.at(iOrigin);

        SymuCore::Origin myOrigin;
        if (myOriginPair.second == NULL)
        {
            myOrigin.setSelfNode(m_pGraphBuilder->GetDrivingGraphBuilder().GetNodeFromConnexion(myOriginPair.first));
        }
        else
        {
            TronconOrigine originLink(myOriginPair.second, NULL);
            FillOriginFromTripNode(&originLink, myOrigin);
        }
        lstSymuCoreOrigins.push_back(myOrigin);
    }

    SymuCore::Destination myDest;
    FillDestinationFromTripNode(pDestination, myDest);

    std::vector<Tuyau*> placeHolder;
    std::vector<SymuCore::Destination> oneDestinationVect(1, myDest);
    ComputePaths(lstSymuCoreOrigins, oneDestinationVect, pTypeVeh, dbInstant, nbShortestPaths, 0, placeHolder, placeHolder, paths, MapFilteredItis);

    for (size_t iOrigin = 0; iOrigin < lstOrigines.size(); iOrigin++)
    {
        const std::pair<Connexion*, Tuyau*> myOriginPair = lstOrigines.at(iOrigin);

        if (myOriginPair.second != NULL)
        {
            // Il faut retirer le tuyau amont de l'itinéraire résultat...
            std::vector<PathResult> & pathForOrigin = paths.at(iOrigin);
            for (size_t iPath = 0; iPath < pathForOrigin.size(); iPath++)
            {
                PathResult & myPath = pathForOrigin[iPath];
                if (!myPath.links.empty())
                {
                    assert(myPath.links.front() == myOriginPair.second);

                    myPath.links.erase(myPath.links.begin());
                }

                // rmq. : pour être rigoureux, il faudrait modifier aussi les itinéraires de MapFilteredItis et recalculer les couts
                // mais ils ne sont pas utilisés en cas d'appel à cette fonction avec un tuyau amont imposé, donc...
            }
        }
    }

    if (!bRecursiveCall)
    {
        m_Lock.EndRead();
    }

}

void SymuCoreManager::FillOriginFromTripNode(TripNode * pOrigine, SymuCore::Origin & myOrigin)
{
    if (pOrigine->GetOutputPosition().IsLink())
    {
        std::map<Tuyau*, SymuCore::Pattern*>::const_iterator iterLink = m_MapTuyauxToPatterns.find(pOrigine->GetOutputPosition().GetLink());
        if (iterLink == m_MapTuyauxToPatterns.end())
        {
            m_pNetwork->log() << Logger::Error << "Unknown origin in the assignment graph " << pOrigine->GetOutputPosition().GetLink()->GetLabel() << std::endl;
            m_pNetwork->log() << Logger::Info; // rebascule en mode INFO pour ne pas avoir à reprendre tous les appels aux log en précisant que c'est des INFO. à supprimer si on reprend tous les appels au log.
            return;
        }

        myOrigin.setPatternAsOrigin(iterLink->second);
    }
    else
    {
        myOrigin.setSelfNode(GetNodeFromOrigin(pOrigine));
    }
}

void SymuCoreManager::FillDestinationFromTripNode(TripNode * pDestination, SymuCore::Destination & myDest)
{
    if (pDestination->GetInputPosition().IsLink())
    {
        std::map<Tuyau*, SymuCore::Pattern*>::const_iterator iterLink = m_MapTuyauxToPatterns.find(pDestination->GetInputPosition().GetLink());
        if (iterLink == m_MapTuyauxToPatterns.end())
        {
            m_pNetwork->log() << Logger::Error << "Unknown destination in the assignment graph " << pDestination->GetInputPosition().GetLink()->GetLabel() << std::endl;
            m_pNetwork->log() << Logger::Info; // rebascule en mode INFO pour ne pas avoir à reprendre tous les appels aux log en précisant que c'est des INFO. à supprimer si on reprend tous les appels au log.
            return;
        }

        myDest.setPatternAsDestination(iterLink->second);
    }
    else
    {
        myDest.setSelfNode(GetNodeFromDestination(pDestination));
    }
}

void SymuCoreManager::ComputePaths(const std::vector<SymuCore::Origin> & lstOrigins, const std::vector<SymuCore::Destination> & lstDestinations, TypeVehicule * pTypeVeh, double dbInstant, int nbShortestPaths,
    int method, const std::vector<Tuyau*> & refPath, const std::vector<Tuyau*> & linksToAvoid,
    std::vector<std::vector<PathResult> > & paths, std::vector<std::map<std::vector<Tuyau*>, double> > & MapFilteredItis)
{
    SymuCore::SubPopulation * pSubPopulation = m_mapSubPopulations.at(pTypeVeh);

    // Pour gestion du calcul rapide si plusieurs origines différentes :
    bool bMultipleOrigins = lstOrigins.size() > 1;
    bool bMultipleDestinations = lstDestinations.size() > 1;

    assert(!(bMultipleOrigins && bMultipleDestinations)); // on ne sait pas faire, ou alors il faut décomposer et pas besoin pour l'instant car on ne fait pas ce type d'appel

    bool bMultiTarget = bMultipleOrigins || bMultipleDestinations;

    // mode multiorigine/destination utilisé uniquement pour du test de chemin possible, donc avec nbShortestPaths = 1
    assert(!bMultiTarget || nbShortestPaths == 1);

    std::map<SymuCore::Origin*, std::map<SymuCore::Destination*, std::vector<SymuCore::ValuetedPath*> > > pathResults;

    std::vector<SymuCore::Origin*> lstOriginsForSymuCore;
    for (size_t iOrigin = 0; iOrigin < lstOrigins.size(); iOrigin++)
    {
        SymuCore::Origin * myOrigin = const_cast<SymuCore::Origin*>(&lstOrigins.at(iOrigin));
        lstOriginsForSymuCore.push_back(myOrigin);
    }

    std::vector<SymuCore::Destination*> lstDestinationsForSymuCore;
    for (size_t iDest = 0; iDest < lstDestinations.size(); iDest++)
    {
        SymuCore::Destination * myDestination = const_cast<SymuCore::Destination*>(&lstDestinations.at(iDest));
        lstDestinationsForSymuCore.push_back(myDestination);
    }

    if (bMultiTarget)
    {
        SymuCore::Dijkstra computer(bMultipleDestinations);

        // On ne peut pas faire de A* si on a plusieurs origines / destinations...
        // rmq. : l'autre option serait de faire du A* mais en décomposant en 1 origine <-> 1 destination...
        /*if (m_pNetwork->GetShortestPathHeuristic() == Reseau::HEURISTIC_EUCLIDIAN && )
        {
            computer.SetHeuristic(SymuCore::SPH_EUCLIDIAN, m_pNetwork->GetAStarBeta(), m_pNetwork->GetHeuristicGamma());
        }*/
        pathResults = computer.getShortestPaths(0, lstOriginsForSymuCore, lstDestinationsForSymuCore, pSubPopulation, dbInstant);
    }
    else
    {
        // récupération des paramètres de commonality filter et chemins prédéfinis pour le couple OD et pour l'instant considéré :
        std::map<SymuCore::Origin*, std::map<SymuCore::Destination*, std::vector<double> > > commonalityFactorParams;
        std::map<SymuCore::Origin*, std::map<SymuCore::Destination*, std::vector<SymuCore::PredefinedPath> > > predefinedPaths;

        SymuCore::KShortestPaths::FromSymuViaParameters KParams;

        std::vector<double> defaultCommonalityFactorParams;
        defaultCommonalityFactorParams.push_back(m_pNetwork->GetCommonalityAlpha());
        defaultCommonalityFactorParams.push_back(m_pNetwork->GetCommonalityBeta());
        defaultCommonalityFactorParams.push_back(m_pNetwork->GetCommonalityGamma());

        assert(lstOriginsForSymuCore.size() == 1 && lstDestinationsForSymuCore.size() == 1);

        SymuCore::Origin * myOrigin = lstOriginsForSymuCore.front();
        SymuCore::Destination * myDest = lstDestinationsForSymuCore.front();

        std::vector<SymuCore::PredefinedPath> & predefPathsForOD = predefinedPaths[myOrigin][myDest];
        std::map<SymuCore::Destination*, std::vector<double> > & commonalityParamsForOrigin = commonalityFactorParams[myOrigin];
        bool bCustomCommonalityParamsFound = false;
        // remarque : pour faire comme avant, on ne cherche pas de paramètres prédéfinis si on a des liens origine ou destination.
        // Autre option : on pourrait ne pas faire ce test, mais après on a des assert qui pètent car on s'attend pas à avoir plus d'un itinéraire quand on n'en demande qu'un...
        // Autre option : modifier le module de calcul d'itinéraire pour ne renvoyer qu'un itinéraire même si on a en a plusieurs prédéfinis, ca semblerait logique aussi ...
        if (!myOrigin->getPattern() && !myDest->getPattern())
        {
            std::map<SymuCore::Node*, SymuViaTripNode*>::const_iterator iterOrigin = m_MapSymuViaTripNodeByNode.find(myOrigin->getNode());
            if (iterOrigin != m_MapSymuViaTripNodeByNode.end())
            {
                std::map<SymuCore::Node*, SymuViaTripNode*>::const_iterator iterDestination = m_MapSymuViaTripNodeByNode.find(myDest->getNode());
                if (iterDestination != m_MapSymuViaTripNodeByNode.end())
                {
                    std::deque<TimeVariation<SimMatOrigDest> > & lstCoeffDest = m_pNetwork->IsInitSimuTrafic() ? iterOrigin->second->GetLstCoeffDest(pTypeVeh) : iterOrigin->second->GetLstCoeffDestInit(pTypeVeh);
                    SimMatOrigDest *pMatDest = GetVariation(dbInstant, &lstCoeffDest, m_pNetwork->GetLag());
                    if (pMatDest)
                    {
                        // Pour chaque destination ...
                        for (size_t iDest = 0; iDest < pMatDest->MatOrigDest.size(); iDest++)
                        {
                            VectDest * pVectDest = pMatDest->MatOrigDest[iDest];
                            if (pVectDest->pDest == iterDestination->second)
                            {
                                if (pVectDest->bHasCustomCommonalityParameters)
                                {
                                    std::vector<double> & commonalityFactorsForOD = commonalityParamsForOrigin[myDest];
                                    commonalityFactorsForOD.push_back(pVectDest->dbCommonalityAlpha);
                                    commonalityFactorsForOD.push_back(pVectDest->dbCommonalityBeta);
                                    commonalityFactorsForOD.push_back(pVectDest->dbCommonalityGamma);
                                    bCustomCommonalityParamsFound = true;
                                }

                                // gestion des itinéraires prédéfinis :
                                for (size_t iPath = 0; iPath < pVectDest->lstItineraires.size(); iPath++)
                                {
                                    const std::pair<double, std::pair<std::pair<std::vector<Tuyau*>, Connexion*>, std::string> > & predefinedPath = pVectDest->lstItineraires[iPath];
                                    std::vector<SymuCore::Pattern*> patterns;
                                    for (size_t iLink = 0; iLink < predefinedPath.second.first.first.size(); iLink++)
                                    {
                                        patterns.push_back(m_MapTuyauxToPatterns.at(predefinedPath.second.first.first[iLink]));
                                    }
                                    SymuCore::Node * pJunction = NULL;
                                    if (predefinedPath.second.first.second != NULL)
                                    {
                                        pJunction = m_pGraphBuilder->GetDrivingGraphBuilder().GetNodeFromConnexion(predefinedPath.second.first.second);
                                    }
                                    predefPathsForOD.push_back(SymuCore::PredefinedPath(patterns, pJunction, predefinedPath.second.second, predefinedPath.first));
                                }

                                break;
                            }
                        }
                    }
                }
            }
        }

        if (!bCustomCommonalityParamsFound)
        {
            commonalityParamsForOrigin[myDest] = defaultCommonalityFactorParams;
        }

        KParams[pSubPopulation][myOrigin][myDest].addTimeFrame(0, m_pNetwork->GetDureeSimu(), boost::make_shared<double>(nbShortestPaths));


        SymuCore::KShortestPaths computer(m_pNetwork->GetDijkstraAlpha(), m_pNetwork->GetDijkstraBeta(), m_pNetwork->GetDijkstraGamma(), m_pNetwork->GetCommonalityFilter(), defaultCommonalityFactorParams, -1, KParams, &predefinedPaths, &commonalityFactorParams);

        for (size_t iLinkToAvoid = 0; iLinkToAvoid < linksToAvoid.size(); iLinkToAvoid++)
        {
            SymuCore::Pattern * pPatternToAvoid = m_MapTuyauxToPatterns.at(linksToAvoid.at(iLinkToAvoid));
            computer.GetPatternsToAvoid().push_back(pPatternToAvoid);
        }

        if (method != 0)
        {

            std::vector<SymuCore::Pattern*> refPatterns;
            for (size_t iLink = 0; iLink < refPath.size(); iLink++)
            {
                refPatterns.push_back(m_MapTuyauxToPatterns.at(refPath[iLink]));
            }
            SymuCore::Path refSymuCorePath(refPatterns);
            computer.SetSimulationGameMethod(method, refSymuCorePath);
        }

        // gestion de l'heuristique
        if (m_pNetwork->GetShortestPathHeuristic() == Reseau::HEURISTIC_EUCLIDIAN)
        {
            computer.SetHeuristicParams(SymuCore::SPH_EUCLIDIAN, m_pNetwork->GetAStarBeta(), m_pNetwork->GetHeuristicGamma());
        }

        pathResults = computer.getShortestPaths(0, lstOriginsForSymuCore, lstDestinationsForSymuCore, pSubPopulation, dbInstant);

        // Gestion des itinéraires filtrés :
        std::map<std::vector<Tuyau*>, double> filteredPathsForOD;
        const std::vector< std::pair<SymuCore::Path, double> > & filteredPaths = computer.getFilteredPaths().at(myOrigin).at(myDest);
        for (size_t iPath = 0; iPath < filteredPaths.size(); iPath++)
        {
            const std::pair<SymuCore::Path, double> & filteredPathPair = filteredPaths.at(iPath);
            std::vector<Tuyau*> filteredPath;

            // liste des tronçons du chemin
            for (size_t iLink = 0; iLink < filteredPathPair.first.GetlistPattern().size(); iLink++)
            {
                TronconDrivingPattern * pLinkPattern = dynamic_cast<TronconDrivingPattern*>(filteredPathPair.first.GetlistPattern().at(iLink));
                if (pLinkPattern)
                {
                    filteredPath.push_back((Tuyau*)pLinkPattern->GetTuyau());
                }
            }

            filteredPathsForOD[filteredPath] = filteredPathPair.second;
        }
        MapFilteredItis.push_back(filteredPathsForOD);
    }

    for (size_t iOrigin = 0; iOrigin < lstOriginsForSymuCore.size(); iOrigin++)
    {
        SymuCore::Origin * myOrigin = lstOriginsForSymuCore.at(iOrigin);

        for (size_t iDestination = 0; iDestination < lstDestinationsForSymuCore.size(); iDestination++)
        {
            SymuCore::Destination * myDest = lstDestinationsForSymuCore.at(iDestination);

            std::vector<PathResult> pathsForOD;

            const std::vector<SymuCore::ValuetedPath*> & symucorePaths = pathResults[myOrigin][myDest];
            for (size_t i = 0; i < symucorePaths.size(); i++)
            {
                const SymuCore::ValuetedPath* pPath = symucorePaths.at(i);
                PathResult path;

                // cout du chemin
                path.dbCost = pPath->GetCost();

                // liste des tronçons du chemin
                for (size_t iLink = 0; iLink < pPath->GetPath().GetlistPattern().size(); iLink++)
                {
                    TronconDrivingPattern * pLinkPattern = dynamic_cast<TronconDrivingPattern*>(pPath->GetPath().GetlistPattern().at(iLink));
                    if (pLinkPattern)
                    {
                        path.links.push_back((Tuyau*)pLinkPattern->GetTuyau());
                    }
                }

                // point de jonction pour les itinéraires vides (zones collées)
                if (path.links.empty())
                {
                    // On suppose que le point de jonction est le premier noeud suivant le noeud d'origine
                    SymuCore::Node * pJunctionNode = NULL;
                    if (pPath->IsPredefined())
                    {
                        pJunctionNode = pPath->getPredefinedJunctionNode();
                    }
                    else if (!pPath->GetPath().GetlistPattern().empty())
                    {
                        // la suite ne fonctionne que si on est dans le cas Zone -> Zone (avec deux patterns dans l'itinéraire du coup).
                        // Si un seul pattern, on est dans le cas Zone -> extremité ponctuelle ou extremité ponctuelle -> Zone, cas pour lequel
                        // le point de jonction est inutile (c'est forcément l'extremité ponctuelle).
                        if (pPath->GetPath().GetlistPattern().size() == 2)
                        {
                            pJunctionNode = pPath->GetPath().GetlistPattern().front()->getLink()->getDownstreamNode();
                        }
                        else
                        {
                            assert(pPath->GetPath().GetlistPattern().size() == 1); // impossible en principe d'avoir aucun pattern ou plus de deux ici.
                        }
                    }
                    if (pJunctionNode)
                    {
						if (pJunctionNode->getNodeType() != SymuCore::NT_Area)
							path.pJunction = m_pGraphBuilder->GetDrivingGraphBuilder().GetConnexionFromNode(pJunctionNode, true);
						else
							path.pJunction = NULL;
                    }
                    else
                    {
                        path.pJunction = NULL;
                    }
                }
                else
                {
                    path.pJunction = NULL;
                }

                path.dbPenalizedCost = pPath->getPenalizedCost();
                path.dbCommonalityFactor = pPath->getCommonalityFactor();
                path.bPredefined = pPath->IsPredefined();
                path.strName = pPath->getStrName();

                pathsForOD.push_back(path);

                delete pPath;
            }
            paths.push_back(pathsForOD);
        }
    }
}

double SymuCoreManager::ComputeCost(TypeVehicule* pTypeVehicule, const std::vector<Tuyau*> & path, bool bIsRecursiveCall)
{
    boost::unique_lock<boost::recursive_mutex> lock(m_Mutex);

    CheckGraph(bIsRecursiveCall);

    if (!bIsRecursiveCall)
    {
        m_Lock.BeginRead();
    }

    std::vector<SymuCore::Pattern*> patterns;
    for(size_t i = 0; i < path.size(); i++)
    {
        patterns.push_back(m_MapTuyauxToPatterns.at(path.at(i)));
    }
    SymuCore::Path symucorePath(patterns);
    const SymuCore::Cost & myCost = symucorePath.GetPathCost(0, 0, m_mapSubPopulations.at(pTypeVehicule), false);

    double dbResult = myCost.getCostValue();

    if (!bIsRecursiveCall)
    {
        m_Lock.EndRead();
    }

    return dbResult;
}

SymuCore::Node* SymuCoreManager::GetNodeFromOrigin(TripNode * pOrigin)
{
    SymuCore::Node* result;
    ZoneDeTerminaison * pZone = dynamic_cast<ZoneDeTerminaison*>(pOrigin);
    if(pZone)
    {
        result = m_pGraphBuilder->GetDrivingGraphBuilder().GetMapZones().at(pZone);
    }
    else
    {
        result = m_pGraphBuilder->GetDrivingGraphBuilder().GetNodeFromConnexion(pOrigin->GetOutputPosition().GetConnection());
    }
    return result;
}

SymuCore::Node* SymuCoreManager::GetNodeFromDestination(TripNode * pDest)
{
    SymuCore::Node* result;
    ZoneDeTerminaison * pZone = dynamic_cast<ZoneDeTerminaison*>(pDest);
    if(pZone)
    {
        result = m_pGraphBuilder->GetDrivingGraphBuilder().GetMapZones().at(pZone);
    }
    else
    {
        result = m_pGraphBuilder->GetDrivingGraphBuilder().GetNodeFromConnexion(pDest->GetInputPosition().GetConnection());
    }
    return result;
}

void SymuCoreManager::ForceGraphRefresh()
{
    m_Lock.BeginWrite();
    Initialize();
    m_pNetwork->GetAffectationModule()->Run(m_pNetwork, m_pNetwork->m_dbInstSimu, 'P', true);
    m_Lock.EndWrite();
}


//////////////////////////////////////////////////////////////////////////////////////
// Méthodes spécifiques aux calculs multimodaux (pour le simulation game par ex)
//////////////////////////////////////////////////////////////////////////////////////

void SymuCoreManager::ComputePathsPublicTransport(Connexion* pOrigin, TripNode* pDestination, double dbInstant, int nbShortestPaths,
    // Sorties
    std::vector<std::pair<double, std::vector<std::string> > > & paths)
{
    std::vector<TripNode*> lstDestinations(1, pDestination);
    std::vector<std::vector<std::pair<double, std::vector<std::string> > > > pathsByDest;
    ComputePathsPublicTransport(pOrigin, lstDestinations, dbInstant, nbShortestPaths, pathsByDest);

    if (!pathsByDest.empty())
    {
        paths = pathsByDest.front();
    }
}

void SymuCoreManager::ComputePathsPublicTransport(Connexion* pOrigin, const std::vector<TripNode*> & lstDestinations, double dbInstant, int nbShortestPaths,
    std::vector<std::vector<std::pair<double, std::vector<std::string> > > > & paths)
{
    CheckGraph(false);

    m_Lock.BeginRead();

    SymuCore::Origin myOrigin;
    myOrigin.setSelfNode(m_pGraphBuilder->GetPublicTransportGraphBuilder().GetOriginNodeFromConnexion(pOrigin));

    // the connexion pOrigin can be linked to no Node in the public transport graph ! so no paths.
    if (myOrigin.getNode())
    {
        std::vector<SymuCore::Destination> lstNonNullDestinations;
        std::vector<bool> isNullDestination;
        for (size_t iDest = 0; iDest < lstDestinations.size(); iDest++)
        {
            SymuCore::Destination myDest;
            myDest.setSelfNode(m_pGraphBuilder->GetPublicTransportGraphBuilder().GetDestinationNodeFromTripNode(lstDestinations.at(iDest)));
            if (myDest.getNode())
            {
                lstNonNullDestinations.push_back(myDest);
                isNullDestination.push_back(false);
            }
            else
            {
                isNullDestination.push_back(true);
            }
        }        

        std::vector<std::vector<std::pair<double, std::vector<std::string> > > > nonNullPaths;
        ComputePathsPublicTransport(myOrigin, lstNonNullDestinations, std::vector<SymuCore::Pattern*>(), dbInstant, nbShortestPaths, nonNullPaths);

        // On réinjecte les destinations inatteignables (noeud null) dans path :
        paths.resize(isNullDestination.size());
        size_t nonNullindex = 0;
        for (size_t iDest = 0; iDest < isNullDestination.size(); iDest++)
        {
            if (!isNullDestination.at(iDest))
            {
                paths[iDest] = nonNullPaths[nonNullindex++];
            }
        }
        assert(nonNullindex == nonNullPaths.size());
    }

    m_Lock.EndRead();
}

void SymuCoreManager::ComputePathsPublicTransport(Arret* pOrigin, PublicTransportLine* pPreviousLine, PublicTransportLine* pNextLine, TripNode* pDestination, double dbInstant, int nbShortestPaths,
    std::vector<std::pair<double, std::vector<std::string> > > & paths)
{
    CheckGraph(false);

    m_Lock.BeginRead();

    SymuCore::Origin myOrigin;
    myOrigin.setSelfNode(m_pGraphBuilder->GetPublicTransportGraphBuilder().GetNodeFromStop((Arret*)pOrigin));

    std::vector<SymuCore::Destination> lstDest(1);
    lstDest[0].setSelfNode(m_pGraphBuilder->GetPublicTransportGraphBuilder().GetDestinationNodeFromTripNode(pDestination));

    // Si la destination n'est pas atteignable par le graph de transport public on peut avoir du noeud nul ici : pas de chemin dans ce cas !
    if (lstDest[0].getNode())
    {
        // Pénalisation des patterns des lignes amont et aval :
        std::vector<SymuCore::Pattern*> lstPatternsToAvoid;
        if (pPreviousLine)
        {
            for (size_t iUpLink = 0; iUpLink < myOrigin.getNode()->getUpstreamLinks().size(); iUpLink++)
            {
                SymuCore::Link * pUpLink = myOrigin.getNode()->getUpstreamLinks().at(iUpLink);
                for (size_t iUpPattern = 0; iUpPattern < pUpLink->getListPatterns().size(); iUpPattern++)
                {
                    SymuCore::Pattern * pUpPattern = pUpLink->getListPatterns().at(iUpPattern);
                    SymuViaPublicTransportPattern * pSymuViaPattern = dynamic_cast<SymuViaPublicTransportPattern*>(pUpPattern);
                    if (pSymuViaPattern && pSymuViaPattern->GetSymuViaLine() == pPreviousLine)
                    {
                        lstPatternsToAvoid.push_back(pSymuViaPattern);
                    }
                }
            }
        }

        if (pNextLine)
        {
            bool bNextLinePatternFound = false;
            for (size_t iDownLink = 0; iDownLink < myOrigin.getNode()->getDownstreamLinks().size(); iDownLink++)
            {
                SymuCore::Link * pDownLink = myOrigin.getNode()->getDownstreamLinks().at(iDownLink);
                for (size_t iDownPattern = 0; iDownPattern < pDownLink->getListPatterns().size(); iDownPattern++)
                {
                    SymuCore::Pattern * pDownPattern = pDownLink->getListPatterns().at(iDownPattern);
                    SymuViaPublicTransportPattern * pSymuViaPattern = dynamic_cast<SymuViaPublicTransportPattern*>(pDownPattern);
                    if (pSymuViaPattern && pSymuViaPattern->GetSymuViaLine() == pNextLine)
                    {
                        lstPatternsToAvoid.push_back(pSymuViaPattern);
                        bNextLinePatternFound = true;
                    }
                }
            }
            // si on trouve pas la ligne, il faut pénaliser les patterns de la ligne à pénaliser après marche à pieds vers cette ligne :
            if (!bNextLinePatternFound)
            {
                for (size_t iDownLink = 0; iDownLink < myOrigin.getNode()->getDownstreamLinks().size(); iDownLink++)
                {
                    SymuCore::Link * pDownLink = myOrigin.getNode()->getDownstreamLinks().at(iDownLink);
                    for (size_t iDownPattern = 0; iDownPattern < pDownLink->getListPatterns().size(); iDownPattern++)
                    {
                        SymuCore::Pattern * pDownPattern = pDownLink->getListPatterns().at(iDownPattern);
                        if (pDownPattern->getPatternType() == SymuCore::PT_Walk)
                        {
                            const std::vector<SymuCore::Link*> & downDownLinks = pDownPattern->getLink()->getDownstreamNode()->getDownstreamLinks();
                            for (size_t iDownDownPattern = 0; iDownDownPattern < downDownLinks.size(); iDownDownPattern++)
                            {
                                SymuCore::Link * pDownDownLink = downDownLinks.at(iDownPattern);
                                for (size_t iDownDownPattern = 0; iDownDownPattern < pDownDownLink->getListPatterns().size(); iDownDownPattern++)
                                {
                                    SymuCore::Pattern * pDownDownPattern = pDownDownLink->getListPatterns().at(iDownDownPattern);
                                    SymuViaPublicTransportPattern * pSymuViaDownPattern = dynamic_cast<SymuViaPublicTransportPattern*>(pDownDownPattern);
                                    if (pSymuViaDownPattern && pSymuViaDownPattern->GetSymuViaLine() == pNextLine)
                                    {
                                        lstPatternsToAvoid.push_back(pSymuViaDownPattern);
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }


        std::vector<std::vector<std::pair<double, std::vector<std::string> > > > pathsByDest;
        ComputePathsPublicTransport(myOrigin, lstDest, lstPatternsToAvoid, dbInstant, nbShortestPaths, pathsByDest);
        if (!pathsByDest.empty())
        {
            paths = pathsByDest.front();
        }
    }

    m_Lock.EndRead();
}

void SymuCoreManager::ComputePathsPublicTransport(SymuCore::Origin myOrigin, const std::vector<SymuCore::Destination> & lstDestinations, const std::vector<SymuCore::Pattern*> & patternsToAvoid, double dbInstant, int nbShortestPaths,
    // Sorties
    std::vector<std::vector<std::pair<double, std::vector<std::string> > > > & paths)
{
    SymuCore::SubPopulation * pSubPopulation = m_pPublicTransportOnlySubPopulation;

    std::vector<SymuCore::Origin*> lstOriginsForSymuCore(1, &myOrigin);

    std::vector<SymuCore::Destination*> lstDestinationsForSymuCore;
    for (size_t iDest = 0; iDest < lstDestinations.size(); iDest++)
    {
        SymuCore::Destination * myDestination = const_cast<SymuCore::Destination*>(&lstDestinations.at(iDest));
        lstDestinationsForSymuCore.push_back(myDestination);
    }

    bool bMultipleDestinations = lstDestinations.size() > 1;

    // mode multidestination utilisé uniquement pour du test de chemin possible, donc avec nbShortestPaths = 1
    assert(!bMultipleDestinations || nbShortestPaths == 1);

    std::map<SymuCore::Origin*, std::map<SymuCore::Destination*, std::vector<SymuCore::ValuetedPath*> > > pathResults;
    if (bMultipleDestinations)
    {
        SymuCore::Dijkstra computer(bMultipleDestinations);

        // On ne peut pas faire de A* si on a plusieurs origines / destinations...
        // rmq. : l'autre option serait de faire du A* mais en décomposant en 1 origine <-> 1 destination...
        /*if (m_pNetwork->GetShortestPathHeuristic() == Reseau::HEURISTIC_EUCLIDIAN && )
        {
        computer.SetHeuristic(SymuCore::SPH_EUCLIDIAN, m_pNetwork->GetAStarBeta(), m_pNetwork->GetHeuristicGamma());
        }*/
        pathResults = computer.getShortestPaths(0, lstOriginsForSymuCore, lstDestinationsForSymuCore, pSubPopulation, dbInstant);
    }
    else
    {
        SymuCore::KShortestPaths::FromSymuViaParameters KParams;
        std::map<SymuCore::Destination*, SymuCore::ListTimeFrame<double> > & KParamsForOrigin = KParams[pSubPopulation][&myOrigin];
        for (size_t iDest = 0; iDest < lstDestinationsForSymuCore.size(); iDest++)
        {
            KParamsForOrigin[lstDestinationsForSymuCore.at(iDest)].addTimeFrame(0, m_pNetwork->GetDureeSimu(), boost::make_shared<double>(nbShortestPaths));
        }

        std::vector<double> defaultCommonalityFactorParams;
        defaultCommonalityFactorParams.push_back(m_pNetwork->GetCommonalityAlpha());
        defaultCommonalityFactorParams.push_back(m_pNetwork->GetCommonalityBeta());
        defaultCommonalityFactorParams.push_back(m_pNetwork->GetCommonalityGamma());
        SymuCore::KShortestPaths computer(m_pNetwork->GetDijkstraAlpha(), m_pNetwork->GetDijkstraBeta(), m_pNetwork->GetDijkstraGamma(), false, defaultCommonalityFactorParams, -1, KParams);

        computer.GetPatternsToAvoid() = patternsToAvoid;

        // gestion de l'heuristique
        if (m_pNetwork->GetShortestPathHeuristic() == Reseau::HEURISTIC_EUCLIDIAN)
        {
            computer.SetHeuristicParams(SymuCore::SPH_EUCLIDIAN, m_pNetwork->GetAStarBeta(), m_pNetwork->GetHeuristicGamma());
        }

        pathResults = computer.getShortestPaths(0, lstOriginsForSymuCore, lstDestinationsForSymuCore, pSubPopulation, dbInstant);
    }

    const std::map<SymuCore::Destination*, std::vector<SymuCore::ValuetedPath*> > & pathResultsForOrigin = pathResults[&myOrigin];

    for (size_t iDest = 0; iDest < lstDestinationsForSymuCore.size(); iDest++)
    {
        std::vector<std::pair<double, std::vector<std::string> > > pathForDestination;
        const std::vector<SymuCore::ValuetedPath*> & symucorePaths = pathResultsForOrigin.at(lstDestinationsForSymuCore.at(iDest));
        for (size_t i = 0; i < symucorePaths.size(); i++)
        {
            const SymuCore::ValuetedPath* pPath = symucorePaths.at(i);
            std::pair<double, std::vector<std::string> > path;

            // cout du chemin
            path.first = pPath->GetCost();

            // liste des étapes du chemin sous la forme : nom arret nom ligne nom arrret nom ligne nom arret.
            Arret * pLastStop = NULL;
            SymuCore::PublicTransportLine * pLastLine = NULL;
            for (size_t iLink = 0; iLink < pPath->GetPath().GetlistPattern().size(); iLink++)
            {
                SymuCore::Pattern * pPattern = pPath->GetPath().GetlistPattern().at(iLink);
                SymuViaPublicTransportPattern * pPublicTransportPattern = dynamic_cast<SymuViaPublicTransportPattern*>(pPattern);
                if (pPublicTransportPattern)
                {
                    if (pPublicTransportPattern->getLine() != pLastLine)
                    {
                        path.second.push_back(pPublicTransportPattern->GetUpstreamStop()->GetID());
                        path.second.push_back(pPublicTransportPattern->getLine()->getStrName());
                    }

                    pLastStop = pPublicTransportPattern->GetDownstreamStop();
                    pLastLine = pPublicTransportPattern->getLine();
                }
                else
                {
                    SymuCore::WalkingPattern * pWalkPattern = dynamic_cast<SymuCore::WalkingPattern*>(pPattern);
                    if (pWalkPattern)
                    {
                        if (pLastStop)
                        {
                            path.second.push_back(pLastStop->GetID());
                        }
                        pLastStop = NULL;
                        pLastLine = NULL;
                    }
                }
            }

            pathForDestination.push_back(path);

            delete pPath;
        }
        paths.push_back(pathForDestination);
    }
}

