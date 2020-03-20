#pragma once
#ifndef sortieH
#define sortieH

#include "ConnectionPonctuel.h"
#include "usage/SymuViaTripNode.h"


// Sortie est un classe fixe de connection. Il s'agit d'un élément de réseau qui
// se trouve en aval d'un tronçon et qui est définit par sa capacité à absorber
// le flux quittant le réseau simulé

class Sortie : public SymuViaTripNode, public ConnectionPonctuel
{
private:        

    // Définition des caractéristiques de la sortie	
    ListOfTimeVariation<tracked_double>         *m_pLTVCapacite;

public:

	// Constructeurs, destructeurs et assimilés
	virtual ~Sortie(void); // Destructeur
	Sortie(void); // constructeur par défaut
    Sortie(char*, Reseau *pReseau);

    ListOfTimeVariation<tracked_double>*    GetLstCapacites() { return m_pLTVCapacite; }

    std::map<std::string, double> & GetCapacitiesPerRoutes() { return m_mapCapacityPerRoute; }

    // Surchage de GetNextEnterInstant pour application de la restriction de capacité
    virtual double  GetNextEnterInstant(int nNbVoie, double  dbPrevInstSortie, double  dbInstant, double  dbPasDeTemps, const std::string & nextRouteID);

    // Arrivée du véhicule dans la sortie
    virtual void        VehiculeEnter(boost::shared_ptr<Vehicule> pVehicle, VoieMicro * pDestinationEnterLane, double dbInstDestinationReached, double dbInstant, double dbTimeStep, bool bForcedOutside = false);

protected:
    // Utilisé pour les restrictions de capacité par route imposées par SymuMaster pour l'hybridation avec SimuRes
    std::map<std::string, double> m_mapCapacityPerRoute;

////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Sérialisation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////
private:

	friend class boost::serialization::access;
	template<class Archive>
	void serialize(Archive& ar, const unsigned int version);
};

#endif
