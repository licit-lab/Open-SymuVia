#include "stdafx.h"
#include "InertialSensor.h"

#include "Logger.h"
#include "voie.h"
#include "ITS/Stations/ITSStation.h"
#include "Xerces/XMLUtil.h"

XERCES_CPP_NAMESPACE_USE

InertialSensor::InertialSensor()
{

}

InertialSensor::InertialSensor(const InertialSensor &SensorCopy)
    : ITSDynamicSensor(SensorCopy.m_pNetwork, SensorCopy.m_strSensorID, SensorCopy.m_strSensorType, SensorCopy.GetOwner(), SensorCopy.m_iUpdateFrequency, SensorCopy.m_dbBreakDownRate,
            SensorCopy.m_iPrecision, SensorCopy.m_dbNoiseRate)
{

}

InertialSensor::InertialSensor(Reseau * pNetwork, const std::string strID, const std::string strType, ITSStation *pOwner, int dbUpdateFrequency,
                               double dbBreakDownRate, int iPrecision, double dbNoiseRate)
    :ITSDynamicSensor(pNetwork, strID, strType, pOwner, dbUpdateFrequency, dbBreakDownRate, iPrecision, dbNoiseRate)
{

}

InertialSensor::~InertialSensor()
{

}

bool InertialSensor::UpdateData(const boost::posix_time::ptime &dtTimeStart, boost::posix_time::ptime& dtTimeEnd)
{
    Reseau* pNetwork = m_pOwner->GetNetwork();
    if(IsDown())
    {
        *(pNetwork->GetLogger()) << Logger::Error << "Error : The Sensor " << GetLabel() << " is down and so, can't update data !" << std::endl;
        return false;
    }

    Vehicule* pVehicule = GetParent();
    if(!pVehicule)
    {
        *(pNetwork->GetLogger()) << Logger::Error << "Error : no vehicle defined for " << m_pOwner->GetLabel() << " !" << std::endl;
        return false;
    }

    if(dtTimeStart > dtTimeEnd)
    {
        *(pNetwork->GetLogger()) << Logger::Error << "Error when calling UpdateData for Sensor " << GetLabel() << " : start time must be before end time !" << std::endl;
        return false;
    }

    //rmk : could be improved by verified if there is an GPS sensor and directly take informations of position and speed
    //get the position at start
    Point ptStartPos, ptEndPos;
    double dbPosVeh;
    Voie* pLaneStart = GetExactPosition(dtTimeStart, dbPosVeh);
    if(!pLaneStart)
        return false;
    CalcCoordFromPos((Tuyau*)pLaneStart->GetParent(), dbPosVeh, ptStartPos);

    //get the position at the end to determine speed
    Voie* pLaneEnd = GetExactPosition(dtTimeEnd, dbPosVeh);
    if(!pLaneEnd)
        return false;
    CalcCoordFromPos((Tuyau*)pLaneEnd->GetParent(), dbPosVeh, ptEndPos);

    //calcul orientation
    m_ptOrientation.dbX = (ptEndPos.dbX - ptStartPos.dbX);
    m_ptOrientation.dbY = (ptEndPos.dbY - ptStartPos.dbY);
    m_ptOrientation.dbZ = (ptEndPos.dbZ - ptStartPos.dbZ);

    double lenghtOrientation = sqrt(m_ptOrientation.dbX*m_ptOrientation.dbX + m_ptOrientation.dbY*m_ptOrientation.dbY + m_ptOrientation.dbZ*m_ptOrientation.dbZ);

    //normalize the vector
    m_ptOrientation.dbX = m_ptOrientation.dbX / lenghtOrientation;
    ComputePrecision(m_ptOrientation.dbX);
    m_ptOrientation.dbY = m_ptOrientation.dbY / lenghtOrientation;
    ComputePrecision(m_ptOrientation.dbY);
    m_ptOrientation.dbZ = m_ptOrientation.dbZ / lenghtOrientation;
    ComputePrecision(m_ptOrientation.dbZ);

    //Calcul acceleration
    Point ptAt1, ptAt0;
    CalcCoordFromPos(pVehicule->GetLink(1), pVehicule->GetPos(1), ptAt1);
    CalcCoordFromPos(pVehicule->GetLink(0), pVehicule->GetPos(0), ptAt0);

    double dbTimeStartStepToStartUpdate = std::fmod((double)(((dtTimeStart - pNetwork->GetSimulationStartTime()).total_microseconds()) / 1000000.0), pNetwork->GetTimeStep());
    double dbTimeEndStepToEndUpdate = pNetwork->GetTimeStep() - (double)(((dtTimeEnd - dtTimeStart).total_microseconds()) / 1000000.0) - dbTimeStartStepToStartUpdate;
    double dbVitStart = ptAt1.DistanceTo(ptStartPos) / dbTimeStartStepToStartUpdate;
    double dbVitEnd = ptEndPos.DistanceTo(ptAt0) / dbTimeEndStepToEndUpdate;
    m_dbAcceleration = (dbVitEnd - dbVitStart) / pNetwork->GetTimeStep();
    ComputePrecision(m_dbAcceleration);

    return true;
}

bool InertialSensor::LoadFromXML(DOMDocument *pDoc, DOMNode *pXMLNode)
{
    //Nothing to do
    return true;
}

ITSSensor *InertialSensor::CreateCopy(ITSStation *pNewOwner)
{
    InertialSensor * pNewSensor = new InertialSensor(m_pNetwork, m_strSensorID, m_strSensorType, pNewOwner, m_iUpdateFrequency, m_dbBreakDownRate, m_iPrecision, m_dbNoiseRate);
    pNewSensor->m_dbAcceleration = m_dbAcceleration;
    pNewSensor->m_ptOrientation = m_ptOrientation;
    return pNewSensor;
}

Point InertialSensor::GetOrientation() const
{
    return m_ptOrientation;
}

double InertialSensor::GetAcceleration() const
{
    return m_dbAcceleration;
}

template void InertialSensor::serialize(boost::archive::xml_woarchive & ar, const unsigned int version);
template void InertialSensor::serialize(boost::archive::xml_wiarchive & ar, const unsigned int version);

template<class Archive>
void InertialSensor::serialize(Archive & ar, const unsigned int version)
{
    ar & BOOST_SERIALIZATION_BASE_OBJECT_NVP(ITSDynamicSensor);

    ar & BOOST_SERIALIZATION_NVP(m_dbAcceleration);
    ar & BOOST_SERIALIZATION_NVP(m_ptOrientation);
}
