#pragma once

#ifndef _SYMUMASTER_EXPORTS_
#define _SYMUMASTER_EXPORTS_

#ifdef WIN32
    #ifdef SYMUMASTER_EXPORTS
        #define SYMUMASTER_EXPORTS_DLL_DEF __declspec(dllexport)
    #else
        #define SYMUMASTER_EXPORTS_DLL_DEF __declspec(dllimport)
    #endif
#else
    #define SYMUMASTER_EXPORTS_DLL_DEF
#endif

#endif // _SYMUMASTER_EXPORTS_

