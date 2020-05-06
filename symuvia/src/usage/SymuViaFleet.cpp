#include "stdafx.h"
#include "SymuViaFleet.h"

#include "SymuViaTripNode.h"
#include "Trip.h"
#include "TripLeg.h"
#include "reseau.h"
#include "entree.h"
#include "sortie.h"
#include "Parking.h"
#include "ZoneDeTerminaison.h"
#include "Logger.h"
#include "voie.h"
#include "tuyau.h"
#include "TronconOrigine.h"
#include "../Affectation.h"
#include "SystemUtil.h"
#include "usage/SymuViaFleetParameters.h"
#include "usage/SymuViaVehicleToCreate.h"
#include "ControlZone.h"
#ifdef USE_SYMUCOM
#include "ITS/Stations/DerivedClass/Simulator.h"
#endif // USE_SYMUCOM


SymuViaFleet::SymuViaFleet()
    : AbstractFleet()
{
}

SymuViaFleet::SymuViaFleet(Reseau * pNetwork)
    : AbstractFleet(pNetwork)
{
}

SymuViaFleet::~SymuViaFleet()
{
}

AbstractFleetParameters * SymuViaFleet::CreateFleetParameters()
{
    return new SymuViaFleetParameters();
}

void SymuViaFleet::InitSimuTrafic(std::deque< TimeVariation<TraceDocTrafic> > & docTrafics)
{
    // Nettoyage (utile en cas de SymReset)
    m_LstTripNodes.clear();
    for (size_t i = 0; i < m_LstTrips.size(); i++)
    {
        delete m_LstTrips[i];
    }
    m_LstTrips.clear();

    // Définition des TripNodes associés à la flotte
    m_LstTripNodes.insert(m_LstTripNodes.end(), m_pNetwork->Liste_entree.begin(), m_pNetwork->Liste_entree.end());
    m_LstTripNodes.insert(m_LstTripNodes.end(), m_pNetwork->Liste_sortie.begin(), m_pNetwork->Liste_sortie.end());
    m_LstTripNodes.insert(m_LstTripNodes.end(), m_pNetwork->Liste_parkings.begin(), m_pNetwork->Liste_parkings.end());
    m_LstTripNodes.insert(m_LstTripNodes.end(), m_pNetwork->Liste_zones.begin(), m_pNetwork->Liste_zones.end());

    // Création d'un Trip par origine.
    for(size_t iOrigin = 0; iOrigin < m_pNetwork->Liste_origines.size(); iOrigin++)
    {
        SymuViaTripNode * pOrigin = m_pNetwork->Liste_origines[iOrigin];

        // Initialisation de l'origine
        pOrigin->CopyDemandeInitToDemande(m_pNetwork->m_LstTypesVehicule); 
        pOrigin->CopyCoeffDestInitToCoeffDest(m_pNetwork->m_LstTypesVehicule);
        pOrigin->CopyRepVoieInitToRepVoie(m_pNetwork->m_LstTypesVehicule);
        pOrigin->CopyRepMotifDestInitToRepMotifDest(m_pNetwork->m_LstTypesVehicule);

        // Création 
        Trip * pNewTrip = new Trip();
        pNewTrip->SetOrigin(m_pNetwork->Liste_origines[iOrigin]);
        pNewTrip->SetID(pNewTrip->GetOrigin()->GetID());
        m_LstTrips.push_back(pNewTrip);
    }

    // Appel de la méthode de la classe mère
    AbstractFleet::InitSimuTrafic(docTrafics);
}

void SymuViaFleet::SortieTrafic(DocTrafic *pXMLDocTrafic)
{
    if (m_pNetwork->DoSortieTraj())
    {
        // Sortie des infos sur les entrées
        std::deque<Entree*>::iterator itEntree;
	    for( itEntree = m_pNetwork->Liste_entree.begin(); itEntree != m_pNetwork->Liste_entree.end(); ++itEntree)	
        {
    	    (*itEntree)->SortieTrafic(pXMLDocTrafic);
        }
    }

    // Sortie des infos des stocks
    if(m_pNetwork->IsTraceStocks())
    {
        std::deque<Parking*>::iterator itParking;
        for( itParking = m_pNetwork->Liste_parkings.begin(); itParking != m_pNetwork->Liste_parkings.end(); itParking++)	
        {
		    (*itParking)->SortieTrafic(pXMLDocTrafic);
        }

        std::deque<ZoneDeTerminaison*>::iterator itZone;
        for( itZone = m_pNetwork->Liste_zones.begin(); itZone != m_pNetwork->Liste_zones.end(); itZone++)	
        {
		    (*itZone)->SortieTrafic(pXMLDocTrafic);
        }
    }
}

void SymuViaFleet::FinCalculTrafic(Vehicule * pVeh)
{
    SymuViaFleetParameters * pParams = (SymuViaFleetParameters*)pVeh->GetFleetParameters();

    if(pParams->GetInstantEntreeZone() != -1)
    {
        // incrémentation de la distance parcourue en terminaison de trajet
        pParams->IncDistanceParcourue(pVeh->GetDstParcourueEx());
    }
}

void SymuViaFleet::ActivateVehicle(double dbInstant, VehicleToCreate * pVehicleToCreate)
{
    SymuViaVehicleToCreate * pVehicle = (SymuViaVehicleToCreate*) pVehicleToCreate;
    boost::shared_ptr<Vehicule> pVeh = pVehicle->GetOrigin()->GenerateVehicle(pVehicle->GetVehicleID(), pVehicle->GetType(), pVehicle->GetNumVoie(), dbInstant, pVehicle->GetTimeFraction(),
        pVehicle->GetDestination(), pVehicle->GetItinerary().get(), pVehicle->GetJunction(), pVehicle->GetPlaqueOrigin(), pVehicle->GetPlaqueDestination(), false);
    if (pVeh)
    {
        pVeh->SetExternalID(pVehicle->GetExternalID());
        pVeh->SetNextRouteID(pVehicle->GetNextRouteID());
        m_pNetwork->GetSymuViaFleet()->OnVehicleActivated(pVeh, dbInstant);

		if (m_pNetwork->GetControlZoneManagement() )
			m_pNetwork->GetControlZoneManagement()->CheckAndRerouteVehicle(pVeh);
    }

}


std::vector<boost::shared_ptr<Vehicule> > SymuViaFleet::ActivateVehiclesForTrip(double dbInstant, double dbTimeStep, Trip * pTrip)
{
    std::vector<boost::shared_ptr<Vehicule>> ListVehicule;
    if (!m_pNetwork->IsSymuMasterMode())
    {
        SymuViaTripNode * pOrigin = (SymuViaTripNode*)pTrip->GetOrigin();
        pOrigin->CreationVehicules(dbInstant, ListVehicule);

        if (m_pNetwork->IsCptItineraire())
        {
            size_t count = ListVehicule.size();
            ZoneDeTerminaison * pZone;
            for (size_t i = 0; i < count; i++)
            {
                boost::shared_ptr<Vehicule> pVeh = ListVehicule[i];
                TypeVehicule * pTypVeh = pVeh->GetType();
                pZone = dynamic_cast<ZoneDeTerminaison*>(pOrigin);
                std::string sOrig = pVeh->GetOrigine()->GetOutputID();
                const std::string & sDest = pVeh->GetDestination()->GetInputID();
                m_pNetwork->GetAffectationModule()->IncreaseNBVehEmis(pTypVeh, sOrig, sDest);

                size_t countj = pVeh->GetItineraire()->size();
                std::vector<std::string> listIti;
                for (size_t j = 0; j<countj; j++)
                {
                    listIti.push_back(pVeh->GetItineraire()->at(j)->GetLabel());
                    listIti.push_back(pVeh->GetItineraire()->at(j)->GetCnxAssAv()->GetID());
                }
                if (listIti.size() > 0)
                {
                    listIti.pop_back();
                }
                // détermination de si l'itinéraire est prédéfini ou pas pour le fichier de résultats d'affectation
                bool bPredefini = false;
                if (m_pNetwork->GetAffectationModule()->IsSaving())
                {
                    std::map<TypeVehicule*, std::map< std::pair<Tuyau*, SymuViaTripNode*>, std::deque<AssignmentData> > >::iterator itMapAssignment = ((SymuViaTripNode*)pVeh->GetOrigine())->GetMapAssignment().find(pTypVeh);
                    if (itMapAssignment != ((SymuViaTripNode*)pVeh->GetOrigine())->GetMapAssignment().end())
                    {
                        std::map< std::pair<Tuyau*, SymuViaTripNode*>, std::deque<AssignmentData> >::const_iterator itAD = itMapAssignment->second.find(std::pair<Tuyau*, SymuViaTripNode*>(nullptr, (SymuViaTripNode*)pVeh->GetDestination()));
                        if (itAD != itMapAssignment->second.end())
                        {
                            for (size_t nIti = 0; nIti < itAD->second.size(); nIti++)
                            {
                                if (itAD->second[nIti].bPredefini)
                                {
                                    if (itAD->second[nIti].dqTuyaux == *pVeh->GetItineraire())
                                    {
                                        bPredefini = true;
                                        break;
                                    }
                                }
                            }
                        }
                    }
                }
                m_pNetwork->GetAffectationModule()->AddRealiseItineraire(pTypVeh, sOrig, sDest, listIti, *pVeh->GetItineraire(), bPredefini, pVeh->GetID());
            }
        }
    }

    return ListVehicule;
}

// Gestion de la terminaison d'une étape par un véhicule de la flotte.
void SymuViaFleet::OnCurrentLegFinished(boost::shared_ptr<Vehicule> pVehicle, VoieMicro * pDestinationEnterLane, double dbInstDestinationReached, double dbInstant, double dbTimeStep)
{
    SymuViaFleetParameters * pParams = (SymuViaFleetParameters*)pVehicle->GetFleetParameters();

    // cas de recherche de résidus, si le temps est écoulé, on arrête l'itinéraire calculé avec le premier algo pour passer au second algo.
    if(pParams->IsTerminaison())
    {
        ZoneDeTerminaison * pZoneDest = dynamic_cast<ZoneDeTerminaison*>(pVehicle->GetDestination());
        
        // remarque : en mode parking, si on arrive dans le parking, c'est de toute façon trop tard pour réaffecter quoi que ce soit : il faut détecter le parking plein en amont.
        if (pZoneDest && !pParams->IsTerminaisonResidentielle() && pParams->IsTerminaisonSurfacique())
        {
            pParams->SetIsRechercheStationnement(true, dbInstant);
            
            if (pParams->DoReaffecteVehicule())
            {
                pZoneDest->ReaffecteVehiculeStationnementDestination(pVehicle.get(), dbInstant, true);
                pParams->SetDoReaffecteVehicule(false);
            }
            else
            {
                pZoneDest->ReaffecteVehiculeTroncon(pVehicle.get(), dbInstant);
            }
        }
        else if (pZoneDest && !pParams->IsTerminaisonSurfacique() && pParams->DoReaffecteVehicule())
        {
            // Cas du réarbitrage lié au fait qu'un véhicule arrive sur un parking plein :
            pParams->SetIsRechercheStationnement(true, dbInstant);
            pZoneDest->ReaffecteVehiculeStationnementDestination(pVehicle.get(), dbInstant, true);
            pParams->SetDoReaffecteVehicule(false);
        }
        else
        {
            // Appel de la méthode de la classe mère
            AbstractFleet::OnCurrentLegFinished(pVehicle, pDestinationEnterLane, dbInstDestinationReached, dbInstant, dbTimeStep);
        }
    }
    else
    {
        // Détection de l'arrivée en zone et traitements associés
        ZoneDeTerminaison * pZoneDest = dynamic_cast<ZoneDeTerminaison*>(pVehicle->GetDestination());
        if( pZoneDest )
        {
            // Tirage de la destination finale du véhicule et passage en mode circulation en zone.
            pZoneDest->ReaffecteVehicule(pVehicle.get(), dbInstant);
        }
		else
		{
			// Appel de la méthode de la classe mère
			AbstractFleet::OnCurrentLegFinished(pVehicle,  pDestinationEnterLane, dbInstDestinationReached, dbInstant, dbTimeStep);
		}
    }
}

// Met à jour le Trip en fonction des tuyaux parcourus
void SymuViaFleet::SetLinkUsed(double dbInstant, Vehicule * pVeh, Tuyau * pLink)
{
    // Appel de la méthode de la classe mère
    AbstractFleet::SetLinkUsed(dbInstant, pVeh, pLink);

    SymuViaFleetParameters * pParams = (SymuViaFleetParameters*)pVeh->GetFleetParameters();

    ZoneDeTerminaison* pZoneDest = dynamic_cast<ZoneDeTerminaison*>(pVeh->GetDestination());

    // détection de l'arrivée en zone
    if(pParams->IsTerminaison() && pZoneDest && pParams->GetInstantEntreeZone() == -1 && pZoneDest->GetInputPosition().IsInZone(pVeh->GetLink(0)))
    {
        double dbDistanceParcourue = pVeh->GetPos(0); // approx si un tronçon a été sauté...
        pParams->InitializeZoneTravel(dbInstant - (dbDistanceParcourue / pVeh->GetVit(0)), pZoneDest->GetInputID(), dbDistanceParcourue, pVeh->GetLink(0)->GetCnxAssAm());
    }

    
    // Gestion de la recherche de stationnement surfacique
    if (pParams->IsTerminaisonSurfacique() && pParams->IsRechercheStationnement())
    {
        assert(pZoneDest);
        assert(!pParams->IsTerminaisonResidentielle());

        // Si recherche de stationnement surfacique, on note le tuyau en tant que tuyau essayé
        if (!pLink->GetBriqueParente())
        {
            pParams->GetLstTuyauxDejaParcourus().insert(pLink);
        }

        bool bIsModeSurfaciqueSimplifie = pZoneDest->IsModeSurfaciqueSimplifie(pParams->GetPlaqueDestination(), dbInstant);

        bool bDeleteVehicle = false;
        // Si on a du stock et qu'on est en mode normal,
        // ou si on est en mode simplifié et que la durée de recherche est écoulée, on se gare :
        if ((!bIsModeSurfaciqueSimplifie && (pLink->GetCapaciteStationnement() == -1 || pLink->GetStockStationnement() < pLink->GetCapaciteStationnement()))
            || (bIsModeSurfaciqueSimplifie && pParams->GetDistanceRechercheParcourue() >= pParams->GetGoalDistance()))
        {
            // le véhicule se gare :
            if (!bIsModeSurfaciqueSimplifie)
            {
                pLink->IncStockStationnement(pVeh->GetLength());
            }
            else
            {
                pZoneDest->IncStockStationnement(pVeh);
            }

            bDeleteVehicle = true;
        }
        // si le temps est écoulé, on réarbitre
        else if (pParams->IsRearbitrageNeeded(dbInstant, pZoneDest))
        {
            pVeh->SetEndCurrentLeg(true);
            pParams->SetDoReaffecteVehicule(true);
        }
        // si le temps est écoulé pour le réarbitrage de la distance uniquement en mdo simplifié, on la met à jour :
        else if (bIsModeSurfaciqueSimplifie && pParams->IsGoalDistanceUpdateNeeded(dbInstant, pZoneDest))
        {
            // MAJ de la distance :
            pZoneDest->UpdateGoalDistance(dbInstant, pParams);
        }
        // gestion de la durée max de recherche de stationnement surfacique :
        else if (pParams->IsMaxSurfaciqueDureeReached(dbInstant, pZoneDest))
        {
            bDeleteVehicle = true;

            // rmk : pour éviter l'avertissement pour les véhicules déjà détruits...
            if (pVeh->GetLink(0))
            {
                m_pNetwork->log() << Logger::Warning << "Maximum duration of parking search elapsed without success : vehicle " << pVeh->GetID() << " destroyed." << std::endl;
                m_pNetwork->log() << Logger::Info; // rebascule en mode INFO pour ne pas avoir à reprendre tous les appels aux log en précisant que c'est des INFO. à supprimer si on reprend tous les appels au log.
            }
        }

        if (bDeleteVehicle)
        {
            // Destruction du véhicule
            pVeh->SetTuyau(NULL, dbInstant);
            pVeh->SetVoie(NULL);
            pVeh->SetDejaCalcule(true);
            pVeh->SetInstantSortie(dbInstant);

            // Pour être sûr qu'on ne le réaffecte plus :
            pVeh->SetEndCurrentLeg(false);

            // Gestion de la demande autogérénée à cause du stationnement :
            pZoneDest->GestionDemandeStationnement(pParams->GetPlaqueDestination(), dbInstant);
        }
    }

    // gestion du réarbitrage quand on arrive devant un parking plein :
    if (pParams->IsTerminaison() && !pParams->IsTerminaisonSurfacique())
    {
        Parking * pParkingDest = (Parking*)pVeh->GetTrip()->GetFinalDestination();
        if (pParkingDest->IsFull())
        {
            for (size_t iInputParkLink = 0; iInputParkLink < pParkingDest->GetInputConnexion()->m_LstTuyAssAm.size(); iInputParkLink++)
            {
                if (pParkingDest->GetInputConnexion()->m_LstTuyAssAm.at(iInputParkLink)->GetCnxAssAm() == pLink->GetCnxAssAv())
                {
                    pVeh->SetEndCurrentLeg(true);
                    pParams->SetDoReaffecteVehicule(true);
                    break;
                }
            }
        }
    }
}

void SymuViaFleet::OnVehicleActivated(boost::shared_ptr<Vehicule> pVeh, double dbInstant)
{
    // rmq. dans le cas de cette flotte, les opérations d'ajout au doc trafic, par exemple,
    // sont gérées au cas par cas : on ne fait que l'association entre le véhicule et la flotte ici
    DoVehicleAssociation(pVeh);
}

std::map<std::string, std::string> SymuViaFleet::GetOutputAttributesAtEnd(Vehicule * pV)
{
    std::map<std::string, std::string> additionalAttributes;

    SymuViaFleetParameters * pParams = (SymuViaFleetParameters*)pV->GetFleetParameters();

    if(pParams->GetInstantEntreeZone() != -1)
    {
        additionalAttributes["zone_id"] = pParams->GetZoneId();
        // On prend le temps de sortie du véhicule si renseigné, sinon l'instant courant (cas du véhicule qui reste sur le réseau après la fin de la simu)
        double dbDuree = (pV->GetExitInstant() == -1 ? pV->GetReseau()->m_dbInstSimu : pV->GetExitInstant())-pParams->GetInstantEntreeZone();
        additionalAttributes["zone_tps"] = SystemUtil::ToString(2, dbDuree);
        additionalAttributes["zone_dst"] = SystemUtil::ToString(2, pParams->GetDistanceParcourue());
    }
    
    return additionalAttributes;
}

template void SymuViaFleet::serialize(boost::archive::xml_woarchive & ar, const unsigned int version);
template void SymuViaFleet::serialize(boost::archive::xml_wiarchive & ar, const unsigned int version);

template<class Archive>
void SymuViaFleet::serialize(Archive& ar, const unsigned int version)
{
    ar & BOOST_SERIALIZATION_BASE_OBJECT_NVP(AbstractFleet);
}

