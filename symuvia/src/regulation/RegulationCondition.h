#pragma once
#ifndef RegulationConditionH
#define RegulationConditionH

#include "RegulationElement.h"

/**
* classe représentant une condition de régulation
*/
class RegulationCondition : public RegulationElement
{
public:

    // ************************************
    // Constructeurs/Destructeur
    // ************************************
    RegulationCondition(void);
    virtual ~RegulationCondition(void);

    // ************************************
    // Accesseurs
    // ************************************
    const std::string & getID() {return m_strID;}
    bool isRemplie() {return m_bRemplie;}
    const boost::python::object & getContext() {return m_ContextDict;}

    // ************************************
    // Traitements publics
    // ************************************
    virtual void initialize(Reseau * pReseau, XERCES_CPP_NAMESPACE::DOMNode * pNodeCondition);

    // Test de la condition
    void test(boost::python::dict capteurs);

protected:

    // ************************************
    // Traitements protégés
    // ************************************
    virtual std::string getBuiltinModuleName();

private:

    // ************************************
    // Membres privés
    // ************************************
    // Identifiant de la condition
    std::string m_strID;

    // résultat de la condition (remplie ou non)
    bool m_bRemplie;


////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Sérialisation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////
private:
	friend class boost::serialization::access;
	template<class Archive>
	void serialize(Archive & ar, const unsigned int version);
};

#endif // RegulationConditionH