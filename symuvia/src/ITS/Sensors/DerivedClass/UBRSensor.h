#pragma once 

#ifndef SYMUCOM_UBRSensor_H
#define SYMUCOM_UBRSensor_H

#include "ITS/Sensors/ITSStaticSensor.h"
#include "symUtil.h"

#pragma warning(push)
#pragma warning(disable : 4251)

class UBRSensor : public ITSStaticSensor{

public:
    UBRSensor();
    UBRSensor(const UBRSensor &SensorCopy);
    UBRSensor(Reseau * pNetwork, const std::string strID, const std::string strType, ITSStation* pOwner, int dbUpdateFrequency = 1,
              double dbBreakDownRate = 0.0, int iPrecision = -6, double dbNoiseRate = 0.0);
    virtual ~UBRSensor();

    virtual bool UpdateData(const boost::posix_time::ptime &dtTimeStart, boost::posix_time::ptime &dtTimeEnd);

    virtual bool LoadFromXML(XERCES_CPP_NAMESPACE::DOMDocument * pDoc, XERCES_CPP_NAMESPACE::DOMNode *pXMLNode);

    virtual ITSSensor* CreateCopy(ITSStation *pNewOwner);

private:
    Point m_ptStartPosition; //start position of the sensor

    Point m_ptEndPosition; //end position of the sensor

////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Sérialisation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////
private:
    friend class boost::serialization::access;
    template<class Archive>
    void serialize(Archive & ar, const unsigned int version);
};

#pragma warning(pop)

#endif // SYMUCOM_UBRSensor_H
