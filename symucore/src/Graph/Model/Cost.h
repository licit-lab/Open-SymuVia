#pragma once

#ifndef SYMUCORE_COST_H
#define SYMUCORE_COST_H

#include "SymuCoreExports.h"

#include "Utils/SymuCoreConstants.h"

#include <map>

#pragma warning(push)
#pragma warning(disable : 4251)

namespace SymuCore {

class SYMUCORE_DLL_DEF Cost {

public:

    Cost();
    Cost(double dCostValue, double dTravelTime);
    virtual ~Cost();

    //getters
	double getCostValue() const;
	double getTravelTime() const;
    double getOtherCostValue(CostFunction eCostFunction) const;

    double getCostValue(CostFunction eCostFunction) const;

    //setters
    void setUsedCostValue(double dCostValue);
	void setTravelTime(double dTravelTime);
    void setOtherCostValue(CostFunction eCostFunction, double dbValue);


    void plus(Cost *otherCost);

private:

    double m_dbCostValue;
	double m_dbTravelTime; // in seconds
    std::map<CostFunction, double> m_mapOtherCosts;
};
}

#pragma warning(pop)

#endif // SYMUCORE_PATTERNSSWITCH_H
