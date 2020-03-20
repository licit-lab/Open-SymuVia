#include "stdafx.h"
#include "ITSExternalStaticSensor.h"

#include "ITS/ExternalLibraries/ITSExternalLibrary.h"

#include "Xerces/XMLUtil.h"

ITSExternalStaticSensor::ITSExternalStaticSensor()
{
    m_pExtLib = NULL;
}

ITSExternalStaticSensor::ITSExternalStaticSensor(const ITSExternalStaticSensor &SensorCopy)
        : ITSStaticSensor(SensorCopy.m_pNetwork, SensorCopy.m_strSensorID, SensorCopy.m_strSensorType, SensorCopy.GetOwner(), SensorCopy.m_iUpdateFrequency, SensorCopy.m_dbBreakDownRate,
                    SensorCopy.m_iPrecision, SensorCopy.m_dbNoiseRate)
{
    m_pExtLib = SensorCopy.m_pExtLib;
    m_customParams = SensorCopy.m_customParams;
}

ITSExternalStaticSensor::ITSExternalStaticSensor(Reseau * pNetwork, const std::string strID, const std::string strType, ITSStation *pOwner, int dbUpdateFrequency,
                                 double dbBreakDownRate, int iPrecision, double dbNoiseRate, ITSExternalLibrary * pExtLib)
       : ITSStaticSensor(pNetwork, strID, strType, pOwner, dbUpdateFrequency, dbBreakDownRate, iPrecision, dbNoiseRate)
{
    m_pExtLib = pExtLib;
}

ITSExternalStaticSensor::~ITSExternalStaticSensor()
{

}

bool ITSExternalStaticSensor::UpdateData(const boost::posix_time::ptime &dtTimeStart, boost::posix_time::ptime &dtTimeEnd)
{
    return m_pExtLib->UpdateSensorData(this, m_pNetwork, m_customParams, dtTimeStart, dtTimeEnd, m_pNetwork->GetLogger());
}

bool ITSExternalStaticSensor::LoadFromXML(XERCES_CPP_NAMESPACE::DOMDocument * pDoc, XERCES_CPP_NAMESPACE::DOMNode *pXMLNode)
{
    m_customParams = XMLUtil::getOuterXml(pXMLNode);
    return true;
}

ITSSensor *ITSExternalStaticSensor::CreateCopy(ITSStation *pNewOwner)
{
    ITSExternalStaticSensor * pNewSensor = new ITSExternalStaticSensor(m_pNetwork, m_strSensorID, m_strSensorType, pNewOwner, m_iUpdateFrequency, m_dbBreakDownRate, m_iPrecision, m_dbNoiseRate, m_pExtLib);
    pNewSensor->m_customParams = m_customParams;
    return pNewSensor;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Sérialisation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////
template void ITSExternalStaticSensor::serialize(boost::archive::xml_woarchive & ar, const unsigned int version);
template void ITSExternalStaticSensor::serialize(boost::archive::xml_wiarchive & ar, const unsigned int version);

template<class Archive>
void ITSExternalStaticSensor::serialize(Archive& ar, const unsigned int version)
{
    ar & BOOST_SERIALIZATION_BASE_OBJECT_NVP(ITSStaticSensor);

    ar & BOOST_SERIALIZATION_NVP(m_pExtLib);
    ar & BOOST_SERIALIZATION_NVP(m_customParams);
}

