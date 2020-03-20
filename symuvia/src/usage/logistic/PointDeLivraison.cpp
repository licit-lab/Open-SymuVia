#include "stdafx.h"
#include "PointDeLivraison.h"

PointDeLivraison::PointDeLivraison()
    : TripNode()
{
    // Les véhicules doivent s'arrêter au point de livraison.
    m_bStopNeeded = true;
    m_bListVehiclesInTripNode = true;

    m_nNbPlaces = INT_MAX;
}

PointDeLivraison::PointDeLivraison(const std::string & strID, Reseau * pNetwork)
    : TripNode(strID, pNetwork)
{
    // Les véhicules doivent s'arrêter au point de livraison.
    m_bStopNeeded = true;
    m_bListVehiclesInTripNode = true;

    m_nNbPlaces = INT_MAX;
}

PointDeLivraison::~PointDeLivraison()
{
}

void PointDeLivraison::SetNbPlaces(int nbPlaces)
{
    m_nNbPlaces = nbPlaces;
}

void PointDeLivraison::VehiculeEnter(boost::shared_ptr<Vehicule> pVehicle, VoieMicro * pDestinationEnterLane, double dbInstDestinationReached, double dbInstant, double dbTimeStep, bool bForcedOutside)
{
    // Si le point de livraison est complet, on laisse le véhicule immobile, sinon on le traite normalement
    if((int)m_LstStoppedVehicles.size() < m_nNbPlaces)
    {
        // Appel de la méthode de la classe mère
        TripNode::VehiculeEnter(pVehicle, pDestinationEnterLane, dbInstDestinationReached, dbInstant, dbTimeStep, bForcedOutside);
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Sérialisation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////
template void PointDeLivraison::serialize(boost::archive::xml_woarchive & ar, const unsigned int version);
template void PointDeLivraison::serialize(boost::archive::xml_wiarchive & ar, const unsigned int version);

template<class Archive>
void PointDeLivraison::serialize(Archive & ar, const unsigned int version)
{
    ar & BOOST_SERIALIZATION_BASE_OBJECT_NVP(TripNode);

    ar & BOOST_SERIALIZATION_NVP(m_nNbPlaces);
}