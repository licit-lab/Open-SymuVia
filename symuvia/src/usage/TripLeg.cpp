#include "stdafx.h"
#include "TripLeg.h"

#include "tuyau.h"
#include "voie.h"
#include "TripNode.h"
#include "reseau.h"
#include "usage/AbstractFleet.h"

#include <boost/serialization/vector.hpp>

TripLeg::TripLeg()
{
    m_pDestination = NULL;

    m_CurrentLinkInLegIndex = -1;
}

TripLeg::~TripLeg()
{
}

void TripLeg::CopyTo(TripLeg * pTripLeg)
{
    pTripLeg->m_pDestination = m_pDestination;
    pTripLeg->m_Path = m_Path;
    pTripLeg->m_CurrentLinkInLegIndex = m_CurrentLinkInLegIndex;
}

void TripLeg::SetDestination(TripNode * pDestination)
{
    m_pDestination = pDestination;
}

TripNode * TripLeg::GetDestination()
{
    return m_pDestination;
}

void TripLeg::SetPath(const std::vector<Tuyau*> & path)
{
    m_Path = path;
}

const std::vector<Tuyau*> & TripLeg::GetPath()
{
    return m_Path;
}

void TripLeg::SetCurrentLinkInLegIndex(int currentLinkInLegIndex)
{
    m_CurrentLinkInLegIndex = currentLinkInLegIndex;
}

void TripLeg::SetLinkUsed(Tuyau * pLink)
{
    // Le path d'un TripLeg ne comporte pas les tuyaux internes
    if(!pLink->GetBriqueParente())
    {
        for (size_t i = std::max<int>(0, m_CurrentLinkInLegIndex); i < m_Path.size(); i++)
        {
            if(m_Path[i] == pLink)
            {
                m_CurrentLinkInLegIndex = (int)i;
                break;
            }
        }
    }
}

std::vector<Tuyau*> TripLeg::GetRemainingPath()
{
    std::vector<Tuyau*> result;
	for (size_t i = std::max<int>(0, m_CurrentLinkInLegIndex); i < m_Path.size(); i++)
    {
        result.push_back(m_Path[i]);
    }
    return result;
}

std::vector<Tuyau*> TripLeg::GetUsedPath()
{
    std::vector<Tuyau*> result;
    for(int i = 0; i <= m_CurrentLinkInLegIndex; i++)
    {
        result.push_back(m_Path[i]);
    }
    return result;
}

bool TripLeg::IsDone(double dbInstant, VoieMicro * pLane, double laneLength, double startPositionOnLane, double endPositionOnLane, bool bExactPosition)
{
    if (!m_pDestination) {
        return false;
    }
    bool bDestinationReached = m_pDestination->IsReached(pLane, laneLength, startPositionOnLane, endPositionOnLane, bExactPosition);
    if (bDestinationReached)
    {
        // Ajout d'un test pour vérifier qu'on est arrivé au bout de l'itinéraire prévu basé sur l'indice du tronçon courant, afin d'éviter de détecter à tort la fin d'un TripLeg
        // quand on arrive dans la zone de destination par un point de jonction qui n'est pas celui à utiliser.
        if (m_Path.size() >= 1)
        {
            Tuyau * pLink = (Tuyau*)pLane->GetParent();
            bDestinationReached = false;
            if (m_Path.back() == pLink)
            {
                bDestinationReached = true;
            }
            // OTK - TODO - vérifier que c'est utile, ça... a priori pas gênant de toute façon mais bon.
            //if (!bDestinationReached && m_Path.size() >= 2 && m_Path.at(m_Path.size() - 2) == pLink)
            //{
            //    bDestinationReached = true;
            //}
        }
    }
    return bDestinationReached;
}

template void TripLeg::serialize(boost::archive::xml_woarchive & ar, const unsigned int version);
template void TripLeg::serialize(boost::archive::xml_wiarchive & ar, const unsigned int version);

template<class Archive>
void TripLeg::serialize(Archive & ar, const unsigned int version)
{
    Reseau * pNetwork;
    if (Archive::is_saving::value)
    {
        pNetwork = m_pDestination->GetNetwork();
    }
    ar & BOOST_SERIALIZATION_NVP(pNetwork);
    // Don't save all attribute if we are in light save mode
    if (pNetwork->m_bLightSavingMode) {
        std::string strDestinationID;
        std::vector<std::string> listPath;
        if (Archive::is_saving::value)
        {
            strDestinationID = m_pDestination->GetID();

            for (int i = 0; i < m_Path.size(); i++) {
                listPath.push_back(m_Path[i]->GetLabel());
            }

        }
        ar & BOOST_SERIALIZATION_NVP(strDestinationID);
        ar & BOOST_SERIALIZATION_NVP(listPath);
        if (Archive::is_loading::value)
        {
            //We don't know to wich fleet this trip belong
            for (int i = 0; i < pNetwork->m_LstFleets.size(); i++){
                m_pDestination = pNetwork->m_LstFleets[i]->GetTripNode(strDestinationID);
                if (m_pDestination){
                    break;
                }
            }
            // if it's a SymuViaTripNode
            if (!m_pDestination){
                char cDestinationType;
                m_pDestination = (TripNode*)pNetwork->GetDestinationFromID(strDestinationID, cDestinationType);
            }

            for (int i = 0; i < listPath.size(); i++){
                if (listPath[i] == ""){
                    m_Path.push_back(NULL);
                }
                else{
                    m_Path.push_back(pNetwork->m_mapTuyaux[listPath[i]]);
                }
            }

        }
    }
    else{
        ar & BOOST_SERIALIZATION_NVP(m_pDestination);
        ar & BOOST_SERIALIZATION_NVP(m_Path);
    }
    ar & BOOST_SERIALIZATION_NVP(m_CurrentLinkInLegIndex);
}
