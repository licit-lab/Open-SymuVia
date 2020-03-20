#include "stdafx.h"
#include "PublicTransportFleet.h"

#include "PublicTransportLine.h"
#include "TripLeg.h"
#include "Xerces/XMLUtil.h"
#include "tools.h"
#include "reseau.h"
#include "Logger.h"
#include "SystemUtil.h"
#include "tuyau.h"
#include "RepartitionTypeVehicule.h"
#include "Schedule.h"
#include "arret.h"
#include "PublicTransportScheduleParameters.h"
#include "PublicTransportFleetParameters.h"
#include "SymuViaTripNode.h"
#include "ConnectionPonctuel.h"
#include "entree.h"
#include "sortie.h"
#include "voie.h"


#include <xercesc/dom/DOMNode.hpp>
#include <xercesc/dom/DOMNodeList.hpp>
#include <xercesc/dom/DOMXPathResult.hpp>

#include <boost/make_shared.hpp>
#include <boost/serialization/map.hpp>


XERCES_CPP_NAMESPACE_USE

PublicTransportFleet::PublicTransportFleet()
{
}

PublicTransportFleet::PublicTransportFleet(Reseau * pNetwork)
    : AbstractFleet(pNetwork)
{
}

PublicTransportFleet::~PublicTransportFleet()
{
    std::map<Trip*, std::map<std::string, ScheduleParameters*> >::iterator iter;
    for(iter = m_MapScheduleParametersByTrip.begin(); iter != m_MapScheduleParametersByTrip.end(); ++iter)
    {
        std::map<std::string, ScheduleParameters*>::iterator iter2;
        for(iter2 = iter->second.begin(); iter2 != iter->second.end(); ++iter2)
        {
            delete iter2->second;
        }
    }
}

AbstractFleetParameters * PublicTransportFleet::CreateFleetParameters()
{
    return new PublicTransportFleetParameters();
}

TypeVehicule * PublicTransportFleet::GetTypeVehicule(Trip * pLine)
{
    return m_MapRepartitionTypeVehicule[pLine]->CalculTypeNewVehicule(m_pNetwork, 0, 0);
}

class StopsSorter 
{
public:
	bool operator()(const Arret *a, const Arret *b) const
	{	
        return a->GetInputPosition().GetPosition() < b->GetInputPosition().GetPosition();	  	
	}
};

bool PublicTransportFleet::Load(DOMNode * pXMLReseau, Logger & loadingLogger)
{
    // SECTION RESEAU / PARAMETRAGE_VEHICULES_GUIDES
    std::string strTmp, strId;
    DOMNode *pLignesBus;
    DOMNode *pArrets;
    DOMNode *pXMLChild;
    Tuyau *pTuyau;
    bool   bSurVoie;

    // On conserve les noeuds de chaque ligne dans une map pour revenir dessus après lecture des arrêts
    std::map<Trip*, DOMNode*> mapLineNodes;

    DOMNode * pXMLNode = m_pNetwork->GetXMLUtil()->SelectSingleNode("./PARAMETRAGE_VEHICULES_GUIDES", pXMLReseau->getOwnerDocument(), (DOMElement*)pXMLReseau);
    
    if(pXMLNode)
    {        
        // Chargement des paramètres d'usage des types de véhicules
        LoadUsageParams(pXMLNode, loadingLogger);

        // SECTION RESEAU / PARAMETRAGE_VEHICULES_GUIDES / LIGNES_TRANSPORT_GUIDEES
        pLignesBus = m_pNetwork->GetXMLUtil()->SelectSingleNode("./LIGNES_TRANSPORT_GUIDEES", pXMLNode->getOwnerDocument(), (DOMElement*)pXMLNode);
        if(pLignesBus)
        {
			XMLSize_t counti = pLignesBus->getChildNodes()->getLength();
			for(XMLSize_t i=0; i<counti;i++)
            {
                pXMLChild = pLignesBus->getChildNodes()->item(i);
				if (pXMLChild->getNodeType() != DOMNode::ELEMENT_NODE) continue;

                // Création de la ligne
                Trip * pLigne = new PublicTransportLine();

                mapLineNodes[pLigne] = pXMLChild;

                // Ajout de la ligne à la liste des trips
                m_LstTrips.push_back(pLigne);

                GetXmlAttributeValue(pXMLChild, "id", strTmp, &loadingLogger);
                pLigne->SetID(strTmp);

                bool bDureeArretsDynamique;
                GetXmlAttributeValue(pXMLChild, "duree_arrets_dynamique", bDureeArretsDynamique, &loadingLogger);
                m_MapDureeArretsDynamique[pLigne] = bDureeArretsDynamique;

                // Définition de la répartition des types de vehicules
                RepartitionTypeVehicule * pRepTypes = new RepartitionTypeVehicule();
                std::vector<TypeVehicule*> lstTypes;
                for(std::map<TypeVehicule*, VehicleTypeUsageParameters>::const_iterator iter = m_MapUsageParameters.begin(); iter != m_MapUsageParameters.end(); ++iter)
                {
                    lstTypes.push_back(iter->first);
                }
                DOMNode * XMLRepTypes = m_pNetwork->GetXMLUtil()->SelectSingleNode("./REP_TYPEVEHICULES", pXMLChild->getOwnerDocument(), (DOMElement*)pXMLChild);
                if(!m_pNetwork->LoadRepTypeVehicules(XMLRepTypes, pLigne->GetID(), 1, lstTypes, pRepTypes, &loadingLogger))
                {
                    return false;
                }
                m_MapRepartitionTypeVehicule[pLigne] = pRepTypes;

                // Chargement des jeux de paramètres
                DOMXPathResult * pListeScheduleParams = m_pNetwork->GetXMLUtil()->SelectNodes("./JEUX_DE_PARAMETRES/PARAMETRES", pXMLChild->getOwnerDocument(), (DOMElement*)pXMLChild);
                XMLSize_t nbParams = pListeScheduleParams->getSnapshotLength();
                for(XMLSize_t iParam = 0; iParam < nbParams; iParam++)
                {
                    pListeScheduleParams->snapshotItem(iParam);
                    DOMNode* pNodeParams = pListeScheduleParams->getNodeValue();

                    PublicTransportScheduleParameters * pParams = new PublicTransportScheduleParameters();
                    pParams->Load(pNodeParams, loadingLogger);
                    m_MapScheduleParametersByTrip[pLigne][pParams->GetID()] = pParams;
                }
                pListeScheduleParams->release();

                // gestion du schedule de la ligne de bus (horaires et fréquences)
                Schedule * pSchedule = new Schedule();
                DOMNode * pXMLNodeSchedule = m_pNetwork->GetXMLUtil()->SelectSingleNode("./CALENDRIER", pXMLChild->getOwnerDocument(), (DOMElement*)pXMLChild);
                if(pXMLNodeSchedule)
                {
                    if(!pSchedule->Load(pXMLNodeSchedule, m_pNetwork, loadingLogger, pLigne->GetID(), m_MapScheduleParametersByTrip[pLigne], m_MapScheduleParameters))
                    {
                        return false;
                    }
                }
                m_MapSchedules[pLigne] = pSchedule;
            }
        }

        // On note la correspondance entre arrêts et lignes dans une map
        std::map<Trip*, std::set<Arret*> > mapStopsForLines;
        
        // SECTION RESEAU / PARAMETRAGE_VEHICULE_GUIDES / ARRETS
        pArrets = m_pNetwork->GetXMLUtil()->SelectSingleNode("./ARRETS", pXMLNode->getOwnerDocument(), (DOMElement*)pXMLNode);
        if(pArrets)
        {
			XMLSize_t counti = pArrets->getChildNodes()->getLength();
			for(XMLSize_t i=0; i<counti;i++)
            {
                pXMLChild = pArrets->getChildNodes()->item(i);
				if (pXMLChild->getNodeType() != DOMNode::ELEMENT_NODE) continue;

                // Chargement des caractéristiques
                GetXmlAttributeValue(pXMLChild, "id", strId, &loadingLogger);

                GetXmlAttributeValue(pXMLChild, "troncon", strTmp, &loadingLogger);
                pTuyau = m_pNetwork->GetLinkFromLabel(strTmp);             
                          
                double dbTmp;
                GetXmlAttributeValue(pXMLChild, "position", dbTmp, &loadingLogger);

                bool bTmp = false;
                GetXmlAttributeValue(pXMLChild, "position_relative", bTmp, &loadingLogger);
                if(bTmp)
                {
                    dbTmp *= pTuyau->GetLength();
                }

                int nTmp = 0;
                GetXmlAttributeValue(pXMLChild, "dureearret", nTmp, &loadingLogger);  

                // Vérification de la position
                if( pTuyau->GetLength() < dbTmp)
                {
                    loadingLogger << Logger::Error << " ERROR : wrong position for station " << strId << " ( position > link's length )" << std::endl;
                    std::cout << "Station position : "; std::cout << dbTmp; std::cout << " Link's length : "; std::cout << pTuyau->GetLength() << std::endl;
                    loadingLogger << Logger::Info; // rebascule en mode INFO pour ne pas avoir à reprendre tous les appels aux log en précisant que c'est des INFO. à supprimer si on reprend tous les appels au log.
                    return false;
                }

                GetXmlAttributeValue(pXMLChild, "pleinevoie", bSurVoie, &loadingLogger);    // Arrêt sur la voie ?

                Arret *pArret = new Arret(strId, m_pNetwork, pTuyau ,dbTmp ,nTmp, bSurVoie);
                m_LstTripNodes.push_back(pArret);

                bool bVoiesManuelles = GetXmlAttributeValue(pXMLChild, "num_voies", strTmp, &loadingLogger);            
				std::deque<std::string> splitVoies;
                if(bVoiesManuelles)
                {
                    splitVoies = SystemUtil::split(strTmp, ' ');
                }

                bool bEligibleForParcRelais;
                GetXmlAttributeValue(pXMLChild, "eligible_parc_relais", bEligibleForParcRelais, &loadingLogger);
                pArret->SetEligibleParcRelais(bEligibleForParcRelais);

                GetXmlAttributeValue(pXMLChild, "lignes", strTmp, &loadingLogger);            
				std::deque<std::string> split = SystemUtil::split(strTmp, ' ');

                if(bVoiesManuelles && (splitVoies.size() != split.size()))
                {
                    loadingLogger << Logger::Error << " ERROR : The number of lane numbers fo station " << strId << " must be equal to the number of lines using this station" << std::endl;
                    loadingLogger << Logger::Info; // rebascule en mode INFO pour ne pas avoir à reprendre tous les appels aux log en précisant que c'est des INFO. à supprimer si on reprend tous les appels au log.
                    return false;
                }

				for(size_t j=0; j<split.size();j++)
                {
                    Trip * pLigne = NULL;
                    for(size_t iLine = 0; iLine < m_LstTrips.size(); iLine++)
                    {
                        if(!m_LstTrips[iLine]->GetID().compare(split.at(j)))
                        {
                            pLigne = m_LstTrips[iLine];
                        }
                    }

                    if(!pLigne)
                    {
                        loadingLogger << Logger::Error << " ERROR : the public transport line " << split.at(j) << " doesn't exist (declaration of station " << pArret->GetID() << "), will be ignored."<< std::endl;                    
                        loadingLogger << Logger::Info; // rebascule en mode INFO pour ne pas avoir à reprendre tous les appels aux log en précisant que c'est des INFO. à supprimer si on reprend tous les appels au log.
                    }
                    else
                    {
                        mapStopsForLines[pLigne].insert(pArret);

                        // récupération des paramètres de montee/descente pour la ligne en cours de traitement
                        // rmq. par défaut, tous les voyageurs descendent à chaque arrêt.
                        double dbRatioDescente = 1;
                        bool bBinomial = false;
                        DOMNode * pXMLNodeDescente = m_pNetwork->GetXMLUtil()->SelectSingleNode("./PARAMETRAGE_DESCENTE/LIGNE[@ligne=\"" + split[j] + "\"]", pXMLChild->getOwnerDocument(), (DOMElement*)pXMLChild);
                        if(pXMLNodeDescente)
                        {
                            GetXmlAttributeValue(pXMLNodeDescente, "ratio_descente", dbRatioDescente, &loadingLogger);
                            std::string typeDescente;
                            GetXmlAttributeValue(pXMLNodeDescente, "typeDescente", typeDescente, &loadingLogger);
                            bBinomial = typeDescente.compare("deterministe");
                        }
                        int nStockInitial = 0;
                        bool bPoisson = false;
                        DOMNode * pXMLNodeMontee = m_pNetwork->GetXMLUtil()->SelectSingleNode("./PARAMETRAGE_MONTEE/LIGNE[@ligne=\"" + split[j] + "\"]", pXMLChild->getOwnerDocument(), (DOMElement*)pXMLChild);
                        if(pXMLNodeMontee)
                        {
                            GetXmlAttributeValue(pXMLNodeMontee, "stock_initial", nStockInitial, &loadingLogger);
                            std::string typeCreation;
                            GetXmlAttributeValue(pXMLNodeMontee, "typeCreation", typeCreation, &loadingLogger);
                            bPoisson = typeCreation.compare("deterministe");

                            // lecture des variantes temporelles de demande
                            DOMXPathResult * pListeDemandes = m_pNetwork->GetXMLUtil()->SelectNodes("DEMANDE", pXMLNodeMontee->getOwnerDocument(), (DOMElement*)pXMLNodeMontee);
                            XMLSize_t nbDemandesArret = pListeDemandes->getSnapshotLength();
                            for(XMLSize_t iDemandeArret = 0; iDemandeArret < nbDemandesArret; iDemandeArret++)
                            {
                                pListeDemandes->snapshotItem(iDemandeArret);
                                DOMNode* pNodeDemandeArret = pListeDemandes->getNodeValue();

                                std::vector<PlageTemporelle*> plages;
                                double dbDuree;
                                if(!GetXmlDuree(pNodeDemandeArret, m_pNetwork, dbDuree, plages, &loadingLogger))
                                {
                                    return false;
                                }

                                boost::shared_ptr<tracked_double> dbDemandeValue = boost::make_shared<tracked_double>();
                                GetXmlAttributeValue(pNodeDemandeArret, "demande", *dbDemandeValue, &loadingLogger);

                                if(plages.size() > 0)
                                {
                                    for(size_t iPlage = 0; iPlage < plages.size(); iPlage++)
                                    {
                                        AddVariation<>( plages[iPlage], dbDemandeValue, pArret->GetLstDemandeTV());  
                                    }
                                }
                                else
                                {
                                    AddVariation<>( dbDuree, dbDemandeValue, pArret->GetLstDemandeTV());  
                                }
                            }
                            pListeDemandes->release();

                            // vérif des plages
                            std::vector<PlageTemporelle*> plages;
                            for(size_t iPlage = 0; iPlage < pArret->GetLstDemandeTV()->size(); iPlage++)
                            {
                                PlageTemporelle * pPlage = pArret->GetLstDemandeTV()->at(iPlage).m_pPlage;
                                if(pPlage)
                                {
                                    plages.push_back(pPlage);
                                }
                            }
                            if(plages.size() > 0 && !CheckPlagesTemporellesEx(m_pNetwork->GetDureeSimu(), plages))
                            {
                                loadingLogger << Logger::Error << "ERROR : The time frames of demand at station " << pArret->GetID() << " don't cover the whole simulation duration !" << std::endl;
                                loadingLogger << Logger::Info; // rebascule en mode INFO pour ne pas avoir à reprendre tous les appels aux log en précisant que c'est des INFO. à supprimer si on reprend tous les appels au log.
                                return false;
                            }
                        }

                        if(bVoiesManuelles)
                        {
                            pArret->AjoutLigne( pLigne, m_MapDureeArretsDynamique[pLigne], bPoisson, dbRatioDescente, bBinomial, nStockInitial, m_MapRepartitionTypeVehicule[pLigne], SystemUtil::ToInt32(splitVoies[j])-1 );
                        }
                        else
                        {
                            pArret->AjoutLigne( pLigne, m_MapDureeArretsDynamique[pLigne], bPoisson, dbRatioDescente, bBinomial, nStockInitial, m_MapRepartitionTypeVehicule[pLigne] );
                        }
                    }
                }
            }
        }

        // Retour sur les lignes de bus
        std::map<Trip*, DOMNode*>::const_iterator iter;
        for(iter = mapLineNodes.begin(); iter != mapLineNodes.end(); ++iter)
        {
            pXMLChild = iter->second;
            Trip * pLigne = iter->first;

            std::set<Arret*> & stops = mapStopsForLines[pLigne];

            // Gestion des origines et destination si différentes du premier et dernier arrêt
            Entree * pEntree = NULL;
            DOMNode * pOriginNode = m_pNetwork->GetXMLUtil()->SelectSingleNode("./ORIGINE", pXMLChild->getOwnerDocument(), (DOMElement*)pXMLChild);
            if(pOriginNode)
            {
                GetXmlAttributeValue(pOriginNode, "id", strTmp, &loadingLogger);
                char cTmp;
                pEntree = dynamic_cast<Entree*>(m_pNetwork->GetOrigineFromID(strTmp, cTmp));
                if(!pEntree)
                {
                    loadingLogger << Logger::Warning << "WARNING : the origin "<< strTmp << " for the bus line " << pLigne->GetID() << " doesn't exist and will be ignored." << std::endl;
                    loadingLogger << Logger::Info; // rebascule en mode INFO pour ne pas avoir à reprendre tous les appels aux log en précisant que c'est des INFO. à supprimer si on reprend tous les appels au log.
                }
            }
            Sortie * pSortie = NULL;
            DOMNode * pDestinationNode = m_pNetwork->GetXMLUtil()->SelectSingleNode("./DESTINATION", pXMLChild->getOwnerDocument(), (DOMElement*)pXMLChild);
            if(pDestinationNode)
            {
                GetXmlAttributeValue(pDestinationNode, "id", strTmp, &loadingLogger);
                char cTmp;
                pSortie = dynamic_cast<Sortie*>(m_pNetwork->GetDestinationFromID(strTmp, cTmp));
                if(!pSortie)
                {
                    loadingLogger << Logger::Warning << "WARNING : the destination "<< strTmp << " for the bus line " << pLigne->GetID() << " doesn't exist and will be ignored." << std::endl;
                    loadingLogger << Logger::Info; // rebascule en mode INFO pour ne pas avoir à reprendre tous les appels aux log en précisant que c'est des INFO. à supprimer si on reprend tous les appels au log.
                }
            }

            Tuyau *pPredTuyau = NULL;    
            // Liste des tronçons parcourus
            std::vector<Tuyau*> lstPreviousLinks;
            DOMXPathResult * pXMLNodeLinks = m_pNetwork->GetXMLUtil()->SelectNodes("./TRONCONS/TRONCON", pXMLChild->getOwnerDocument(), (DOMElement*)pXMLChild);
            XMLSize_t nbLinks = pXMLNodeLinks->getSnapshotLength();
            int nbStopsInLine = 0;
	        for(XMLSize_t iLink = 0; iLink<nbLinks; iLink++)
            {
                pXMLNodeLinks->snapshotItem(iLink);
                DOMNode * pNodeLink = pXMLNodeLinks->getNodeValue();

                GetXmlAttributeValue(pNodeLink, "id", strTmp, &loadingLogger);

				pTuyau = m_pNetwork->GetLinkFromLabel(strTmp);

                if(!pTuyau)
                {            
                    loadingLogger << Logger::Error << "ERROR : the link "<< strTmp << " of the public transport line " << pLigne->GetID() << " doesn't exist." << std::endl;
                    loadingLogger << Logger::Info; // rebascule en mode INFO pour ne pas avoir à reprendre tous les appels aux log en précisant que c'est des INFO. à supprimer si on reprend tous les appels au log.
                    return false;
                }

                // Vérification des coutures
                if(pPredTuyau)
                {
                    if (pTuyau->GetBriqueAmont() != pPredTuyau->GetBriqueAval() ||
                        (pTuyau->GetBriqueAmont() == NULL && pTuyau->getConnectionAmont() != pPredTuyau->getConnectionAval()))
                    {
                        loadingLogger << Logger::Error << "ERROR : the link " << pPredTuyau->GetLabel() << " is not connected to link " << pTuyau->GetLabel() << " (line " << pLigne->GetID() << ")";
                        loadingLogger << Logger::Info; // rebascule en mode INFO pour ne pas avoir à reprendre tous les appels aux log en précisant que c'est des INFO. à supprimer si on reprend tous les appels au log.
                        return false;
                    }
                }       

                // Si on a une origine définie, qu'on est sur iLink == 0 et qu'il correspond, on l'utilise, sinon on l'ignore
                if(pEntree && iLink == 0)
                {
                    if(pEntree->m_LstTuyAv.front() == pTuyau)
                    {
                        // tout est ok : affectation de l'entrée en tant qu'origine
                        pLigne->SetOrigin(pEntree);
                    }
                    else
                    {
                        loadingLogger << Logger::Warning << "WARNING : lhe origin "<< strTmp << " for bus line " << pLigne->GetID() << " is not on the first link of the line and will be ignored." << std::endl;
                        loadingLogger << Logger::Info; // rebascule en mode INFO pour ne pas avoir à reprendre tous les appels aux log en précisant que c'est des INFO. à supprimer si on reprend tous les appels au log.
                    }
                }
                
                // Pour chaque arrêt de la ligne sur le tuyau...
                std::set<Arret*, StopsSorter> stopsForLink;
                for(std::set<Arret*>::const_iterator iter = stops.begin(); iter != stops.end();)
                {
                    if((*iter)->GetInputPosition().GetLink() == pTuyau)
                    {
                        stopsForLink.insert(*iter);
                        stops.erase(iter++);
                    }
                    else
                    {
                        ++iter;
                    }
                }

                // Création éventuelle des TripLeg
                for(std::set<Arret*, StopsSorter>::const_iterator iter = stopsForLink.begin(); iter != stopsForLink.end(); ++iter)
                {
                    // Initialisation de l'origine de la ligne au premier arrêt rencontré si pas d'origine définie
                    if(!pLigne->GetOrigin())
                    {
                        pLigne->SetOrigin(*iter);

                        if(iLink != 0)
                        {
                            loadingLogger << Logger::Warning << "WARNING: the first station ( " << (*iter)->GetID() << " ) of the line " << pLigne->GetID() << " is not on the first link of its path" << std::endl;
                            loadingLogger << Logger::Info; // rebascule en mode INFO pour ne pas avoir à reprendre tous les appels aux log en précisant que c'est des INFO. à supprimer si on reprend tous les appels au log.
                        }

                        lstPreviousLinks.clear();
                        nbStopsInLine++;
                    }
                    else
                    {
                        TripLeg * pTripLeg = new TripLeg();
                        pTripLeg->SetDestination(*iter);
                        std::vector<Tuyau*> path = lstPreviousLinks;
                        path.push_back(pTuyau);
                        pTripLeg->SetPath(path);
                        pLigne->AddTripLeg(pTripLeg);

                        // RAZ de l'itinéraire jusqu'au prochain arrêt
                        lstPreviousLinks.clear();
                        nbStopsInLine++;
                    }
                }

                // Si on a une destination définie sur ce tuyau, on l'utilise + warning si Link != size() - 1  + break 
                if(pSortie && iLink == nbLinks-1)
                {
                    if(pSortie->m_LstTuyAm.front() == pTuyau)
                    {
                        // tout est ok : affectation de la sortie en tant que destination d'une ultime étape
                        TripLeg * pTripLeg = new TripLeg();
                        pTripLeg->SetDestination(pSortie);
                        std::vector<Tuyau*> path = lstPreviousLinks;
                        path.push_back(pTuyau);
                        pTripLeg->SetPath(path);
                        pLigne->AddTripLeg(pTripLeg);
                    }
                    else
                    {
                        loadingLogger << Logger::Warning << "WARNING : the destination "<< strTmp << " for bus line " << pLigne->GetID() << " is not on the last link of the line and will be ignored." << std::endl;
                        loadingLogger << Logger::Info; // rebascule en mode INFO pour ne pas avoir à reprendre tous les appels aux log en précisant que c'est des INFO. à supprimer si on reprend tous les appels au log.
                    }
                }

                pPredTuyau = pTuyau;
                lstPreviousLinks.push_back(pTuyau);
            }
            pXMLNodeLinks->release();


            // Gestion du cas où un seul arrêt est défini pour la ligne, et qu'on n'a pas non plus de destination !
            if(pLigne->GetLegs().empty())
            {
                // Dans ce cas, la destination est le dernier tronçon de la ligne
                Tuyau * pLastLink = lstPreviousLinks.back();
                SymuViaTripNode * pExit = dynamic_cast<SymuViaTripNode*>(pLastLink->getConnectionAval());
                TripLeg * pTripLeg = new TripLeg();
                TripNode * pDest;
                if(pExit)
                {
                    pDest = pExit;
                }
                else
                {
                    pDest = new TripNode(lstPreviousLinks.back()->GetLabel(), m_pNetwork);
                    Position pos(pLastLink, pLastLink->GetLength());
                    pDest->SetInputPosition(pos);
                    m_LstTripNodes.push_back(pDest);
                }
                pTripLeg->SetDestination(pDest);
                pTripLeg->SetPath(lstPreviousLinks);
                pLigne->AddTripLeg(pTripLeg);
            }

            // S'il reste des arrêts, c'est que la ligne est mal définie :
            for(std::set<Arret*>::const_iterator iter = stops.begin(); iter != stops.end(); ++iter)
            {
                loadingLogger << Logger::Warning << "WARNING: the line " <<  pLigne->GetID() << " don't use the link " << (*iter)->GetInputPosition().GetLink()->GetLabel() << " on witch is the station " << (*iter)->GetID() << ". This line is not added to this station's list of lines." << std::endl;
                loadingLogger << Logger::Info; // rebascule en mode INFO pour ne pas avoir à reprendre tous les appels aux log en précisant que c'est des INFO. à supprimer si on reprend tous les appels au log.
            }

            // retour sur les jeux de paramètres associés au calendrier pour associer les durées d'arrêts aux arrêts et vérifier que le nombre correspond.
            std::map<std::string, ScheduleParameters*> & mapScheduleParams = m_MapScheduleParametersByTrip[pLigne];
            std::map<std::string, ScheduleParameters*>::iterator iterParams;
            for(iterParams = mapScheduleParams.begin(); iterParams != mapScheduleParams.end(); ++iterParams)
            {
                if(!((PublicTransportScheduleParameters*)iterParams->second)->AssignStopTimesToStops(pLigne, loadingLogger))
                {
                    return false;
                }
            }
        }
    }

    // Pour le wrapper python
    for(size_t i = 0; i < m_LstTrips.size(); i++)
    {
        m_pNetwork->m_LstLTP.push_back(m_LstTrips[i]);
    }

    return true;
}

bool PublicTransportFleet::OutputVehicleLoad(Vehicule * pVehicle)
{
    Trip * pTrip = GetTrip(pVehicle->GetTrip()->GetID());
    return m_MapDureeArretsDynamique[pTrip];
}

std::string PublicTransportFleet::GetVehicleIdentifier(boost::shared_ptr<Vehicule> pVeh)
{
    std::ostringstream oss;
    oss << "BUS_" << pVeh->GetTrip()->GetID();
    return oss.str();
}

template void PublicTransportFleet::serialize(boost::archive::xml_woarchive & ar, const unsigned int version);
template void PublicTransportFleet::serialize(boost::archive::xml_wiarchive & ar, const unsigned int version);

template<class Archive>
void PublicTransportFleet::serialize(Archive& ar, const unsigned int version)
{
    ar & BOOST_SERIALIZATION_BASE_OBJECT_NVP(AbstractFleet);

    ar & BOOST_SERIALIZATION_NVP(m_MapScheduleParametersByTrip);
    ar & BOOST_SERIALIZATION_NVP(m_MapDureeArretsDynamique);
}

