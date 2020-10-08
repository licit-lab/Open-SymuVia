#pragma once

#ifndef _SYMUMASTER_MFD_INTERFACE_
#define _SYMUMASTER_MFD_INTERFACE_

#include "Simulation/Simulators/SimulatorsInterface.h"

namespace SymuMaster {

    class SYMUMASTER_EXPORTS_DLL_DEF MFDInterface : public AbstractSimulatorInterface {

    public:
        MFDInterface();
        MFDInterface(TraficSimulatorHandler * pSimulator, const std::string & strName, bool bUpstream);

        virtual bool Check(std::vector<AbstractSimulatorInterface*> & associatedInterfaces) const;

        virtual void BuildUpstreamPenalties(SymuCore::Pattern * pPatternToVirtualNode);
        virtual void BuildDownstreamPenalties(SymuCore::Pattern * pPatternFromVirtualNode);

		virtual bool CanRestrictCapacity() const;
		virtual bool CanComputeOffer() const;

    private:
        int m_iRouteID;
    };
}

#endif // _SYMUMASTER_MFD_INTERFACE_
