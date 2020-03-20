#include "stdafx.h"
#include "Trip.h"

#include "TripLeg.h"
#include "TripNode.h"
#include "vehicule.h"
#include "tuyau.h"
#include "reseau.h"
#include "usage/AbstractFleet.h"

#include <boost/thread/lock_guard.hpp>

#include <boost/serialization/vector.hpp>
#include <boost/serialization/shared_ptr.hpp>

TripSnapshot::TripSnapshot()
{

}

TripSnapshot::~TripSnapshot()
{

}

Trip::Trip()
{
    m_pOrigin = NULL;
    m_pFullPath = NULL;

    m_CurrentLegIndex = -1;
}

// Définition d'un constructeur par recopie obligatoire à cause du wrapper python et de la présence d'un Mutex (qui n'est pas copyable)
Trip::Trip(const Trip & other)
{
    m_strID = other.m_strID;
    m_pOrigin = other.m_pOrigin;
    m_Path = other.m_Path;
    m_CurrentLegIndex = other.m_CurrentLegIndex;
    m_pFullPath = NULL;
}

Trip::~Trip()
{
    for(size_t i = 0 ; i < m_Path.size(); i++)
    {
        delete m_Path[i];
    }

    if(m_pFullPath)
    {
        delete m_pFullPath;
    }

    // rmq : on ne détruit pas m_pOrigin (l'ownership des TripNode revient à la flotte correspondante).
}

void Trip::CopyTo(Trip * pTrip)
{
    // Identifiant du trip
    pTrip->m_strID = m_strID;
    pTrip->m_pOrigin = m_pOrigin;
    pTrip->m_CurrentLegIndex = m_CurrentLegIndex;
    for(size_t i = 0; i < m_Path.size(); i++)
    {
        TripLeg * pTripLeg = new TripLeg();
        m_Path[i]->CopyTo(pTripLeg);
        pTrip->m_Path.push_back(pTripLeg);
    }

    // rmq. : m_AssignedVehicles non copié car le CopyTo n'est utilisé que dans le contexte 
    // des Trips associés aux véhicules, et pas aux flottes
}

void Trip::SetID(const std::string & strID)
{
    m_strID = strID;
}

const std::string & Trip::GetID()
{
    return m_strID;
}

void Trip::SetOrigin(TripNode * pOrigin)
{
    m_pOrigin = pOrigin;
}

TripNode * Trip::GetOrigin()
{
    return m_pOrigin;
}

const std::vector<TripLeg*> & Trip::GetLegs()
{
    return m_Path;
}

TripNode * Trip::GetFinalDestination()
{
    return m_Path.back()->GetDestination();
}

TripNode * Trip::GetCurrentDestination()
{
    TripNode * pDest = NULL;
    TripLeg * pLeg = GetCurrentLeg();
    if(pLeg)
    {
        pDest = pLeg->GetDestination();
    }
    return pDest;
}

// Utile pour la matrice OD par exemple : on souhaite avoir la destination
// initiale du véhicule et pas les tronçons de stationnement par exemple.
TripNode * Trip::GetInitialDestination()
{
    return m_Path.front()->GetDestination();
}

void Trip::SetLinkUsed(Tuyau * pLink)
{
    GetCurrentLeg()->SetLinkUsed(pLink);
}

const std::vector<boost::shared_ptr<Vehicule> > & Trip::GetAssignedVehicles()
{
    return m_AssignedVehicles;
}

void Trip::AddAssignedVehicle(boost::shared_ptr<Vehicule> pVeh)
{
    m_AssignedVehicles.push_back(pVeh);
}
    
void Trip::RemoveAssignedVehicle(boost::shared_ptr<Vehicule> pVeh)
{
    m_AssignedVehicles.erase(std::remove(m_AssignedVehicles.begin(), m_AssignedVehicles.end(), pVeh), m_AssignedVehicles.end());
}

void Trip::ClearAssignedVehicles()
{
    m_AssignedVehicles.clear();
}

std::vector<TripNode*> Trip::GetTripNodes()
{
    std::vector<TripNode*> result;
    if(m_pOrigin)
    {
        result.push_back(m_pOrigin);
    }
    for(size_t i = 0; i < m_Path.size(); i++)
    {
        TripNode * pDest = m_Path[i]->GetDestination();
        if(pDest)
        {
            result.push_back(pDest);
        }
    }
    return result;
}

TripNode * Trip::GetTripNode(const std::string & nodeName) const
{
    if (m_pOrigin && m_pOrigin->GetID() == nodeName)
    {
        return m_pOrigin;
    }
    for (size_t i = 0; i < m_Path.size(); i++)
    {
        TripNode * pDest = m_Path[i]->GetDestination();
        if (pDest && pDest->GetID() == nodeName)
        {
            return pDest;
        }
    }
    return NULL;
}

std::vector<Tuyau*> * Trip::BuildFullPath()
{
    std::vector<Tuyau*> * pFullPath = new std::vector<Tuyau*>();
    for (size_t i = 0; i < m_Path.size(); i++)
    {
        const std::vector<Tuyau*> & path = m_Path[i]->GetPath();
        // Gestion des doublons aux jointures :
        for (size_t j = 0; j < path.size(); j++)
        {
            if (j > 0 || (pFullPath->empty() || path[j] != pFullPath->back()))
            {
                pFullPath->push_back(path[j]);
            }
        }
    }
    return pFullPath;
}

std::vector<Tuyau*> Trip::GetRemainingPath()
{
	std::vector<Tuyau*> result;
	for (int i = std::max<int>(0, m_CurrentLegIndex); i < (int)m_Path.size(); i++)
	{
		std::vector<Tuyau*> legRemainingPath = m_Path[i]->GetRemainingPath();
		// Gestion des doublons aux jointures :
		for (size_t j = 0; j < legRemainingPath.size(); j++)
		{
			if (j > 0 || (result.empty() || legRemainingPath[j] != result.back()))
			{
				result.push_back(legRemainingPath[j]);
			}
		}
	}
	return result;
}

std::vector<Tuyau*> * Trip::GetFullPath()
{
    if(!m_pFullPath)
    {
        m_pFullPath = BuildFullPath();
    }
    return m_pFullPath;
}

std::vector<Tuyau*> Trip::GetFullPathThreadSafe()
{
    boost::lock_guard<boost::mutex> guard(m_Mutex);

    if (!m_pFullPath)
    {
        m_pFullPath = BuildFullPath();
    }
    return *m_pFullPath;
}

TripLeg * Trip::GetCurrentLeg()
{
    TripLeg * pResult = NULL;
    if(m_CurrentLegIndex != -1)
    {
        pResult = m_Path[m_CurrentLegIndex];
    }
    return pResult;
}

int Trip::GetCurrentLegIndex()
{
    return m_CurrentLegIndex;
}

void Trip::AddTripLeg(TripLeg* pTripLeg)
{
    m_Path.push_back(pTripLeg);

    // Invalidation de l'itinéraire complet calculé précédemment
    InvalidateFullPath();
}

void Trip::InsertTripLeg(TripLeg * pTripLeg, size_t position)
{
    m_Path.insert(m_Path.begin() + position, pTripLeg);

    // Si on insère le leg avant le leg courant, l'indice du leg courant est incrémenté de fait.
    // Si on insère juste devant le leg courant (position == m_CurrentLegIndex), on n'incrémente pas
    // car le leg courant est le nouveau leg inséré.
    if((int)position < m_CurrentLegIndex)
    {
        IncCurrentLegIndex();
    }

    // Invalidation de l'itinéraire complet calculé précédemment
    InvalidateFullPath();
}

void Trip::RemoveTripLeg(size_t position)
{
    m_Path.erase(m_Path.begin() + position);

    // Si on supprime avant le leg courant, l'indice du leg courant est décrémenté de fait.
    // Si on supprime le leg courant, (position == m_CurrentLegIndex), on ne décrémente pas
    // car le leg suivant devient le leg courant.
    if((int)position < m_CurrentLegIndex)
    {
        m_CurrentLegIndex--;
    }

    // Invalidation de l'itinéraire complet calculé précédemment
    InvalidateFullPath();
}

void Trip::IncCurrentLegIndex()
{
    m_CurrentLegIndex++;
    assert(m_CurrentLegIndex < (int)m_Path.size());
}

bool Trip::ChangeRemainingPath(const std::vector<Tuyau*> & remainingPath, Vehicule * pVeh)
{
    // on prend les tuyaux déjà parcourus de l'itinéraire actuel
    std::vector<Tuyau*> newPath = m_Path[m_CurrentLegIndex]->GetUsedPath();

    // Si on a un véhicule, on vérifie qu'il n'est pas sur un tronçon interne qui empêcherait d'aller vers le nouvel itinéraire
    Tuyau * pInternalLink = NULL;
    if (pVeh)
    {
        if (!pVeh->GetTuyauxParcourus().empty())
        {
            if (pVeh->GetTuyauxParcourus().back()->GetBriqueParente())
            {
                pInternalLink = pVeh->GetTuyauxParcourus().back();
            }
        }
    }

    Tuyau * pNextTuyau = remainingPath.empty()? NULL : remainingPath.front();
    bool bIsAnticipated = false;

    if(!remainingPath.empty())
    {
        if (!newPath.empty())
        {
            bool bOk = false;
            // recherche du dernier tuyau parcouru dans le nouvel itinéraire pour faire la jointure au bon endroit :
            Tuyau * pLastLink = newPath.back();
            Tuyau * pNextLink = NULL;
            for (std::vector<Tuyau*>::const_iterator iter = remainingPath.begin(); iter != remainingPath.end(); ++iter)
            {
                if (*iter == pLastLink)
                {
                    if (iter + 1 != remainingPath.end())
                    {
                        pNextLink = *(iter + 1);
                        newPath.insert(newPath.end(), iter + 1, remainingPath.end());
                    }
                    bOk = true;
                    break;
                }
            }
            if (!bOk)
            {
                if (pLastLink->GetCnxAssAv() == remainingPath.front()->GetCnxAssAm()
                    && pLastLink->GetCnxAssAv()->IsMouvementAutorise(pLastLink, remainingPath.front(), pVeh ? pVeh->GetType() : NULL, NULL))
                {
                    pNextLink = remainingPath.front();
                    newPath.insert(newPath.end(), remainingPath.begin(), remainingPath.end());
                    bOk = true;
                }
                else
                {
                    // si l'itinéraire actuel permet d'atteindre le début du remainingPath, 
                    // on permet de faire la modification ! (cas des changements d'itiénraires anticipés par le SG, par exemple).
                    const std::vector<Tuyau*> & currentPath = m_Path[m_CurrentLegIndex]->GetPath();
                    for (size_t iLink = newPath.size(); iLink < currentPath.size(); iLink++)
                    {
                        Tuyau * pLink = currentPath.at(iLink);
                        newPath.push_back(pLink);
                        if (pLink->GetCnxAssAv() == remainingPath.front()->GetCnxAssAm()
                            && pLink->GetCnxAssAv()->IsMouvementAutorise(pLink, remainingPath.front(), pVeh?pVeh->GetType():NULL, NULL)) {
                            bIsAnticipated = true;
                            bOk = true;
                            newPath.insert(newPath.end(), remainingPath.begin(), remainingPath.end());
                            break;
                        }
                    }
                }
            }
            if (bOk && pInternalLink && pNextLink && !bIsAnticipated)
            {
                // Vérification du fait que le tronçon interne courant permet bien d'accéder à la nouvelle portion de l'itinéraire
                std::set<Tuyau*> lstTuyauxInt = pInternalLink->GetBriqueParente()->GetAllTuyauxInternes(pLastLink, pNextLink);
                bOk = lstTuyauxInt.find(pInternalLink) != lstTuyauxInt.end();
            }
            if (!bOk)
            {
                return false;
            }
            else
            {
                pNextTuyau = pNextLink;
            }
        }
		else
		{
			newPath.insert(newPath.end(), remainingPath.begin(), remainingPath.end());
		}
    }

    if (pVeh && !bIsAnticipated)
    {
        pVeh->SetNextTuyau(pNextTuyau);
    }

    // Pour robustesse avec appels asynchrones SG
    boost::lock_guard<boost::mutex> guard(m_Mutex);

    // On affecte de nouvel itinéraire au Leg
    m_Path[m_CurrentLegIndex]->SetPath(newPath);

    // Invalidation de l'itinéraire complet calculé précédemment
    InvalidateFullPath();

    return true;
}

void Trip::ChangeCurrentPath(const std::vector<Tuyau*> & newPath, int currentLinkInPath)
{
    m_Path[m_CurrentLegIndex]->SetCurrentLinkInLegIndex(currentLinkInPath);
    m_Path[m_CurrentLegIndex]->SetPath(newPath);

    // Invalidation de l'itinéraire complet calculé précédemment
    InvalidateFullPath();
}

void Trip::EndCurrentPath()
{
    if (m_CurrentLegIndex >= 0)
    {
        TripLeg * pCurrentLeg = m_Path[m_CurrentLegIndex];

        // on prend les tuyaux déjà parcourus de l'itinéraire actuel
        std::vector<Tuyau*> newPath = pCurrentLeg->GetUsedPath();

        if (newPath != pCurrentLeg->GetPath())
        {
            // On affecte de nouvel itinéraire au Leg
            pCurrentLeg->SetPath(newPath);

            // Invalidation de l'itinéraire complet calculé précédemment
            InvalidateFullPath();
        }
    }
}

void Trip::InvalidateFullPath()
{
    if(m_pFullPath)
    {
        delete m_pFullPath;
        m_pFullPath = NULL;
    }
}

bool Trip::ContinueDirectlyAfterDestination()
{
    TripNode * pDestination = GetCurrentLeg()->GetDestination();

    if (!pDestination)
        return true;

    return pDestination->ContinueDirectlyAfterDestination();
}

// Indique si un itinéraire est défini pour le Trip
bool Trip::IsPathDefined()
{
    bool result = false;
    if(!m_Path.empty())
    {
        result = !m_Path.front()->GetPath().empty();
    }
    return result;
}

int Trip::GetPassageNbToCurrentLeg(TripNode * pTripNode)
{
    return GetPassageNb(pTripNode, m_CurrentLegIndex);
}

int Trip::GetPassageNb(TripNode * pTripNode, size_t tripLegIndex)
{
    // On compte le passage à l'origine
    int passageNb = pTripNode == m_pOrigin ? 1 : 0;

    for(size_t i = 0; i < GetLegs().size(); i++)
    {
        // On ignore le leg courant et les suivants puisque par définition leur destination n'a pas déjà été atteinte
        if(i == tripLegIndex)
        {
            break;
        }

        if(GetLegs()[i]->GetDestination() == pTripNode)
        {
            passageNb++;
        }
    }
    return passageNb;
}

TripSnapshot * Trip::TakeSnapshot()
{
    TripSnapshot * pResult = new TripSnapshot();
    for(size_t j = 0; j < GetAssignedVehicles().size(); j++)
    {
        pResult->assignedVehicles.push_back(GetAssignedVehicles()[j]->GetID());
    }
    return pResult;
}

void Trip::Restore(Reseau * pNetwork, TripSnapshot * backup)
{
    ClearAssignedVehicles();
    for (size_t j = 0; j < backup->assignedVehicles.size(); j++)
    {
        AddAssignedVehicle(pNetwork->GetVehiculeFromID(backup->assignedVehicles[j]));
    }
}

template void TripSnapshot::serialize(boost::archive::xml_woarchive & ar, const unsigned int version);
template void TripSnapshot::serialize(boost::archive::xml_wiarchive & ar, const unsigned int version);

template<class Archive>
void TripSnapshot::serialize(Archive& ar, const unsigned int version)
{
    ar & BOOST_SERIALIZATION_NVP(assignedVehicles);
}

template void Trip::serialize(boost::archive::xml_woarchive & ar, const unsigned int version);
template void Trip::serialize(boost::archive::xml_wiarchive & ar, const unsigned int version);

template<class Archive>
void Trip::serialize(Archive & ar, const unsigned int version)
{
    ar & BOOST_SERIALIZATION_NVP(m_strID);
    ar & BOOST_SERIALIZATION_NVP(m_CurrentLegIndex);
    Reseau * pNetwork;
    if (Archive::is_saving::value)
    {
        pNetwork = m_pOrigin->GetNetwork();
    }
    ar & BOOST_SERIALIZATION_NVP(pNetwork);
    // Don't save all attribute if we are in light save mode
    if (pNetwork->m_bLightSavingMode) {
        std::string strOriginID;
        if (Archive::is_saving::value)
        {
            strOriginID = m_pOrigin->GetID();
        }
        ar & BOOST_SERIALIZATION_NVP(strOriginID);
        if (Archive::is_loading::value)
        {
            //We don't know to wich fleet this trip belong
            for (int i = 0; i < pNetwork->m_LstFleets.size(); i++){
                m_pOrigin = pNetwork->m_LstFleets[i]->GetTripNode(strOriginID);
                if (m_pOrigin){
                    break;
                }
            }
            // if it's a SymuViaTripNode
            if (!m_pOrigin){
                char cOriginType;
                m_pOrigin = (TripNode*)pNetwork->GetOrigineFromID(strOriginID, cOriginType);
            }
        }
    }
    else{
        ar & BOOST_SERIALIZATION_NVP(m_pOrigin);
        ar & BOOST_SERIALIZATION_NVP(m_AssignedVehicles);
    }
    ar & BOOST_SERIALIZATION_NVP(m_Path);
}
