#include "stdafx.h"
#include "NewellContext.h"

#include "../vehicule.h"
#include "../DiagFonda.h"

NewellContext::NewellContext()
    : CarFollowingContext()
{
    m_dbDeltaN = 1;
    m_bContraction = false;
    m_bJorge = false;
    m_dbJorgeStartTime = 0;
    m_dbJorgeEndTime = 0;
}

NewellContext::NewellContext(Reseau * pNetwork, Vehicule * pVehicle, double dbInstant, bool bIsPostProcessing)
    : CarFollowingContext(pNetwork, pVehicle, dbInstant, bIsPostProcessing)
{
    m_dbDeltaN = 1;
    m_bContraction = false;
    m_bJorge = false;
    m_dbJorgeStartTime = 0;
    m_dbJorgeEndTime = 0;
}

NewellContext::~NewellContext()
{
}

void NewellContext::Build(double dbRange, CarFollowingContext * pPreviousContext)
{
    // Appel de la méthode de la classe mère
    CarFollowingContext::Build(dbRange, pPreviousContext);

    // Conservation de l'état de contraction éventuel
    if(pPreviousContext)
    {
        m_bContraction = ((NewellContext*)pPreviousContext)->IsContraction();
    }
    
    // Calcul du DeltaN
    ComputeDeltaN((NewellContext*)pPreviousContext);
}

void NewellContext::SetContext(const std::vector<boost::shared_ptr<Vehicule> > & leaders,
                               const std::vector<double> & leaderDistances,
                               const std::vector<VoieMicro*> & lstLanes,
                               double dbStartPosition,
                               CarFollowingContext * pOriginalContext,
                               bool bRebuild)
{
    // Appel de la méthode de la classe mère
    CarFollowingContext::SetContext(leaders, leaderDistances, lstLanes, dbStartPosition);

    // Initialisation du deltaN
    if(pOriginalContext)
    {
        SetDeltaN(((NewellContext*)pOriginalContext)->GetDeltaN());
        SetContraction(((NewellContext*)pOriginalContext)->IsContraction());
        SetJorge(((NewellContext*)pOriginalContext)->IsJorge());
        SetJorgeStartTime(((NewellContext*)pOriginalContext)->GetJorgeStartTime());
        SetJorgeEndTime(((NewellContext*)pOriginalContext)->GetJorgeEndTime());
    }
    else if(bRebuild)
    {
        ComputeDeltaN(NULL);
    }
}

double NewellContext::GetDeltaN() const
{
    return m_dbDeltaN;
}

void NewellContext::SetDeltaN(double dbDeltaN)
{
    m_dbDeltaN = dbDeltaN;
}

bool NewellContext::IsContraction() const
{
    return m_bContraction;
}

void NewellContext::SetContraction(bool bContraction)
{
    m_bContraction = bContraction;
}

bool NewellContext::IsJorge() const
{
    return m_bJorge;
}

void NewellContext::SetJorge(bool bJorge)
{
    m_bJorge = bJorge;
}

double NewellContext::GetJorgeStartTime() const
{
    return m_dbJorgeStartTime;
}

void NewellContext::SetJorgeStartTime(double dbValue)
{
    m_dbJorgeStartTime = dbValue;
}

double NewellContext::GetJorgeEndTime() const
{
    return m_dbJorgeEndTime;
}

void NewellContext::SetJorgeEndTime(double dbValue)
{
    m_dbJorgeEndTime = dbValue;
}

void NewellContext::ComputeDeltaN(NewellContext * pPreviousContext)
{
    double dbPreviousDeltaN;

    // MAJ du deltaN pour le début du pas de temps courant à partir de la valeur du pas de temps précédent
    if(GetLeaders().empty())
    {
        dbPreviousDeltaN = 1;
    }
    else
    {
        if( m_bContraction )
        {
            assert(pPreviousContext);
            dbPreviousDeltaN = pPreviousContext->GetDeltaN();
        }
        else
        {
            boost::shared_ptr<Vehicule> pVehLeader = GetLeaders().front();

            // on n'utilise pas le deltaN de la fin du pas de temps précédent dans trois cas :
            // 1 - c'est le premier instant du véhicule sur le réseau : on prend 1 pour être conforme à l'ancienne version
            if(!pPreviousContext)
            {
                dbPreviousDeltaN = 1;
            }
            // 2 - le leader a changé 
            // 3 - le leader est le même, mais on était en état fluide au pas de temps précédent, donc c'est comme s'il n'y avait pas de leader au pas de temps précédent
            else if(pPreviousContext
                && ((pPreviousContext->GetLeaders().size() == 0 || pPreviousContext->GetLeaders().front() != pVehLeader) 
                || pPreviousContext->IsFreeFlow()))
            {
                // récupération de l'espacement au début du pas de temps       
                double dbS = GetLeaderDistances().front();

                // OTK - LOI - Question : si le véhicule vient de rentrer sur le réseau, problème de dbS, non ? comment adapter la formule ?
                // Calcul de Kvfin
                double dbKvFin = - pVehLeader->GetDiagFonda()->GetKMax() * pVehLeader->GetDiagFonda()->GetW() / ( pVehLeader->GetVit(1) -  pVehLeader->GetDiagFonda()->GetW());
                dbPreviousDeltaN = std::min<double>(1, dbKvFin * dbS); 
            }
            else
            {
                dbPreviousDeltaN = pPreviousContext->GetDeltaN();
            }
        }
    }
    
    SetDeltaN(dbPreviousDeltaN);
}

void NewellContext::CopyTo(CarFollowingContext * pDestinationContext)
{
    // Appel de la méthode de la classe mère
    CarFollowingContext::CopyTo(pDestinationContext);

    ((NewellContext*)pDestinationContext)->SetDeltaN(m_dbDeltaN);
    ((NewellContext*)pDestinationContext)->SetContraction(m_bContraction);
    ((NewellContext*)pDestinationContext)->SetJorge(m_bJorge);
    ((NewellContext*)pDestinationContext)->SetJorgeStartTime(m_dbJorgeStartTime);
    ((NewellContext*)pDestinationContext)->SetJorgeEndTime(m_dbJorgeEndTime);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Sérialisation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////
template void NewellContext::serialize(boost::archive::xml_woarchive & ar, const unsigned int version);
template void NewellContext::serialize(boost::archive::xml_wiarchive & ar, const unsigned int version);

template<class Archive>
void NewellContext::serialize(Archive& ar, const unsigned int version)
{
    ar & BOOST_SERIALIZATION_BASE_OBJECT_NVP(CarFollowingContext);
    ar & BOOST_SERIALIZATION_NVP(m_dbDeltaN);
    ar & BOOST_SERIALIZATION_NVP(m_bContraction);
    ar & BOOST_SERIALIZATION_NVP(m_bJorge);
    ar & BOOST_SERIALIZATION_NVP(m_dbJorgeStartTime);
    ar & BOOST_SERIALIZATION_NVP(m_dbJorgeEndTime);
}

