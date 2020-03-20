#include "stdafx.h"
#include "PlaqueDrivingPattern.h"

#include "tuyau.h"
#include "Plaque.h"
#include "ZoneDeTerminaison.h"
#include "reseau.h"
#include "Connection.h"

#include <Demand/SubPopulation.h>
#include <Demand/Population.h>

using namespace SymuCore;

PlaqueDrivingPattern::PlaqueDrivingPattern()
: DrivingPattern(NULL)
{
}

PlaqueDrivingPattern::PlaqueDrivingPattern(SymuCore::Link* pLink, CPlaque * pPlaque, Connexion * pJunction, bool bIsOrigin)
: DrivingPattern(pLink)
{
    m_pPlaque = pPlaque;
    m_pJunction = pJunction;
    m_bIsOrigin = bIsOrigin;
}

PlaqueDrivingPattern::~PlaqueDrivingPattern()
{

}

CPlaque * PlaqueDrivingPattern::getPlaque()
{
    return m_pPlaque;
}

std::string PlaqueDrivingPattern::toString() const
{
    return "SubArea Pattern";
}

void PlaqueDrivingPattern::fillMeasuredCostsForTravelTimesUpdatePeriod(int iTravelTimesUpdatePeriodIndex, const std::vector<SubPopulation*>& listSubPopulations, const std::map<SymuCore::SubPopulation*, SymuCore::CostFunction> & mapCostFunctions)
{
    // on regroupe les sous-population par macro-type :
    std::map< MacroType*, std::vector<SubPopulation*> > mapSubPopsByMacroType;
    for (size_t iSubPop = 0; iSubPop < listSubPopulations.size(); iSubPop++)
    {
        SubPopulation * pSubPop = listSubPopulations[iSubPop];
        mapSubPopsByMacroType[pSubPop->GetPopulation()->GetMacroType()].push_back(pSubPop);
    }

    // Pour chaque Macro-type
    for (std::map< MacroType*, std::vector<SubPopulation*> >::const_iterator iter = mapSubPopsByMacroType.begin(); iter != mapSubPopsByMacroType.end(); ++iter)
    {
        bool bMustComputeMarginals;
        if (m_pJunction->GetReseau()->GetComputeAllCosts())
        {
            bMustComputeMarginals = true;
        }
        else
        {
            bMustComputeMarginals = false;
            for (size_t iSubPop = 0; iSubPop < iter->second.size() && !bMustComputeMarginals; iSubPop++)
            {
                if (mapCostFunctions.at(iter->second[iSubPop]) == CF_Marginals)
                {
                    bMustComputeMarginals = true;
                }
            }
        }

        MacroType * pMacroType = iter->first;

        const TimeFrame<std::map<SubPopulation *, Cost>> & costVariation = getTemporalCosts().front().getTimeFrame((size_t)iTravelTimesUpdatePeriodIndex);

        // rmq. en mode sptial, dbMarginal prend la valeur du nombre de véhicules, à stocker en plus du temps de parcours pour calculer le cout (marginal) a posteriori
        double dbMarginal, dbNbOfVehiclesForAllMacroTypes, dbTravelTimeForAllMacroTypes;
        double dbTravelTime = m_pPlaque->m_AreaHelper.GetTravelTime(m_bIsOrigin, m_pJunction, pMacroType, m_pPlaque->GetParentZone(), dbMarginal, dbNbOfVehiclesForAllMacroTypes, dbTravelTimeForAllMacroTypes);
        double dbMarginalCost;

        if (bMustComputeMarginals)
        {
            if (m_pJunction->GetReseau()->UseTravelTimesAsMarginalsInAreas())
            {
                dbMarginalCost = dbTravelTime;
            }
            else
            {
                if (m_pJunction->GetReseau()->UseSpatialTTMethod())
                {
                    dbMarginalCost = dbTravelTime;

                    m_marginalsHelper.SetNbVehicles(iTravelTimesUpdatePeriodIndex, costVariation.getStartTime(), costVariation.getEndTime(), pMacroType,
                        dbMarginal, dbNbOfVehiclesForAllMacroTypes, dbTravelTimeForAllMacroTypes);
                }
                else
                {
                    dbMarginalCost = dbMarginal;
                }
            }
        }

        for (size_t iSubPop = 0; iSubPop < iter->second.size(); iSubPop++)
        {
            SubPopulation * pSubPop = iter->second.at(iSubPop);

            Cost & cost = costVariation.getData()->operator[](pSubPop);
            cost.setTravelTime(dbTravelTime);

            if (mapCostFunctions.at(pSubPop) == CF_TravelTime)
            {
                cost.setUsedCostValue(dbTravelTime);
                if (bMustComputeMarginals)
                {
                    cost.setOtherCostValue(CF_Marginals, dbMarginalCost);
                }
            }
            else if (mapCostFunctions.at(pSubPop) == CF_Marginals)
            {
                cost.setUsedCostValue(dbMarginalCost);
                cost.setOtherCostValue(CF_TravelTime, dbTravelTime);
            }
            else
            {
                assert(false);
            }
        }
    }
}

void PlaqueDrivingPattern::postProcessCosts(const std::vector<SubPopulation*>& listSubPopulations, const std::map<SymuCore::SubPopulation*, SymuCore::CostFunction> & mapCostFunctions)
{
    if (m_pJunction->GetReseau()->UseSpatialTTMethod() && !m_pJunction->GetReseau()->UseTravelTimesAsMarginalsInAreas())
    {
        // on regroupe les sous-population par macro-type :
        std::map< MacroType*, std::vector<std::pair<CostFunction, SubPopulation* > > > mapSubPopsByMacroType;
        for (size_t iSubPop = 0; iSubPop < listSubPopulations.size(); iSubPop++)
        {
            SubPopulation * pSubPop = listSubPopulations[iSubPop];
            CostFunction subPopCostFunc = mapCostFunctions.at(pSubPop);
            if (subPopCostFunc == SymuCore::CF_Marginals || m_pJunction->GetReseau()->GetComputeAllCosts())
            {
                mapSubPopsByMacroType[pSubPop->GetPopulation()->GetMacroType()].push_back(std::pair<CostFunction, SubPopulation*>(subPopCostFunc, pSubPop));
            }
        }

        std::map<MacroType*, bool> forbiddenMacroTypes;
        for (std::map<MacroType*, std::vector<std::pair<CostFunction, SubPopulation*> > >::const_iterator iter = mapSubPopsByMacroType.begin(); iter != mapSubPopsByMacroType.end(); ++iter)
        {
            MacroType * pMacroType = iter->first;

            bool bIsForbidden = !m_pPlaque->m_AreaHelper.HasPath(m_pJunction, pMacroType, m_bIsOrigin);
            forbiddenMacroTypes[pMacroType] = bIsForbidden;
        }

        m_marginalsHelper.ComputeMarginal(mapSubPopsByMacroType, forbiddenMacroTypes, getTemporalCosts().front(), m_pJunction->GetReseau()->GetMaxMarginalsValue(), 1.0);
    }
}
