#pragma once

#ifndef SYMUCORE_MACROTYPE_H
#define SYMUCORE_MACROTYPE_H

#include "SymuCoreExports.h"

#include "Utils/SymuCoreConstants.h"

#include <vector>
#include <string>

#pragma warning(push)
#pragma warning(disable : 4251)

namespace SymuCore {

class VehicleType;

class SYMUCORE_DLL_DEF MacroType {

public:

    MacroType();
    virtual ~MacroType();

    void Clear();

    void DeepCopy(const MacroType & source);

    //getters
    const std::vector<VehicleType*>& getListVehicleTypes() const;
    virtual VehicleType * getVehicleType(const std::string &strName);

    //adders
    void addVehicleType(VehicleType* vehType);

    bool operator ==(MacroType& macro);

    // to avoid creation of the vehicle type when using getVehicleType and a DefaultMacroType.
    bool hasVehicleType(const std::string &strName) const;

protected:

    std::vector<VehicleType*>       m_listVehicleTypes; //the list of all vehicle that compose this macro

};
}

#pragma warning(pop)

#endif // SYMUCORE_MACROTYPE_H
