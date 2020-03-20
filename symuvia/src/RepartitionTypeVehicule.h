#pragma once
#ifndef RepartitionTypeVehiculeH
#define RepartitionTypeVehiculeH

#include "tools.h"

class RepartitionTypeVehicule
{
public:

	RepartitionTypeVehicule();
	~RepartitionTypeVehicule();

    std::deque<TimeVariation<std::vector<std::vector<double> > > > & GetLstCoefficients();

    // Ajout des variations temporelles de répartition des types de véhicules
    void AddVariation(const std::vector<std::vector<double> > & coeffs, PlageTemporelle * pPlage);
    void AddVariation(const std::vector<std::vector<double> > & coeffs, double dbDuree);

    // Supprime toutes les répartitions définies
    void Clear();

    // Supprime les répartitions définies depuis l'instant passé en paramètres
    void ClearFrom(double dbInstant, double dbLag);

    // Effectue le tirage d'un nouveau type de véhicule
    TypeVehicule* CalculTypeNewVehicule(Reseau * pNetwork, double  dbInstant, int nVoie);

    // Indique s'il existe un coefficient non nul pour un type de véhicule donné entre les instants spécifiés
    bool HasVehicleType(Reseau * pNetwork, int indexVehicleType, double dbStartTime, double dbEndTime);

protected:

    // Récupération de la proportion d'un type de véhicule à un instant donné
    double GetRepartitionTypeVehicule(double dbInstant, double dbLag, int nTypeVehicule, int nVoie);

protected:

    // Coefficients par type de véhicule puis par voie
    std::deque<TimeVariation<std::vector<std::vector<double> > > > m_LstCoefficients;

////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Sérialisation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////
private:
	friend class boost::serialization::access;
	template<class Archive>
	void serialize(Archive & ar, const unsigned int version);
};

#endif // RepartitionTypeVehicule