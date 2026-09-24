// vim: set ts=4 sw=4 tw=99 noet:
//
// AMX Mod X MemStore Module
// In-Memory Transient Cross-Map Storage for AMX Mod X
//
// Author: iceeedR
//
#ifndef __MEMSTORE_H__
#define __MEMSTORE_H__

#include <string>
#include <vector>
#include <unordered_map>
#include <ctime>
#include <cstdint>
#include "amxxmodule.h"

enum class ExpirePolicy : int32_t
{
	Persistent = 0, // Survives maps indefinitely until HLDS shutdown/restart or manual deletion
	MapEnd     = 1, // Cleared automatically on next map change
	MapCount   = 2, // Survives for N map changes, then expires
	TTL        = 3  // Expires after N seconds based on wall-clock time
};

enum class EntryType : int32_t
{
	None   = 0,
	Int    = 1,
	Float  = 2,
	String = 3,
	Array  = 4
};

enum class RankOrder : int32_t
{
	Desc = 0, // Highest score ranks first (kills, points, score)
	Asc  = 1  // Lowest score ranks first (timer, deaths, speedrun)
};

const char *GetTypeName(EntryType type);

struct StoreEntry
{
	EntryType type = EntryType::None;
	ExpirePolicy policy = ExpirePolicy::Persistent;
	int32_t mapCounter = 0;
	time_t expireTimestamp = 0;

	cell cellValue = 0;
	float floatValue = 0.0f;
	std::string stringValue;
	std::vector<cell> arrayValue;

	bool IsExpired(time_t now) const
	{
		if (policy == ExpirePolicy::TTL)
		{
			return (now >= expireTimestamp);
		}
		if (policy == ExpirePolicy::MapCount)
		{
			return (mapCounter <= 0);
		}
		return false;
	}
};

struct RankEntry
{
	cell score = 0;
	uint64_t sequence = 0; // Monotonic sequence for tie-breaking
	ExpirePolicy policy = ExpirePolicy::Persistent;
	int32_t mapCounter = 0;
	time_t expireTimestamp = 0;

	bool IsExpired(time_t now) const
	{
		if (policy == ExpirePolicy::TTL)
		{
			return (now >= expireTimestamp);
		}
		if (policy == ExpirePolicy::MapCount)
		{
			return (mapCounter <= 0);
		}
		return false;
	}
};

class MemStoreManager
{
public:
	MemStoreManager();
	~MemStoreManager();

	// Setters
	bool SetInt(const std::string &ns, const std::string &key, cell val, ExpirePolicy policy, int32_t extra);
	bool SetFloat(const std::string &ns, const std::string &key, float val, ExpirePolicy policy, int32_t extra);
	bool SetString(const std::string &ns, const std::string &key, const std::string &val, ExpirePolicy policy, int32_t extra);
	bool SetArray(const std::string &ns, const std::string &key, const cell *data, size_t size, ExpirePolicy policy, int32_t extra);

	// Getters
	bool GetInt(const std::string &ns, const std::string &key, cell &val, cell defaultVal, bool &typeMismatch, bool &found);
	bool GetFloat(const std::string &ns, const std::string &key, float &val, float defaultVal, bool &typeMismatch, bool &found);
	bool GetString(const std::string &ns, const std::string &key, std::string &val, const std::string &defaultVal, bool &typeMismatch, bool &found);
	bool GetArray(const std::string &ns, const std::string &key, cell *dest, size_t maxlen, size_t &copied, bool &typeMismatch, bool &found);

	// Inspect & Manage
	size_t GetArraySize(const std::string &ns, const std::string &key);
	EntryType GetKeyType(const std::string &ns, const std::string &key);
	bool KeyExists(const std::string &ns, const std::string &key);
	bool DeleteKey(const std::string &ns, const std::string &key);
	bool ClearNamespace(const std::string &ns);
	bool SetExpire(const std::string &ns, const std::string &key, ExpirePolicy policy, int32_t extra);
	size_t GetNamespaceCount(const std::string &ns);

	// Feature B: Key Enumeration / Iteration
	bool GetKeyAt(const std::string &ns, size_t index, std::string &keyOut);

	// Feature A: Native Rank / Leaderboard Engine (Sorted Sets)
	bool RankSet(const std::string &ns, const std::string &key, cell score, ExpirePolicy policy, int32_t extra);
	bool RankGetScore(const std::string &ns, const std::string &key, cell &scoreOut);
	int32_t RankGetPosition(const std::string &ns, const std::string &key, RankOrder order);
	bool RankGetTop(const std::string &ns, int32_t rankPos, std::string &keyOut, cell &scoreOut, RankOrder order);
	size_t RankGetCount(const std::string &ns);
	bool RankDelete(const std::string &ns, const std::string &key);
	bool RankClear(const std::string &ns);

	// Lifecycle hooks
	void SweepMapEnd();
	void ClearAll();

	// Safety limits configuration
	void SetLimits(size_t maxKeysPerNs, size_t maxArraySize, size_t maxStringLen);
	size_t GetMaxKeysPerNs() const { return m_MaxKeysPerNamespace; }
	size_t GetMaxArraySize() const { return m_MaxArraySize; }
	size_t GetMaxStringLen() const { return m_MaxStringLength; }

private:
	bool CheckNamespaceKeyLimit(const std::string &ns, const std::string &key);
	void ApplyExpiration(StoreEntry &entry, ExpirePolicy policy, int32_t extra);
	void ApplyRankExpiration(RankEntry &entry, ExpirePolicy policy, int32_t extra);

	using KeyMap = std::unordered_map<std::string, StoreEntry>;
	std::unordered_map<std::string, KeyMap> m_Namespaces;

	using RankMap = std::unordered_map<std::string, RankEntry>;
	std::unordered_map<std::string, RankMap> m_Ranks;

	uint64_t m_RankSequenceCounter;

	size_t m_MaxKeysPerNamespace;
	size_t m_MaxArraySize;
	size_t m_MaxStringLength;
};

extern MemStoreManager g_MemStore;

#endif // __MEMSTORE_H__
