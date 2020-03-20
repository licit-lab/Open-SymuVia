#pragma once
#ifndef RegulationCapteurH
#define RegulationCapteurH

#include "RegulationElement.h"

/**
* classe représentant un capteur de régulation
*/
class RegulationCapteur : public RegulationElement
{
public:

    // ************************************
    // Constructeurs/Destructeur
    // ************************************
    RegulationCapteur(void);
    virtual ~RegulationCapteur(void);

    // ************************************
    // Accesseurs
    // ************************************
    const std::string & getID() {return m_strID;}
    const boost::python::object & getResults() {return m_Result;}

    // ************************************
    // Traitements publics
    // ************************************
    virtual void initialize(Reseau * pReseau, XERCES_CPP_NAMESPACE::DOMNode * pNodeCapteur);

    // mise à jour des mesures du capteur
    void update();

protected:

    // ************************************
    // Traitements protégés
    // ************************************
    virtual std::string getBuiltinModuleName();

private:

    // ************************************
    // Membres privés
    // ************************************
    // Identifiant du capteur
    std::string m_strID;

    // Résultats de la fonction associée au capteur pour le PDT courant
    boost::python::object m_Result;


////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Sérialisation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////
private:
	friend class boost::serialization::access;
	template<class Archive>
	void serialize(Archive & ar, const unsigned int version);
};

#endif // RegulationCapteurH