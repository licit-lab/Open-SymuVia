#pragma once

#ifndef SYMUCORE_DrivingPenalty_H
#define SYMUCORE_DrivingPenalty_H

#include "SymuCoreExports.h"

#include "Graph/Model/AbstractPenalty.h"

#include <map>

#pragma warning(push)
#pragma warning(disable : 4251)

namespace SymuCore {

class SYMUCORE_DLL_DEF DrivingPenalty : public AbstractPenalty {

public:

    DrivingPenalty();
    DrivingPenalty(Node* pParentNode, const PatternsSwitch & patternsSwitch);
    virtual ~DrivingPenalty();

    //getters
    std::vector<ListTimeFrame<std::map<SubPopulation*, Cost> > > & getTemporalCosts();
    const std::vector<ListTimeFrame<std::map<SubPopulation*, Cost> > > & getTemporalCosts() const;

    virtual void prepareTimeFrames(double startPeriodTime, double endPeriodTime, double travelTimesUpdatePeriod, const std::vector<SubPopulation *> &listSubPopulations, const std::map<SymuCore::SubPopulation*, SymuCore::CostFunction> & mapCostFunctions, int nbSimulationInstances, int iInstance);

    virtual Cost* getPenaltyCost(int iSimulationInstance, double t, SubPopulation* pSubPopulation);

    virtual void fillFromSecondaryInstance(AbstractPenalty * pFrom, int iInstance);

private:

    std::vector<ListTimeFrame<std::map<SubPopulation*, Cost> > >     m_mapTemporalCosts; //cost to switch patterns for each simulation instance

};
}

#pragma warning(pop)

#endif // SYMUCORE_DrivingPenalty_H
