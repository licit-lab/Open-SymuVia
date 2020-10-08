#pragma once

#ifndef _SYMUMASTER_MFDHANDLER_
#define _SYMUMASTER_MFDHANDLER_

#include "Simulation/Simulators/TraficSimulatorHandler.h"

#include <Graph/Model/Cost.h>

#pragma warning(push)
#pragma warning(disable : 4251)

namespace SymuMaster {

    class SYMUMASTER_EXPORTS_DLL_DEF MFDHandler : public TraficSimulatorHandler {

    public:
        MFDHandler(const SimulatorConfiguration * pConfig, TraficSimulators * pParentTraficSimulators);

        virtual ~MFDHandler();

        const std::map<int, std::map<SymuCore::CostFunction, SymuCore::Cost> > & GetMapCost(int iInstance) const;
        std::map<int, std::map<SymuCore::CostFunction, SymuCore::Cost> > & GetMapCost(int iInstance);

        virtual void AddSimulatorInstance(TraficSimulatorInstance * pSimInstance);

    private:
        std::vector<std::map<int, std::map<SymuCore::CostFunction, SymuCore::Cost> > > m_lstMapCostByRoute;
    };

}

#pragma warning(pop)

#endif // _SYMUMASTER_MFDHANDLER_
