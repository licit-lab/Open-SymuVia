#pragma once

#ifndef SYMUCORE_WALKINGPATTERN_H
#define SYMUCORE_WALKINGPATTERN_H

#include "SymuCoreExports.h"

#include "Graph/Model/Pattern.h"
#include "Graph/Model/Cost.h"

#include <map>

#pragma warning(push)
#pragma warning(disable : 4251)

namespace SymuCore {

class SYMUCORE_DLL_DEF WalkingPattern : public Pattern {

public:

    WalkingPattern();
    WalkingPattern(Link* pLink, bool bIsInitialPattern, bool bUseRealLength, double dbLength);
    virtual ~WalkingPattern();

    virtual void prepareTimeFrames(double startPeriodTime, double endPeriodTime, double travelTimesUpdatePeriod, const std::vector<SubPopulation*>& listSubPopulations, const std::map<SymuCore::SubPopulation*, SymuCore::CostFunction> & mapCostFunctions, int nbSimulationInstances, int iInstance);
    virtual Cost* getPatternCost(int iSimulationInstance, double t, SubPopulation* pSubPopulation);

    virtual std::string toString() const;

    virtual void fillMeasuredCostsForTravelTimesUpdatePeriod(int iTravelTimesUpdatePeriodIndex, const std::vector<SubPopulation*>& listSubPopulations, const std::map<SymuCore::SubPopulation*, SymuCore::CostFunction> & mapCostFunctions);

    std::map<SubPopulation*, Cost> & GetCostBySubPopulation();
    double getWalkLength() const;

    virtual void fillFromSecondaryInstance(Pattern * pFrom, int iInstance);

protected:

    bool m_bIsInitialPattern; // don't use the same walk speed and radius for initial and intermediate walk patterns
    bool m_bUseRealLength; // for zones, we don't use a real length value but the initial walk radius instead
    double m_dbLength;

    std::map<SubPopulation*, Cost> m_CostBySubPopulation;

};
}

#pragma warning(pop)

#endif // SYMUCORE_WALKINGPATTERN_H
