#pragma once

#ifndef _SYMUMASTER_DEMANDFILEHANDLER_
#define _SYMUMASTER_DEMANDFILEHANDLER_

#include "SymuMasterExports.h"

#include <string>
#include <map>

namespace SymuCore {
    class Populations;
    class Graph;
    class Trip;
}

namespace boost {
    namespace posix_time {
        class ptime;
    }
}

#pragma warning( push )
#pragma warning( disable : 4251 )

namespace SymuMaster {

    class SimulationDescriptor;
    class SimulationConfiguration;

    class SYMUMASTER_EXPORTS_DLL_DEF DemandFileHandler {

    public:
        DemandFileHandler();
        virtual ~DemandFileHandler();

        bool FillDemands(const std::string & strCSVFilePath, const boost::posix_time::ptime &startSimulationTime, const boost::posix_time::ptime &endSimulationTime,
                         SymuCore::Populations &pPopulations, SymuCore::Graph *pGraph);
        bool AddUsersPredefinedPaths(SymuCore::Graph *pGraph, int nbInstances);
        bool CheckOriginAndDestinations(SymuCore::Graph *pGraph);

        bool Write(const SimulationConfiguration & config, const SimulationDescriptor & pSimDesc);

    private:
        std::string BuildAvailableDemandFileName(const SimulationConfiguration & config);

    private:
        std::map<SymuCore::Trip*, std::string> m_mapPaths; // contains predefineds paths that were read on FillDemands

    };

}

#pragma warning( pop )

#endif // _SYMUMASTER_DEMANDFILEHANDLER_
