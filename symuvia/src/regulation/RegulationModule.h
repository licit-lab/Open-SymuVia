#pragma once
#ifndef RegulationModuleH
#define RegulationModuleH

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

class RegulationBrique;
class Reseau;
class TraceDocTrafic;
class SousTypeVehicule;

/**
* Point d'entrée du mécanisme de gestion générique des briques de régulation
*/
class RegulationModule
{
public:

    // ************************************
    // Constructeurs/Destructeur
    // ************************************
    RegulationModule(void);
    RegulationModule(Reseau * pReseau);
    virtual ~RegulationModule(void);

    // ************************************
    // Traitements publics
    // ************************************

    // exécute les briques de régulation pou le pas de temps écoulé
    void run();

    // restitution des sorties éventuelles dans le fichier de trafic
    void restitution(TraceDocTrafic * pDocTrafic);

    // Instancie une nouvelle brique de régulation
    void addBrique(XERCES_CPP_NAMESPACE::DOMNode * pNodeBrique);


    // ************************************
    // Accesseurs
    // ************************************
    std::vector<RegulationBrique*> & GetLstBriquesRegulation() {return m_LstBriquesRegulation;}

    RegulationBrique * getBriqueActive();

private:

    // ************************************
    // Membres privés
    // ************************************
    // Pointeur vers le réseau
    Reseau * m_pReseau;
  
    // Liste des briques de régulation utilisées
    std::vector<RegulationBrique*> m_LstBriquesRegulation;

    // Brique de régulation dont les actions sont en cours d'exécution
    RegulationBrique * m_pBriqueActive;

////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Sérialisation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////
private:
	friend class boost::serialization::access;
	template<class Archive>
	void serialize(Archive & ar, const unsigned int version);
};
#endif // RegulationModuleH