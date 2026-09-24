// vim: set ts=4 sw=4 tw=99 noet:
//
// AMX Mod X MemStore Module
// In-Memory Transient Cross-Map Storage for AMX Mod X
//
// Author: iceeedR
//
#include "MemStore.h"

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
	if (!IsValidExpirePolicy(params[4]))
	{
		MF_LogError(amx, AMX_ERR_NATIVE, "[%s] Invalid ExpirePolicy value: %d", MODULE_LOGTAG, params[4]);
		return 0;
	}
	ExpirePolicy policy = static_cast<ExpirePolicy>(params[4]);
	int32_t extra = static_cast<int32_t>(params[5]);

	if (!g_MemStore.SetInt(ns, key, val, policy, extra))
	{
		MF_LogError(amx, AMX_ERR_NATIVE, "[%s] Namespace '%s' reached key limit (%u keys)! Set rejected.", MODULE_LOGTAG, ns.c_str(), static_cast<unsigned int>(g_MemStore.GetMaxKeysPerNs()));
		return 0;
	}

	return 1;
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
	cell defaultVal = params[4];

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
	if (!IsValidExpirePolicy(params[4]))
	{
		MF_LogError(amx, AMX_ERR_NATIVE, "[%s] Invalid ExpirePolicy value: %d", MODULE_LOGTAG, params[4]);
		return 0;
	}
	ExpirePolicy policy = static_cast<ExpirePolicy>(params[4]);
	int32_t extra = static_cast<int32_t>(params[5]);

	if (!g_MemStore.SetFloat(ns, key, val, policy, extra))
	{
		MF_LogError(amx, AMX_ERR_NATIVE, "[%s] Namespace '%s' reached key limit (%u keys)! Set rejected.", MODULE_LOGTAG, ns.c_str(), static_cast<unsigned int>(g_MemStore.GetMaxKeysPerNs()));
		return 0;
	}

	return 1;
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
	float defaultVal = amx_ctof(params[4]);

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

	if (!IsValidExpirePolicy(params[4]))
	{
		MF_LogError(amx, AMX_ERR_NATIVE, "[%s] Invalid ExpirePolicy value: %d", MODULE_LOGTAG, params[4]);
		return 0;
	}
	ExpirePolicy policy = static_cast<ExpirePolicy>(params[4]);
	int32_t extra = static_cast<int32_t>(params[5]);

	if (!g_MemStore.SetString(ns, key, val, policy, extra))
	{
		MF_LogError(amx, AMX_ERR_NATIVE, "[%s] Namespace '%s' reached key limit (%u keys)! Set rejected.", MODULE_LOGTAG, ns.c_str(), static_cast<unsigned int>(g_MemStore.GetMaxKeysPerNs()));
		return 0;
	}

	return 1;
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

	int defaultLen = 0;
	char *defaultStr = MF_GetAmxString(amx, params[5], 2, &defaultLen);
	std::string def = (defaultStr != nullptr) ? defaultStr : "";

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

	if (!IsValidExpirePolicy(params[5]))
	{
		MF_LogError(amx, AMX_ERR_NATIVE, "[%s] Invalid ExpirePolicy value: %d", MODULE_LOGTAG, params[5]);
		return 0;
	}
	ExpirePolicy policy = static_cast<ExpirePolicy>(params[5]);
	int32_t extra = static_cast<int32_t>(params[6]);

	if (!g_MemStore.SetArray(ns, key, arrPtr, static_cast<size_t>(size), policy, extra))
	{
		MF_LogError(amx, AMX_ERR_NATIVE, "[%s] Namespace '%s' reached key limit (%u keys)! Set rejected.", MODULE_LOGTAG, ns.c_str(), static_cast<unsigned int>(g_MemStore.GetMaxKeysPerNs()));
		return 0;
	}

	return 1;
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

	cell *copiedSizeAddr = nullptr;
	unsigned int numParams = (*params) / sizeof(cell);
	if (numParams >= 5)
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
	int32_t extra = static_cast<int32_t>(params[4]);

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

// native bool:mem_rank_set(const namespace[], const key[], score, ExpirePolicy:policy = EXP_PERSISTENT, extra = 0);
static cell AMX_NATIVE_CALL amxx_mem_rank_set(AMX *amx, cell *params)
{
	std::string ns, key;
	if (!GetNamespaceAndKey(amx, params[1], params[2], ns, key))
	{
		return 0;
	}

	cell score = params[3];
	if (!IsValidExpirePolicy(params[4]))
	{
		MF_LogError(amx, AMX_ERR_NATIVE, "[%s] Invalid ExpirePolicy value: %d", MODULE_LOGTAG, params[4]);
		return 0;
	}
	ExpirePolicy policy = static_cast<ExpirePolicy>(params[4]);
	int32_t extra = static_cast<int32_t>(params[5]);

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

	if (!IsValidRankOrder(params[3]))
	{
		MF_LogError(amx, AMX_ERR_NATIVE, "[%s] Invalid RankOrder value: %d", MODULE_LOGTAG, params[3]);
		return 0;
	}
	RankOrder order = static_cast<RankOrder>(params[3]);
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

	cell *scoreAddr = nullptr;
	unsigned int numParams = (*params) / sizeof(cell);
	if (numParams >= 5)
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
		return 0;
	}

	return g_MemStore.RankClear(nsStr) ? 1 : 0;
}

// native mem_set_limits(max_keys_per_ns = 10000, max_array_size = 4096, max_string_len = 4096);
static cell AMX_NATIVE_CALL amxx_mem_set_limits(AMX *amx, cell *params)
{
	cell maxKeys = params[1];
	cell maxArray = params[2];
	cell maxStr = params[3];

	size_t maxNs = 0;
	unsigned int numParams = (*params) / sizeof(cell);
	if (numParams >= 4)
	{
		cell maxNsParam = params[4];
		maxNs = (maxNsParam > 0) ? static_cast<size_t>(maxNsParam) : 0;
	}

	g_MemStore.SetLimits(
		(maxKeys > 0) ? static_cast<size_t>(maxKeys) : 10000,
		(maxArray > 0) ? static_cast<size_t>(maxArray) : 4096,
		(maxStr > 0) ? static_cast<size_t>(maxStr) : 4096,
		maxNs
	);

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
	{"mem_rank_set",            amxx_mem_rank_set},
	{"mem_rank_get_score",      amxx_mem_rank_get_score},
	{"mem_rank_get_pos",        amxx_mem_rank_get_pos},
	{"mem_rank_get_top",        amxx_mem_rank_get_top},
	{"mem_rank_get_count",      amxx_mem_rank_get_count},
	{"mem_rank_delete",         amxx_mem_rank_delete},
	{"mem_rank_clear",          amxx_mem_rank_clear},
	{"mem_set_limits",          amxx_mem_set_limits},
	{nullptr,                   nullptr}
};
