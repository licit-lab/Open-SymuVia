#include "NullPenalty.h"

using namespace SymuCore;

NullPenalty::NullPenalty() :
AbstractPenalty()
{
}

NullPenalty::NullPenalty(Node* pParentNode, const PatternsSwitch & patternsSwitch) :
AbstractPenalty(pParentNode, patternsSwitch)
{
}

NullPenalty::~NullPenalty()
{

}

void NullPenalty::prepareTimeFrames(double startPeriodTime, double endPeriodTime, double travelTimesUpdatePeriod, const std::vector<SubPopulation *> &listSubPopulations, const std::map<SymuCore::SubPopulation*, SymuCore::CostFunction> & mapCostFunctions, int nbSimulationInstances, int iInstance)
{
    // nothing
}

void NullPenalty::fillMeasuredCostsForTravelTimesUpdatePeriod(int iTravelTimesUpdatePeriodIndex, const std::vector<SubPopulation*>& listSubPopulations, const std::map<SymuCore::SubPopulation*, SymuCore::CostFunction> & mapCostFunctions)
{
    // nothing
}

void NullPenalty::fillFromSecondaryInstance(AbstractPenalty * pFrom, int iInstance)
{
    // nothing
}

Cost* NullPenalty::getPenaltyCost(int iSimulationInstance, double t, SubPopulation* pSubPopulation)
{
    return &m_NullCost;
}
