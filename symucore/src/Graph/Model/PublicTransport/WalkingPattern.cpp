#include "WalkingPattern.h"

#include <Demand/SubPopulation.h>
#include <Demand/Population.h>

using namespace SymuCore;


WalkingPattern::WalkingPattern() : Pattern(NULL, PT_Walk),
m_bIsInitialPattern(true),
m_bUseRealLength(true),
m_dbLength(0)
{
}

WalkingPattern::WalkingPattern(Link* pLink, bool bIsInitialPattern, bool bUseRealLength, double dbLength)
: Pattern(pLink, PT_Walk)
{
    m_bIsInitialPattern = bIsInitialPattern;
    m_bUseRealLength = bUseRealLength;
    m_dbLength = dbLength;
}

WalkingPattern::~WalkingPattern()
{
}

std::map<SubPopulation*, Cost> & WalkingPattern::GetCostBySubPopulation()
{
    return m_CostBySubPopulation;
}

double WalkingPattern::getWalkLength() const
{
    return m_dbLength;
}

void WalkingPattern::prepareTimeFrames(double startPeriodTime, double endPeriodTime, double travelTimesUpdatePeriod, const std::vector<SubPopulation*>& listSubPopulations, const std::map<SymuCore::SubPopulation*, SymuCore::CostFunction> & mapCostFunctions, int nbSimulationInstances, int iInstance)
{
    m_CostBySubPopulation.clear();

    for (size_t iSubPop = 0; iSubPop < listSubPopulations.size(); iSubPop++)
    {
        SubPopulation * pSubPop = listSubPopulations[iSubPop];

        assert(mapCostFunctions.at(pSubPop) == CF_TravelTime || mapCostFunctions.at(pSubPop) == CF_Marginals);

        Cost & cost = m_CostBySubPopulation[pSubPop];

        double dbTravelTime;
        if (!m_bUseRealLength)
        {
            if (m_bIsInitialPattern)
            {
                dbTravelTime = pSubPop->GetInitialWalkRadius() / pSubPop->GetInitialWalkSpeed();
            }
            else
            {
                dbTravelTime = pSubPop->GetPopulation()->GetIntermediateWalkRadius() / pSubPop->GetPopulation()->GetIntermediateWalkSpeed();
            }
        }
        else
        {
            double dbMaxRadius = m_bIsInitialPattern ? pSubPop->GetInitialWalkRadius() : pSubPop->GetPopulation()->GetIntermediateWalkRadius();
            if (m_dbLength > dbMaxRadius)
            {
                dbTravelTime = std::numeric_limits<double>::infinity();
            }
            else
            {
                dbTravelTime = m_dbLength / (m_bIsInitialPattern ? pSubPop->GetInitialWalkSpeed() : pSubPop->GetPopulation()->GetIntermediateWalkSpeed());
            }
        }

        // marginals = travel times for walk patterns for now...
        cost.setTravelTime(dbTravelTime);
        cost.setUsedCostValue(dbTravelTime);
        cost.setOtherCostValue(mapCostFunctions.at(pSubPop) == CF_TravelTime ? CF_Marginals : CF_TravelTime, dbTravelTime);
    }
}

Cost* WalkingPattern::getPatternCost(int iSimulationInstance, double t, SubPopulation* pSubPopulation)
{
    // rmq. same cost for all simulation instance
    return &m_CostBySubPopulation.at(pSubPopulation);
}

std::string WalkingPattern::toString() const
{
    return "Walk";
}

void WalkingPattern::fillMeasuredCostsForTravelTimesUpdatePeriod(int iTravelTimesUpdatePeriodIndex, const std::vector<SubPopulation*>& listSubPopulations, const std::map<SymuCore::SubPopulation*, SymuCore::CostFunction> & mapCostFunctions)
{
    // nothing : constant cost for now...
}

void WalkingPattern::fillFromSecondaryInstance(Pattern * pFrom, int iInstance)
{
    // nothing
}
