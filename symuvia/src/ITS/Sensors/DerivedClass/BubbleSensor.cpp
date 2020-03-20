#include "stdafx.h"
#include "BubbleSensor.h"

#include "Logger.h"
#include "ITS/Stations/ITSStation.h"
#include "Xerces/XMLUtil.h"

XERCES_CPP_NAMESPACE_USE

BubbleSensor::BubbleSensor()
{

}

BubbleSensor::BubbleSensor(const BubbleSensor &SensorCopy)
    : ITSDynamicSensor(SensorCopy.m_pNetwork, SensorCopy.m_strSensorID, SensorCopy.m_strSensorType, SensorCopy.GetOwner(), SensorCopy.m_iUpdateFrequency, SensorCopy.m_dbBreakDownRate,
                SensorCopy.m_iPrecision, SensorCopy.m_dbNoiseRate), m_dbBubbleRange(SensorCopy.m_dbBubbleRange)
{

}

BubbleSensor::BubbleSensor(Reseau * pNetwork, const std::string strID, const std::string strType, ITSStation *pOwner, int dbUpdateFrequency,
                           double dbBreakDownRate, int iPrecision, double dbNoiseRate)
    :ITSDynamicSensor(pNetwork, strID, strType, pOwner, dbUpdateFrequency, dbBreakDownRate, iPrecision, dbNoiseRate)
{

}

BubbleSensor::~BubbleSensor()
{

}

bool BubbleSensor::UpdateData(const boost::posix_time::ptime &dtTimeStart, boost::posix_time::ptime& dtTimeEnd)
{
    Reseau* pNetwork = m_pOwner->GetNetwork();
    if(IsDown())
    {
        *(pNetwork->GetLogger()) << Logger::Error << "Error : The Sensor " << GetLabel() << " is down and so, can't update data !" << std::endl;
        return false;
    }

    Vehicule* pVehicule = GetParent();
    if(!pVehicule)
    {
        *(pNetwork->GetLogger()) << Logger::Error << "Error : no vehicle defined for " << m_pOwner->GetLabel() << " !" << std::endl;
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

bool BubbleSensor::LoadFromXML(DOMDocument *pDoc, DOMNode *pXMLNode)
{
    GetXmlAttributeValue(pXMLNode, "bubble_range", m_dbBubbleRange, m_pOwner->GetNetwork()->GetLogger());

    return true;
}

ITSSensor *BubbleSensor::CreateCopy(ITSStation* pNewOwner)
{
    BubbleSensor * pNewSensor = new BubbleSensor(m_pNetwork, m_strSensorID, m_strSensorType, pNewOwner, m_iUpdateFrequency, m_dbBreakDownRate, m_iPrecision, m_dbNoiseRate);
    pNewSensor->m_dbBubbleRange = m_dbBubbleRange;
    return pNewSensor;
}

template void BubbleSensor::serialize(boost::archive::xml_woarchive & ar, const unsigned int version);
template void BubbleSensor::serialize(boost::archive::xml_wiarchive & ar, const unsigned int version);

template<class Archive>
void BubbleSensor::serialize(Archive & ar, const unsigned int version)
{
    ar & BOOST_SERIALIZATION_BASE_OBJECT_NVP(ITSDynamicSensor);

    ar & BOOST_SERIALIZATION_NVP(m_dbBubbleRange);
}
