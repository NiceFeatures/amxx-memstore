// vim: set ts=4 sw=4 tw=99 noet:
//
// AMX Mod X MemStore Module
// In-Memory Transient Cross-Map Storage for AMX Mod X
//
// Author: iceeedR
//
#include "MemStore.h"
#include <cstring>
#include <algorithm>

MemStoreManager g_MemStore;

const char *GetTypeName(EntryType type)
{
	switch (type)
	{
		case EntryType::Int:    return "Int";
		case EntryType::Float:  return "Float";
		case EntryType::String: return "String";
		case EntryType::Array:  return "Array";
		default:                return "None";
	}
}

// --------------------------------------------------
// MEMSTORE MANAGER IMPLEMENTATION
// --------------------------------------------------

MemStoreManager::MemStoreManager()
	: m_RankSequenceCounter(0),
	  m_MaxKeysPerNamespace(5000),
	  m_MaxArraySize(4096),
	  m_MaxStringLength(4096),
	  m_MaxNamespaces(200),
	  m_MaxMemoryBytes(64 * 1024 * 1024), // 64 MB default safety quota
	  m_ApproxMemoryUsed(0)
{
}

MemStoreManager::~MemStoreManager()
{
	ClearAll();
}

void MemStoreManager::SetLimits(size_t maxKeysPerNs, size_t maxArraySize, size_t maxStringLen, size_t maxNamespaces, size_t maxMemoryMb)
{
	if (maxKeysPerNs > 0)
	{
		m_MaxKeysPerNamespace = maxKeysPerNs;
	}
	if (maxArraySize > 0)
	{
		m_MaxArraySize = maxArraySize;
	}
	if (maxStringLen > 0)
	{
		m_MaxStringLength = maxStringLen;
	}
	if (maxNamespaces > 0)
	{
		m_MaxNamespaces = maxNamespaces;
	}
	if (maxMemoryMb > 0)
	{
		m_MaxMemoryBytes = maxMemoryMb * 1024 * 1024;
	}
}

void MemStoreManager::InvalidateKeyCache(const std::string &ns)
{
	auto it = m_KeyCaches.find(ns);
	if (it != m_KeyCaches.end())
	{
		it->second.valid = false;
		it->second.keys.clear();
	}
}

bool MemStoreManager::CheckMemoryLimit(size_t additionalBytes)
{
	return (m_ApproxMemoryUsed + additionalBytes <= m_MaxMemoryBytes);
}

void MemStoreManager::ApplyExpiration(StoreEntry &entry, ExpirePolicy policy, int32_t extra)
{
	entry.policy = policy;
	if (policy == ExpirePolicy::MapCount)
	{
		entry.mapCounter = (extra > 0) ? extra : 1;
	}
	else if (policy == ExpirePolicy::TTL)
	{
		static const int32_t MAX_TTL_SECONDS = 30 * 24 * 60 * 60; // 30 days cap
		int32_t ttl = (extra > 0) ? std::min(extra, MAX_TTL_SECONDS) : 60;
		entry.expireTime = std::chrono::steady_clock::now() + std::chrono::seconds(ttl);
	}
	else if (policy == ExpirePolicy::MapEnd)
	{
		entry.mapCounter = 1;
	}
}

void MemStoreManager::ApplyRankExpiration(RankEntry &entry, ExpirePolicy policy, int32_t extra)
{
	entry.policy = policy;
	if (policy == ExpirePolicy::MapCount)
	{
		entry.mapCounter = (extra > 0) ? extra : 1;
	}
	else if (policy == ExpirePolicy::TTL)
	{
		static const int32_t MAX_TTL_SECONDS = 30 * 24 * 60 * 60; // 30 days cap
		int32_t ttl = (extra > 0) ? std::min(extra, MAX_TTL_SECONDS) : 60;
		entry.expireTime = std::chrono::steady_clock::now() + std::chrono::seconds(ttl);
	}
	else if (policy == ExpirePolicy::MapEnd)
	{
		entry.mapCounter = 1;
	}
}

bool MemStoreManager::CheckNamespaceKeyLimit(const std::string &ns, const std::string &key)
{
	auto nsIt = m_Namespaces.find(ns);
	if (nsIt != m_Namespaces.end())
	{
		auto keyIt = nsIt->second.find(key);
		if (keyIt != nsIt->second.end())
		{
			// Existing key update is permitted
			return true;
		}
		return nsIt->second.size() < m_MaxKeysPerNamespace;
	}
	// New namespace — enforce global namespace limit
	return m_Namespaces.size() < m_MaxNamespaces;
}

// --------------------------------------------------
// SETTERS
// --------------------------------------------------

bool MemStoreManager::SetInt(const std::string &ns, const std::string &key, cell val, ExpirePolicy policy, int32_t extra)
{
	if (!CheckNamespaceKeyLimit(ns, key))
	{
		return false;
	}

	StoreEntry &entry = m_Namespaces[ns][key];

	// Adjust memory tracking if old entry held dynamic payload
	if (entry.type == EntryType::String)
	{
		size_t bytes = entry.stringValue.capacity();
		m_ApproxMemoryUsed = (m_ApproxMemoryUsed >= bytes) ? (m_ApproxMemoryUsed - bytes) : 0;
	}
	else if (entry.type == EntryType::Array)
	{
		size_t bytes = entry.arrayValue.capacity() * sizeof(cell);
		m_ApproxMemoryUsed = (m_ApproxMemoryUsed >= bytes) ? (m_ApproxMemoryUsed - bytes) : 0;
	}

	entry.type = EntryType::Int;
	entry.cellValue = val;
	entry.floatValue = 0.0f;
	entry.stringValue.clear();
	entry.stringValue.shrink_to_fit();
	entry.arrayValue.clear();
	entry.arrayValue.shrink_to_fit();
	ApplyExpiration(entry, policy, extra);

	InvalidateKeyCache(ns);
	return true;
}

bool MemStoreManager::SetFloat(const std::string &ns, const std::string &key, float val, ExpirePolicy policy, int32_t extra)
{
	if (!CheckNamespaceKeyLimit(ns, key))
	{
		return false;
	}

	StoreEntry &entry = m_Namespaces[ns][key];

	if (entry.type == EntryType::String)
	{
		size_t bytes = entry.stringValue.capacity();
		m_ApproxMemoryUsed = (m_ApproxMemoryUsed >= bytes) ? (m_ApproxMemoryUsed - bytes) : 0;
	}
	else if (entry.type == EntryType::Array)
	{
		size_t bytes = entry.arrayValue.capacity() * sizeof(cell);
		m_ApproxMemoryUsed = (m_ApproxMemoryUsed >= bytes) ? (m_ApproxMemoryUsed - bytes) : 0;
	}

	entry.type = EntryType::Float;
	entry.cellValue = 0;
	entry.floatValue = val;
	entry.stringValue.clear();
	entry.stringValue.shrink_to_fit();
	entry.arrayValue.clear();
	entry.arrayValue.shrink_to_fit();
	ApplyExpiration(entry, policy, extra);

	InvalidateKeyCache(ns);
	return true;
}

bool MemStoreManager::SetString(const std::string &ns, const std::string &key, const std::string &val, ExpirePolicy policy, int32_t extra)
{
	if (!CheckNamespaceKeyLimit(ns, key))
	{
		return false;
	}

	std::string stored = val;
	if (stored.length() > m_MaxStringLength)
	{
		MF_Log("[%s] WARNING: String for key '%s' in namespace '%s' truncated from %u to %u chars.",
			MODULE_LOGTAG, key.c_str(), ns.c_str(),
			static_cast<unsigned int>(stored.length()),
			static_cast<unsigned int>(m_MaxStringLength));
		stored.resize(m_MaxStringLength);
	}

	size_t newBytes = stored.length();
	if (!CheckMemoryLimit(newBytes))
	{
		MF_Log("ERROR: Global memory quota exceeded (%u MB). String insertion rejected.",
			static_cast<unsigned int>(m_MaxMemoryBytes / (1024 * 1024)));
		return false;
	}

	StoreEntry &entry = m_Namespaces[ns][key];

	// Adjust memory tracking
	if (entry.type == EntryType::String)
	{
		size_t oldBytes = entry.stringValue.capacity();
		m_ApproxMemoryUsed = (m_ApproxMemoryUsed >= oldBytes) ? (m_ApproxMemoryUsed - oldBytes) : 0;
	}
	else if (entry.type == EntryType::Array)
	{
		size_t oldBytes = entry.arrayValue.capacity() * sizeof(cell);
		m_ApproxMemoryUsed = (m_ApproxMemoryUsed >= oldBytes) ? (m_ApproxMemoryUsed - oldBytes) : 0;
	}

	m_ApproxMemoryUsed += newBytes;

	entry.type = EntryType::String;
	entry.cellValue = 0;
	entry.floatValue = 0.0f;
	entry.stringValue = std::move(stored);
	entry.arrayValue.clear();
	entry.arrayValue.shrink_to_fit();
	ApplyExpiration(entry, policy, extra);

	InvalidateKeyCache(ns);
	return true;
}

bool MemStoreManager::SetArray(const std::string &ns, const std::string &key, const cell *data, size_t size, ExpirePolicy policy, int32_t extra)
{
	if (!CheckNamespaceKeyLimit(ns, key))
	{
		return false;
	}

	size_t clampedSize = std::min(size, m_MaxArraySize);
	size_t newBytes = clampedSize * sizeof(cell);

	if (!CheckMemoryLimit(newBytes))
	{
		MF_Log("ERROR: Global memory quota exceeded (%u MB). Array insertion rejected.",
			static_cast<unsigned int>(m_MaxMemoryBytes / (1024 * 1024)));
		return false;
	}

	StoreEntry &entry = m_Namespaces[ns][key];

	if (entry.type == EntryType::String)
	{
		size_t oldBytes = entry.stringValue.capacity();
		m_ApproxMemoryUsed = (m_ApproxMemoryUsed >= oldBytes) ? (m_ApproxMemoryUsed - oldBytes) : 0;
	}
	else if (entry.type == EntryType::Array)
	{
		size_t oldBytes = entry.arrayValue.capacity() * sizeof(cell);
		m_ApproxMemoryUsed = (m_ApproxMemoryUsed >= oldBytes) ? (m_ApproxMemoryUsed - oldBytes) : 0;
	}

	m_ApproxMemoryUsed += newBytes;

	entry.type = EntryType::Array;
	entry.cellValue = 0;
	entry.floatValue = 0.0f;
	entry.stringValue.clear();
	entry.stringValue.shrink_to_fit();
	entry.arrayValue.assign(data, data + clampedSize);
	ApplyExpiration(entry, policy, extra);

	InvalidateKeyCache(ns);
	return true;
}

// --------------------------------------------------
// GETTERS
// --------------------------------------------------

bool MemStoreManager::GetInt(const std::string &ns, const std::string &key, cell &val, cell defaultVal, bool &typeMismatch, bool &found)
{
	typeMismatch = false;
	found = false;

	auto nsIt = m_Namespaces.find(ns);
	if (nsIt == m_Namespaces.end())
	{
		val = defaultVal;
		return false;
	}

	auto keyIt = nsIt->second.find(key);
	if (keyIt == nsIt->second.end())
	{
		val = defaultVal;
		return false;
	}

	auto now = std::chrono::steady_clock::now();
	if (keyIt->second.IsExpired(now))
	{
		nsIt->second.erase(keyIt);
		InvalidateKeyCache(ns);
		if (nsIt->second.empty())
		{
			m_Namespaces.erase(nsIt);
			m_KeyCaches.erase(ns);
		}
		val = defaultVal;
		return false;
	}

	found = true;
	if (keyIt->second.type != EntryType::Int)
	{
		typeMismatch = true;
		val = defaultVal;
		return false;
	}

	val = keyIt->second.cellValue;
	return true;
}

bool MemStoreManager::GetFloat(const std::string &ns, const std::string &key, float &val, float defaultVal, bool &typeMismatch, bool &found)
{
	typeMismatch = false;
	found = false;

	auto nsIt = m_Namespaces.find(ns);
	if (nsIt == m_Namespaces.end())
	{
		val = defaultVal;
		return false;
	}

	auto keyIt = nsIt->second.find(key);
	if (keyIt == nsIt->second.end())
	{
		val = defaultVal;
		return false;
	}

	auto now = std::chrono::steady_clock::now();
	if (keyIt->second.IsExpired(now))
	{
		nsIt->second.erase(keyIt);
		InvalidateKeyCache(ns);
		if (nsIt->second.empty())
		{
			m_Namespaces.erase(nsIt);
			m_KeyCaches.erase(ns);
		}
		val = defaultVal;
		return false;
	}

	found = true;
	if (keyIt->second.type != EntryType::Float)
	{
		typeMismatch = true;
		val = defaultVal;
		return false;
	}

	val = keyIt->second.floatValue;
	return true;
}

bool MemStoreManager::GetString(const std::string &ns, const std::string &key, std::string &val, const std::string &defaultVal, bool &typeMismatch, bool &found)
{
	typeMismatch = false;
	found = false;

	auto nsIt = m_Namespaces.find(ns);
	if (nsIt == m_Namespaces.end())
	{
		val = defaultVal;
		return false;
	}

	auto keyIt = nsIt->second.find(key);
	if (keyIt == nsIt->second.end())
	{
		val = defaultVal;
		return false;
	}

	auto now = std::chrono::steady_clock::now();
	if (keyIt->second.IsExpired(now))
	{
		size_t bytes = keyIt->second.stringValue.capacity();
		m_ApproxMemoryUsed = (m_ApproxMemoryUsed >= bytes) ? (m_ApproxMemoryUsed - bytes) : 0;

		nsIt->second.erase(keyIt);
		InvalidateKeyCache(ns);
		if (nsIt->second.empty())
		{
			m_Namespaces.erase(nsIt);
			m_KeyCaches.erase(ns);
		}
		val = defaultVal;
		return false;
	}

	found = true;
	if (keyIt->second.type != EntryType::String)
	{
		typeMismatch = true;
		val = defaultVal;
		return false;
	}

	val = keyIt->second.stringValue;
	return true;
}

bool MemStoreManager::GetArray(const std::string &ns, const std::string &key, cell *dest, size_t maxlen, size_t &copied, bool &typeMismatch, bool &found)
{
	typeMismatch = false;
	found = false;
	copied = 0;

	auto nsIt = m_Namespaces.find(ns);
	if (nsIt == m_Namespaces.end())
	{
		return false;
	}

	auto keyIt = nsIt->second.find(key);
	if (keyIt == nsIt->second.end())
	{
		return false;
	}

	auto now = std::chrono::steady_clock::now();
	if (keyIt->second.IsExpired(now))
	{
		size_t bytes = keyIt->second.arrayValue.capacity() * sizeof(cell);
		m_ApproxMemoryUsed = (m_ApproxMemoryUsed >= bytes) ? (m_ApproxMemoryUsed - bytes) : 0;

		nsIt->second.erase(keyIt);
		InvalidateKeyCache(ns);
		if (nsIt->second.empty())
		{
			m_Namespaces.erase(nsIt);
			m_KeyCaches.erase(ns);
		}
		return false;
	}

	found = true;
	if (keyIt->second.type != EntryType::Array)
	{
		typeMismatch = true;
		return false;
	}

	copied = std::min(maxlen, keyIt->second.arrayValue.size());
	if (copied > 0 && dest != nullptr)
	{
		std::copy(keyIt->second.arrayValue.begin(), keyIt->second.arrayValue.begin() + copied, dest);
	}

	return true;
}

size_t MemStoreManager::GetArraySize(const std::string &ns, const std::string &key)
{
	auto nsIt = m_Namespaces.find(ns);
	if (nsIt == m_Namespaces.end())
	{
		return 0;
	}

	auto keyIt = nsIt->second.find(key);
	if (keyIt == nsIt->second.end())
	{
		return 0;
	}

	auto now = std::chrono::steady_clock::now();
	if (keyIt->second.IsExpired(now))
	{
		size_t bytes = keyIt->second.arrayValue.capacity() * sizeof(cell);
		m_ApproxMemoryUsed = (m_ApproxMemoryUsed >= bytes) ? (m_ApproxMemoryUsed - bytes) : 0;

		nsIt->second.erase(keyIt);
		InvalidateKeyCache(ns);
		if (nsIt->second.empty())
		{
			m_Namespaces.erase(nsIt);
			m_KeyCaches.erase(ns);
		}
		return 0;
	}

	if (keyIt->second.type != EntryType::Array)
	{
		return 0;
	}

	return keyIt->second.arrayValue.size();
}

EntryType MemStoreManager::GetKeyType(const std::string &ns, const std::string &key)
{
	auto nsIt = m_Namespaces.find(ns);
	if (nsIt == m_Namespaces.end())
	{
		return EntryType::None;
	}

	auto keyIt = nsIt->second.find(key);
	if (keyIt == nsIt->second.end())
	{
		return EntryType::None;
	}

	auto now = std::chrono::steady_clock::now();
	if (keyIt->second.IsExpired(now))
	{
		nsIt->second.erase(keyIt);
		InvalidateKeyCache(ns);
		if (nsIt->second.empty())
		{
			m_Namespaces.erase(nsIt);
			m_KeyCaches.erase(ns);
		}
		return EntryType::None;
	}

	return keyIt->second.type;
}

bool MemStoreManager::KeyExists(const std::string &ns, const std::string &key)
{
	auto nsIt = m_Namespaces.find(ns);
	if (nsIt == m_Namespaces.end())
	{
		return false;
	}

	auto keyIt = nsIt->second.find(key);
	if (keyIt == nsIt->second.end())
	{
		return false;
	}

	auto now = std::chrono::steady_clock::now();
	if (keyIt->second.IsExpired(now))
	{
		nsIt->second.erase(keyIt);
		InvalidateKeyCache(ns);
		if (nsIt->second.empty())
		{
			m_Namespaces.erase(nsIt);
			m_KeyCaches.erase(ns);
		}
		return false;
	}

	return true;
}

bool MemStoreManager::DeleteKey(const std::string &ns, const std::string &key)
{
	auto nsIt = m_Namespaces.find(ns);
	if (nsIt == m_Namespaces.end())
	{
		return false;
	}

	auto keyIt = nsIt->second.find(key);
	if (keyIt == nsIt->second.end())
	{
		return false;
	}

	if (keyIt->second.type == EntryType::String)
	{
		size_t bytes = keyIt->second.stringValue.capacity();
		m_ApproxMemoryUsed = (m_ApproxMemoryUsed >= bytes) ? (m_ApproxMemoryUsed - bytes) : 0;
	}
	else if (keyIt->second.type == EntryType::Array)
	{
		size_t bytes = keyIt->second.arrayValue.capacity() * sizeof(cell);
		m_ApproxMemoryUsed = (m_ApproxMemoryUsed >= bytes) ? (m_ApproxMemoryUsed - bytes) : 0;
	}

	nsIt->second.erase(keyIt);
	InvalidateKeyCache(ns);

	// TASK 2: Clean up empty namespace to avoid leaking against m_MaxNamespaces limit
	if (nsIt->second.empty())
	{
		m_Namespaces.erase(nsIt);
		m_KeyCaches.erase(ns);
	}

	return true;
}

bool MemStoreManager::ClearNamespace(const std::string &ns)
{
	auto nsIt = m_Namespaces.find(ns);
	if (nsIt == m_Namespaces.end())
	{
		return false;
	}

	for (const auto &pair : nsIt->second)
	{
		if (pair.second.type == EntryType::String)
		{
			size_t bytes = pair.second.stringValue.capacity();
			m_ApproxMemoryUsed = (m_ApproxMemoryUsed >= bytes) ? (m_ApproxMemoryUsed - bytes) : 0;
		}
		else if (pair.second.type == EntryType::Array)
		{
			size_t bytes = pair.second.arrayValue.capacity() * sizeof(cell);
			m_ApproxMemoryUsed = (m_ApproxMemoryUsed >= bytes) ? (m_ApproxMemoryUsed - bytes) : 0;
		}
	}

	m_Namespaces.erase(nsIt);
	m_KeyCaches.erase(ns);
	return true;
}

bool MemStoreManager::SetExpire(const std::string &ns, const std::string &key, ExpirePolicy policy, int32_t extra)
{
	auto nsIt = m_Namespaces.find(ns);
	if (nsIt == m_Namespaces.end())
	{
		return false;
	}

	auto keyIt = nsIt->second.find(key);
	if (keyIt == nsIt->second.end())
	{
		return false;
	}

	ApplyExpiration(keyIt->second, policy, extra);
	return true;
}

size_t MemStoreManager::GetNamespaceCount(const std::string &ns)
{
	auto nsIt = m_Namespaces.find(ns);
	if (nsIt == m_Namespaces.end())
	{
		return 0;
	}

	auto now = std::chrono::steady_clock::now();
	size_t activeCount = 0;
	for (auto it = nsIt->second.begin(); it != nsIt->second.end(); )
	{
		if (it->second.IsExpired(now))
		{
			it = nsIt->second.erase(it);
			InvalidateKeyCache(ns);
		}
		else
		{
			++activeCount;
			++it;
		}
	}

	if (nsIt->second.empty())
	{
		m_Namespaces.erase(nsIt);
		m_KeyCaches.erase(ns);
	}

	return activeCount;
}

// --------------------------------------------------
// FEATURE B: KEY ENUMERATION / ITERATION
// --------------------------------------------------

std::vector<std::string> MemStoreManager::GetActiveKeys(const std::string &ns)
{
	std::vector<std::string> keys;
	auto nsIt = m_Namespaces.find(ns);
	if (nsIt == m_Namespaces.end())
	{
		return keys;
	}

	auto now = std::chrono::steady_clock::now();
	keys.reserve(nsIt->second.size());

	for (auto it = nsIt->second.begin(); it != nsIt->second.end(); )
	{
		if (it->second.IsExpired(now))
		{
			it = nsIt->second.erase(it);
		}
		else
		{
			keys.push_back(it->first);
			++it;
		}
	}

	if (nsIt->second.empty())
	{
		m_Namespaces.erase(nsIt);
		m_KeyCaches.erase(ns);
	}

	return keys;
}

bool MemStoreManager::GetKeyAt(const std::string &ns, size_t index, std::string &keyOut)
{
	auto nsIt = m_Namespaces.find(ns);
	if (nsIt == m_Namespaces.end())
	{
		return false;
	}

	// TASK 4: On-demand indexed caching for O(1) loop access
	auto &cache = m_KeyCaches[ns];
	if (!cache.valid)
	{
		cache.keys = GetActiveKeys(ns);
		cache.valid = true;
	}

	if (index < cache.keys.size())
	{
		keyOut = cache.keys[index];
		return true;
	}

	return false;
}

// --------------------------------------------------
// FEATURE A: NATIVE RANK / LEADERBOARD ENGINE
// --------------------------------------------------

bool MemStoreManager::RankSet(const std::string &ns, const std::string &key, cell score, ExpirePolicy policy, int32_t extra)
{
	auto rankNsIt = m_Ranks.find(ns);
	if (rankNsIt == m_Ranks.end())
	{
		// New rank namespace — enforce global limit
		if (m_Ranks.size() >= m_MaxNamespaces)
		{
			return false;
		}
	}

	auto &rankMap = m_Ranks[ns];
	auto it = rankMap.find(key);
	if (it == rankMap.end())
	{
		if (rankMap.size() >= m_MaxKeysPerNamespace)
		{
			return false;
		}
	}

	RankEntry &entry = rankMap[key];
	entry.score = score;
	entry.sequence = ++m_RankSequenceCounter;
	ApplyRankExpiration(entry, policy, extra);

	return true;
}

bool MemStoreManager::RankGetScore(const std::string &ns, const std::string &key, cell &scoreOut)
{
	auto nsIt = m_Ranks.find(ns);
	if (nsIt == m_Ranks.end())
	{
		return false;
	}

	auto it = nsIt->second.find(key);
	if (it == nsIt->second.end())
	{
		return false;
	}

	auto now = std::chrono::steady_clock::now();
	if (it->second.IsExpired(now))
	{
		nsIt->second.erase(it);
		if (nsIt->second.empty())
		{
			m_Ranks.erase(nsIt);
		}
		return false;
	}

	scoreOut = it->second.score;
	return true;
}

int32_t MemStoreManager::RankGetPosition(const std::string &ns, const std::string &key, RankOrder order)
{
	auto nsIt = m_Ranks.find(ns);
	if (nsIt == m_Ranks.end())
	{
		return 0;
	}

	auto now = std::chrono::steady_clock::now();
	auto targetIt = nsIt->second.find(key);
	if (targetIt == nsIt->second.end())
	{
		return 0;
	}

	if (targetIt->second.IsExpired(now))
	{
		nsIt->second.erase(targetIt);
		if (nsIt->second.empty())
		{
			m_Ranks.erase(nsIt);
		}
		return 0;
	}

	cell targetScore = targetIt->second.score;
	uint64_t targetSeq = targetIt->second.sequence;
	int32_t rank = 1;

	// TASK 3: O(N) direct count without heap allocations or full sort
	for (auto it = nsIt->second.begin(); it != nsIt->second.end(); )
	{
		if (it->second.IsExpired(now))
		{
			it = nsIt->second.erase(it);
			continue;
		}

		if (it->first != key)
		{
			if (order == RankOrder::Desc)
			{
				if (it->second.score > targetScore || 
				   (it->second.score == targetScore && it->second.sequence < targetSeq))
				{
					++rank;
				}
			}
			else
			{
				if (it->second.score < targetScore || 
				   (it->second.score == targetScore && it->second.sequence < targetSeq))
				{
					++rank;
				}
			}
		}
		++it;
	}

	if (nsIt->second.empty())
	{
		m_Ranks.erase(nsIt);
	}

	return rank;
}

bool MemStoreManager::RankGetTop(const std::string &ns, int32_t rankPos, std::string &keyOut, cell &scoreOut, RankOrder order)
{
	if (rankPos < 1)
	{
		return false;
	}

	auto nsIt = m_Ranks.find(ns);
	if (nsIt == m_Ranks.end())
	{
		return false;
	}

	auto now = std::chrono::steady_clock::now();
	std::vector<std::pair<std::string, RankEntry>> activeEntries;
	activeEntries.reserve(nsIt->second.size());

	for (auto it = nsIt->second.begin(); it != nsIt->second.end(); )
	{
		if (it->second.IsExpired(now))
		{
			it = nsIt->second.erase(it);
		}
		else
		{
			activeEntries.emplace_back(it->first, it->second);
			++it;
		}
	}

	if (nsIt->second.empty())
	{
		m_Ranks.erase(nsIt);
		return false;
	}

	if (static_cast<size_t>(rankPos) > activeEntries.size())
	{
		return false;
	}

	size_t targetIdx = static_cast<size_t>(rankPos - 1);

	// TASK 3: std::nth_element achieves O(N) selection instead of O(N log N) full sort
	if (order == RankOrder::Desc)
	{
		auto compDesc = [](const std::pair<std::string, RankEntry> &a, const std::pair<std::string, RankEntry> &b) {
			if (a.second.score != b.second.score)
				return a.second.score > b.second.score;
			return a.second.sequence < b.second.sequence;
		};
		std::nth_element(activeEntries.begin(), activeEntries.begin() + targetIdx, activeEntries.end(), compDesc);
	}
	else
	{
		auto compAsc = [](const std::pair<std::string, RankEntry> &a, const std::pair<std::string, RankEntry> &b) {
			if (a.second.score != b.second.score)
				return a.second.score < b.second.score;
			return a.second.sequence < b.second.sequence;
		};
		std::nth_element(activeEntries.begin(), activeEntries.begin() + targetIdx, activeEntries.end(), compAsc);
	}

	keyOut = activeEntries[targetIdx].first;
	scoreOut = activeEntries[targetIdx].second.score;
	return true;
}

size_t MemStoreManager::RankGetCount(const std::string &ns)
{
	auto nsIt = m_Ranks.find(ns);
	if (nsIt == m_Ranks.end())
	{
		return 0;
	}

	auto now = std::chrono::steady_clock::now();
	size_t count = 0;
	for (auto it = nsIt->second.begin(); it != nsIt->second.end(); )
	{
		if (it->second.IsExpired(now))
		{
			it = nsIt->second.erase(it);
		}
		else
		{
			++count;
			++it;
		}
	}

	if (nsIt->second.empty())
	{
		m_Ranks.erase(nsIt);
	}

	return count;
}

bool MemStoreManager::RankDelete(const std::string &ns, const std::string &key)
{
	auto nsIt = m_Ranks.find(ns);
	if (nsIt == m_Ranks.end())
	{
		return false;
	}

	auto it = nsIt->second.find(key);
	if (it == nsIt->second.end())
	{
		return false;
	}

	nsIt->second.erase(it);

	// TASK 2: Clean up empty rank namespace
	if (nsIt->second.empty())
	{
		m_Ranks.erase(nsIt);
	}

	return true;
}

bool MemStoreManager::RankClear(const std::string &ns)
{
	auto nsIt = m_Ranks.find(ns);
	if (nsIt == m_Ranks.end())
	{
		return false;
	}

	m_Ranks.erase(nsIt);
	return true;
}

// --------------------------------------------------
// LIFECYCLE HOOKS
// --------------------------------------------------

void MemStoreManager::SweepMapEnd()
{
	auto now = std::chrono::steady_clock::now();

	// 1. Sweep regular key-values
	for (auto nsIt = m_Namespaces.begin(); nsIt != m_Namespaces.end(); )
	{
		auto &keyMap = nsIt->second;
		for (auto keyIt = keyMap.begin(); keyIt != keyMap.end(); )
		{
			auto &entry = keyIt->second;

			if (entry.policy == ExpirePolicy::MapEnd)
			{
				if (entry.type == EntryType::String)
				{
					size_t bytes = entry.stringValue.capacity();
					m_ApproxMemoryUsed = (m_ApproxMemoryUsed >= bytes) ? (m_ApproxMemoryUsed - bytes) : 0;
				}
				else if (entry.type == EntryType::Array)
				{
					size_t bytes = entry.arrayValue.capacity() * sizeof(cell);
					m_ApproxMemoryUsed = (m_ApproxMemoryUsed >= bytes) ? (m_ApproxMemoryUsed - bytes) : 0;
				}
				keyIt = keyMap.erase(keyIt);
			}
			else if (entry.policy == ExpirePolicy::MapCount)
			{
				--entry.mapCounter;
				if (entry.mapCounter <= 0)
				{
					if (entry.type == EntryType::String)
					{
						size_t bytes = entry.stringValue.capacity();
						m_ApproxMemoryUsed = (m_ApproxMemoryUsed >= bytes) ? (m_ApproxMemoryUsed - bytes) : 0;
					}
					else if (entry.type == EntryType::Array)
					{
						size_t bytes = entry.arrayValue.capacity() * sizeof(cell);
						m_ApproxMemoryUsed = (m_ApproxMemoryUsed >= bytes) ? (m_ApproxMemoryUsed - bytes) : 0;
					}
					keyIt = keyMap.erase(keyIt);
				}
				else
				{
					++keyIt;
				}
			}
			else if (entry.policy == ExpirePolicy::TTL)
			{
				if (now >= entry.expireTime)
				{
					if (entry.type == EntryType::String)
					{
						size_t bytes = entry.stringValue.capacity();
						m_ApproxMemoryUsed = (m_ApproxMemoryUsed >= bytes) ? (m_ApproxMemoryUsed - bytes) : 0;
					}
					else if (entry.type == EntryType::Array)
					{
						size_t bytes = entry.arrayValue.capacity() * sizeof(cell);
						m_ApproxMemoryUsed = (m_ApproxMemoryUsed >= bytes) ? (m_ApproxMemoryUsed - bytes) : 0;
					}
					keyIt = keyMap.erase(keyIt);
				}
				else
				{
					++keyIt;
				}
			}
			else
			{
				// ExpirePolicy::Persistent stays intact
				++keyIt;
			}
		}

		if (keyMap.empty())
		{
			nsIt = m_Namespaces.erase(nsIt);
		}
		else
		{
			++nsIt;
		}
	}

	// Clear cached key vectors
	m_KeyCaches.clear();

	// 2. Sweep rank leaderboards
	for (auto rankIt = m_Ranks.begin(); rankIt != m_Ranks.end(); )
	{
		auto &rankMap = rankIt->second;
		for (auto entryIt = rankMap.begin(); entryIt != rankMap.end(); )
		{
			auto &entry = entryIt->second;

			if (entry.policy == ExpirePolicy::MapEnd)
			{
				entryIt = rankMap.erase(entryIt);
			}
			else if (entry.policy == ExpirePolicy::MapCount)
			{
				--entry.mapCounter;
				if (entry.mapCounter <= 0)
				{
					entryIt = rankMap.erase(entryIt);
				}
				else
				{
					++entryIt;
				}
			}
			else if (entry.policy == ExpirePolicy::TTL)
			{
				if (now >= entry.expireTime)
				{
					entryIt = rankMap.erase(entryIt);
				}
				else
				{
					++entryIt;
				}
			}
			else
			{
				// ExpirePolicy::Persistent stays intact
				++entryIt;
			}
		}

		if (rankMap.empty())
		{
			rankIt = m_Ranks.erase(rankIt);
		}
		else
		{
			++rankIt;
		}
	}
}

void MemStoreManager::ClearAll()
{
	m_Namespaces.clear();
	m_Ranks.clear();
	m_KeyCaches.clear();
	m_ApproxMemoryUsed = 0;
	m_RankSequenceCounter = 0;
}
