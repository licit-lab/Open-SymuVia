#pragma once

#ifndef SYMUCORE_SPATIAL_MARGINALS_HELPER_H
#define SYMUCORE_SPATIAL_MARGINALS_HELPER_H

#include "SymuCoreExports.h"

#include "Utils/SymuCoreConstants.h"

#include <Graph/Model/ListTimeFrame.h>

#include <map>

#pragma warning(push)
#pragma warning(disable : 4251)

namespace SymuCore {

    class MacroType;
    class SubPopulation;
    class Cost;

    class SYMUCORE_DLL_DEF SpatialMarginalsHelper
    {
    public:
        void SetNbVehicles(size_t iPeriodIndex, double dbStartTime, double dbEndTime, SymuCore::MacroType * pMacroType, double dbNbVehicles, double dbNbVehiclesForAllMacroTypes,
            double dbTravelTimeForAllMacroTypes);

        void ComputeMarginal(const std::map<MacroType*, std::vector<std::pair<SymuCore::CostFunction, SubPopulation*> > > & listMacroType,
            const std::map<MacroType*, bool> & forbiddenMacroTypes,
            const SymuCore::ListTimeFrame<std::map<SymuCore::SubPopulation*, SymuCore::Cost> > & temporalCosts,
            double dbMaxMarginalsValue, double dbPenalisationRatio);

    private:
        // Pour le stockage des nombres de véhicules en mode marginals spatialisé pour calcul a posteriori
        SymuCore::ListTimeFrame<std::map<SymuCore::MacroType*, double>  > m_mapTemporalNbVehicles;
        // Stockage du temps de parcours pour tous les macro-types
        SymuCore::ListTimeFrame<double>                                   m_mapTravelTimesForAllMacroTypes;
    };

}

#pragma warning(pop)

#endif // SYMUCORE_SPATIAL_MARGINALS_HELPER_H


