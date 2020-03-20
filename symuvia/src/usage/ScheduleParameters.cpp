#include "stdafx.h"
#include "ScheduleParameters.h"

#include <boost/archive/xml_woarchive.hpp>
#include <boost/archive/xml_wiarchive.hpp>

ScheduleParameters::ScheduleParameters()
{
}

ScheduleParameters::~ScheduleParameters()
{
}

void ScheduleParameters::SetID(const std::string & strID)
{
    m_strID = strID;
}

const std::string & ScheduleParameters::GetID()
{
    return m_strID;
}

template void ScheduleParameters::serialize(boost::archive::xml_woarchive & ar, const unsigned int version);
template void ScheduleParameters::serialize(boost::archive::xml_wiarchive & ar, const unsigned int version);

template<class Archive>
void ScheduleParameters::serialize(Archive& ar, const unsigned int version)
{
    ar & BOOST_SERIALIZATION_NVP(m_strID);
}