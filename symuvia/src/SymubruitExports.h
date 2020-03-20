#pragma once

#ifndef SYMUBRUITEXPORTS
#define SYMUBRUITEXPORTS

#ifdef WIN32
#ifdef SYMUVIA_EXPORTS
#define SYMUBRUIT_EXPORT __declspec(dllexport)
#define SYMUBRUIT_CDECL  __cdecl
#else
#define SYMUBRUIT_EXPORT __declspec(dllimport)
#define SYMUBRUIT_CDECL  __cdecl
#endif
#else
#define SYMUBRUIT_EXPORT __attribute__ ((visibility ("default")))
#define SYMUBRUIT_CDECL
#endif

#endif // SYMUBRUITEXPORTS
