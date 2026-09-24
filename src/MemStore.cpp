// vim: set ts=4 sw=4 tw=99 noet:
//
// AMX Mod X MemStore Module
// In-Memory Transient Cross-Map Storage for AMX Mod X
//
// Author: iceeedR
//
#include "MemStore.h"
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

MemStoreManager::MemStoreManager()
	: m_RankSequenceCounter(0),
	  m_MaxKeysPerNamespace(10000),
	  m_MaxArraySize(4096),
	  m_MaxStringLength(4096),
	  m_MaxNamespaces(1000)
{
}

MemStoreManager::~MemStoreManager()
{
	ClearAll();
}

void MemStoreManager::SetLimits(size_t maxKeysPerNs, size_t maxArraySize, size_t maxStringLen, size_t maxNamespaces)
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
		static const int32_t MAX_TTL_SECONDS = 30 * 24 * 60 * 60; // 30 days
		int32_t ttl = (extra > 0) ? std::min(extra, MAX_TTL_SECONDS) : 60;
		entry.expireTimestamp = time(nullptr) + static_cast<time_t>(ttl);
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
		static const int32_t MAX_TTL_SECONDS = 30 * 24 * 60 * 60; // 30 days
		int32_t ttl = (extra > 0) ? std::min(extra, MAX_TTL_SECONDS) : 60;
		entry.expireTimestamp = time(nullptr) + static_cast<time_t>(ttl);
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
			// Existing key update is always permitted
			return true;
		}
		return nsIt->second.size() < m_MaxKeysPerNamespace;
	}
	// New namespace — enforce global namespace limit
	return m_Namespaces.size() < m_MaxNamespaces;
}

bool MemStoreManager::SetInt(const std::string &ns, const std::string &key, cell val, ExpirePolicy policy, int32_t extra)
{
	if (!CheckNamespaceKeyLimit(ns, key))
	{
		return false;
	}

	StoreEntry &entry = m_Namespaces[ns][key];
	entry.type = EntryType::Int;
	entry.cellValue = val;
	entry.floatValue = 0.0f;
	entry.stringValue.clear();
	entry.stringValue.shrink_to_fit();
	entry.arrayValue.clear();
	entry.arrayValue.shrink_to_fit();
	ApplyExpiration(entry, policy, extra);

	return true;
}

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

	time_t now = time(nullptr);
	if (keyIt->second.IsExpired(now))
	{
		nsIt->second.erase(keyIt);
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

bool MemStoreManager::SetFloat(const std::string &ns, const std::string &key, float val, ExpirePolicy policy, int32_t extra)
{
	if (!CheckNamespaceKeyLimit(ns, key))
	{
		return false;
	}

	StoreEntry &entry = m_Namespaces[ns][key];
	entry.type = EntryType::Float;
	entry.cellValue = 0;
	entry.floatValue = val;
	entry.stringValue.clear();
	entry.stringValue.shrink_to_fit();
	entry.arrayValue.clear();
	entry.arrayValue.shrink_to_fit();
	ApplyExpiration(entry, policy, extra);

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

	time_t now = time(nullptr);
	if (keyIt->second.IsExpired(now))
	{
		nsIt->second.erase(keyIt);
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

	StoreEntry &entry = m_Namespaces[ns][key];
	entry.type = EntryType::String;
	entry.cellValue = 0;
	entry.floatValue = 0.0f;
	entry.stringValue = std::move(stored);
	entry.arrayValue.clear();
	entry.arrayValue.shrink_to_fit();
	ApplyExpiration(entry, policy, extra);

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

	time_t now = time(nullptr);
	if (keyIt->second.IsExpired(now))
	{
		nsIt->second.erase(keyIt);
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

bool MemStoreManager::SetArray(const std::string &ns, const std::string &key, const cell *data, size_t size, ExpirePolicy policy, int32_t extra)
{
	if (!CheckNamespaceKeyLimit(ns, key))
	{
		return false;
	}

	size_t clampedSize = std::min(size, m_MaxArraySize);

	StoreEntry &entry = m_Namespaces[ns][key];
	entry.type = EntryType::Array;
	entry.cellValue = 0;
	entry.floatValue = 0.0f;
	entry.stringValue.clear();
	entry.stringValue.shrink_to_fit();
	entry.arrayValue.assign(data, data + clampedSize);
	ApplyExpiration(entry, policy, extra);

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

	time_t now = time(nullptr);
	if (keyIt->second.IsExpired(now))
	{
		nsIt->second.erase(keyIt);
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

	time_t now = time(nullptr);
	if (keyIt->second.IsExpired(now))
	{
		nsIt->second.erase(keyIt);
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

	time_t now = time(nullptr);
	if (keyIt->second.IsExpired(now))
	{
		nsIt->second.erase(keyIt);
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

	time_t now = time(nullptr);
	if (keyIt->second.IsExpired(now))
	{
		nsIt->second.erase(keyIt);
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

	nsIt->second.erase(keyIt);
	return true;
}

bool MemStoreManager::ClearNamespace(const std::string &ns)
{
	auto nsIt = m_Namespaces.find(ns);
	if (nsIt == m_Namespaces.end())
	{
		return false;
	}

	m_Namespaces.erase(nsIt);
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

	time_t now = time(nullptr);
	size_t activeCount = 0;
	for (auto it = nsIt->second.begin(); it != nsIt->second.end(); )
	{
		if (it->second.IsExpired(now))
		{
			it = nsIt->second.erase(it);
		}
		else
		{
			++activeCount;
			++it;
		}
	}

	return activeCount;
}

// --------------------------------------------------
// FEATURE B: KEY ENUMERATION / ITERATION
// --------------------------------------------------
bool MemStoreManager::GetKeyAt(const std::string &ns, size_t index, std::string &keyOut)
{
	auto nsIt = m_Namespaces.find(ns);
	if (nsIt == m_Namespaces.end())
	{
		return false;
	}

	time_t now = time(nullptr);
	size_t current = 0;
	for (auto it = nsIt->second.begin(); it != nsIt->second.end(); )
	{
		if (it->second.IsExpired(now))
		{
			it = nsIt->second.erase(it);
		}
		else
		{
			if (current == index)
			{
				keyOut = it->first;
				return true;
			}
			++current;
			++it;
		}
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

	time_t now = time(nullptr);
	if (it->second.IsExpired(now))
	{
		nsIt->second.erase(it);
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

	time_t now = time(nullptr);
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

	if (order == RankOrder::Desc)
	{
		// Higher score ranks first; ties broken by earlier sequence
		std::sort(activeEntries.begin(), activeEntries.end(), [](const auto &a, const auto &b) {
			if (a.second.score != b.second.score)
				return a.second.score > b.second.score;
			return a.second.sequence < b.second.sequence;
		});
	}
	else
	{
		// Lower score ranks first (speedrun); ties broken by earlier sequence
		std::sort(activeEntries.begin(), activeEntries.end(), [](const auto &a, const auto &b) {
			if (a.second.score != b.second.score)
				return a.second.score < b.second.score;
			return a.second.sequence < b.second.sequence;
		});
	}

	for (size_t i = 0; i < activeEntries.size(); ++i)
	{
		if (activeEntries[i].first == key)
		{
			return static_cast<int32_t>(i + 1); // 1-based position
		}
	}

	return 0;
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

	time_t now = time(nullptr);
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

	if (static_cast<size_t>(rankPos) > activeEntries.size())
	{
		return false;
	}

	if (order == RankOrder::Desc)
	{
		std::sort(activeEntries.begin(), activeEntries.end(), [](const auto &a, const auto &b) {
			if (a.second.score != b.second.score)
				return a.second.score > b.second.score;
			return a.second.sequence < b.second.sequence;
		});
	}
	else
	{
		std::sort(activeEntries.begin(), activeEntries.end(), [](const auto &a, const auto &b) {
			if (a.second.score != b.second.score)
				return a.second.score < b.second.score;
			return a.second.sequence < b.second.sequence;
		});
	}

	keyOut = activeEntries[rankPos - 1].first;
	scoreOut = activeEntries[rankPos - 1].second.score;
	return true;
}

size_t MemStoreManager::RankGetCount(const std::string &ns)
{
	auto nsIt = m_Ranks.find(ns);
	if (nsIt == m_Ranks.end())
	{
		return 0;
	}

	time_t now = time(nullptr);
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

void MemStoreManager::SweepMapEnd()
{
	time_t now = time(nullptr);

	// 1. Sweep regular key-values
	for (auto nsIt = m_Namespaces.begin(); nsIt != m_Namespaces.end(); )
	{
		auto &keyMap = nsIt->second;
		for (auto keyIt = keyMap.begin(); keyIt != keyMap.end(); )
		{
			auto &entry = keyIt->second;

			if (entry.policy == ExpirePolicy::MapEnd)
			{
				keyIt = keyMap.erase(keyIt);
			}
			else if (entry.policy == ExpirePolicy::MapCount)
			{
				--entry.mapCounter;
				if (entry.mapCounter <= 0)
				{
					keyIt = keyMap.erase(keyIt);
				}
				else
				{
					++keyIt;
				}
			}
			else if (entry.policy == ExpirePolicy::TTL)
			{
				if (now >= entry.expireTimestamp)
				{
					keyIt = keyMap.erase(keyIt);
				}
				else
				{
					++keyIt;
				}
			}
			else
			{
				// ExpirePolicy::Persistent stays intact!
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

	// 2. Sweep rank namespaces
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
				if (now >= entry.expireTimestamp)
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
	m_RankSequenceCounter = 0;
}
