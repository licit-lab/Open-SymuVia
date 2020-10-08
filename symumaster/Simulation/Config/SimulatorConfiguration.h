#pragma once

#ifndef _SYMUMASTER_SIMULATORCONFIGURATION_
#define _SYMUMASTER_SIMULATORCONFIGURATION_

#include "SymuMasterExports.h"
#include "Utils/SymuMasterConstants.h"

#include <string>

#include <xercesc/util/XercesVersion.hpp>

namespace XERCES_CPP_NAMESPACE {
    class DOMNode;
}

#pragma warning( push )  
#pragma warning( disable : 4251 )  

namespace SymuMaster {

    class SYMUMASTER_EXPORTS_DLL_DEF SimulatorConfiguration {

    public:
        SimulatorConfiguration();

        virtual ~SimulatorConfiguration();

        ESimulatorName GetSimulatorName() const;

        virtual bool LoadFromXML(XERCES_CPP_NAMESPACE::DOMNode * pSimulationConfigNode) = 0;

    protected:
        ESimulatorName m_eSimulatorName;
    };

}

#pragma warning( pop ) 

#endif // _SYMUMASTER_SIMULATORCONFIGURATION_
