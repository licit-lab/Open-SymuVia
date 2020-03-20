#include "stdafx.h"
#undef MessageBox
#include "Simulator.h"

#include "Xerces/XMLUtil.h"
#include "Xerces/DOMLSSerializerSymu.hpp"
#include "Xerces/XMLReaderSymuvia.h"
#include "reseau.h"
#include "Logger.h"
#include "voie.h"
#include "tuyau.h"
#include "SystemUtil.h"
#include "RandManager.h"
#include "ControleurDeFeux.h"
#include "SimulationSnapshot.h"
#include "ITS/Applications/DerivedClass/GLOSA.h"
#include "ITS/Applications/C_ITSScriptableApplication.h"
#include "ITS/Applications/C_ITSExternalApplication.h"
#include "ITS/Stations/ITSScriptableStation.h"
#include "ITS/Stations/ITSExternalStation.h"
#include "ITS/Stations/DerivedClass/ITSVehicle.h"
#include "ITS/Stations/DerivedClass/ITSCAF.h"
#include "ITS/Stations/DerivedClass/BMAVehicle.h"
#include "ITS/Sensors/DerivedClass/BubbleSensor.h"
#include "ITS/Sensors/DerivedClass/GPSSensor.h"
#include "ITS/Sensors/DerivedClass/InertialSensor.h"
#include "ITS/Sensors/ITSScriptableDynamicSensor.h"
#include "ITS/Sensors/ITSScriptableStaticSensor.h"
#include "ITS/Sensors/ITSExternalDynamicSensor.h"
#include "ITS/Sensors/ITSExternalStaticSensor.h"
#include "ITS/Sensors/DerivedClass/MagicSensor.h"
#include "ITS/Sensors/DerivedClass/TelemetrySensor.h"
#include "ITS/Sensors/DerivedClass/UBRSensor.h"
#include "ITS/ExternalLibraries/ITSExternalLibrary.h"

#include "ITS/SymuComSerialization.h"

#include <Communication/CommunicationRunner.h>
#include <Communication/Graph.h>
#include <Communication/MessageBox.h>
#include <Communication/NoiseSimulation.h>

#include <boost/date_time/posix_time/posix_time.hpp>

#include <boost/serialization/vector.hpp>
#include <boost/serialization/map.hpp>

XERCES_CPP_NAMESPACE_USE

Simulator::Simulator() :
m_pGraph(NULL), m_pCommunicationRunner(NULL), m_pCurrentSimulationSnapshot(NULL), m_bReinitialized(false)
{
}

Simulator::Simulator(Reseau* pNetwork, const std::string& strID, const std::string& strType, const SymuCom::Point& pPoint, SymuCom::Graph* pGraph, SymuCom::CommunicationRunner *pCom, bool bCanSend, bool bCanReceive,
                       double dbEntityRange, double dbAlphaDown, double dbBetaDown)
    : ITSStation(pNetwork, strID, strType, pPoint, pGraph, bCanSend,bCanReceive, dbEntityRange, dbAlphaDown, dbBetaDown), m_pGraph(pGraph), m_pCommunicationRunner(pCom),
    m_pCurrentSimulationSnapshot(NULL), m_bReinitialized(false)
{

}

Simulator::~Simulator()
{
    for(size_t i=0; i<m_lstAllApplications.size(); i++)
    {
        delete m_lstAllApplications[i];
    }
    for (size_t i = 0; i < m_lstExternalLibraries.size(); i++)
    {
        delete m_lstExternalLibraries[i];
    }
    for (size_t i = 0; i < m_lstDynamicPatterns.size(); i++)
    {
        delete m_lstDynamicPatterns[i].pDynamic;
    }
}

void Simulator::UpdateSensorsData(const boost::posix_time::ptime &dtTimeStart, double dbTimeDuration)
{
    //Do nothing (simulator doesn't 'own' any sensors)
}

bool Simulator::TreatMessageData(SymuCom::Message *pMessage)
{
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

        if(lstAtIAction[1] == "AccelerateDecelerate") // Action;AccelerateDecelerate/Target;vehicle_ID/Value;double_value
        {
            if(lstAction.size() != 3)
            {
                *(m_pNetwork->GetLogger()) << Logger::Error << "Error while reading the message ID " << pMessage->GetMessageID() << " : the action AccelerateDecelerate has not the correct format !" << std::endl;
                return false;
            }
            std::vector<std::string> lstAtTarget = split(lstAction[1], ';');
            if(lstAtTarget.size() != 2)
            {
                *(m_pNetwork->GetLogger()) << Logger::Error << "Error while reading the message ID " << pMessage->GetMessageID() << " : the action AccelerateDecelerate has not the correct format !" << std::endl;
                return false;
            }
            std::vector<std::string> lstAtValue = split(lstAction[2], ';');
            if(lstAtValue.size() != 2)
            {
                *(m_pNetwork->GetLogger()) << Logger::Error << "Error while reading the message ID " << pMessage->GetMessageID() << " : the action AccelerateDecelerate has not the correct format !" << std::endl;
                return false;
            }

            m_mapAccelerateDecelerate[lstAtTarget[1]].push_back(SystemUtil::ToDouble(lstAtValue[1]));
            bProcessOk = true;
        }else if(lstAtIAction[1] == "SpeedTarget") // Action;SpeedTarget/Target;vehicle_ID/Value;double_value
        {
            if(lstAction.size() != 3)
            {
                *(m_pNetwork->GetLogger()) << Logger::Error << "Error while reading the message ID " << pMessage->GetMessageID() << " : the action SpeedTarget has not the correct format !" << std::endl;
                return false;
            }
            std::vector<std::string> lstAtTarget = split(lstAction[1], ';');
            if(lstAtTarget.size() != 2)
            {
                *(m_pNetwork->GetLogger()) << Logger::Error << "Error while reading the message ID " << pMessage->GetMessageID() << " : the action SpeedTarget has not the correct format !" << std::endl;
                return false;
            }
            std::vector<std::string> lstAtValue = split(lstAction[2], ';');
            if(lstAtValue.size() != 2)
            {
                *(m_pNetwork->GetLogger()) << Logger::Error << "Error while reading the message ID " << pMessage->GetMessageID() << " : the action SpeedTarget has not the correct format !" << std::endl;
                return false;
            }

            m_mapSpeedTarget[lstAtTarget[1]].push_back(SystemUtil::ToDouble(lstAtValue[1]));
            bProcessOk = true;
        }else if(lstAtIAction[1] == "ChangeLane") // Action;ChangeLane/Target;vehicle_ID/Value;int_new_lane
        {
            if(lstAction.size() != 3)
            {
                *(m_pNetwork->GetLogger()) << Logger::Error << "Error while reading the message ID " << pMessage->GetMessageID() << " : the action ChangeLane has not the correct format !" << std::endl;
                return false;
            }
            std::vector<std::string> lstAtTarget = split(lstAction[1], ';');
            if(lstAtTarget.size() != 2)
            {
                *(m_pNetwork->GetLogger()) << Logger::Error << "Error while reading the message ID " << pMessage->GetMessageID() << " : the action ChangeLane has not the correct format !" << std::endl;
                return false;
            }
            std::vector<std::string> lstAtValue = split(lstAction[2], ';');
            if(lstAtValue.size() != 2)
            {
                *(m_pNetwork->GetLogger()) << Logger::Error << "Error while reading the message ID " << pMessage->GetMessageID() << " : the action ChangeLane has not the correct format !" << std::endl;
                return false;
            }

            m_mapChangeLane[lstAtTarget[1]].push_back(SystemUtil::ToInt32(lstAtValue[1]));
            bProcessOk = true;
        }else if(lstAtIAction[1] == "ChangeRoute") // Action;ChangeRoute/Target;vehicle_ID/Value;new_route(separate with ':')
        {
            if(lstAction.size() != 3)
            {
                *(m_pNetwork->GetLogger()) << Logger::Error << "Error while reading the message ID " << pMessage->GetMessageID() << " : the action ChangeRoute has not the correct format !" << std::endl;
                return false;
            }
            std::vector<std::string> lstAtTarget = split(lstAction[1], ';');
            if(lstAtTarget.size() != 2)
            {
                *(m_pNetwork->GetLogger()) << Logger::Error << "Error while reading the message ID " << pMessage->GetMessageID() << " : the action ChangeRoute has not the correct format !" << std::endl;
                return false;
            }
            std::vector<std::string> lstAtValue = split(lstAction[2], ';');
            if(lstAtValue.size() != 2)
            {
                *(m_pNetwork->GetLogger()) << Logger::Error << "Error while reading the message ID " << pMessage->GetMessageID() << " : the action ChangeRoute has not the correct format !" << std::endl;
                return false;
            }

            std::vector<std::string> lstValues = split(lstAtValue[1], ':');

            for(size_t i=0; i<lstValues.size(); i++)
                m_mapChangeRoute[lstAtTarget[1]].push_back(SystemUtil::ToInt32(lstValues[i]));

            bProcessOk = true;
        }else{
            *(m_pNetwork->GetLogger()) << Logger::Error << "Error while reading the message ID " << pMessage->GetMessageID() << " : the action " << lstAtIAction[1] << " is not reconized !" << std::endl;
            return false;
        }
    }

    return bProcessOk;
}

bool Simulator::Run(const boost::posix_time::ptime &dtTimeStart, double dbTimeDuration)
{
    //Nothing to do
    return true;
}

ITSStation *Simulator::CreateCopy(const std::string& strID)
{
    //Simulator can't be copied
    return NULL;
}

Reseau *Simulator::GetNetwork() const
{
    return m_pNetwork;
}

const std::vector<ITSStation *> &Simulator::GetStations() const
{
    return m_lstAllStations;
}

C_ITSApplication *Simulator::GetApplicationByTypeFromAll(const std::string &strTypeApplication)
{
    C_ITSApplication* ApplicationRes = NULL;

    for(size_t i=0; i<m_lstAllApplications.size(); i++)
    {
        if(m_lstAllApplications[i]->GetLabel() == strTypeApplication)
        {
            ApplicationRes = m_lstAllApplications[i];
            break;
        }
    }

    return ApplicationRes;
}

// Load each parameter from XML file
bool Simulator::LoadFromXML(DOMDocument * pDoc, DOMNode *pXMLNode)
{
    std::string strXMLSymuCom;
    Logger* pLogger = m_pNetwork->GetLogger();
    DOMNode * pXMLRepartitionNode = GetNetwork()->GetXMLUtil()->SelectSingleNode("./REPARTITION", pDoc, (DOMElement *)pXMLNode);

    GetXmlAttributeValue(pXMLRepartitionNode, "symucom_xml", strXMLSymuCom, pLogger);
    if (!m_pCommunicationRunner->LoadSimulation(GetNetwork()->GetSimulationStartTime(), strXMLSymuCom))
    {
        *(m_pNetwork->GetLogger()) << Logger::Error << "Failed loading SymuCom configuration file '" << strXMLSymuCom << "'";
        return false;
    }

    DOMNode * pXMLExternalLibrariesNode = GetNetwork()->GetXMLUtil()->SelectSingleNode("./EXTERNAL_DYNAMIC_LIBRARIES", pDoc, (DOMElement *)pXMLNode);
    if (pXMLExternalLibrariesNode)
    {
        XMLSize_t countExtLibs = pXMLExternalLibrariesNode->getChildNodes()->getLength();
        for (XMLSize_t i = 0; i < countExtLibs; i++)
        {
            DOMNode* pXMLExtLib = pXMLExternalLibrariesNode->getChildNodes()->item(i);

            std::string libId, libModule;
            GetXmlAttributeValue(pXMLExtLib, "id", libId, pLogger);
            GetXmlAttributeValue(pXMLExtLib, "module", libModule, pLogger);

            ITSExternalLibrary * pExtLib = new ITSExternalLibrary();
            if (!pExtLib->LoadFromXML(pDoc, pXMLExtLib, pLogger))
            {
                delete pExtLib;
                return false;
            }
            m_lstExternalLibraries.push_back(pExtLib);
        }
    }

    DOMNode * pXMLSensorsNode = GetNetwork()->GetXMLUtil()->SelectSingleNode("./SENSORS", pDoc, (DOMElement *)pXMLNode);
    XMLSize_t countSensors = pXMLSensorsNode->getChildNodes()->getLength();
    for(XMLSize_t i=0; i<countSensors; i++)
    {
        SensorParams sensParams;
        std::string strSensorID;
        DOMNode* pXMLSensor = pXMLSensorsNode->getChildNodes()->item(i);

        GetXmlAttributeValue(pXMLSensor, "sensor_id", strSensorID, pLogger);
        GetXmlAttributeValue(pXMLSensor, "sensor_type", sensParams.strSensorType, pLogger);
        GetXmlAttributeValue(pXMLSensor, "python_module", sensParams.strPythonModule, pLogger);
        GetXmlAttributeValue(pXMLSensor, "ext_library_id", sensParams.strExternalLibID, pLogger);
        GetXmlAttributeValue(pXMLSensor, "update_frequency", sensParams.iUpdateFrenquency, pLogger);
        GetXmlAttributeValue(pXMLSensor, "precision", sensParams.iPrecision, pLogger);
        GetXmlAttributeValue(pXMLSensor, "break_down_rate", sensParams.dbBreakDownRate, pLogger);
        GetXmlAttributeValue(pXMLSensor, "noise_rate", sensParams.dbNoiseRate, pLogger);
        GetXmlAttributeValue(pXMLSensor, "is_static_sensor", sensParams.bStaticSensor, pLogger);

        std::string strSensorType = sensParams.strSensorType;
        if(sensParams.bStaticSensor)
        {
            if(strSensorType == "BubbleSensor" || strSensorType == "MagicSensor" || strSensorType == "GPSSensor" || strSensorType == "InertialSensor" || strSensorType == "TelemetrySensor")
            {
                *(m_pNetwork->GetLogger()) << Logger::Error << "Error while reading the XML : The ITS Sensor " << strSensorID << " is type of " << strSensorType << " and thus can't be static !" << std::endl;
                return false;
            }
        }else
        {
           if(strSensorType == "UBRSensor")
           {
               *(m_pNetwork->GetLogger()) << Logger::Error << "Error while reading the XML : The ITS Sensor " << strSensorID << " is type of " << strSensorType << " and thus can't be dynamic !" << std::endl;
               return false;
           }
        }
        DOMNode * pXMLSensorParamNode = GetNetwork()->GetXMLUtil()->SelectSingleNode("./PARAMETERS", pDoc, (DOMElement *)pXMLSensor);
        sensParams.pXMLSensorNode = pXMLSensorParamNode;

        m_mapSensorsParams[strSensorID] = sensParams;
    }

    DOMNode * pXMLApplicationsNode = GetNetwork()->GetXMLUtil()->SelectSingleNode("./APPLICATIONS", pDoc, (DOMElement *)pXMLNode);
    XMLSize_t countApplications = pXMLApplicationsNode->getChildNodes()->getLength();
    for(XMLSize_t i=0; i<countApplications; i++)
    {
        DOMNode* pXMLApplication = pXMLApplicationsNode->getChildNodes()->item(i);
        std::string strApplicationType, strApplicationID, strPythonModule, strExtLibId;
        GetXmlAttributeValue(pXMLApplication, "application_id", strApplicationID, pLogger);
        GetXmlAttributeValue(pXMLApplication, "application_type", strApplicationType, pLogger);
        GetXmlAttributeValue(pXMLApplication, "python_module", strPythonModule, pLogger);
        GetXmlAttributeValue(pXMLApplication, "ext_library_id", strExtLibId, pLogger);

        DOMNode * pXMLAppliParamNode = GetNetwork()->GetXMLUtil()->SelectSingleNode("./PARAMETERS", pDoc, (DOMElement *)pXMLApplication);

        if (strPythonModule.empty() && strExtLibId.empty())
        {
            if (strApplicationType == "GLOSA")
            {
                GLOSA* pGlosa = new GLOSA(strApplicationID, strApplicationType);
                if (!pGlosa->LoadFromXML(pDoc, pXMLAppliParamNode, m_pNetwork))
                    return false;

                m_lstAllApplications.push_back(pGlosa);
            }
            else
            {
                *(m_pNetwork->GetLogger()) << Logger::Error << "Unknown application type '" << strApplicationType << "'" << std::endl;
                *(m_pNetwork->GetLogger()) << Logger::Info; // rebascule en mode INFO pour ne pas avoir � reprendre tous les appels aux log en pr�cisant que c'est des INFO. � supprimer si on reprend tous les appels au log.
                return false;
            }
        }
        else if (!strPythonModule.empty())
        {
            if (!strExtLibId.empty())
            {
                *(m_pNetwork->GetLogger()) << Logger::Warning << "External library AND python module set for application '" << strApplicationID << "'. Ignoring ext_library_id." << std::endl;
                *(m_pNetwork->GetLogger()) << Logger::Info; // rebascule en mode INFO pour ne pas avoir � reprendre tous les appels aux log en pr�cisant que c'est des INFO. � supprimer si on reprend tous les appels au log.
            }
            C_ITSScriptableApplication* pAppli = new C_ITSScriptableApplication(strApplicationID, strApplicationType);
            if (!pAppli->initialize(m_pNetwork, strPythonModule))
                    return false;
            if (!pAppli->LoadFromXML(pDoc, pXMLAppliParamNode, m_pNetwork))
                return false;

            m_lstAllApplications.push_back((C_ITSApplication*)pAppli);
        }
        else
        {
            ITSExternalLibrary * pExtLib = GetExternalLibraryFromID(strExtLibId);
            assert(pExtLib); // Il y en a forc�ment une puisque XSD
            C_ITSExternalApplication* pAppli = new C_ITSExternalApplication(strApplicationID, strApplicationType, pExtLib, m_pNetwork);
            if (!pAppli->LoadFromXML(pDoc, pXMLAppliParamNode, m_pNetwork))
            {
                delete pAppli;
                return false;
            }

            m_lstAllApplications.push_back(pAppli);
        }
    }

    //Read stations
    DOMNode * pXMLStationsNode = GetNetwork()->GetXMLUtil()->SelectSingleNode("./ITS_STATIONS", pDoc, (DOMElement *)pXMLNode);
    XMLSize_t countStations = pXMLStationsNode->getChildNodes()->getLength();
    for(XMLSize_t i=0; i<countStations; i++)
    {
        DOMNode* pXMLStation = pXMLStationsNode->getChildNodes()->item(i);
        std::string strStationType, strStationID, strPythonModule, strExtLibId;;
        GetXmlAttributeValue(pXMLStation, "station_type", strStationType, pLogger);
        GetXmlAttributeValue(pXMLStation, "station_id", strStationID, pLogger);
        GetXmlAttributeValue(pXMLStation, "python_module", strPythonModule, pLogger);
        GetXmlAttributeValue(pXMLStation, "ext_library_id", strExtLibId, pLogger);

        double dbRange, dbPosX, dbPosY, dbPosZ;
        GetXmlAttributeValue(pXMLStation, "range", dbRange, m_pNetwork->GetLogger());

        bool bHasPos = GetXmlAttributeValue(pXMLStation, "pos_x", dbPosX, m_pNetwork->GetLogger());
        if(bHasPos)
            bHasPos = GetXmlAttributeValue(pXMLStation, "pos_y", dbPosY, m_pNetwork->GetLogger());
        if(bHasPos)
            bHasPos = GetXmlAttributeValue(pXMLStation, "pos_z", dbPosZ, m_pNetwork->GetLogger());

        SymuCom::Point ptEntityPos;
        if(bHasPos)
            ptEntityPos = SymuCom::Point(dbPosX, dbPosY, dbPosZ);

        bool canSend, canReceive;
        double dbAlphaDown, dbBetaDown;
        GetXmlAttributeValue(pXMLStation, "can_send", canSend, m_pNetwork->GetLogger());

        GetXmlAttributeValue(pXMLStation, "can_receive", canReceive, m_pNetwork->GetLogger());

        GetXmlAttributeValue(pXMLStation, "alpha_down", dbAlphaDown, m_pNetwork->GetLogger());

        GetXmlAttributeValue(pXMLStation, "beta_down", dbBetaDown, m_pNetwork->GetLogger());

        DOMNode * pXMLDynamicElementsNode = GetNetwork()->GetXMLUtil()->SelectSingleNode("./DYNAMIC_ELEMENTS", pDoc, (DOMElement *)pXMLStation);

        DOMNode * pXMLStationParamNode = GetNetwork()->GetXMLUtil()->SelectSingleNode("./PARAMETERS", pDoc, (DOMElement *)pXMLStation);

        if (strPythonModule.empty() && strExtLibId.empty())
        {
            if(strStationType == "VehicleITS")
            {
                if(!pXMLDynamicElementsNode)
                {
                    *(m_pNetwork->GetLogger()) << Logger::Error << "Error for station '" << strStationID << "' : VehicleITS must be dynamic !" << std::endl;
                    *(m_pNetwork->GetLogger()) << Logger::Info; // rebascule en mode INFO pour ne pas avoir à reprendre tous les appels aux log en précisant que c'est des INFO. à supprimer si on reprend tous les appels au log.
                    return false;
                }
                ITSVehicle* pVehicle = new ITSVehicle(NULL, m_pNetwork, strStationID, strStationType, ptEntityPos, m_pGraph, canSend, canReceive, dbRange, dbAlphaDown, dbBetaDown);

                if (!AddSpecificSensorsAndApplications(pVehicle, pDoc, pXMLStation))
                    return false;

                if(!pVehicle->LoadFromXML(pDoc, pXMLStationParamNode))
                    return false;

                CreateDynamicPattern(pVehicle, pXMLDynamicElementsNode);
            }
            else if (strStationType == "CAFITS")
            {
                StationParams stParams;
                stParams.pXMLStationParamNode = pXMLStationParamNode;
                stParams.pXMLStationNode = pXMLStation;
                stParams.pXMLDoc = pDoc;
                stParams.strStationID = strStationID;
                stParams.strStationType = strStationType;
                stParams.dbRange = dbRange;
                stParams.canSend = canSend;
                stParams.canReceive = canReceive;
                stParams.dbAlphaDown = dbAlphaDown;
                stParams.dbBetaDown = dbBetaDown;
                stParams.ptEntityPos = ptEntityPos;
                if (!CreateCAFStations(stParams))
                    return false;
            }
			else if (strStationType == "VehicleBMA")
			{
				if (!pXMLDynamicElementsNode)
				{
					*(m_pNetwork->GetLogger()) << Logger::Error << "Error for station '" << strStationID << "' : VehicleBMA must be dynamic !" << std::endl;
					*(m_pNetwork->GetLogger()) << Logger::Info; // rebascule en mode INFO pour ne pas avoir à reprendre tous les appels aux log en précisant que c'est des INFO. à supprimer si on reprend tous les appels au log.
					return false;
				}
				BMAVehicle* pVehicle = new BMAVehicle(NULL, m_pNetwork, strStationID, strStationType, ptEntityPos, m_pGraph, canSend, canReceive, dbRange, dbAlphaDown, dbBetaDown);

				if (!AddSpecificSensorsAndApplications(pVehicle, pDoc, pXMLStation))
					return false;

				if (!pVehicle->LoadFromXML(pDoc, pXMLStationParamNode))
					return false;

				CreateDynamicPattern(pVehicle, pXMLDynamicElementsNode);
			}
            else
            {
                *(m_pNetwork->GetLogger()) << Logger::Error << "Unknown station type '" << strStationType << "'" << std::endl;
                *(m_pNetwork->GetLogger()) << Logger::Info; // rebascule en mode INFO pour ne pas avoir à reprendre tous les appels aux log en précisant que c'est des INFO. à supprimer si on reprend tous les appels au log.
                return false;
            }
        }
        else if (!strPythonModule.empty())
        {
            if (!strExtLibId.empty())
            {
                *(m_pNetwork->GetLogger()) << Logger::Warning << "External library AND python module set for station '" << strStationID << "'. Ignoring ext_library_id." << std::endl;
                *(m_pNetwork->GetLogger()) << Logger::Info; // rebascule en mode INFO pour ne pas avoir à reprendre tous les appels aux log en précisant que c'est des INFO. à supprimer si on reprend tous les appels au log.
            }
            ITSScriptableStation* pStation = new ITSScriptableStation(m_pNetwork, strStationID, strStationType, ptEntityPos, m_pGraph, true, true, dbRange, dbAlphaDown, dbBetaDown);
			if (!pStation->initialize(m_pNetwork, strPythonModule))
				return false;
            if(!pStation->LoadFromXML(pDoc, pXMLStationParamNode))
                return false;
            if (!AddSpecificSensorsAndApplications(pStation, pDoc, pXMLStation))
                return false;

            if(pXMLDynamicElementsNode)
                CreateDynamicPattern(pStation, pXMLDynamicElementsNode);
            else
                AddStation(pStation);
        }
        else
        {
            ITSExternalLibrary * pExtLib = GetExternalLibraryFromID(strExtLibId);
            assert(pExtLib); // Il y en a forc�ment une puisque XSD
            ITSExternalStation* pStation = new ITSExternalStation(m_pNetwork, strStationID, strStationType, ptEntityPos, m_pGraph, true, true, dbRange, dbAlphaDown, dbBetaDown, pExtLib);
            if (!pStation->LoadFromXML(pDoc, pXMLStationParamNode))
            {
                delete pStation;
                return false;
            }
            if (!AddSpecificSensorsAndApplications(pStation, pDoc, pXMLStation))
            {
                delete pStation;
                return false;
            }

            if(pXMLDynamicElementsNode)
                CreateDynamicPattern(pStation, pXMLDynamicElementsNode);
            else
                AddStation(pStation);
        }
    }

    return true;
}


bool Simulator::AddSpecificSensorsAndApplications(ITSStation* pStation, DOMDocument *pDoc, DOMNode *pXMLNode)
{
    //Read applications for this station
    DOMNode * pXMLStApplicationsNode = GetNetwork()->GetXMLUtil()->SelectSingleNode("./FLUX_APPLICATIONS", pDoc, (DOMElement *)pXMLNode);
    XMLSize_t countStApplis = pXMLStApplicationsNode->getChildNodes()->getLength();
    for(XMLSize_t i=0; i<countStApplis; i++)
    {
        DOMNode* pXMLAppli = pXMLStApplicationsNode->getChildNodes()->item(i);
        std::string strAppliID;
        GetXmlAttributeValue(pXMLAppli, "application_id", strAppliID, m_pNetwork->GetLogger());
        C_ITSApplication* pAppli = GetApplicationByTypeFromAll(strAppliID);
        if(!pAppli)
        {
            *(m_pNetwork->GetLogger()) << Logger::Error << "Error while reading the XML : The ITS Application " << strAppliID << " needed for " << pStation->GetLabel() << " can't be found !" << std::endl;
            return false;
        }
        pStation->AddApplication(pAppli);
    }

    //Read sensors for this station
    DOMNode * pXMLStSensorsNode = GetNetwork()->GetXMLUtil()->SelectSingleNode("./FLUX_SENSORS", pDoc, (DOMElement *)pXMLNode);
    XMLSize_t countStSensors = pXMLStSensorsNode->getChildNodes()->getLength();
    for(XMLSize_t i=0; i<countStSensors; i++)
    {
        DOMNode* pXMLSensor = pXMLStSensorsNode->getChildNodes()->item(i);
        std::string strSensorID;
        GetXmlAttributeValue(pXMLSensor, "sensor_id", strSensorID, m_pNetwork->GetLogger());
        std::map<std::string, SensorParams>::iterator itSensorParam = m_mapSensorsParams.find(strSensorID);
        if(itSensorParam == m_mapSensorsParams.end())
        {
            *(m_pNetwork->GetLogger()) << Logger::Error << "Error while reading the XML : The ITS Sensor " << strSensorID << " needed for " << pStation->GetLabel() << " can't be found !" << std::endl;
            return false;
        }

        //create the Sensor
        SensorParams sParam = itSensorParam->second;
        ITSSensor* pSensor;
        strSensorID = strSensorID + "_" + pStation->GetLabel();

        if (sParam.strPythonModule.empty() && sParam.strExternalLibID.empty())
        {
            if (sParam.strSensorType == "BubbleSensor")
            {
                pSensor = new BubbleSensor(m_pNetwork, strSensorID, sParam.strSensorType, pStation, sParam.iUpdateFrenquency, sParam.dbBreakDownRate, sParam.iPrecision, sParam.dbNoiseRate);
            }
            else if (sParam.strSensorType == "GPSSensor")
            {
                pSensor = new GPSSensor(m_pNetwork, strSensorID, sParam.strSensorType, pStation, sParam.iUpdateFrenquency, sParam.dbBreakDownRate, sParam.iPrecision, sParam.dbNoiseRate);
            }
            else if (sParam.strSensorType == "InertialSensor")
            {
                pSensor = new InertialSensor(m_pNetwork, strSensorID, sParam.strSensorType, pStation, sParam.iUpdateFrenquency, sParam.dbBreakDownRate, sParam.iPrecision, sParam.dbNoiseRate);
            }
            else if (sParam.strSensorType == "MagicSensor")
            {
                pSensor = new MagicSensor(m_pNetwork, strSensorID, sParam.strSensorType, pStation, sParam.iUpdateFrenquency, sParam.dbBreakDownRate, sParam.iPrecision, sParam.dbNoiseRate);
            }
            else if (sParam.strSensorType == "TelemetrySensor")
            {
                pSensor = new TelemetrySensor(m_pNetwork, strSensorID, sParam.strSensorType, pStation, sParam.iUpdateFrenquency, sParam.dbBreakDownRate, sParam.iPrecision, sParam.dbNoiseRate);
            }
            else
            {
                *(m_pNetwork->GetLogger()) << Logger::Error << "Unknown sensor type '" << sParam.strSensorType << "'" << std::endl;
                *(m_pNetwork->GetLogger()) << Logger::Info; // rebascule en mode INFO pour ne pas avoir � reprendre tous les appels aux log en pr�cisant que c'est des INFO. � supprimer si on reprend tous les appels au log.
                return false;
            }
        }
        else if (!sParam.strPythonModule.empty())
        {
            if (!sParam.strExternalLibID.empty())
            {
                *(m_pNetwork->GetLogger()) << Logger::Warning << "External library AND python module set for sensor '" << strSensorID << "'. Ignoring ext_library_id." << std::endl;
                *(m_pNetwork->GetLogger()) << Logger::Info; // rebascule en mode INFO pour ne pas avoir � reprendre tous les appels aux log en pr�cisant que c'est des INFO. � supprimer si on reprend tous les appels au log.
            }
            if(sParam.bStaticSensor)
            {
                ITSScriptableStaticSensor* pScriptSensor = new ITSScriptableStaticSensor(m_pNetwork, strSensorID, sParam.strSensorType, pStation, sParam.iUpdateFrenquency, sParam.dbBreakDownRate, sParam.iPrecision, sParam.dbNoiseRate);
                if (!pScriptSensor->initialize(m_pNetwork, sParam.strPythonModule))
                    return false;
                pSensor = pScriptSensor;
            }else{
                ITSScriptableDynamicSensor* pScriptSensor = new ITSScriptableDynamicSensor(m_pNetwork, strSensorID, sParam.strSensorType, pStation, sParam.iUpdateFrenquency, sParam.dbBreakDownRate, sParam.iPrecision, sParam.dbNoiseRate);
                if (!pScriptSensor->initialize(m_pNetwork, sParam.strPythonModule))
                    return false;
                pSensor = pScriptSensor;
            }
        }
        else
        {
            ITSExternalLibrary * pExtLib = GetExternalLibraryFromID(sParam.strExternalLibID);
            assert(pExtLib); // Il y en a forcément une puisque XSD
            if (sParam.bStaticSensor)
            {
                pSensor = new ITSExternalStaticSensor(m_pNetwork, strSensorID, sParam.strSensorType, pStation, sParam.iUpdateFrenquency, sParam.dbBreakDownRate, sParam.iPrecision, sParam.dbNoiseRate, pExtLib);
            }
            else{
                pSensor = new ITSExternalDynamicSensor(m_pNetwork, strSensorID, sParam.strSensorType, pStation, sParam.iUpdateFrenquency, sParam.dbBreakDownRate, sParam.iPrecision, sParam.dbNoiseRate, pExtLib);
            }
        }
        if(!pSensor->LoadFromXML(pDoc, sParam.pXMLSensorNode))
            return false;

        pStation->AddSensor(pSensor);
        m_lstAllSensors.push_back(pSensor);
    }

    return true;
}

void Simulator::RemoveConnectedVehicle(ITSStation *pStation)
{
    std::vector<ITSSensor*>::iterator itSens;
    for (size_t i = 0; i<pStation->GetSensors().size(); i++)
    {
        itSens = find(m_lstAllSensors.begin(), m_lstAllSensors.end(), pStation->GetSensors()[i]);
        if (itSens != m_lstAllSensors.end())
            m_lstAllSensors.erase(itSens);
    }

    std::vector<ITSStation*>::iterator itStat = find(m_lstAllStations.begin(), m_lstAllStations.end(), pStation);
    if (itStat != m_lstAllStations.end())
        m_lstAllStations.erase(itStat);

    m_pGraph->RemoveEntity(pStation);
    delete pStation;
}

ITSExternalLibrary * Simulator::GetExternalLibraryFromID(const std::string & strExternalLibID)
{
    for (size_t i = 0; i < m_lstExternalLibraries.size(); i++)
    {
        if (!m_lstExternalLibraries.at(i)->GetID().compare(strExternalLibID))
        {
            return m_lstExternalLibraries.at(i);
        }
    }
    return NULL;
}

SymuCom::CommunicationRunner *Simulator::GetCommunicationRunner() const
{
    return m_pCommunicationRunner;
}

void Simulator::AddStation(ITSStation *pStation)
{
    WriteEntityToXML(pStation);
    m_lstAllStations.push_back(pStation);
}

void Simulator::WriteEntityToXML(ITSStation *pStation)
{
    if(!pStation)
        return;

    Vehicule* pVehicle = pStation->GetDynamicParent();

    DOMLSSerializerSymu* pWriter = m_pCurrentSimulationSnapshot ? m_pCurrentSimulationSnapshot->m_pSymuComWriter : m_pXMLWriter;

    pWriter->writeStartElement(XS("ENTITY"));

    pWriter->writeAttributeString(XS("entity_id"), XS(SystemUtil::ToString((int)pStation->GetEntityID()).c_str()));
    pWriter->writeAttributeString(XS("range"), XS(SystemUtil::ToString(2, pStation->GetRange()).c_str()));

    if(pVehicle)
    {
        pWriter->writeAttributeString(XS("vehicle_id"), XS(SystemUtil::ToString(pVehicle->GetID()).c_str()));
        pWriter->writeAttributeString(XS("is_dynamic"), XS(std::string("true").c_str()));
    }
    else
    {
        std::string strCoord = SystemUtil::ToString(2, pStation->GetCoordinates().getX()) + " " + SystemUtil::ToString(2, pStation->GetCoordinates().getY()) + " " + SystemUtil::ToString(2, pStation->GetCoordinates().getZ());
        pWriter->writeAttributeString(XS("coord"), XS(strCoord.c_str()));
        pWriter->writeAttributeString(XS("is_dynamic"), XS(std::string("false").c_str()));
    }

    pWriter->writeEndElement(); // ENTITY
}

void Simulator::WriteMessageToXML(SymuCom::Message *pMessage)
{
    DOMLSSerializerSymu* pWriter = m_pCurrentSimulationSnapshot ? m_pCurrentSimulationSnapshot->m_pSymuComWriter : m_pXMLWriter;

    pWriter->writeStartElement(XS("MESSAGE"));

    pWriter->writeAttributeString(XS("message_id"), XS(SystemUtil::ToString((int)pMessage->GetMessageID()).c_str()));
    pWriter->writeAttributeString(XS("origin_id"), XS(SystemUtil::ToString((int)pMessage->GetOriginID()).c_str()));
    pWriter->writeAttributeString(XS("destination_id"), XS(SystemUtil::ToString((int)pMessage->GetDestinationID()).c_str()));

    double instSend = (double)(((pMessage->GetTimeSend() - m_pNetwork->GetSimulationStartTime()).total_milliseconds()) / 1000) / m_pNetwork->GetTimeStep();
    pWriter->writeAttributeString(XS("inst_send"), XS(SystemUtil::ToString(3, instSend).c_str()));

    double instReceive = (double)(((pMessage->GetTimeReceive() - m_pNetwork->GetSimulationStartTime()).total_milliseconds()) / 1000) / m_pNetwork->GetTimeStep();
    pWriter->writeAttributeString(XS("inst_receive"), XS(SystemUtil::ToString(3, instReceive).c_str()));

    pWriter->writeAttributeString(XS("data"), XS(pMessage->GetData().c_str()));

    pWriter->writeEndElement(); // MESSAGE
}

void Simulator::InitSymuComXML(const std::string& strFile, DOMLSSerializerSymu** pWriter) const
{
    // m_pXMLWriter
    (*pWriter) = new DOMLSSerializerSymu();
    (*pWriter)->initXmlWriter(strFile.c_str(), gXMLDecl_ver10, gXMLDecl_encodingASCII);
    (*pWriter)->setIndent(true);

    // Noeud déclaration
    (*pWriter)->writeXmlDeclaration();

    (*pWriter)->writeStartElement(XS("ROOT_SYMUCOM"));
}

SymuCom::Graph *Simulator::GetGraph() const
{
    return m_pGraph;
}

void Simulator::InitSymuComXML(const std::string& strFile)
{
    InitSymuComXML(strFile, &m_pXMLWriter);
}

void Simulator::TerminateSymuComXML()
{
    if (m_pXMLWriter)
    {
        if (!m_bReinitialized)
        {
            m_pXMLWriter->writeEndElement(); // ROOT_SYMUCOM
        }
        else
        {
            m_pXMLWriter->setCurrentIndentLevel(0);
            m_pXMLWriter->writeEndElement(XS("ROOT_SYMUCOM"));
        }

        m_pXMLWriter->close();

        delete m_pXMLWriter;
        m_pXMLWriter = NULL;
    }
}

// passage en mode d'écriture dans fichiers temporaires (affectation dynamique)
void Simulator::doUseTempFiles(SimulationSnapshot * pSimulationSnapshot, size_t snapshotIdx)
{
    m_pCurrentSimulationSnapshot = pSimulationSnapshot;

    assert(m_pCurrentSimulationSnapshot->m_pSymuComWriter == NULL);

    std::string ssFile = m_pNetwork->GetPrefixOutputFiles();
    if (snapshotIdx == 0)
    {
        ssFile += "_SymuComTMP.xml";
    }
    else
    {
        ssFile += "_SymuComTMP_" + SystemUtil::ToString(snapshotIdx) + ".xml";
    }

    InitSymuComXML(ssFile, &m_pCurrentSimulationSnapshot->m_pSymuComWriter);
}

// validation de la dernière période d'affectation et recopie des fichiers temporaires dans les fichiers finaux
void Simulator::converge(SimulationSnapshot * pSimulationSnapshot)
{
    if (pSimulationSnapshot && pSimulationSnapshot->m_pSymuComWriter)
    {
        std::string tmpFile = pSimulationSnapshot->m_pSymuComWriter->getTargetFileName();

        // Terminaison propre du fichier XML temporaire retenu
        pSimulationSnapshot->m_pSymuComWriter->writeEndElement(); // ROOT_SYMUCOM
        pSimulationSnapshot->m_pSymuComWriter->close();
        delete pSimulationSnapshot->m_pSymuComWriter;
        pSimulationSnapshot->m_pSymuComWriter = NULL;

        // Ajout de l'ensemble des noeuds du document temporaire retenu dans le document final :
        XmlReaderSymuvia * XmlReader;
        XmlReader = XmlReaderSymuvia::Create(tmpFile);
        while (XmlReader->Read() && XmlReader->Name.compare("ENTITY") && XmlReader->Name.compare("MESSAGE"))
        {
            // NO OP
        }
        while (!XmlReader->Name.compare("ENTITY") || !XmlReader->Name.compare("MESSAGE"))
        {
            XmlReader->WriteNode(m_pXMLWriter, false);
        }
        delete XmlReader;

        // Suppression du fichier temporaire
        remove(tmpFile.c_str());


    }
    m_pCurrentSimulationSnapshot = NULL;
}

bool Simulator::CreateDynamicPattern(ITSStation* DynPattern, DOMNode*  pXMLDynamic)
{
    double dbEquipedRate;
    std::string strIDTypeVeh;
    TypeVehicule* pTypeVeh;

    //remove Station
    m_pGraph->RemoveEntity(DynPattern);

    XMLSize_t countDynamicElement = pXMLDynamic->getChildNodes()->getLength();
    for(XMLSize_t i=0; i<countDynamicElement; i++)
    {
        DOMNode* pXMLDynamicElement = pXMLDynamic->getChildNodes()->item(i);

        GetXmlAttributeValue(pXMLDynamicElement, "id_type_vehicule", strIDTypeVeh, m_pNetwork->GetLogger());

        GetXmlAttributeValue(pXMLDynamicElement, "equiped_rate", dbEquipedRate, m_pNetwork->GetLogger());

        pTypeVeh = m_pNetwork->GetVehicleTypeFromID(strIDTypeVeh);
        if(!pTypeVeh)
        {
            *(m_pNetwork->GetLogger()) << Logger::Error << "ID VehicleType '" << strIDTypeVeh << "' is unknow for '" << DynPattern->GetType() << "' station !" << std::endl;
            *(m_pNetwork->GetLogger()) << Logger::Info; // rebascule en mode INFO pour ne pas avoir à reprendre tous les appels aux log en précisant que c'est des INFO. à supprimer si on reprend tous les appels au log.
            return false;
        }

        VehiclePattern VehPattern;
        VehPattern.pDynamic = DynPattern;
        VehPattern.pTypeVeh = pTypeVeh;
        VehPattern.dbPenetrationRate = dbEquipedRate;

        m_lstDynamicPatterns.push_back(VehPattern);

    }

    return true;
}

ITSStation* Simulator::CreateDynamicStationFromPattern(Vehicule* pVehicle)//Creation of a connected vehicle
{
    ITSStation* pNewDynStation = NULL;
    for(size_t i=0; i<m_lstDynamicPatterns.size(); i++)
    {
        VehiclePattern VehPattern = m_lstDynamicPatterns[i];

        if(pVehicle->GetType() != VehPattern.pTypeVeh)
            continue;

        if (((double)pVehicle->GetReseau()->GetRandManager()->myRand() / (double)MAXIMUM_RANDOM_NUMBER) > VehPattern.dbPenetrationRate)
            continue;

        ITSStation* pDynamicPat = VehPattern.pDynamic;
        std::string strID = pDynamicPat->GetLabel() + "_" + std::to_string(pVehicle->GetID());
        pNewDynStation = pDynamicPat->CreateCopy(strID);

        pNewDynStation->SetDynamicParent(pVehicle);

        for(size_t j=0; j<pDynamicPat->GetApplications().size(); j++)
        {
            pNewDynStation->AddApplication(pDynamicPat->GetApplications()[j]);
        }
        for(size_t j=0; j<pDynamicPat->GetSensors().size(); j++)
        {
            ITSSensor* pSensor = pDynamicPat->GetSensors()[j]->CreateCopy(pNewDynStation);
            pNewDynStation->AddSensor(pSensor);
            m_lstAllSensors.push_back(pSensor);
        }
        AddStation(pNewDynStation);
        break;
    }

    return pNewDynStation;
}


bool Simulator::CreateCAFStations(StationParams lstParams)
{
    if (m_pNetwork->GetLstControleursDeFeux().size() == 0)
    {
        *(m_pNetwork->GetLogger()) << Logger::Error << "Error : No ControleurDeFeux found in the network when creating CAFITS !" << std::endl;
        return false;
    }

    double dbPenetrationRate;
    int iSendFrenquency;
    bool bOk = true;

	if(!GetXmlAttributeValue(lstParams.pXMLStationParamNode, "penetration_rate", dbPenetrationRate, m_pNetwork->GetLogger()))
	{
		*(m_pNetwork->GetLogger()) << Logger::Error << "Error : CAFITS need parameter penetration_rate !" << std::endl;
		return false;
	}

	if(!GetXmlAttributeValue(lstParams.pXMLStationParamNode, "send_frequency", iSendFrenquency, m_pNetwork->GetLogger()))
	{
		*(m_pNetwork->GetLogger()) << Logger::Error << "Error : CAFITS need parameter send_frequency !" << std::endl;
		return false;
	}

	std::deque<ControleurDeFeux*> lstControleurs = m_pNetwork->GetLstControleursDeFeux();
    int nbCaf = (int)(lstControleurs.size() * dbPenetrationRate);

    //We will randomnly choose nbCaf ControleurDeFeux to to be connected
    for(size_t i=0; i<nbCaf && bOk; i++)
    {
        int iRand = (int)(((double)m_pNetwork->GetRandManager()->myRand() / (double)MAXIMUM_RANDOM_NUMBER) * (lstControleurs.size() - 1));
		std::deque<ControleurDeFeux*>::iterator itQue = lstControleurs.begin() + iRand;
		ControleurDeFeux* pControleur = *(itQue);

        //The ID of the station is a concatenation of the ID for all those specific CAFITS + '_' + ID_ControleurDeFeux
        std::string strID = lstParams.strStationID + "_" + pControleur->GetUnifiedID();

        //Point is the mean of all LigneDeFeux for this ControleurDeFeux
        SymuCom::Point ptPos(pControleur->GetPos().dbX, pControleur->GetPos().dbY, pControleur->GetPos().dbZ);

        //No need to call LoadFromXML for each Stations. It's better to direcly read the range value because, for now, it's the same for every one of them
        ITSCAF* pCAF = new ITSCAF(pControleur, m_pNetwork, strID, lstParams.strStationType, ptPos, m_pGraph, iSendFrenquency, true, false, lstParams.dbRange);
        if(!pCAF->LoadFromXML(lstParams.pXMLDoc, lstParams.pXMLStationParamNode))
            return false;

		if (!AddSpecificSensorsAndApplications(pCAF, lstParams.pXMLDoc, lstParams.pXMLStationNode))
            return false;

		AddStation(pCAF);
		lstControleurs.erase(itQue);
    }

    return bOk;
}

// Mise en veille (prise snapshot pour reprise plus tard)
void Simulator::Suspend()
{
    if (m_pXMLWriter != NULL)
    {
        m_pXMLWriter->close();
        delete m_pXMLWriter;
        m_pXMLWriter = NULL;
    }
}

// Re-initialisation (après une désérialisation par exemple)
void Simulator::ReInit(const std::string & strXMLWriter)
{
    // ré-ouverture du writer
    m_pXMLWriter = new DOMLSSerializerSymu();
    m_pXMLWriter->initXmlWriter((m_pNetwork->GetPrefixOutputFiles() + "_SymuCom.xml").c_str(), gXMLDecl_ver10, gXMLDecl_encodingASCII);
    m_pXMLWriter->setIndent(true);
    m_pXMLWriter->write(strXMLWriter);
    m_pXMLWriter->setCurrentIndentLevel(1);
    m_bReinitialized = true;
}

SimulatorSnapshot * Simulator::TakeSimulatorSnapshot() const
{
    SimulatorSnapshot * pSnapshot = new SimulatorSnapshot();
    pSnapshot->stationSnapshot = ITSStationSnapshot(this);
    for (size_t iStation = 0; iStation < m_lstAllStations.size(); iStation++)
    {
        ITSStation* pStation = m_lstAllStations.at(iStation);
        pSnapshot->lstStations.push_back(pStation->TakeSnapshot());
    }
    for (size_t iSensor = 0; iSensor < m_lstAllSensors.size(); iSensor++)
    {
        ITSSensor* pSensor = m_lstAllSensors.at(iSensor);
        pSnapshot->lstSensors.push_back(pSensor->TakeSnapshot());
    }
    pSnapshot->uiNextEntityID = m_pGraph->GetNextEntityIDCurrent();

    pSnapshot->iRandCount = m_pGraph->GetNoiseSimulation()->GetRandCount();

    pSnapshot->iLastMessageID = m_pCommunicationRunner->GetLastMessageID();

    pSnapshot->dtBeginningTime = m_pCommunicationRunner->GetRunBeginningTime();
    pSnapshot->dtCurrentTime = m_pCommunicationRunner->GetRunCurrentTime();
    pSnapshot->dtRunEndTime = m_pCommunicationRunner->GetRunEndTime();

     return pSnapshot;
}

void Simulator::Restore(SimulatorSnapshot* pSnapshot)
{
    ITSStation::Restore(&pSnapshot->stationSnapshot);
    for (size_t iStation = 0; iStation < pSnapshot->lstStations.size(); iStation++)
    {
        m_lstAllStations.at(iStation)->Restore(pSnapshot->lstStations.at(iStation));
    }
    // Suppression des stations qui n'existaient pas au dernier point de sauvegarde
    for (size_t iStation = pSnapshot->lstStations.size(); iStation < m_lstAllStations.size(); iStation++)
    {
        ITSStation * pStation = m_lstAllStations.at(iStation);
        m_pGraph->RemoveEntity(pStation);
        delete pStation;
    }
    m_lstAllStations.resize(pSnapshot->lstStations.size());

    for (size_t iSensor = 0; iSensor < pSnapshot->lstSensors.size(); iSensor++)
    {
        m_lstAllSensors.at(iSensor)->Restore(pSnapshot->lstSensors.at(iSensor));
    }
    // Suppression des sensors qui n'existaient pas au dernier point de sauvegarde
    // rmq. : pas de delete des capteurs car fait lors de la destruction des stations correspondantes (elles ont l'owwnership dessus)
    m_lstAllSensors.resize(pSnapshot->lstSensors.size());

    m_pGraph->SetNextEntityID(pSnapshot->uiNextEntityID);

    m_pGraph->GetNoiseSimulation()->Restore(pSnapshot->iRandCount);

    m_pCommunicationRunner->SetLastMessageID(pSnapshot->iLastMessageID);

    m_pCommunicationRunner->SetRunBeginningTime(pSnapshot->dtBeginningTime);
    m_pCommunicationRunner->SetRunCurrentTime(pSnapshot->dtCurrentTime);
    m_pCommunicationRunner->SetRunEndTime(pSnapshot->dtRunEndTime);
}

void Simulator::ArbitrateVehicleChange()
{
    std::map<std::string, std::vector<int> >::iterator itroute =  m_mapChangeRoute.begin();
    for(itroute; itroute != m_mapChangeRoute.end(); itroute++)
    {
        boost::shared_ptr<Vehicule> pVehicle = m_pNetwork->GetVehiculeFromID(SystemUtil::ToInt32(itroute->first));
        if(!pVehicle)
            continue;
        std::vector<int> lstValue = itroute->second;
        std::vector<Tuyau*> lstTuyau;
        Tuyau* currentTuyau = NULL;
		bool bOk = true;
        for(size_t i = 0; i< lstValue.size(); i++)
        {
            currentTuyau = NULL;
            for(size_t j=0; j<m_pNetwork->m_LstTuyaux.size(); j++)
            {
                if(lstValue[i] == m_pNetwork->m_LstTuyaux[j]->GetID())
                {
                    currentTuyau = m_pNetwork->m_LstTuyaux[j];
                    lstTuyau.push_back(currentTuyau);
                    break;
                }
            }
            if(!currentTuyau)
            {
                bOk = false;
                break;
            }
        }
		if (bOk)
			pVehicle->AlterRoute(lstTuyau);
    }
	m_mapChangeRoute.clear();

    std::map<std::string, std::vector<int> >::iterator itlane =  m_mapChangeLane.begin();
    for(itlane; itlane != m_mapChangeLane.end(); itlane++)
    {
		boost::shared_ptr<Vehicule> pVehicle = m_pNetwork->GetVehiculeFromID(SystemUtil::ToInt32(itlane->first));
        if(!pVehicle)
            continue;
        std::vector<int> lstValue = itlane->second;
        std::map<int, int> bestLaneOccur;
        for(size_t i = 0; i< lstValue.size(); i++)
        {
            bestLaneOccur[lstValue[i]]++;
        }
        int bestLane = 0;
        int bestOccur = 0;
        std::map<int, int>::iterator itMap = bestLaneOccur.begin();
        for(itMap; itMap != bestLaneOccur.end(); itMap++)
        {
            if(itMap->second >= bestOccur)
                bestLane = itMap->first;
        }
        pVehicle->SetVoieDesiree(bestLane);
    }
	m_mapChangeLane.clear();

    std::map<std::string, std::vector<double> >::iterator itspeed =  m_mapSpeedTarget.begin();
    for(itspeed; itspeed != m_mapSpeedTarget.end(); itspeed++)
    {
		boost::shared_ptr<Vehicule> pVehicle = m_pNetwork->GetVehiculeFromID(SystemUtil::ToInt32(itspeed->first));
        if(!pVehicle)
            continue;
        double newSpeed = 0.0;
        std::vector<double> lstValue = itspeed->second;
        for(size_t i = 0; i< lstValue.size(); i++)
        {
            newSpeed += lstValue[i];
        }
        newSpeed = newSpeed/lstValue.size();
		double oldPos = pVehicle->GetPos(1);
        double newAcc = (newSpeed - pVehicle->GetVit(1)) / m_pNetwork->GetTimeStep();
        double newPos = pVehicle->GetPos(1) + newSpeed * m_pNetwork->GetTimeStep();

        std::vector<Tuyau*> Itinaries = m_pNetwork->ComputeItineraireComplet(*pVehicle->GetItineraire());
        std::vector<Tuyau*>::iterator itCurent = find(Itinaries.begin(), Itinaries.end(), pVehicle->GetLink(1));

        if(itCurent == Itinaries.end()) //Link not found in the itinary
        {
            continue;
        }

        VoieMicro* newVoie = pVehicle->GetVoie(1);
        Tuyau* newTuyau = NULL;
        for( itCurent; itCurent != Itinaries.end(); itCurent++)
        {
            if( newPos <= (*itCurent)->GetLength() )
            {
                newTuyau = (*itCurent);
                break;
            }
            else
            {
				newVoie = dynamic_cast<VoieMicro*>(pVehicle->CalculNextVoie(newVoie, m_pNetwork->m_dbInstSimu));
                newPos -= (*itCurent)->GetLength();
            }
            if(!newVoie)
                break;
        }

        if(!newVoie || !newTuyau)
            continue;

        //all is OK
        pVehicle->SetTuyau(newTuyau, m_pNetwork->m_dbInstSimu);
        pVehicle->SetVoie(newVoie);
        pVehicle->SetPos(newPos);
        pVehicle->SetVit(newSpeed);
        pVehicle->SetAcc(newAcc);
    }
	m_mapSpeedTarget.clear();

    std::map<std::string, std::vector<double> >::iterator itacc =  m_mapAccelerateDecelerate.begin();
    for(itacc; itacc != m_mapAccelerateDecelerate.end(); itacc++)
    {
		boost::shared_ptr<Vehicule> pVehicle = m_pNetwork->GetVehiculeFromID(SystemUtil::ToInt32(itacc->first));
        if(!pVehicle)
            continue;
        double accel = 0.0;
        std::vector<double> lstValue = itacc->second;
        for(size_t i = 0; i< lstValue.size(); i++)
        {
            accel += lstValue[i];
        }
        accel = accel/lstValue.size();
        double newSpeed = (accel * m_pNetwork->GetTimeStep()) + pVehicle->GetVit(1);
        double newPos = pVehicle->GetPos(1) + newSpeed * m_pNetwork->GetTimeStep();

        std::vector<Tuyau*> Itinaries = m_pNetwork->ComputeItineraireComplet(*pVehicle->GetItineraire());
        std::vector<Tuyau*>::iterator itCurent = find(Itinaries.begin(), Itinaries.end(), pVehicle->GetLink(1));

        if(itCurent == Itinaries.end()) //Link not found in the itinary
        {
            continue;
        }

		VoieMicro* newVoie = pVehicle->GetVoie(1);
        Tuyau* newTuyau = NULL;
        for( itCurent; itCurent != Itinaries.end(); itCurent++)
        {
            if( newPos <= (*itCurent)->GetLength() )
            {
                newTuyau = (*itCurent);
                break;
            }
            else
			{
				newVoie = dynamic_cast<VoieMicro*>(pVehicle->CalculNextVoie(newVoie, m_pNetwork->m_dbInstSimu));
                newPos -= (*itCurent)->GetLength();
            }
            if(!newVoie)
                break;
        }

        if(!newVoie || !newTuyau)
            continue;

        //all is OK
        pVehicle->SetTuyau(newTuyau, m_pNetwork->m_dbInstSimu);
        pVehicle->SetVoie(newVoie);
        pVehicle->SetPos(newPos);
        pVehicle->SetAcc(accel);
        pVehicle->SetVit(newSpeed);
    }
	m_mapAccelerateDecelerate.clear();

}

template void SimulatorSnapshot::serialize(boost::archive::xml_woarchive & ar, const unsigned int version);
template void SimulatorSnapshot::serialize(boost::archive::xml_wiarchive & ar, const unsigned int version);

template<class Archive>
void SimulatorSnapshot::serialize(Archive & ar, const unsigned int version)
{
    ar & BOOST_SERIALIZATION_NVP(stationSnapshot);
    ar & BOOST_SERIALIZATION_NVP(lstStations);
    ar & BOOST_SERIALIZATION_NVP(lstSensors);
    ar & BOOST_SERIALIZATION_NVP(iRandCount);
    ar & BOOST_SERIALIZATION_NVP(uiNextEntityID);
    ar & BOOST_SERIALIZATION_NVP(iLastMessageID);
    ar & BOOST_SERIALIZATION_NVP(dtBeginningTime);
    ar & BOOST_SERIALIZATION_NVP(dtCurrentTime);
    ar & BOOST_SERIALIZATION_NVP(dtRunEndTime);
}


template void Simulator::VehiclePattern::serialize(boost::archive::xml_woarchive & ar, const unsigned int version);
template void Simulator::VehiclePattern::serialize(boost::archive::xml_wiarchive & ar, const unsigned int version);

template<class Archive>
void Simulator::VehiclePattern::serialize(Archive & ar, const unsigned int version)
{
    ar & BOOST_SERIALIZATION_NVP(pDynamic);
    ar & BOOST_SERIALIZATION_NVP(pTypeVeh);
    ar & BOOST_SERIALIZATION_NVP(dbPenetrationRate);
}


template void Simulator::serialize(boost::archive::xml_woarchive & ar, const unsigned int version);
template void Simulator::serialize(boost::archive::xml_wiarchive & ar, const unsigned int version);

template<class Archive>
void Simulator::serialize(Archive & ar, const unsigned int version)
{
    ar & BOOST_SERIALIZATION_BASE_OBJECT_NVP(ITSStation);

    ar & BOOST_SERIALIZATION_NVP(m_lstAllStations);
    ar & BOOST_SERIALIZATION_NVP(m_lstAllSensors);
    ar & BOOST_SERIALIZATION_NVP(m_lstAllApplications);
    ar & BOOST_SERIALIZATION_NVP(m_lstExternalLibraries);
    ar & BOOST_SERIALIZATION_NVP(m_pGraph);
    ar & BOOST_SERIALIZATION_NVP(m_pCommunicationRunner);
    ar & BOOST_SERIALIZATION_NVP(m_lstDynamicPatterns);
    ar & BOOST_SERIALIZATION_NVP(m_mapChangeRoute);
    ar & BOOST_SERIALIZATION_NVP(m_mapChangeLane);
    ar & BOOST_SERIALIZATION_NVP(m_mapSpeedTarget);
    ar & BOOST_SERIALIZATION_NVP(m_mapAccelerateDecelerate);
    //ar & BOOST_SERIALIZATION_NVP(m_pCurrentSimulationSnapshot);

    std::string writerStr;
    if (Archive::is_saving::value)
    {
        // fichier résultat : on ferme le fichier actuel et on récupère son contenu pour le sauvegarder dans 
        // le fichier de sérialisation
        Suspend();

        std::ifstream ifs((m_pNetwork->GetPrefixOutputFiles()+"_SymuCom.xml").c_str(), std::ios::in | std::ios::binary); // lecture en binaire sinon les CR LF deviennent LF
        std::stringstream ss;
        ss << ifs.rdbuf();
        ifs.close();
        writerStr = ss.str();
    }
    ar & BOOST_SERIALIZATION_NVP(writerStr);

    // on réinitialise les autres membres déductibles des autres et on rouvre le fichier de résultat
    if (Archive::is_loading::value)
    {
        ReInit(writerStr);
    }
}

