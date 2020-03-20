#pragma once

#ifndef SYMUCORE_VEHICLETYPE_H
#define SYMUCORE_VEHICLETYPE_H

#include "SymuCoreExports.h"
#include <string>

#pragma warning(push)
#pragma warning(disable : 4251)

namespace SymuCore {

class SYMUCORE_DLL_DEF VehicleType {

public:

    VehicleType();
    VehicleType(const std::string &strName);
    virtual ~VehicleType();

    //getters
    const std::string & getStrName() const;

    //setters
    void setStrName(const std::string &strName);

private:

    std::string                m_strName; //name of the vehicle

};
}

#pragma warning(pop)

#endif // SYMUCORE_VEHICLETYPE_H
