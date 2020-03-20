#pragma once
#ifndef RegulationBriqueH
#define RegulationBriqueH

//rmq. positionnement de cet include avant le reste sinon erreur de compilation sou MAC OS X
#pragma warning( disable : 4244 )
#include <boost/python/dict.hpp>
#pragma warning( default : 4244 )

#include <xercesc/util/XercesVersion.hpp>


namespace XERCES_CPP_NAMESPACE {
    class DOMNode;
}

namespace boost {
    namespace serialization {
        class access;
    }
}

#include <vector>
#include <string>

class Reseau;
class RegulationCapteur;
class RegulationCondition;
class RegulationAction;
class RegulationRestitution;
class TraceDocTrafic;

/**
* Classe représentant une brique de régulation
*/
class RegulationBrique
{
public:

    // ************************************
    // Constructeurs/Destructeur
    // ************************************
    RegulationBrique(void);
    RegulationBrique(Reseau * pReseau, const std::string & strID, double dbRespect);
    virtual ~RegulationBrique(void);

    // ************************************
    // Traitements publics
    // ************************************

    // Instancie un nouveau détecteur pour la brique
    void addDetecteur(XERCES_CPP_NAMESPACE::DOMNode * pNodeDetecteur);

    // Instancie un nouveau déclencheur pour la brique
    void addDeclencheur(XERCES_CPP_NAMESPACE::DOMNode * pNodeDeclencheur);

    // Instancie une novuelle action pour la brique
    void addAction(XERCES_CPP_NAMESPACE::DOMNode * pNodeAction);

    // Instancie une novuelle action pour la brique
    void addRestitution(XERCES_CPP_NAMESPACE::DOMNode * pNodeRestitution);

    // exécute les traitements associés à la brique pour le pas de temps écoulé
    void run();

    // restitution des sorties éventuelles dans le fichier de trafic
    void restitution(TraceDocTrafic * pDocTrafic);

    // ************************************
    // Accesseurs
    // ************************************
    std::vector<RegulationCapteur*> & GetLstCapteurs() {return m_LstCapteurs;}
    std::vector<RegulationCondition*> & GetLstConditions() {return m_LstConditions;}
    std::vector<RegulationAction*> & GetLstActions() {return m_LstActions;}
    std::vector<RegulationRestitution*> & GetLstRestitutions() {return m_LstRestitutions;}
    double getTauxRespect() {return m_dbRespect;}
    Reseau * GetNetwork() { return m_pReseau; }

private:

    // ************************************
    // Membres privés
    // ************************************
    // Pointeur vers le réseau
    Reseau * m_pReseau;

    // Identifiant
    std::string m_strID;

    // Liste des capteurs utilisés par la brique de regulation
    std::vector<RegulationCapteur*> m_LstCapteurs;

    // Liste des conditions utilisées par la brique de regulation
    std::vector<RegulationCondition*> m_LstConditions;

    // Liste des actions utilisées par la brique de regulation
    std::vector<RegulationAction*> m_LstActions;

    // Liste des actions de restitution utilisées par la brique de regulation
    std::vector<RegulationRestitution*> m_LstRestitutions;

    // pourcentage de respect de la consigne
    double  m_dbRespect;

////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Sérialisation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////
private:
	friend class boost::serialization::access;
	template<class Archive>
	void serialize(Archive & ar, const unsigned int version);
};

#endif // RegulationBriqueH