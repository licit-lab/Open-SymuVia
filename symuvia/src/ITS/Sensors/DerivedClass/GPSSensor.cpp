#include "stdafx.h"
#include "GPSSensor.h"

#include "tools.h"
#include "Logger.h"
#include "tuyau.h"
#include "voie.h"
#include "ITS/Stations/ITSStation.h"
#include "Xerces/XMLUtil.h"

XERCES_CPP_NAMESPACE_USE

GPSSensor::GPSSensor() : m_pLaneAtStart(NULL), m_pLaneAtEnd(NULL)
{

}

GPSSensor::GPSSensor(const GPSSensor &SensorCopy)
    : ITSDynamicSensor(SensorCopy.m_pNetwork, SensorCopy.m_strSensorID, SensorCopy.m_strSensorType, SensorCopy.GetOwner(), SensorCopy.m_iUpdateFrequency, SensorCopy.m_dbBreakDownRate,
            SensorCopy.m_iPrecision, SensorCopy.m_dbNoiseRate),
            m_pLaneAtStart(NULL), m_pLaneAtEnd(NULL)
{

}

GPSSensor::GPSSensor(Reseau * pNetwork, const std::string strID, const std::string strType, ITSStation *pOwner, int dbUpdateFrequency,
                     double dbBreakDownRate, int iPrecision, double dbNoiseRate)
    :ITSDynamicSensor(pNetwork, strID, strType, pOwner, dbUpdateFrequency, dbBreakDownRate, iPrecision, dbNoiseRate),
    m_pLaneAtStart(NULL), m_pLaneAtEnd(NULL)
{

}

GPSSensor::~GPSSensor()
{

}

bool GPSSensor::UpdateData(const boost::posix_time::ptime &dtTimeStart, boost::posix_time::ptime& dtTimeEnd)
{
    Reseau* pNetwork = m_pOwner->GetNetwork();
    if(IsDown())
    {
		*(pNetwork->GetLogger()) << Logger::Error << "Error : The Sensor " << GetLabel() << " is down and so, can't update data !" << std::endl;
		*(pNetwork->GetLogger()) << Logger::Info;
        return false;
    }

    Vehicule* pVehicule = GetParent();
    if(!pVehicule)
    {
		*(pNetwork->GetLogger()) << Logger::Error << "Error when calling UpdateData for Sensor " << GetLabel() << " : no vehicle defined !" << std::endl;
		*(pNetwork->GetLogger()) << Logger::Info;
        return false;
    }

    if(dtTimeStart > dtTimeEnd)
    {
		*(pNetwork->GetLogger()) << Logger::Error << "Error when calling UpdateData for Sensor " << GetLabel() << " : start time must be before end time !" << std::endl;
		*(pNetwork->GetLogger()) << Logger::Info;
        return false;
    }

    //Possible optimisation : if the time beetwen dtTimeStart and dtTimeEnd is equal to the TimeStep, don't need to do all of this and only get GetPos(0) and GetLink(0)
	double dbTimeDuration = (double)(((dtTimeEnd - dtTimeStart).total_microseconds()) / 1000000.0);

	if (dbTimeDuration == pNetwork->GetTimeStep())
	{
		m_pLaneAtStart = (Voie*)pVehicule->GetVoie(1);
		CalcCoordFromPos((Tuyau*)m_pLaneAtStart->GetParent(), pVehicule->GetPos(1), m_ptPositionAtStart);

		m_pLaneAtEnd = (Voie*)pVehicule->GetVoie(0);
		double dbPosVeh = pVehicule->GetPos(0);
		if (!m_pLaneAtEnd){
			m_pLaneAtEnd = GetExactPosition(dtTimeEnd, dbPosVeh);
		}
		CalcCoordFromPos((Tuyau*)m_pLaneAtEnd->GetParent(), dbPosVeh, m_ptPositionAtEnd);
		m_dbSpeed = pVehicule->GetVit(0);
		ComputePrecision(m_dbSpeed);

		m_pOwner->SetCoordinates(m_ptPositionAtEnd.dbX, m_ptPositionAtEnd.dbY, m_ptPositionAtEnd.dbZ);
		return true;
	}

    //get the position at start
	double dbPosVeh;
    Voie* pLaneStart = GetExactPosition(dtTimeStart, dbPosVeh);
    if (!pLaneStart)
		return false; 
    CalcCoordFromPos((Tuyau*)pLaneStart->GetParent(), dbPosVeh, m_ptPositionAtStart);
    m_pLaneAtStart = pLaneStart;
    m_dbPositionAtStart = dbPosVeh;

    //get the position at the end to determine speed
    Voie* pLaneEnd = GetExactPosition(dtTimeEnd, dbPosVeh);
    if (!pLaneEnd)
		return false;
    CalcCoordFromPos((Tuyau*)pLaneEnd->GetParent(), dbPosVeh, m_ptPositionAtEnd);
    m_pLaneAtEnd = pLaneEnd;
    m_dbPositionAtEnd = dbPosVeh;

    m_dbSpeed = m_ptPositionAtStart.DistanceTo(m_ptPositionAtEnd) / dbTimeDuration;
	//m_dbSpeed = pVehicule->GetVit(0);
    ComputePrecision(m_dbSpeed);

	m_pOwner->SetCoordinates(m_ptPositionAtEnd.dbX, m_ptPositionAtEnd.dbY, m_ptPositionAtEnd.dbZ);

    return true;
}

bool GPSSensor::LoadFromXML(DOMDocument *pDoc, DOMNode *pXMLNode)
{
    //Nothing to do
    return true;
}

ITSSensor *GPSSensor::CreateCopy(ITSStation *pNewOwner)
{
    GPSSensor * pNewSensor = new GPSSensor(m_pNetwork, m_strSensorID, m_strSensorType, pNewOwner, m_iUpdateFrequency, m_dbBreakDownRate, m_iPrecision, m_dbNoiseRate);
    pNewSensor->m_ptPositionAtStart = m_ptPositionAtStart;
    pNewSensor->m_dbSpeed = m_dbSpeed;
    pNewSensor->m_dbPositionAtStart = m_dbPositionAtStart;
    pNewSensor->m_pLaneAtStart = m_pLaneAtStart;
	pNewSensor->m_ptPositionAtEnd = m_ptPositionAtEnd;
	pNewSensor->m_dbPositionAtEnd = m_dbPositionAtEnd;
    pNewSensor->m_pLaneAtEnd = m_pLaneAtEnd;
    return pNewSensor;
}

Point GPSSensor::GetPointPosition() const
{
    return m_ptPositionAtStart;
}

double GPSSensor::GetSpeed() const
{
    return m_dbSpeed;
}

double GPSSensor::GetDoublePosition() const
{
    return m_dbPositionAtStart;
}

Voie *GPSSensor::GetLaneAtStart() const
{
    return m_pLaneAtStart;
}

Voie *GPSSensor::GetLaneAtEnd() const
{
    return m_pLaneAtEnd;
}

double GPSSensor::GetDoublePositionAtEnd() const
{
    return m_dbPositionAtEnd;
}

Point GPSSensor::GetPointPositionAtEnd() const
{
    return m_ptPositionAtEnd;
}

template void GPSSensor::serialize(boost::archive::xml_woarchive & ar, const unsigned int version);
template void GPSSensor::serialize(boost::archive::xml_wiarchive & ar, const unsigned int version);

template<class Archive>
void GPSSensor::serialize(Archive & ar, const unsigned int version)
{
    ar & BOOST_SERIALIZATION_BASE_OBJECT_NVP(ITSDynamicSensor);

    ar & BOOST_SERIALIZATION_NVP(m_dbSpeed);
    ar & BOOST_SERIALIZATION_NVP(m_dbPositionAtStart);
	ar & BOOST_SERIALIZATION_NVP(m_ptPositionAtStart);
	ar & BOOST_SERIALIZATION_NVP(m_dbPositionAtEnd);
	ar & BOOST_SERIALIZATION_NVP(m_ptPositionAtEnd);
    ar & BOOST_SERIALIZATION_NVP(m_pLaneAtStart);
    ar & BOOST_SERIALIZATION_NVP(m_pLaneAtEnd);
}
