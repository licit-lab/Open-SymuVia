#include "Cost.h"

#include <limits>

using namespace SymuCore;

Cost::Cost()
{
	m_dbCostValue = 0;
	m_dbTravelTime = 0;
}

Cost::Cost(double dCostValue, double dTravelTime)
{
	m_dbCostValue = dCostValue;
    m_dbTravelTime = dTravelTime;
}

Cost::~Cost()
{

}

void Cost::setUsedCostValue(double dCostValue)
{
    m_dbCostValue = dCostValue;
}

void Cost::setOtherCostValue(CostFunction eCostFunction, double dbValue)
{
    m_mapOtherCosts[eCostFunction] = dbValue;
}

double Cost::getOtherCostValue(CostFunction eCostFunction) const
{
    std::map<CostFunction, double>::const_iterator iter = m_mapOtherCosts.find(eCostFunction);
    if (iter != m_mapOtherCosts.end())
    {
        return iter->second;
    }
    else
    {
        return 0;
    }
}

double Cost::getCostValue(CostFunction eCostFunction) const
{
    std::map<CostFunction, double>::const_iterator iter = m_mapOtherCosts.find(eCostFunction);
    if (iter != m_mapOtherCosts.end())
    {
        return iter->second;
    }
    else
    {
        return m_dbCostValue;
    }
}

double Cost::getCostValue() const
{
	return m_dbCostValue;
}

void Cost::setTravelTime(double dTravelTime)
{
    m_dbTravelTime = dTravelTime;
}

double Cost::getTravelTime() const
{
    return m_dbTravelTime;
}

// OTK - TODO - Pourquoi pas un vrai opérateur + ?
void Cost::plus(Cost * otherCost)
{
    // rmq. we don't bother to process m_mapOtherCosts here since it is only use for now to output costs pattern per pattern or penalty per penalty.
    if(!otherCost)
    {
        m_dbCostValue = std::numeric_limits<double>::infinity();
        m_dbTravelTime = std::numeric_limits<double>::infinity();
    }else{
        m_dbCostValue += otherCost->getCostValue();
        m_dbTravelTime += otherCost->getTravelTime();
    }
}
