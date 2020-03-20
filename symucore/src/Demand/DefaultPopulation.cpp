#include "DefaultPopulation.h"
#include "DefaultMacroType.h"
#include "SubPopulation.h"

using namespace SymuCore;

DefaultPopulation::DefaultPopulation() :
Population("Default Population")
{
    SetMacroType(new DefaultMacroType());
    m_listEnabledServices.push_back(ST_All);
    addSubPopulation(new SubPopulation("Default SubPopulation"));
}

DefaultPopulation::DefaultPopulation(const std::string &strName) :
Population(strName)
{
    SetMacroType(new DefaultMacroType());
    m_listEnabledServices.push_back(ST_All);
    addSubPopulation(new SubPopulation("Default SubPopulation"));
}

DefaultPopulation::~DefaultPopulation()
{

}
