#pragma once

#ifndef SYMUCORE_DEFAULTPOPULATION_H
#define SYMUCORE_DEFAULTPOPULATION_H

#include "SymuCoreExports.h"

#include "Utils/SymuCoreConstants.h"
#include "Demand/Population.h"

#pragma warning(push)
#pragma warning(disable : 4251)

namespace SymuCore {

class SYMUCORE_DLL_DEF DefaultPopulation : public Population{

public:

    DefaultPopulation();
    DefaultPopulation(const std::string &strName);
    virtual ~DefaultPopulation();

};
}

#pragma warning(pop)

#endif // SYMUCORE_DEFAULTPOPULATION_H
