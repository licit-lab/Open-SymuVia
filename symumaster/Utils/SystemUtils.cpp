#include "SystemUtils.h"

#include <boost/filesystem/convenience.hpp>
#include <boost/log/trivial.hpp>

using namespace SymuMaster;

#ifdef WIN32
HMODULE SystemUtils::m_hInstance = NULL;
#endif

#ifndef WIN32
#include <dlfcn.h>
#endif

#ifdef WIN32
#ifdef _MANAGED
#pragma managed(push, off)
#endif


bool APIENTRY DllMain(HMODULE hModule,
    DWORD  ul_reason_for_call,
    LPVOID lpReserved
    )
{
    switch (ul_reason_for_call)
    {
    case DLL_PROCESS_ATTACH:
        SystemUtils::m_hInstance = hModule;
        break;
    case DLL_PROCESS_DETACH:
        break;
    default:
        break;
    }
    return TRUE;
}

#ifdef _MANAGED
#pragma managed(pop)
#endif
#endif

std::string SystemUtils::GetCurrentModuleFolder()
{
    std::string modulePath;
#ifdef WIN32
    char lpBuff[512];
    DWORD size;

    size = ::GetModuleFileName(m_hInstance, lpBuff, sizeof(lpBuff) / sizeof(lpBuff[0]));

    if (size >= 0)
    {
        lpBuff[size] = 0;
    }

    modulePath = lpBuff;
    return boost::filesystem::path(modulePath).remove_filename().string();
#else
    std::string strPath = "";
    Dl_info info;
    if (dladdr((const void *)(&SystemUtils::GetCurrentModuleFolder), &info))
    {
        strPath = info.dli_fname;
    }
    else
    {
        BOOST_LOG_TRIVIAL(error) << "Error in GetCurrentModuleFolder...";
    }
    modulePath = strPath;
    // doing absolute here because dladdr doesn't always gives an absolute path...
    return boost::filesystem::absolute(modulePath).remove_filename().string();
#endif
}