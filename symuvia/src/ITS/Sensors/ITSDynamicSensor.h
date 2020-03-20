#pragma once

#ifndef SYMUCOM_ITSDYNAMICSENSOR_H
#define SYMUCOM_ITSDYNAMICSENSOR_H

#include "ITSSensor.h"

#pragma warning(push)
#pragma warning(disable : 4251)

class Vehicule;

class SYMUBRUIT_EXPORT ITSDynamicSensor : public ITSSensor{

public:
    ITSDynamicSensor();
    ITSDynamicSensor(const ITSDynamicSensor& SensorCopy);
    ITSDynamicSensor(Reseau * pNetwork, const std::string strID, const std::string strType, ITSStation* pOwner, int dbUpdateFrequency = 1, double dbBreakDownRate = 0.0,
                     int iPrecision = -6, double dbNoiseRate = 0.0);
    virtual ~ITSDynamicSensor();

    virtual bool UpdateData(const boost::posix_time::ptime &dtTimeStart, boost::posix_time::ptime& dtTimeEnd) = 0;

    virtual bool LoadFromXML(XERCES_CPP_NAMESPACE::DOMDocument * pDoc, XERCES_CPP_NAMESPACE::DOMNode *pXMLNode) = 0;

    virtual ITSSensor* CreateCopy(ITSStation *pNewOwner) = 0;

    Vehicule *GetParent() const;

protected:
    Voie * GetExactPosition(const boost::posix_time::ptime &dtTimeStart, double& dbPos);

////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Sérialisation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////
private:
    friend class boost::serialization::access;
    template<class Archive>
    void serialize(Archive & ar, const unsigned int version);
};

#pragma warning(pop)

#endif // SYMUCOM_ITSDYNAMICSENSOR_H
