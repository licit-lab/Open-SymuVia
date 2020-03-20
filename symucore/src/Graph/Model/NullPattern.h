#pragma once

#ifndef SYMUCORE_NULLPATTERN_H
#define SYMUCORE_NULLPATTERN_H

#include "SymuCoreExports.h"

#include "Pattern.h"
#include "Cost.h"

namespace SymuCore {

class SYMUCORE_DLL_DEF NullPattern : public Pattern {

public:

    NullPattern();
    NullPattern(Link* pLink);
    virtual ~NullPattern();

    void prepareTimeFrames(double startPeriodTime, double endPeriodTime, double travelTimesUpdatePeriod, const std::vector<SubPopulation *> &listSubPopulations, const std::map<SymuCore::SubPopulation*, SymuCore::CostFunction> & mapCostFunctions, int nbSimulationInstances, int iInstance);
    virtual Cost* getPatternCost(int iSimulationInstance, double t, SubPopulation* pSubPopulation);

    virtual std::string toString() const;

    virtual void fillMeasuredCostsForTravelTimesUpdatePeriod(int iTravelTimesUpdatePeriodIndex, const std::vector<SubPopulation *> &listSubPopulations, const std::map<SymuCore::SubPopulation*, SymuCore::CostFunction> & mapCostFunctions);

    virtual void fillFromSecondaryInstance(Pattern * pFrom, int iInstance);

protected:
    Cost m_NullCost;
};
}

#endif // SYMUCORE_NULLPATTERN_H
