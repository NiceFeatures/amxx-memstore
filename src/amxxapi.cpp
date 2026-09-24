// vim: set ts=4 sw=4 tw=99 noet:
//
// AMX Mod X MemStore Module
// In-Memory Transient Cross-Map Storage for AMX Mod X
//
#include "MemStore.h"

extern AMX_NATIVE_INFO MemStore_natives[];

void OnAmxxAttach()
{
	MF_AddNatives(MemStore_natives);
	MF_Log("[%s] v%s initialized successfully. In-RAM Cross-Map storage engine active.", MODULE_LOGTAG, MODULE_VERSION);
}

void OnPluginsLoaded()
{
	// Called after all plugins in plugins.ini are loaded for the map
}

void OnPluginsUnloaded()
{
	// Hook executed during map transitions (ServerDeactivate / Plugins Unload)
	// Sweeps entries marked as MapEnd, decrements MapCount, and checks TTL.
	// Preserves all EXP_PERSISTENT and remaining MapCount entries directly in RAM!
	g_MemStore.SweepMapEnd();
	MF_Log("[%s] Map transition sweep executed: persistent in-RAM entries retained.", MODULE_LOGTAG);
}

void OnAmxxDetach()
{
	// Clean up all memory allocations when module is completely detached/server shutdown
	g_MemStore.ClearAll();
	MF_Log("[%s] Module detached. All in-memory allocations safely cleared.", MODULE_LOGTAG);
}
