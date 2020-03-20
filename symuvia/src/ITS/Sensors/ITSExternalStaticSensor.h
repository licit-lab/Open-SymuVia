#pragma once 

#ifndef SYMUCOM_ITSEXTERNALSTATICSENSOR_H
#define SYMUCOM_ITSEXTERNALSTATICSENSOR_H

#include "ITSStaticSensor.h"

class Reseau;
class ITSExternalLibrary;
class ITSExternalStaticSensor : public ITSStaticSensor {

public:
    ITSExternalStaticSensor();
    ITSExternalStaticSensor(const ITSExternalStaticSensor &SensorCopy);
    ITSExternalStaticSensor(Reseau * pNetwork, const std::string strID, const std::string strType, ITSStation* pOwner, int dbUpdateFrequency = 1, double dbBreakDownRate = 0.0,
        int iPrecision = -6, double dbNoiseRate = 0.0, ITSExternalLibrary * pExtLib = NULL);
    virtual ~ITSExternalStaticSensor();

    virtual bool UpdateData(const boost::posix_time::ptime &dtTimeStart, boost::posix_time::ptime& dtTimeEnd);

    virtual bool LoadFromXML(XERCES_CPP_NAMESPACE::DOMDocument * pDoc, XERCES_CPP_NAMESPACE::DOMNode *pXMLNode);

    virtual ITSSensor* CreateCopy(ITSStation *pNewOwner);

private:
    ITSExternalLibrary * m_pExtLib;

    std::string m_customParams;

////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Sérialisation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////
private:
    friend class boost::serialization::access;
    template<class Archive>
    void serialize(Archive & ar, const unsigned int version);

};


#endif // SYMUCOM_ITSEXTERNALSTATICSENSOR_H
