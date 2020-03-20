#include "stdafx.h"
#include "RegulationCondition.h"

#include "PythonUtils.h"

#include "../reseau.h"
#include "../SystemUtil.h"
#include "../Logger.h"

#include <xercesc/dom/DOMNode.hpp>

#include <boost/python/exec.hpp>
#include <boost/python/extract.hpp>


using namespace boost::python;

XERCES_CPP_NAMESPACE_USE


RegulationCondition::RegulationCondition(void)
{
    m_bRemplie = false;
}


RegulationCondition::~RegulationCondition(void)
{
}

std::string RegulationCondition::getBuiltinModuleName()
{
    return SCRIPTS_CONDITION_MODULE_NAME;
}

void RegulationCondition::initialize(Reseau * pReseau, DOMNode * pNodeCondition)
{
    // Appel à la méthode mère
    RegulationElement::initialize(pReseau, pNodeCondition);

    // récupération du nom de fonction correspondant à la fonction de la condition
    GetXmlAttributeValue(pNodeCondition, "id", m_strID, pReseau->GetLogger());
}

// Test de la condition
void RegulationCondition::test(dict capteurs)
{
    try
    {
        object globals = m_pReseau->GetPythonUtils()->getMainModule()->attr("__dict__");
        dict locals;
        locals["sensors"] = capteurs;
        locals["network"] = ptr(m_pReseau);
        locals["parameters"] = m_FuncParamDict;
        locals["context"] = m_ContextDict;
        object result = eval((getModuleName() + "." + m_strFunc + "(sensors,context,network,parameters)\n").c_str(), globals, locals);
        m_bRemplie = extract<bool>(result);
    }
    catch( error_already_set )
    {
        m_pReseau->log() << Logger::Error << m_pReseau->GetPythonUtils()->getPythonErrorString() << std::endl;
        m_pReseau->log() << Logger::Info; // rebascule en mode INFO pour ne pas avoir à reprendre tous les appels aux log en précisant que c'est des INFO. à supprimer si on reprend tous les appels au log.
        // Si une exception survient, la fonction condition est mal définie et mal exécutée : on ne prendra pas d'action
        m_bRemplie = false;
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Sérialisation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////
template void RegulationCondition::serialize(boost::archive::xml_woarchive & ar, const unsigned int version);
template void RegulationCondition::serialize(boost::archive::xml_wiarchive & ar, const unsigned int version);

template<class Archive>
void RegulationCondition::serialize(Archive & ar, const unsigned int version)
{
    ar & BOOST_SERIALIZATION_BASE_OBJECT_NVP(RegulationElement);

    ar & BOOST_SERIALIZATION_NVP(m_strID);
}