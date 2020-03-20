#include "stdafx.h"
#include "UBRSensor.h"

#include "Logger.h"
#include "ITS/Stations/ITSStation.h"
#include "Xerces/XMLUtil.h"

XERCES_CPP_NAMESPACE_USE

UBRSensor::UBRSensor()
{

}

UBRSensor::UBRSensor(const UBRSensor &SensorCopy)
    : ITSStaticSensor(SensorCopy.m_pNetwork, SensorCopy.m_strSensorID, SensorCopy.m_strSensorType, SensorCopy.GetOwner(), SensorCopy.m_iUpdateFrequency, SensorCopy.m_dbBreakDownRate,
            SensorCopy.m_iPrecision, SensorCopy.m_dbNoiseRate), m_ptStartPosition(SensorCopy.m_ptStartPosition), m_ptEndPosition(SensorCopy.m_ptEndPosition)
{

}

UBRSensor::UBRSensor(Reseau * pNetwork, const std::string strID, const std::string strType, ITSStation *pOwner,
                     int dbUpdateFrequency, double dbBreakDownRate, int iPrecision, double dbNoiseRate)
    :ITSStaticSensor(pNetwork, strID, strType, pOwner, dbUpdateFrequency, dbBreakDownRate, iPrecision, dbNoiseRate)
{

}

UBRSensor::~UBRSensor()
{

}

bool UBRSensor::UpdateData(const boost::posix_time::ptime &dtTimeStart, boost::posix_time::ptime& dtTimeEnd)
{
    Reseau* pNetwork = m_pOwner->GetNetwork();
    if(IsDown())
    {
        *(pNetwork->GetLogger()) << Logger::Error << "Error : The Sensor " << GetLabel() << " is down and so, can't update data !" << std::endl;
        return false;
    }

    StaticElement* pStaticElement = GetParent();
    if(!pStaticElement)
    {
        *(pNetwork->GetLogger()) << Logger::Error << "Error : no static element defined for " << m_pOwner->GetLabel() << " !" << std::endl;
        return false;
    }

    if(dtTimeStart > dtTimeEnd)
    {
        *(pNetwork->GetLogger()) << Logger::Error << "Error when calling UpdateData for Sensor " << GetLabel() << " : start time must be before end time !" << std::endl;
        return false;
    }

    //TODO

    return true;
}

bool UBRSensor::LoadFromXML(DOMDocument *pDoc, DOMNode *pXMLNode)
{
    GetXmlAttributeValue(pXMLNode, "start_point_x", m_ptStartPosition.dbX, m_pOwner->GetNetwork()->GetLogger());

    GetXmlAttributeValue(pXMLNode, "start_point_y", m_ptStartPosition.dbY, m_pOwner->GetNetwork()->GetLogger());

    GetXmlAttributeValue(pXMLNode, "start_point_z", m_ptStartPosition.dbZ, m_pOwner->GetNetwork()->GetLogger());

    GetXmlAttributeValue(pXMLNode, "end_point_x", m_ptEndPosition.dbX, m_pOwner->GetNetwork()->GetLogger());

    GetXmlAttributeValue(pXMLNode, "end_point_x", m_ptEndPosition.dbY, m_pOwner->GetNetwork()->GetLogger());

    GetXmlAttributeValue(pXMLNode, "end_point_x", m_ptEndPosition.dbZ, m_pOwner->GetNetwork()->GetLogger());

    return true;
}

ITSSensor *UBRSensor::CreateCopy(ITSStation *pNewOwner)
{
    UBRSensor * pNewSensor = new UBRSensor(m_pNetwork, m_strSensorID, m_strSensorType, pNewOwner, m_iUpdateFrequency, m_dbBreakDownRate, m_iPrecision, m_dbNoiseRate);
    pNewSensor->m_ptStartPosition = m_ptStartPosition;
    pNewSensor->m_ptEndPosition = m_ptEndPosition;
    return pNewSensor;
}


template void UBRSensor::serialize(boost::archive::xml_woarchive & ar, const unsigned int version);
template void UBRSensor::serialize(boost::archive::xml_wiarchive & ar, const unsigned int version);

template<class Archive>
void UBRSensor::serialize(Archive & ar, const unsigned int version)
{
    ar & BOOST_SERIALIZATION_BASE_OBJECT_NVP(ITSStaticSensor);

    ar & BOOST_SERIALIZATION_NVP(m_ptStartPosition);
    ar & BOOST_SERIALIZATION_NVP(m_ptEndPosition);
}


