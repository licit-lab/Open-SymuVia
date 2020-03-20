#include "stdafx.h"
#include "RegulationModule.h"

#include "PythonUtils.h"
#include "RegulationBrique.h"

#include "../reseau.h"

#include "../Xerces/XMLUtil.h"

#include <xercesc/dom/DOMNode.hpp>
#include <xercesc/dom/DOMNodeList.hpp>

#include <boost/serialization/vector.hpp>

using namespace boost::python;

XERCES_CPP_NAMESPACE_USE

RegulationModule::RegulationModule(void)
{
    m_pReseau = NULL;
    m_pBriqueActive = NULL;
}

RegulationModule::RegulationModule(Reseau * pReseau)
{
    m_pReseau = pReseau;
    m_pBriqueActive = NULL;
}


RegulationModule::~RegulationModule(void)
{
    if(m_LstBriquesRegulation.size() > 0)
    {
        m_pReseau->GetPythonUtils()->lockChildInterpreter();
        for(size_t i = 0; i < m_LstBriquesRegulation.size(); i++)
        {
            delete m_LstBriquesRegulation[i];
        }
        m_pReseau->GetPythonUtils()->unlockChildInterpreter();
    }
}

// exécute les briques de régulation pour le pas de temps écoulé
void RegulationModule::run()
{
    if(m_LstBriquesRegulation.size() > 0)
    {
        m_pReseau->GetPythonUtils()->enterChildInterpreter();
        // traitement de chaque brique
        for(size_t iBrique = 0; iBrique < m_LstBriquesRegulation.size(); iBrique++)
        {
            m_pBriqueActive = m_LstBriquesRegulation[iBrique];
            m_LstBriquesRegulation[iBrique]->run();
            m_pBriqueActive = NULL;
        }
        m_pReseau->GetPythonUtils()->exitChildInterpreter();
    }
}


// restitution des sorties éventuelles dans le fichier de trafic
void RegulationModule::restitution(TraceDocTrafic * pDocTrafic)
{
    if(m_LstBriquesRegulation.size() > 0)
    {
        m_pReseau->GetPythonUtils()->enterChildInterpreter();
        // traitement de chaque brique
        for(size_t iBrique = 0; iBrique < m_LstBriquesRegulation.size(); iBrique++)
        {
            m_LstBriquesRegulation[iBrique]->restitution(pDocTrafic);
        }
        m_pReseau->GetPythonUtils()->exitChildInterpreter();
    }
}

// Instancie une nouvelle brique de régulation
void RegulationModule::addBrique(DOMNode * pNodeBrique)
{
    m_pReseau->GetPythonUtils()->enterChildInterpreter();

    // Récupération de l'ID de la brique
    std::string strTmp;
    GetXmlAttributeValue(pNodeBrique, "id", strTmp, m_pReseau->GetLogger());

    // Récupération du taux de respect de la consigne
    double dbTmp = 1.0;
    GetXmlAttributeValue(pNodeBrique, "taux_respect_consigne", dbTmp, m_pReseau->GetLogger());

    // Instanciation de la nouvelle brique de régulation
    RegulationBrique * pBrique = new RegulationBrique(m_pReseau, strTmp, dbTmp);
    m_LstBriquesRegulation.push_back(pBrique);

    // Ajout de détecteurs
    DOMNode * pXMLNodeDetecteurs = m_pReseau->GetXMLUtil()->SelectSingleNode(".//DETECTEURS", pNodeBrique->getOwnerDocument(), (DOMElement*)pNodeBrique);

    if(pXMLNodeDetecteurs)
    {
		XMLSize_t counti = pXMLNodeDetecteurs->getChildNodes()->getLength();
		for(XMLSize_t i=0; i<counti;i++)
        {      
			DOMNode *pXMLDetecteur = pXMLNodeDetecteurs->getChildNodes()->item(i);
			if (pXMLDetecteur->getNodeType() != DOMNode::ELEMENT_NODE) continue;

            // Lecture de l'identifiant du contrôleur
            pBrique->addDetecteur(pXMLDetecteur);
        }
    }

    // Ajout de déclencheurs
    DOMNode * pXMLNodeDeclencheurs = m_pReseau->GetXMLUtil()->SelectSingleNode(".//DECLENCHEURS", pNodeBrique->getOwnerDocument(), (DOMElement*)pNodeBrique);

    if(pXMLNodeDeclencheurs)
    {
		XMLSize_t counti = pXMLNodeDeclencheurs->getChildNodes()->getLength();
		for(XMLSize_t i=0; i<counti;i++)
        {      
			DOMNode *pXMLDeclencheur = pXMLNodeDeclencheurs->getChildNodes()->item(i);
			if (pXMLDeclencheur->getNodeType() != DOMNode::ELEMENT_NODE) continue;

            // Lecture de l'identifiant du contrôleur
            pBrique->addDeclencheur(pXMLDeclencheur);
        }
    }

    // Ajout des actions
    DOMNode * pXMLNodeActions = m_pReseau->GetXMLUtil()->SelectSingleNode(".//ACTIONS", pNodeBrique->getOwnerDocument(), (DOMElement*)pNodeBrique);

    if(pXMLNodeActions)
    {
		XMLSize_t counti = pXMLNodeActions->getChildNodes()->getLength();
		for(XMLSize_t i=0; i<counti;i++)
        {      
			DOMNode *pXMLAction = pXMLNodeActions->getChildNodes()->item(i);
			if (pXMLAction->getNodeType() != DOMNode::ELEMENT_NODE) continue;

            // Lecture de l'identifiant du contrôleur
            pBrique->addAction(pXMLAction);
        }
    }

    // Ajout des restitutions
    DOMNode * pXMLNodeRestitutions = m_pReseau->GetXMLUtil()->SelectSingleNode(".//RESTITUTIONS", pNodeBrique->getOwnerDocument(), (DOMElement*)pNodeBrique);

    if(pXMLNodeRestitutions)
    {
		XMLSize_t counti = pXMLNodeRestitutions->getChildNodes()->getLength();
		for(XMLSize_t i=0; i<counti;i++)
        {      
			DOMNode *pXMLRestitution = pXMLNodeRestitutions->getChildNodes()->item(i);
			if (pXMLRestitution->getNodeType() != DOMNode::ELEMENT_NODE) continue;

            // Lecture de l'identifiant du contrôleur
            pBrique->addRestitution(pXMLRestitution);
        }
    }

    m_pReseau->GetPythonUtils()->exitChildInterpreter();
}

// renvoie la brique de régulation dont les actions sont en cours d'exécution
RegulationBrique * RegulationModule::getBriqueActive()
{
    return m_pBriqueActive;
}



////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Sérialisation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////
template void RegulationModule::serialize(boost::archive::xml_woarchive & ar, const unsigned int version);
template void RegulationModule::serialize(boost::archive::xml_wiarchive & ar, const unsigned int version);

template<class Archive>
void RegulationModule::serialize(Archive & ar, const unsigned int version)
{
    ar & BOOST_SERIALIZATION_NVP(m_pReseau);
    ar & BOOST_SERIALIZATION_NVP(m_LstBriquesRegulation);
    ar & BOOST_SERIALIZATION_NVP(m_pBriqueActive);
}