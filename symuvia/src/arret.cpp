#include "stdafx.h"
#include "arret.h"

#include "tuyau.h"
#include "usage/Trip.h"
#include "usage/Schedule.h"
#include "usage/AbstractFleet.h"
#include "usage/PublicTransportFleetParameters.h"
#include "usage/PublicTransportScheduleParameters.h"
#include "usage/PublicTransportLine.h"
#include "Logger.h"
#include "TraceDocTrafic.h"
#include "SystemUtil.h"
#include "RepartitionTypeVehicule.h"
#include "RandManager.h"

#ifdef USE_SYMUCORE
#include <Users/IUserHandler.h>
#endif // USE_SYMUCORE


#include <boost/serialization/deque.hpp>
#include <boost/serialization/vector.hpp>
#include <boost/serialization/map.hpp>
#include <boost/serialization/shared_ptr.hpp>

#pragma warning(disable : 4996)

//---------------------------------------------------------------------------
// Constructeur et destructeur
//--------------------------------------------------------------------------

// constructeur
Arret::Arret() : TripNode()
{
    m_bStopNeeded = true;
    m_bListVehiclesInTripNode = true;
    m_bEligibleParcRelais = true;
}
// constructeur
Arret::Arret(const std::string & nom, Reseau * pNetwork, Tuyau* T, double distance, double tps, bool bSurVoie)
    : TripNode(nom, pNetwork)
{
    Position stopPosition(T, distance);
    stopPosition.SetOutside(!bSurVoie);

    SetInputPosition(stopPosition);
    SetOutputPosition(stopPosition);

    m_bStopNeeded = true;
    m_bListVehiclesInTripNode = true;

    tps_arret=tps;

    m_bEligibleParcRelais = true;
}

// destructeur
Arret::~Arret()
{
}


void Arret::Initialize(std::deque< TimeVariation<TraceDocTrafic> > & docTrafics)
{
    // ajout d'un arrêt par ligne 
    for(size_t i = 0; i < getLstLTPs().size(); i++)
    {
        int numVoie = getNumVoie(getLstLTPs()[i], NULL);
                
        std::deque< TimeVariation<TraceDocTrafic> >::iterator itXmlDocTrafic;
        for( itXmlDocTrafic = docTrafics.begin(); itXmlDocTrafic!= docTrafics.end(); ++itXmlDocTrafic )
        {
            itXmlDocTrafic->m_pData->AddArret(GetID(), numVoie, const_cast<char*>(getLstLTPs()[i]->GetID().c_str()));
        }
    }
}

// Traitement sur entrée d'un véhicule dans le TripNode
void Arret::VehiculeEnter(boost::shared_ptr<Vehicule> pVehicle, VoieMicro * pDestinationEnterLane, double dbInstDestinationReached, double dbInstant, double dbTimeStep, bool bForcedOutside)
{
    TripNode::VehiculeEnter(pVehicle, pDestinationEnterLane, dbInstDestinationReached, dbInstant, dbTimeStep, bForcedOutside);

    // on ne remplit pas les map en mode non SymuMaster (pas utilisé et méchanisme de nettoyage de la map intégrée aux fonctions de calcul qui l'utilisent)
    if (m_pNetwork->IsSymuMasterMode())
    {
        // gestion du dernier arrêt de la ligne (le véhicule disparait et on ne passe pas dans GetStopDuration...)
        Trip * pLigne = getLine(pVehicle->GetTrip());
        if (this == pVehicle->GetTrip()->GetFinalDestination())
        {
            LoadUsageParameters & loadParams = pVehicle->GetFleetParameters()->GetUsageParameters().GetLoadUsageParameters();
            GestionDescente(loadParams);
        }

        // gestion de l'historisation des temps de passage
        std::map<int, std::pair<double, double> > & linePassingTimes = m_LastPassingTimes[pLigne];
        std::pair<double, double> & busPassingTimes = linePassingTimes[pVehicle->GetID()];
        busPassingTimes.first = dbInstant;
        busPassingTimes.second = -1;
    }
}

// Traitement sur sortie d'un véhicule du TripNode
void Arret::VehiculeExit(boost::shared_ptr<Vehicule> pVehicle)
{
    TripNode::VehiculeExit(pVehicle);

    // on ne remplit pas les map en mode non SymuMaster (pas utilisé et méchanisme de nettoyage de la map intégrée aux fonctions de calcul qui l'utilisent)
    if (m_pNetwork->IsSymuMasterMode())
    {
        Trip * pLigne = getLine(pVehicle->GetTrip());
        if (pVehicle->GetOrigine() == this)
        {
            // gestion de la montée au premier arrêt (on ne passe pas dans GetStopDuration dans ce cas ...)
            LoadUsageParameters & loadParams = pVehicle->GetFleetParameters()->GetUsageParameters().GetLoadUsageParameters();
            std::vector<Passenger> & waitingPassengers = getRealStockRestant(pLigne);
            int nbPietonsMontant = std::min<int>((int)waitingPassengers.size(), loadParams.GetMaxLoad());
            GetionMontee(pLigne, pVehicle, loadParams, nbPietonsMontant);
        }
        
        // gestion de l'historisation des temps de passage
        std::map<int, std::pair<double, double> > & linePassingTimes = m_LastPassingTimes[pLigne];
        std::map<int, std::pair<double, double> >::iterator iter = linePassingTimes.find(pVehicle->GetID());
        if (iter != linePassingTimes.end())
        {
            iter->second.second = m_pNetwork->m_dbInstSimu;
        }
        else
        {
            //rmq. : un bus peut ne pas être présent pour l'instant dans la map s'il a été créé à l'arrêt :
            assert(pVehicle->GetOrigine() == this);

            std::pair<double, double> & busPassingTimes = linePassingTimes[pVehicle->GetID()];
            busPassingTimes.first = m_pNetwork->m_dbInstSimu;
            busPassingTimes.second = m_pNetwork->m_dbInstSimu;
        }
    }
}

void Arret::GetionMontee(Trip * pLigne, boost::shared_ptr<Vehicule> pVehicle, LoadUsageParameters & loadParams, int nbPietonsMontant)
{
    std::vector<Passenger> & waitingPassengers = getRealStockRestant(pLigne);
    for (int iMontant = 0; iMontant < nbPietonsMontant; iMontant++)
    {
        const Passenger & passenger = waitingPassengers.front();
        loadParams.GetPassengers(passenger.GetDestination()).push_back(passenger);
        waitingPassengers.erase(waitingPassengers.begin());
    }

    int nbAttente = nbPietonsMontant + (int)waitingPassengers.size();
    double dbLastRatioMontee = nbAttente > 0 ? (double)nbPietonsMontant / nbAttente : 1.0;
    setLastRatioMontee(pLigne, dbLastRatioMontee);
}

void Arret::GestionDescente(LoadUsageParameters & loadParams)
{
    std::vector<Passenger> & passengers = loadParams.GetPassengers(this);
    for (size_t iPassenger = 0; iPassenger < passengers.size(); iPassenger++)
    {
        const Passenger & passenger = passengers[iPassenger];
#ifdef USE_SYMUCORE
        m_pNetwork->GetSymuMasterUserHandler()->OnPPathCompleted(passenger.GetExternalID(), loadParams.GetTempsDescenteIndividuel() * (iPassenger + 1));
#endif // USE_SYMUCORE
    }
    passengers.clear();
}

//---------------------------------------------------------------------------
// Fonctions permettant la gestion des lignes d'autobus
//--------------------------------------------------------------------------

// ajoute la ligne à la liste des lignes conernées par l'arret
void Arret::AjoutLigne(Trip* ligne, bool bDureeArretDynamique, bool bPoisson, double dbRatioDescente, bool bBinomial, int nStockInit, RepartitionTypeVehicule * pRepTypesVeh, int num_voie)
{
    Tuyau * pLink = m_InputPosition.GetLink();

    m_LstLTPs.push_back(ligne);
    m_LstDureeArretDynamique.push_back(bDureeArretDynamique);
    m_LstPoissonCreation.push_back(bPoisson);
    m_LstRatioDescente.push_back(dbRatioDescente);
    m_LstBinomialDescente.push_back(bBinomial);
    m_LstStockRestant.push_back(nStockInit);
    m_LstDernierRamassage.push_back(0);
    m_LstWaitingUsers.push_back(std::vector<Passenger>());
    m_LstLastRatioMontee.push_back(1.0);

    // Construction du numéro de la voie de l'arrêt pour chacune des lignes. On prend par défaut la voie
    // la plus à droite qui n'est pas interdite.

    if(m_InputPosition.IsOutside())
    {
        m_LstVoies.push_back(0);
        return;
    }

    if(num_voie != -1)
    {
        m_LstVoies.push_back(num_voie);
    }
    else
    {
        for(int iVoie = 0; iVoie < pLink->getNbVoiesDis(); iVoie++)
        {
            bool bAllVehTypesAuthorized = true;
            for(size_t iVehType = 0; iVehType < m_pNetwork->m_LstTypesVehicule.size() && bAllVehTypesAuthorized; iVehType++)
            {
                if (pRepTypesVeh->HasVehicleType(m_pNetwork, (int)iVehType, 0.0, m_pNetwork->GetDureeSimu()) && pLink->IsVoieInterdite(m_pNetwork->m_LstTypesVehicule[iVehType], iVoie, 0))
                {
                    bAllVehTypesAuthorized = false;
                }
            }
            if(bAllVehTypesAuthorized)
            {
                m_LstVoies.push_back(iVoie);
                break;
            }
        }

        // Si aucune voie possible : on laisse la voie de droite
        if(m_LstVoies.size() != m_LstLTPs.size())
        {
            m_LstVoies.push_back(0);
        }
    }
}

//---------------------------------------------------------------------------
// Fonctions de renvoi de données de l'arret
//--------------------------------------------------------------------------

double Arret::getTempsArret()
{
    return tps_arret;
}

Trip * Arret::getLine(Trip * pTrip)
{
    Trip * pLigne = NULL;
    for(size_t i = 0; i < m_LstLTPs.size() && !pLigne; i++)
    {
        if(!m_LstLTPs[i]->GetID().compare(pTrip->GetID()))
        {
            pLigne = m_LstLTPs[i];
        }
    }
    return pLigne;
}

// retourne le numéro de la voie sur lequel se trouve l'arrêt pour la ligne considérée
int Arret::getNumVoie(Trip * pLigne, TypeVehicule * pTypeVeh)
{
    int numVoie = 0;
    for(size_t i = 0; i < m_LstLTPs.size(); i++)
    {
        if(!m_LstLTPs[i]->GetID().compare(pLigne->GetID()))
        {
            numVoie = m_LstVoies[i];
            break;
        }
    }
    return numVoie;
}

// Indique si le TripNode correspond à un numéro de voie en particulier ou non
bool Arret::hasNumVoie(Trip * pLigne)
{
    return true;
}

void Arret::setInstantDernierRamassage(Trip * pLigne, double dbInstant)
{
    for(size_t i = 0; i < m_LstLTPs.size(); i++)
    {
        if(!m_LstLTPs[i]->GetID().compare(pLigne->GetID()))
        {
            m_LstDernierRamassage[i] = dbInstant;
            break;
        }
    }
}

double Arret::getInstantDernierRamassage(Trip * pLigne)
{
    for(size_t i = 0; i < m_LstLTPs.size(); i++)
    {
        if(!m_LstLTPs[i]->GetID().compare(pLigne->GetID()))
        {
            return m_LstDernierRamassage[i];
        }
    }

    assert(false);
    return -1;
}

int Arret::getStockRestant(Trip * pLigne)
{
    for(size_t i = 0; i < m_LstLTPs.size(); i++)
    {
        if(!m_LstLTPs[i]->GetID().compare(pLigne->GetID()))
        {
            return m_LstStockRestant[i];
        }
    }

    assert(false);
    return 0;
}

void Arret::setStockRestant(Trip * pLigne, int nStock)
{
    for(size_t i = 0; i < m_LstLTPs.size(); i++)
    {
        if(!m_LstLTPs[i]->GetID().compare(pLigne->GetID()))
        {
            m_LstStockRestant[i] = nStock;
            break;
        }
    }
}

std::vector<Passenger> & Arret::getRealStockRestant(Trip * pLigne)
{
    for (size_t i = 0; i < m_LstLTPs.size(); i++)
    {
        if (!m_LstLTPs[i]->GetID().compare(pLigne->GetID()))
        {
            return m_LstWaitingUsers[i];
        }
    }
    assert(false);
    static std::vector<Passenger> empty;
    return empty;
}

double Arret::getLastRatioMontee(Trip * pLigne)
{
    for (size_t i = 0; i < m_LstLTPs.size(); i++)
    {
        if (!m_LstLTPs[i]->GetID().compare(pLigne->GetID()))
        {
            return m_LstLastRatioMontee[i];
        }
    }

    assert(false);
    return 1.0;
}

void Arret::setLastRatioMontee(Trip * pLigne, double dbLastRatio)
{
    for (size_t i = 0; i < m_LstLTPs.size(); i++)
    {
        if (!m_LstLTPs[i]->GetID().compare(pLigne->GetID()))
        {
            m_LstLastRatioMontee[i] = dbLastRatio;
            break;
        }
    }
}

double Arret::getRatioDescente(Trip * pLigne)
{
    for(size_t i = 0; i < m_LstLTPs.size(); i++)
    {
        if(!m_LstLTPs[i]->GetID().compare(pLigne->GetID()))
        {
            return m_LstRatioDescente[i];
        }
    }

    assert(false);
    return 0;
}

bool Arret::isBinomial(Trip * pLigne)
{
    for(size_t i = 0; i < m_LstLTPs.size(); i++)
    {
        if(!m_LstLTPs[i]->GetID().compare(pLigne->GetID()))
        {
            return m_LstBinomialDescente[i];
        }
    }

    assert(false);
    return false;
}

bool Arret::isPoisson(Trip * pLigne)
{
    for(size_t i = 0; i < m_LstLTPs.size(); i++)
    {
        if(!m_LstLTPs[i]->GetID().compare(pLigne->GetID()))
        {
            return m_LstPoissonCreation[i];
        }
    }

    assert(false);
    return false;
}

bool Arret::isDureeArretDynamique(Trip * pLigne)
{
    for(size_t i = 0; i < m_LstLTPs.size(); i++)
    {
        if(!m_LstLTPs[i]->GetID().compare(pLigne->GetID()))
        {
            return m_LstDureeArretDynamique[i];
        }
    }

    assert(false);
    return false;
}

bool Arret::hasLine(PublicTransportLine * pLine)
{
    for (size_t i = 0; i < m_LstLTPs.size(); i++)
    {
        if (m_LstLTPs[i] == pLine)
        {
            return true;
        }
    }
    return false;
}

double Arret::GetStopDuration(boost::shared_ptr<Vehicule> pVehicle, bool bIsRealStop)
{
    Trip * pLigne = getLine(pVehicle->GetTrip());
    if(!isDureeArretDynamique(pLigne) && !m_pNetwork->IsSymuMasterMode())
    {
        double dbResult = getTempsArret();
        
        // Si on a une durée d'arrêt définie pour cet arrêt, on l'utilise à la place
        PublicTransportScheduleParameters * pParams = dynamic_cast<PublicTransportScheduleParameters*>(pVehicle->GetFleetParameters()->GetScheduleParameters());
        if(pParams)
        {
            std::map<Arret*, double>::const_iterator iter = pParams->GetStopTimes().find(this);
            if(iter != pParams->GetStopTimes().end())
            {
                dbResult = iter->second;
            }
        }

        return dbResult;
    }
    else
    {
        // Cas du calcul dynamique de la durée de l'arrêt
        LoadUsageParameters & loadParams = pVehicle->GetFleetParameters()->GetUsageParameters().GetLoadUsageParameters();
        int nbPietonsRestant;
        int nbPassagersDescendant;
        if (m_pNetwork->IsSymuMasterMode())
        {
            nbPietonsRestant = (int)getRealStockRestant(pLigne).size();
            nbPassagersDescendant = (int)loadParams.GetPassengers(this).size();
        }
        else
        {
            nbPietonsRestant = getStockRestant(pLigne);
            double dbDureeAccumulation = m_pNetwork->m_dbInstSimu - getInstantDernierRamassage(pLigne);
            double dbAccumulation = 0;
            tracked_double * pStock = GetVariation(getInstantDernierRamassage(pLigne), m_pNetwork->m_dbInstSimu, GetLstDemandeTV(), m_pNetwork->GetLag());
            if (pStock != NULL)
            {
                dbAccumulation += *pStock;
                delete pStock;
            }

            if (!isPoisson(pLigne))
            {
                // Mode déterministe. On arrondi pour avoir un nombre de passagers entiers
                nbPietonsRestant += (int)(ceil(dbAccumulation));
            }
            else
            {
                // Utilisation d'une distribution de poisson
                nbPietonsRestant += m_pNetwork->GetRandManager()->myPoissonRand(dbAccumulation);
            }

            if (!isBinomial(pLigne))
            {
                // Mode déterministe. On arrondi pour avoir un nombre de passagers entiers
                nbPassagersDescendant = (int)ceil(getRatioDescente(pLigne) * loadParams.GetCurrentLoad());
            }
            else
            {
                // Utilisation d'une loi binomiale
                nbPassagersDescendant = m_pNetwork->GetRandManager()->myBinomialRand(getRatioDescente(pLigne), loadParams.GetCurrentLoad());
            }
        }

        int remainingRoom = loadParams.GetMaxLoad() - loadParams.GetCurrentLoad() + nbPassagersDescendant;
        int nbPietonsMontant = nbPietonsRestant;
        if(nbPietonsMontant > remainingRoom)
        {
            // On ne peut pas charger plus de passagers que ne le permet la place restante
            nbPietonsMontant = remainingRoom;
            nbPietonsRestant -= nbPietonsMontant;
        }
        else
        {
            // l'arrêt est vide de piétons puisqu'ils ont tous pu monter
            nbPietonsRestant = 0;
        }
        
        // Calcul du temps d'arrêt
        double dbDureeArret = loadParams.ComputeLoadTime(nbPietonsMontant, nbPassagersDescendant);

        if(bIsRealStop)
        {
            if (m_pNetwork->IsSymuMasterMode())
            {
                // Gestion de la descente
                GestionDescente(loadParams);

                // gestion de la montée
                GetionMontee(pLigne, pVehicle, loadParams, nbPietonsMontant);
            }
            else
            {
                // Application des modifications liées à un arrêt concrétisé
                PublicTransportFleetParameters* pParams = (PublicTransportFleetParameters*)pVehicle->GetFleetParameters();
                pParams->SetLastNbMontees(nbPietonsMontant);
                pParams->SetLastNbDescentes(nbPassagersDescendant);
                setInstantDernierRamassage(pVehicle->GetTrip(), m_pNetwork->m_dbInstSimu);
                loadParams.SetCurrentLoad(loadParams.GetCurrentLoad() + nbPietonsMontant - nbPassagersDescendant);
                setStockRestant(pLigne, nbPietonsRestant);
            }
        }

        // On s'arrête au moins un pas de temps
        return std::max<double>(dbDureeArret, m_pNetwork->GetTimeStep());
    }
}

// Renvoie une map d'attributs à sortir de façon spécifique pour un véhicule dans le TripNode
std::map<std::string, std::string> Arret::GetOutputAttributes(Vehicule * pV)
{
    std::map<std::string, std::string> result;
    result["arret"] = GetID();
    if(isDureeArretDynamique(pV->GetTrip()))
    {
        PublicTransportFleetParameters * pParams = (PublicTransportFleetParameters*)pV->GetFleetParameters();
        result["nb_montees"] = SystemUtil::ToString(pParams->GetLastNbMontees());
        result["nb_descentes"] = SystemUtil::ToString(pParams->GetLastNbDescentes());
    }
    return result;
}

TripNodeSnapshot * Arret::TakeSnapshot()
{
    ArretSnapshot * pResult = new ArretSnapshot();
    for(size_t i = 0; i < m_LstStoppedVehicles.size(); i++)
    {
        pResult->lstStoppedVehicles.push_back(m_LstStoppedVehicles[i]->GetID());
    }
    // par rapport à la classe mère, on sauvegarde aussi les derniers instants de passages
    pResult->lstDernierRamassage = m_LstDernierRamassage;
    pResult->lstStockRestant = m_LstStockRestant;
    pResult->lstWaitingUsers = m_LstWaitingUsers;
    pResult->lastPassingTimes = m_LastPassingTimes;
    pResult->lastRatioMontee = m_LstLastRatioMontee;
    return pResult;
}

void Arret::Restore(Reseau * pNetwork, TripNodeSnapshot * backup)
{
    ArretSnapshot * pArretSnapshot = (ArretSnapshot*)backup;
    m_LstStoppedVehicles.clear();
    for(size_t i = 0; i < backup->lstStoppedVehicles.size(); i++)
    {
        m_LstStoppedVehicles.push_back(pNetwork->GetVehiculeFromID(backup->lstStoppedVehicles[i]));
    }
    // par rapport à la classe mère, on restore aussi les derniers instants de passages
    m_LstDernierRamassage = pArretSnapshot->lstDernierRamassage;
    m_LstStockRestant = pArretSnapshot->lstStockRestant;
    m_LstWaitingUsers = pArretSnapshot->lstWaitingUsers;
    m_LastPassingTimes = pArretSnapshot->lastPassingTimes;
    m_LstLastRatioMontee = pArretSnapshot->lastRatioMontee;
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Sérialisation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////
template void ArretSnapshot::serialize(boost::archive::xml_woarchive & ar, const unsigned int version);
template void ArretSnapshot::serialize(boost::archive::xml_wiarchive & ar, const unsigned int version);

template<class Archive>
void ArretSnapshot::serialize(Archive & ar, const unsigned int version)
{
    ar & BOOST_SERIALIZATION_BASE_OBJECT_NVP(TripNodeSnapshot);

    ar & BOOST_SERIALIZATION_NVP(lstDernierRamassage);
    ar & BOOST_SERIALIZATION_NVP(lstStockRestant);
    ar & BOOST_SERIALIZATION_NVP(lstWaitingUsers);
    ar & BOOST_SERIALIZATION_NVP(lastPassingTimes);
    ar & BOOST_SERIALIZATION_NVP(lastRatioMontee);
}

template void Arret::serialize(boost::archive::xml_woarchive & ar, const unsigned int version);
template void Arret::serialize(boost::archive::xml_wiarchive & ar, const unsigned int version);

template<class Archive>
void Arret::serialize(Archive & ar, const unsigned int version)
{
    ar & BOOST_SERIALIZATION_BASE_OBJECT_NVP(TripNode);

    ar & BOOST_SERIALIZATION_NVP(tps_arret);
    ar & BOOST_SERIALIZATION_NVP(m_bEligibleParcRelais);
    ar & BOOST_SERIALIZATION_NVP(m_LstLTPs);
    ar & BOOST_SERIALIZATION_NVP(m_LstVoies);
    ar & BOOST_SERIALIZATION_NVP(m_LstPoissonCreation);
    ar & BOOST_SERIALIZATION_NVP(m_LstDureeArretDynamique);
    ar & BOOST_SERIALIZATION_NVP(m_LstRatioDescente);
    ar & BOOST_SERIALIZATION_NVP(m_LstBinomialDescente);
    ar & BOOST_SERIALIZATION_NVP(m_LstStockRestant);
    ar & BOOST_SERIALIZATION_NVP(m_LstDemandes);
    ar & BOOST_SERIALIZATION_NVP(m_LstDernierRamassage);
    ar & BOOST_SERIALIZATION_NVP(m_LstWaitingUsers);
    ar & BOOST_SERIALIZATION_NVP(m_LastPassingTimes);
    ar & BOOST_SERIALIZATION_NVP(m_LstLastRatioMontee);
}

