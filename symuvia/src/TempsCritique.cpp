#include "stdafx.h"
#include "TempsCritique.h"

#include "vehicule.h"

#include <boost/archive/xml_woarchive.hpp>

template void TempsCritique::serialize(boost::archive::xml_woarchive & ar, const unsigned int version);
template void TempsCritique::serialize(boost::archive::xml_wiarchive & ar, const unsigned int version);

template<class Archive>
void TempsCritique::serialize(Archive & ar, const unsigned int version)
{
    ar & BOOST_SERIALIZATION_NVP(pTV);
    ar & BOOST_SERIALIZATION_NVP(dbtc);
}