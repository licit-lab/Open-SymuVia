#pragma once

#ifndef _SYMUMASTER_MFDDRIVINGPATTERN_
#define _SYMUMASTER_MFDDRIVINGPATTERN_

#include "SymuMasterExports.h"

#include <Graph/Model/Driving/DrivingPattern.h>

namespace SymuCore {
    class Node;
}

namespace SymuMaster {

    class SYMUMASTER_EXPORTS_DLL_DEF MFDDrivingPattern : public SymuCore::DrivingPattern {

    public:
        MFDDrivingPattern(SymuCore::Link* pLink, int routeID, std::map<int, std::map<SymuCore::CostFunction, SymuCore::Cost> > & mapCostByRoute);

        virtual ~MFDDrivingPattern();

        int getRouteID() const;

        virtual std::string toString() const;

        virtual void fillMeasuredCostsForTravelTimesUpdatePeriod(int iTravelTimesUpdatePeriodIndex,
            const std::vector<SymuCore::SubPopulation*>& listSubPopulations,
            const std::map<SymuCore::SubPopulation*,
            SymuCore::CostFunction> & mapCostFunctions);

    private:

        int m_iRouteID;

        std::map<int, std::map<SymuCore::CostFunction, SymuCore::Cost> > & m_mapCostByRoute;
    };

}

#endif // _SYMUMASTER_MFDDRIVINGPATTERN_
