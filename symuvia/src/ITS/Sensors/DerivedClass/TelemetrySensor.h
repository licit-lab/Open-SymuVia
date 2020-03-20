#pragma once 

#ifndef SYMUCOM_TelemetrySensor_H
#define SYMUCOM_TelemetrySensor_H

#include "ITS/Sensors/ITSDynamicSensor.h"

#include <boost/shared_ptr.hpp>
#include <vector>

#pragma warning(push)
#pragma warning(disable : 4251)

class StaticElement;

class TelemetrySensor : public ITSDynamicSensor{

public:
    TelemetrySensor();
    TelemetrySensor(const TelemetrySensor &SensorCopy);
    TelemetrySensor(Reseau * pNetwork, const std::string strID, const std::string strType, ITSStation* pOwner, int dbUpdateFrequency = 1, double dbBreakDownRate = 0.0,
                    int iPrecision = -6, double dbNoiseRate = 0.0);
    virtual ~TelemetrySensor();

    virtual bool UpdateData(const boost::posix_time::ptime &dtTimeStart, boost::posix_time::ptime &dtTimeEnd);

    virtual bool LoadFromXML(XERCES_CPP_NAMESPACE::DOMDocument * pDoc, XERCES_CPP_NAMESPACE::DOMNode *pXMLNode);

    virtual ITSSensor* CreateCopy(ITSStation *pNewOwner);

	const std::map<Vehicule*, std::pair<double, Point> >& GetVehicleSeen() const;

    const std::vector<StaticElement *>& GetStaticElementsSeen() const;

	bool IsWithinSight(Point ptOtherPoint, bool bOnlyForward = false, bool bUseRangeInstead= false);

private:
	std::map<Vehicule*, std::pair<double, Point> > m_lstVehicleSeen; // list of vehicles that can be see by the vehicle, with his speed and distance

    std::vector<StaticElement*> m_lstStaticElementsSeen;//list of StaticElement that can be see by the vehicle

    int     m_iDistanceView; //maximum distance to view other elements

    double  m_dbAngle; //angle of the field of view

    bool    m_bForward; //If the sensor can see in front of the vehicle

    bool    m_bBackward; //If the sensor can see behind the vehicle

    Point   m_ptVehiclePosition; //Actual position of the vehicle

    Point   m_ptVehicleOrientation; //3D representation of the actual orientation of the vehicle

////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Sérialisation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////
private:
    friend class boost::serialization::access;
    template<class Archive>
    void serialize(Archive & ar, const unsigned int version);
};

#pragma warning(pop)

#endif // SYMUCOM_TelemetrySensor_H
