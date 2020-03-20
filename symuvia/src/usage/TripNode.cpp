#include "stdafx.h"
#include "TripNode.h"

#include "vehicule.h"
#include "Trip.h"
#include "reseau.h"
#include "tuyau.h"
#include "AbstractFleet.h"

#include <boost/serialization/vector.hpp>
#include <boost/serialization/shared_ptr.hpp>


TripNodeSnapshot::TripNodeSnapshot()
{
}

TripNodeSnapshot::~TripNodeSnapshot()
{
}

TripNode::TripNode()
{
    m_pNetwork = NULL;
    m_bStopNeeded = false;
    m_bListVehiclesInTripNode = false;
}

TripNode::TripNode(const std::string & strID, Reseau * pNetwork)
{
    m_strID = strID;
    m_pNetwork = pNetwork;
    m_bStopNeeded = false;
    m_bListVehiclesInTripNode = false;
}

TripNode::~TripNode()
{
}

void TripNode::SetInputPosition(const Position & pos)
{
    m_InputPosition = pos;
}

void TripNode::SetOutputPosition(const Position & pos)
{
    m_OutputPosition = pos;
}

const Position & TripNode::GetInputPosition() const
{
    return m_InputPosition;
}

const Position & TripNode::GetOutputPosition() const
{
    return m_OutputPosition;
}

void TripNode::SetStopNeeded(bool bStopNeeded)
{
    m_bStopNeeded = bStopNeeded;
}

bool TripNode::ContinueDirectlyAfterDestination()
{
    return !m_bStopNeeded;
}

void TripNode::SetListVehiclesInTripNode(bool bListVehicles)
{
    m_bListVehiclesInTripNode = bListVehicles;
}

bool TripNode::IsReached(VoieMicro * pLane, double laneLength, double startPositionOnLane, double endPositionOnLane, bool bExactPosition)
{
    // dans le cas des TripNode avec décélération, on ajoute une tolérance (la décélération peut faire s'arrêter le véhicule un peu avant
    // la cible ...
    if(!bExactPosition)
    {
        endPositionOnLane += 0.5;
    }

    return m_InputPosition.IsReached(pLane, laneLength, startPositionOnLane, endPositionOnLane);
}

bool TripNode::HasVehicle(boost::shared_ptr<Vehicule> pVeh)
{
    return std::find(m_LstStoppedVehicles.begin(), m_LstStoppedVehicles.end(), pVeh) != m_LstStoppedVehicles.end();
}

// Calcule le temps d'arrêt du véhicule dans le TripNode
double TripNode::GetStopDuration(boost::shared_ptr<Vehicule> pVehicle, bool bIsRealStop)
{
    // On ne sait pas s'artêter moins d'un pas de temps (code de gestion des arrêts à revoir si on veut pouvoir le faire)
    return pVehicle->GetReseau()->GetTimeStep();
}

bool TripNode::ManageStop(boost::shared_ptr<Vehicule> pVeh, double dbInstant, double dbTimeStep)
{
    bool bNoStopDuringAllTimeStep = true;
    if(dbTimeStep <= pVeh->GetTpsArretRestant())   // si le bus reste a l'arret tout le pas de temps
    {
        pVeh->SetVit(0);
        pVeh->SetPos(pVeh->GetPos(1));
        pVeh->SetVoie(pVeh->GetVoie(1));
        pVeh->SetTuyau(pVeh->GetLink(1), dbInstant);
        pVeh->SetTpsArretRestant(pVeh->GetTpsArretRestant() - dbTimeStep);
        pVeh->SetAcc((pVeh->GetVit(0)-pVeh->GetVit(1)) / dbTimeStep);

        if( pVeh->GetTpsArretRestant()<= 0)
            pVeh->SetVoieDesiree(pVeh->GetNumVoieInsertion());

        bNoStopDuringAllTimeStep = false;
    }
    else // le bus redémarre au cours du pas de temps
    {
        pVeh->MoveToNextLeg();

        if(pVeh->GetTpsArretRestant()>0)
        {
            pVeh->SetTpsArretRestant(0);

            if( pVeh->IsOutside())   
                pVeh->SetVoieDesiree(pVeh->GetNumVoieInsertion());  // Force le changement de voie (au début du prochain pas de temps)
            else                        
                pVeh->CalculTraficEx();  // Gere le redemarrage du bus sur le temps restant

            bNoStopDuringAllTimeStep = false;
        }
    }

    return bNoStopDuringAllTimeStep;
}

int TripNode::getNumVoie(Trip * pTrip, TypeVehicule * pTypeVeh)
{
    int numVoie = 0;
    if(hasNumVoie(pTrip))
    {
        //rmq. on suppose que la voie d'entrée et de sortie est la même. à changer si besoin !
        numVoie = GetInputPosition().GetLaneNumber();
    }
    else
    {
        // Si pas d'insertion vers une voie définie par le TripNode, le véhicule apparaît sur la première voie autorisée pour son type
        Tuyau * pTuyau = GetOutputPosition().GetDownstreamLink();
        for(size_t i = 0; i < pTuyau->GetLstLanes().size(); i++)
        {
            if(!pTuyau->IsVoieInterdite(pTypeVeh, (int)i))
            {
                numVoie = (int)i;
                break;
            }
        }
    }
    return numVoie;
}

bool TripNode::hasNumVoie(Trip * pTrip)
{
    //rmq. on suppose que la voie d'entrée et de sortie est la même. à changer si besoin !
    return GetInputPosition().HasLaneNumber();
}

void TripNode::VehiculeEnter(boost::shared_ptr<Vehicule> pVehicle, VoieMicro * pDestinationEnterLane, double dbInstDestinationReached, double dbInstant, double dbTimeStep, bool bForcedOutside)
{
    pVehicle->SetLastTripNode(this);
    // Si c'est le dernier arrêt, on supprime le véhicule
    if(pVehicle->GetTrip()->GetFinalDestination() == this)
    {
        pVehicle->SetTuyau(NULL, dbInstant);
        pVehicle->SetVoie(NULL);
        pVehicle->SetDejaCalcule(true);
        pVehicle->SetInstantSortie(dbInstDestinationReached);
    }
    else
    {
        pVehicle->SetTpsArretRestant(pVehicle->GetFleet()->GetStopDuration(pVehicle, this, true));
        if(ListVehiclesInTripNode())
        {
            m_LstStoppedVehicles.push_back(pVehicle);
        }
		if( !bForcedOutside )
			pVehicle->SetOutside(GetInputPosition().IsOutside());
		else
			pVehicle->SetOutside(false);

        pVehicle->SetNumVoieInsertion(getNumVoie(pVehicle->GetTrip(), pVehicle->GetType()));
    }
}

void TripNode::VehiculeExit(boost::shared_ptr<Vehicule> pVehicle)
{
    if(ListVehiclesInTripNode())
    {
        m_LstStoppedVehicles.erase(std::remove(m_LstStoppedVehicles.begin(), m_LstStoppedVehicles.end(), pVehicle), m_LstStoppedVehicles.end());
    }
}

TripNodeSnapshot * TripNode::TakeSnapshot()
{
    TripNodeSnapshot * pResult = new TripNodeSnapshot();
    for(size_t i = 0; i < m_LstStoppedVehicles.size(); i++)
    {
        pResult->lstStoppedVehicles.push_back(m_LstStoppedVehicles[i]->GetID());
    }
    return pResult;
}

void TripNode::Restore(Reseau * pNetwork, TripNodeSnapshot * backup)
{
    m_LstStoppedVehicles.clear();
    for(size_t i = 0; i < backup->lstStoppedVehicles.size(); i++)
    {
        m_LstStoppedVehicles.push_back(pNetwork->GetVehiculeFromID(backup->lstStoppedVehicles[i]));
    }
}


template void TripNodeSnapshot::serialize(boost::archive::xml_woarchive & ar, const unsigned int version);
template void TripNodeSnapshot::serialize(boost::archive::xml_wiarchive & ar, const unsigned int version);

template<class Archive>
void TripNodeSnapshot::serialize(Archive& ar, const unsigned int version)
{
    ar & BOOST_SERIALIZATION_NVP(lstStoppedVehicles);
}

template void TripNode::serialize(boost::archive::xml_woarchive & ar, const unsigned int version);
template void TripNode::serialize(boost::archive::xml_wiarchive & ar, const unsigned int version);

template<class Archive>
void TripNode::serialize(Archive& ar, const unsigned int version)
{
    ar & BOOST_SERIALIZATION_NVP(m_strID);
    ar & BOOST_SERIALIZATION_NVP(m_pNetwork);
    ar & BOOST_SERIALIZATION_NVP(m_InputPosition);
    ar & BOOST_SERIALIZATION_NVP(m_OutputPosition);
    ar & BOOST_SERIALIZATION_NVP(m_bStopNeeded);
    ar & BOOST_SERIALIZATION_NVP(m_bListVehiclesInTripNode);
    ar & BOOST_SERIALIZATION_NVP(m_LstStoppedVehicles);
}