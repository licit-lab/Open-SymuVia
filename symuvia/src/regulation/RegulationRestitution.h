#pragma once
#ifndef RegulationRestitutionH
#define RegulationRestitutionH

#include "RegulationAction.h"

class TraceDocTrafic;

/**
* Classe représentant une action de régulation
*/
class RegulationRestitution : public RegulationAction
{
public:

    // ************************************
    // Constructeurs/Destructeur
    // ************************************
    RegulationRestitution(void);
    virtual ~RegulationRestitution(void);

    // ************************************
    // Traitements publics
    // ************************************
    // initialisation de l'élément restitution
    virtual void initialize(Reseau * pReseau, XERCES_CPP_NAMESPACE::DOMNode * pNodeAction, const std::vector<RegulationCondition*> & lstConditions);
    // exécution de l'action de restitution
    void restitution(TraceDocTrafic * pDocTrafic, const std::string & briqueID);

protected:

    // ************************************
    // Traitements protégés
    // ************************************
    virtual std::string getBuiltinModuleName();

////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Sérialisation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////
private:
	friend class boost::serialization::access;
	template<class Archive>
	void serialize(Archive & ar, const unsigned int version);
};

#endif // RegulationRestitutionH