#include "stdafx.h"
#include "ArretPublicTransportPenalty.h"

#include "SymuViaPublicTransportPattern.h"
#include "usage/PublicTransportFleet.h"
#include "reseau.h"
#include "arret.h"
#include "tuyau.h"
#include "usage/PublicTransportLine.h"

#include <Graph/Model/Node.h>
#include <Graph/Model/Graph.h>
#include <Graph/Model/Penalty.h>

#include <Utils/TravelTimeUtils.h>

#include <boost/make_shared.hpp>

using namespace SymuCore;

ArretPublicTransportPenalty::ArretPublicTransportPenalty(SymuCore::Node* pParentNode, const SymuCore::PatternsSwitch & patternsSwitch, Reseau * pNetwork, Arret * pArret)
: SymuCore::Penalty(pParentNode, patternsSwitch)
{
    m_pNetwork = pNetwork;
    m_pArret = pArret;
}

ArretPublicTransportPenalty::~ArretPublicTransportPenalty()
{

}

void ArretPublicTransportPenalty::ComputeTravelTimeAndMarginalWhenTakinNewLine(SymuViaPublicTransportPattern* patternAvl, bool bComingFromAnotherLine, double & dbTravelTime, double & dbMarginal)
{
    TravelTimeUtils::ComputeTravelTimeAndMarginalAtBusStation(m_pNetwork->GetMeanBusExitTime(),
        patternAvl->GetSymuViaLine()->GetLastFrequency(m_pArret),
        m_pArret->getLastRatioMontee(patternAvl->GetSymuViaLine()),
        patternAvl->GetSymuViaLine()->GetLastStopDuration(m_pArret),
        bComingFromAnotherLine,
        dbTravelTime, dbMarginal); // sorties
}

void ArretPublicTransportPenalty::fillMeasuredCostsForTravelTimesUpdatePeriod(int iTravelTimesUpdatePeriodIndex, const std::vector<SubPopulation*>& listSubPopulations, const std::map<SymuCore::SubPopulation*, SymuCore::CostFunction> & mapCostFunctions)
{
    double dbTravelTime;
    double dbMarginal;

    SymuViaPublicTransportPattern* patternAmt = dynamic_cast<SymuViaPublicTransportPattern*>(m_PatternsSwitch.getUpstreamPattern());
    SymuViaPublicTransportPattern* patternAvl = dynamic_cast<SymuViaPublicTransportPattern*>(m_PatternsSwitch.getDownstreamPattern());
    if (patternAmt && patternAvl)
    {
        if (patternAmt->getLine() == patternAvl->getLine())
        {
            // Cas du passager qui reste et ne descend pas à l'arrêt :
            dbTravelTime = patternAmt->GetSymuViaLine()->GetLastStopDuration(m_pArret);
            dbMarginal = dbTravelTime;
        }
        else
        {
            ComputeTravelTimeAndMarginalWhenTakinNewLine(patternAvl, true, dbTravelTime, dbMarginal);
        }
    }
    else if (patternAmt)
    {
        // Cas du pattern marche à pieds en aval : facile, il n'y a qu'à descendre du bus
        dbTravelTime = m_pNetwork->GetMeanBusExitTime();
        dbMarginal = dbTravelTime;
    }
    else if (patternAvl)
    {
        // Cas du pattern marche à pieds en amont : idem que pour un changement de ligne sauf qu'on n'a pas de temps de descente
        ComputeTravelTimeAndMarginalWhenTakinNewLine(patternAvl, false, dbTravelTime, dbMarginal);
    }
    else
    {
        // Cas marche vers marche : aucune pénalité.
        // pour le SG, on interdit le transferts via marche à pieds aux arrêts. Sinon, on aurait 
        // le problème du nombre d'itinéraires demandés à gérer car doublons à supprimer.
        if (m_pNetwork->IsWithPublicTransportGraph())
        {
            dbTravelTime = std::numeric_limits<double>::infinity();
            dbMarginal = std::numeric_limits<double>::infinity();
        }
        else
        {
            dbTravelTime = 0;
            dbMarginal = 0;
        }
    }

    std::map<SubPopulation*, Cost> * pMapCosts = getTemporalCosts().front().getData((size_t)iTravelTimesUpdatePeriodIndex);

    for (size_t iSubPop = 0; iSubPop < listSubPopulations.size(); iSubPop++)
    {
        SubPopulation * pSubPop = listSubPopulations.at(iSubPop);
        Cost & cost = pMapCosts->operator[](pSubPop);

        cost.setTravelTime(dbTravelTime);

        switch (mapCostFunctions.at(pSubPop))
        {
        case SymuCore::CF_Marginals:
            cost.setOtherCostValue(SymuCore::CF_TravelTime, dbTravelTime);
            cost.setUsedCostValue(dbMarginal);
            break;
        case SymuCore::CF_TravelTime:
            cost.setOtherCostValue(SymuCore::CF_Marginals, dbMarginal);
            cost.setUsedCostValue(dbTravelTime);
            break;
        default:
            assert(false); // unimplemented cost function
            break;
        }
    }
}
