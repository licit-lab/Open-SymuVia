#include "stdafx.h"
#include "TelemetrySensor.h"

#include "voie.h"
#include "tuyau.h"
#include "Logger.h"
#include "ControleurDeFeux.h"
#include "ITS/Stations/ITSStation.h"
#include "Xerces/XMLUtil.h"

#include <boost/serialization/vector.hpp>

XERCES_CPP_NAMESPACE_USE

TelemetrySensor::TelemetrySensor()
{

}

TelemetrySensor::TelemetrySensor(const TelemetrySensor &SensorCopy)
    : ITSDynamicSensor(SensorCopy.m_pNetwork, SensorCopy.m_strSensorID, SensorCopy.m_strSensorType, SensorCopy.GetOwner(), SensorCopy.m_iUpdateFrequency, SensorCopy.m_dbBreakDownRate,
            SensorCopy.m_iPrecision, SensorCopy.m_dbNoiseRate), m_iDistanceView(SensorCopy.m_iDistanceView), m_dbAngle(SensorCopy.m_dbAngle),
            m_bForward(SensorCopy.m_bForward), m_bBackward(SensorCopy.m_bBackward)
{

}

TelemetrySensor::TelemetrySensor(Reseau * pNetwork, const std::string strID, const std::string strType, ITSStation *pOwner, int dbUpdateFrequency, double dbBreakDownRate,
                                 int iPrecision, double dbNoiseRate)
    :ITSDynamicSensor(pNetwork, strID, strType, pOwner, dbUpdateFrequency, dbBreakDownRate, iPrecision, dbNoiseRate)
{

}

TelemetrySensor::~TelemetrySensor()
{

}

bool TelemetrySensor::LoadFromXML(DOMDocument *pDoc, DOMNode *pXMLNode)
{
    GetXmlAttributeValue(pXMLNode, "look_forward", m_bForward, m_pOwner->GetNetwork()->GetLogger());

    GetXmlAttributeValue(pXMLNode, "look_backward", m_bBackward, m_pOwner->GetNetwork()->GetLogger());
    if(!m_bForward && !m_bBackward)
    {
        *m_pOwner->GetNetwork()->GetLogger() << Logger::Warning << "Telemetry sensor should at least be able to see forward or backward. Forward is take by default." << std::endl;
        return false;
    }

    GetXmlAttributeValue(pXMLNode, "angle", m_dbAngle, m_pOwner->GetNetwork()->GetLogger());

    GetXmlAttributeValue(pXMLNode, "distance_view", m_iDistanceView, m_pOwner->GetNetwork()->GetLogger());

    return true;
}

ITSSensor *TelemetrySensor::CreateCopy(ITSStation *pNewOwner)
{
    TelemetrySensor * pNewSensor = new TelemetrySensor(m_pNetwork, m_strSensorID, m_strSensorType, pNewOwner, m_iUpdateFrequency, m_dbBreakDownRate, m_iPrecision, m_dbNoiseRate);
    pNewSensor->m_iDistanceView = m_iDistanceView;
    pNewSensor->m_dbAngle = m_dbAngle;
    pNewSensor->m_bForward = m_bForward;
    pNewSensor->m_bBackward = m_bBackward;
    return pNewSensor;
}

bool TelemetrySensor::UpdateData(const boost::posix_time::ptime &dtTimeStart, boost::posix_time::ptime& dtTimeEnd)
{
    m_lstVehicleSeen.clear();
    m_lstStaticElementsSeen.clear();

    Reseau* pNetwork = m_pOwner->GetNetwork();
    if(IsDown())
    {
        *(pNetwork->GetLogger()) << Logger::Error << "Error : The Sensor " << GetLabel() << " is down and so, can't update data !" << std::endl;
        return false;
    }

    Vehicule* pVehicle = GetParent();
    if(!pVehicle)
    {
        *(pNetwork->GetLogger()) << Logger::Error << "Error when calling UpdateData for Sensor " << GetLabel() << " : no vehicle defined !" << std::endl;
        return false;
    }

    if(dtTimeStart > dtTimeEnd)
    {
        *(pNetwork->GetLogger()) << Logger::Error << "Error when calling UpdateData for Sensor " << GetLabel() << " : start time must be before end time !" << std::endl;
        return false;
    }

    std::vector<boost::shared_ptr<Vehicule>> lstVehicles = pNetwork->m_LstVehicles;
    std::vector<boost::shared_ptr<Vehicule>> lstVehPossiblyInRange;

    //calcul actual position and orientation of the vehicle
    //rmk : could be improved by verified if there is an GPS sensor and directly take informations of position and speed
    //get the position at start
    double dbPosVeh;
    Voie* pLaneStart = GetExactPosition(dtTimeStart, dbPosVeh);
    if(!pLaneStart)
        return false;
    CalcCoordFromPos((Tuyau*)pLaneStart->GetParent(), dbPosVeh, m_ptVehiclePosition);

    //get the position at the end to determine speed
    Point ptEndPos;
    Voie* pLaneEnd = GetExactPosition(dtTimeEnd, dbPosVeh);
    if(!pLaneEnd)
        return false;
    CalcCoordFromPos((Tuyau*)pLaneEnd->GetParent(), dbPosVeh, ptEndPos);

    //calcul orientation
    m_ptVehicleOrientation.dbX = (ptEndPos.dbX - m_ptVehiclePosition.dbX);
    m_ptVehicleOrientation.dbY = (ptEndPos.dbY - m_ptVehiclePosition.dbY);
    m_ptVehicleOrientation.dbZ = (ptEndPos.dbZ - m_ptVehiclePosition.dbZ);

    double lenghtOrientation = sqrt(m_ptVehicleOrientation.dbX*m_ptVehicleOrientation.dbX + m_ptVehicleOrientation.dbY*m_ptVehicleOrientation.dbY + m_ptVehicleOrientation.dbZ*m_ptVehicleOrientation.dbZ);

    //normalize the vector
    m_ptVehicleOrientation.dbX = m_ptVehicleOrientation.dbX / lenghtOrientation;
    ComputePrecision(m_ptVehicleOrientation.dbX);
    m_ptVehicleOrientation.dbY = m_ptVehicleOrientation.dbY / lenghtOrientation;
    ComputePrecision(m_ptVehicleOrientation.dbY);
    m_ptVehicleOrientation.dbZ = m_ptVehicleOrientation.dbZ / lenghtOrientation;
    ComputePrecision(m_ptVehicleOrientation.dbZ);

    //first, we do a rapid selection of only eligible vehicle
    Point ptAmont, ptAval;
	Tuyau* pTuyau;
    boost::shared_ptr<Vehicule> pOtherVehicle;
    for(size_t i=0; i<lstVehicles.size(); i++) //Find all vehicles that may be in range using Tuyau positions
    {
        pOtherVehicle = lstVehicles[i];
        pTuyau = pOtherVehicle->GetLink(0);
		if (!pTuyau)
			continue;

        ptAmont.dbX = pTuyau->GetAbsAmont();
        ptAmont.dbY = pTuyau->GetOrdAmont();
        ptAmont.dbZ = pTuyau->GetHautAmont();

        if(GetDistance(ptAmont, m_ptVehiclePosition) - pOtherVehicle->GetPos(0) > m_iDistanceView)
            continue;

        ptAval.dbX = pTuyau->GetAbsAval();
        ptAval.dbY = pTuyau->GetOrdAval();
        ptAval.dbZ = pTuyau->GetHautAval();

        if(GetDistance(ptAval, m_ptVehiclePosition) - (pTuyau->GetLength() - pOtherVehicle->GetPos(0)) > m_iDistanceView)
            continue;

        lstVehPossiblyInRange.push_back(pOtherVehicle);
    }

    //Then we calculate exact positions of vehicles and actually find if they can be seen
    for(size_t i=0; i<lstVehPossiblyInRange.size(); i++)
    {
        pOtherVehicle = lstVehPossiblyInRange[i];
        Point ptOtherVehPos;
        CalcCoordFromPos(pOtherVehicle->GetLink(0), pOtherVehicle->GetPos(0), ptOtherVehPos);
		if (IsWithinSight(ptOtherVehPos)){
			double dbVehSpeed = pOtherVehicle->GetVit(0);
			double dbVehDist = GetDistance(m_ptVehiclePosition, ptOtherVehPos);
			ComputePrecision(dbVehSpeed);
			ComputePrecision(dbVehDist);
			
			m_lstVehicleSeen.insert(std::pair<Vehicule*, std::pair<double, Point>>(pOtherVehicle.get(), std::pair<double, Point>(dbVehSpeed, ptOtherVehPos)));
		}
    }

    for(size_t i=0; i<pNetwork->GetLstControleursDeFeux().size(); i++)
    {
        ControleurDeFeux* pControleur = pNetwork->GetLstControleursDeFeux()[i];

        //ControleurDeFeux is within sight if at least one ligneDeFeux is
        Point ptLignePose;
        bool bControleurIsSeen = false;
        for(size_t j=0; j<pControleur->GetLstCoupleEntreeSortie()->size() && !bControleurIsSeen; j++)
        {
            CoupleEntreeSortie Couple = (*pControleur->GetLstCoupleEntreeSortie())[j];
            Voie* pVoie;
            for(size_t k=0; k<Couple.pEntree->GetLstLanes().size() && !bControleurIsSeen; k++)
            {
                pVoie = Couple.pEntree->GetLstLanes()[k];
                for(size_t l=0; l<pVoie->GetLignesFeux().size() && !bControleurIsSeen; l++)
                {
                    LigneDeFeu ligne = pVoie->GetLignesFeux()[l];
                    if(ligne.m_pControleur == pControleur)
                    {
                        if(ligne.m_dbPosition < 0)
                            CalcCoordFromPos(Couple.pEntree, Couple.pEntree->GetLength() + ligne.m_dbPosition, ptLignePose);
                        else
                            CalcCoordFromPos(Couple.pSortie, ligne.m_dbPosition, ptLignePose);

                        if(IsWithinSight(ptLignePose))
                        {
                            bControleurIsSeen = true;
                            m_lstStaticElementsSeen.push_back(pControleur);
                            break;
                        }
                    }
                }
            }
        }
    }

    return true;
}

bool TelemetrySensor::IsWithinSight(Point ptOtherPoint, bool bOnlyForward, bool bUseRangeInstead)
{
	int iDistanceView = m_iDistanceView;
	if (bUseRangeInstead)
	{
		iDistanceView = int(GetParent()->GetConnectedVehicle()->GetRange());
	}

	if (GetDistance(m_ptVehiclePosition, ptOtherPoint) > iDistanceView)
        return false;

    Point ptVecToOtherPoint;
    ptVecToOtherPoint.dbX = ptOtherPoint.dbX - m_ptVehiclePosition.dbX;
    ptVecToOtherPoint.dbY = ptOtherPoint.dbY - m_ptVehiclePosition.dbY;
    ptVecToOtherPoint.dbZ = ptOtherPoint.dbZ - m_ptVehiclePosition.dbZ;

    double lenghtToOtherVeh = sqrt(ptVecToOtherPoint.dbX*ptVecToOtherPoint.dbX + ptVecToOtherPoint.dbY*ptVecToOtherPoint.dbY + ptVecToOtherPoint.dbZ*ptVecToOtherPoint.dbZ);
    ComputePrecision(lenghtToOtherVeh);

    if(m_bForward)
    {
        Point ptVehToForward;
		ptVehToForward.dbX = (m_ptVehicleOrientation.dbX * iDistanceView);
		ptVehToForward.dbY = (m_ptVehicleOrientation.dbY * iDistanceView);
		ptVehToForward.dbZ = (m_ptVehicleOrientation.dbZ * iDistanceView);

        double dotProduct = (ptVehToForward.dbX * ptVecToOtherPoint.dbX) + (ptVehToForward.dbY * ptVecToOtherPoint.dbY) + (ptVehToForward.dbZ * ptVecToOtherPoint.dbZ);
        double lenghtToForward = sqrt(ptVehToForward.dbX*ptVehToForward.dbX + ptVehToForward.dbY*ptVehToForward.dbY + ptVehToForward.dbZ*ptVehToForward.dbZ);
        ComputePrecision(lenghtToForward);
        double angle = acos(dotProduct / (lenghtToOtherVeh * lenghtToForward));
        if(angle < m_dbAngle/2 || (angle-360) > m_dbAngle/(-2))
            return true;
    }

    if(m_bBackward && !bOnlyForward)
    {
        Point ptVehToBackward;
		ptVehToBackward.dbX = (m_ptVehicleOrientation.dbX * (-iDistanceView));
		ptVehToBackward.dbY = (m_ptVehicleOrientation.dbY * (-iDistanceView));
		ptVehToBackward.dbZ = (m_ptVehicleOrientation.dbZ * (-iDistanceView));

        double dotProduct = (ptVehToBackward.dbX * ptVecToOtherPoint.dbX) + (ptVehToBackward.dbY * ptVecToOtherPoint.dbY) + (ptVehToBackward.dbZ * ptVecToOtherPoint.dbZ);
        double lenghtToBackward = sqrt(ptVehToBackward.dbX*ptVehToBackward.dbX + ptVehToBackward.dbY*ptVehToBackward.dbY + ptVehToBackward.dbZ*ptVehToBackward.dbZ);
        ComputePrecision(lenghtToBackward);
        double angle = acos(dotProduct / (lenghtToOtherVeh * lenghtToBackward));
        if(angle < m_dbAngle/2 || (angle-360) > m_dbAngle/(-2))
            return true;
    }

    return false;
}

const std::vector<StaticElement *> &TelemetrySensor::GetStaticElementsSeen() const
{
    return m_lstStaticElementsSeen;
}

const std::map<Vehicule*, std::pair<double, Point> > &TelemetrySensor::GetVehicleSeen() const
{
    return m_lstVehicleSeen;
}

template void TelemetrySensor::serialize(boost::archive::xml_woarchive & ar, const unsigned int version);
template void TelemetrySensor::serialize(boost::archive::xml_wiarchive & ar, const unsigned int version);

template<class Archive>
void TelemetrySensor::serialize(Archive & ar, const unsigned int version)
{
    ar & BOOST_SERIALIZATION_BASE_OBJECT_NVP(ITSDynamicSensor);

    ar & BOOST_SERIALIZATION_NVP(m_lstVehicleSeen);
    ar & BOOST_SERIALIZATION_NVP(m_lstStaticElementsSeen);
    ar & BOOST_SERIALIZATION_NVP(m_iDistanceView);
    ar & BOOST_SERIALIZATION_NVP(m_dbAngle);
    ar & BOOST_SERIALIZATION_NVP(m_bForward);
    ar & BOOST_SERIALIZATION_NVP(m_bBackward);
    ar & BOOST_SERIALIZATION_NVP(m_ptVehiclePosition);
    ar & BOOST_SERIALIZATION_NVP(m_ptVehicleOrientation);
}

