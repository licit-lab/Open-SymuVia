#pragma once

#ifndef PlaqueDrivingPattern_H
#define PlaqueDrivingPattern_H

#include <Utils/SpatialMarginalsHelper.h>

#include <Graph/Model/Driving/DrivingPattern.h>

class Tuyau;
class Connexion;
class CPlaque;

class PlaqueDrivingPattern : public SymuCore::DrivingPattern
{
public:

    PlaqueDrivingPattern();
    PlaqueDrivingPattern(SymuCore::Link* pLink, CPlaque * pPlaque, Connexion * pJunction, bool bIsOrigin);

    virtual ~PlaqueDrivingPattern();

    virtual std::string toString() const;

    virtual void fillMeasuredCostsForTravelTimesUpdatePeriod(int iTravelTimesUpdatePeriodIndex, const std::vector<SymuCore::SubPopulation *> &listSubPopulations, const std::map<SymuCore::SubPopulation*, SymuCore::CostFunction> & mapCostFunctions);

    virtual void postProcessCosts(const std::vector<SymuCore::SubPopulation*>& listSubPopulations, const std::map<SymuCore::SubPopulation*, SymuCore::CostFunction> & mapCostFunctions);

public:
    CPlaque * getPlaque();

protected:
    CPlaque * m_pPlaque;
    Connexion * m_pJunction;

    // true si le pattern est un pattern de départ de plaque, false s'il s'agit d'un pattern d'arrivée en plaque
    bool      m_bIsOrigin;

    SymuCore::SpatialMarginalsHelper m_marginalsHelper;

};

#endif // PlaqueDrivingPattern_H


