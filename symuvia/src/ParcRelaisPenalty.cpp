#include "stdafx.h"
#include "ParcRelaisPenalty.h"

#include "Parking.h"
#include "reseau.h"

#include <Graph/Model/Pattern.h>
#include <Demand/SubPopulation.h>
#include <Demand/Population.h>

using namespace SymuCore;

ParcRelaisPenalty::ParcRelaisPenalty(SymuCore::Node* pParentNode, const SymuCore::PatternsSwitch & patternsSwitch, Parking * pParking)
: SymuCore::DrivingPenalty(pParentNode, patternsSwitch)
{
    m_pParking = pParking;
}

ParcRelaisPenalty::~ParcRelaisPenalty()
{

}

void ParcRelaisPenalty::fillMeasuredCostsForTravelTimesUpdatePeriod(int iTravelTimesUpdatePeriodIndex, const std::vector<SubPopulation*>& listSubPopulations, const std::map<SymuCore::SubPopulation*, SymuCore::CostFunction> & mapCostFunctions)
{
    const TimeFrame<std::map<SubPopulation *, Cost> > & costVariation = getTemporalCosts().front().getTimeFrame((size_t)iTravelTimesUpdatePeriodIndex);

    // on regroupe les sous-population par macro-type :
    for (size_t iSubPop = 0; iSubPop < listSubPopulations.size(); iSubPop++)
    {
        SubPopulation * pSubPop = listSubPopulations[iSubPop];

        bool bCanEnter = false;
        MacroType * pMacroType = pSubPop->GetPopulation()->GetMacroType();
        if (pMacroType && m_pParking->IsAllowedVehiculeType(m_pParking->GetNetwork()->GetVehiculeTypeFromMacro(pMacroType)))
        {
            bCanEnter = true;

            // dans le cas de l'entrée dans le parking, on doit vérifier en plus si le parking est plein ou non.
            if (m_PatternsSwitch.getDownstreamPattern()->getPatternType() == PT_Walk && m_pParking->IsFull())
            {
                bCanEnter = false;
            }
        }

        Cost & cost = costVariation.getData()->operator[](pSubPop);

        double dbTravelTime = bCanEnter ? 0 : std::numeric_limits<double>::infinity();

        cost.setTravelTime(dbTravelTime);

        switch (mapCostFunctions.at(pSubPop))
        {
            // Pour ce type de pénalité, le marginal est le temps de parcours
        case SymuCore::CF_Marginals:
            cost.setUsedCostValue(dbTravelTime);
            cost.setOtherCostValue(CF_TravelTime, dbTravelTime);
            break;
        case SymuCore::CF_TravelTime:
            
            cost.setUsedCostValue(dbTravelTime);
            cost.setOtherCostValue(CF_Marginals, dbTravelTime);
            break;
        default:
            assert(false); // unimplemented cost function
            break;
        }
    }
}
