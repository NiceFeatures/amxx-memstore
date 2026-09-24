// vim: set ts=4 sw=4 tw=99 noet:
//
// AMX Mod X MemStore Module
// In-Memory Transient Cross-Map Storage for AMX Mod X
//
#ifndef __MODULECONFIG_H__
#define __MODULECONFIG_H__

#include <amxmodx_version.h>

// Module info
#define MODULE_NAME "MemStore"
#define MODULE_VERSION "1.1.0"
#define MODULE_AUTHOR "iceeedR"
#define MODULE_URL "https://github.com/NiceFeatures/amxx-memstore"
#define MODULE_LOGTAG "MEMSTORE"
#define MODULE_LIBRARY "memstore"
#define MODULE_LIBCLASS ""

// CRITICAL: Comment out MODULE_RELOAD_ON_MAPCHANGE to ensure Metamod/AMXX keeps
// the module DLL/SO continuously resident in RAM across map changes!
// #define MODULE_RELOAD_ON_MAPCHANGE

#ifdef __DATE__
#define MODULE_DATE __DATE__
#else
#define MODULE_DATE "Unknown"
#endif

// AMXX Init and lifecycle callbacks
#define FN_AMXX_ATTACH OnAmxxAttach
#define FN_AMXX_DETACH OnAmxxDetach
#define FN_AMXX_PLUGINSUNLOADED OnPluginsUnloaded
#define FN_AMXX_PLUGINSLOADED OnPluginsLoaded

#endif // __MODULECONFIG_H__
