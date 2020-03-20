#include "stdafx.h"
#include "DocAcoustique.h"

#include <boost/archive/xml_woarchive.hpp>
#include <boost/archive/xml_wiarchive.hpp>

DocAcoustique::~DocAcoustique()
{
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Sérialisation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////
template void DocAcoustique::serialize(boost::archive::xml_woarchive & ar, const unsigned int version);
template void DocAcoustique::serialize(boost::archive::xml_wiarchive & ar, const unsigned int version);

template<class Archive>
void DocAcoustique::serialize(Archive & ar, const unsigned int version)
{
    // rien à sauvegarder dans cette classe pure virtual
}

