#pragma once

#ifndef _SYMUMASTER_ASSIGNMENTMODELFACTORY_
#define _SYMUMASTER_ASSIGNMENTMODELFACTORY_

#include "SymuMasterExports.h"

#include <string>

namespace SymuMaster {

    class AssignmentModel;

    class SYMUMASTER_EXPORTS_DLL_DEF AssignmentModelFactory {

    public:

        static AssignmentModel * CreateAssignmentModel(const std::string & assignmentModelNodeName);

    };

}

#endif // _SYMUMASTER_ASSIGNMENTMODELFACTORY_