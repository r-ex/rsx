#include <pch.h>
#include <core/cache/cachedb.h>
//#include <mutex>
#include <game/rtech/utils/utils.h>

CCacheDBManager g_cacheDBManager;

bool CCacheDBManager::SaveToFile(const std::string& path)
{
	HANDLE fileHandle = WaitForFileHandle(250u, 10000u, path.c_str(), (GENERIC_READ | GENERIC_WRITE), FILE_SHARE_READ, NULL, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);

	if (fileHandle == INVALID_HANDLE_VALUE)
	{
		Log("CACHE: Failed to get a file handle after waiting, cache file will not be updated...\n");
		return false;
	}

	// [rika]: update cache if crc is different
	const size_t currFileCRC = LoadCRCFromFile(path);
	if (currFileCRC > 0 && currFileCRC != m_sourceCRC)
	{
		// [rika]: update cache file from the one on disk
		LoadFromFile(path);
		Log("CACHE: Reloaded cache file after being updated by another process...\n");
	}

	// acquire total string size for a buffer
	size_t stringBufSize = 1ull; // we always have a null terminator
	for (const auto& it : m_cacheEntries)
	{
		const CCacheEntry& entry = it.second;
		stringBufSize += entry.origString.length() + 1; // add one for null terminator

		if (!entry.fileName.empty())
		{
			stringBufSize += entry.fileName.length() + 1;
		}
	}

	const uint32_t numMappings = static_cast<uint32_t>(m_cacheEntries.size());

	const size_t fileSize = sizeof(CacheDBHeader_t) + (numMappings * sizeof(CacheHashMapping_t)) + stringBufSize;
	char* const fileBuf = new char[fileSize]{};

	CacheDBHeader_t* const hdr = reinterpret_cast<CacheDBHeader_t* const>(fileBuf);
	CacheHashMapping_t* mappings = reinterpret_cast<CacheHashMapping_t*>(&hdr[1]);
	char* strings = reinterpret_cast<char*>(mappings + numMappings);

	uint32_t stringOffset = 1u; // skipping the null terminator
	strings += stringOffset;
	for (const auto& it : m_cacheEntries)
	{
		const CCacheEntry& entry = it.second;
		mappings->guid = entry.guid;

		// write the asset name
		{
			const uint32_t strLength = static_cast<uint32_t>(entry.origString.length());
			mappings->strOffset = stringOffset;
			strncpy_s(strings, stringBufSize - stringOffset, entry.origString.c_str(), strLength);

			strings += strLength + 1;
			stringOffset += strLength + 1;
		}

		// write the pakfile name
		if (!entry.fileName.empty())
		{
			const uint32_t strLength = static_cast<uint32_t>(entry.fileName.length());
			mappings->fileNameOffset = stringOffset;
			strncpy_s(strings, stringBufSize - stringOffset, entry.fileName.c_str(), strLength);

			strings += strLength + 1;
			stringOffset += strLength + 1;
		}

		mappings++;
	}

	hdr->fileVersion = CACHE_DB_FILE_VERSION;
	hdr->fileCRC = crc32::byteLevel(reinterpret_cast<const uint8_t*>(&hdr[1]), fileSize - sizeof(CacheDBHeader_t));
	hdr->numMappings = numMappings;
	hdr->stringTableOffset = sizeof(CacheDBHeader_t) + (sizeof(CacheHashMapping_t) * numMappings);

	FILE* file = FileFromHandle(fileHandle, eStreamIOMode::Write);
	StreamIO cacheFile(file, eStreamIOMode::Write);

	cacheFile.write(fileBuf, fileSize);
	cacheFile.close(); // closes file handle

	FreeAllocArray(fileBuf);

	return true;
}

bool CCacheDBManager::LoadFromFile(const std::string& path)
{
	if (!std::filesystem::exists(path))
	{
		// if the file doesn't exist yet, save the file immediately with no contents
		// so that there is a base file to build off
		this->SaveToFile(path);
		return true;
	}

	StreamIO cacheFile(path, eStreamIOMode::Read);

	const uint64_t cacheFileSize = cacheFile.size();

	const char* fileData = new char[cacheFileSize];
	cacheFile.read(const_cast<char*>(fileData), cacheFileSize);

	const CacheDBHeader_t* header = reinterpret_cast<const CacheDBHeader_t*>(fileData);

	switch (header->fileVersion)
	{
	case CACHE_DB_FILE_VERSION:
	{
		break;
	}
	case 1:
	{
		Log("CACHE: CacheDB file was old version: \"%s\". Upgrading file...\n", path.c_str());

		fileData = UpgradeLegacyFile_V1(path, fileData, cacheFileSize);
		header = reinterpret_cast<const CacheDBHeader_t*>(fileData);

		break;
	}
	default:
	{
		Log("CACHE: Failed to load CacheDB file: \"%s\". Invalid version\n", path.c_str());
		return false;
	}
	}

	const uint32_t fileCRC = crc32::byteLevel(reinterpret_cast<const uint8_t*>(&header[1]), cacheFileSize - sizeof(CacheDBHeader_t));
	if (header->fileCRC != fileCRC)
	{
		Log("CACHE: Failed to load CacheDB file: \"%s\". Invalid CRC\n", path.c_str());
		return false;
	}

	m_sourceCRC = header->fileCRC;

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
	std::unique_lock lock(m_cacheMutex);

	m_cacheEntries.emplace(entry.guid, entry);
}

const uint32_t CCacheDBManager::LoadCRCFromFile(const std::string& path) const
{
	if (!std::filesystem::exists(path) || std::filesystem::file_size(path) == 0ull)
	{
		return 0u;
	}

	StreamIO cacheFile(path, eStreamIOMode::Read);

	CacheDBHeader_t tmp;
	cacheFile.read(tmp);
	cacheFile.close();

	return tmp.fileCRC;
}

const uint32_t CCacheDBManager::ParseCRCFromFile(const std::string& path) const
{
	constexpr size_t headerSize = sizeof(CacheDBHeader_t);

	assertm(std::filesystem::exists(path), "file should exist by this point");
	const size_t fileSize = std::filesystem::file_size(path) - headerSize;

	if (!fileSize)
	{
		return crc32::crc32InitialValue;
	}

	char* const fileBuf = new char[fileSize];

	StreamIO cacheFile(path, eStreamIOMode::Read);
	cacheFile.seek(headerSize);
	cacheFile.read(fileBuf, fileSize);
	cacheFile.close();

	const uint32_t out = crc32::byteLevel(reinterpret_cast<const uint8_t*>(fileBuf), fileSize);

	return out;
}

char* CCacheDBManager::UpgradeLegacyFile_V1(const std::string& path, const char* const fileBuf, const size_t fileBufSize) const
{
	assertm(fileBufSize, "invalid buffer size");

	constexpr size_t newHeaderSize = sizeof(CacheDBHeader_t);
	constexpr size_t oldHeaderSize = sizeof(CacheDBHeader_v1_t);
	constexpr size_t sizeDifference = newHeaderSize - oldHeaderSize;

	const size_t bufSize = fileBufSize + sizeDifference;
	char* buf = new char[bufSize]{};

	CacheDBHeader_t* const newHdr = reinterpret_cast<CacheDBHeader_t* const>(buf);
	const CacheDBHeader_v1_t* const oldHdr = reinterpret_cast<const CacheDBHeader_v1_t* const>(fileBuf);

	memcpy_s(buf + newHeaderSize, bufSize - newHeaderSize, fileBuf + oldHeaderSize, fileBufSize - oldHeaderSize);

	newHdr->fileVersion = CACHE_DB_FILE_VERSION;
	newHdr->fileCRC = crc32::byteLevel(reinterpret_cast<const uint8_t*>(buf) + newHeaderSize, bufSize - newHeaderSize);
	newHdr->numMappings = oldHdr->numMappings;
	newHdr->stringTableOffset = oldHdr->stringTableOffset + sizeDifference;

	FreeAllocArray(fileBuf);

	// [rika]: write the new cache file
	StreamIO cacheFile(path, eStreamIOMode::Write);
	cacheFile.write(buf, bufSize);
	cacheFile.close();

	return buf;
}