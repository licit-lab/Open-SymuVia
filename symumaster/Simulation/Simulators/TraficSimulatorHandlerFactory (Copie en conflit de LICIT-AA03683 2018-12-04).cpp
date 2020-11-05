#include "TraficSimulatorHandlerFactory.h"

#include "Simulation/Config/SimulatorConfiguration.h"
#include "SymuVia/SymuViaHandler.h"
#ifdef USE_MATLAB_MFD
#include "MFD/MFDHandler.h"
#endif // USE_MATLAB_MFD

#include <cassert>

using namespace SymuMaster;

TraficSimulatorHandler * TraficSimulatorHandlerFactory::CreateTraficSimulatorHandler(SimulatorConfiguration * pSimConfig)
{
    switch (pSimConfig->GetSimulatorName()) {
    case ST_SymuVia:
        return new SymuViaHandler(pSimConfig);
#ifdef USE_MATLAB_MFD
    case ST_MFD:
        return new MFDHandler(pSimConfig);
#endif
    // add other simulators here...
    default:
        assert(false); // should not happen
    }

    return NULL;
}

