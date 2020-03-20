#pragma once

#ifndef SYMUCORE_POPULATIONS_H
#define SYMUCORE_POPULATIONS_H

#include "SymuCoreExports.h"

#include <vector>
#include <string>

#pragma warning(push)
#pragma warning(disable : 4251)

namespace SymuCore {

class Population;
class SubPopulation;
class VehicleType;
class MacroType;
class MultiLayersGraph;

class SYMUCORE_DLL_DEF Populations {

public:

    Populations();
    virtual ~Populations();

    void Clear();
    void DeepCopy(const Populations & source, MultiLayersGraph * pGraph);

    const std::vector<Population*> & getListPopulations() const;
    std::vector<Population*> & getListPopulations();

    void getPopulationAndVehicleType(Population ** pPopulation, SubPopulation ** pSubPopulation, VehicleType ** pVehType, std::string strVehicleTypeName = std::string(), std::string strMotive = std::string()) const;
    Population * getPopulation(const std::string & strPopulationName) const;

    double getMaxInitialWalkRadius() const;
    double getMaxIntermediateWalkRadius() const;

private:
    std::vector<Population*> m_listPopulations;
};
}

#pragma warning(pop)

#endif // SYMUCORE_POPULATIONS_H
