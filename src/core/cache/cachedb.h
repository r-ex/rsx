#pragma once

constexpr int CACHE_DB_FILE_VERSION = 1;

#pragma pack(push, 1)
struct CacheDBHeader_t
{
	uint32_t fileVersion; // doesnt need to be 32-bit but it'll get padded to it anyway
	
	uint32_t numMappings; // mappings immediately follow the header

	uint64_t stringTableOffset;

	const char* GetString(uint64_t offset) const
	{
		return reinterpret_cast<const char*>(this) + stringTableOffset + offset;
	}
};

struct CacheHashMapping_t
{
	uint64_t guid;
	uint32_t strOffset; // offset relative to stringTableOffset in the cache file for this asset's string name
	
	// If this mapping relates to an asset, this offset will point to a string for where the asset can be found
	// This allows RSX to automatically load the container file that holds any missing dependency assets.
	uint32_t fileNameOffset; 
};
#pragma pack(pop)

// cache entry struct for when stored in memory
struct CCacheEntry
{
	uint64_t guid;
	std::string origString; // original string that was used to generate the associated guid
	std::string fileName;
};

class CCacheDBManager
{
public:
	bool SaveToFile(const std::string& path);
	bool LoadFromFile(const std::string& path);

	bool LookupGuid(const uint64_t guid, CCacheEntry* const outEntry) const
	{
		const bool foundGuid = m_cacheEntries.contains(guid);

		// must copy! if an asset calls CCacheDBManager::Add from another thread
		// while LookupGuid is being called, we end up with UB from a bad pointer
		if (foundGuid)
			*outEntry = m_cacheEntries.at(guid);

		return foundGuid;
	}

	void Add(const std::string& str);

	void Add(const CCacheEntry& entry);

	void Clear()
	{
		m_cacheEntries.clear();
	}

private:
	void AddInternal(const CCacheEntry& entry);

private:
	std::unordered_map<uint64_t, CCacheEntry> m_cacheEntries;

	std::mutex m_cacheMutex;
};

extern CCacheDBManager g_cacheDBManager;