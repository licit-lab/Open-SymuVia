#include "stdafx.h"
#include "RegulationElement.h"

#include "PythonUtils.h"

#include "../reseau.h"
#include "../SystemUtil.h"
#include "../Logger.h"

#include "../Xerces/XMLUtil.h"

#include <xercesc/dom/DOMNode.hpp>

#include <boost/python/exec.hpp>

using namespace boost::python;

XERCES_CPP_NAMESPACE_USE


RegulationElement::RegulationElement(void)
{
    m_pReseau = NULL;
}


RegulationElement::~RegulationElement(void)
{
}

std::string RegulationElement::getModuleName()
{
    if(!m_strModule.compare(""))
    {
        return getBuiltinModuleName();
    }
    else
    {
        return SystemUtil::GetFileName(m_strModule);
    }
}

void RegulationElement::initialize(Reseau * pReseau, DOMNode * pNodeElement)
{
    m_pReseau = pReseau;

    // récupération du nom de fonction correspondant à la fonction de l'élément
    GetXmlAttributeValue(pNodeElement, "fonction", m_strFunc, pReseau->GetLogger());

    // récupération du chemin du module contenant la fonction de l'élément
    if(GetXmlAttributeValue(pNodeElement, "module", m_strModule, pReseau->GetLogger()))
    {
        // Chargement du module
        try
        {
            object globals = m_pReseau->GetPythonUtils()->getMainModule()->attr("__dict__");
            exec("import imp\n", globals);
            m_pReseau->GetPythonUtils()->importModule(globals, getModuleName(), m_strModule);
        }
        catch( error_already_set )
        {
            m_pReseau->log() << Logger::Error << m_pReseau->GetPythonUtils()->getPythonErrorString() << std::endl;
            m_pReseau->log() << Logger::Info; // rebascule en mode INFO pour ne pas avoir à reprendre tous les appels aux log en précisant que c'est des INFO. à supprimer si on reprend tous les appels au log.
        }
    }

    // construction du dictionnaire correspondant aux paramètres de la fonction de l'élément
    // récupération du sous noedu PARAMETRES
    DOMNode * pNodeParametres = m_pReseau->GetXMLUtil()->SelectSingleNode("PARAMETRES", pNodeElement->getOwnerDocument(), (DOMElement*)pNodeElement);
    if(pNodeParametres)
    {
        m_pReseau->GetPythonUtils()->buildDictFromNode(pNodeParametres, m_FuncParamDict);
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Sérialisation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////
template void RegulationElement::serialize(boost::archive::xml_woarchive & ar, const unsigned int version);
template void RegulationElement::serialize(boost::archive::xml_wiarchive & ar, const unsigned int version);

template<class Archive>
void RegulationElement::serialize(Archive & ar, const unsigned int version)
{
    ar & BOOST_SERIALIZATION_NVP(m_pReseau);
    ar & BOOST_SERIALIZATION_NVP(m_strFunc);
    ar & BOOST_SERIALIZATION_NVP(m_strModule);
    ar & BOOST_SERIALIZATION_NVP(m_FuncParamDict);
    ar & BOOST_SERIALIZATION_NVP(m_ContextDict);
}