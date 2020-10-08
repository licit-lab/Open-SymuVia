#include "TraficSimulatorHandlerFactory.h"

#include "Simulation/Config/SimulatorConfiguration.h"
#include "Simulation/Simulators/TraficSimulatorHandler.h"
#include "SymuVia/SymuViaInstance.h"
#ifdef USE_MATLAB_MFD
#include "MFD/MFDInstance.h"
#include "MFD/MFDHandler.h"
#endif // USE_MATLAB_MFD

#include <cassert>

using namespace SymuMaster;

TraficSimulatorHandler * TraficSimulatorHandlerFactory::CreateTraficSimulatorHandler(SimulatorConfiguration * pSimConfig, TraficSimulators * pTraficSimulators, int nbSimulatorInstances)
{
    TraficSimulatorHandler * pHandler;

    switch (pSimConfig->GetSimulatorName()) {
    case ST_SymuVia:
        pHandler = new TraficSimulatorHandler(pSimConfig, pTraficSimulators);
        break;
#ifdef USE_MATLAB_MFD
    case ST_MFD:
        pHandler = new MFDHandler(pSimConfig, pTraficSimulators);
        break;
#endif
        // add other simulators here...
    default:
        pHandler = NULL;

        assert(false); // should not happen (XSD validation)

        return pHandler;
    }

    for (int iInstance = 0; iInstance < nbSimulatorInstances; iInstance++)
    {
        switch (pSimConfig->GetSimulatorName()) {
        case ST_SymuVia:
            pHandler->AddSimulatorInstance(new SymuViaInstance(pHandler, iInstance));
            break;
#ifdef USE_MATLAB_MFD
        case ST_MFD:
            pHandler->AddSimulatorInstance(new MFDInstance(pHandler, iInstance));
            break;
#endif
            // add other simulators here...
        default:
            delete pHandler;
            pHandler = NULL;

            assert(false); // should not happen (XSD validation)

            return pHandler;
        }
    }

    return pHandler;
}

