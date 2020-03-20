#pragma once

#ifndef VEHICLETYPEPENALTY_H
#define VEHICLETYPEPENALTY_H

#include <Graph/Model/AbstractPenalty.h>

#include <set>

class Reseau;
class TypeVehicule;

class VehicleTypePenalty : public SymuCore::AbstractPenalty
{
public:

    VehicleTypePenalty(SymuCore::Node* pParentNode, const SymuCore::PatternsSwitch & patternsSwitch, Reseau * pNetwork, const std::set<TypeVehicule*> & lstALlowedVehicleTypes);

    virtual ~VehicleTypePenalty();

    virtual void prepareTimeFrames(double startPeriodTime, double endPeriodTime, double travelTimesUpdatePeriod, const std::vector<SymuCore::SubPopulation*>& listSubPopulations, const std::map<SymuCore::SubPopulation*, SymuCore::CostFunction> & mapCostFunctions, int nbSimulationInstances, int iInstance) {}
    virtual void fillMeasuredCostsForTravelTimesUpdatePeriod(int iTravelTimesUpdatePeriodIndex, const std::vector<SymuCore::SubPopulation*>& listSubPopulations, const std::map<SymuCore::SubPopulation*, SymuCore::CostFunction> & mapCostFunctions) {}

    virtual SymuCore::Cost* getPenaltyCost(int iSimulationInstance, double t, SymuCore::SubPopulation* pSubPopulation);

    virtual void fillFromSecondaryInstance(AbstractPenalty * pFrom, int iInstance);

protected:

    Reseau*                 m_pNetwork;
    std::set<TypeVehicule*> m_lstAllowedVehicleTypes;

    static SymuCore::Cost m_NullCost;
    static SymuCore::Cost m_InfCost;
};

#endif // VEHICLETYPEPENALTY_H


