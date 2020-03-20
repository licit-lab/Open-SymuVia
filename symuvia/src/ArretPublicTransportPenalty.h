#pragma once

#ifndef ARRETPUBLICTRANSPORTPENALTY_H
#define ARRETPUBLICTRANSPORTPENALTY_H

#include <Graph/Model/Penalty.h>

class Reseau;
class Arret;
class SymuViaPublicTransportPattern;

class ArretPublicTransportPenalty : public SymuCore::Penalty
{
public:

    ArretPublicTransportPenalty(SymuCore::Node* pParentNode, const SymuCore::PatternsSwitch & patternsSwitch, Reseau * pNetwork, Arret *pArret);

    virtual ~ArretPublicTransportPenalty();

    virtual void fillMeasuredCostsForTravelTimesUpdatePeriod(int iTravelTimesUpdatePeriodIndex, const std::vector<SymuCore::SubPopulation *> &listSubPopulations, const std::map<SymuCore::SubPopulation*, SymuCore::CostFunction> & mapCostFunctions);

private:
    void ComputeTravelTimeAndMarginalWhenTakinNewLine(SymuViaPublicTransportPattern* patternAvl, bool bComingFromAnotherLine, double & dbTravelTime, double & dbMarginal);

protected:

    Reseau * m_pNetwork;
    Arret * m_pArret;
};

#endif // ARRETPUBLICTRANSPORTPENALTY_H


