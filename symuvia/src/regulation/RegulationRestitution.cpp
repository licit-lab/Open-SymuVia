#include "stdafx.h"
#include "RegulationRestitution.h"

#include "RegulationCondition.h"
#include "PythonUtils.h"

#include "../reseau.h"
#include "../TraceDocTrafic.h"
#include "../XMLDocTrafic.h"
#include "../Logger.h"

#include "Xerces/XMLUtil.h"

#include <xercesc/dom/DOMNode.hpp>
#include <xercesc/dom/DOMElement.hpp>
#include <xercesc/dom/DOMDocument.hpp>

#include <boost/python/exec.hpp>
#include <boost/python/extract.hpp>

using namespace boost::python;

XERCES_CPP_NAMESPACE_USE


RegulationRestitution::RegulationRestitution(void)
{
}


RegulationRestitution::~RegulationRestitution(void)
{
}

std::string RegulationRestitution::getBuiltinModuleName()
{
    return SCRIPTS_RESTITUTION_MODULE_NAME;
}

void RegulationRestitution::initialize(Reseau * pReseau, DOMNode * pNodeRestitution, const std::vector<RegulationCondition*> & lstConditions)
{
    // Appel à la méthode mère
    RegulationElement::initialize(pReseau, pNodeRestitution);

    // Dans le cas des restitutions, on met toutes les conditions (on porura ainsi se servir de tous ces contextes pour la restitution)
    m_LstConditions.insert(m_LstConditions.end(), lstConditions.begin(), lstConditions.end());
}


// exécution de l'action de restitution
void RegulationRestitution::restitution(TraceDocTrafic * pDocTrafic, const std::string & briqueID)
{
    try
    {
        object globals = m_pReseau->GetPythonUtils()->getMainModule()->attr("__dict__");
        dict locals;

        // Construction de la liste des contextes des conditions associées
        dict conditions;
        for(size_t i = 0; i < m_LstConditions.size(); i++)
        {
            conditions[m_LstConditions[i]->getID()] = m_LstConditions[i]->getContext();
        }

        locals["conditions"] = conditions;
        locals["network"] = ptr(m_pReseau);
        locals["parameters"] = m_FuncParamDict;
        locals["context"] = m_ContextDict;
        object result = eval((getModuleName() + "." + m_strFunc + "(conditions,context,network,parameters)\n").c_str(), globals, locals);

        // construction du noeud à partir du resultat
        if(!result.is_none() && pDocTrafic->GetXMLDocTrafic())
        {
            DOMElement* pNodeRegulation = pDocTrafic->GetXMLDocTrafic()->getTemporaryInstantXMLDocument()->createElement(XS("REGULATION"));
            pNodeRegulation->setAttribute(XS("id"), XS(briqueID.c_str()));
            m_pReseau->GetPythonUtils()->buildNodeFromDict(pNodeRegulation, extract<dict>(result));
            pDocTrafic->AddSimuRegulation(pNodeRegulation);
        }
    }
    catch( error_already_set )
    {
        m_pReseau->log() << Logger::Error << m_pReseau->GetPythonUtils()->getPythonErrorString() << std::endl;
        m_pReseau->log() << Logger::Info; // rebascule en mode INFO pour ne pas avoir à reprendre tous les appels aux log en précisant que c'est des INFO. à supprimer si on reprend tous les appels au log.
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Sérialisation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////
template void RegulationRestitution::serialize(boost::archive::xml_woarchive & ar, const unsigned int version);
template void RegulationRestitution::serialize(boost::archive::xml_wiarchive & ar, const unsigned int version);

template<class Archive>
void RegulationRestitution::serialize(Archive & ar, const unsigned int version)
{
    ar & BOOST_SERIALIZATION_BASE_OBJECT_NVP(RegulationAction);
}