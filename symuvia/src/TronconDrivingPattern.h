#pragma once

#ifndef TronconDrivingPattern_H
#define TronconDrivingPattern_H

#include "SymuViaDrivingPattern.h"

#include <Utils/SpatialMarginalsHelper.h>

class TuyauMicro;

class TronconDrivingPattern : public SymuViaDrivingPattern
{
public:

    TronconDrivingPattern(SymuCore::Link* pLink, Reseau * pNetwork, TuyauMicro * pTuyau);

    virtual ~TronconDrivingPattern();

    virtual std::string toString() const;

    TuyauMicro * GetTuyau() const;

    virtual void fillMeasuredCostsForTravelTimesUpdatePeriod(int iTravelTimesUpdatePeriodIndex, const std::vector<SymuCore::SubPopulation *> &listSubPopulations, const std::map<SymuCore::SubPopulation*, SymuCore::CostFunction> & mapCostFunctions);

    virtual void postProcessCosts(const std::vector<SymuCore::SubPopulation*>& listSubPopulations, const std::map<SymuCore::SubPopulation*, SymuCore::CostFunction> & mapCostFunctions);

    virtual void fillCost(const std::map<TypeVehicule*, SymuCore::SubPopulation *> &listSubPopulations);

protected:
    TuyauMicro * m_pTuyau;

    SymuCore::SpatialMarginalsHelper m_marginalsHelper;

    bool m_bDoComputeMarginals;
};

#endif // TronconDrivingPattern_H


