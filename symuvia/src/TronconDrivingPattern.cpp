#include "stdafx.h"
#include "TronconDrivingPattern.h"

#include "reseau.h"
#include "tuyau.h"

#include <Graph/Model/Link.h>
#include <Graph/Model/Graph.h>
#include <Graph/Model/Cost.h>
#include <Demand/Population.h>
#include <Demand/SubPopulation.h>

#include <boost/log/trivial.hpp>


using namespace SymuCore;

TronconDrivingPattern::TronconDrivingPattern(SymuCore::Link* pLink, Reseau * pNetwork, TuyauMicro * pTuyau)
: SymuViaDrivingPattern(pLink, pNetwork)
{
    m_pTuyau = pTuyau;

    m_bDoComputeMarginals = pTuyau->GetLength() >= pNetwork->GetMinLengthForMarginals();
}

TronconDrivingPattern::~TronconDrivingPattern()
{

}

std::string TronconDrivingPattern::toString() const
{
    return m_pTuyau->GetLabel();
}

TuyauMicro * TronconDrivingPattern::GetTuyau() const
{
    return m_pTuyau;
}

void TronconDrivingPattern::fillMeasuredCostsForTravelTimesUpdatePeriod(int iTravelTimesUpdatePeriodIndex, const std::vector<SubPopulation*>& listSubPopulations, const std::map<SymuCore::SubPopulation*, SymuCore::CostFunction> & mapCostFunctions)
{
    double dbNbOfVehiclesForAllMacroTypes = m_pTuyau->GetMeanNbVehiclesForTravelTimeUpdatePeriod(NULL);

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
        if (m_pNetwork->GetComputeAllCosts())
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

        assert(pMacroType); // a réfléchir si ca peut arriver (devrait être interdit ?)

        const TimeFrame<std::map<SubPopulation*, Cost>> & costVariation = getTemporalCosts()[0].getTimeFrame((size_t)iTravelTimesUpdatePeriodIndex);

        double dbTravelTime = m_pTuyau->GetTravelTime(pMacroType);
        double dbCostValue = dbTravelTime * m_pTuyau->GetPenalisation();
        double dbMarginalsValue;

        if (bMustComputeMarginals)
        {
            if (m_bDoComputeMarginals)
            {
				if (m_pNetwork->UseSpatialTTMethod() && m_pNetwork->UseClassicSpatialTTMethod())
                {
                    dbMarginalsValue = dbTravelTime;

                    // m_dbTTForAllMacroTypes peut ne pas être initialisé si dbNbOfVehiclesForAllMacroTypes est nul : on prend le temps de parcours du macro-type courant.
                    double dbTravelTimeForAllMacroTypes = dbNbOfVehiclesForAllMacroTypes > 0 ? m_pTuyau->m_dbTTForAllMacroTypes : dbTravelTime;

                    m_marginalsHelper.SetNbVehicles(iTravelTimesUpdatePeriodIndex, costVariation.getStartTime(), costVariation.getEndTime(), pMacroType,
                        m_pTuyau->GetMeanNbVehiclesForTravelTimeUpdatePeriod(pMacroType), dbNbOfVehiclesForAllMacroTypes, dbTravelTimeForAllMacroTypes);
                }
                else
                {
                    dbMarginalsValue = m_pTuyau->GetMarginal(pMacroType);
                }

                // Application de la pénalisation éventuelle sur le cout.
                dbMarginalsValue *= m_pTuyau->GetPenalisation();
            }
            else
            {
                dbMarginalsValue = 0;
            }
        }

        for (size_t iSubPop = 0; iSubPop < iter->second.size(); iSubPop++)
        {
            SubPopulation * pSubPop = iter->second.at(iSubPop);

            Cost & cost = costVariation.getData()->operator[](pSubPop);

            cost.setTravelTime(dbTravelTime);

            if (mapCostFunctions.at(pSubPop) == CF_TravelTime)
            {
	            // TODO - DEBUG - on laisse pour signaler d'�ventuels autres cas o� SymuVia nous donne un temps de parcours n�gatif...
                if (dbCostValue < 0)
                {
                    BOOST_LOG_TRIVIAL(error) << "negative travel time : " << dbCostValue << " for link " << m_pTuyau->GetLabel();
                }
                cost.setUsedCostValue(dbCostValue);

				if (bMustComputeMarginals)
					cost.setOtherCostValue(CF_Marginals, dbMarginalsValue);
            }
            else if (mapCostFunctions.at(pSubPop) == CF_Marginals)
            {
                cost.setUsedCostValue(dbMarginalsValue);
                cost.setOtherCostValue(CF_TravelTime, dbCostValue);
            }
            else
            {
                assert(false);
            }

			if (m_pNetwork->IsPollutantEmissionComputation())
			{
				cost.setOtherCostValue(CF_Emissions, m_pTuyau->GetEmissionForAllMacroTypes());
			}
        }
    }
}

void TronconDrivingPattern::postProcessCosts(const std::vector<SubPopulation*>& listSubPopulations, const std::map<SymuCore::SubPopulation*, SymuCore::CostFunction> & mapCostFunctions)
{
    if (m_pNetwork->UseSpatialTTMethod() && m_bDoComputeMarginals)
    {
        // on regroupe les sous-population par macro-type :
        std::map< MacroType*, std::vector<std::pair<CostFunction, SubPopulation*> > > mapSubPopsByMacroType;
        for (size_t iSubPop = 0; iSubPop < listSubPopulations.size(); iSubPop++)
        {
            SubPopulation * pSubPop = listSubPopulations[iSubPop];
            CostFunction subPopCostFunc = mapCostFunctions.at(pSubPop);
            if (subPopCostFunc == SymuCore::CF_Marginals || m_pNetwork->GetComputeAllCosts())
            {
                mapSubPopsByMacroType[pSubPop->GetPopulation()->GetMacroType()].push_back(std::pair<CostFunction, SubPopulation*>(subPopCostFunc, pSubPop));
            }
        }

        std::map<MacroType*, bool> forbiddenMacroTypes;
        for (std::map<MacroType*, std::vector<std::pair<CostFunction, SubPopulation*> > >::const_iterator iter = mapSubPopsByMacroType.begin(); iter != mapSubPopsByMacroType.end(); ++iter)
        {
            MacroType * pMacroType = iter->first;

            TypeVehicule * pTypeVeh = m_pNetwork->GetVehiculeTypeFromMacro(pMacroType);
            double dbTpsVitReg = m_pTuyau->GetCoutAVide(m_pNetwork->m_dbInstSimu, pTypeVeh, false, true);
            bool bIsForbiddenLink = dbTpsVitReg >= DBL_MAX || !pTypeVeh;
            forbiddenMacroTypes[pMacroType] = bIsForbiddenLink;
        }

		if (m_pNetwork->UseClassicSpatialTTMethod())
			m_marginalsHelper.ComputeMarginal(mapSubPopsByMacroType, forbiddenMacroTypes, getTemporalCosts().front(), m_pNetwork->GetMaxMarginalsValue(), m_pTuyau->GetPenalisation());
    }
}

void TronconDrivingPattern::fillCost(const std::map<TypeVehicule*, SymuCore::SubPopulation *> &listSubPopulations)
{
    const TimeFrame<std::map<SubPopulation *, Cost> > & costVariation = getTemporalCosts()[0].getTimeFrame(0);

    for (std::map<TypeVehicule*, SymuCore::SubPopulation *>::const_iterator iterTV = listSubPopulations.begin(); iterTV != listSubPopulations.end(); ++iterTV)
    {
        double dbCost = m_pTuyau->ComputeCost(m_pNetwork->m_dbInstSimu, iterTV->first, true);

        if (dbCost >= DBL_MAX)
        {
            dbCost = std::numeric_limits<double>::infinity();
        }

        Cost & cost = costVariation.getData()->operator[](iterTV->second);
        cost.setTravelTime(dbCost);
        cost.setUsedCostValue(dbCost);
    }
}


