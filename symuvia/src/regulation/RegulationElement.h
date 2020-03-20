#pragma once
#ifndef RegulationElementH
#define RegulationElementH

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

class Reseau;

/**
* Classe abstraite commune aux éléments de régulation (association avec une fonction python)
*/
class RegulationElement
{
public:

    // ************************************
    // Constructeurs/Destructeur
    // ************************************
    RegulationElement(void);
    virtual ~RegulationElement(void);

    // ************************************
    // Traitements publics
    // ************************************
    virtual void initialize(Reseau * pReseau, XERCES_CPP_NAMESPACE::DOMNode * pNode);

    // ************************************
    // Accesseurs
    // ************************************
    boost::python::dict & GetContext() {return m_ContextDict;}
    void SetContext(boost::python::dict & context) {m_ContextDict = context;}

protected:

    // ************************************
    // Traitements protégés
    // ************************************
    virtual std::string getModuleName();
    virtual std::string getBuiltinModuleName() = 0;

    // ************************************
    // Membres privés
    // ************************************
    // Pointeur vers le réseau
    Reseau* m_pReseau;

    // Fonction python correspondant à l'élément
    std::string m_strFunc;
    // Chemin complet du fichier module contenant la fonction python à exécuter
    std::string m_strModule;
    // Dictionnaire Python correspondant aux paramètres à injecter avant le traitement de la fonction m_strFunc
    boost::python::dict m_FuncParamDict;
    // Contexte associé à l'élément (permet d'avoir un objet persistent manipulable par script entre les différents pas de temps)
    boost::python::dict m_ContextDict;


////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Sérialisation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////
private:
	friend class boost::serialization::access;
	template<class Archive>
	void serialize(Archive & ar, const unsigned int version);
};

#endif // RegulationElementH