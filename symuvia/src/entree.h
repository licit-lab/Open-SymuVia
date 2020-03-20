#pragma once
#ifndef entreeH
#define entreeH

#include "ConnectionPonctuel.h"
#include "usage/SymuViaTripNode.h"

class Tuyau;
class DocTrafic;

// Entree est une classe fille de connection. Il s'agit d'un des deux élements
// réseau qu'il est possible de trouver en amont d'un tronçon. Une entrée est
// responsable de l'injection des véhicules sur le réseau

class Entree : public SymuViaTripNode, public ConnectionPonctuel
{
		
public:

        // Constructeurs, destructeurs et assimilés
	    virtual ~Entree(void) ; // Destructeur
	    Entree(char*, Reseau *pR); 
        Entree() ; // Constructeurs par défaut

		void SortieTrafic(DocTrafic *pXMLDocTrafic);

////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Sérialisation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////
private:

	friend class boost::serialization::access;
	template<class Archive>
	void serialize(Archive & ar, const unsigned int version);
};
#endif

