#include "stdafx.h"
#undef MessageBox
#include "ITSStation.h"

#include "reseau.h"
#include "Logger.h"
#include "StaticElement.h"
#include "ITS/Stations/DerivedClass/Simulator.h"
#include "ITS/Sensors/ITSSensor.h"
#include "ITS/Applications/C_ITSApplication.h"

#include "ITS/SymuComSerialization.h"

#include <Communication/CommunicationRunner.h>
#include <Communication/MessageBox.h>

#include <boost/date_time/posix_time/posix_time.hpp>

#include <boost/serialization/vector.hpp>
#include <boost/serialization/set.hpp>
#include <boost/serialization/map.hpp>

ITSStation::ITSStation()
{

}

ITSStation::ITSStation(const ITSStation &StationCopy)
    :Entity(StationCopy.m_strLabel, StationCopy.m_ptCoordinates, StationCopy.m_pCommunicationRunner->GetGraph(), StationCopy.m_bCanSend, StationCopy.m_bCanReceive,
            StationCopy.m_dbRange, StationCopy.m_dbAlphaDown, StationCopy.m_dbBetaDown), m_pNetwork(StationCopy.m_pNetwork), m_strStationLabel(StationCopy.m_strStationLabel),
            m_strStationType(StationCopy.m_strStationType), m_pDynamicParent(NULL), m_pStaticParent(NULL)
{

}

ITSStation::ITSStation(Reseau* pNetwork, const std::string& strID, const std::string& strType, const SymuCom::Point& pPoint, SymuCom::Graph* pGraph, bool bCanSend, bool bCanReceive,
                       double dbEntityRange, double dbAlphaDown, double dbBetaDown)
    : Entity(strID, pPoint, pGraph, bCanSend, bCanReceive, dbEntityRange, dbAlphaDown, dbBetaDown),
	m_pNetwork(pNetwork), m_strStationLabel(strID), m_strStationType(strType), m_pDynamicParent(NULL), m_pStaticParent(NULL)
{

}

ITSStation::~ITSStation()
{
    for(size_t i=0; i<m_lstSensors.size(); i++)
    {
        delete m_lstSensors[i];
    }

    if(m_pDynamicParent)
        m_pDynamicParent->SetConnectedVehicle(NULL);
}

ITSSensor* ITSStation::GetSensorByLabel(const std::string& strIDSensor)
{
    ITSSensor* SensorRes = NULL;

    for(size_t i=0; i<m_lstSensors.size(); i++)
    {
        if(m_lstSensors[i]->GetLabel() == strIDSensor)
        {
            SensorRes = m_lstSensors[i];
            break;
        }
    }

    return SensorRes;
}

ITSSensor* ITSStation::GetSensorByType(const std::string& strTypeSensor)
{
    ITSSensor* SensorRes = NULL;

    for(size_t i=0; i<m_lstSensors.size(); i++)
    {
        if(m_lstSensors[i]->GetType() == strTypeSensor)
        {
            SensorRes = m_lstSensors[i];
            break;
        }
    }

    return SensorRes;
}

C_ITSApplication* ITSStation::GetApplicationByLabel(const std::string& strIDApplication)
{
    C_ITSApplication* ApplicationRes = NULL;

    for(size_t i=0; i<m_lstApplications.size(); i++)
    {
        if(m_lstApplications[i]->GetLabel() == strIDApplication)
        {
            ApplicationRes = m_lstApplications[i];
            break;
        }
    }

    return ApplicationRes;
}

C_ITSApplication* ITSStation::GetApplicationByType(const std::string& strTypeApplication)
{
    C_ITSApplication* ApplicationRes = NULL;

    for(size_t i=0; i<m_lstApplications.size(); i++)
    {
        if(m_lstApplications[i]->GetType() == strTypeApplication)
        {
            ApplicationRes = m_lstApplications[i];
            break;
        }
    }

    return ApplicationRes;
}

bool ITSStation::ProcessMessage(SymuCom::Message *pMessage)
{
    if(pMessage->GetDestinationID() != pMessage->GetOriginID() && pMessage->GetDestinationID() != 0)
        m_pNetwork->m_pSymucomSimulator->WriteMessageToXML(pMessage);

    bool bProcessOk = false;

    std::string data = pMessage->GetData();
    std::vector<std::string> lstAction = split(data, '/');

    std::vector<std::string> lstAtIAction = split(lstAction[0], ';');
    if(lstAtIAction[0] == "Action")
    {
        if(lstAtIAction.size() != 2)
        {
            *(m_pNetwork->GetLogger()) << Logger::Error << "Error while reading the message ID " << pMessage->GetMessageID() << " : the action is not reconized !" << std::endl;
            return false;
        }

        if(lstAtIAction[1] == "UpdateData")// data for UpdateData must be Action;UpdateData/target;IDOfTheSensor/startTime;2002-01-20 23:00:00.000/endTime;2002-01-20 23:59:59.000
        {
            if(lstAction.size() != 4)
            {
                *(m_pNetwork->GetLogger()) << Logger::Error << "Error while reading the message ID " << pMessage->GetMessageID() << " : too few information for action UpdateData !" << std::endl;
                return false;
            }

            std::vector<std::string> lstStrTarget = split(lstAction[1], ';');
            std::vector<std::string> lstStrTimeStart = split(lstAction[2], ';');
            std::vector<std::string> lstStrTimeEnd = split(lstAction[3], ';');

            if(lstStrTarget.size() != 2 || lstStrTimeStart.size() != 2 || lstStrTimeEnd.size() != 2)
            {
                *(m_pNetwork->GetLogger()) << Logger::Error << "Error while reading the message ID " << pMessage->GetMessageID() << " : informations for action UpdateData are not valid !" << std::endl;
                return true;
            }

            ITSSensor * pSensor = GetSensorByLabel(lstStrTarget[1]);
            if(!pSensor)
            {
                *(m_pNetwork->GetLogger()) << Logger::Error << "Error while reading the message ID " << pMessage->GetMessageID() << " : can't find sensor " << lstStrTarget[1]  << " for action UpdateData !" << std::endl;
                return false;
            }

            boost::posix_time::ptime dtTimeStart = boost::posix_time::time_from_string(lstStrTimeStart[1]);
            if(dtTimeStart.is_not_a_date_time())
            {
                *(m_pNetwork->GetLogger()) << Logger::Error << "Error while reading the message ID " << pMessage->GetMessageID() << " : start time " << lstStrTimeStart[1]  << " is not a valid date time for action UpdateData !" << std::endl;
                return false;
            }

            boost::posix_time::ptime dtTimeEnd = boost::posix_time::time_from_string(lstStrTimeEnd[1]);
            if(dtTimeEnd.is_not_a_date_time())
            {
                *(m_pNetwork->GetLogger()) << Logger::Error << "Error while reading the message ID " << pMessage->GetMessageID() << " : end time " << lstStrTimeStart[1]  << " is not a valid date time for action UpdateData !" << std::endl;
                return false;
            }

            bProcessOk = pSensor->UpdateData(dtTimeStart, dtTimeEnd);
        }
        else if(lstAtIAction[1] == "RunApplication") // data for RunApplication must be Action;RunApplication/target;Type_Application[/Any_other_information;Value_information]
        {
            if(lstAction.size() < 2)
            {
                *(m_pNetwork->GetLogger()) << Logger::Error << "Error while reading the message ID " << pMessage->GetMessageID() << " : too few information for action RunApplication !" << std::endl;
                return false;
            }

            std::vector<std::string> lstStrTarget = split(lstAction[1], ';');

            if(lstStrTarget.size() != 2)
            {
                *(m_pNetwork->GetLogger()) << Logger::Error << "Error while reading the message ID " << pMessage->GetMessageID() << " : information of target for action RunApplication is not valid !" << std::endl;
                return false;
            }

            C_ITSApplication * pApplication = GetApplicationByType(lstStrTarget[1]);
            if(!pApplication)
            {
                *(m_pNetwork->GetLogger()) << Logger::Error << "Error while reading the message ID " << pMessage->GetMessageID() << " : can't find application " << lstStrTarget[1]  << " for action RunApplication !" << std::endl;
                return false;
            }

            bProcessOk = pApplication->RunApplication(pMessage, this);
        }
        else //This is not a predefined action for Message; let the station treat the message
            bProcessOk = TreatMessageData(pMessage);

    }
    else //There is no word key 'action' for Message; let the station treat the message
        bProcessOk = TreatMessageData(pMessage);

    return bProcessOk;
}

Reseau *ITSStation::GetNetwork() const
{
    return m_pNetwork;
}

const std::vector<C_ITSApplication *>& ITSStation::GetApplications() const
{
    return m_lstApplications;
}

void ITSStation::AddSensor(ITSSensor *pSensor)
{
    m_lstSensors.push_back(pSensor);
}

void ITSStation::AddApplication(C_ITSApplication *pApplication)
{
    m_lstApplications.push_back(pApplication);
}

Vehicule *ITSStation::GetDynamicParent() const
{
    return m_pDynamicParent;
}

void ITSStation::SetDynamicParent(Vehicule *pDynamicParent)
{
    if(m_pStaticParent)
    {
        *(m_pNetwork->GetLogger()) << Logger::Warning << "The station " << m_strLabel << " already has a static element and can't define a dynamic element at the same time. The link to the static element will be lost !" << std::endl;
        m_pStaticParent = NULL;
    }

    m_pDynamicParent = pDynamicParent;
}

void ITSStation::SetStaticParent(StaticElement *pStaticParent)
{
    if(m_pDynamicParent)
    {
        *(m_pNetwork->GetLogger()) << Logger::Warning << "The station " << m_strLabel << " already has a dynamic element and can't define a static element at the same time. The link to the dynamic element will be lost !" << std::endl;
        m_pDynamicParent = NULL;
    }

    m_pStaticParent = pStaticParent;
}

StaticElement *ITSStation::GetStaticParent() const
{
    return m_pStaticParent;
}

const std::vector<ITSSensor *> &ITSStation::GetSensors() const
{
    return m_lstSensors;
}

std::string ITSStation::GetLabel() const
{
    return m_strStationLabel;
}

std::string ITSStation::GetType() const
{
    return m_strStationType;
}

void ITSStation::UpdateSensorsData(const boost::posix_time::ptime &dtTimeStart, double dbTimeDuration)
{
    for(size_t i=0; i<m_lstSensors.size(); i++)
    {
        m_lstSensors[i]->UpdateAllData(dtTimeStart, dbTimeDuration);
    }
}

ITSStationSnapshot::ITSStationSnapshot()
{
    this->nIDDynamicParent = -1;
    bIsDown = false;
}

ITSStationSnapshot::ITSStationSnapshot(const ITSStation* station)
{
    coordinates = station->GetCoordinates();
    for (size_t i = 0; i < station->GetMessageBox()->GetTravellingMessages().size(); i++)
    {
        lstTravellingMessages.push_back(SymuCom::Message(*station->GetMessageBox()->GetTravellingMessages().at(i)));
    }
    for (size_t i = 0; i < station->GetMessageBox()->GetTreatedMessages().size(); i++)
    {
        lstTreatedMessages.push_back(SymuCom::Message(*station->GetMessageBox()->GetTreatedMessages().at(i)));
    }
    boost::posix_time::ptime dtLastBreakDown = station->GetLastBreakDown();
    bIsDown = station->isDown();

    if (station->GetDynamicParent())
    {
        this->nIDDynamicParent = station->GetDynamicParent()->GetID();
    }
    else
    {
        this->nIDDynamicParent = -1;
    }
}

ITSStationSnapshot * ITSStation::TakeSnapshot() const
{
    return new ITSStationSnapshot(this);
}

void ITSStation::Restore(ITSStationSnapshot* pSnapshot)
{
    if (pSnapshot->nIDDynamicParent != -1)
    {
        m_pDynamicParent = m_pNetwork->GetVehiculeFromID(pSnapshot->nIDDynamicParent).get();
        assert(m_pDynamicParent);
    }
    SetCoordinates(pSnapshot->coordinates);
    for (size_t i = 0; i < GetMessageBox()->GetTravellingMessages().size(); i++)
    {
        delete GetMessageBox()->GetTravellingMessages().at(i);
    }
    std::vector<SymuCom::Message*> lstTravMessages;
    for (size_t i = 0; i < pSnapshot->lstTravellingMessages.size(); i++)
    {
        lstTravMessages.push_back(new SymuCom::Message(pSnapshot->lstTravellingMessages.at(i)));
    }
    GetMessageBox()->SetTravellingMessages(lstTravMessages);
    for (size_t i = 0; i < GetMessageBox()->GetTreatedMessages().size(); i++)
    {
        delete GetMessageBox()->GetTreatedMessages().at(i);
    }
    std::vector<SymuCom::Message*> lstTreatedMessages;
    for (size_t i = 0; i < pSnapshot->lstTreatedMessages.size(); i++)
    {
        lstTreatedMessages.push_back(new SymuCom::Message(pSnapshot->lstTreatedMessages.at(i)));
    }
    GetMessageBox()->SetTreatedMessages(lstTreatedMessages);
    this->SetLastBreakDown(pSnapshot->dtLastBreakDown);
    this->SetIsDown(pSnapshot->bIsDown);
}


template void ITSStationSnapshot::serialize(boost::archive::xml_woarchive & ar, const unsigned int version);
template void ITSStationSnapshot::serialize(boost::archive::xml_wiarchive & ar, const unsigned int version);

template<class Archive>
void ITSStationSnapshot::serialize(Archive & ar, const unsigned int version)
{
    ar & BOOST_SERIALIZATION_NVP(coordinates);
    ar & BOOST_SERIALIZATION_NVP(lstTravellingMessages);
    ar & BOOST_SERIALIZATION_NVP(lstTreatedMessages);
    ar & BOOST_SERIALIZATION_NVP(dtLastBreakDown);
    ar & BOOST_SERIALIZATION_NVP(bIsDown);
}


template void ITSStation::serialize(boost::archive::xml_woarchive & ar, const unsigned int version);
template void ITSStation::serialize(boost::archive::xml_wiarchive & ar, const unsigned int version);

template<class Archive>
void ITSStation::serialize(Archive & ar, const unsigned int version)
{
    ar & BOOST_SERIALIZATION_BASE_OBJECT_NVP(Entity);

    ar & BOOST_SERIALIZATION_NVP(m_strStationLabel);
    ar & BOOST_SERIALIZATION_NVP(m_strStationType);
    ar & BOOST_SERIALIZATION_NVP(m_pNetwork);
    ar & BOOST_SERIALIZATION_NVP(m_lstSensors);
    ar & BOOST_SERIALIZATION_NVP(m_lstApplications);
    ar & BOOST_SERIALIZATION_NVP(m_pDynamicParent);
    ar & BOOST_SERIALIZATION_NVP(m_pStaticParent);
}
