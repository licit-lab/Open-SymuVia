#pragma once 

#ifndef SYMUCOM_GPSSensor_H
#define SYMUCOM_GPSSensor_H

#include "ITS/Sensors/ITSDynamicSensor.h"
#include "symUtil.h"

#pragma warning(push)
#pragma warning(disable : 4251)

class GPSSensor : public ITSDynamicSensor{

public:
    GPSSensor();
    GPSSensor(const GPSSensor &SensorCopy);
    GPSSensor(Reseau * pNetwork, const std::string strID, const std::string strType, ITSStation* pOwner, int dbUpdateFrequency = 1, double dbBreakDownRate = 0.0,
              int iPrecision = -6, double dbNoiseRate = 0.0);
    virtual ~GPSSensor();

    virtual bool UpdateData(const boost::posix_time::ptime &dtTimeStart, boost::posix_time::ptime& dtTimeEnd);

    virtual bool LoadFromXML(XERCES_CPP_NAMESPACE::DOMDocument * pDoc, XERCES_CPP_NAMESPACE::DOMNode *pXMLNode);

    virtual ITSSensor* CreateCopy(ITSStation *pNewOwner);

    Point GetPointPosition() const;
    double GetSpeed() const;
    double GetDoublePosition() const;
    Voie *GetLaneAtStart() const;
    Voie *GetLaneAtEnd() const;
    double GetDoublePositionAtEnd() const;
    Point GetPointPositionAtEnd() const;

private:
    double  m_dbSpeed; // Speed of the vehicle

    Point   m_ptPositionAtStart; //Position of the vehicle at the start of the update (3D position)

	Point   m_ptPositionAtEnd; //Position of the vehicle at the end of the update (3D position)

    double  m_dbPositionAtStart; //Position of the vehicle at the start of the update (Double position)

    double  m_dbPositionAtEnd; //Position of the vehicle at the end of the update (Double position)

    Voie*  m_pLaneAtStart; //Tuyau where the vehicle currently is

	Voie*  m_pLaneAtEnd; //Tuyau where the vehicle is at the end of the update

////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Sérialisation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////
private:
    friend class boost::serialization::access;
    template<class Archive>
    void serialize(Archive & ar, const unsigned int version);
};

#pragma warning(pop)

#endif // SYMUCOM_GPSSensor_H
