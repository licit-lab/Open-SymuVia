#include "stdafx.h"
#include "RegulationAction.h"

#include "RegulationCondition.h"
#include "PythonUtils.h"

#include "../reseau.h"
#include "../SystemUtil.h"
#include "../Logger.h"

#include <xercesc/dom/DOMNode.hpp>

#include <boost/python/exec.hpp>

#include <boost/serialization/vector.hpp>

using namespace boost::python;

XERCES_CPP_NAMESPACE_USE


RegulationAction::RegulationAction(void)
{
}


RegulationAction::~RegulationAction(void)
{
}

std::string RegulationAction::getBuiltinModuleName()
{
    return SCRIPTS_ACTION_MODULE_NAME;
}

void RegulationAction::initialize(Reseau * pReseau, DOMNode * pNodeAction, const std::vector<RegulationCondition*> & lstConditions)
{
    // Appel à la méthode mère
    RegulationElement::initialize(pReseau, pNodeAction);

    // récupération de la liste des conditions nécessaires à l'action
    std::string strTmp;
    GetXmlAttributeValue(pNodeAction, "conditions", strTmp, pReseau->GetLogger());
    std::deque<std::string> lstConditionsID = SystemUtil::split(strTmp, ' ');
    for(size_t iCond = 0; iCond < lstConditionsID.size(); iCond++)
    {
        for(size_t jCond = 0; jCond < lstConditions.size(); jCond++)
        {
            if(!lstConditions[jCond]->getID().compare(lstConditionsID[iCond]))
            {
                m_LstConditions.push_back(lstConditions[jCond]);
                break;
            }
        }
    }
}

// test préliminaire à l'exécution de l'action
bool RegulationAction::test()
{
    // on fait, sauf si une des conditions est fausse (si pas de condition : on fait)
    bool bDo = true;
    for(size_t i = 0; i < m_LstConditions.size() && bDo; i++)
    {
        bDo = m_LstConditions[i]->isRemplie();
    }
    return bDo;
}

// exécution de l'action étant donné les conditions passées en paramètres
void RegulationAction::execute()
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
        exec((getModuleName() + "." + m_strFunc + "(conditions,context,network,parameters)\n").c_str(), globals, locals);
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
template void RegulationAction::serialize(boost::archive::xml_woarchive & ar, const unsigned int version);
template void RegulationAction::serialize(boost::archive::xml_wiarchive & ar, const unsigned int version);

template<class Archive>
void RegulationAction::serialize(Archive & ar, const unsigned int version)
{
    ar & BOOST_SERIALIZATION_BASE_OBJECT_NVP(RegulationElement);

    ar & BOOST_SERIALIZATION_NVP(m_LstConditions);
}