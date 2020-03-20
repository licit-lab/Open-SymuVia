#include "stdafx.h"
#include "VehicleTypePenalty.h"

#include "reseau.h"

#include <Demand/SubPopulation.h>
#include <Demand/Population.h>

using namespace SymuCore;

SymuCore::Cost VehicleTypePenalty::m_NullCost(0, 0);
SymuCore::Cost VehicleTypePenalty::m_InfCost(std::numeric_limits<double>::infinity(), std::numeric_limits<double>::infinity());

VehicleTypePenalty::VehicleTypePenalty(Node* pParentNode, const PatternsSwitch & patternsSwitch, Reseau * pNetwork, const std::set<TypeVehicule*> & lstALlowedVehicleTypes)
: SymuCore::AbstractPenalty(pParentNode, patternsSwitch),
m_pNetwork(pNetwork),
m_lstAllowedVehicleTypes(lstALlowedVehicleTypes)
{
}

VehicleTypePenalty::~VehicleTypePenalty()
{

}

Cost* VehicleTypePenalty::getPenaltyCost(int iSimulationInstance, double t, SymuCore::SubPopulation* pSubPopulation)
{
    SymuCore::MacroType * pMacroType = pSubPopulation->GetPopulation()->GetMacroType();
    TypeVehicule * pTypeVeh = m_pNetwork->GetVehiculeTypeFromMacro(pMacroType);
    if (m_lstAllowedVehicleTypes.find(pTypeVeh) != m_lstAllowedVehicleTypes.end())
    {
        return &m_NullCost;
    }
    else
    {
        return &m_InfCost;
    }
}

void VehicleTypePenalty::fillFromSecondaryInstance(AbstractPenalty * pFrom, int iInstance)
{
    //NO OP
}