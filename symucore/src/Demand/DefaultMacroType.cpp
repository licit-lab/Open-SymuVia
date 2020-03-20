#include "DefaultMacroType.h"

#include "VehicleType.h"

using namespace SymuCore;


DefaultMacroType::DefaultMacroType()
{
    
}

DefaultMacroType::~DefaultMacroType()
{
    
}

VehicleType *DefaultMacroType::getVehicleType(const std::string &vehTypeName)
{
    VehicleType* newVehType = MacroType::getVehicleType(vehTypeName);
    if(!newVehType)
    {
        newVehType = new VehicleType(vehTypeName);
        addVehicleType(newVehType);
    }

    return newVehType;
}
