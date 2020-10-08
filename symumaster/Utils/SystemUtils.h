#pragma once

#ifndef _SYMUMASTER_SYSTEMUTILS_
#define _SYMUMASTER_SYSTEMUTILS_

#include "SymuMasterExports.h"

#include <string>

#ifdef WIN32
#include <Windows.h>
#endif

#pragma warning( push )
#pragma warning( disable : 4251 )

namespace SymuMaster {

    class SYMUMASTER_EXPORTS_DLL_DEF SystemUtils {
    public:
        static std::string GetCurrentModuleFolder();

#ifdef WIN32
    static HMODULE m_hInstance;
#endif
    };
}

#pragma warning( pop )

#endif // _SYMUMASTER_SYSTEMUTILS_
