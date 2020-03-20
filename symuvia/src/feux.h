#pragma once
#ifndef feuxH
#define feuxH

#include "tools.h"

//---------------------------------------------------------------------------
//***************************************************************************//
//                          structure CycleFeux                              //
//***************************************************************************//
struct CycleFeu
{
    //double      dbDuree;                         // Durée d'application du cycle

    double      dbCycle;                        // Durée du cycle
    double      dbOrange;                       // Durée de l'orange

////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Sérialisation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////
private:

	friend class boost::serialization::access;
	template<class Archive>
	void serialize(Archive & ar, const unsigned int version);
};

// La classe feux contient toutes les informations correspondant au fonctionnement
// de circulation durant la simulation. Pour info, un feu est défini pour une voie
// d'un tronçon en amont d'un répartiteur

class Repartiteur;

class Feux
{
private:
    //std::deque<CycleFeu*>        m_LstCycles;                   
    Repartiteur*                m_pRepartiteur;                 // Répartiteur parent

    std::deque<TimeVariation<CycleFeu>>           *m_pLstCycle; // Liste des cycles

public:

    // Constructeurs, destructeurs et assimilés
    ~Feux(void) ; // destructeur
	Feux(Repartiteur *pRep) ; // constructeur par défaut      

    Repartiteur*    GetRepartiteur(){return m_pRepartiteur;};

    std::deque<TimeVariation<CycleFeu>>* GetLstCycle(){return m_pLstCycle;};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Sérialisation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////
private:

	friend class boost::serialization::access;
	template<class Archive>
	void serialize(Archive & ar, const unsigned int version);

};

#endif
