#pragma once

#ifndef SYMUCORE_IUSER_HANDLER_H
#define SYMUCORE_IUSER_HANDLER_H

#include "SymuCoreExports.h"

namespace SymuCore {

class SYMUCORE_DLL_DEF IUserHandler {

public:

    /**
     * Event occuring when a user completes a subpath (also called PPath).
     */
    virtual void OnPPathCompleted(int tripID, double dbNextPPathDepartureTimeFromcurrentTimeStep) = 0;

};
}

#endif // SYMUCORE_IUSER_HANDLER_H
