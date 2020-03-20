#include "stdafx.h"
#include "GLOSA.h"

#include "Logger.h"
#include "voie.h"
#include "tuyau.h"
#include "DiagFonda.h"
#include "ITS/Stations/ITSStation.h"
#include "ITS/Stations/DerivedClass/ITSVehicle.h"
#include "ITS/Sensors/DerivedClass/GPSSensor.h"
#include "ITS/Sensors/DerivedClass/InertialSensor.h"
#include "Xerces/XMLUtil.h"
#include "SystemUtil.h"

#include <Communication/Message.h>
#include <Communication/Entity.h>

#include "boost/date_time/posix_time/posix_time.hpp"

XERCES_CPP_NAMESPACE_USE

GLOSA::GLOSA()
{

}

GLOSA::GLOSA(const std::string strID, const std::string strType) : C_ITSApplication(strID, strType)
{

}

GLOSA::~GLOSA()
{

}

bool isSame(Tuyau * pTuyau, void * pArg)
{
    Tuyau * pTuyauRef = (Tuyau*)pArg;
    return pTuyauRef == pTuyau;
}

//Variation of GetTempsDistance with more accurate position and speed values
double GetTimeDistance(Reseau* pNetwork, Vehicule* pVehicle, const boost::posix_time::ptime& dtActualTime, GPSSensor* pGPS, InertialSensor* pInertial, double dbDistanceToReach)
{
    double dbTimeStep = pNetwork->GetTimeStep();
    double dbInst = (int)(((dtActualTime - pNetwork->GetSimulationStartTime()).total_milliseconds()) / 1000) / dbTimeStep;
    double dbTimeToReach = 0.0;
    double dbDstTo;
    int nbTimeStep;
    double dbVitMax, dbAccMax, dbVit, dbDstTmp;

    // Case of guided vehicle with stop
    double dbDstArret;
    TripNode * pNextTripNode;

    dbDstArret = pVehicle->GetNextTripNodeDistance(0, &pNextTripNode);

    if(pNextTripNode->GetInputPosition().HasPosition() && dbDstArret<dbDistanceToReach) // the stop is before the position
    {
        return pVehicle->CalculTempsAvecArret(dbInst, dbTimeStep, 0, (Tuyau*)pGPS->GetLaneAtStart()->GetParent(), pGPS->GetDoublePosition() + dbDistanceToReach, pNextTripNode);
    }

    // fluid flow aval
    if( pVehicle->IsRegimeFluide())
    {
        bool isAccelerating = false;
        if(pInertial)
            isAccelerating = pInertial->GetAcceleration() > 0;
        else{
            isAccelerating = (pGPS->GetSpeed() >= pVehicle->GetVit(1) && pGPS->GetSpeed() < pVehicle->GetVit(0)) || (pGPS->GetSpeed() > pVehicle->GetVit(1) && pGPS->GetSpeed() <= pVehicle->GetVit(0));
        }

        if( isAccelerating)
        {
            dbAccMax = pVehicle->GetAccMax(pGPS->GetSpeed());
			dbVitMax = std::min<double>(pVehicle->GetDiagFonda()->GetVitesseLibre(), ((Tuyau*)pGPS->GetLaneAtStart()->GetParent())->GetVitRegByTypeVeh(pVehicle->GetType(), dbInst, pGPS->GetDoublePosition(), pGPS->GetLaneAtStart()->GetNum()));

            // Number of time step needed to reach free speed
            nbTimeStep = (int)ceill(  ( dbVitMax - pGPS->GetSpeed() ) / (dbAccMax * dbTimeStep));

            // Distance needed to reach free speed
            dbDstTo = nbTimeStep * pGPS->GetSpeed() * dbTimeStep + nbTimeStep * (nbTimeStep+1) / 2 * dbAccMax * dbTimeStep * dbTimeStep;

            if( dbDstTo < dbDistanceToReach  )    // The free speed is reach before dbDistanceToReach
                dbTimeToReach = dbTimeStep * ( nbTimeStep + floor( dbDistanceToReach / (dbVitMax * dbTimeStep)));
            else // The vehicle is still accelerating when he reach dbDistanceToReach
            {
                dbTimeToReach = 0;
                dbVit = std::min<double>( dbVitMax, pGPS->GetSpeed() + (pVehicle->GetAccMax(pGPS->GetSpeed()) * dbTimeStep));
                dbDstTmp = dbVit*dbTimeStep;
                while(dbDstTmp <= dbDistanceToReach)
                {
                    dbVit = std::min<double>( dbVitMax, dbVit + pVehicle->GetAccMax(dbVit) * dbTimeStep);
                    dbDstTmp += dbVit*dbTimeStep;
                    dbTimeToReach += dbTimeStep;
                }
                dbTimeToReach += (dbDstTmp - dbDistanceToReach) / dbVit;
            }
        }
        else
        {
            if( pGPS->GetSpeed() > 0)
                dbTimeToReach = dbDistanceToReach / pGPS->GetSpeed();
            else
                dbTimeToReach = DBL_MAX;
        }
    }
    else    // Congested flow
    {
        if(pGPS->GetSpeed()>0)
            dbTimeToReach = dbDistanceToReach / pGPS->GetSpeed();
        else
            dbTimeToReach = DBL_MAX;
    }
    return dbTimeToReach;

}

bool GLOSA::RunApplication(SymuCom::Message *pMessage, ITSStation *pStation)
{
    if(!pMessage)//GLOSA can only be activated with a message
        return true;

    if(!pStation)
        return false;

    Reseau* pNetwork = pStation->GetNetwork();
    Vehicule* pVehicle = pStation->GetDynamicParent();
    if(!pVehicle)
    {
        *(pNetwork->GetLogger()) << Logger::Error << "Error while reading the message ID " << pMessage->GetMessageID() << " in GLOSA : The station that call GLOSA don't specified a vehicle in SymuVia !" << std::endl;
        return false;
    }

    GPSSensor* pGPS = dynamic_cast<GPSSensor*>(pStation->GetSensorByType("GPSSensor"));
    if(!pGPS)
    {
        *(pNetwork->GetLogger()) << Logger::Error << "Error while reading the message ID " << pMessage->GetMessageID() << " in GLOSA : The station that call GLOSA must be equiped with a GPS sensor !" << std::endl;
        return false;
    }

    InertialSensor* pInertial = dynamic_cast<InertialSensor*>(pStation->GetSensorByType("Inertial"));

    //Data for TraficLightMessage must be the form Action;TraficLightMessage/InTronc;ID_first_TronconIn/OutTronc;ID_first_TronconOut/Pos;12.156/ActualColor;Red/NextPhase;18[/NextPhase;14][/InTronc;ID_second_TronconIn/OutTronc;ID_second_TronconOut/Pos;6.425/ActualColor;Green/NextPhase;12[/NextPhase;17]]
    std::string data = pMessage->GetData();
    std::vector<std::string> lstAction = split(data, '/');

    int iCommandIndex = 1;
    bool shouldContinue = true;
    std::string strCommand, strValue;
    int iTuyauInID, iTuyauOutID;
	Tuyau* pTuyauIn = NULL, *pTuyauOut = NULL;
    double dbTraficLightPosition;
    double dbDistanceToTraficLight;
    eCouleurFeux eCurrentColor;
    std::vector<boost::posix_time::ptime> lstNextPhases;
    Tuyau* pActualTuyau = (Tuyau*)pGPS->GetLaneAtStart()->GetParent();
    std::vector<Tuyau*> Itinaries = pNetwork->ComputeItineraireComplet(*pVehicle->GetItineraire());
    int iTuyIdx = pVehicle->GetItineraireIndex(Itinaries, isSame, pActualTuyau);

    //Loop to read data of all trafic light of this controleur
    while(shouldContinue && iCommandIndex < lstAction.size())
    {
        //verify if we have at least 5 Command
        if(iCommandIndex + 5 > lstAction.size())
        {
            *(pNetwork->GetLogger()) << Logger::Error << "Error while reading the message ID " << pMessage->GetMessageID() << " in GLOSA : Wrong format for data; too few commands (minimum required is InTronc, OutTronc, Pos, ActualColor and at least one NextPhase) !" << std::endl;
            return false;
        }

        //first we read InTronc
        if(!ReadAction(pNetwork, lstAction[iCommandIndex], strCommand, strValue))
            return false;
        iCommandIndex++;

        if(strCommand != "InTronc")
            continue;
        iTuyauInID = SystemUtil::ToInt32(strValue);

        //then we read OutTronc
        if(!ReadAction(pNetwork, lstAction[iCommandIndex], strCommand, strValue))
            return false;
        iCommandIndex++;

        if(strCommand != "OutTronc")
        {
            *(pNetwork->GetLogger()) << Logger::Error << "Error while reading the message ID " << pMessage->GetMessageID() << " in GLOSA : OutTronc was expected but Command " << strCommand << " is found !" << std::endl;
            return false;
        }
        iTuyauOutID = SystemUtil::ToInt32(strValue);

        //then we read the position
        if(!ReadAction(pNetwork, lstAction[iCommandIndex], strCommand, strValue))
            return false;
        iCommandIndex++;

        if(strCommand != "Pos")
        {
            *(pNetwork->GetLogger()) << Logger::Error << "Error while reading the message ID " << pMessage->GetMessageID() << " in GLOSA : Pos was expected but Command " << strCommand << " is found !" << std::endl;
            return false;
        }
        dbTraficLightPosition = SystemUtil::ToDouble(strValue);

        //Find if this trafic light is on vehicle itinary
        bool isInRoute = false;
        dbDistanceToTraficLight = pGPS->GetLaneAtStart()->GetLength() - pGPS->GetDoublePosition();
        for( int i=iTuyIdx; i< (int)Itinaries.size(); i++)
        {
            if(Itinaries[i]->GetID() == iTuyauInID && (dbTraficLightPosition > 0.0 || Itinaries[i]->GetLength() + dbTraficLightPosition > pGPS->GetDoublePosition()))
            {
				for (int j = i+1; j < (int)Itinaries.size(); j++)
				{
					if (Itinaries[j]->GetID() == iTuyauOutID)
					{
						dbDistanceToTraficLight += dbTraficLightPosition;
						pTuyauIn = Itinaries[i];
						pTuyauOut = Itinaries[j];
						isInRoute = true;
					}
				}
                break;
			}
			dbDistanceToTraficLight += Itinaries[i]->GetLength();
        }

        //not on the route
        if(!isInRoute)
        {
            iCommandIndex += 2;
            continue;
        }

        //At this step, we are sure that there is a traficLight in the route (no need to continue)
        shouldContinue = false;

        //then the Color
        if(!ReadAction(pNetwork, lstAction[iCommandIndex], strCommand, strValue))
            return false;
        iCommandIndex++;

        if(strCommand != "ActualColor")
        {
            *(pNetwork->GetLogger()) << Logger::Error << "Error while reading the message ID " << pMessage->GetMessageID() << " in GLOSA : ActualColor was expected but Command " << strCommand << " is found !" << std::endl;
            return false;
        }
        if(strValue == "Green")
            eCurrentColor = FEU_VERT;
        else if(strValue == "Orange")
            eCurrentColor = FEU_ORANGE;
        else
            eCurrentColor = FEU_ROUGE;

        //then we read all the next phases to finish
        bool OneNextPhase = false;
        boost::posix_time::ptime dtNextPhase;
        while(iCommandIndex < lstAction.size())
        {
            if(!ReadAction(pNetwork, lstAction[iCommandIndex], strCommand, strValue))
                return false;

            if(strCommand != "NextPhase")
            {
                if(OneNextPhase)
                {
                    break;
                }else{
                    *(pNetwork->GetLogger()) << Logger::Error << "Error while reading the message ID " << pMessage->GetMessageID() << " in GLOSA : NextPhase was expected but Command " << strCommand << " is found !" << std::endl;
                    return false;
                }
            }
            iCommandIndex++;
            OneNextPhase = true;
            dtNextPhase = boost::posix_time::time_from_string(strValue) ;
            if(dtNextPhase.is_not_a_date_time()){
                *(pNetwork->GetLogger()) << Logger::Error << "Error while reading the message ID " << pMessage->GetMessageID() << " in GLOSA : " << strValue << " is not a correct value for NextPhase !" << std::endl;
                return false;
            }
            lstNextPhases.push_back(dtNextPhase);
        }
    }//End loop

	if (!pTuyauIn || !pTuyauOut)//No Caf on the route
		return true;

    boost::posix_time::ptime dtActualTime = pMessage->GetTimeReceive();
    double timeToReach = GetTimeDistance(pNetwork, pVehicle, dtActualTime, pGPS, pInertial, dbDistanceToTraficLight);
    boost::posix_time::ptime dtTTL = dtActualTime + boost::posix_time::microseconds((int)(timeToReach * 1000000.0));

    eCouleurFeux eNextColor = eCurrentColor;
    bool bTrafLightIsTooFar = true;
    int index = 0;
    for(size_t i=0; i<lstNextPhases.size(); i++)
    {
        index = (int)i;
        if(dtTTL < lstNextPhases[i])
        {
            bTrafLightIsTooFar = false;
            break;
        }
        eNextColor = static_cast<eCouleurFeux>((eNextColor + 1) % 3);
    }
    if(bTrafLightIsTooFar)//the trafic light is still too far. Adapt speed at next message reception
        return true;

    double newTimeToReach = 0.0;
    double speedAdvisory = pGPS->GetSpeed();
    if(eNextColor == FEU_ROUGE)
    {
        newTimeToReach = (double)(((lstNextPhases[index] - dtActualTime).total_microseconds()) / 1000000.0); // time before next green
        speedAdvisory = dbDistanceToTraficLight / newTimeToReach;
    }else if(eNextColor == FEU_ORANGE)
    {
        /*double newTimeToReach = 0.0;
        if(index==0)
            newTimeToReach = (double)(((lstNextPhases[index-1] - dtActualTime).total_microseconds()) / 1000000.0); // time at the end of green

        if(newTimeToReach == 0.0 || !PossibleAcceleration(pVehicle, pGPS, newTimeToReach, dbDistanceToTraficLight))
        {
            if(index+1 == lstNextPhases.size())//trafic light is too far away
                return true;
            newTimeToReach = (double)(((lstNextPhases[index+1] - dtActualTime).total_microseconds()) / 1000000.0);//if can't reach green before orange, reach the next green
        }
        speedAdvisory = dbDistanceToTraficLight / newTimeToReach;*/

        if(index+1 >= lstNextPhases.size())//the trafic light is still too far. Adapt speed at next message reception
            return true;

        newTimeToReach = (double)(((lstNextPhases[index+1] - dtActualTime).total_microseconds()) / 1000000.0); // time before next green
        speedAdvisory = dbDistanceToTraficLight / newTimeToReach;

    }else//Green : No speed advisory
        speedAdvisory = std::min<double>(pVehicle->GetDiagFonda()->GetVitesseLibre(), ((Tuyau*)pGPS->GetLaneAtStart()->GetParent())->GetVitRegByTypeVeh(pVehicle->GetType(), pNetwork->GetInstSim(), pGPS->GetDoublePosition(), pGPS->GetLaneAtStart()->GetNum()));

    std::string strData;
    if(m_bUseSpeedAsTarget)
	{
		strData = "Action;SpeedTarget/Target;" + SystemUtil::ToString(pVehicle->GetID()) + "/Value;" + SystemUtil::ToString(5, speedAdvisory);
	}
	else{
		double acceleration = (newTimeToReach == 0.0) ? 0.0 : ((speedAdvisory - pGPS->GetSpeed()) / newTimeToReach);
		strData = "Action;AccelerateDecelerate/Target;" + SystemUtil::ToString(pVehicle->GetID()) + "/Value;" + SystemUtil::ToString(5, acceleration);
	}
    bool bOk = pStation->SendUniqueMessage(0, strData, dtActualTime, SymuCom::Message::MT_Unicast, -1, false);//Destination 0 is Simulator

    return bOk;
}

bool GLOSA::LoadFromXML(DOMDocument *pDoc, DOMNode *pXMLNode, Reseau *pNetwork)
{
    if(!GetXmlAttributeValue(pXMLNode, "use_speed_as_target", m_bUseSpeedAsTarget, pNetwork->GetLogger()))
    {
        *(pNetwork->GetLogger()) << Logger::Error << "GLOSA need attribute 'use_speed_as_target' to work !" << std::endl;
        *(pNetwork->GetLogger()) << Logger::Info; // rebascule en mode INFO pour ne pas avoir à reprendre tous les appels aux log en précisant que c'est des INFO. à supprimer si on reprend tous les appels au log.
        return false;
    }
    return true;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Sérialisation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////
template void GLOSA::serialize(boost::archive::xml_woarchive & ar, const unsigned int version);
template void GLOSA::serialize(boost::archive::xml_wiarchive & ar, const unsigned int version);

template<class Archive>
void GLOSA::serialize(Archive& ar, const unsigned int version)
{
    ar & BOOST_SERIALIZATION_BASE_OBJECT_NVP(C_ITSApplication);
    ar & BOOST_SERIALIZATION_NVP(m_bUseSpeedAsTarget);
}
