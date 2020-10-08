#pragma once

#ifndef _SYMUMASTER_SIMULATORS_INTERFACE_
#define _SYMUMASTER_SIMULATORS_INTERFACE_

#include "SymuMasterExports.h"

#include <string>
#include <vector>


#pragma warning(push)
#pragma warning(disable : 4251)

namespace SymuCore {
    class Node;
    class Pattern;
}

namespace SymuMaster {

    class TraficSimulatorHandler;

    // abstract class that matches a simulator's interface for hybridation.
    // For SymuVia, the specialized class will match a punctual entry or exit.
    // For SimuRes, the specialized class will match a route.
    // Depending on their type, a downstream interface can have one or more corresponding upstream interfaces,
    // and an upstream interface can have one or more corresponding downstream interfaces :
    // A SimuRes route can have only one upstream interface, and one downstream interface,
    // A SymuVia entry can have multiple upstream routes,
    // A SymuVia exit can have multiple downstream routes.
    class SYMUMASTER_EXPORTS_DLL_DEF AbstractSimulatorInterface {

    public:

        AbstractSimulatorInterface();
        AbstractSimulatorInterface(TraficSimulatorHandler * pSimulator, const std::string & strName, bool bUpstream);

        TraficSimulatorHandler * GetSimulator() const;

        const std::string & getStrName() const;

        SymuCore::Node * GetNode() const;

        virtual bool Check(std::vector<AbstractSimulatorInterface*> & associatedInterfaces) const = 0;

        virtual void BuildUpstreamPenalties(SymuCore::Pattern * pPatternToVirtualNode) = 0;
        virtual void BuildDownstreamPenalties(SymuCore::Pattern * pPatternFromVirtualNode) = 0;

		virtual bool CanRestrictCapacity() const = 0;
		virtual bool CanComputeOffer() const = 0;

    protected:
        TraficSimulatorHandler* m_pSimulator;
        std::string             m_strName;
        bool                    m_bUpstream;
        SymuCore::Node*         m_pNode;
    };

    class SYMUMASTER_EXPORTS_DLL_DEF SimulatorsInterface {
    public:
        SimulatorsInterface();
        SimulatorsInterface(AbstractSimulatorInterface * pUpstreamInterface, AbstractSimulatorInterface * pDownstreamInterface);

        void SetUpstreamInterface(AbstractSimulatorInterface * pUpstreamInterface);
        void SetDownstreamInterface(AbstractSimulatorInterface * pDownstreamInterface);

        AbstractSimulatorInterface * GetUpstreamInterface() const;
        AbstractSimulatorInterface * GetDownstreamInterface() const;

		// tells if we have to do the the capacity/offer adaptation for the interface. For now,
		// only SymuVia's punctuel extremities and SimuRes's routes can handle this.
		bool HasToAdaptCapacityAndOffer() const;

    private:
        AbstractSimulatorInterface * m_pUpstreamInterface;
        AbstractSimulatorInterface * m_pDownstreamInterface;
    };

}

#pragma warning(pop)

#endif // _SYMUMASTER_SIMULATORS_INTERFACE_
