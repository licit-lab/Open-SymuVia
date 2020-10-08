#include "SymuMasterConstants.h"

#include <Utils/SymuCoreConstants.h>

#include <cassert>

using namespace SymuMaster;

const std::string SymuMasterConstants::s_SYMUMASTER_CONFIG_XSD_NAME = "SymuMasterConfig.xsd";

const std::string SymuMasterConstants::s_SYMUMASTER_CONFIG_XSD_VERSION = "1.00";

const std::string SymuMasterConstants::s_SIMULATOR_SYMUVIA_NODE_NAME = "SYMUVIA";
const std::string SymuMasterConstants::s_SIMULATOR_MFD_NODE_NAME = "MFD";

const std::string SymuMasterConstants::s_ASSIGNMENT_MODEL_MSA_DUAL_LOOP = "MSA_DUAL_LOOP";
const std::string SymuMasterConstants::s_ASSIGNMENT_MODEL_HEURISTIC_DUAL_LOOP = "HEURISTIC_DUAL_LOOP";

const std::string SymuMasterConstants::s_SERVICE_TYPE_NAME_DRIVING = "driving";
const std::string SymuMasterConstants::s_SERVICE_TYPE_NAME_PUBLIC_TRANSPORT = "public_transport";

ESimulatorName SymuMasterConstants::GetSimulatorNameFromXMLNodeName(const std::string & simulatorNodeName)
{
    if (simulatorNodeName == s_SIMULATOR_SYMUVIA_NODE_NAME)
    {
        return ST_SymuVia;
    }
    else if (simulatorNodeName == s_SIMULATOR_MFD_NODE_NAME)
    {
        return ST_MFD;
    }

    assert(false); // implement the other simulators

    return ST_Unknown;
}

EAssignmentModelName SymuMasterConstants::GetAssignmentModelNameFromXMLNodeName(const std::string & assignmentModelNodeName)
{
    if (assignmentModelNodeName == s_ASSIGNMENT_MODEL_MSA_DUAL_LOOP)
    {
        return AM_MSADualLoop;
    }
    else if(assignmentModelNodeName == s_ASSIGNMENT_MODEL_HEURISTIC_DUAL_LOOP)
    {
        return AM_HeuristicDualLoop;
    }

    assert(false); // implement the other assignment model

    return AM_Unknown;
}

SymuCore::ServiceType SymuMasterConstants::GetServiceTypeFromXMLTypeName(const std::string & serviceTypeName)
{
    if (serviceTypeName == s_SERVICE_TYPE_NAME_DRIVING)
    {
        return SymuCore::ST_Driving;
    }
    else if (serviceTypeName == s_SERVICE_TYPE_NAME_PUBLIC_TRANSPORT)
    {
        return SymuCore::ST_PublicTransport;
    }

    assert(false); // should not occur before adding other types in the XSD available values for available service types

    return SymuCore::ST_Undefined;
}