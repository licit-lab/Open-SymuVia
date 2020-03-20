#pragma once

#ifndef ConnectionDrivingPenalty_H
#define ConnectionDrivingPenalty_H

#include <Utils/SpatialMarginalsHelper.h>

#include <Graph/Model/Driving/DrivingPenalty.h>

class Reseau;
class Connexion;
class BriqueDeConnexion;
class TypeVehicule;

class ConnectionDrivingPenalty : public SymuCore::DrivingPenalty
{
public:

    ConnectionDrivingPenalty(SymuCore::Node* pParentNode, const SymuCore::PatternsSwitch & patternsSwitch, Reseau * pNetwork, Connexion * pCon);

    virtual ~ConnectionDrivingPenalty();

    virtual void fillMeasuredCostsForTravelTimesUpdatePeriod(int iTravelTimesUpdatePeriodIndex, const std::vector<SymuCore::SubPopulation *> &listSubPopulations, const std::map<SymuCore::SubPopulation*, SymuCore::CostFunction> & mapCostFunctions);

    virtual void postProcessCosts(const std::vector<SymuCore::SubPopulation*>& listSubPopulations, const std::map<SymuCore::SubPopulation*, SymuCore::CostFunction> & mapCostFunctions);

    virtual void prepareTimeFrame();
    virtual void fillCost(const std::map<TypeVehicule*, SymuCore::SubPopulation *> &listSubPopulations);

protected:

    Reseau * m_pNetwork;
    Connexion * m_pConnection;

    BriqueDeConnexion * m_pBrique;

    SymuCore::SpatialMarginalsHelper m_marginalsHelper;

    bool m_bDoComputeMarginals;
};

#endif // ConnectionDrivingPenalty_H


