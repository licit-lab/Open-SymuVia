#include "stdafx.h"
#include "ITSExternalStation.h"

#include "ITS/ExternalLibraries/ITSExternalLibrary.h"

#include "reseau.h"
#include "Logger.h"

#include "Xerces/XMLUtil.h"

#include <Communication/CommunicationRunner.h>


ITSExternalStation::ITSExternalStation()
{
    m_pExtLib = NULL;
}

ITSExternalStation::ITSExternalStation(const ITSExternalStation &StationCopy)
    :ITSStation(StationCopy.GetNetwork(), StationCopy.m_strLabel, StationCopy.m_strStationType, StationCopy.m_ptCoordinates,
                StationCopy.m_pCommunicationRunner->GetGraph(), StationCopy.m_bCanSend, StationCopy.m_bCanReceive, StationCopy.m_dbRange, StationCopy.m_dbAlphaDown, StationCopy.m_dbBetaDown)
{
    m_pExtLib = StationCopy.m_pExtLib;
    m_customParams = StationCopy.m_customParams;
}

ITSExternalStation::ITSExternalStation(Reseau* pNetwork, const std::string& strID, const std::string& strType, const SymuCom::Point& pPoint, SymuCom::Graph* pGraph, bool bCanSend, bool bCanReceive,
    double dbEntityRange, double dbAlphaDown, double dbBetaDown, ITSExternalLibrary * pExtLib)
    : ITSStation(pNetwork, strID, strType, pPoint, pGraph, bCanSend, bCanReceive, dbEntityRange, dbAlphaDown, dbBetaDown)
{
    m_pExtLib = pExtLib;
}

ITSExternalStation::~ITSExternalStation()
{

}

bool ITSExternalStation::TreatMessageData(SymuCom::Message *pMessage)
{
    return m_pExtLib->TreatStationMessageData(this, m_pNetwork, m_customParams, pMessage, m_pNetwork->GetLogger());
}

bool ITSExternalStation::Run(const boost::posix_time::ptime &dtTimeStart, double dbTimeDuration)
{
    return m_pExtLib->RunStation(this, m_pNetwork, m_customParams, dtTimeStart, dbTimeDuration, m_pNetwork->GetLogger());
}

bool ITSExternalStation::LoadFromXML(XERCES_CPP_NAMESPACE::DOMDocument *pDoc, XERCES_CPP_NAMESPACE::DOMNode *pXMLNode)
{
    m_customParams = XMLUtil::getOuterXml(pXMLNode);
    return true;
}

ITSStation *ITSExternalStation::CreateCopy(const std::string& strID)
{
    ITSExternalStation* pNewStation = new ITSExternalStation(m_pNetwork, strID, m_strStationType, m_ptCoordinates, m_pCommunicationRunner->GetGraph(), m_bCanSend, m_bCanReceive, m_dbRange);
    pNewStation->m_pExtLib = m_pExtLib;
    pNewStation->m_customParams = m_customParams;

    return pNewStation;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Sérialisation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////
template void ITSExternalStation::serialize(boost::archive::xml_woarchive & ar, const unsigned int version);
template void ITSExternalStation::serialize(boost::archive::xml_wiarchive & ar, const unsigned int version);

template<class Archive>
void ITSExternalStation::serialize(Archive& ar, const unsigned int version)
{
    ar & BOOST_SERIALIZATION_BASE_OBJECT_NVP(ITSStation);
    ar & BOOST_SERIALIZATION_NVP(m_pExtLib);
    ar & BOOST_SERIALIZATION_NVP(m_customParams);
}
