#include "stdafx.h"
#include "ITSSensor.h"

#include "ITS/Stations/ITSStation.h"

#include "RandManager.h"
#include "boost/date_time/posix_time/posix_time.hpp"
#include <Communication/CommunicationRunner.h>

ITSSensor::ITSSensor()
{
    m_pNetwork = NULL;
}

ITSSensor::ITSSensor(const ITSSensor &SensorCopy)
    :m_pNetwork(SensorCopy.m_pNetwork), m_dbBreakDownRate(SensorCopy.m_dbBreakDownRate), m_strSensorID(SensorCopy.m_strSensorID), m_strSensorType(SensorCopy.m_strSensorType), m_pOwner(SensorCopy.m_pOwner),
      m_iUpdateFrequency(SensorCopy.m_iUpdateFrequency), m_iPrecision(SensorCopy.m_iPrecision), m_dbNoiseRate(SensorCopy.m_dbNoiseRate), m_bIsDown(false)
{

}

ITSSensor::ITSSensor(Reseau * pNetwork, const std::string strID, const std::string strType, ITSStation *pOwner, int dbUpdateFrequency, double dbBreakDownRate,
                     int iPrecision, double dbNoiseRate)
        :m_pNetwork(pNetwork), m_dbBreakDownRate(dbBreakDownRate), m_strSensorID(strID), m_strSensorType(strType), m_pOwner(pOwner), m_iUpdateFrequency(dbUpdateFrequency),
          m_iPrecision(iPrecision), m_dbNoiseRate(dbNoiseRate), m_bIsDown(false)
{

}

ITSSensor::~ITSSensor()
{

}

bool ITSSensor::UpdateAllData(const boost::posix_time::ptime &dtStartTime, double dbTimeDuration)
{
    double dbUpdateTimeDuration = dbTimeDuration / m_iUpdateFrequency;
    boost::posix_time::ptime dtEndTime = dtStartTime + boost::posix_time::milliseconds((int64_t)dbTimeDuration);

    boost::posix_time::ptime dtUpdateStart = boost::posix_time::ptime(dtStartTime);
    boost::posix_time::ptime dtUpdateEnd = dtUpdateStart + boost::posix_time::milliseconds((int)dbUpdateTimeDuration);

    //do the first Update
    UpdateData(dtUpdateStart, dtUpdateEnd);

    //send message for all other Update
    while(dtUpdateEnd < dtEndTime){
        dtUpdateStart = boost::posix_time::ptime(dtUpdateEnd);
        dtUpdateEnd = dtUpdateStart + boost::posix_time::milliseconds((int64_t)dbUpdateTimeDuration);
        if(dtUpdateEnd > dtEndTime)
            dtUpdateEnd = dtEndTime;

        std::string strData;
        strData = "Action;UpdateData/target;" + m_strSensorID + "/startTime;" + boost::posix_time::to_simple_string(dtUpdateStart)
                + "/startTime;" + boost::posix_time::to_simple_string(dtUpdateEnd);

        SymuCom::Message* pMessage = m_pOwner->GetCommunicationRunner()->Encode(-1, m_pOwner->GetEntityID(), m_pOwner->GetEntityID(), strData, dtUpdateStart, SymuCom::Message::MT_Unicast);
        pMessage->SetTimeReceive(dtUpdateStart);

        m_pOwner->GetCommunicationRunner()->AddWaitingMessage(pMessage, m_pOwner);
    }

    return true;
}

std::string ITSSensor::GetLabel() const
{
    return m_strSensorID;
}

std::string ITSSensor::GetType() const
{
    return m_strSensorType;
}

ITSStation *ITSSensor::GetOwner() const
{
    return m_pOwner;
}

bool ITSSensor::IsDown()
{
    //For now, a sensor can't be repaired once it broke down
    if (m_bIsDown || (m_dbBreakDownRate != 0.0 && ((double)m_pNetwork->GetRandManager()->myRand() / (double)MAXIMUM_RANDOM_NUMBER) < m_dbBreakDownRate))
    {
        m_bIsDown = true;
        return true;
    }

    return false;
}

void ITSSensor::ComputePrecision(double& dbValue)
{
    double powValue = std::pow(10, -m_iPrecision);
    long double extendedValue = dbValue * powValue;
    long int truncValue = (long int)extendedValue;
    dbValue = (double)(truncValue / powValue);
}

double ITSSensor::GetNoiseRate() const
{
    return m_dbNoiseRate;
}

int ITSSensor::GetPrecision() const
{
    return m_iPrecision;
}

int ITSSensor::GetUpdateFrequency() const
{
    return m_iUpdateFrequency;
}

ITSSensorSnapshot * ITSSensor::TakeSnapshot() const
{
    ITSSensorSnapshot * pResult = new ITSSensorSnapshot();
    pResult->bIsDown = m_bIsDown;
    return pResult;
}

void ITSSensor::Restore(ITSSensorSnapshot* pSnapshot)
{
    m_bIsDown = pSnapshot->bIsDown;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Sérialisation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////

template void ITSSensorSnapshot::serialize(boost::archive::xml_woarchive & ar, const unsigned int version);
template void ITSSensorSnapshot::serialize(boost::archive::xml_wiarchive & ar, const unsigned int version);

template<class Archive>
void ITSSensorSnapshot::serialize(Archive& ar, const unsigned int version)
{
    ar & BOOST_SERIALIZATION_NVP(bIsDown);
}

template void ITSSensor::serialize(boost::archive::xml_woarchive & ar, const unsigned int version);
template void ITSSensor::serialize(boost::archive::xml_wiarchive & ar, const unsigned int version);

template<class Archive>
void ITSSensor::serialize(Archive& ar, const unsigned int version)
{
    ar & BOOST_SERIALIZATION_NVP(m_pNetwork);
    ar & BOOST_SERIALIZATION_NVP(m_strSensorID);
    ar & BOOST_SERIALIZATION_NVP(m_strSensorType);
    ar & BOOST_SERIALIZATION_NVP(m_pOwner);
    ar & BOOST_SERIALIZATION_NVP(m_iUpdateFrequency);
    ar & BOOST_SERIALIZATION_NVP(m_dbBreakDownRate);
    ar & BOOST_SERIALIZATION_NVP(m_iPrecision);
    ar & BOOST_SERIALIZATION_NVP(m_dbNoiseRate);
    ar & BOOST_SERIALIZATION_NVP(m_bIsDown);
}
