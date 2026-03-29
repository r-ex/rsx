#pragma once

#include <shared_mutex>

#define RSX_CACHE_DB_FILENAME "rsx_cache_db.bin"

// v1: intial revision
// v2: adds crc to header
constexpr int CACHE_DB_FILE_VERSION = 2;
constexpr size_t maxCacheFileSize = 1024 * 1024 * 32;

#pragma pack(push, 1)
struct CacheDBHeader_t
{
	uint32_t fileVersion; // doesnt need to be 32-bit but it'll get padded to it anyway
	uint32_t fileCRC;
	uint32_t numMappings;

	uint32_t reserved_0;

	uint64_t stringTableOffset;

	uint32_t reserved_1[2];

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

// legacy structs
struct CacheDBHeader_v1_t
{
	uint32_t fileVersion; // doesnt need to be 32-bit but it'll get padded to it anyway

	uint32_t numMappings; // mappings immediately follow the header

	uint64_t stringTableOffset;

	const char* GetString(uint64_t offset) const
	{
		return reinterpret_cast<const char*>(this) + stringTableOffset + offset;
	}
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

	std::optional<CCacheEntry> TryGetEntry(const uint64_t guid) const
	{
		std::shared_lock lock(m_cacheMutex);

		auto iter = m_cacheEntries.find(guid);
		if (iter == m_cacheEntries.end())
			return std::nullopt;

		return iter->second;
	}

	void Add(const std::string& str);

	void Add(const CCacheEntry& entry);

	void Clear()
	{
		m_cacheEntries.clear();
	}

private:
	void AddInternal(const CCacheEntry& entry);

	const uint32_t LoadCRCFromFile(const std::string& path) const;
	const uint32_t ParseCRCFromFile(const std::string& path) const;
	char* UpgradeLegacyFile_V1(const std::string& path, const char* const fileBuf, const size_t fileBufSize) const;

private:
	std::unordered_map<uint64_t, CCacheEntry> m_cacheEntries;

	mutable std::shared_mutex m_cacheMutex;

	uint32_t m_sourceCRC;
};

extern CCacheDBManager g_cacheDBManager;