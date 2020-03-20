#include "stdafx.h"
#include "AbstractSensor.h"

#include <boost/archive/xml_woarchive.hpp>
#include <boost/archive/xml_wiarchive.hpp>

AbstractSensorSnapshot::~AbstractSensorSnapshot()
{
}

AbstractSensor::AbstractSensor()
{
}

AbstractSensor::AbstractSensor(const std::string & strNom)
{
    m_strNom = strNom;
}

AbstractSensor::~AbstractSensor()
{
}

void AbstractSensor::ResetData()
{
}

std::string AbstractSensor::GetUnifiedID()
{
    return m_strNom;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Sérialisation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////

template void AbstractSensor::serialize(boost::archive::xml_woarchive & ar, const unsigned int version);
template void AbstractSensor::serialize(boost::archive::xml_wiarchive & ar, const unsigned int version);

template<class Archive>
void AbstractSensor::serialize(Archive& ar, const unsigned int version)
{
	ar & BOOST_SERIALIZATION_NVP(m_strNom);
}

template void AbstractSensorSnapshot::serialize(boost::archive::xml_woarchive & ar, const unsigned int version);
template void AbstractSensorSnapshot::serialize(boost::archive::xml_wiarchive & ar, const unsigned int version);

template<class Archive>
void AbstractSensorSnapshot::serialize(Archive& ar, const unsigned int version)
{
}