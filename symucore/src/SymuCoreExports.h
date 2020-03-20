#pragma once

#ifndef SYMUCORE_EXPORTS_H
#define SYMUCORE_EXPORTS_H

#ifdef WIN32
    #ifdef SYMUCORE_EXPORTS
        #define SYMUCORE_DLL_DEF __declspec(dllexport)
    #else
        #define SYMUCORE_DLL_DEF __declspec(dllimport)
    #endif
#else
    #define SYMUCORE_DLL_DEF
#endif

#endif // SYMUCORE_EXPORTS_H

