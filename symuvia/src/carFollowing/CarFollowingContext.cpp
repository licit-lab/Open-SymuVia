#include "stdafx.h"
#include "CarFollowingContext.h"

#include "../vehicule.h"
#include "../tuyau.h"
#include "../reseau.h"
#include "../voie.h"

#include <boost/serialization/shared_ptr.hpp>
#include <boost/serialization/vector.hpp>

using namespace std;


CarFollowingContext::CarFollowingContext()
{
    m_pNetwork = NULL;
    m_dbInstant = 0;
    m_dbStartPosition = 0;
    m_bIsPostProcessing = false;
}

CarFollowingContext::CarFollowingContext(Reseau * pNetwork, Vehicule * pVehicle, double dbInstant, bool bIsPostProcessing)
{
    m_pNetwork = pNetwork;
    m_dbInstant = dbInstant;
    m_pVehicle = pVehicle;
    m_dbStartPosition = 0;
    m_bIsPostProcessing = bIsPostProcessing;
}


CarFollowingContext::~CarFollowingContext()
{
}


void CarFollowingContext::Build(double dbRange, CarFollowingContext * pPreviousContext)
{
    // Position initiale du véhicule sur la première voie
    m_dbStartPosition = m_pVehicle->GetPos(1);

    // Détermination des voies d'intérêt
    BuildLstLanes(dbRange);

    // Calcul du leader
    SearchLeaders(dbRange);
}

void CarFollowingContext::SetContext(const std::vector<boost::shared_ptr<Vehicule> > & leaders,
                const std::vector<double> & leaderDistances,
                const std::vector<VoieMicro*> & lstLanes,
                double dbStartPosition,
                CarFollowingContext * pOriginalContext,
                bool bRebuild)
{
    m_Leaders = leaders;
    m_LeaderDistances = leaderDistances;
    m_LstLanes = lstLanes;
    m_dbStartPosition = dbStartPosition;
}

const vector<boost::shared_ptr<Vehicule> > & CarFollowingContext::GetLeaders()
{
    return m_Leaders;
}

void CarFollowingContext::SetLeaders(const std::vector<boost::shared_ptr<Vehicule> > & newLeaders)
{
    m_Leaders = newLeaders;
}

const vector<double> & CarFollowingContext::GetLeaderDistances()
{
    return m_LeaderDistances;
}

const std::vector<VoieMicro*> & CarFollowingContext::GetLstLanes()
{
    return m_LstLanes;
}

double CarFollowingContext::GetStartPosition()
{
    return m_dbStartPosition;
}

bool CarFollowingContext::IsFreeFlow() const
{
    return m_bIsFreeFlow;
}

void CarFollowingContext::SetFreeFlow(bool bFreeFlow)
{
    m_bIsFreeFlow = bFreeFlow;
}

Vehicule * CarFollowingContext::GetVehicle()
{
    return m_pVehicle;
}

bool CarFollowingContext::IsPostProcessing() const
{
    return m_bIsPostProcessing;
}

void CarFollowingContext::BuildLstLanes(double dbRange)
{
    // Calcul des voies suivantes en fonction de la distance
    VoieMicro * pLane = m_pVehicle->GetVoie(1);

    assert(pLane); // celà peut-il arriver ?

    // On commence par ajouter la voie courante du véhicule
    m_LstLanes.push_back(pLane);

    double dbDistance = pLane->GetLongueurEx(m_pVehicle->GetType()) - (m_pVehicle->GetPos(1) + m_pVehicle->GetCreationPosition());

    // On ajoute ensuite les voies suivantes si nécessaire (dans le cas de l'insertion d'un bus, pas la peine)
    while((dbDistance < dbRange) && (pLane = (VoieMicro*)m_pVehicle->CalculNextVoie((Voie*)pLane, m_dbInstant)))
    {
        m_LstLanes.push_back(pLane);
        dbDistance += pLane->GetLongueurEx(m_pVehicle->GetType());
    }
}


void CarFollowingContext::SearchLeaders(double dbRange)
{
    double dbDistance = 0;
    for(size_t iLane = 0; iLane < m_LstLanes.size(); iLane++)
    {
        VoieMicro * pLane = m_LstLanes[iLane];
        if(iLane == 0)
        {
            boost::shared_ptr<Vehicule> pLeader = m_pNetwork->GetPrevVehicule(m_pVehicle, false, true);
            // rmq. pour des questions de perfs, on s'arrête au premier leader. A mettre en place le jour où une loi de poursuite utilise plusieurs leaders:
            // remettre le while en virant le break et prévoir un attribut dans l'objet CarFollowing permettant de ne calculer les leaders suivants que pour les lois de poursuite qui s'en servent
            if/*while*/(pLeader)
            {
                double dbLeaderDistance = pLeader->GetPos(1) - (m_pVehicle->GetPos(1) + m_pVehicle->GetCreationPosition());
                if(dbLeaderDistance <= dbRange)                                               
                {
                    m_Leaders.push_back(pLeader);
                    m_LeaderDistances.push_back(dbLeaderDistance);
                }
                break;
                //pLeader = m_pNetwork->GetPrevVehicule(pLeader.get(), false, true);
            }
            dbDistance += pLane->GetLongueurEx(m_pVehicle->GetType()) - (m_pVehicle->GetPos(1) + m_pVehicle->GetCreationPosition());
        }
        else
        {
            boost::shared_ptr<Vehicule> pLeader = m_pNetwork->GetNearAvalVehicule(m_LstLanes[iLane], 0, m_pVehicle->GetVehiculeDepasse().get(), true);
            if/*while*/(pLeader)
            {
                double dbLeaderDistance = pLeader->GetPos(1) + dbDistance;
                if(dbLeaderDistance <= dbRange)
                {
                    m_Leaders.push_back(pLeader);
                    m_LeaderDistances.push_back(dbLeaderDistance);
                }
                break;
                //pLeader = m_pNetwork->GetPrevVehicule(pLeader.get(), false, true);
            }
            dbDistance += pLane->GetLongueurEx(m_pVehicle->GetType());
        }
    }
}

void CarFollowingContext::CopyTo(CarFollowingContext * pDestinationContext)
{
    // rmq. on ignore le nextstop et la liste des voies, qui ne sert pas en tant que contexte précédent.
    pDestinationContext->SetContext(m_Leaders, m_LeaderDistances, std::vector<VoieMicro*>(), m_dbStartPosition, this);
    pDestinationContext->SetFreeFlow(IsFreeFlow());
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Sérialisation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////
template void CarFollowingContext::serialize(boost::archive::xml_woarchive & ar, const unsigned int version);
template void CarFollowingContext::serialize(boost::archive::xml_wiarchive & ar, const unsigned int version);

template<class Archive>
void CarFollowingContext::serialize(Archive & ar, const unsigned int version)
{
    ar & BOOST_SERIALIZATION_NVP(m_pNetwork);
    ar & BOOST_SERIALIZATION_NVP(m_dbInstant);
    ar & BOOST_SERIALIZATION_NVP(m_pVehicle);

    ar & BOOST_SERIALIZATION_NVP(m_Leaders);
    ar & BOOST_SERIALIZATION_NVP(m_LeaderDistances);
    ar & BOOST_SERIALIZATION_NVP(m_LstLanes);
    ar & BOOST_SERIALIZATION_NVP(m_dbStartPosition);
    ar & BOOST_SERIALIZATION_NVP(m_bIsFreeFlow);

    //rmq : on ne sérialize pas les variables qui ne sont utiles que pendant le pas de temps (arrêt, m_dbStopDistance, etc...)
}