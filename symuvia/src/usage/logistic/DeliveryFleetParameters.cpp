#include "stdafx.h"
#include "DeliveryFleetParameters.h"

#include "usage/TripNode.h"

#include <boost/archive/xml_woarchive.hpp>
#include <boost/archive/xml_wiarchive.hpp>

#include <boost/serialization/vector.hpp>
#include <boost/serialization/map.hpp>

DeliveryFleetParameters::DeliveryFleetParameters()
{
}

DeliveryFleetParameters::~DeliveryFleetParameters()
{

}

std::map<TripNode*, std::vector<std::pair<int,int> > > & DeliveryFleetParameters::GetMapLoadsUnloads()
{
    return m_mapLoadsUnloads;
}

AbstractFleetParameters * DeliveryFleetParameters::Clone()
{
    DeliveryFleetParameters * pResult = new DeliveryFleetParameters();
    pResult->m_mapLoadsUnloads = m_mapLoadsUnloads;
    return pResult;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Sérialisation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////
template void DeliveryFleetParameters::serialize(boost::archive::xml_woarchive & ar, const unsigned int version);
template void DeliveryFleetParameters::serialize(boost::archive::xml_wiarchive & ar, const unsigned int version);

template<class Archive>
void DeliveryFleetParameters::serialize(Archive & ar, const unsigned int version)
{
    ar & BOOST_SERIALIZATION_BASE_OBJECT_NVP(AbstractFleetParameters);

    ar & BOOST_SERIALIZATION_NVP(m_mapLoadsUnloads);
}