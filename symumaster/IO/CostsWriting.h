#pragma once

#ifndef _SYMUMASTER_CostsWriting_
#define _SYMUMASTER_CostsWriting_

#include "SymuMasterExports.h"

#include <boost/date_time/posix_time/ptime.hpp>

#include <string>

#pragma warning( push )
#pragma warning( disable : 4251 )

namespace SymuCore {
    class Graph;
    class Populations;
}

namespace SymuMaster {

    class SimulationConfiguration;
    class SimulationDescriptor;

    class SYMUMASTER_EXPORTS_DLL_DEF CostsWriting {

	public:
        CostsWriting(const SimulationConfiguration &config, const SimulationDescriptor & simulationDescriptor);

        virtual ~CostsWriting();

        bool WriteCosts(int iPeriod, int iIteration, int iInstance, int iInstanceInCurrentBatch, const boost::posix_time::ptime &startPeriodTime, const boost::posix_time::ptime &endPeriodTime, double dbMiddleTime, const SymuCore::Populations & populations, SymuCore::Graph * pGraph);

    private:

        const SimulationDescriptor &        m_SimulationDescriptor;

        std::string                         m_strOutpuDirectoryPath;

        std::string                         m_strOutputPrefix;
    };

}

#pragma warning( pop )

#endif // _SYMUMASTER_CostsWriting_
