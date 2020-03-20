#pragma once
#ifndef divergentH
#define divergentH

#include "ConnectionPonctuel.h"
#include <string>

class Tuyau;

class Divergent :  public ConnectionPonctuel
{
private:

    // Variables caractéristiques du divergent

    Tuyau*          m_pTuyauAmont;              // Fictif pour un giratoire 

    // Variables de simulation
private:

public:
    // Constructeurs, destructeurs et assimilés
    virtual ~Divergent(); // Destructeur
    Divergent() ; // Constructeur par défaut
	Divergent(std::string strID,  Reseau *pReseau);

    // Fonctions relatives à la simulation des divergents    
    void            Init();
    void            Init(Tuyau *pTAm, Tuyau *pTAvP, Tuyau *pTAvS);

    void            CorrigeAcceleration(double dbInst);

////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Sérialisation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////
private:

	friend class boost::serialization::access;
	template<class Archive>
	void serialize(Archive & ar, const unsigned int version);
};

#endif
