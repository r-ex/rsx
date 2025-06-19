#include "pch.h"
#include "cachedb.h"
#include <mutex>
#include <game/rtech/utils/utils.h>

CCacheDBManager g_cacheDBManager;

bool CCacheDBManager::SaveToFile(const std::string& path)
{
	CacheDBHeader_t header = {};

	header.fileVersion = CACHE_DB_FILE_VERSION;
	header.numMappings = static_cast<uint32_t>(m_cacheEntries.size());

	StreamIO cacheFile(path, eStreamIOMode::Write);

	// Initialise the string table with one empty string.
	// This makes sure that any mapping entries that aren't associated
	std::vector<std::string> stringTableEntries = { "" };
	uint64_t nextStringOffset = 1;

	cacheFile.write(header);

	for (auto& entry : m_cacheEntries)
	{
		CacheHashMapping_t mapping = {};
		mapping.guid = entry.second.guid;
		mapping.strOffset = static_cast<uint32_t>(nextStringOffset);

		stringTableEntries.push_back(entry.second.origString);
		
		nextStringOffset += entry.second.origString.length() + 1;

		if (!entry.second.fileName.empty())
		{
			stringTableEntries.push_back(entry.second.fileName);

			mapping.fileNameOffset = static_cast<uint32_t>(nextStringOffset);

			nextStringOffset += entry.second.fileName.length() + 1;
		}

		cacheFile.write(mapping);
	}

	// time to go back and write the string table offset since we are about to
	// write some strings!
	header.stringTableOffset = cacheFile.tell();

	for (auto& it : stringTableEntries)
	{
		cacheFile.write(it.c_str(), it.length());
		
		// leave a null terminator!
		const uint8_t nt = 0;
		cacheFile.write(nt);
	}

	cacheFile.seek(0);
	cacheFile.write(header);
	cacheFile.close();

	return true;
}

bool CCacheDBManager::LoadFromFile(const std::string& path)
{
	UNUSED(path);

	if (!std::filesystem::exists(path))
	{
		// if the file doesn't exist yet, save the file immediately with no contents
		// so that there is a base file to build off
		this->SaveToFile(path);
		return true;
	}

	StreamIO cacheFile(path, eStreamIOMode::Read);

	const uint64_t cacheFileSize = cacheFile.size();

	const char* const fileData = new char[cacheFileSize];
	cacheFile.read(const_cast<char*>(fileData), cacheFileSize);

	const CacheDBHeader_t* header = reinterpret_cast<const CacheDBHeader_t*>(fileData);

	if (header->fileVersion != CACHE_DB_FILE_VERSION)
	{
		Log("CACHE: Failed to load CacheDB file: \"%s\". Invalid version\n", path.c_str());
		return false;
	}

	const CacheHashMapping_t* mappings = reinterpret_cast<const CacheHashMapping_t*>(&header[1]);

	for (uint32_t i = 0; i < header->numMappings; ++i)
	{
		const CacheHashMapping_t* mapping = &mappings[i];

		CCacheEntry entry = {};
		entry.guid = mapping->guid;
		entry.origString = header->GetString(mapping->strOffset);
		entry.fileName = header->GetString(mapping->fileNameOffset);

		AddInternal(entry);
	}


	delete[] fileData;
	cacheFile.close();

	return true;
}

void CCacheDBManager::Add(const std::string& str)
{
	const uint64_t guid = RTech::StringToGuid(str.c_str());

	AddInternal(CCacheEntry{ guid, str });
}

void CCacheDBManager::Add(const CCacheEntry& entry)
{
	CCacheEntry newEntry = entry;

	// do not trust that the guid has been provided by the caller
	// recalculate just in case! the cost of recalculating the guid is worth it for
	// the assurance that the cache db is consistent.
	newEntry.guid = RTech::StringToGuid(newEntry.origString.c_str());

	AddInternal(newEntry);
}

void CCacheDBManager::AddInternal(const CCacheEntry& entry)
{
	std::lock_guard lock(m_cacheMutex);

	m_cacheEntries.emplace(entry.guid, entry);
}