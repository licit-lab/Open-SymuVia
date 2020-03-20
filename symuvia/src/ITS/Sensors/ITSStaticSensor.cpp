#include "stdafx.h"
#include "ITSStaticSensor.h"

#include "reseau.h"
#include "Logger.h"
#include "ITS/Stations/ITSStation.h"

ITSStaticSensor::ITSStaticSensor()
{

}

ITSStaticSensor::ITSStaticSensor(const ITSStaticSensor &SensorCopy)
        : ITSSensor(SensorCopy.m_pNetwork, SensorCopy.m_strSensorID, SensorCopy.m_strSensorType, SensorCopy.GetOwner(), SensorCopy.m_iUpdateFrequency, SensorCopy.m_dbBreakDownRate,
                    SensorCopy.m_iPrecision, SensorCopy.m_dbNoiseRate)
{

}

ITSStaticSensor::ITSStaticSensor(Reseau * pNetwork, const std::string strID, const std::string strType, ITSStation *pOwner, int dbUpdateFrequency,
                                 double dbBreakDownRate, int iPrecision, double dbNoiseRate)
        :ITSSensor(pNetwork, strID, strType, pOwner, dbUpdateFrequency, dbBreakDownRate, iPrecision, dbNoiseRate)
{

}

ITSStaticSensor::~ITSStaticSensor()
{

}

StaticElement *ITSStaticSensor::GetParent() const
{
    StaticElement *pStaticParent = m_pOwner->GetStaticParent();
    if(!pStaticParent)
        *(m_pOwner->GetNetwork()->m_pLogger) << Logger::Warning << "Warning : Sensor " << m_strSensorID << " is type " << m_strSensorType << " but is not own by a static station !" << std::endl;

    return pStaticParent;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Sérialisation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////
template void ITSStaticSensor::serialize(boost::archive::xml_woarchive & ar, const unsigned int version);
template void ITSStaticSensor::serialize(boost::archive::xml_wiarchive & ar, const unsigned int version);

template<class Archive>
void ITSStaticSensor::serialize(Archive& ar, const unsigned int version)
{
    ar & BOOST_SERIALIZATION_BASE_OBJECT_NVP(ITSSensor);
}
