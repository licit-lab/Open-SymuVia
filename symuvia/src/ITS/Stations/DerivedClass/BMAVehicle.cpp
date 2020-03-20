#include "stdafx.h"
#include "BMAVehicle.h"

#include "reseau.h"
#include "Logger.h"
#include "ITS/Applications/C_ITSApplication.h"
#include "ITS/Sensors/DerivedClass/GPSSensor.h"
#include "ITS/Sensors/DerivedClass/TelemetrySensor.h"
#include "Xerces/XMLUtil.h"
#include "SystemUtil.h"

#include <Communication/CommunicationRunner.h>

XERCES_CPP_NAMESPACE_USE

BMAVehicle::BMAVehicle()
{

}

BMAVehicle::BMAVehicle(Vehicule *pParent, Reseau* pNetwork, const std::string &strID, const std::string &strType, const SymuCom::Point &pPoint, SymuCom::Graph *pGraph,
                       bool bCanSend, bool bCanReceive, double dbEntityRange, double dbAlphaDown, double dbBetaDown)
    : ITSStation(pNetwork, strID, strType, pPoint, pGraph, bCanSend, bCanReceive, dbEntityRange, dbAlphaDown, dbBetaDown)
{
    SetDynamicParent(pParent);

	m_dbSelfTrust = 1; //At start, we have total trust in ourselves
}

BMAVehicle::~BMAVehicle()
{

}

bool BMAVehicle::Run(const boost::posix_time::ptime &dtTimeStart, double dbTimeDuration)
{

	// 1) Add direct vehicle seen by telemetry
	Vehicule* pVehicle = GetDynamicParent();
	Point ptPos;
	double dbSpeed;

	//get exact speed ans pos from sensors
	if (!GetSpeed(dbSpeed))
		return false;

	if (!GetPos(ptPos))
		return false;
	//std::cout << "Vehicle ID " << pVehicle->GetID() << " has pos " << ptPos.dbX << " " << ptPos.dbY << std::endl;

	double minDistFront = DBL_MAX;
	double minDistBack = DBL_MAX;
	double dbOtherSpeedFront = -1;
	std::string strOtherVehIDFront;
	Point ptOtherPosFront;
	double dbOtherSpeedBack = -1;
	std::string strOtherVehIDBack;
	Point ptOtherPosBack;
	m_dbDirectRelativeDistance = -1;


	//To be sure we have the correct sensor
	TelemetrySensor* pTelemetry = dynamic_cast<TelemetrySensor*>(GetSensorByType("TelemetrySensor"));
	if (!pTelemetry){
		*(m_pNetwork->GetLogger()) << Logger::Error << "Error while getting speed for connected vehicle " << pVehicle->GetID() << " : No Telemetry Sensor found !" << std::endl;
		return false;
	}

	//If no Vehicle seen, nothing to add
	if (!pTelemetry->GetVehicleSeen().empty())
	{
		std::map<Vehicule*, std::pair<double, Point> > listVehSeen = pTelemetry->GetVehicleSeen();
		std::map<Vehicule*, std::pair<double, Point> >::iterator itListVehSeen = listVehSeen.begin();
		for (itListVehSeen; itListVehSeen != listVehSeen.end(); itListVehSeen++)
		{
			if (pVehicle->GetVoie(1) != itListVehSeen->first->GetVoie(1))
			{
				if (pTelemetry->IsWithinSight(itListVehSeen->second.second, true))
				{
					//other vehicle is in front of us
					if (GetDistance(ptPos, itListVehSeen->second.second) < minDistFront)
					{
						minDistFront = GetDistance(ptPos, itListVehSeen->second.second);
						strOtherVehIDFront = std::to_string(itListVehSeen->first->GetID());
						dbOtherSpeedFront = itListVehSeen->second.first;
						ptOtherPosFront = itListVehSeen->second.second;
						m_dbDirectRelativeDistance = minDistFront - itListVehSeen->first->GetLength();
					}
				}
				else{
					//other vehicle is behind us
					if (GetDistance(ptPos, itListVehSeen->second.second) < minDistBack)
					{
						minDistBack = GetDistance(ptPos, itListVehSeen->second.second);
						strOtherVehIDBack = std::to_string(itListVehSeen->first->GetID());
						dbOtherSpeedBack = itListVehSeen->second.first;
						ptOtherPosBack = itListVehSeen->second.second;
					}
				}
			}
		}
		//We add the first vehicle seen in front and in back
		if (dbOtherSpeedFront > -1)
		{
			VehicleToInteract newVehSeen;
			newVehSeen.iIDRealVehicle = strOtherVehIDFront;
			newVehSeen.dbSpeed = dbOtherSpeedFront;
			newVehSeen.ptPos = ptOtherPosFront;
			newVehSeen.dbSelfTrust = 1; //Full trust in himself
			m_mVehicleInRange.insert(std::pair<std::string, VehicleToInteract>(strOtherVehIDFront, newVehSeen));
		}
		if (dbOtherSpeedBack > -1)
		{
			/*VehicleToInteract newVehSeen;
			newVehSeen.iIDRealVehicle = strOtherVehIDBack;
			newVehSeen.dbSpeed = dbOtherSpeedBack;
			newVehSeen.ptPos = ptOtherPosBack;
			newVehSeen.dbSelfTrust = 1; //Full trust in himself
			m_mVehicleInRange.insert(std::pair<std::string, VehicleToInteract>(strOtherVehIDBack, newVehSeen));*/
		}
	}



    // 2) Send Speed and pos to any other vehicle
	//prepare message and send it
	std::string message;
	message = "Action;SpeedAndPos/VehID;" + std::to_string(pVehicle->GetID()) + "/Speed;" + std::to_string(dbSpeed) + "/PosX;" + std::to_string(ptPos.dbX) + "/PosY;" + std::to_string(ptPos.dbY) + "/PosZ;" + std::to_string(ptPos.dbZ);
	SendBroadcastMessage(message, dtTimeStart);



	// 3) Send value of Trust in all connected vehicle
	message = "Action;TrustValues/VehID;" + std::to_string(pVehicle->GetID()) + "/SelfTrust;" + std::to_string(m_dbSelfTrust);
	std::map<std::string, double>::iterator itVehTrust = m_mTrustToOther.begin();
	for (itVehTrust; itVehTrust != m_mTrustToOther.end(); itVehTrust++)
	{ 
		message += "/OtherVehID;" + itVehTrust->first + "/OtherTrust;" + std::to_string(itVehTrust->second);
	}
	SendBroadcastMessage(message, dtTimeStart);


	// map will be refill when messages from other are receive
	m_mVehicleInRange.clear();
	m_mTrustToOther.clear();
	m_dbSumWeightedProximity = -1;
	m_dbSelfTrust = -1;

    return true;
}

bool BMAVehicle::TreatMessageData(SymuCom::Message *pMessage)
{
    bool bProcessOk = false;

    std::string data = pMessage->GetData();
    std::vector<std::string> lstAction = split(data, '/');

    std::vector<std::string> lstAtIAction = split(lstAction[0], ';');
    if(lstAtIAction[0] == "Action")
    {
        //Data for SpeedAndPos must be the form Action;SpeedAndPos/VehID;iVehID/Speed;dbSpeed/PosX;dbPosX/PosY;dbPosY/PosZ;dbPosZ
        if(lstAtIAction[1] == "SpeedAndPos")
        {
			int nbElement = 1;
			std::string strVehID;
			double dbSpeed;
			Point ptPos;

			//we read the ID of the Vehicle that send the message
			std::vector<std::string> lstElement = split(lstAction[nbElement], ';');
			strVehID = lstElement[1];
			nbElement++;

			//we read the speed of the Vehicle
			lstElement = split(lstAction[nbElement], ';');
			dbSpeed = SystemUtil::ToDouble(lstElement[1]);
			nbElement++;

			//we read the X position of the Vehicle
			lstElement = split(lstAction[nbElement], ';');
			ptPos.dbX = SystemUtil::ToDouble(lstElement[1]);
			nbElement++;

			//we read the Y position of the Vehicle
			lstElement = split(lstAction[nbElement], ';');
			ptPos.dbY = SystemUtil::ToDouble(lstElement[1]);
			nbElement++;

			//we read the Z position of the Vehicle
			lstElement = split(lstAction[nbElement], ';');
			ptPos.dbZ = SystemUtil::ToDouble(lstElement[1]);
			nbElement++;

			//We verify if we already receive message from this vehicle. If not, add it
			std::map<std::string, VehicleToInteract>::iterator itMapFind = m_mVehicleInRange.find(strVehID);
			if (itMapFind == m_mVehicleInRange.end())
			{
				VehicleToInteract newVeh = m_mVehicleInRange[strVehID];
				newVeh.dbSelfTrust = -1;
				newVeh.iIDRealVehicle = strVehID;
				newVeh.dbSpeed = dbSpeed;
				newVeh.ptPos = ptPos;
			}
			else{
				itMapFind->second.iIDRealVehicle = strVehID;
				itMapFind->second.dbSpeed = dbSpeed;
				itMapFind->second.ptPos = ptPos;
			}
			bProcessOk = true;

		}
		else if (lstAtIAction[1] == "TrustValues")//Data for TrustValues must be the form Action;TrustValues/VehID;iID/SelfTrust;dbSelfTrust[/OtherVehID;iOtherID/OtherTrust;dbTrust]
		{
			int nbElement = 1;
			std::string strVehID;
			double dbSelfTrust;
			std::map<std::string, double> mRelativeTrust;
			std::string strTempID;
			double dbTempSelfTrust;

			//we read the ID of the Vehicle that send the message
			std::vector<std::string> lstElement = split(lstAction[nbElement], ';');
			strVehID = lstElement[1];
			nbElement++;

			//we read the trust value in himself
			lstElement = split(lstAction[nbElement], ';');
			dbSelfTrust = SystemUtil::ToDouble(lstElement[1]);
			nbElement++;

			//Loop to read data of all other vehicle Trust of this vehicle
			while (nbElement < lstAction.size())
			{
				//we read the ID of the other vehicle
				lstElement = split(lstAction[nbElement], ';');
				strTempID = lstElement[1];
				nbElement++;

				//we read the trust in this vehicle
				lstElement = split(lstAction[nbElement], ';');
				dbTempSelfTrust = SystemUtil::ToDouble(lstElement[1]);
				nbElement++;

				mRelativeTrust.insert(std::pair<std::string, double>(strTempID, dbTempSelfTrust));
			}

			//We verify if we already receive message from this vehicle. If not, add it
			std::map<std::string, VehicleToInteract>::iterator itMapFind = m_mVehicleInRange.find(strVehID);
			if (itMapFind == m_mVehicleInRange.end())
			{
				VehicleToInteract newVeh = m_mVehicleInRange[strVehID];
				newVeh.dbSpeed = -1;
				newVeh.iIDRealVehicle = strVehID;
				newVeh.dbSelfTrust = dbSelfTrust;
				newVeh.mRelativeTrust = mRelativeTrust;
			}
			else{
				itMapFind->second.iIDRealVehicle = strVehID;
				itMapFind->second.dbSelfTrust = dbSelfTrust;
				itMapFind->second.mRelativeTrust = mRelativeTrust;
			}

			bProcessOk = true;
		}
		else{
            *(m_pNetwork->GetLogger()) << Logger::Error << "Error while reading the message ID " << pMessage->GetMessageID() << " : the action " << lstAtIAction[1] << " is not reconized !" << std::endl;
            return false;
        }
    }

    return bProcessOk;
}


bool BMAVehicle::GetSpeed(double & dbSpeed)
{
	GPSSensor* pGPS = dynamic_cast<GPSSensor*>(GetSensorByType("GPSSensor"));
	if (!pGPS){
		*(m_pNetwork->GetLogger()) << Logger::Error << "Error while getting speed for connected vehicle " << GetDynamicParent()->GetID() << " : No GPS Sensor found !" << std::endl;
		return false;
	}
	dbSpeed = pGPS->GetSpeed();
	return true;
}


bool BMAVehicle::GetPos(Point & pPos)
{
	GPSSensor* pGPS = dynamic_cast<GPSSensor*>(GetSensorByType("GPSSensor"));
	if (!pGPS){
		*(m_pNetwork->GetLogger()) << Logger::Error << "Error while getting speed for connected vehicle " << GetDynamicParent()->GetID() << " : No GPS Sensor found !" << std::endl;
		return false;
	}
	pPos = pGPS->GetPointPositionAtEnd();
	return true;
}


bool BMAVehicle::GetRelativeSpeed(double & dbRelativeSpeed)
{
	double dbSpeed;
	GetSpeed(dbSpeed);
	double sumWeightedRelativeSpeed = 0;

	bool bAtLeastOneValue = false;
	std::map<std::string, VehicleToInteract>::iterator itListVehSeen = m_mVehicleInRange.begin();
	for (itListVehSeen; itListVehSeen != m_mVehicleInRange.end(); itListVehSeen++)
	{
		if (itListVehSeen->second.dbSpeed > 0)
		{
			bAtLeastOneValue = true;
			break;
		}
	}
	//if no vehicle around, relative speed is the one of the vehicle
	if (!bAtLeastOneValue)
	{
		GetSpeed(dbRelativeSpeed);
		return true;
	}

	//weight value between all vehicle that communicate
	itListVehSeen = m_mVehicleInRange.begin();
	for (itListVehSeen; itListVehSeen != m_mVehicleInRange.end(); itListVehSeen++)
	{
		double dbWeight = CalculDistanceWeighted(itListVehSeen->second) * CalculTrustWeighted(itListVehSeen->second);
		sumWeightedRelativeSpeed += dbWeight * (dbSpeed - itListVehSeen->second.dbSpeed);
	}

	dbRelativeSpeed = sumWeightedRelativeSpeed;

	return true;
}


bool BMAVehicle::GetRelativePosition(double & dbRelativePosition)
{
	Point ptPos;
	GetPos(ptPos);
	double sumWeightedRelativePos = 0;

	//In all case, we calcul SelfTrust
	std::string strID = std::to_string(GetDynamicParent()->GetID());
	//be sure we already calculate the self trust this timeStep
	//If not, we calculate it
	if (m_dbSelfTrust == -1)
	{
		m_dbSelfTrust = 0;
		double dbSumOtherVehSelfTrust = 0;

		std::map<std::string, double>::iterator itOtherVehTrust;
		std::map<std::string, VehicleToInteract>::iterator itListVehSeen = m_mVehicleInRange.begin();
		for (itListVehSeen; itListVehSeen != m_mVehicleInRange.end(); itListVehSeen++)
		{
			if (itListVehSeen->second.dbSelfTrust > 0)
			{
				itOtherVehTrust = itListVehSeen->second.mRelativeTrust.find(strID);
				if (itOtherVehTrust != itListVehSeen->second.mRelativeTrust.end())
				{
					m_dbSelfTrust += itOtherVehTrust->second * itListVehSeen->second.dbSelfTrust;
					dbSumOtherVehSelfTrust += itListVehSeen->second.dbSelfTrust;
				}
			}
		}
		if (dbSumOtherVehSelfTrust == 0)
		{
			m_dbSelfTrust = 1;
		}
		else{
			m_dbSelfTrust = m_dbSelfTrust / dbSumOtherVehSelfTrust;
		}
	}


	bool bAtLeastOneValue = false;
	std::map<std::string, VehicleToInteract>::iterator itListVehSeen = m_mVehicleInRange.begin();
	for (itListVehSeen; itListVehSeen != m_mVehicleInRange.end(); itListVehSeen++)
	{
		if (itListVehSeen->second.dbSpeed > 0)
		{
			bAtLeastOneValue = true;
			break;
		}
	}
	//If no Vehicle seen, relative position is equal to infinite
	if (!bAtLeastOneValue)
	{
		dbRelativePosition = DBL_MAX;
		return true;
	}

	//weight value between all vehicle that communicate
	itListVehSeen = m_mVehicleInRange.begin();
	for (itListVehSeen; itListVehSeen != m_mVehicleInRange.end(); itListVehSeen++)
	{
		double dbWeight = CalculDistanceWeighted(itListVehSeen->second) * CalculTrustWeighted(itListVehSeen->second);
		boost::shared_ptr<Vehicule> pOtherVeh = m_pNetwork->GetVehiculeFromID(SystemUtil::ToInt32(itListVehSeen->first));
		sumWeightedRelativePos += dbWeight * ((GetDistance(ptPos, itListVehSeen->second.ptPos)) - pOtherVeh->GetLength());
	}

	dbRelativePosition = sumWeightedRelativePos;

	return true;
}


double BMAVehicle::CalculDistanceWeighted(VehicleToInteract& VehInfo)
{
	double dbActualProximity;
	Point ptPos;
	GetPos(ptPos);
	double dbSpeed;
	GetSpeed(dbSpeed);

	//To be sure we have the correct sensor
	TelemetrySensor* pTelemetry = dynamic_cast<TelemetrySensor*>(GetSensorByType("TelemetrySensor"));

	//be sure we already calculate the sum of Weight this timeStep
	//If not, we calculate it
	if (m_dbSumWeightedProximity == -1)
	{
		m_dbSumWeightedProximity = 0;

		std::map<std::string, VehicleToInteract>::iterator itListVehSeen = m_mVehicleInRange.begin();
		for (itListVehSeen; itListVehSeen != m_mVehicleInRange.end(); itListVehSeen++)
		{
			if (itListVehSeen->second.dbSpeed > 0)
			{
				double dbPowValue = 0.5;
				if (pTelemetry->IsWithinSight(itListVehSeen->second.ptPos, true, true))
				{
					dbPowValue = 2;
				}
				boost::shared_ptr<Vehicule> pOtherVeh = m_pNetwork->GetVehiculeFromID(SystemUtil::ToInt32(itListVehSeen->first));
				m_dbSumWeightedProximity += (ABS(dbSpeed - itListVehSeen->second.dbSpeed) / pow((GetDistance(ptPos, itListVehSeen->second.ptPos)) - pOtherVeh->GetLength(), 2 ));
			}
		}
	}

	//we compare the proximity with this vehicle to the one of all other
	boost::shared_ptr<Vehicule> pOtherVeh = m_pNetwork->GetVehiculeFromID(SystemUtil::ToInt32(VehInfo.iIDRealVehicle));
	dbActualProximity = (ABS(dbSpeed - VehInfo.dbSpeed) / ((GetDistance(ptPos, VehInfo.ptPos)) - pOtherVeh->GetLength()));

	return dbActualProximity / m_dbSumWeightedProximity;
}


double BMAVehicle::CalculTrustWeighted(VehicleToInteract& VehInfo)
{
	double dbFinalTrust;
	Point ptPos;
	GetPos(ptPos);
	double dbSpeed;
	GetSpeed(dbSpeed);


	// 1) SelfTrust is already calculated


	// 2) We Calcul Direct Trust
	double dbDirectTrust = 1;
	if (m_dbDirectRelativeDistance != -1)
	{
		boost::shared_ptr<Vehicule> pOtherVeh = m_pNetwork->GetVehiculeFromID(SystemUtil::ToInt32(VehInfo.iIDRealVehicle));
		dbDirectTrust = ABS(m_dbDirectRelativeDistance + GetDynamicParent()->GetPos(0) - pOtherVeh->GetPos(0));
		dbDirectTrust = dbDirectTrust / (m_dbDirectRelativeDistance + GetDynamicParent()->GetPos(0));
	}


	// 3) We Calcul Indirect Trust
	double dbIndirectTrust = 0;
	double dbSumIndirectTrust = 0;

	//search a vehicle for wich we already have a trust value and who also have a trust value for the vehicle we are interrested in
	std::map<std::string, double>::iterator itThisVehTrust;
	std::map<std::string, double>::iterator itOtherVehTrust;
	std::map<std::string, VehicleToInteract>::iterator itListVehSeen = m_mVehicleInRange.begin();
	for (itListVehSeen; itListVehSeen != m_mVehicleInRange.end(); itListVehSeen++)
	{
		if (itListVehSeen->second.dbSelfTrust > 0)
		{
			itOtherVehTrust = itListVehSeen->second.mRelativeTrust.find(VehInfo.iIDRealVehicle);
			if (itOtherVehTrust != itListVehSeen->second.mRelativeTrust.end())
			{
				double dbTrustAlreadyKnow = 1;
				itThisVehTrust = m_mTrustToOther.find(itListVehSeen->first);
				if (itThisVehTrust != m_mTrustToOther.end())
				{
					dbTrustAlreadyKnow = itThisVehTrust->second;
				}
				dbIndirectTrust += dbTrustAlreadyKnow * itOtherVehTrust->second;
				dbSumIndirectTrust += dbTrustAlreadyKnow;
			}
		}
	}
	if (dbSumIndirectTrust == 0)
	{
		dbIndirectTrust = 1;
	}
	else{
		dbIndirectTrust = dbIndirectTrust / dbSumIndirectTrust;
	}


	dbFinalTrust = ((m_dbSelfTrust * dbDirectTrust) + dbIndirectTrust) / (m_dbSelfTrust + 1);
	m_mTrustToOther[VehInfo.iIDRealVehicle] = dbFinalTrust;

	return dbFinalTrust;
}


bool BMAVehicle::LoadFromXML(DOMDocument *pDoc, DOMNode *pXMLNode)
{
    //Nothing to do
    return true;
}

ITSStation *BMAVehicle::CreateCopy(const std::string& strID)
{
    BMAVehicle* pNewStation = new BMAVehicle(NULL, m_pNetwork, strID, m_strStationType, m_ptCoordinates, m_pCommunicationRunner->GetGraph(), m_bCanSend, m_bCanReceive, m_dbRange);

    return pNewStation;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Sérialisation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////
template void BMAVehicle::serialize(boost::archive::xml_woarchive & ar, const unsigned int version);
template void BMAVehicle::serialize(boost::archive::xml_wiarchive & ar, const unsigned int version);

template<class Archive>
void BMAVehicle::serialize(Archive& ar, const unsigned int version)
{
    ar & BOOST_SERIALIZATION_BASE_OBJECT_NVP(ITSStation);
}
