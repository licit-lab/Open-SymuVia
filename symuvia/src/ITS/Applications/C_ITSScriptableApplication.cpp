#include "stdafx.h"
#include "C_ITSScriptableApplication.h"

#include "Xerces/XMLUtil.h"
#include "ITS/Stations/ITSStation.h"
#include "regulation/PythonUtils.h"

#include <boost/python/exec.hpp>
#include <boost/python/extract.hpp>

XERCES_CPP_NAMESPACE_USE

using namespace boost::python;

C_ITSScriptableApplication::C_ITSScriptableApplication()
{

}

C_ITSScriptableApplication::C_ITSScriptableApplication(const std::string strID, const std::string strType) : C_ITSApplication(strID, strType)
{

}

C_ITSScriptableApplication::~C_ITSScriptableApplication()
{

}

bool C_ITSScriptableApplication::RunApplication(SymuCom::Message *pMessage, ITSStation *pStation)
{
    try
    {
        object globals = m_pScriptNetwork->GetPythonUtils()->getMainModule()->attr("__dict__");
        dict locals;

		locals["network"] = ptr(m_pScriptNetwork);
        locals["parameters"] = m_dictParameters;
        locals["message"] = ptr(pMessage);
		locals["station"] = ptr(pStation);
        exec((m_strModuleName + ".RunApplication(network,parameters,message,station)\n").c_str(), globals, locals);
    }
    catch( error_already_set )
    {
        m_pScriptNetwork->log() << Logger::Error << m_pScriptNetwork->GetPythonUtils()->getPythonErrorString() << std::endl;
        m_pScriptNetwork->log() << Logger::Info; // rebascule en mode INFO pour ne pas avoir à reprendre tous les appels aux log en précisant que c'est des INFO. à supprimer si on reprend tous les appels au log.
        return false;
    }

    return true;
}

bool C_ITSScriptableApplication::LoadFromXML(XERCES_CPP_NAMESPACE::DOMDocument *pDoc, XERCES_CPP_NAMESPACE::DOMNode *pXMLNode, Reseau *pNetwork)
{
    // build a dictionnary with all parameters for the element
    if(pXMLNode)
        pNetwork->GetPythonUtils()->buildDictFromNode(pXMLNode, m_dictParameters);
    m_dictParameters["this"] = ptr(this);

    return true;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Sérialisation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////
template void C_ITSScriptableApplication::serialize(boost::archive::xml_woarchive & ar, const unsigned int version);
template void C_ITSScriptableApplication::serialize(boost::archive::xml_wiarchive & ar, const unsigned int version);

template<class Archive>
void C_ITSScriptableApplication::serialize(Archive& ar, const unsigned int version)
{
    ar & BOOST_SERIALIZATION_BASE_OBJECT_NVP(C_ITSApplication);
    ar & BOOST_SERIALIZATION_BASE_OBJECT_NVP(ITSScriptableElement);
}
