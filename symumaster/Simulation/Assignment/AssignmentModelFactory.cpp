#include "AssignmentModelFactory.h"

#include "Utils/SymuMasterConstants.h"
#include "Simulation/Assignment/MSADualLoop.h"
#include "Simulation/Assignment/HeuristicDualLoop.h"

#include <cassert>

using namespace SymuMaster;

AssignmentModel * AssignmentModelFactory::CreateAssignmentModel(const std::string & assignmentModelNodeName)
{
    AssignmentModel * pResult;
    EAssignmentModelName assignmentName = SymuMasterConstants::GetAssignmentModelNameFromXMLNodeName(assignmentModelNodeName);
    switch (assignmentName)
    {
    case AM_MSADualLoop:
        pResult = new MSADualLoop();
        break;
    case AM_HeuristicDualLoop:
        pResult = new HeuristicDualLoop();
        break;
    case AM_Unknown:
    default:
        pResult = NULL;
        assert(false);
        break;
    }
    
    return pResult;
}

