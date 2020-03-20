#pragma once
#ifndef RegulationActionH
#define RegulationActionH

#include "RegulationElement.h"

#include <vector>

class RegulationCondition;

/**
* Classe représentant une action de régulation
*/
class RegulationAction : public RegulationElement
{
public:

    // ************************************
    // Constructeurs/Destructeur
    // ************************************
    RegulationAction(void);
    virtual ~RegulationAction(void);

    // ************************************
    // Traitements publics
    // ************************************
    virtual void initialize(Reseau * pReseau, XERCES_CPP_NAMESPACE::DOMNode * pNodeAction, const std::vector<RegulationCondition*> & lstConditions);

    // test préliminaire à l'exécution de l'action
    bool test();

    // exécution de l'action étant donné les conditions passées en paramètres
    void execute();

protected:

    // ************************************
    // Traitements protégés
    // ************************************
    virtual std::string getBuiltinModuleName();


    // ************************************
    // Membres protégés
    // ************************************
    // Liste des conditions nécessaires à l'exécution de l'action
    std::vector<RegulationCondition*> m_LstConditions;


////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Sérialisation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////
private:
	friend class boost::serialization::access;
	template<class Archive>
	void serialize(Archive & ar, const unsigned int version);
};

#endif // RegulationActionH