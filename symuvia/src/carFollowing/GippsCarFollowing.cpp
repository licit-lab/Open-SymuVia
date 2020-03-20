#include "stdafx.h"
#include "GippsCarFollowing.h"

#include "../reseau.h"
#include "../vehicule.h"
#include "../voie.h"
#include "../tuyau.h"
#include "CarFollowingContext.h"

GippsCarFollowing::GippsCarFollowing()
{
}

GippsCarFollowing::~GippsCarFollowing()
{
}

bool GippsCarFollowing::IsPositionComputed()
{
    return false;
}

bool GippsCarFollowing::IsSpeedComputed()
{
    return true;
}

bool GippsCarFollowing::IsAccelerationComputed()
{
    return false;
}

double GippsCarFollowing::GetMaxInfluenceRange()
{
    return 3 * m_pVehicle->GetVitMax() * m_pNetwork->GetTimeStep();
}

void GippsCarFollowing::InternalCompute(double dbTimeStep, double dbInstant, CarFollowingContext * pContext)
{
    bool bFreeFlow = true;
    m_dbComputedTravelSpeed = ComputeFreeFlowSpeed(dbTimeStep, dbInstant, pContext);

    if(!pContext->GetLeaders().empty())
    {
        double dbCongestedSpeed = ComputeCongestedSpeed(dbInstant, dbTimeStep, pContext);
        if(dbCongestedSpeed < m_dbComputedTravelSpeed)
        {
            bFreeFlow = false;
            m_dbComputedTravelSpeed = dbCongestedSpeed;
        }
    }
    pContext->SetFreeFlow(bFreeFlow);
}

#define GIPPS_ALPHA 2.5
#define GIPPS_BETA  0.025
#define GIPPS_GAMMA 0.5
#define GIPPS_TETA 1

double GippsCarFollowing::ComputeFreeFlowSpeed(double dbTimeStep, double dbInstant, CarFollowingContext * pContext)
{
    // rmq. : contrairement à Newell, on ne tient compte que de la vitesse réglementaire correspondant à la position
    // du véhicule au début du pas de temps => approximation pas bien méchante.
    double dbFreeFlowSpeed = 0;

    if(!pContext->GetLstLanes().empty())
    {
        VoieMicro * pFirstLane = pContext->GetLstLanes().front();

        // Récupération de l'accélération maximum et de la vitesse maximum désirée
        double dbMaximumSpeedOnLane = ((Tuyau*)pFirstLane->GetParent())->GetVitRegByTypeVeh(m_pVehicle->GetType(), dbInstant, pContext->GetStartPosition(), pFirstLane->GetNum());
        dbMaximumSpeedOnLane = std::min<double>(m_pVehicle->GetVitMax(), dbMaximumSpeedOnLane);
        double dbMaximumAcceleration = m_pVehicle->GetType()->GetAccMax(m_pVehicle->GetVit(1));

        // Application de la formule
        dbFreeFlowSpeed = m_pVehicle->GetVit(1) + GIPPS_ALPHA * dbMaximumAcceleration * dbTimeStep * (1 - m_pVehicle->GetVit(1)/dbMaximumSpeedOnLane) * pow(GIPPS_BETA + m_pVehicle->GetVit(1)/dbMaximumSpeedOnLane,GIPPS_GAMMA);
    }

    return dbFreeFlowSpeed;
}


double GippsCarFollowing::ComputeCongestedSpeed(double dbInstant, double dbTimeStep, CarFollowingContext * pContext)
{
    // La position congestionnée n'a de sens que si on a un leader.
    assert(pContext->GetLeaders().size() != 0);
    boost::shared_ptr<Vehicule> pLeader = pContext->GetLeaders().front();

    double dbVehicleDistance = pContext->GetLeaderDistances().front();

    // rmq. on n'utilise pas l'écart type de la décélération : à corriger ?
    double dbDecelerationMax = - m_pVehicle->GetType()->CalculDeceleration(m_pNetwork);
    // rmq. par défaut, sans SymuVia, pas de décélération définie = décélération à 0. On remplace par une décélération infinie !
    if(dbDecelerationMax == 0)
    {
        dbDecelerationMax = -DBL_MAX;
    }

    double dbLeaderDecelerationMax = -pLeader->GetType()->CalculDeceleration(m_pNetwork);
    // rmq. par défaut, sans SymuVia, pas de décélération définie = décélération à 0. On remplace par une décélération infinie !
    if(dbLeaderDecelerationMax == 0)
    {
        dbLeaderDecelerationMax = -DBL_MAX;
    }

    double dbFactor = dbDecelerationMax * (dbTimeStep/2.0 + GIPPS_TETA);
    double dbSqrtContent = dbFactor*dbFactor - dbDecelerationMax * (2.0 * (dbVehicleDistance - (pLeader->GetLength() + m_pVehicle->GetType()->GetEspacementArret()))
        - m_pVehicle->GetVit(1)*dbTimeStep - pLeader->GetVit(1)*pLeader->GetVit(1)/ dbLeaderDecelerationMax);
    double dbCongestedSpeed;
    if(dbSqrtContent >= 0)
    {
        dbCongestedSpeed = dbFactor + sqrt(dbSqrtContent);
    }
    else
    {
        // rmq. le modèle GIPPS ne prévoit pas que les véhicules puissent se rentrer dedans. Si ca arrive, pour pas avoir une vitesse indéfinie,
        // on met une vitesse nulle.
        dbCongestedSpeed = 0;
    }

    return dbCongestedSpeed;
}

//================================================================
double GippsCarFollowing::CalculVitesseApprochee
//----------------------------------------------------------------
// Fonction  :  Calcule de la vitesse approchée d'un véhicule à la fin
//              du pas de temps dans le cas de la nouvelle loi de poursuite
// Remarque  :  (3) de 'A microscopic dual-regime model for roundabouts'
// Version du:  07/12/2007
// Historique:  07/12/2007 (C.Bécarie - Tinea )
//              Création
//================================================================
(
    boost::shared_ptr<Vehicule> pVehicle,
    boost::shared_ptr<Vehicule> pVehLeader,
    double dbDistanceBetweenVehicles
)
{    
    // OTK - GIPPS - pas la peine de l'implémenter tant qu'on ne fait pas de lois de poursutie mixtes (plusieurs lois sur un même réseau) : ne sert que pour Newell.
    return 0;
}


double GippsCarFollowing::ComputeLaneSpeed(VoieMicro * pTargetLane)
{
    return 0.0;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Sérialisation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////
template void GippsCarFollowing::serialize(boost::archive::xml_woarchive & ar, const unsigned int version);
template void GippsCarFollowing::serialize(boost::archive::xml_wiarchive & ar, const unsigned int version);

template<class Archive>
void GippsCarFollowing::serialize(Archive& ar, const unsigned int version)
{
    ar & BOOST_SERIALIZATION_BASE_OBJECT_NVP(AbstractCarFollowing);
}
