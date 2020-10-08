#pragma once

#ifndef _SYMUMASTER_SYMUMASTERCONSTANTS_
#define _SYMUMASTER_SYMUMASTERCONSTANTS_

#include <Utils/SymuCoreConstants.h>

#include "SymuMasterExports.h"

#include <string>

#pragma warning( push )
#pragma warning( disable : 4251 )

namespace SymuMaster {

    enum ESimulatorName {
        ST_Unknown = 0,
        ST_SymuVia,
        ST_MFD
        // add here other simulator types...
    };

    enum EAssignmentModelName {
        AM_Unknown = 0,
        AM_MSADualLoop,
        AM_HeuristicDualLoop
        // add here other assignment models...
    };

    class SYMUMASTER_EXPORTS_DLL_DEF SymuMasterConstants {
    public:

        // Configuration XSD file name
        static const std::string s_SYMUMASTER_CONFIG_XSD_NAME;

        // Current configuration XSD file version
        static const std::string s_SYMUMASTER_CONFIG_XSD_VERSION;

        // Name of the xml node for the SymuVia simulator
        static const std::string s_SIMULATOR_SYMUVIA_NODE_NAME;

        // Name of the xml node for the MFD simulator
        static const std::string s_SIMULATOR_MFD_NODE_NAME;

        // Name of the xml node for the MSA dual loop assignment model
        static const std::string s_ASSIGNMENT_MODEL_MSA_DUAL_LOOP;

        // Name of the xml node for the Heuristic dual loop assignment model
        static const std::string s_ASSIGNMENT_MODEL_HEURISTIC_DUAL_LOOP;

        // types of available services i the XML configuration file
        static const std::string s_SERVICE_TYPE_NAME_DRIVING;
        static const std::string s_SERVICE_TYPE_NAME_PUBLIC_TRANSPORT;

        // Defines the associations between the ESimulatorName enum and the simulator names in the xml configuration file
        static ESimulatorName GetSimulatorNameFromXMLNodeName(const std::string & simulatorNodeName);

        // Defines the associations between the EAssignmentModelName enum and the assignment node names in the xml configuration file
        static EAssignmentModelName GetAssignmentModelNameFromXMLNodeName(const std::string & assignmentModelNodeName);

        // Defines the associations between the EServiceType enum and the service type names in the xml configuration file
        static SymuCore::ServiceType GetServiceTypeFromXMLTypeName(const std::string & serviceTypeName);


    };
}

#pragma warning( pop )

#endif // _SYMUMASTER_SYMUMASTERCONSTANTS_
