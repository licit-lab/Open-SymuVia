#include "stdafx.h"
#include "Passenger.h"

#include "TripNode.h"


Passenger::Passenger()
{
    m_ExternalID = -1;
    m_pDestination = NULL;
}

Passenger::Passenger(int externalID, TripNode * pDestination)
{
    m_ExternalID = externalID;
    m_pDestination = pDestination;
}

Passenger::~Passenger()
{
}

int Passenger::GetExternalID() const
{
    return m_ExternalID;
}

TripNode * Passenger::GetDestination() const
{
    return m_pDestination;
}

template void Passenger::serialize(boost::archive::xml_woarchive & ar, const unsigned int version);
template void Passenger::serialize(boost::archive::xml_wiarchive & ar, const unsigned int version);

template<class Archive>
void Passenger::serialize(Archive& ar, const unsigned int version)
{

    ar & BOOST_SERIALIZATION_NVP(m_ExternalID);
    ar & BOOST_SERIALIZATION_NVP(m_pDestination);
}
