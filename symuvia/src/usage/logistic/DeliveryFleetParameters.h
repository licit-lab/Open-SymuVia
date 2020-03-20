#pragma once
#ifndef DeliveryFleetParametersH
#define DeliveryFleetParametersH

#include "usage/AbstractFleetParameters.h"

#include <map>
#include <vector>

class TripNode;

class DeliveryFleetParameters : public AbstractFleetParameters
{

public:
    DeliveryFleetParameters();
    virtual ~DeliveryFleetParameters();

    std::map<TripNode*, std::vector<std::pair<int,int> > > & GetMapLoadsUnloads();

    virtual AbstractFleetParameters * Clone();
    
protected:

    // Définition des quantités à charger et décharger à chaque pointde livraison (un vector car une tournée peut passer plusieurs fois par un même point de livraison)
    std::map<TripNode*, std::vector<std::pair<int,int> > > m_mapLoadsUnloads;


////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Sérialisation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////
private:
	friend class boost::serialization::access;
	template<class Archive>
	void serialize(Archive& ar, const unsigned int version);
};

#endif // DeliveryFleetParametersH
