#include "stdafx.h"
#include "Motif.h"

#include <boost/archive/xml_woarchive.hpp>
#include <boost/archive/xml_wiarchive.hpp>

CMotif::CMotif()
{
}

CMotif::CMotif(const std::string & strID)
{
    m_strID = strID;
}

CMotif::~CMotif()
{
}

const std::string & CMotif::GetID()
{
    return m_strID;
}

void CMotif::setDescription(const std::string & strDesc)
{
    m_strDescription = strDesc;
}

template void CMotif::serialize(boost::archive::xml_woarchive & ar, const unsigned int version);
template void CMotif::serialize(boost::archive::xml_wiarchive & ar, const unsigned int version);

template<class Archive>
void CMotif::serialize(Archive & ar, const unsigned int version)
{
    ar & BOOST_SERIALIZATION_NVP(m_strID);
    ar & BOOST_SERIALIZATION_NVP(m_strDescription);
}
