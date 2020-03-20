#pragma once 

#ifndef SYMUCOM_BMAVEHICLE_H
#define SYMUCOM_BMAVEHICLE_H

#include "ITS/Stations/ITSStation.h"
#include "symUtil.h"

#pragma warning(push)
#pragma warning(disable : 4251)

class BMAVehicle : public ITSStation{

protected:
	struct VehicleToInteract {
		std::string iIDRealVehicle;
		Point ptPos;
		double dbSpeed;

		double dbSelfTrust;
		std::map<std::string, double> mRelativeTrust; //trust in other vehicles
	};

public:
    BMAVehicle();
    BMAVehicle(Vehicule* pParent, Reseau *pNetwork, const std::string& strID, const std::string& strType, const SymuCom::Point& pPoint, SymuCom::Graph* pGraph, bool bCanSend = true, bool bCanReceive = true,
               double dbEntityRange = 10, double dbAlphaDown = -1, double dbBetaDown = -1);
    virtual ~BMAVehicle();

    virtual bool Run(const boost::posix_time::ptime &dtTimeStart, double dbTimeDuration);

    virtual bool TreatMessageData(SymuCom::Message *pMessage);

    virtual bool LoadFromXML(XERCES_CPP_NAMESPACE::DOMDocument * pDoc, XERCES_CPP_NAMESPACE::DOMNode *pXMLNode);

    virtual ITSStation* CreateCopy(const std::string& strID);

	bool GetSpeed(double & dbSpeed);

	bool GetPos(Point & dbPos);

	bool GetRelativeSpeed(double & dbRelativeSpeed);

	bool GetRelativePosition(double & dbRelativePosition);

protected:
	std::map<std::string, VehicleToInteract>	m_mVehicleInRange; //speed and trust in an other vehicle

	std::map<std::string, double>				m_mTrustToOther; //Values of trust to other connected vehicle

	double										m_dbSelfTrust; //trust in himseelf

	double										m_dbSumWeightedProximity; //sum of all weight

	double										m_dbDirectRelativeDistance; //Relative distance with the direct leader (obtained by telemetry)

private:
	double CalculDistanceWeighted(VehicleToInteract & VehInfo);

	double CalculTrustWeighted(VehicleToInteract & VehInfo);

////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Sérialisation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////
private:
    friend class boost::serialization::access;
    template<class Archive>
    void serialize(Archive & ar, const unsigned int version);
};

#pragma warning(pop)

#endif // SYMUCOM_BMAVEHICLE_H
