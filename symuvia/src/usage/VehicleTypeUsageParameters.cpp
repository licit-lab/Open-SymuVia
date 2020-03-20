#include "stdafx.h"
#include "VehicleTypeUsageParameters.h"

#include <boost/archive/xml_woarchive.hpp>
#include <boost/archive/xml_wiarchive.hpp>

VehicleTypeUsageParameters::VehicleTypeUsageParameters()
{
}

VehicleTypeUsageParameters::~VehicleTypeUsageParameters()
{
}

LoadUsageParameters & VehicleTypeUsageParameters::GetLoadUsageParameters()
{
    return m_loadUsageParameters;
}


template void VehicleTypeUsageParameters::serialize(boost::archive::xml_woarchive & ar, const unsigned int version);
template void VehicleTypeUsageParameters::serialize(boost::archive::xml_wiarchive & ar, const unsigned int version);

template<class Archive>
void VehicleTypeUsageParameters::serialize(Archive& ar, const unsigned int version)
{
    ar & BOOST_SERIALIZATION_NVP(m_loadUsageParameters);
}
