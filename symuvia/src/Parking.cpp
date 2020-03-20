#include "stdafx.h"
#include "Parking.h"

#include "tuyau.h"
#include "SystemUtil.h"
#include "DocTrafic.h"
#include "reseau.h"
#include "ZoneDeTerminaison.h"
#include "vehicule.h"
#include "voie.h"
#include "DiagFonda.h"
#include "usage/SymuViaFleetParameters.h"

#include <boost/serialization/vector.hpp>
#include <boost/serialization/shared_ptr.hpp>

// *************************************
// Classe ParkingParameters
// *************************************

Parking::Parking() :
    SymuViaTripNode(),
    m_nStockMax(-1),
    m_nStock(0),
    m_dbInterTempsEntree(0),
    m_dbInterTempsSortie(0),
    m_pParentZone(NULL),
    m_bIsParcRelais(false),
    m_dbParcRelaisMaxDistance(100)
{
}

Parking::Parking(char _Nom[256], Reseau *pR,
    int nStockMax, int nStockInitial,
    double dbInterTempsEntree, double dbInterTempsSortie,
    std::vector<TypeVehicule*> lstTypesInterdits, bool bParcRelais, double dbParcRelaisMaxDistance) :
    SymuViaTripNode(_Nom, pR),
    m_nStockMax(nStockMax),
    m_nStock(nStockInitial),
    m_dbInterTempsEntree(dbInterTempsEntree),
    m_dbInterTempsSortie(dbInterTempsSortie),
    m_lstVehInterdits(lstTypesInterdits),
    m_pParentZone(NULL),
    m_bIsParcRelais(bParcRelais),
    m_dbParcRelaisMaxDistance(dbParcRelaisMaxDistance)
{
}

Parking::~Parking()
{
}

::Point Parking::getCoordonnees()
{
    Point pt;
    if (GetInputConnexion())
    {
        GetInputConnexion()->calculBary();
        pt.dbX = GetInputConnexion()->GetAbscisse();
        pt.dbY = GetInputConnexion()->GetOrdonnee();
    }
    if (GetOutputConnexion())
    {
        GetOutputConnexion()->calculBary();
        if (GetInputConnexion())
        {
            pt.dbX = (pt.dbX + GetOutputConnexion()->GetAbscisse()) / 2;
            pt.dbY = (pt.dbY + GetOutputConnexion()->GetOrdonnee()) / 2;
        }
        else
        {
            pt.dbX = GetOutputConnexion()->GetAbscisse();
            pt.dbY = GetOutputConnexion()->GetOrdonnee();
        }
    }
    return pt;
}


//================================================================
void Parking::SortieTrafic
//----------------------------------------------------------------
// Fonction  : Module de sortie des données trafic du parking
// Version du: 21/05/2012
// Historique: 21/05/2012 (O.Tonck - Ipsis)
//             Création
//================================================================
(
	DocTrafic *pXMLDocTrafic
)
{
	int nVeh = 0;

    if( GetOutputConnexion() && !GetOutputConnexion()->m_LstTuyAssAv.empty() )
    {
        for(int nVoie = 0; nVoie < GetOutputConnexion()->m_LstTuyAssAv.front()->getNb_voies(); nVoie++)	
	    {
		    nVeh += (int) m_mapVehEnAttente[GetOutputConnexion()->m_LstTuyAssAv.front()].find( nVoie )->second.size();		// Comptage des véhicules en attente
		    if( m_mapVehPret[GetOutputConnexion()->m_LstTuyAssAv.front()].find( nVoie )->second != -1)						// Ajout du véhicule en attente mais prêt
			    nVeh += 1;
	    }
    }

    pXMLDocTrafic->AddSimuParking(GetID(), GetStock(), nVeh);
}

// Renvoie la valeur de la demande pour l'origine considérée
double Parking::GetDemandeValue(double dbInst, double & dbVariationEndTime)
{
    double dbDemande;
    
    if(this->GetZoneParent() != NULL)
    {
        dbDemande = 0.0;
        dbVariationEndTime = DBL_MAX;
    }
    else
    {
        dbDemande = SymuViaTripNode::GetDemandeValue(dbInst, dbVariationEndTime);
    }

    return dbDemande;
}

bool Parking::IsAllowedVehiculeType(TypeVehicule * pTypeVehicule)
{
    bool result = SymuViaTripNode::IsAllowedVehiculeType(pTypeVehicule);

    for(size_t i = 0 ; i < GetLstTypesInterdits().size() && result; i++)
    {
        if(GetLstTypesInterdits()[i] == pTypeVehicule)
        {
            result = false;
        }
    }

    return result;
}


double Parking::GetNextEnterInstant(int nNbVoie, double  dbPrevInstSortie, double  dbInstant, double  dbPasDeTemps, const std::string & nextRouteID)
{
    double dbNextInstSortie = dbPrevInstSortie + GetInterTpsEntree();
    if(dbNextInstSortie > m_pNetwork->GetDureeSimu())
    {
        dbNextInstSortie = -1;
    }
    return dbNextInstSortie;
}

bool Parking::IsFull()
{
    // StockMax non infini et stock courant >= stock max
    return (GetStockMax() != -1) && (GetStock() >= GetStockMax());
}

void Parking::VehiculeEnter(boost::shared_ptr<Vehicule> pVehicle, VoieMicro * pDestinationEnterLane, double dbInstDestinationReached, double dbInstant, double dbTimeStep, bool bForcedOutside)
{
    TripNode::VehiculeEnter(pVehicle, pDestinationEnterLane, dbInstDestinationReached, dbInstant, dbTimeStep, bForcedOutside);

    // Incrémentation du stock
    IncStock();

    // Gestion de la demande en zone le cas échéant
    ZoneDeTerminaison * pZoneDest = dynamic_cast<ZoneDeTerminaison*>(pVehicle->GetDestination());
    if (pZoneDest)
    {
        pZoneDest->GestionDemandeStationnement(((SymuViaFleetParameters*)pVehicle->GetFleetParameters())->GetPlaqueDestination(), dbInstant);
    }

    // Calcul de l'instant de sortie au plus tôt du véhicule suivant (en respectant la capacité/temps intersortie)
    double dbNextInstSortie = GetNextEnterInstant( ((Tuyau*)pDestinationEnterLane->GetParent())->getNbVoiesDis(), dbInstDestinationReached, dbInstant, dbTimeStep, pVehicle->GetNextRouteID());
    pDestinationEnterLane->GetNextInstSortie()[std::string()] = dbNextInstSortie;

    pVehicle->SetRegimeFluide(true);
}

TripNodeSnapshot * Parking::TakeSnapshot()
{
    ParkingSnapshot * pResult = new ParkingSnapshot();
    for(size_t i = 0; i < m_LstStoppedVehicles.size(); i++)
    {
        pResult->lstStoppedVehicles.push_back(m_LstStoppedVehicles[i]->GetID());
    }
    // par rapport à la classe mère, on sauvegarde aussi les stocks
    pResult->stock = m_nStock;
    return pResult;
}

void Parking::Restore(Reseau * pNetwork, TripNodeSnapshot * backup)
{
    ParkingSnapshot * pParkingSnapShot = (ParkingSnapshot*)backup;
    m_LstStoppedVehicles.clear();
    for(size_t i = 0; i < backup->lstStoppedVehicles.size(); i++)
    {
        m_LstStoppedVehicles.push_back(pNetwork->GetVehiculeFromID(backup->lstStoppedVehicles[i]));
    }
    // par rapport à la classe mère, on restore aussi les stocks
    m_nStock = pParkingSnapShot->stock;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Sérialisation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////
template void ParkingSnapshot::serialize(boost::archive::xml_woarchive & ar, const unsigned int version);
template void ParkingSnapshot::serialize(boost::archive::xml_wiarchive & ar, const unsigned int version);

template<class Archive>
void ParkingSnapshot::serialize(Archive & ar, const unsigned int version)
{
    ar & BOOST_SERIALIZATION_BASE_OBJECT_NVP(TripNodeSnapshot);

    ar & BOOST_SERIALIZATION_NVP(stock);
}

template void Parking::serialize(boost::archive::xml_woarchive & ar, const unsigned int version);
template void Parking::serialize(boost::archive::xml_wiarchive & ar, const unsigned int version);

template<class Archive>
void Parking::serialize(Archive & ar, const unsigned int version)
{
    ar & BOOST_SERIALIZATION_BASE_OBJECT_NVP(SymuViaTripNode);

	ar & BOOST_SERIALIZATION_NVP(m_nStockMax);
    ar & BOOST_SERIALIZATION_NVP(m_nStock);
    ar & BOOST_SERIALIZATION_NVP(m_dbInterTempsEntree);
    ar & BOOST_SERIALIZATION_NVP(m_dbInterTempsSortie);
    ar & BOOST_SERIALIZATION_NVP(m_lstVehInterdits);
    ar & BOOST_SERIALIZATION_NVP(m_pParentZone);
    ar & BOOST_SERIALIZATION_NVP(m_bIsParcRelais);
    ar & BOOST_SERIALIZATION_NVP(m_dbParcRelaisMaxDistance);
}

