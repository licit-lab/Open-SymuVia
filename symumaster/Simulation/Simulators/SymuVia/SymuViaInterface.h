#pragma once

#ifndef _SYMUMASTER_SYMUVIA_INTERFACE_
#define _SYMUMASTER_SYMUVIA_INTERFACE_

#include "Simulation/Simulators/SimulatorsInterface.h"

namespace SymuMaster {

    class SYMUMASTER_EXPORTS_DLL_DEF SymuViaInterface : public AbstractSimulatorInterface {

    public:
        SymuViaInterface();
        SymuViaInterface(TraficSimulatorHandler * pSimulator, const std::string & strName, bool bUpstream);

        virtual bool Check(std::vector<AbstractSimulatorInterface*> & associatedInterfaces) const;

        virtual void BuildUpstreamPenalties(SymuCore::Pattern * pPatternToVirtualNode);
        virtual void BuildDownstreamPenalties(SymuCore::Pattern * pPatternFromVirtualNode);

		virtual bool CanRestrictCapacity() const;
		virtual bool CanComputeOffer() const;

    };
}

#endif // _SYMUMASTER_SYMUVIA_INTERFACE_
