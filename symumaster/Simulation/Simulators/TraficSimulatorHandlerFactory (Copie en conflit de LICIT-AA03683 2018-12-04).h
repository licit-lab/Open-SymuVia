#pragma once

#ifndef _SYMUMASTER_TRAFICSIMULATORHANDLERFACTORY_
#define _SYMUMASTER_TRAFICSIMULATORHANDLERFACTORY_

#include "SymuMasterExports.h"

#include <string>

namespace SymuMaster {

    class TraficSimulatorHandler;
    class SimulatorConfiguration;

    class SYMUMASTER_EXPORTS_DLL_DEF TraficSimulatorHandlerFactory {

    public:

        static TraficSimulatorHandler * CreateTraficSimulatorHandler(SimulatorConfiguration * pSimConfig);

    };

}

#endif // _SYMUMASTER_TRAFICSIMULATORHANDLERFACTORY_