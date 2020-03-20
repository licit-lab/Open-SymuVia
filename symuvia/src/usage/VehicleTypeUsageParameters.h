#pragma once
#ifndef VehicleTypeUsageParametersH
#define VehicleTypeUsageParametersH

#include "LoadUsageParameters.h"

class VehicleTypeUsageParameters
{

public:
    VehicleTypeUsageParameters();
    virtual ~VehicleTypeUsageParameters();

    LoadUsageParameters & GetLoadUsageParameters();

protected:

   LoadUsageParameters m_loadUsageParameters;

////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Sérialisation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////
private:
	friend class boost::serialization::access;
	template<class Archive>
	void serialize(Archive& ar, const unsigned int version);
};

#endif // VehicleTypeUsageParametersH
