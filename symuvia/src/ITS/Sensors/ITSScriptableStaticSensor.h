#pragma once 

#ifndef SYMUCOM_ITSSCRIPTABLESTATICSENSOR_H
#define SYMUCOM_ITSSCRIPTABLESTATICSENSOR_H

#include "ITSStaticSensor.h"
#include "ITS/ITSScriptableElement.h"

#pragma warning(push)
#pragma warning(disable : 4251)

class ITSScriptableStaticSensor : public ITSStaticSensor, public ITSScriptableElement{

public:
    ITSScriptableStaticSensor();
    ITSScriptableStaticSensor(const ITSScriptableStaticSensor &SensorCopy);
    ITSScriptableStaticSensor(Reseau * pNetwork, const std::string strID, const std::string strType, ITSStation* pOwner, int dbUpdateFrequency = 1, double dbBreakDownRate = 0.0,
                    int iPrecision = -6, double dbNoiseRate = 0.0);
    virtual ~ITSScriptableStaticSensor();

    virtual bool UpdateData(const boost::posix_time::ptime &dtTimeStart, boost::posix_time::ptime& dtTimeEnd);

    virtual bool LoadFromXML(XERCES_CPP_NAMESPACE::DOMDocument * pDoc, XERCES_CPP_NAMESPACE::DOMNode *pXMLNode);

    virtual ITSSensor* CreateCopy(ITSStation *pNewOwner);

////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Sérialisation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////
private:
    friend class boost::serialization::access;
    template<class Archive>
    void serialize(Archive & ar, const unsigned int version);

};

#pragma warning(pop)

#endif // SYMUCOM_ITSSCRIPTABLESTATICSENSOR_H
