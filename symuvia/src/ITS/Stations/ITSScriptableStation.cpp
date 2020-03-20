#include "stdafx.h"
#include "ITSScriptableStation.h"

#include "reseau.h"
#include "Logger.h"
#include "Xerces/XMLUtil.h"
#include "regulation/PythonUtils.h"
#include "ITS/Stations/DerivedClass/Simulator.h"

#include <Communication/CommunicationRunner.h>
#include <boost/python/exec.hpp>
#include <boost/python/extract.hpp>

XERCES_CPP_NAMESPACE_USE

using namespace boost::python;

ITSScriptableStation::ITSScriptableStation()
{

}

ITSScriptableStation::ITSScriptableStation(const ITSScriptableStation &StationCopy)
    :ITSStation(StationCopy.GetNetwork(), StationCopy.m_strLabel, StationCopy.m_strStationType, StationCopy.m_ptCoordinates,
                StationCopy.m_pCommunicationRunner->GetGraph(), StationCopy.m_bCanSend, StationCopy.m_bCanReceive, StationCopy.m_dbRange, StationCopy.m_dbAlphaDown, StationCopy.m_dbBetaDown)
{

}

ITSScriptableStation::ITSScriptableStation(Reseau* pNetwork, const std::string& strID, const std::string& strType, const SymuCom::Point& pPoint, SymuCom::Graph* pGraph, bool bCanSend, bool bCanReceive,
                       double dbEntityRange, double dbAlphaDown, double dbBetaDown)
    : ITSStation(pNetwork, strID, strType, pPoint, pGraph, bCanSend, bCanReceive, dbEntityRange, dbAlphaDown, dbBetaDown)
{

}

ITSScriptableStation::~ITSScriptableStation()
{

}

bool ITSScriptableStation::TreatMessageData(SymuCom::Message *pMessage)
{
    try
    {
        object globals = m_pScriptNetwork->GetPythonUtils()->getMainModule()->attr("__dict__");
        dict locals;

		locals["network"] = ptr(m_pScriptNetwork);
        locals["parameters"] = m_dictParameters;
        locals["message"] = ptr(pMessage);
        exec((m_strModuleName + ".TreatMessageData(network,parameters,message)\n").c_str(), globals, locals);
    }
    catch( error_already_set )
    {
        m_pScriptNetwork->log() << Logger::Error << m_pScriptNetwork->GetPythonUtils()->getPythonErrorString() << std::endl;
		m_pScriptNetwork->log() << Logger::Info; // rebascule en mode INFO pour ne pas avoir à reprendre tous les appels aux log en précisant que c'est des INFO. à supprimer si on reprend tous les appels au log.
        return false;
    }

    return true;
}

bool ITSScriptableStation::Run(const boost::posix_time::ptime &dtTimeStart, double dbTimeDuration)
{
    try
    {
        object globals = m_pScriptNetwork->GetPythonUtils()->getMainModule()->attr("__dict__");
        dict locals;

		locals["network"] = ptr(m_pScriptNetwork);
        locals["parameters"] = m_dictParameters;
        locals["timeStart"] = dtTimeStart;
        locals["timeDuration"] = dbTimeDuration;
        exec((m_strModuleName + ".Run(network,parameters,timeStart,timeDuration)\n").c_str(), globals, locals);
    }
    catch (error_already_set)
    {
        m_pScriptNetwork->log() << Logger::Error << m_pScriptNetwork->GetPythonUtils()->getPythonErrorString() << std::endl;
        m_pScriptNetwork->log() << Logger::Info; // rebascule en mode INFO pour ne pas avoir à reprendre tous les appels aux log en précisant que c'est des INFO. à supprimer si on reprend tous les appels au log.
        return false;
    }

    return true;
}

bool ITSScriptableStation::LoadFromXML(XERCES_CPP_NAMESPACE::DOMDocument *pDoc, XERCES_CPP_NAMESPACE::DOMNode *pXMLNode)
{
    // build a dictionnary with all parameters for the element
    if(pXMLNode)
        m_pScriptNetwork->GetPythonUtils()->buildDictFromNode(pXMLNode, m_dictParameters);
    m_dictParameters["this"] = ptr(this);

    return true;
}

ITSStation *ITSScriptableStation::CreateCopy(const std::string& strID)
{
	ITSScriptableStation* pNewVeh = new ITSScriptableStation(m_pScriptNetwork, strID, m_strStationType, m_ptCoordinates, m_pCommunicationRunner->GetGraph(), m_bCanSend, m_bCanReceive, m_dbRange);
    pNewVeh->m_strModuleName = m_strModuleName;
    pNewVeh->m_pScriptNetwork = m_pScriptNetwork;
    pNewVeh->m_dictParameters = m_dictParameters.copy();
    pNewVeh->m_dictParameters["this"] = ptr(pNewVeh);

    return pNewVeh;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Sérialisation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////
template void ITSScriptableStation::serialize(boost::archive::xml_woarchive & ar, const unsigned int version);
template void ITSScriptableStation::serialize(boost::archive::xml_wiarchive & ar, const unsigned int version);

template<class Archive>
void ITSScriptableStation::serialize(Archive& ar, const unsigned int version)
{
    ar & BOOST_SERIALIZATION_BASE_OBJECT_NVP(ITSStation);
    ar & BOOST_SERIALIZATION_BASE_OBJECT_NVP(ITSScriptableElement);
}
