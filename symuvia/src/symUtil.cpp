#include "stdafx.h"
#include "symUtil.h"

#include <boost/archive/xml_woarchive.hpp>
#include <boost/archive/xml_wiarchive.hpp>

////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Sérialisation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////
template void Point::serialize(boost::archive::xml_woarchive & ar, const unsigned int version);
template void Point::serialize(boost::archive::xml_wiarchive & ar, const unsigned int version);

template<class Archive>
void Point::serialize(Archive & ar, const unsigned int version)
{
    ar & BOOST_SERIALIZATION_NVP(dbX);
    ar & BOOST_SERIALIZATION_NVP(dbY);
    ar & BOOST_SERIALIZATION_NVP(dbZ);
}