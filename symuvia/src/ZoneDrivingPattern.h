#pragma once

#ifndef ZoneDrivingPattern_H
#define ZoneDrivingPattern_H

#include "SymuViaDrivingPattern.h"

#include <Utils/SpatialMarginalsHelper.h>

class Tuyau;
class ZoneDeTerminaison;
class Connexion;
class TypeVehicule;

class ZoneDrivingPattern : public SymuViaDrivingPattern
{
public:

    ZoneDrivingPattern();
    ZoneDrivingPattern(SymuCore::Link* pLink, ZoneDeTerminaison * pZone, Connexion * pJunction, bool bIsOrigin);

    virtual ~ZoneDrivingPattern();

    virtual std::string toString() const;

    virtual void fillMeasuredCostsForTravelTimesUpdatePeriod(int iTravelTimesUpdatePeriodIndex, const std::vector<SymuCore::SubPopulation *> &listSubPopulations, const std::map<SymuCore::SubPopulation*, SymuCore::CostFunction> & mapCostFunctions);

    virtual void postProcessCosts(const std::vector<SymuCore::SubPopulation*>& listSubPopulations, const std::map<SymuCore::SubPopulation*, SymuCore::CostFunction> & mapCostFunctions);

    virtual void fillCost(const std::map<TypeVehicule*, SymuCore::SubPopulation *> &listSubPopulations);

public:
    ZoneDeTerminaison * getZone();

private:
    ZoneDeTerminaison * m_pZone;
    Connexion * m_pJunction;

    // true si le pattern est un pattern de départ de zone, false s'il s'agit d'un pattern d'arrivée en zone
    bool      m_bIsOrigin;

    SymuCore::SpatialMarginalsHelper m_marginalsHelper;
};

#endif // ZoneDrivingPattern_H


