#pragma once

#ifndef _SYMUMASTER_SYMUCORECONSTANTS_
#define _SYMUMASTER_SYMUCORECONSTANTS_

#include "SymuCoreExports.h"

#define MPS2KPH_MF         3.6    // Multiplicative factor for conversion factor from m/s to km/h

namespace SymuCore {

    enum NodeType
    {
        NT_Undefined = 0,
        NT_InterLayerOrigin,
        NT_InterLayerDestination,
        NT_RoadExtremity,
        NT_RoadJunction,
        NT_Area,
        NT_SubArea,
        NT_PublicTransportStation,
        NT_TaxiStation,
		NT_Parking,
        NT_MacroNode,
        NT_NetworksInterface,
    };

    enum PatternType     //define each possible pattern mode
    {
        PT_Undefined = 0,
        PT_Walk,
        PT_Cycling,
        PT_Driving,
        PT_PublicTransport,
        PT_Taxi
    };

    enum ServiceType     //define each possible Layers
    {
        ST_Undefined = 0,
        ST_All,
        ST_Driving,
        ST_PublicTransport,
        ST_Taxi
    };

    enum CostFunction     //define each possible cost function here
    {
        CF_Undefined = 0,
        CF_TravelTime,
        CF_Marginals,
		CF_Emissions
    };

    enum ShortestPathHeuristic // Heuristic for the shortest path calculations
    {
        SPH_NONE = 0,
        SPH_EUCLIDIAN = 1
    };

	enum Pollutant // Pollutant
	{
		P_CO = 0,
		P_FC = 1,
		P_NOx = 2,
		P_PM = 3
	};

    class SYMUCORE_DLL_DEF SymuCoreConstants {

    };
}

#endif // _SYMUMASTER_SYMUCORECONSTANTS_
