#include "stdafx.h"
#include "sortie.h"

#include "reseau.h"
#include "voie.h"
#include "tuyau.h"
#include "usage/Position.h"

#include <boost/serialization/deque.hpp>
#include <boost/serialization/map.hpp>
#include <boost/serialization/shared_ptr.hpp>

//---------------------------------------------------------------------------
// Constructeurs, destructeurs et assimilés
//---------------------------------------------------------------------------

// constructeur normal
Sortie::Sortie( char _Nom[256], Reseau *pReseau ): SymuViaTripNode(_Nom, pReseau),
    ConnectionPonctuel(_Nom, pReseau)
{    
    m_pLTVCapacite = new ListOfTimeVariation<tracked_double>;

    // La position de sortie du TripNode vers le réseau est l'entrée elle-même
    m_InputPosition.SetConnection(this);
}

Sortie::Sortie()
{
}

Sortie::~Sortie()
{
    delete(m_pLTVCapacite);
}

//===============================================================
   double  Sortie::GetNextEnterInstant
//----------------------------------------------------------------
// Fonction  : Calcule l'instant de sortie du véhicule suivant par
//             rapport aux valeurs de la capacité
// Remarque  : Retourne -1 si la capacité 1 n'est pas atteinte avant 
//             la fin de 
// Version du: 07/04/2008
// Historique: 07/04/2008 (C.Bécarie - Tinea)
//             Création
//===============================================================
(
    int     nNbVoie,                // Nombre de voie du tronçon aval
    double  dbPrevInstSortie,       // Instant de sortie du dernier véhicule
    double  dbInstant,              // Instant courant en train d'être calculé
    double  dbPasDeTemps,           // Pas de temps
    const std::string & strNextRouteID // Route suivante (pour hybridation SymuMaster avec SimuRes)
)
{
/*    double  dbCap, dbCapEx;
    double  dbTmp;
    double  dbCapPrev = 0;
    double  dbNextInstSortie;
    double  dbCapTmp;
   
    dbCap = *m_pLTVCapacite->GetVariationEx( dbInstant );

    dbCapEx = (dbPasDeTemps - dbPrevInstSortie) * dbCap;
    dbTmp = dbInstant;    

    if(dbCap>=1)
        return dbInstant;
    
    while(dbCapEx<1 && dbTmp <= GetReseau()->GetDureeSimu() )
    {
        dbTmp += dbPasDeTemps;
        dbCapPrev = dbCapEx;
        dbCapTmp = *m_pLTVCapacite->GetVariationEx( dbTmp );
        dbCapEx += dbPasDeTemps*dbCapTmp/nNbVoie;        
    }

    if(dbCapEx<1)
        return -1;          // la capacité 1 n'est pas atteinte avant la fin de la simulation

    // Calcul du moment exacte où la capacité passe à 1    
    dbNextInstSortie = (dbTmp - dbPasDeTemps) + (1 - dbCapPrev)/(dbCapEx-dbCapPrev) * dbPasDeTemps;

    return dbNextInstSortie;*/

	double  dbCum		= 0;				// Nombre de véhicule cumulé
	double  dbCumPrev	= 0;				// Nombre de véhicule cumulé précedent
	double  dbInst = dbPrevInstSortie;		// Boucle temporelle

    double dbCapacityRestriction = (*m_pLTVCapacite->GetVariationEx(dbInst)).t;

    std::map<std::string, double>::const_iterator iter = m_mapCapacityPerRoute.find(strNextRouteID);
    if (iter != m_mapCapacityPerRoute.end())
    {
        dbCapacityRestriction = std::min<double>(iter->second, dbCapacityRestriction);
    }

    if (dbCapacityRestriction >= NONVAL_DOUBLE)	// Pas de limite de capacité
		return dbInstant;
	
	while(dbCum<1 && dbInst <= GetReseau()->GetDureeSimu() )
	{		
		dbCumPrev = dbCum;
        dbCum += dbCapacityRestriction*dbPasDeTemps / nNbVoie;
		dbInst += dbPasDeTemps;
	}

	if(dbCum < 1)
		return -1;          // la capacité 1 n'est pas atteinte avant la fin de la simulation	

	// Calcul de l'instant exact où la capacité passe à 1    
	return (dbInst - dbPasDeTemps) + (1 - dbCumPrev)/(dbCum-dbCumPrev) * dbPasDeTemps;
}


void Sortie::VehiculeEnter(boost::shared_ptr<Vehicule> pVehicle, VoieMicro * pDestinationEnterLane, double dbInstDestinationReached, double dbInstant, double dbTimeStep, bool bForcedOutside)
{

    TripNode::VehiculeEnter(pVehicle, pDestinationEnterLane, dbInstDestinationReached, dbInstant, dbTimeStep, bForcedOutside);

    // Calcul de l'instant de sortie au plus tôt du véhicule suivant (en respectant la capacité/temps intersortie)
    double dbNextInstSortie = GetNextEnterInstant( ((Tuyau*)pDestinationEnterLane->GetParent())->getNbVoiesDis(), dbInstDestinationReached, dbInstant, dbTimeStep, pVehicle->GetNextRouteID());
    pDestinationEnterLane->GetNextInstSortie()[pVehicle->GetNextRouteID()] = dbNextInstSortie;

    pVehicle->SetRegimeFluide(true);
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Sérialisation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////
template void Sortie::serialize(boost::archive::xml_woarchive & ar, const unsigned int version);
template void Sortie::serialize(boost::archive::xml_wiarchive & ar, const unsigned int version);

template<class Archive>
void Sortie::serialize(Archive& ar, const unsigned int version)
{
    ar & BOOST_SERIALIZATION_BASE_OBJECT_NVP(SymuViaTripNode);
    ar & BOOST_SERIALIZATION_BASE_OBJECT_NVP(ConnectionPonctuel);
	ar & BOOST_SERIALIZATION_NVP(m_pLTVCapacite);
    ar & BOOST_SERIALIZATION_NVP(m_mapCapacityPerRoute);
}
