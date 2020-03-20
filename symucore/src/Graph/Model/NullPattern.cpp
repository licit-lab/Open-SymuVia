#include "NullPattern.h"
#include "Link.h"
#include "Node.h"

using namespace SymuCore;

NullPattern::NullPattern()
:Pattern()
{
}

NullPattern::NullPattern(Link* pLink)
: Pattern(pLink, PT_Undefined)
{
}

NullPattern::~NullPattern()
{
    
}

void NullPattern::prepareTimeFrames(double startPeriodTime, double endPeriodTime, double travelTimesUpdatePeriod, const std::vector<SubPopulation *> &listSubPopulations, const std::map<SymuCore::SubPopulation*, SymuCore::CostFunction> & mapCostFunctions, int nbSimulationInstances, int iInstance)
{
    // nothing
}

Cost* NullPattern::getPatternCost(int iSimulationInstance, double t, SubPopulation* pSubPopulation)
{
    return &m_NullCost;
}

std::string NullPattern::toString() const
{
	return "Null Pattern";
}

void NullPattern::fillMeasuredCostsForTravelTimesUpdatePeriod(int iTravelTimesUpdatePeriodIndex, const std::vector<SubPopulation*>& listSubPopulations, const std::map<SymuCore::SubPopulation*, SymuCore::CostFunction> & mapCostFunctions)
{
    // nothing
}

void NullPattern::fillFromSecondaryInstance(Pattern * pFrom, int iInstance)
{
    // nothing
}


