#include "stdafx.h"
#include "feux.h"

#include "repartiteur.h"

#include <boost/serialization/deque.hpp>
#include <boost/serialization/shared_ptr.hpp>

//---------------------------------------------------------------------------
// Constructeurs, destructeurs et assimilés
//--------------------------------------------------------------------------

// Destructeur
Feux::~Feux()
{
    //DeleteCycles();
    EraseListOfVariation(m_pLstCycle);
    delete m_pLstCycle;
}

// Constructeur par défaut
Feux::Feux(Repartiteur *pRep)
{
    m_pRepartiteur = pRep;

    m_pLstCycle = new std::deque<TimeVariation<CycleFeu>>;
}

//================================================================
/*    void Feux::DeleteCycles
//----------------------------------------------------------------
// Fonction  : Suppression des cycles de feu
// Version du: 09/11/2006
// Historique: 09/11/2006 (C.Bécarie - Tinea)
//================================================================
(
)
{
	std::deque <CycleFeu*>::iterator par;
    std::deque <CycleFeu*>::iterator debut = m_LstCycles.begin();
    std::deque <CycleFeu*>::iterator fin = m_LstCycles.end();

    for (par=debut;par!=fin;par++)
    {
        delete *(par);
    }
    m_LstCycles.erase(debut,fin);
}*/

/*//================================================================
    CycleFeu* Feux::GetCycle
//----------------------------------------------------------------
// Fonction  : Retourne le cycle de l'instant passé en paramètre
// Version du: 05/09/2009
// Historique: 05/09/2009 (C.Bécarie - Tinea)
//================================================================
(
    double   dbInstant,
    double  &dbTpsPrevCycle      // Temps passé dans les cycles précédents
)
{
    std::deque <CycleFeu*>::iterator cur, debut, fin;
    double              dbInt;
    double              dbOldInt;
    CycleFeu*    pCF;

    // Recherche de la répartition active
    debut   = m_LstCycles.begin();
    fin     = m_LstCycles.end();

    pCF = NULL;

    dbInt = 0;
    dbOldInt = 0;
    dbTpsPrevCycle = 0;
    for (cur=debut;cur!=fin;cur++)
    {
        pCF = *cur;
        dbInt += pCF->dbDuree;

        if( dbInt >= dbInstant )
        {
            dbTpsPrevCycle = dbOldInt;
            return *cur;
        }
        dbOldInt = dbInt;
    }

    // L'instant simulé est supérieur à la somme des durées des feux définies, on renvoie alors
    // le dernier cycle et un message de log est généré
    *Reseau::m_pFicSimulation << "Cycle du répartiteur "<< m_pRepartiteur->getNom() <<" non défini pour l'instant simulé. " << std::endl;

    dbTpsPrevCycle = dbOldInt;    
    return pCF;
}*/



////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Sérialisation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////
template void CycleFeu::serialize(boost::archive::xml_woarchive & ar, const unsigned int version);
template void CycleFeu::serialize(boost::archive::xml_wiarchive & ar, const unsigned int version);

template<class Archive>
void CycleFeu::serialize(Archive & ar, const unsigned int version)
{
	ar & BOOST_SERIALIZATION_NVP(dbCycle);
    ar & BOOST_SERIALIZATION_NVP(dbOrange);
}

template void Feux::serialize(boost::archive::xml_woarchive & ar, const unsigned int version);
template void Feux::serialize(boost::archive::xml_wiarchive & ar, const unsigned int version);

template<class Archive>
void Feux::serialize(Archive & ar, const unsigned int version)
{
	ar & BOOST_SERIALIZATION_NVP(m_pRepartiteur);
    ar & BOOST_SERIALIZATION_NVP(m_pLstCycle);
}