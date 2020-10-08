#pragma once

#ifndef _SYMUMASTER_SIMULATORCONFIGURATIONFACTORY_
#define _SYMUMASTER_SIMULATORCONFIGURATIONFACTORY_

#include "SymuMasterExports.h"

#include <string>

namespace SymuMaster {

    class SimulatorConfiguration;

    class SYMUMASTER_EXPORTS_DLL_DEF SimulatorConfigurationFactory {

    public:

        static SimulatorConfiguration * CreateSimulatorConfiguration(const std::string & simulatorNodeName);

    };

}

#endif // _SYMUMASTER_SIMULATORCONFIGURATIONFACTORY_