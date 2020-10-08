#pragma once

#ifndef _SYMUMASTER_USERENGINES_
#define _SYMUMASTER_USERENGINES_

#include "SymuMasterExports.h"

#include <vector>

namespace SymuCore {
    class Populations;
    class Population;
}

namespace boost {
    namespace posix_time {
        class ptime;
        class time_duration;
    }
}

#pragma warning( push )
#pragma warning( disable : 4251 )

namespace SymuMaster {

    class UserPathWriting;
    class UserEngine;
    class TraficSimulatorHandler;
    class SimulationConfiguration;

    class SYMUMASTER_EXPORTS_DLL_DEF UserEngines {

    public:
        UserEngines();

        virtual ~UserEngines();

        void Initialize(int nbInstances, const SimulationConfiguration & config, const std::vector<TraficSimulatorHandler*> & lstTraficSimulators);

        // handling commits/rollbacks
        virtual void AssignmentPeriodStart(int nbMaxInstancesNb);
        virtual void AssignmentPeriodRollback(int nbMaxInstancesNb);
        virtual void AssignmentPeriodCommit();

        const std::vector<UserEngine*> & GetUserEngines() const;
        std::vector<UserEngine*> & GetUserEngines();


    protected:
        std::vector<UserEngine*> m_lstEngines;
    };

}

#pragma warning( pop )

#endif // _SYMUMASTER_USERENGINES_
