#pragma once

#ifndef SYMUCORE_DEFAULTMACROTYPE_H
#define SYMUCORE_DEFAULTMACROTYPE_H

#include "SymuCoreExports.h"

#include "Utils/SymuCoreConstants.h"
#include "Demand/MacroType.h"
#include <vector>

#pragma warning(push)
#pragma warning(disable : 4251)

namespace SymuCore {

class VehicleType;

class SYMUCORE_DLL_DEF DefaultMacroType : public MacroType {

public:

    DefaultMacroType();
    virtual ~DefaultMacroType();

    virtual VehicleType* getVehicleType(const std::string& vehTypeName);

};
}

#pragma warning(pop)

#endif // SYMUCORE_DEFAULTMACROTYPE_H
