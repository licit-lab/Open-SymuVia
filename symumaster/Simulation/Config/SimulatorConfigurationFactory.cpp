#include "SimulatorConfigurationFactory.h"

#include "Simulation/Config/SimulatorConfiguration.h"
#include "Simulation/Simulators/SymuVia/SymuViaConfig.h"
#ifdef USE_MATLAB_MFD
#include "Simulation/Simulators/MFD/MFDConfig.h"
#endif // USE_MATLAB_MFD

#include <cassert>

using namespace SymuMaster;

SimulatorConfiguration * SimulatorConfigurationFactory::CreateSimulatorConfiguration(const std::string & simulatorNodeName)
{
    SimulatorConfiguration * pResult;
    ESimulatorName simName = SymuMasterConstants::GetSimulatorNameFromXMLNodeName(simulatorNodeName);
    switch (simName)
    {
    case ST_SymuVia:
        pResult = new SymuViaConfig();
        break;
#ifdef USE_MATLAB_MFD
    case ST_MFD:
        pResult = new MFDConfig();
        break;
#endif // USE_MATLAB_MFD
    case ST_Unknown:
    default:
        pResult = NULL;
        assert(false);
        break;
    }
    
    return pResult;
}

