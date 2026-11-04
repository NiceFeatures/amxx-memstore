// vim: set ts=4 sw=4 tw=99 noet:
//
// AMX Mod X MemStore Module
// In-Memory Transient Cross-Map Storage for AMX Mod X
//
// Author: iceeedR
//
#include "MemStore.h"
#include <new>

// Helper to validate and extract namespace and key
static bool GetNamespaceAndKey(AMX *amx, cell paramNs, cell paramKey, std::string &nsOut, std::string &keyOut)
{
	int len = 0;
	char *nsStr = MF_GetAmxString(amx, paramNs, 0, &len);
	if (!nsStr || len == 0)
	{
		MF_LogError(amx, AMX_ERR_NATIVE, "[%s] Namespace name cannot be empty!", MODULE_LOGTAG);
		return false;
	}

	char *keyStr = MF_GetAmxString(amx, paramKey, 1, &len);
	if (!keyStr || len == 0)
	{
		MF_LogError(amx, AMX_ERR_NATIVE, "[%s] Key name cannot be empty!", MODULE_LOGTAG);
		return false;
	}

	nsOut = nsStr;
	keyOut = keyStr;
	return true;
}

static bool IsValidExpirePolicy(cell value)
{
	return (value >= 0 && value <= 3);
}

static bool IsValidRankOrder(cell value)
{
	return (value >= 0 && value <= 1);
}

// native bool:mem_set_int(const namespace[], const key[], value, ExpirePolicy:policy = EXP_PERSISTENT, extra = 0);
static cell AMX_NATIVE_CALL amxx_mem_set_int(AMX *amx, cell *params)
{
	std::string ns, key;
	if (!GetNamespaceAndKey(amx, params[1], params[2], ns, key))
	{
		return 0;
	}

	cell val = params[3];
	unsigned int numParams = (*params) / sizeof(cell);

	ExpirePolicy policy = ExpirePolicy::Persistent;
	if (numParams >= 4)
	{
		if (!IsValidExpirePolicy(params[4]))
		{
			MF_LogError(amx, AMX_ERR_NATIVE, "[%s] Invalid ExpirePolicy value: %d", MODULE_LOGTAG, params[4]);
			return 0;
		}
		policy = static_cast<ExpirePolicy>(params[4]);
	}
	int32_t extra = (numParams >= 5) ? static_cast<int32_t>(params[5]) : 0;

	return g_MemStore.SetInt(ns, key, val, policy, extra) ? 1 : 0;
}

// native bool:mem_get_int(const namespace[], const key[], &value, default_val = 0);
static cell AMX_NATIVE_CALL amxx_mem_get_int(AMX *amx, cell *params)
{
	std::string ns, key;
	if (!GetNamespaceAndKey(amx, params[1], params[2], ns, key))
	{
		return 0;
	}

	cell *valAddr = MF_GetAmxAddr(amx, params[3]);
	unsigned int numParams = (*params) / sizeof(cell);
	cell defaultVal = (numParams >= 4) ? params[4] : 0;

	bool typeMismatch = false;
	bool found = false;
	cell outVal = 0;

	bool success = g_MemStore.GetInt(ns, key, outVal, defaultVal, typeMismatch, found);
	if (typeMismatch)
	{
		MF_LogError(amx, AMX_ERR_NATIVE, "[%s] Type mismatch! Key '%s' in namespace '%s' is not of type Int (actual: %s).",
			MODULE_LOGTAG, key.c_str(), ns.c_str(), GetTypeName(g_MemStore.GetKeyType(ns, key)));
		if (valAddr)
		{
			*valAddr = defaultVal;
		}
		return 0;
	}

	if (valAddr)
	{
		*valAddr = outVal;
	}

	return (success && found) ? 1 : 0;
}

// native bool:mem_set_float(const namespace[], const key[], Float:value, ExpirePolicy:policy = EXP_PERSISTENT, extra = 0);
static cell AMX_NATIVE_CALL amxx_mem_set_float(AMX *amx, cell *params)
{
	std::string ns, key;
	if (!GetNamespaceAndKey(amx, params[1], params[2], ns, key))
	{
		return 0;
	}

	float val = amx_ctof(params[3]);
	unsigned int numParams = (*params) / sizeof(cell);

	ExpirePolicy policy = ExpirePolicy::Persistent;
	if (numParams >= 4)
	{
		if (!IsValidExpirePolicy(params[4]))
		{
			MF_LogError(amx, AMX_ERR_NATIVE, "[%s] Invalid ExpirePolicy value: %d", MODULE_LOGTAG, params[4]);
			return 0;
		}
		policy = static_cast<ExpirePolicy>(params[4]);
	}
	int32_t extra = (numParams >= 5) ? static_cast<int32_t>(params[5]) : 0;

	return g_MemStore.SetFloat(ns, key, val, policy, extra) ? 1 : 0;
}

// native bool:mem_get_float(const namespace[], const key[], &Float:value, Float:default_val = 0.0);
static cell AMX_NATIVE_CALL amxx_mem_get_float(AMX *amx, cell *params)
{
	std::string ns, key;
	if (!GetNamespaceAndKey(amx, params[1], params[2], ns, key))
	{
		return 0;
	}

	cell *valAddr = MF_GetAmxAddr(amx, params[3]);
	unsigned int numParams = (*params) / sizeof(cell);
	float defaultVal = (numParams >= 4) ? amx_ctof(params[4]) : 0.0f;

	bool typeMismatch = false;
	bool found = false;
	float outVal = 0.0f;

	bool success = g_MemStore.GetFloat(ns, key, outVal, defaultVal, typeMismatch, found);
	if (typeMismatch)
	{
		MF_LogError(amx, AMX_ERR_NATIVE, "[%s] Type mismatch! Key '%s' in namespace '%s' is not of type Float (actual: %s).",
			MODULE_LOGTAG, key.c_str(), ns.c_str(), GetTypeName(g_MemStore.GetKeyType(ns, key)));
		if (valAddr)
		{
			*valAddr = amx_ftoc((REAL)defaultVal);
		}
		return 0;
	}

	if (valAddr)
	{
		*valAddr = amx_ftoc((REAL)outVal);
	}

	return (success && found) ? 1 : 0;
}

// native bool:mem_set_string(const namespace[], const key[], const value[], ExpirePolicy:policy = EXP_PERSISTENT, extra = 0);
static cell AMX_NATIVE_CALL amxx_mem_set_string(AMX *amx, cell *params)
{
	std::string ns, key;
	if (!GetNamespaceAndKey(amx, params[1], params[2], ns, key))
	{
		return 0;
	}

	int len = 0;
	char *valStr = MF_GetAmxString(amx, params[3], 2, &len);
	std::string val = (valStr != nullptr) ? valStr : "";

	unsigned int numParams = (*params) / sizeof(cell);
	ExpirePolicy policy = ExpirePolicy::Persistent;
	if (numParams >= 4)
	{
		if (!IsValidExpirePolicy(params[4]))
		{
			MF_LogError(amx, AMX_ERR_NATIVE, "[%s] Invalid ExpirePolicy value: %d", MODULE_LOGTAG, params[4]);
			return 0;
		}
		policy = static_cast<ExpirePolicy>(params[4]);
	}
	int32_t extra = (numParams >= 5) ? static_cast<int32_t>(params[5]) : 0;

	return g_MemStore.SetString(ns, key, val, policy, extra) ? 1 : 0;
}

// native bool:mem_get_string(const namespace[], const key[], dest[], maxlen, const default_val[] = "");
static cell AMX_NATIVE_CALL amxx_mem_get_string(AMX *amx, cell *params)
{
	std::string ns, key;
	if (!GetNamespaceAndKey(amx, params[1], params[2], ns, key))
	{
		return 0;
	}

	int maxlen = static_cast<int>(params[4]);
	if (maxlen <= 0)
	{
		MF_LogError(amx, AMX_ERR_NATIVE, "[%s] String buffer maxlen must be greater than 0!", MODULE_LOGTAG);
		return 0;
	}

	unsigned int numParams = (*params) / sizeof(cell);
	std::string def = "";
	if (numParams >= 5 && params[5] != 0)
	{
		int defaultLen = 0;
		char *defaultStr = MF_GetAmxString(amx, params[5], 2, &defaultLen);
		if (defaultStr != nullptr)
		{
			def = defaultStr;
		}
	}

	bool typeMismatch = false;
	bool found = false;
	std::string outVal;

	bool success = g_MemStore.GetString(ns, key, outVal, def, typeMismatch, found);
	if (typeMismatch)
	{
		MF_LogError(amx, AMX_ERR_NATIVE, "[%s] Type mismatch! Key '%s' in namespace '%s' is not of type String (actual: %s).",
			MODULE_LOGTAG, key.c_str(), ns.c_str(), GetTypeName(g_MemStore.GetKeyType(ns, key)));
		MF_SetAmxString(amx, params[3], def.c_str(), maxlen);
		return 0;
	}

	MF_SetAmxString(amx, params[3], outVal.c_str(), maxlen);
	return (success && found) ? 1 : 0;
}

// native bool:mem_set_array(const namespace[], const key[], const any:values[], size, ExpirePolicy:policy = EXP_PERSISTENT, extra = 0);
static cell AMX_NATIVE_CALL amxx_mem_set_array(AMX *amx, cell *params)
{
	std::string ns, key;
	if (!GetNamespaceAndKey(amx, params[1], params[2], ns, key))
	{
		return 0;
	}

	cell size = params[4];
	if (size <= 0)
	{
		MF_LogError(amx, AMX_ERR_NATIVE, "[%s] Array size must be greater than 0!", MODULE_LOGTAG);
		return 0;
	}

	cell *arrPtr = MF_GetAmxAddr(amx, params[3]);
	if (!arrPtr)
	{
		MF_LogError(amx, AMX_ERR_NATIVE, "[%s] Invalid array pointer!", MODULE_LOGTAG);
		return 0;
	}

	unsigned int numParams = (*params) / sizeof(cell);
	ExpirePolicy policy = ExpirePolicy::Persistent;
	if (numParams >= 5)
	{
		if (!IsValidExpirePolicy(params[5]))
		{
			MF_LogError(amx, AMX_ERR_NATIVE, "[%s] Invalid ExpirePolicy value: %d", MODULE_LOGTAG, params[5]);
			return 0;
		}
		policy = static_cast<ExpirePolicy>(params[5]);
	}
	int32_t extra = (numParams >= 6) ? static_cast<int32_t>(params[6]) : 0;

	return g_MemStore.SetArray(ns, key, arrPtr, static_cast<size_t>(size), policy, extra) ? 1 : 0;
}

// native bool:mem_get_array(const namespace[], const key[], any:dest[], maxlen, &copied_size = 0);
static cell AMX_NATIVE_CALL amxx_mem_get_array(AMX *amx, cell *params)
{
	std::string ns, key;
	if (!GetNamespaceAndKey(amx, params[1], params[2], ns, key))
	{
		return 0;
	}

	cell maxlen = params[4];
	if (maxlen <= 0)
	{
		MF_LogError(amx, AMX_ERR_NATIVE, "[%s] Array destination maxlen must be greater than 0!", MODULE_LOGTAG);
		return 0;
	}

	cell *destPtr = MF_GetAmxAddr(amx, params[3]);
	if (!destPtr)
	{
		MF_LogError(amx, AMX_ERR_NATIVE, "[%s] Invalid destination array pointer!", MODULE_LOGTAG);
		return 0;
	}

	// TASK 1: Check params[5] != 0 so omitting default parameter does NOT corrupt address 0 of plugin memory!
	cell *copiedSizeAddr = nullptr;
	unsigned int numParams = (*params) / sizeof(cell);
	if (numParams >= 5 && params[5] != 0)
	{
		copiedSizeAddr = MF_GetAmxAddr(amx, params[5]);
	}

	bool typeMismatch = false;
	bool found = false;
	size_t copied = 0;

	bool success = g_MemStore.GetArray(ns, key, destPtr, static_cast<size_t>(maxlen), copied, typeMismatch, found);
	if (copiedSizeAddr)
	{
		*copiedSizeAddr = static_cast<cell>(copied);
	}

	if (typeMismatch)
	{
		MF_LogError(amx, AMX_ERR_NATIVE, "[%s] Type mismatch! Key '%s' in namespace '%s' is not of type Array (actual: %s).",
			MODULE_LOGTAG, key.c_str(), ns.c_str(), GetTypeName(g_MemStore.GetKeyType(ns, key)));
		return 0;
	}

	return (success && found) ? 1 : 0;
}

// native mem_get_array_size(const namespace[], const key[]);
static cell AMX_NATIVE_CALL amxx_mem_get_array_size(AMX *amx, cell *params)
{
	std::string ns, key;
	if (!GetNamespaceAndKey(amx, params[1], params[2], ns, key))
	{
		return 0;
	}

	return static_cast<cell>(g_MemStore.GetArraySize(ns, key));
}

// native EntryType:mem_get_key_type(const namespace[], const key[]);
static cell AMX_NATIVE_CALL amxx_mem_get_key_type(AMX *amx, cell *params)
{
	std::string ns, key;
	if (!GetNamespaceAndKey(amx, params[1], params[2], ns, key))
	{
		return static_cast<cell>(EntryType::None);
	}

	return static_cast<cell>(g_MemStore.GetKeyType(ns, key));
}

// native bool:mem_key_exists(const namespace[], const key[]);
static cell AMX_NATIVE_CALL amxx_mem_key_exists(AMX *amx, cell *params)
{
	std::string ns, key;
	if (!GetNamespaceAndKey(amx, params[1], params[2], ns, key))
	{
		return 0;
	}

	return g_MemStore.KeyExists(ns, key) ? 1 : 0;
}

// native bool:mem_delete_key(const namespace[], const key[]);
static cell AMX_NATIVE_CALL amxx_mem_delete_key(AMX *amx, cell *params)
{
	std::string ns, key;
	if (!GetNamespaceAndKey(amx, params[1], params[2], ns, key))
	{
		return 0;
	}

	return g_MemStore.DeleteKey(ns, key) ? 1 : 0;
}

// native bool:mem_clear_namespace(const namespace[]);
static cell AMX_NATIVE_CALL amxx_mem_clear_namespace(AMX *amx, cell *params)
{
	int len = 0;
	char *nsStr = MF_GetAmxString(amx, params[1], 0, &len);
	if (!nsStr || len == 0)
	{
		MF_LogError(amx, AMX_ERR_NATIVE, "[%s] Namespace name cannot be empty!", MODULE_LOGTAG);
		return 0;
	}

	return g_MemStore.ClearNamespace(nsStr) ? 1 : 0;
}

// native bool:mem_set_expire(const namespace[], const key[], ExpirePolicy:policy, extra = 0);
static cell AMX_NATIVE_CALL amxx_mem_set_expire(AMX *amx, cell *params)
{
	std::string ns, key;
	if (!GetNamespaceAndKey(amx, params[1], params[2], ns, key))
	{
		return 0;
	}

	if (!IsValidExpirePolicy(params[3]))
	{
		MF_LogError(amx, AMX_ERR_NATIVE, "[%s] Invalid ExpirePolicy value: %d", MODULE_LOGTAG, params[3]);
		return 0;
	}
	ExpirePolicy policy = static_cast<ExpirePolicy>(params[3]);
	unsigned int numParams = (*params) / sizeof(cell);
	int32_t extra = (numParams >= 4) ? static_cast<int32_t>(params[4]) : 0;

	return g_MemStore.SetExpire(ns, key, policy, extra) ? 1 : 0;
}

// native mem_get_namespace_count(const namespace[]);
static cell AMX_NATIVE_CALL amxx_mem_get_namespace_count(AMX *amx, cell *params)
{
	int len = 0;
	char *nsStr = MF_GetAmxString(amx, params[1], 0, &len);
	if (!nsStr || len == 0)
	{
		MF_LogError(amx, AMX_ERR_NATIVE, "[%s] Namespace name cannot be empty!", MODULE_LOGTAG);
		return 0;
	}

	return static_cast<cell>(g_MemStore.GetNamespaceCount(nsStr));
}

// native bool:mem_get_key_at(const namespace[], index, dest[], maxlen);
static cell AMX_NATIVE_CALL amxx_mem_get_key_at(AMX *amx, cell *params)
{
	int len = 0;
	char *nsStr = MF_GetAmxString(amx, params[1], 0, &len);
	if (!nsStr || len == 0)
	{
		MF_LogError(amx, AMX_ERR_NATIVE, "[%s] Namespace name cannot be empty!", MODULE_LOGTAG);
		return 0;
	}

	cell index = params[2];
	if (index < 0)
	{
		return 0;
	}

	cell maxlen = params[4];
	if (maxlen <= 0)
	{
		return 0;
	}

	std::string keyOut;
	if (!g_MemStore.GetKeyAt(nsStr, static_cast<size_t>(index), keyOut))
	{
		return 0;
	}

	MF_SetAmxString(amx, params[3], keyOut.c_str(), maxlen);
	return 1;
}

// native mem_iterate_keys(const namespace[], const callback_function[], any:data = 0);
static cell AMX_NATIVE_CALL amxx_mem_iterate_keys(AMX *amx, cell *params)
{
	int len = 0;
	char *nsStr = MF_GetAmxString(amx, params[1], 0, &len);
	if (!nsStr || len == 0)
	{
		MF_LogError(amx, AMX_ERR_NATIVE, "[%s] Namespace name cannot be empty!", MODULE_LOGTAG);
		return 0;
	}

	char *funcName = MF_GetAmxString(amx, params[2], 1, &len);
	if (!funcName || len == 0)
	{
		MF_LogError(amx, AMX_ERR_NATIVE, "[%s] Callback function name cannot be empty!", MODULE_LOGTAG);
		return 0;
	}

	cell extraData = 0;
	unsigned int numParams = (*params) / sizeof(cell);
	if (numParams >= 3)
	{
		extraData = params[3];
	}

	int forwardId = MF_RegisterSPForwardByName(amx, funcName, FP_STRING, FP_CELL, FP_DONE);
	if (forwardId <= 0)
	{
		MF_LogError(amx, AMX_ERR_NATIVE, "[%s] Callback function '%s' not found in plugin!", MODULE_LOGTAG, funcName);
		return 0;
	}

	std::vector<std::string> keys = g_MemStore.GetActiveKeys(nsStr);
	cell count = 0;

	for (const auto &k : keys)
	{
		cell ret = MF_ExecuteForward(forwardId, k.c_str(), extraData);
		++count;
		// PLUGIN_HANDLED (1) aborts iteration early
		if (ret == 1)
		{
			break;
		}
	}

	MF_UnregisterSPForward(forwardId);
	return count;
}

// native bool:mem_rank_set(const namespace[], const key[], score, ExpirePolicy:policy = EXP_PERSISTENT, extra = 0);
static cell AMX_NATIVE_CALL amxx_mem_rank_set(AMX *amx, cell *params)
{
	std::string ns, key;
	if (!GetNamespaceAndKey(amx, params[1], params[2], ns, key))
	{
		return 0;
	}

	cell score = params[3];
	unsigned int numParams = (*params) / sizeof(cell);

	ExpirePolicy policy = ExpirePolicy::Persistent;
	if (numParams >= 4)
	{
		if (!IsValidExpirePolicy(params[4]))
		{
			MF_LogError(amx, AMX_ERR_NATIVE, "[%s] Invalid ExpirePolicy value: %d", MODULE_LOGTAG, params[4]);
			return 0;
		}
		policy = static_cast<ExpirePolicy>(params[4]);
	}
	int32_t extra = (numParams >= 5) ? static_cast<int32_t>(params[5]) : 0;

	return g_MemStore.RankSet(ns, key, score, policy, extra) ? 1 : 0;
}

// native bool:mem_rank_get_score(const namespace[], const key[], &score);
static cell AMX_NATIVE_CALL amxx_mem_rank_get_score(AMX *amx, cell *params)
{
	std::string ns, key;
	if (!GetNamespaceAndKey(amx, params[1], params[2], ns, key))
	{
		return 0;
	}

	cell *scoreAddr = MF_GetAmxAddr(amx, params[3]);
	cell scoreOut = 0;

	if (!g_MemStore.RankGetScore(ns, key, scoreOut))
	{
		return 0;
	}

	if (scoreAddr)
	{
		*scoreAddr = scoreOut;
	}

	return 1;
}

// native mem_rank_get_pos(const namespace[], const key[], RankOrder:order = RANK_DESC);
static cell AMX_NATIVE_CALL amxx_mem_rank_get_pos(AMX *amx, cell *params)
{
	std::string ns, key;
	if (!GetNamespaceAndKey(amx, params[1], params[2], ns, key))
	{
		return 0;
	}

	unsigned int numParams = (*params) / sizeof(cell);
	RankOrder order = RankOrder::Desc;
	if (numParams >= 3)
	{
		if (!IsValidRankOrder(params[3]))
		{
			MF_LogError(amx, AMX_ERR_NATIVE, "[%s] Invalid RankOrder value: %d", MODULE_LOGTAG, params[3]);
			return 0;
		}
		order = static_cast<RankOrder>(params[3]);
	}

	return static_cast<cell>(g_MemStore.RankGetPosition(ns, key, order));
}

// native bool:mem_rank_get_top(const namespace[], rank_pos, key_dest[], maxlen, &score = 0, RankOrder:order = RANK_DESC);
static cell AMX_NATIVE_CALL amxx_mem_rank_get_top(AMX *amx, cell *params)
{
	int len = 0;
	char *nsStr = MF_GetAmxString(amx, params[1], 0, &len);
	if (!nsStr || len == 0)
	{
		MF_LogError(amx, AMX_ERR_NATIVE, "[%s] Namespace name cannot be empty!", MODULE_LOGTAG);
		return 0;
	}

	cell rankPos = params[2];
	cell maxlen = params[4];
	if (maxlen <= 0 || rankPos < 1)
	{
		return 0;
	}

	// TASK 1: Check params[5] != 0 so omitting default parameter does NOT corrupt address 0 of plugin memory!
	cell *scoreAddr = nullptr;
	unsigned int numParams = (*params) / sizeof(cell);
	if (numParams >= 5 && params[5] != 0)
	{
		scoreAddr = MF_GetAmxAddr(amx, params[5]);
	}

	RankOrder order = RankOrder::Desc;
	if (numParams >= 6)
	{
		if (!IsValidRankOrder(params[6]))
		{
			MF_LogError(amx, AMX_ERR_NATIVE, "[%s] Invalid RankOrder value: %d", MODULE_LOGTAG, params[6]);
			return 0;
		}
		order = static_cast<RankOrder>(params[6]);
	}

	std::string keyOut;
	cell scoreOut = 0;

	if (!g_MemStore.RankGetTop(nsStr, rankPos, keyOut, scoreOut, order))
	{
		return 0;
	}

	MF_SetAmxString(amx, params[3], keyOut.c_str(), maxlen);
	if (scoreAddr)
	{
		*scoreAddr = scoreOut;
	}

	return 1;
}

// native mem_rank_get_count(const namespace[]);
static cell AMX_NATIVE_CALL amxx_mem_rank_get_count(AMX *amx, cell *params)
{
	int len = 0;
	char *nsStr = MF_GetAmxString(amx, params[1], 0, &len);
	if (!nsStr || len == 0)
	{
		MF_LogError(amx, AMX_ERR_NATIVE, "[%s] Namespace name cannot be empty!", MODULE_LOGTAG);
		return 0;
	}

	return static_cast<cell>(g_MemStore.RankGetCount(nsStr));
}

// native bool:mem_rank_delete(const namespace[], const key[]);
static cell AMX_NATIVE_CALL amxx_mem_rank_delete(AMX *amx, cell *params)
{
	std::string ns, key;
	if (!GetNamespaceAndKey(amx, params[1], params[2], ns, key))
	{
		return 0;
	}

	return g_MemStore.RankDelete(ns, key) ? 1 : 0;
}

// native bool:mem_rank_clear(const namespace[]);
static cell AMX_NATIVE_CALL amxx_mem_rank_clear(AMX *amx, cell *params)
{
	int len = 0;
	char *nsStr = MF_GetAmxString(amx, params[1], 0, &len);
	if (!nsStr || len == 0)
	{
		MF_LogError(amx, AMX_ERR_NATIVE, "[%s] Namespace name cannot be empty!", MODULE_LOGTAG);
		return 0;
	}

	return g_MemStore.RankClear(nsStr) ? 1 : 0;
}

// native mem_set_limits(max_keys_per_ns = 5000, max_array_size = 4096, max_string_len = 4096, max_namespaces = 200, max_memory_mb = 64);
static cell AMX_NATIVE_CALL amxx_mem_set_limits(AMX *amx, cell *params)
{
	cell maxKeys = params[1];
	cell maxArray = params[2];
	cell maxStr = params[3];

	size_t maxNs = 0;
	size_t maxMemMb = 0;
	unsigned int numParams = (*params) / sizeof(cell);

	if (numParams >= 4)
	{
		cell maxNsParam = params[4];
		maxNs = (maxNsParam > 0) ? static_cast<size_t>(maxNsParam) : 0;
	}
	if (numParams >= 5)
	{
		cell maxMemParam = params[5];
		maxMemMb = (maxMemParam > 0) ? static_cast<size_t>(maxMemParam) : 0;
	}

	g_MemStore.SetLimits(
		(maxKeys > 0) ? static_cast<size_t>(maxKeys) : 5000,
		(maxArray > 0) ? static_cast<size_t>(maxArray) : 4096,
		(maxStr > 0) ? static_cast<size_t>(maxStr) : 4096,
		maxNs,
		maxMemMb
	);

	return 1;
}

// native bool:mem_inc_int(const namespace[], const key[], delta = 1, &new_value = 0, ExpirePolicy:policy = EXP_PERSISTENT, extra = 0);
static cell AMX_NATIVE_CALL amxx_mem_inc_int(AMX *amx, cell *params)
{
	std::string ns, key;
	if (!GetNamespaceAndKey(amx, params[1], params[2], ns, key))
	{
		return 0;
	}

	unsigned int numParams = (*params) / sizeof(cell);
	cell delta = (numParams >= 3) ? params[3] : 1;

	cell *newValAddr = nullptr;
	if (numParams >= 4 && params[4] != 0)
	{
		newValAddr = MF_GetAmxAddr(amx, params[4]);
	}

	ExpirePolicy policy = ExpirePolicy::Persistent;
	if (numParams >= 5)
	{
		if (!IsValidExpirePolicy(params[5]))
		{
			MF_LogError(amx, AMX_ERR_NATIVE, "[%s] Invalid ExpirePolicy value: %d", MODULE_LOGTAG, params[5]);
			return 0;
		}
		policy = static_cast<ExpirePolicy>(params[5]);
	}
	int32_t extra = (numParams >= 6) ? static_cast<int32_t>(params[6]) : 0;

	cell outVal = 0;
	bool success = g_MemStore.IncInt(ns, key, delta, outVal, policy, extra);
	if (newValAddr)
	{
		*newValAddr = outVal;
	}

	return success ? 1 : 0;
}

// native bool:mem_inc_float(const namespace[], const key[], Float:delta = 1.0, &Float:new_value = 0.0, ExpirePolicy:policy = EXP_PERSISTENT, extra = 0);
static cell AMX_NATIVE_CALL amxx_mem_inc_float(AMX *amx, cell *params)
{
	std::string ns, key;
	if (!GetNamespaceAndKey(amx, params[1], params[2], ns, key))
	{
		return 0;
	}

	unsigned int numParams = (*params) / sizeof(cell);
	float delta = (numParams >= 3) ? amx_ctof(params[3]) : 1.0f;

	cell *newValAddr = nullptr;
	if (numParams >= 4 && params[4] != 0)
	{
		newValAddr = MF_GetAmxAddr(amx, params[4]);
	}

	ExpirePolicy policy = ExpirePolicy::Persistent;
	if (numParams >= 5)
	{
		if (!IsValidExpirePolicy(params[5]))
		{
			MF_LogError(amx, AMX_ERR_NATIVE, "[%s] Invalid ExpirePolicy value: %d", MODULE_LOGTAG, params[5]);
			return 0;
		}
		policy = static_cast<ExpirePolicy>(params[5]);
	}
	int32_t extra = (numParams >= 6) ? static_cast<int32_t>(params[6]) : 0;

	float outVal = 0.0f;
	bool success = g_MemStore.IncFloat(ns, key, delta, outVal, policy, extra);
	if (newValAddr)
	{
		*newValAddr = amx_ftoc((REAL)outVal);
	}

	return success ? 1 : 0;
}

// native bool:mem_rank_inc(const namespace[], const key[], delta = 1, &new_score = 0, ExpirePolicy:policy = EXP_PERSISTENT, extra = 0);
static cell AMX_NATIVE_CALL amxx_mem_rank_inc(AMX *amx, cell *params)
{
	std::string ns, key;
	if (!GetNamespaceAndKey(amx, params[1], params[2], ns, key))
	{
		return 0;
	}

	unsigned int numParams = (*params) / sizeof(cell);
	cell delta = (numParams >= 3) ? params[3] : 1;

	cell *newScoreAddr = nullptr;
	if (numParams >= 4 && params[4] != 0)
	{
		newScoreAddr = MF_GetAmxAddr(amx, params[4]);
	}

	ExpirePolicy policy = ExpirePolicy::Persistent;
	if (numParams >= 5)
	{
		if (!IsValidExpirePolicy(params[5]))
		{
			MF_LogError(amx, AMX_ERR_NATIVE, "[%s] Invalid ExpirePolicy value: %d", MODULE_LOGTAG, params[5]);
			return 0;
		}
		policy = static_cast<ExpirePolicy>(params[5]);
	}
	int32_t extra = (numParams >= 6) ? static_cast<int32_t>(params[6]) : 0;

	cell outScore = 0;
	bool success = g_MemStore.RankInc(ns, key, delta, outScore, policy, extra);
	if (newScoreAddr)
	{
		*newScoreAddr = outScore;
	}

	return success ? 1 : 0;
}

// native mem_get_ttl(const namespace[], const key[]);
static cell AMX_NATIVE_CALL amxx_mem_get_ttl(AMX *amx, cell *params)
{
	std::string ns, key;
	if (!GetNamespaceAndKey(amx, params[1], params[2], ns, key))
	{
		return -2;
	}

	return static_cast<cell>(g_MemStore.GetTTL(ns, key));
}

// native mem_rank_get_range(const namespace[], start_pos, count, keys[][], scores[], max_items, max_key_len, RankOrder:order = RANK_DESC);
static cell AMX_NATIVE_CALL amxx_mem_rank_get_range(AMX *amx, cell *params)
{
	int len = 0;
	char *nsStr = MF_GetAmxString(amx, params[1], 0, &len);
	if (!nsStr || len == 0)
	{
		MF_LogError(amx, AMX_ERR_NATIVE, "[%s] Namespace name cannot be empty!", MODULE_LOGTAG);
		return 0;
	}

	cell startPos = params[2];
	cell count = params[3];
	if (startPos < 1 || count <= 0)
	{
		return 0;
	}

	cell maxItems = params[6];
	cell maxKeyLen = params[7];
	if (maxItems <= 0 || maxKeyLen <= 0)
	{
		return 0;
	}

	unsigned int numParams = (*params) / sizeof(cell);
	RankOrder order = RankOrder::Desc;
	if (numParams >= 8)
	{
		if (!IsValidRankOrder(params[8]))
		{
			MF_LogError(amx, AMX_ERR_NATIVE, "[%s] Invalid RankOrder value: %d", MODULE_LOGTAG, params[8]);
			return 0;
		}
		order = static_cast<RankOrder>(params[8]);
	}

	size_t fetchCount = std::min(static_cast<size_t>(count), static_cast<size_t>(maxItems));
	std::vector<std::pair<std::string, cell>> results;
	size_t retrieved = g_MemStore.RankGetRange(nsStr, startPos, fetchCount, results, order);
	if (retrieved == 0)
	{
		return 0;
	}

	cell *keysPtr = (params[4] != 0) ? MF_GetAmxAddr(amx, params[4]) : nullptr;
	cell *scoresPtr = (params[5] != 0) ? MF_GetAmxAddr(amx, params[5]) : nullptr;

	for (size_t i = 0; i < retrieved; ++i)
	{
		if (keysPtr)
		{
			cell rowAddr = params[4] + keysPtr[i];
			MF_SetAmxString(amx, rowAddr, results[i].first.c_str(), maxKeyLen);
		}
		if (scoresPtr)
		{
			scoresPtr[i] = results[i].second;
		}
	}

	return static_cast<cell>(retrieved);
}

// native bool:mem_rank_exists(const namespace[], const key[]);
static cell AMX_NATIVE_CALL amxx_mem_rank_exists(AMX *amx, cell *params)
{
	std::string ns, key;
	if (!GetNamespaceAndKey(amx, params[1], params[2], ns, key))
	{
		return 0;
	}

	return g_MemStore.RankExists(ns, key) ? 1 : 0;
}

// native mem_get_memory_stats(&used_bytes = 0, &max_bytes = 0, &namespaces_count = 0, &rank_namespaces_count = 0);
static cell AMX_NATIVE_CALL amxx_mem_get_memory_stats(AMX *amx, cell *params)
{
	unsigned int numParams = (*params) / sizeof(cell);

	size_t usedBytes = 0, maxBytes = 0, nsCount = 0, rankNsCount = 0;
	g_MemStore.GetMemoryStats(usedBytes, maxBytes, nsCount, rankNsCount);

	if (numParams >= 1 && params[1] != 0)
	{
		cell *ptr = MF_GetAmxAddr(amx, params[1]);
		if (ptr) *ptr = static_cast<cell>(usedBytes);
	}
	if (numParams >= 2 && params[2] != 0)
	{
		cell *ptr = MF_GetAmxAddr(amx, params[2]);
		if (ptr) *ptr = static_cast<cell>(maxBytes);
	}
	if (numParams >= 3 && params[3] != 0)
	{
		cell *ptr = MF_GetAmxAddr(amx, params[3]);
		if (ptr) *ptr = static_cast<cell>(nsCount);
	}
	if (numParams >= 4 && params[4] != 0)
	{
		cell *ptr = MF_GetAmxAddr(amx, params[4]);
		if (ptr) *ptr = static_cast<cell>(rankNsCount);
	}

	return 1;
}

// native mem_flush_all();
static cell AMX_NATIVE_CALL amxx_mem_flush_all(AMX *amx, cell *params)
{
	g_MemStore.FlushAll();
	return 1;
}

AMX_NATIVE_INFO MemStore_natives[] = {
	{"mem_set_int",             amxx_mem_set_int},
	{"mem_get_int",             amxx_mem_get_int},
	{"mem_set_float",           amxx_mem_set_float},
	{"mem_get_float",           amxx_mem_get_float},
	{"mem_set_string",          amxx_mem_set_string},
	{"mem_get_string",          amxx_mem_get_string},
	{"mem_set_array",           amxx_mem_set_array},
	{"mem_get_array",           amxx_mem_get_array},
	{"mem_get_array_size",      amxx_mem_get_array_size},
	{"mem_get_key_type",        amxx_mem_get_key_type},
	{"mem_key_exists",          amxx_mem_key_exists},
	{"mem_delete_key",          amxx_mem_delete_key},
	{"mem_clear_namespace",     amxx_mem_clear_namespace},
	{"mem_set_expire",          amxx_mem_set_expire},
	{"mem_get_namespace_count", amxx_mem_get_namespace_count},
	{"mem_get_key_at",          amxx_mem_get_key_at},
	{"mem_iterate_keys",        amxx_mem_iterate_keys},
	{"mem_rank_set",            amxx_mem_rank_set},
	{"mem_rank_get_score",      amxx_mem_rank_get_score},
	{"mem_rank_get_pos",        amxx_mem_rank_get_pos},
	{"mem_rank_get_top",        amxx_mem_rank_get_top},
	{"mem_rank_get_count",      amxx_mem_rank_get_count},
	{"mem_rank_delete",         amxx_mem_rank_delete},
	{"mem_rank_clear",          amxx_mem_rank_clear},
	{"mem_set_limits",          amxx_mem_set_limits},
	{"mem_inc_int",             amxx_mem_inc_int},
	{"mem_inc_float",           amxx_mem_inc_float},
	{"mem_rank_inc",            amxx_mem_rank_inc},
	{"mem_get_ttl",             amxx_mem_get_ttl},
	{"mem_rank_get_range",      amxx_mem_rank_get_range},
	{"mem_rank_exists",         amxx_mem_rank_exists},
	{"mem_get_memory_stats",    amxx_mem_get_memory_stats},
	{"mem_flush_all",           amxx_mem_flush_all},
	{nullptr,                   nullptr}
};
