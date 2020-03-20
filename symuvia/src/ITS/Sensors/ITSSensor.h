#pragma once 

#ifndef SYMUCOM_ITSSENSOR_H
#define SYMUCOM_ITSSENSOR_H

#include "SymubruitExports.h"

#include "reseau.h"

#include <boost/date_time/posix_time/posix_time_types.hpp>
#include <string>

#pragma warning(push)
#pragma warning(disable : 4251)

class ITSStation;

namespace XERCES_CPP_NAMESPACE {
    class DOMNode;
    class DOMDocument;
}

struct ITSSensorSnapshot {
    bool bIsDown;

////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Sérialisation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////
private:
    friend class boost::serialization::access;
    template<class Archive>
    void serialize(Archive & ar, const unsigned int version);
};

class SYMUBRUIT_EXPORT ITSSensor{

public:
    ITSSensor();
    ITSSensor(const ITSSensor& SensorCopy);
    ITSSensor(Reseau * pNetwork, const std::string strID, const std::string strType, ITSStation* pOwner, int dbUpdateFrequency = 1, double dbBreakDownRate = 0.0,
              int iPrecision = -6, double dbNoiseRate = 0.0);
    virtual ~ITSSensor();

    virtual bool UpdateData(const boost::posix_time::ptime &dtTimeStart, boost::posix_time::ptime& dtTimeEnd) = 0;

    virtual bool UpdateAllData(const boost::posix_time::ptime &dtStartTime, double dbTimeDuration);

    virtual bool LoadFromXML(XERCES_CPP_NAMESPACE::DOMDocument * pDoc, XERCES_CPP_NAMESPACE::DOMNode *pXMLNode) = 0;

    virtual ITSSensor* CreateCopy(ITSStation *pNewOwner) = 0;

    std::string GetLabel() const;
    std::string GetType() const;
    ITSStation *GetOwner() const;
    bool IsDown();
    int GetUpdateFrequency() const;
    int GetPrecision() const;
    double GetNoiseRate() const;

    virtual ITSSensorSnapshot * TakeSnapshot() const;
    virtual void Restore(ITSSensorSnapshot* pSnapshot);

protected:
    void ComputePrecision(double &dbValue);

protected:

    Reseau * m_pNetwork;

    std::string m_strSensorID; // Name of the sensor

    std::string m_strSensorType; //type of the sensor ex: GPS

    ITSStation* m_pOwner; //Pointer to the station that own this sensor. It can be a static station or a dynamic station

    int         m_iUpdateFrequency; //frequency to update value (number of update by run)

    double      m_dbBreakDownRate; // probability for the sensor to break down

    int         m_iPrecision; //Precision of results that this sensor give (10^x)

    double      m_dbNoiseRate; //Noise included in the results of the sensor

    bool        m_bIsDown; //true when the sensor is down

////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Sérialisation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////
private:
    friend class boost::serialization::access;
    template<class Archive>
    void serialize(Archive & ar, const unsigned int version);

};

#pragma warning(pop)

#endif // SYMUCOM_ITSSENSOR_H
