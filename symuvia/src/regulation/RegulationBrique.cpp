#include "stdafx.h"
#include "RegulationBrique.h"

#include "RegulationCapteur.h"
#include "RegulationCondition.h"
#include "RegulationAction.h"
#include "RegulationRestitution.h"

#include "../reseau.h"

#include <xercesc/dom/DOMNode.hpp>

#include <boost/serialization/vector.hpp>

using namespace boost::python;

XERCES_CPP_NAMESPACE_USE


RegulationBrique::RegulationBrique(void)
{
    m_pReseau = NULL;
    m_dbRespect = 1.0;
}

RegulationBrique::RegulationBrique(Reseau * pReseau, const std::string & strID, double dbRespect)
{
    m_pReseau = pReseau;
    m_strID = strID;
    m_dbRespect = dbRespect;
}

RegulationBrique::~RegulationBrique(void)
{
    for(size_t iCapteur = 0; iCapteur < m_LstCapteurs.size(); iCapteur++)
    {
        delete m_LstCapteurs[iCapteur];
    }

    for(size_t iCondition = 0; iCondition < m_LstConditions.size(); iCondition++)
    {
        delete m_LstConditions[iCondition];
    }

    for(size_t iAction = 0; iAction < m_LstActions.size(); iAction++)
    {
        delete m_LstActions[iAction];
    }

    for(size_t iRestitution = 0; iRestitution < m_LstRestitutions.size(); iRestitution++)
    {
        delete m_LstRestitutions[iRestitution];
    }
}

// Instancie un nouveau détecteur pour la brique
void RegulationBrique::addDetecteur(DOMNode * pNodeDetecteur)
{
    RegulationCapteur * pCapteur = new RegulationCapteur;
    m_LstCapteurs.push_back(pCapteur);

    pCapteur->initialize(m_pReseau, pNodeDetecteur);
}

// Instancie un nouveau déclencheur pour la brique
void RegulationBrique::addDeclencheur(DOMNode * pNodeDeclencheur)
{
    RegulationCondition* pCondition = new RegulationCondition;
    m_LstConditions.push_back(pCondition);

    pCondition->initialize(m_pReseau, pNodeDeclencheur);
}

// Instancie une nouvelle action pour la brique
void RegulationBrique::addAction(DOMNode * pNodeAction)
{
    RegulationAction* pAction = new RegulationAction;
    m_LstActions.push_back(pAction);

    pAction->initialize(m_pReseau, pNodeAction, m_LstConditions);
}

// Instancie une nouvelle restitution pour la brique
void RegulationBrique::addRestitution(DOMNode * pNodeRestitution)
{
    RegulationRestitution* pRestitution = new RegulationRestitution;
    m_LstRestitutions.push_back(pRestitution);

    pRestitution->initialize(m_pReseau, pNodeRestitution, m_LstConditions);
}

// exécute les traitements associés à la brique pour le pas de temps écoulé
void RegulationBrique::run()
{
    dict capteursData;

    // 1 - Mise à jour des détecteurs
    for(size_t iCapteur = 0; iCapteur < m_LstCapteurs.size(); iCapteur++)
    {
        RegulationCapteur * pCapteur = m_LstCapteurs[iCapteur];
        pCapteur->update();
        capteursData[pCapteur->getID()] = pCapteur->getResults();
    }

    // 2 - Test des conditions
    for(size_t iCondition = 0; iCondition < m_LstConditions.size(); iCondition++)
    {
        m_LstConditions[iCondition]->test(capteursData);
    }

    // 3 - Traitement des actions
    for(size_t iAction = 0; iAction < m_LstActions.size(); iAction++)
    {
        if(m_LstActions[iAction]->test())
        {
            m_LstActions[iAction]->execute();
        }
    }
}

// restitution des sorties éventuelles dans le fichier de trafic
void RegulationBrique::restitution(TraceDocTrafic * pDocTrafic)
{
    // traitement de chaque brique
    for(size_t iRestitution = 0; iRestitution < m_LstRestitutions.size(); iRestitution++)
    {
        m_LstRestitutions[iRestitution]->restitution(pDocTrafic, m_strID);
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Sérialisation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////
template void RegulationBrique::serialize(boost::archive::xml_woarchive & ar, const unsigned int version);
template void RegulationBrique::serialize(boost::archive::xml_wiarchive & ar, const unsigned int version);

template<class Archive>
void RegulationBrique::serialize(Archive & ar, const unsigned int version)
{
    ar & BOOST_SERIALIZATION_NVP(m_pReseau);
    ar & BOOST_SERIALIZATION_NVP(m_strID);
    ar & BOOST_SERIALIZATION_NVP(m_LstCapteurs);
    ar & BOOST_SERIALIZATION_NVP(m_LstConditions);
    ar & BOOST_SERIALIZATION_NVP(m_LstActions);
    ar & BOOST_SERIALIZATION_NVP(m_LstRestitutions);
    ar & BOOST_SERIALIZATION_NVP(m_dbRespect);
}