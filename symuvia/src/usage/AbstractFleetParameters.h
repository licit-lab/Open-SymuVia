#pragma once
#ifndef AbstractFleetParametersH
#define AbstractFleetParametersH

#include "VehicleTypeUsageParameters.h"

class ScheduleParameters;

class AbstractFleetParameters
{

public:
    AbstractFleetParameters();
    virtual ~AbstractFleetParameters();

    virtual AbstractFleetParameters * Clone();

    VehicleTypeUsageParameters & GetUsageParameters();
    
    void SetScheduleParameters(ScheduleParameters * pScheduleParams);
    ScheduleParameters * GetScheduleParameters();

protected:

    // Paramètres pseudo génériques communs à plusieurs types de flottes
    VehicleTypeUsageParameters m_UsageParameters;

    // Pointeur vers le jeu de paramètres lié à l'élement de calendrier déclenchant l'activation du véhicule
    ScheduleParameters * m_pScheduleParameters;

////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Sérialisation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////
private:
	friend class boost::serialization::access;
	template<class Archive>
	void serialize(Archive& ar, const unsigned int version);
};

#endif // AbstractFleetParametersH
