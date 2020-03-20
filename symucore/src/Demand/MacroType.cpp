#include "MacroType.h"

#include "VehicleType.h"

using namespace SymuCore;


MacroType::MacroType()
{
    
}

MacroType::~MacroType()
{
    Clear();
}

void MacroType::Clear()
{
    for (size_t i = 0; i < m_listVehicleTypes.size(); i++)
    {
        delete m_listVehicleTypes[i];
    }
    m_listVehicleTypes.clear();
}

void MacroType::DeepCopy(const MacroType & source)
{
    Clear();
    for (size_t i = 0; i < source.m_listVehicleTypes.size(); i++)
    {
        VehicleType * pNewVehType = new VehicleType(*source.m_listVehicleTypes[i]);
        m_listVehicleTypes.push_back(pNewVehType);
    }
}

const std::vector<VehicleType*>& MacroType::getListVehicleTypes() const
{
    return m_listVehicleTypes;
}

void MacroType::addVehicleType(VehicleType* vehType)
{
    m_listVehicleTypes.push_back(vehType);
}

bool MacroType::operator ==(MacroType &macro)
{
    return m_listVehicleTypes == macro.getListVehicleTypes();
}

VehicleType * MacroType::getVehicleType(const std::string &strName)
{
    VehicleType * pResult = NULL;
    for (size_t i = 0; i < m_listVehicleTypes.size() && !pResult; i++)
    {
        VehicleType * pVType = m_listVehicleTypes[i];
        if (pVType->getStrName() == strName)
        {
            pResult = pVType;
        }
    }
    return pResult;
}

bool MacroType::hasVehicleType(const std::string &strName) const
{
    for (size_t i = 0; i < m_listVehicleTypes.size(); i++)
    {
        if (m_listVehicleTypes[i]->getStrName() == strName)
        {
            return true;
        }
    }
    return false;
}


