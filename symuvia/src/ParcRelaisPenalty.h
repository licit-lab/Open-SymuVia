#pragma once

#ifndef PARCRELAISPENALTY_H
#define PARCRELAISPENALTY_H

#include <Graph/Model/Driving/DrivingPenalty.h>

class Parking;

class ParcRelaisPenalty : public SymuCore::DrivingPenalty
{
public:

    ParcRelaisPenalty(SymuCore::Node* pParentNode, const SymuCore::PatternsSwitch & patternsSwitch, Parking * pParking);

    virtual ~ParcRelaisPenalty();

    virtual void fillMeasuredCostsForTravelTimesUpdatePeriod(int iTravelTimesUpdatePeriodIndex, const std::vector<SymuCore::SubPopulation *> &listSubPopulations, const std::map<SymuCore::SubPopulation*, SymuCore::CostFunction> & mapCostFunctions);

protected:
    Parking * m_pParking;
};

#endif // PARCRELAISPENALTY_H


