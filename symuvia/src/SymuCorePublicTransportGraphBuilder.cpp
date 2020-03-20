#include "stdafx.h"
#include "SymuCorePublicTransportGraphBuilder.h"

#include "SymuCoreGraphBuilder.h"
#include "SymuCoreDrivingGraphBuilder.h"

#include "reseau.h"
#include "tuyau.h"
#include "tools.h"
#include "arret.h"
#include "Plaque.h"
#include "Parking.h"
#include "usage/Trip.h"
#include "usage/TripLeg.h"
#include "usage/TripNode.h"
#include "usage/PublicTransportFleet.h"
#include "ArretPublicTransportPenalty.h"
#include "SymuViaPublicTransportPattern.h"
#include "ParcRelaisPenalty.h"

#include <Graph/Model/MultiLayersGraph.h>
#include <Graph/Model/Link.h>
#include <Graph/Model/Node.h>
#include <Graph/Model/PublicTransport/PublicTransportPattern.h>
#include <Graph/Model/PublicTransport/WalkingPattern.h>
#include <Graph/Model/PatternsSwitch.h>
#include <Graph/Model/NullPenalty.h>
#include <Graph/Model/PublicTransport/PublicTransportLine.h>
#include <Demand/MacroType.h>
#include <Demand/Path.h>
#include <Demand/Origin.h>
#include <Utils/Point.h>

#include <boost/date_time/posix_time/posix_time_types.hpp>

using namespace SymuCore;

SymuCorePublicTransportGraphBuilder::SymuCorePublicTransportGraphBuilder()
: m_pParent(NULL),
m_pNetwork(NULL)
{
}

SymuCorePublicTransportGraphBuilder::SymuCorePublicTransportGraphBuilder(Reseau * pNetwork)
: m_pParent(NULL),
m_pNetwork(pNetwork)
{
}


SymuCorePublicTransportGraphBuilder::~SymuCorePublicTransportGraphBuilder()
{
}

void SymuCorePublicTransportGraphBuilder::SetParent(SymuCoreGraphBuilder * pParent)
{
	m_pParent = pParent;
}

SymuCoreGraphBuilder * SymuCorePublicTransportGraphBuilder::GetParent()
{
    return m_pParent;
}

bool SymuCorePublicTransportGraphBuilder::CompareLink::operator()(const SymuCore::Link* l1, const SymuCore::Link* l2) const
{
    return l1->getUpstreamNode()->getStrName() < l2->getUpstreamNode()->getStrName()
        || (l1->getUpstreamNode()->getStrName() == l2->getUpstreamNode()->getStrName() && l1->getDownstreamNode()->getStrName() < l2->getDownstreamNode()->getStrName());
}

bool SymuCorePublicTransportGraphBuilder::CompareLine::operator()(const SymuCore::PublicTransportLine* l1, const SymuCore::PublicTransportLine* l2) const
{
    return l1->getStrName() < l2->getStrName();
}

bool SymuCorePublicTransportGraphBuilder::CreatePublicTransportLayer(SymuCore::MultiLayersGraph * pGraph, SymuCoreDrivingGraphBuilder * pDrivingGraphBuilder)
{
    // 1 - Recupération des PublicTransportFleet
    m_pPublicTransportFleet = m_pNetwork->GetPublicTransportFleet();
    if(!m_pPublicTransportFleet)
    {
        //on stop si il n'y a pas de transport en commun pour symuvia
        return true;
    }

    Graph * pPublicTransportLayer = pGraph->CreateAndAddLayer(SymuCore::ST_PublicTransport);

    // 2 - création des Nodes
    Arret * pArret;
    Node * pNode;
    for (size_t iTripNode = 0; iTripNode < m_pPublicTransportFleet->GetTripNodes().size(); iTripNode++)
    {
        pArret = dynamic_cast<Arret *>(m_pPublicTransportFleet->GetTripNodes()[iTripNode]);
        if(!pArret)
        {
            continue;
        }

        const std::string & arretId = pArret->GetID();

        // Un arret est forcement rattaché a un tuyau
        assert(pArret->GetInputPosition().GetLink());

        ::Point SymuViaPt;
        CalcCoordFromPos(pArret->GetInputPosition().GetLink(), pArret->GetInputPosition().GetPosition(), SymuViaPt);
        SymuCore::Point pt(SymuViaPt.dbX, SymuViaPt.dbY);

        NodeType nodeType = NT_PublicTransportStation;
        pNode = pPublicTransportLayer->CreateAndAddNode(arretId, nodeType, &pt);

        m_mapArretsToNode[pArret] = pNode;
        m_mapNodeToArrets[pNode] = pArret;
    }

    // 3 - création des PublicTransportLines et des Links entre les arrêts d'une même ligne
    ::Trip * pTrip;
    Link * pLink;
    std::map<std::pair<Node*, Node*>, Link*> mapLinks;
    for (size_t iTrip = 0; iTrip < m_pPublicTransportFleet->GetTrips().size(); iTrip++)
    {
        pTrip = m_pPublicTransportFleet->GetTrips()[iTrip];

        //Création et ajout du PublicTransportLine
        SymuCore::PublicTransportLine * pPublicTransportLine = new SymuCore::PublicTransportLine(pTrip->GetID());
        pPublicTransportLayer->AddPublicTransportLine(pPublicTransportLine);

        m_mapLineToTrip[pPublicTransportLine] = pTrip;

        //identification des différents couples d'arrêts
        TripNode * pPreviousStop = pTrip->GetOrigin();
        TripNode * pNextStop;
        for(size_t i = 0; i < pTrip->GetLegs().size(); i++)
        {
            TripLeg * pTripLeg = pTrip->GetLegs()[i];
            pNextStop = pTripLeg->GetDestination();
            // rmq. on peut ne pas avoir des arrêts ici aux bouts des lignes (extremités ponctuelles si la ligne sort du réseau)
            Arret * pUpstreamStop = dynamic_cast<Arret*>(pPreviousStop);
            Arret * pDownstreamStop = dynamic_cast<Arret*>(pNextStop);
            if (pUpstreamStop && pDownstreamStop)
            {
                // On crée un seul Link même si plusieurs trip relient le même couple noeud amont + noeud aval
                std::pair<Node*, Node*> linkDef = std::make_pair(m_mapArretsToNode.at(pUpstreamStop), m_mapArretsToNode.at(pDownstreamStop));

                std::map<std::pair<Node*, Node*>, Link*>::const_iterator iterLink = mapLinks.find(linkDef);
                if (iterLink == mapLinks.end())
                {
                    // pas de doublon pour le moment : on crée le link
                    pLink = pPublicTransportLayer->CreateAndAddLink(linkDef.first, linkDef.second);
                    mapLinks[linkDef] = pLink;
                }
                else
                {
                    // cas du doublon
                    pLink = iterLink->second;
                }
                m_mapLinksToTuyaux[pLink][pPublicTransportLine] = pTripLeg->GetPath();
            }
            pPreviousStop = pNextStop;
        }
    }

    // 4 - création des links entre lignes de bus
    BuildLinksBetweenLines(pPublicTransportLayer);

    assert(m_pNetwork->IsWithPublicTransportGraph() || m_pNetwork->IsSymuMasterMode());

    // Pour le SG (donc sans SymuMaster), on ne gère pas les notions de zones ni de park relai (en tout cas pas 
    // comme ça pour les parcs relais : on décompose les calculs d'itinéraires en une phase VL et une phase TEC)
    if (m_pNetwork->IsSymuMasterMode())
    {
        // 5 - création des Links de marche en zone / plaque
        BuildLinksWithZones(pPublicTransportLayer, pDrivingGraphBuilder);

        // 6 - création des Links de marche entre parcs relais et arrêts associés
        BuildLinksWithCarParks(pPublicTransportLayer, pDrivingGraphBuilder);
    }      

    // initialement, on ne faisait pas de lien vers les origines et destinations ponctuelles avec SymuMaster (mais uniquement pour le SG),
    // mais suite au besoin Mostafa, on fait ces deux étapes dans tous les cas à présent.

    // 7 - création des Links de marche enter origine potentielle et arrêt
    BuildLinksWithOrigins(pPublicTransportLayer, pDrivingGraphBuilder);

    // 8 - création des Links de marche entre arret et destination potentielle
    BuildLinksWithDestinations(pPublicTransportLayer, pDrivingGraphBuilder);

    // 9 - création des patterns et penalties
    BuildPatternsAndPenalties(pPublicTransportLayer);

    return true;
}

void SymuCorePublicTransportGraphBuilder::BuildLinksBetweenLines(SymuCore::Graph * pPublicTransportLayer)
{
    for (std::map<Arret*, Node*, LessConnexionPtr<Arret> >::const_iterator iterStop1 = m_mapArretsToNode.begin(); iterStop1 != m_mapArretsToNode.end(); ++iterStop1)
    {
        for (std::map<Arret*, Node*, LessConnexionPtr<Arret> >::const_iterator iterStop2 = m_mapArretsToNode.begin(); iterStop2 != m_mapArretsToNode.end(); ++iterStop2)
        {
            if (iterStop1->first != iterStop2->first)
            {
                // On vérifie que les deux arrêts ne partagent pas une ligne
                if(std::find_first_of(iterStop1->first->getLstLTPs().begin(), iterStop1->first->getLstLTPs().end(),
                    iterStop2->first->getLstLTPs().begin(), iterStop2->first->getLstLTPs().end()) == iterStop1->first->getLstLTPs().end())
                {
                    // Si la distance est inférieure au radius max de marche, création d'un link
                    double dbDistance = iterStop1->second->getCoordinates()->distanceTo(iterStop2->second->getCoordinates());
                    if (dbDistance <= m_pNetwork->GetMaxIntermediateWalkRadius())
                    {
                        m_lstIntermediateWalkingLinks[pPublicTransportLayer->CreateAndAddLink(iterStop1->second, iterStop2->second)] = dbDistance;
                    }
                }
            }
        }
    }
}

void SymuCorePublicTransportGraphBuilder::BuildLinksWithZones(SymuCore::Graph * pPublicTransportLayer, SymuCoreDrivingGraphBuilder * pDrivingGraphBuilder)
{
    // Pour chaque zone :
    const std::map<ZoneDeTerminaison*, SymuCore::Node*, LessConnexionPtr<ZoneDeTerminaison> > & mapZones = pDrivingGraphBuilder->GetMapZones();
    for (std::map<ZoneDeTerminaison*, SymuCore::Node*, LessConnexionPtr<ZoneDeTerminaison> >::const_iterator iterZones = mapZones.begin(); iterZones != mapZones.end(); ++iterZones)
    {
        ZoneDeTerminaison * pZone = iterZones->first;

        bool bZoneOrigin = pPublicTransportLayer->getParent()->hasOriginOrDownstreamInterface(pZone->GetID());
        bool bZoneDestination = pPublicTransportLayer->getParent()->hasDestinationOrUpstreamInterface(pZone->GetID());

        // identification de l'ensemble des arrêts de la zone :
        for (std::map<Arret*, Node*, LessConnexionPtr<Arret> >::const_iterator iterStop = m_mapArretsToNode.begin(); iterStop != m_mapArretsToNode.end(); ++iterStop)
        {
            if (pZone->GetOutputPosition().IsInZone(iterStop->first->GetOutputPosition().GetLink()))
            {
                // l'arrêt est en zone, création des links correspondants, quel que soit la distance malgré le radius de marche défini.
				if (bZoneOrigin)
                {
                    m_lstTerminationWalkingLinks.push_back(pPublicTransportLayer->CreateAndAddLink(iterZones->second, iterStop->second));
                }

				if (bZoneDestination)
                {
                    m_lstTerminationWalkingLinks.push_back(pPublicTransportLayer->CreateAndAddLink(iterStop->second, iterZones->second));
                }
            }
        }
    }

    // Pour chaque plaque :
    const std::map<CPlaque*, SymuCore::Node*, LessConnexionPtr<CPlaque> > & mapPlaques = pDrivingGraphBuilder->GetMapPlaques();
    for (std::map<CPlaque*, SymuCore::Node*, LessConnexionPtr<CPlaque> >::const_iterator iterPlaques = mapPlaques.begin(); iterPlaques != mapPlaques.end(); ++iterPlaques)
    {
        CPlaque * pPlaque = iterPlaques->first;

		bool bPlaqueOrigin = pPublicTransportLayer->getParent()->hasOriginOrDownstreamInterface(pPlaque->GetID());
        bool bPlaqueDestination = pPublicTransportLayer->getParent()->hasDestinationOrUpstreamInterface(pPlaque->GetID());

        // Constitution de la liste des arrêts de la zone :
        ZoneDeTerminaison * pParentZone = pPlaque->GetParentZone();

        // identification de l'ensemble des arrêts de la zone et calcul de la distance à la plaque :
        std::map<double, std::vector<std::pair<Arret*, Node*> > > stopsInZone;
        std::vector<Node*> stopsInPlaque;
        for (std::map<Arret*, Node*, LessConnexionPtr<Arret> >::const_iterator iterStop = m_mapArretsToNode.begin(); iterStop != m_mapArretsToNode.end(); ++iterStop)
        {
            if (pPlaque->ContainsPosition(iterStop->first->GetInputPosition()))
            {
                stopsInPlaque.push_back(iterStop->second);
            }
            else
            {
                // S'il n'est pas dans la plaque, on regarde quand même s'il est dans la zone en calculant la distance
                if (pParentZone->GetOutputPosition().IsInZone(iterStop->first->GetOutputPosition().GetLink()))
                {
                    double dbDistance = iterStop->second->getCoordinates()->distanceTo(iterPlaques->second->getCoordinates());
                    stopsInZone[dbDistance].push_back(std::make_pair(iterStop->first, iterStop->second));
                }
            }
        }

        // du coup, on conserve tous les arrêts de la plaque plus jusqu'à N plus proches dans la zone:
        int nbStopsInZoneAdded = 0;
        for (std::map<double, std::vector<std::pair<Arret*, Node*> > >::const_iterator iterStopInZone = stopsInZone.begin(); iterStopInZone != stopsInZone.end() && nbStopsInZoneAdded < m_pNetwork->GetNbStopsToConnectOutsideOfSubAreas(); ++iterStopInZone)
        {
            for (size_t iStopInZoneForDistance = 0; iStopInZoneForDistance < iterStopInZone->second.size() && nbStopsInZoneAdded < m_pNetwork->GetNbStopsToConnectOutsideOfSubAreas(); iStopInZoneForDistance++)
            {
                stopsInPlaque.push_back(iterStopInZone->second.at(iStopInZoneForDistance).second);
                nbStopsInZoneAdded++;
            }
        }
            
        for (size_t iStop = 0; iStop < stopsInPlaque.size(); iStop++)
        {
			if (bPlaqueOrigin)
            {
                m_lstTerminationWalkingLinks.push_back(pPublicTransportLayer->CreateAndAddLink(iterPlaques->second, stopsInPlaque[iStop]));
            }

			if (bPlaqueDestination)
            {
                m_lstTerminationWalkingLinks.push_back(pPublicTransportLayer->CreateAndAddLink(stopsInPlaque[iStop], iterPlaques->second));
            }
        }
    }
}

void SymuCorePublicTransportGraphBuilder::BuildLinksWithCarParks(SymuCore::Graph * pPublicTransportLayer, SymuCoreDrivingGraphBuilder * pDrivingGraphBuilder)
{
    // Pour chaque parc relais :
    for (size_t iParking = 0; iParking < m_pNetwork->Liste_parkings.size(); iParking++)
    {
        Parking * pPark = m_pNetwork->Liste_parkings.at(iParking);
        if (pPark->IsParcRelais() && (pPark->GetInputConnexion() || pPark->GetOutputConnexion()))
        {
            Node * pParkNode = pDrivingGraphBuilder->GetNodeFromConnexion(pPark->GetInputConnexion() ? pPark->GetInputConnexion() : pPark->GetOutputConnexion());
            
            bool bConnectedCarPark = false;
            // Recherche des stations proches :
            for (std::map<Arret*, Node*, LessConnexionPtr<Arret> >::const_iterator iterStop = m_mapArretsToNode.begin(); iterStop != m_mapArretsToNode.end(); ++iterStop)
            {
                if (iterStop->first->IsEligibleParcRelais())
                {
                    // Calcul de la distance et test avec le radius
                    double dbDistance = pParkNode->getCoordinates()->distanceTo(iterStop->second->getCoordinates());
                    if (dbDistance <= pPark->GetParcRelaisMaxDistance())
                    {
                        if (pPark->GetInputConnexion())
                        {
                            m_lstCarParkWalkingLinks[pPublicTransportLayer->CreateAndAddLink(pParkNode, iterStop->second)] = dbDistance;
                            bConnectedCarPark = true;
                        }
                        if (pPark->GetOutputConnexion())
                        {
                            m_lstCarParkWalkingLinks[pPublicTransportLayer->CreateAndAddLink(iterStop->second, pParkNode)] = dbDistance;
                            bConnectedCarPark = true;
                        }
                    }
                }
            }
            if (bConnectedCarPark)
            {
                m_lstCarParks[pParkNode] = pPark;
            }
        }
    }
}

void SymuCorePublicTransportGraphBuilder::BuildLinksWithOrigins(SymuCore::Graph * pPublicTransportLayer, SymuCoreDrivingGraphBuilder * pDrivingGraphBuilder)
{

    SymuCore::Graph * pDrivingLayer = pPublicTransportLayer->getParent()->GetListLayers().front();
    const std::vector<SymuCore::Node*> & listNodes = pDrivingLayer->GetListNodes();
    for (std::map<Arret*, Node*, LessConnexionPtr<Arret> >::const_iterator iterStop = m_mapArretsToNode.begin(); iterStop != m_mapArretsToNode.end(); ++iterStop)
    {
        for (size_t iNode = 0; iNode < listNodes.size(); iNode++)
        {
            SymuCore::Node * pCandidateNode = listNodes.at(iNode);

            // For SymuMaster : only care about real origins
            if (m_pNetwork->IsSymuMasterMode() && !pPublicTransportLayer->getOrigin(pCandidateNode->getStrName()))
                continue;

            double dbDistance = iterStop->second->getCoordinates()->distanceTo(pCandidateNode->getCoordinates());
            if (dbDistance <= m_pNetwork->GetMaxInitialWalkRadius())
            {
                Connexion * pCon = pDrivingGraphBuilder->GetConnexionFromNode(pCandidateNode, false);
                // on élimine les sorties en vérifiant qu'on a au moins un tronçon aval
                if (pCon && !pCon->m_LstTuyAssAv.empty())
                {
                    SymuCore::Node * pOriginPublicTransportNode = NULL;
                    std::map<Connexion*, SymuCore::Node*>::const_iterator iterCon = m_mapOriginConnexionsToNodes.find(pCon);
                    if (iterCon == m_mapOriginConnexionsToNodes.end())
                    {
                        pOriginPublicTransportNode = pPublicTransportLayer->CreateAndAddNode(pCon->GetID(), SymuCore::NT_Undefined, pCandidateNode->getCoordinates());
                        m_mapOriginConnexionsToNodes[pCon] = pOriginPublicTransportNode;
                        m_mapPonctualOriginToSymuCoreNodes[pCandidateNode] = pOriginPublicTransportNode;
                    }
                    else
                    {
                        pOriginPublicTransportNode = iterCon->second;
                    }

                    m_lstInitialWalkingLinks[pPublicTransportLayer->CreateAndAddLink(pOriginPublicTransportNode, iterStop->second)] = dbDistance;
                }
            }
        }
    }
}

SymuCore::Node * SymuCorePublicTransportGraphBuilder::GetOriginNodeFromConnexion(Connexion * pCon) const
{
    std::map<Connexion*, SymuCore::Node*>::const_iterator iter = m_mapOriginConnexionsToNodes.find(pCon);
    if (iter != m_mapOriginConnexionsToNodes.end())
    {
        return iter->second;
    }
    return NULL;
}

SymuCore::Node * SymuCorePublicTransportGraphBuilder::GetDestinationNodeFromTripNode(TripNode * pTripNode) const
{
    std::map<TripNode*, SymuCore::Node*>::const_iterator iter = m_mapPonctualDestinationToNodes.find(pTripNode);
    if (iter != m_mapPonctualDestinationToNodes.end())
    {
        return iter->second;
    }
    return NULL;
}

SymuCore::Node * SymuCorePublicTransportGraphBuilder::GetOriginNodeFromNode(SymuCore::Node * pNode) const
{
    std::map<SymuCore::Node*, SymuCore::Node*>::const_iterator iter = m_mapPonctualOriginToSymuCoreNodes.find(pNode);
    if (iter != m_mapPonctualOriginToSymuCoreNodes.end())
    {
        return iter->second;
    }
    return NULL;
}

SymuCore::Node * SymuCorePublicTransportGraphBuilder::GetDestinationNodeFromNode(SymuCore::Node * pNode) const
{
    std::map<SymuCore::Node*, SymuCore::Node*>::const_iterator iter = m_mapPonctualDestinationToSymuCoreNodes.find(pNode);
    if (iter != m_mapPonctualDestinationToSymuCoreNodes.end())
    {
        return iter->second;
    }
    return NULL;
}

SymuCore::Node * SymuCorePublicTransportGraphBuilder::GetNodeFromStop(Arret * pStop) const
{
    return m_mapArretsToNode.at(pStop);
}

void SymuCorePublicTransportGraphBuilder::BuildLinksWithDestinations(SymuCore::Graph * pPublicTransportLayer, SymuCoreDrivingGraphBuilder * pDrivingGraphBuilder)
{
    for (std::map<Arret*, Node*, LessConnexionPtr<Arret> >::const_iterator iterStop = m_mapArretsToNode.begin(); iterStop != m_mapArretsToNode.end(); ++iterStop)
    {
        std::map<SymuCore::Node*, SymuViaTripNode*>::const_iterator iterExternalCon;
        for (iterExternalCon = pDrivingGraphBuilder->GetMapNodesToExternalConnexions().begin(); iterExternalCon != pDrivingGraphBuilder->GetMapNodesToExternalConnexions().end(); ++iterExternalCon)
        {
            // For SymuMaster : only care about real origins
            if (m_pNetwork->IsSymuMasterMode() && !pPublicTransportLayer->getDestination(iterExternalCon->first->getStrName()))
                continue;

            double dbDistance = iterStop->second->getCoordinates()->distanceTo(iterExternalCon->first->getCoordinates());
            if (dbDistance <= m_pNetwork->GetMaxInitialWalkRadius())
            {
                SymuCore::Node * pDestinationPublicTransportNode = NULL;
                std::map<TripNode*, SymuCore::Node*>::const_iterator iterCon = m_mapPonctualDestinationToNodes.find(iterExternalCon->second);
                if (iterCon == m_mapPonctualDestinationToNodes.end())
                {
                    pDestinationPublicTransportNode = pPublicTransportLayer->CreateAndAddNode(iterExternalCon->second->GetID(), SymuCore::NT_Undefined, iterExternalCon->first->getCoordinates());
                    m_mapPonctualDestinationToNodes[iterExternalCon->second] = pDestinationPublicTransportNode;
                    m_mapPonctualDestinationToSymuCoreNodes[iterExternalCon->first] = pDestinationPublicTransportNode;
                }
                else
                {
                    pDestinationPublicTransportNode = iterCon->second;
                }

                m_lstInitialWalkingLinks[pPublicTransportLayer->CreateAndAddLink(iterStop->second, pDestinationPublicTransportNode)] = dbDistance;
            }
        }
    }
}

void SymuCorePublicTransportGraphBuilder::BuildPatternsAndPenalties(SymuCore::Graph * pPublicTransportLayer)
{
    // 1 - création des patterns
    BuildPatterns(pPublicTransportLayer);

    // 2 - création des penalties
    BuildPenalties(pPublicTransportLayer);

}

void SymuCorePublicTransportGraphBuilder::BuildPatterns(SymuCore::Graph * pPublicTransportLayer)
{
    // 1 - Création de patterns en bus le long des lignes
    Link * pLink;
    std::map<SymuCore::Link*, std::map<SymuCore::PublicTransportLine*, std::vector<Tuyau*>, CompareLine>, CompareLink>::iterator itMap;
    for (itMap = m_mapLinksToTuyaux.begin(); itMap != m_mapLinksToTuyaux.end(); ++itMap)
    {
        pLink = itMap->first;
        assert(pLink->getParent() == pPublicTransportLayer);
        Arret* arretAmt = m_mapNodeToArrets.at(pLink->getUpstreamNode());
        Arret* arretAvl = m_mapNodeToArrets.at(pLink->getDownstreamNode());

        // Pour chaque ligne passant par ce Link, création des patterns correspondants :
        std::map<SymuCore::PublicTransportLine*, std::vector<Tuyau*>, CompareLine>::const_iterator iterLine;
        for (iterLine = itMap->second.begin(); iterLine != itMap->second.end(); ++iterLine)
        {
            // Un pattern pour le bus :
            SymuViaPublicTransportPattern* pPublicTransportPattern = new SymuViaPublicTransportPattern(pLink, iterLine->first, m_pNetwork, m_mapLineToTrip.at(iterLine->first),
                iterLine->second, arretAmt, arretAvl);
            pLink->getListPatterns().push_back(pPublicTransportPattern);
        }

        // Un pattern pour la marche à pieds si la distance est bonne
        double dbLength = pLink->getUpstreamNode()->getCoordinates()->distanceTo(pLink->getDownstreamNode()->getCoordinates());
        if (dbLength <= m_pNetwork->GetMaxIntermediateWalkRadius())
        {
            WalkingPattern * pWalkingPattern = new WalkingPattern(pLink, false, true, dbLength);
            pLink->getListPatterns().push_back(pWalkingPattern);
        }
    }

    // 2 - création de patterns de marche
    for (size_t iWalkingLink = 0; iWalkingLink < m_lstTerminationWalkingLinks.size(); iWalkingLink++)
    {
        Link * pWalkingLink = m_lstTerminationWalkingLinks[iWalkingLink];
        WalkingPattern * pWalkingPattern = new WalkingPattern(pWalkingLink, true, false, -1);
        pWalkingLink->getListPatterns().push_back(pWalkingPattern);
    }
    for (std::map<SymuCore::Link*, double>::const_iterator iterLink = m_lstIntermediateWalkingLinks.begin(); iterLink != m_lstIntermediateWalkingLinks.end(); ++iterLink)
    {
        Link * pWalkingLink = iterLink->first;
        WalkingPattern * pWalkingPattern = new WalkingPattern(pWalkingLink, false, true, iterLink->second);
        pWalkingLink->getListPatterns().push_back(pWalkingPattern);
    }
    for (std::map<SymuCore::Link*, double>::const_iterator iterLink = m_lstInitialWalkingLinks.begin(); iterLink != m_lstInitialWalkingLinks.end(); ++iterLink)
    {
        Link * pWalkingLink = iterLink->first;
        WalkingPattern * pWalkingPattern = new WalkingPattern(pWalkingLink, true, true, iterLink->second);
        pWalkingLink->getListPatterns().push_back(pWalkingPattern);
    }
    for (std::map<SymuCore::Link*, double>::const_iterator iterLink = m_lstCarParkWalkingLinks.begin(); iterLink != m_lstCarParkWalkingLinks.end(); ++iterLink)
    {
        Link * pWalkingLink = iterLink->first;
        WalkingPattern * pWalkingPattern = new WalkingPattern(pWalkingLink, false, true, iterLink->second);
        pWalkingLink->getListPatterns().push_back(pWalkingPattern);
	}
	/*for (std::map<SymuCore::Link*, double>::const_iterator iterLink = m_lstOtherSimulatorWalkingLinks.begin(); iterLink != m_lstOtherSimulatorWalkingLinks.end(); ++iterLink)
	{
		Link * pWalkingLink = iterLink->first;
		WalkingPattern * pWalkingPattern = new WalkingPattern(pWalkingLink, false, true, iterLink->second);
		pWalkingLink->getListPatterns().push_back(pWalkingPattern);
	}*/
}

void SymuCorePublicTransportGraphBuilder::BuildPenalties(SymuCore::Graph * pPublicTransportLayer)
{
    // 1 - création des pénalités aux arrêts
    std::map<Arret*, SymuCore::Node*, LessConnexionPtr<Arret> >::const_iterator iterNode;
    Pattern * pUpstreamPat, *pDownstreamPat;
    Link * pUpstreamLink, *pDownstreamLink;
    Arret * pArret;
    Node * pNode;
    for (iterNode = m_mapArretsToNode.begin(); iterNode != m_mapArretsToNode.end(); ++iterNode)
    {
        pNode = iterNode->second;

        assert(pNode->getParent() == pPublicTransportLayer);

        pArret = iterNode->first;

		// Pour chaque lien amont
		for (size_t iUpstreamLink = 0; iUpstreamLink < pNode->getUpstreamLinks().size(); iUpstreamLink++)
		{
			pUpstreamLink = pNode->getUpstreamLinks()[iUpstreamLink];

            // pas encore de liens intercouches à ce niveau
            assert(pUpstreamLink->getParent() == pPublicTransportLayer);

            // Pour chaque pattern amont
            for (size_t iUpstreamPattern = 0; iUpstreamPattern < pUpstreamLink->getListPatterns().size(); iUpstreamPattern++)
            {
                pUpstreamPat = pUpstreamLink->getListPatterns()[iUpstreamPattern];

                // Pour chaque lien aval
                for (size_t iDownstreamLink = 0; iDownstreamLink < pNode->getDownstreamLinks().size(); iDownstreamLink++)
                {
                    pDownstreamLink = pNode->getDownstreamLinks()[iDownstreamLink];
                    // pas encore de liens intercouches à ce niveau
                    assert(pDownstreamLink->getParent() == pPublicTransportLayer);
                    
                    // Pour chaque pattern aval
                    for (size_t iDownstreamPattern = 0; iDownstreamPattern < pDownstreamLink->getListPatterns().size(); iDownstreamPattern++)
                    {
                        pDownstreamPat = pDownstreamLink->getListPatterns()[iDownstreamPattern];

                        ArretPublicTransportPenalty * pPenalty = new ArretPublicTransportPenalty(pNode, PatternsSwitch(pUpstreamPat, pDownstreamPat), m_pNetwork, pArret);
                        pNode->getMapPenalties()[pUpstreamPat][pDownstreamPat] = pPenalty;
                    }
                }
            }
		}
    }

    // 2 - création de pénalités aux parcs relais
    for (std::map<Node*, Parking*>::const_iterator iterPark = m_lstCarParks.begin(); iterPark != m_lstCarParks.end(); ++iterPark)
    {
        pNode = iterPark->first;

        // Pour chaque lien amont
        for (size_t iUpstreamLink = 0; iUpstreamLink < pNode->getUpstreamLinks().size(); iUpstreamLink++)
        {
            pUpstreamLink = pNode->getUpstreamLinks()[iUpstreamLink];

            // Pour chaque pattern amont
            for (size_t iUpstreamPattern = 0; iUpstreamPattern < pUpstreamLink->getListPatterns().size(); iUpstreamPattern++)
            {
                pUpstreamPat = pUpstreamLink->getListPatterns()[iUpstreamPattern];

                // Pour chaque lien aval
                for (size_t iDownstreamLink = 0; iDownstreamLink < pNode->getDownstreamLinks().size(); iDownstreamLink++)
                {
                    pDownstreamLink = pNode->getDownstreamLinks()[iDownstreamLink];

                    // Pour chaque pattern aval
                    for (size_t iDownstreamPattern = 0; iDownstreamPattern < pDownstreamLink->getListPatterns().size(); iDownstreamPattern++)
                    {
                        pDownstreamPat = pDownstreamLink->getListPatterns()[iDownstreamPattern];

                        // si marche -> driving ou dricing -> marche
                        if ((pUpstreamPat->getPatternType() == PT_Walk && pDownstreamPat->getPatternType() == PT_Driving)
                            || (pUpstreamPat->getPatternType() == PT_Driving && pDownstreamPat->getPatternType() == PT_Walk))
                        {
                            ParcRelaisPenalty * pPenalty = new ParcRelaisPenalty(pNode, PatternsSwitch(pUpstreamPat, pDownstreamPat), iterPark->second);
                            pNode->getMapPenalties()[pUpstreamPat][pDownstreamPat] = pPenalty;
                        }
                        // Pas de création de pénalité dans les autres cas (on peut pas arriver en conduisant et repartir en conduisant, idem pour la marche à pieds
                    }
                }
            }
        }
    }
}
