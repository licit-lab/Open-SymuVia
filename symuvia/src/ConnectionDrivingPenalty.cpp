#include "stdafx.h"
#include "ConnectionDrivingPenalty.h"

#include "TronconDrivingPattern.h"
#include "reseau.h"
#include "Connection.h"
#include "tuyau.h"

#include <Graph/Model/Node.h>
#include <Graph/Model/Graph.h>
#include <Demand/Population.h>
#include <Demand/SubPopulation.h>
#include <Demand/VehicleType.h>


using namespace SymuCore;

ConnectionDrivingPenalty::ConnectionDrivingPenalty(SymuCore::Node* pParentNode, const SymuCore::PatternsSwitch & patternsSwitch, Reseau * pNetwork, Connexion * pCon)
: DrivingPenalty(pParentNode, patternsSwitch)
{
    m_pNetwork = pNetwork;
    m_pConnection = pCon;

    m_pBrique = dynamic_cast<BriqueDeConnexion*>(m_pConnection);

    
    if (m_pBrique)
    {
        Tuyau * pUpstreamTuy = ((TronconDrivingPattern*)patternsSwitch.getUpstreamPattern())->GetTuyau();
        Tuyau * pDownstreamTuy = ((TronconDrivingPattern*)patternsSwitch.getDownstreamPattern())->GetTuyau();
        std::set<Tuyau*> lstTuyauxInternes = m_pBrique->GetAllTuyauxInternes(pUpstreamTuy, pDownstreamTuy);
        double dbMoveLength = 0;
        for (std::set<Tuyau*>::const_iterator iter = lstTuyauxInternes.begin(); iter != lstTuyauxInternes.end(); ++iter)
        {
            dbMoveLength += (*iter)->GetLength();
        }
        m_bDoComputeMarginals = pNetwork->GetMinLengthForMarginals() <= dbMoveLength;
    }
    else
    {
        // equivalent au test pour les briques mais pour une longueur nulle (connexions ponctuelles)
        m_bDoComputeMarginals = pNetwork->GetMinLengthForMarginals() <= 0;
    }
}

ConnectionDrivingPenalty::~ConnectionDrivingPenalty()
{

}

void ConnectionDrivingPenalty::fillMeasuredCostsForTravelTimesUpdatePeriod(int iTravelTimesUpdatePeriodIndex, const std::vector<SubPopulation*>& listSubPopulations, const std::map<SymuCore::SubPopulation*, SymuCore::CostFunction> & mapCostFunctions)
{
    Tuyau * pUpstreamTuy = ((TronconDrivingPattern*)m_PatternsSwitch.getUpstreamPattern())->GetTuyau();
    Tuyau * pDownstreamTuy = ((TronconDrivingPattern*)m_PatternsSwitch.getDownstreamPattern())->GetTuyau();

    double dbNbOfVehiclesForAllMacroTypes;
    if (m_pNetwork->UseSpatialTTMethod() && m_pBrique)
    {
        dbNbOfVehiclesForAllMacroTypes = m_pBrique->GetMeanNbVehiclesForTravelTimeUpdatePeriod(NULL, pUpstreamTuy, pDownstreamTuy);
    }

    const TimeFrame<std::map<SubPopulation *, Cost> > & costVariation = getTemporalCosts().front().getTimeFrame((size_t)iTravelTimesUpdatePeriodIndex);

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
        MacroType * pMacroType = iter->first;

        double dbTravelTime, dbCost, dbMarginalsCost = -1;

        // on teste le mouvement interdit sur le premier type de véhicule connu du macro type (ils sont sensés avoir les mêmes permissions)

        TypeVehicule* firstVehicleType = m_pNetwork->GetVehiculeTypeFromMacro(pMacroType);
        if (firstVehicleType && m_pConnection->IsMouvementAutorise(pUpstreamTuy, pDownstreamTuy, firstVehicleType, NULL))
        {
            double dbPenalty = m_pConnection->ComputeCost(pMacroType, pUpstreamTuy, pDownstreamTuy);
            dbTravelTime = dbPenalty;

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

            dbCost = dbPenalty;
            if (bMustComputeMarginals)
            {
                if (m_bDoComputeMarginals)
                {
					if (m_pNetwork->UseSpatialTTMethod() && m_pBrique && m_pNetwork->UseClassicSpatialTTMethod())
                    {
                        dbMarginalsCost = dbPenalty;

                        // m_dbTTForAllMacroTypes peut ne pas être initialisé si dbNbOfVehiclesForAllMacroTypes est nul : on prend le temps de parcours du macro-type courant.
                        double dbTravelTimeForAllMacroTypes = dbNbOfVehiclesForAllMacroTypes > 0 ? m_pBrique->m_dbTTForAllMacroTypes.at(std::make_pair(pUpstreamTuy, pDownstreamTuy)) : dbPenalty;

                        m_marginalsHelper.SetNbVehicles(iTravelTimesUpdatePeriodIndex, costVariation.getStartTime(), costVariation.getEndTime(), pMacroType,
                            m_pBrique->GetMeanNbVehiclesForTravelTimeUpdatePeriod(pMacroType, pUpstreamTuy, pDownstreamTuy), dbNbOfVehiclesForAllMacroTypes, dbTravelTimeForAllMacroTypes
                            );
                    }
                    else
                    {
                        dbMarginalsCost = m_pConnection->GetMarginal(pMacroType, pUpstreamTuy, pDownstreamTuy);
                    }
                }
                else
                {
                    dbMarginalsCost = 0;
                }
            }
        }
        else
        {
            dbTravelTime = std::numeric_limits<double>::infinity();
            dbCost = std::numeric_limits<double>::infinity();
            dbMarginalsCost = std::numeric_limits<double>::infinity();
        }

        for (size_t iSubPop = 0; iSubPop < iter->second.size(); iSubPop++)
        {
            SubPopulation * pSubPop = iter->second.at(iSubPop);
            Cost & cost = costVariation.getData()->operator[](pSubPop);
            cost.setTravelTime(dbTravelTime);

            if (mapCostFunctions.at(pSubPop) == CF_TravelTime)
            {
                cost.setUsedCostValue(dbCost);
                cost.setOtherCostValue(CF_Marginals, dbMarginalsCost);
            }
            else if (mapCostFunctions.at(pSubPop) == CF_Marginals)
            {
                cost.setUsedCostValue(dbMarginalsCost);
                cost.setOtherCostValue(CF_TravelTime, dbCost);
            }
            else
            {
                assert(false);
            }

			if (m_pNetwork->IsPollutantEmissionComputation() && m_pBrique)
			{
				double dbEmissionsForAllMacroTypes = 0;

				if(m_pBrique->m_dbEmissionsForAllMacroTypes.size()>0)
					if (m_pBrique->m_dbEmissionsForAllMacroTypes.find(std::make_pair(pUpstreamTuy, pDownstreamTuy)) != m_pBrique->m_dbEmissionsForAllMacroTypes.end())
						dbEmissionsForAllMacroTypes = m_pBrique->m_dbEmissionsForAllMacroTypes.at(std::make_pair(pUpstreamTuy, pDownstreamTuy));
				
				cost.setOtherCostValue(CF_Emissions, dbEmissionsForAllMacroTypes);
				
			}
        }
    }
}

void ConnectionDrivingPenalty::postProcessCosts(const std::vector<SubPopulation*>& listSubPopulations, const std::map<SymuCore::SubPopulation*, SymuCore::CostFunction> & mapCostFunctions)
{
    if (m_pNetwork->UseSpatialTTMethod() && m_pBrique && m_bDoComputeMarginals)
    {
        Tuyau * pUpstreamTuy = ((TronconDrivingPattern*)m_PatternsSwitch.getUpstreamPattern())->GetTuyau();
        Tuyau * pDownstreamTuy = ((TronconDrivingPattern*)m_PatternsSwitch.getDownstreamPattern())->GetTuyau();

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

            TypeVehicule* firstVehicleType = m_pNetwork->GetVehiculeTypeFromMacro(pMacroType);
            forbiddenMacroTypes[pMacroType] = !(firstVehicleType && m_pConnection->IsMouvementAutorise(pUpstreamTuy, pDownstreamTuy, firstVehicleType, NULL));
        }

		if (m_pNetwork->UseClassicSpatialTTMethod())
			m_marginalsHelper.ComputeMarginal(mapSubPopsByMacroType, forbiddenMacroTypes, getTemporalCosts().front(), m_pNetwork->GetMaxMarginalsValue(), 1);
    }
}

void ConnectionDrivingPenalty::prepareTimeFrame()
{
    getTemporalCosts().resize(1);
    getTemporalCosts()[0].addTimeFrame(0, m_pNetwork->GetDureeSimu(), boost::make_shared<std::map<SubPopulation*, Cost>>());
}

void ConnectionDrivingPenalty::fillCost(const std::map<TypeVehicule*, SymuCore::SubPopulation *> &listSubPopulations)
{
    Tuyau * pUpstreamTuy = ((TronconDrivingPattern*)m_PatternsSwitch.getUpstreamPattern())->GetTuyau();
    Tuyau * pDownstreamTuy = ((TronconDrivingPattern*)m_PatternsSwitch.getDownstreamPattern())->GetTuyau();

    const TimeFrame<std::map<SubPopulation *, Cost> > & costVariation = getTemporalCosts()[0].getTimeFrame(0);

    for (std::map<TypeVehicule*, SymuCore::SubPopulation *>::const_iterator iterTV = listSubPopulations.begin(); iterTV != listSubPopulations.end(); ++iterTV)
    {
        double dbPenalty;
        if (m_pConnection->IsMouvementAutorise(pUpstreamTuy, pDownstreamTuy, iterTV->first, NULL))
        {
            dbPenalty = m_pConnection->ComputeCost(iterTV->first, pUpstreamTuy, pDownstreamTuy);
            if (dbPenalty >= DBL_MAX)
            {
                dbPenalty = std::numeric_limits<double>::infinity();
            }
        }
        else
        {
            dbPenalty = std::numeric_limits<double>::infinity();
        }

        Cost & cost = costVariation.getData()->operator[](iterTV->second);
        cost.setTravelTime(dbPenalty);
        cost.setUsedCostValue(dbPenalty);
    }
}


