#include "stdafx.h"
#include "ITSDynamicSensor.h"

#include "reseau.h"
#include "Logger.h"
#include "voie.h"
#include "tuyau.h"
#include "ITS/Stations/ITSStation.h"

ITSDynamicSensor::ITSDynamicSensor()
{

}

ITSDynamicSensor::ITSDynamicSensor(const ITSDynamicSensor &SensorCopy)
: ITSSensor(SensorCopy.m_pNetwork, SensorCopy.m_strSensorID, SensorCopy.m_strSensorType, SensorCopy.GetOwner(), SensorCopy.m_iUpdateFrequency, SensorCopy.m_dbBreakDownRate,
            SensorCopy.m_iPrecision, SensorCopy.m_dbNoiseRate)
{

}

ITSDynamicSensor::ITSDynamicSensor(Reseau * pNetwork, const std::string strID, const std::string strType, ITSStation *pOwner, int dbUpdateFrequency,
                                   double dbBreakDownRate, int iPrecision, double dbNoiseRate)
        :ITSSensor(pNetwork, strID, strType, pOwner, dbUpdateFrequency, dbBreakDownRate, iPrecision, dbNoiseRate)
{

}

ITSDynamicSensor::~ITSDynamicSensor()
{

}

Vehicule *ITSDynamicSensor::GetParent() const
{
    Vehicule *pDynamicParent = m_pOwner->GetDynamicParent();
    if(!pDynamicParent)
        *(m_pOwner->GetNetwork()->m_pLogger) << Logger::Warning << "Warning : Sensor " << m_strSensorID << " is type " << m_strSensorType << " but is not owned by a dynamic station !" << std::endl;

    return pDynamicParent;
}

//redo a variation of CalculPosition to get actual position after dbTimeStart
Voie* ITSDynamicSensor::GetExactPosition(const boost::posix_time::ptime &dtTimeStart, double& dbPos)
{
    //rmk : Pour l'instant, il s'agit simplement d'une interpolation de la position en se basant sur la position du véhicule au début et à la fin du pas de temps
    Reseau* pNetwork = m_pOwner->GetNetwork();
    Vehicule* pVehicule = GetParent();
    if(!pVehicule)
    {
        *(pNetwork->GetLogger()) << Logger::Error << "Error : no vehicle defined for " << m_pOwner->GetLabel() << " !" << std::endl;
        return NULL;
    }

	//double dbTimeStart = (double)(((dtTimeStart - pNetwork->GetSimulationStartTime()).total_microseconds()) / 1000000.0);
    double dbTimeStart = std::fmod((double)(((dtTimeStart - pNetwork->GetSimulationStartTime()).total_microseconds()) / 1000000.0), pNetwork->GetTimeStep());

    double dbPosVeh = (pVehicule->GetPos(1) + pVehicule->GetVit(0) * dbTimeStart);  // Position after dbTimeStart, starting at pVehicule->GetPos(1) on pVehicule->GetVoie(1)
    ComputePrecision(dbPosVeh);

    Voie* pStartTuyau = (Voie*)pVehicule->GetVoie(1);
    if(dbPosVeh <= pVehicule->GetVoie(1)->GetLength())//still on the same lane
    {
		dbPos = dbPosVeh;
        ComputePrecision(dbPos);
        return pStartTuyau;
	}
    else{
        std::deque<Voie*>::iterator itCurent = pVehicule->m_LstUsedLanes.begin();

        dbPosVeh = dbPosVeh - pVehicule->GetVoie(1)->GetLength();

		for (itCurent; itCurent != pVehicule->m_LstUsedLanes.end(); itCurent++)
        {
            if( dbPosVeh <= (*itCurent)->GetLength() )
            {
				dbPos = dbPosVeh;
                ComputePrecision(dbPos);
				return (*itCurent);
            }
            else
            {
                dbPosVeh -= (*itCurent)->GetLength();
            }
        }
        //the vehicle is in the last lane, unless the vehicle went out of the network
        VoieMicro * pLastLane = pVehicule->GetVoie(0);
        if (pLastLane) {
            dbPos = std::min<double>(dbPosVeh, pLastLane->GetLength());
            ComputePrecision(dbPos);
            return pLastLane;
        }
    }

    return NULL;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Sérialisation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////
template void ITSDynamicSensor::serialize(boost::archive::xml_woarchive & ar, const unsigned int version);
template void ITSDynamicSensor::serialize(boost::archive::xml_wiarchive & ar, const unsigned int version);

template<class Archive>
void ITSDynamicSensor::serialize(Archive& ar, const unsigned int version)
{
    ar & BOOST_SERIALIZATION_BASE_OBJECT_NVP(ITSSensor);
}
