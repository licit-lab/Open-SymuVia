#pragma once 

#ifndef SYMUCOM_ITSSTATICSENSOR_H
#define SYMUCOM_ITSSTATICSENSOR_H

#include "ITSSensor.h"

#pragma warning(push)
#pragma warning(disable : 4251)

class StaticElement;
class ITSStaticStation;

class SYMUBRUIT_EXPORT ITSStaticSensor : public ITSSensor{

public:
    ITSStaticSensor();
    ITSStaticSensor(const ITSStaticSensor &SensorCopy);
    ITSStaticSensor(Reseau * pNetwork, const std::string strID, const std::string strType, ITSStation* pOwner, int dbUpdateFrequency = 1, double dbBreakDownRate = 0.0,
                    int iPrecision = -6, double dbNoiseRate = 0.0);
    virtual ~ITSStaticSensor();

    virtual bool UpdateData(const boost::posix_time::ptime &dtTimeStart, boost::posix_time::ptime& dtTimeEnd) = 0;

    virtual bool LoadFromXML(XERCES_CPP_NAMESPACE::DOMDocument * pDoc, XERCES_CPP_NAMESPACE::DOMNode *pXMLNode) = 0;

    virtual ITSSensor* CreateCopy(ITSStation *pNewOwner) = 0;

    StaticElement* GetParent() const;

////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Sérialisation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////
private:
    friend class boost::serialization::access;
    template<class Archive>
    void serialize(Archive & ar, const unsigned int version);
};

#pragma warning(pop)

#endif // SYMUCOM_ITSSTATICSENSOR_H
