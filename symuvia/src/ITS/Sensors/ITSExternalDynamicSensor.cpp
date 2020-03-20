#include "stdafx.h"
#include "ITSExternalDynamicSensor.h"

#include "ITS/ExternalLibraries/ITSExternalLibrary.h"

#include "Xerces/XMLUtil.h"

ITSExternalDynamicSensor::ITSExternalDynamicSensor()
{
    m_pExtLib = NULL;
}

ITSExternalDynamicSensor::ITSExternalDynamicSensor(const ITSExternalDynamicSensor &SensorCopy)
: ITSDynamicSensor(SensorCopy.m_pNetwork, SensorCopy.m_strSensorID, SensorCopy.m_strSensorType, SensorCopy.GetOwner(), SensorCopy.m_iUpdateFrequency, SensorCopy.m_dbBreakDownRate,
SensorCopy.m_iPrecision, SensorCopy.m_dbNoiseRate)
{
    m_pExtLib = SensorCopy.m_pExtLib;
    m_customParams = SensorCopy.m_customParams;
}

ITSExternalDynamicSensor::ITSExternalDynamicSensor(Reseau * pNetwork, const std::string strID, const std::string strType, ITSStation *pOwner, int dbUpdateFrequency,
    double dbBreakDownRate, int iPrecision, double dbNoiseRate, ITSExternalLibrary * pExtLib)
    : ITSDynamicSensor(pNetwork, strID, strType, pOwner, dbUpdateFrequency, dbBreakDownRate, iPrecision, dbNoiseRate)
{
    m_pExtLib = pExtLib;
}

ITSExternalDynamicSensor::~ITSExternalDynamicSensor()
{

}

bool ITSExternalDynamicSensor::UpdateData(const boost::posix_time::ptime &dtTimeStart, boost::posix_time::ptime &dtTimeEnd)
{
    return m_pExtLib->UpdateSensorData(this, m_pNetwork, m_customParams, dtTimeStart, dtTimeEnd, m_pNetwork->GetLogger());
}

bool ITSExternalDynamicSensor::LoadFromXML(XERCES_CPP_NAMESPACE::DOMDocument * pDoc, XERCES_CPP_NAMESPACE::DOMNode *pXMLNode)
{
    m_customParams = XMLUtil::getOuterXml(pXMLNode);
    return true;
}

ITSSensor *ITSExternalDynamicSensor::CreateCopy(ITSStation *pNewOwner)
{
    ITSExternalDynamicSensor * pNewSensor = new ITSExternalDynamicSensor(m_pNetwork, m_strSensorID, m_strSensorType, pNewOwner, m_iUpdateFrequency, m_dbBreakDownRate, m_iPrecision, m_dbNoiseRate, m_pExtLib);
    pNewSensor->m_customParams = m_customParams;
    return pNewSensor;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Sérialisation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////
template void ITSExternalDynamicSensor::serialize(boost::archive::xml_woarchive & ar, const unsigned int version);
template void ITSExternalDynamicSensor::serialize(boost::archive::xml_wiarchive & ar, const unsigned int version);

template<class Archive>
void ITSExternalDynamicSensor::serialize(Archive& ar, const unsigned int version)
{
    ar & BOOST_SERIALIZATION_BASE_OBJECT_NVP(ITSDynamicSensor);

    ar & BOOST_SERIALIZATION_NVP(m_pExtLib);
    ar & BOOST_SERIALIZATION_NVP(m_customParams);
}


