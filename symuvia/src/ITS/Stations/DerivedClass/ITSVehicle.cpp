#include "stdafx.h"
#include "ITSVehicle.h"

#include "reseau.h"
#include "Logger.h"
#include "ITS/Applications/C_ITSApplication.h"
#include "Xerces/XMLUtil.h"

#include <Communication/CommunicationRunner.h>

XERCES_CPP_NAMESPACE_USE

ITSVehicle::ITSVehicle()
{

}

ITSVehicle::ITSVehicle(Vehicule *pParent, Reseau* pNetwork, const std::string &strID, const std::string &strType, const SymuCom::Point &pPoint, SymuCom::Graph *pGraph,
                       bool bCanSend, bool bCanReceive, double dbEntityRange, double dbAlphaDown, double dbBetaDown)
    : ITSStation(pNetwork, strID, strType, pPoint, pGraph, bCanSend, bCanReceive, dbEntityRange, dbAlphaDown, dbBetaDown)
{
    SetDynamicParent(pParent);
}

ITSVehicle::~ITSVehicle()
{

}

bool ITSVehicle::Run(const boost::posix_time::ptime &dtTimeStart, double dbTimeDuration)
{
    //nothing to do
    return true;
}

bool ITSVehicle::TreatMessageData(SymuCom::Message *pMessage)
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

        //Data for TraficLightMessage must be the form Action;TraficLightMessage/InTronc;ID_first_TronconIn/OutTronc;ID_first_TronconOut/Pos;12.156/ActualColor;Red/NextPhase;18[/NextPhase;14][/InTronc;ID_second_TronconIn/OutTronc;ID_second_TronconOut/Pos;6.425/ActualColor;Green/NextPhase;12[/NextPhase;17]]
        if(lstAtIAction[1] == "TraficLightMessage")
        {
            C_ITSApplication* pAppli = GetApplicationByType("GLOSA");
            if(!pAppli){
                *(m_pNetwork->GetLogger()) << Logger::Error << "Error while reading the message ID " << pMessage->GetMessageID() << " : No application found to handle TraficLightMessage for Vehicle " << GetLabel() << " !" << std::endl;
                return false;
            }
            bProcessOk = pAppli->RunApplication(pMessage, this);
        }else{
            *(m_pNetwork->GetLogger()) << Logger::Error << "Error while reading the message ID " << pMessage->GetMessageID() << " : the action " << lstAtIAction[1] << " is not reconized !" << std::endl;
            return false;
        }
    }

    return bProcessOk;
}

bool ITSVehicle::LoadFromXML(DOMDocument *pDoc, DOMNode *pXMLNode)
{
    //Nothing to do
    return true;
}

ITSStation *ITSVehicle::CreateCopy(const std::string& strID)
{
    ITSVehicle* pNewStation = new ITSVehicle(NULL, m_pNetwork, strID, m_strStationType, m_ptCoordinates, m_pCommunicationRunner->GetGraph(), m_bCanSend, m_bCanReceive, m_dbRange);

    return pNewStation;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Sérialisation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////
template void ITSVehicle::serialize(boost::archive::xml_woarchive & ar, const unsigned int version);
template void ITSVehicle::serialize(boost::archive::xml_wiarchive & ar, const unsigned int version);

template<class Archive>
void ITSVehicle::serialize(Archive& ar, const unsigned int version)
{
    ar & BOOST_SERIALIZATION_BASE_OBJECT_NVP(ITSStation);
}
