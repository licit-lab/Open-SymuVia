#pragma once

class TypeVehicule;

namespace boost {
    namespace serialization {
        class access;
    }
}

// Structure de stockage des temps d'insertion spécifique à chaque type de véhicule
struct TempsCritique
{
    TypeVehicule    *pTV;   // Type de véhicule
    double          dbtc;   // Temps d'insertion

////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Sérialisation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////
private:
	friend class boost::serialization::access;
	template<class Archive>
	void serialize(Archive & ar, const unsigned int version);
};