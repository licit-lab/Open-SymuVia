#pragma once

#ifndef SYMUCORE_DRIVINGPATTERN_H
#define SYMUCORE_DRIVINGPATTERN_H

#include "SymuCoreExports.h"
#include "Graph/Model/Pattern.h"
#include "Graph/Model/Cost.h"

#include <map>

#pragma warning(push)
#pragma warning(disable : 4251)

namespace SymuCore{

class SYMUCORE_DLL_DEF DrivingPattern : public Pattern {

public:

    DrivingPattern();
    DrivingPattern(Link* pLink);
    virtual ~DrivingPattern();

    const std::vector<ListTimeFrame<std::map<SubPopulation*, Cost> > > & getTemporalCosts() const;
    std::vector<ListTimeFrame<std::map<SubPopulation *, Cost> > > &getTemporalCosts();

    virtual void prepareTimeFrames(double startPeriodTime, double endPeriodTime, double travelTimesUpdatePeriod, const std::vector<SubPopulation *> &listSubPopulations, const std::map<SymuCore::SubPopulation*, SymuCore::CostFunction> & mapCostFunctions, int nbSimulationInstances, int iInstance);
    virtual Cost* getPatternCost(int iSimulationInstance, double t, SubPopulation* pSubPopulation);

    virtual void fillFromSecondaryInstance(Pattern * pFrom, int iInstance);

private:

    std::vector<ListTimeFrame<std::map<SubPopulation*, Cost>  > >   m_mapTemporalCosts; // cost to use this pattern, for every simulation instance, and depends on time

};
}

#pragma warning(pop)

#endif // SYMUCORE_DRIVINGPATTERN_H
