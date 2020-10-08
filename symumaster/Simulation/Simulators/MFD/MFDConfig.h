#pragma once

#ifndef _SYMUMASTER_MFDCONFIG_
#define _SYMUMASTER_MFDCONFIG_

#include "SymuMasterExports.h"

#include "Simulation/Config/SimulatorConfiguration.h"

#include <string>

#pragma warning( push )  
#pragma warning( disable : 4251 )  

namespace SymuMaster {

    class SYMUMASTER_EXPORTS_DLL_DEF MFDConfig : public SimulatorConfiguration {

    public:
        MFDConfig();

        virtual ~MFDConfig();

        virtual bool LoadFromXML(XERCES_CPP_NAMESPACE::DOMNode * pSimulationConfigNode);

        void SetMatlabWorkspacePath(const std::string & strMatlabWorkspacePath);
        const std::string & GetMatlabWorkspacePath() const;

        void SetDemandPeriodsNbPerPredictionPeriod(int nDemandPeriodsNbPerPredictionPeriod);
        int GetDemandPeriodsNbPerPredictionPeriod() const;

    protected:

        /**
         * Matlab MFD workspace folder
         */
        std::string m_strMatlabWorkspacePath;

        /**
         * Discretization number for the demand over the prediction period
         */
        int m_nDemandPeriodsNbPerPredictionPeriod;
    };

}

#pragma warning( pop )

#endif // _SYMUMASTER_MFDCONFIG_
