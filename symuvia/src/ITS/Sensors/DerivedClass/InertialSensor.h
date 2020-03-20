#pragma once 

#ifndef SYMUCOM_InertialSensor_H
#define SYMUCOM_InertialSensor_H

#include "ITS/Sensors/ITSDynamicSensor.h"
#include "symUtil.h"

#pragma warning(push)
#pragma warning(disable : 4251)

class InertialSensor : public ITSDynamicSensor{

public:
    InertialSensor();
    InertialSensor(const InertialSensor &SensorCopy);
    InertialSensor(Reseau * pNetwork, const std::string strID, const std::string strType, ITSStation* pOwner, int dbUpdateFrequency = 1, double dbBreakDownRate = 0.0,
                   int iPrecision = -6, double dbNoiseRate = 0.0);
    virtual ~InertialSensor();

    virtual bool UpdateData(const boost::posix_time::ptime &dtTimeStart, boost::posix_time::ptime &dtTimeEnd);

    virtual bool LoadFromXML(XERCES_CPP_NAMESPACE::DOMDocument * pDoc, XERCES_CPP_NAMESPACE::DOMNode *pXMLNode);

    virtual ITSSensor* CreateCopy(ITSStation *pNewOwner);

    Point GetOrientation() const;
    double GetAcceleration() const;

private:
    double  m_dbAcceleration;//acceleration of the vehicle

    Point   m_ptOrientation;//orientation of the vehicle (expressed as a 3D vector)

////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Sérialisation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////
private:
    friend class boost::serialization::access;
    template<class Archive>
    void serialize(Archive & ar, const unsigned int version);
};

#pragma warning(pop)

#endif // SYMUCOM_InertialSensor_H
