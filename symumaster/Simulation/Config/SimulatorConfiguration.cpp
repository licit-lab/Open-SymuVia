#include "SimulatorConfiguration.h"

using namespace SymuMaster;

SimulatorConfiguration::SimulatorConfiguration()
{
}

SimulatorConfiguration::~SimulatorConfiguration()
{
}

ESimulatorName SimulatorConfiguration::GetSimulatorName() const
{
    return m_eSimulatorName;
}
