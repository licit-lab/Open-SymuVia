#include "DrivingPenalty.h"

#include <boost/make_shared.hpp>

using namespace SymuCore;

DrivingPenalty::DrivingPenalty()
{
    m_pNode = NULL;
}

DrivingPenalty::DrivingPenalty(Node* pParentNode, const PatternsSwitch & patternsSwitch) : AbstractPenalty(pParentNode, patternsSwitch)
{

}

DrivingPenalty::~DrivingPenalty()
{

}

std::vector<ListTimeFrame<std::map<SubPopulation *, Cost> > > &DrivingPenalty::getTemporalCosts()
{
    return m_mapTemporalCosts;
}

const std::vector<ListTimeFrame<std::map<SubPopulation *, Cost> > > &DrivingPenalty::getTemporalCosts() const
{
    return m_mapTemporalCosts;
}

void DrivingPenalty::prepareTimeFrames(double startPeriodTime, double endPeriodTime, double travelTimesUpdatePeriod, const std::vector<SubPopulation*>& listSubPopulations, const std::map<SymuCore::SubPopulation*, SymuCore::CostFunction> & mapCostFunctions, int nbSimulationInstances, int iInstance)
{
    m_mapTemporalCosts.clear();
    m_mapTemporalCosts.resize(iInstance==0?nbSimulationInstances:1);

    int nbTravelTimesUpdatePeriodsByPeriod = (int)ceil((endPeriodTime - startPeriodTime) / travelTimesUpdatePeriod);
    double currentTime = startPeriodTime;
    for (int iTravelTimesUpdatePeriod = 0; iTravelTimesUpdatePeriod < nbTravelTimesUpdatePeriodsByPeriod; iTravelTimesUpdatePeriod++)
    {
        double startTravelTimesUpdatePeriodTime = currentTime;
        double endTravelTimesUpdatePeriodTime = std::min<double>(startTravelTimesUpdatePeriodTime + travelTimesUpdatePeriod, endPeriodTime);

        ListTimeFrame<std::map<SubPopulation*, Cost>  > & mapTemporalCosts = m_mapTemporalCosts[0];
        mapTemporalCosts.addTimeFrame(startTravelTimesUpdatePeriodTime, endTravelTimesUpdatePeriodTime, boost::make_shared<std::map<SubPopulation*, Cost>>());
        fillMeasuredCostsForTravelTimesUpdatePeriod(iTravelTimesUpdatePeriod, listSubPopulations, mapCostFunctions);

        currentTime += travelTimesUpdatePeriod;
    }
}

Cost * DrivingPenalty::getPenaltyCost(int iSimulationInstance, double t, SubPopulation *pSubPopulation)
{
    // remark : we suppose here that the map fo costs is always filled even if the macrotype can't use the pattern
    return &m_mapTemporalCosts[iSimulationInstance].getData(t)->at(pSubPopulation);
}

void DrivingPenalty::fillFromSecondaryInstance(AbstractPenalty * pFrom, int iInstance)
{
    DrivingPenalty * pOriginalPenalty = (DrivingPenalty*)pFrom;
    m_mapTemporalCosts[iInstance] = std::move(pOriginalPenalty->m_mapTemporalCosts.front());
}
