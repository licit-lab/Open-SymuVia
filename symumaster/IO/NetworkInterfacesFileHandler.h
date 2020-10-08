#pragma once

#ifndef _SYMUMASTER_NETWORKINTERFACESFILEHANDLER_
#define _SYMUMASTER_NETWORKINTERFACESFILEHANDLER_

#include "SymuMasterExports.h"

#include <string>
#include <vector>
#include <map>

#pragma warning( push )
#pragma warning( disable : 4251 )

namespace SymuCore {
    class Graph;
    class MultiLayersGraph;
    class Node;
}

namespace SymuMaster {

    class TraficSimulatorHandler;
    class SimulationDescriptor;

    class SYMUMASTER_EXPORTS_DLL_DEF NetworkInterfacesFileHandler {

    public:
        NetworkInterfacesFileHandler();
        virtual ~NetworkInterfacesFileHandler();

		bool Load(const std::string & strFilePath, SimulationDescriptor & simulationDescriptor);
        bool Apply(SimulationDescriptor & simulationDescriptor, const std::vector<TraficSimulatorHandler*> & listSimulators);

    private:
        bool doLinks(const SimulationDescriptor & simulationDescriptor);

	private:
		std::vector<std::pair<std::pair<int, std::string>, std::pair<int, std::string> > > m_interfacesFileContents;
    };

}

#pragma warning( pop )

#endif // _SYMUMASTER_NETWORKINTERFACESFILEHANDLER_
