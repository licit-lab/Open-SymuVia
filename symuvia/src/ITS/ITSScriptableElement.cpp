#include "stdafx.h"
#include "ITSScriptableElement.h"

#include "regulation/PythonUtils.h"

#include "reseau.h"
#include "SystemUtil.h"
#include "Logger.h"

#include "Xerces/XMLUtil.h"

#include <xercesc/dom/DOMNode.hpp>

#include <boost/python/exec.hpp>

using namespace boost::python;

XERCES_CPP_NAMESPACE_USE


ITSScriptableElement::ITSScriptableElement()
{
    m_pScriptNetwork = NULL;
}


ITSScriptableElement::~ITSScriptableElement()
{
}

const std::string& ITSScriptableElement::GetScriptName()
{
    return m_strModuleName;
}

bool ITSScriptableElement::initialize(Reseau * pReseau, const std::string& strModuleName)
{
    m_pScriptNetwork = pReseau;
    m_strModuleName = strModuleName;

    // Load of the script
    try
    {
        object globals = pReseau->GetPythonUtils()->getMainModule()->attr("__dict__");
        std::string scriptsPath = SystemUtil::GetFilePath(const_cast<char*>(SCRIPTS_DIRECTORY.c_str()));

        exec("import imp\n", globals);
        pReseau->GetPythonUtils()->importModule(globals, m_strModuleName, scriptsPath + DIRECTORY_SEPARATOR + ITS_SCRIPTS_DIRECTORY + DIRECTORY_SEPARATOR + m_strModuleName + ".py");
    }
    catch( error_already_set )
    {
        *(pReseau->GetLogger()) << Logger::Error << pReseau->GetPythonUtils()->getPythonErrorString() << std::endl;
		*(pReseau->GetLogger()) << Logger::Info;
        return false;
    }
    return true;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Sérialisation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////
template void ITSScriptableElement::serialize(boost::archive::xml_woarchive & ar, const unsigned int version);
template void ITSScriptableElement::serialize(boost::archive::xml_wiarchive & ar, const unsigned int version);

template<class Archive>
void ITSScriptableElement::serialize(Archive& ar, const unsigned int version)
{
    ar & BOOST_SERIALIZATION_NVP(m_pScriptNetwork);
    ar & BOOST_SERIALIZATION_NVP(m_strModuleName);
    if (Archive::is_saving::value)
    {
        if (m_dictParameters.has_key("this")) {
            boost::python::delitem(m_dictParameters, boost::python::str("this"));
        }
    }
    ar & BOOST_SERIALIZATION_NVP(m_dictParameters);
    if (Archive::is_loading::value)
    {
        m_dictParameters["this"] = ptr(this);
        initialize(m_pScriptNetwork, m_strModuleName);
    }
}

