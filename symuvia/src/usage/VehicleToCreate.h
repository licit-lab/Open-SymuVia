#pragma once
#ifndef VehicleToCreateH
#define VehicleToCreateH

class AbstractFleet;
class Trip;

namespace boost {
    namespace serialization {
        class access;
    }
}

#include <string>

// Définition des paramètres de création arbitraire d'un véhicule (fonctions SymCreateVehicle)
class VehicleToCreate
{
public:
    VehicleToCreate();
    VehicleToCreate(int vehId, AbstractFleet * pFleet);
    virtual ~VehicleToCreate();

    int GetVehicleID();
    AbstractFleet * GetFleet();

    Trip * GetTrip();
    void SetTrip(Trip * pTrip);

protected:
    int m_VehicleId;
    AbstractFleet * m_pFleet;
    Trip * m_pTrip;

////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Sérialisation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////
private:
	friend class boost::serialization::access;
	template<class Archive>
	void serialize(Archive & ar, const unsigned int version);
};

#endif // VehicleToCreateH