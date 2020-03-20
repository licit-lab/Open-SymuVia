#pragma once

#ifndef SYMUCORE_Penalty_H
#define SYMUCORE_Penalty_H

#include "SymuCoreExports.h"

#include "AbstractPenalty.h"

#pragma warning(push)
#pragma warning(disable : 4251)

namespace SymuCore {

class SYMUCORE_DLL_DEF Penalty : public AbstractPenalty {

public:

    Penalty();
    Penalty(Node* pParentNode, const PatternsSwitch & patternsSwitch);
    virtual ~Penalty();

    //getters
    std::vector<ListTimeFrame<std::map<SubPopulation*, Cost> > > & getTemporalCosts();
    const std::vector<ListTimeFrame<std::map<SubPopulation*, Cost> > > & getTemporalCosts() const;

    virtual void prepareTimeFrames(double startPeriodTime, double endPeriodTime, double travelTimesUpdatePeriod, const std::vector<SubPopulation *> &listSubPopulations, const std::map<SymuCore::SubPopulation*, SymuCore::CostFunction> & mapCostFunctions, int nbSimulationInstances, int iInstance);

    virtual Cost* getPenaltyCost(int iSimulationInstance, double t, SubPopulation* pSubPopulation);

    virtual void fillFromSecondaryInstance(AbstractPenalty * pFrom, int iInstance);

private:

    std::vector<ListTimeFrame<std::map<SubPopulation*, Cost> > > m_TemporalCosts; //cost to switch patterns for each simulation instance

};
}

#pragma warning(pop)

#endif // SYMUCORE_Penalty_H
