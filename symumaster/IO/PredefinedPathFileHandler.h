#pragma once

#ifndef _SYMUMASTER_PREDEFINEDPATHFILEHANDLER_
#define _SYMUMASTER_PREDEFINEDPATHFILEHANDLER_

#include "SymuMasterExports.h"

#include <Simulation/Assignment/AssignmentModel.h>

#include <string>
#include <map>
#include <vector>

namespace SymuCore {
    class Populations;
    class SubPopulation;
    class Graph;
    class Path;
    class Origin;
    class Destination;
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

    class SYMUMASTER_EXPORTS_DLL_DEF PredefinedPathFileHandler {

    public:
        PredefinedPathFileHandler();
        virtual ~PredefinedPathFileHandler();

        bool FillPredefinedPaths(const std::string &strCSVFilePath, SymuCore::Graph* pGraph, SymuCore::Populations &pPopulations,
                        std::map<SymuCore::SubPopulation *, std::map<SymuCore::Origin *, std::map<SymuCore::Destination *, std::map<SymuCore::Path, std::vector<double> >, AssignmentModel::SortDestination >, AssignmentModel::SortOrigin >, AssignmentModel::SortSubPopulation > &mapPredefinedPaths);

    };

}

#pragma warning( pop )

#endif // _SYMUMASTER_PREDEFINEDPATHFILEHANDLER_
