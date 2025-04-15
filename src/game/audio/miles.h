#pragma once
#include <game/asset.h>

// Used for offsets that are initially stored as 32-bit values and then converted to
// pointers at runtime
union OffsetPtr_t
{
	// The offset is actually 32-bit, but we might as well use a 64-bit type here
	// since the space is reserved
	size_t offset;
	void* ptr;
};

struct MilesASIUserData_t
{
	StreamIO* streamReader;
	uint64_t dataRead;
	uint64_t headerSize;
	uint64_t audioStreamOffset;
	uint64_t audioStreamSize;
};

// correct as of s22 apex
struct MilesASIDecoder_t
{
	int unk0;
	int decoderType;
	void* ASI_stream_parse_metadata; // 8
	void* ASI_open_stream; // 16
	void* ASI_notify_seek; // 24
	void* ASI_stream_seek_to_frame; // 32
	void* ASI_stream_seek_direct; // 40
	void* ASI_decode_block; // 48
	void* ASI_get_block_size; // 56
	void* ASI_unk_dealloc_maybe; // 64
};

typedef uint32_t(*ASI_read_stream_f)(char*, uint64_t, void*); // buffer, readAmount, userData
typedef bool(*ASI_parse_metadata_f)(void*, size_t, uint16_t*, uint32_t*, uint32_t*, int*, uint32_t*);
typedef uint8_t(*ASI_open_stream_f)(void*, size_t*, void*, void*);
typedef void(*ASI_notify_seek_f)(void*);
typedef size_t(*ASI_stream_seek_to_frame_f)(void*, size_t, size_t*, uint32_t*);
typedef size_t(*ASI_stream_seek_direct_f)(const char*, size_t, size_t, size_t*, size_t*);
typedef size_t(*ASI_decode_block_f)(void*, const char*, size_t, void*, size_t, uint32_t*, uint32_t*);
typedef void(*ASI_get_block_size_f)(void*, const char*, size_t, uint32_t*, uint32_t*, uint32_t*);


// it seems that this struct has never changed.... yet...
struct MilesStreamHeader_t
{
	uint32_t magic;
	uint16_t version;
	uint16_t languageIdx;

	uint32_t streamDataOffset;
	int patchIdx;

	uint32_t buildTag; // must match with the associated MBNK file
};

// s0
struct MilesSource_v28_t
{
	char gap0[12];
	uint16_t languageIdx; // sound language ID
	uint16_t patchIdx; // index of the patch file that contains this sound
	uint32_t nameOffset; // relative to string table
	uint16_t sampleRate;
	uint16_t bitRate;

	char gap_18[2];
	uint8_t channelCount;
	char gap_1B[21];

	uint32_t streamHeaderSize;
	uint32_t streamDataSize;
	uint64_t streamHeaderOffset;
	uint64_t streamDataOffset;
	char gap_end[16];
};
static_assert(offsetof(MilesSource_v28_t, channelCount) == 26);
static_assert(offsetof(MilesSource_v28_t, streamHeaderSize) == 48);
static_assert(sizeof(MilesSource_v28_t) == 0x58);

// s22
struct MilesSource_v39_t
{
	uint32_t nameOffset; // relative to string table
	uint16_t languageIdx; // sound language ID
	uint16_t patchIdx; // index of the patch file that contains this sound
	uint32_t unk8;
	uint16_t sampleRate;
	uint16_t bitRate;

	char gap[16];

	uint32_t streamHeaderSize;
	uint32_t streamDataSize;
	uint64_t streamHeaderOffset;
	uint64_t streamDataOffset;
	uint64_t minusOne;
	char gap8[8];
};
static_assert(offsetof(MilesSource_v39_t, streamDataOffset) == 48);
static_assert(sizeof(MilesSource_v39_t) == 72);

struct MilesSource_t
{
	MilesSource_t(const MilesSource_v39_t* const a) :
		streamDataOffset(a->streamDataOffset), streamHeaderOffset(a->streamHeaderOffset),
		streamDataSize(a->streamDataSize), streamHeaderSize(a->streamHeaderSize),
		nameOffset(a->nameOffset),
		languageIdx(a->languageIdx), patchIdx(a->patchIdx)
	{};

	MilesSource_t(const MilesSource_v28_t* const a) :
		streamDataOffset(a->streamDataOffset), streamHeaderOffset(a->streamHeaderOffset),
		streamDataSize(a->streamDataSize), streamHeaderSize(a->streamHeaderSize),
		nameOffset(a->nameOffset),
		languageIdx(a->languageIdx), patchIdx(a->patchIdx)
	{};

	uint64_t streamDataOffset;
	uint64_t streamHeaderOffset;
	uint32_t streamDataSize;
	uint32_t streamHeaderSize;
	uint32_t nameOffset;

	uint16_t languageIdx;
	uint16_t patchIdx;

};

/*
MBNK Versions:

11 - Titanfall 2 Tech Test
13 - Titanfall 2 Release
28 - Apex Legends Release    -> Season 2.?
32 - Apex Legends Season 2.? -> Season 3.1
33 - Apex Legends Season 3.1 -> Season 4.1(?)
34 - Apex Legends Season 4.1 -> Season 5.1(?)
36 - Apex Legends Season 5.1 -> Season 6.1
38 - Apex Legends Season 7.0 -> Season 7.2
39 - Apex Legends Season 8.0 -> Season 8.1
40 - Apex Legends Season 9.0 -> Season 18.0
42 - Apex Legends Season 18.1
43 - Apex Legends Season 19.0 -> Season 19.1
44 - Apex Legends Season 20.0
45 - Apex Legends Season 20.1 -> Season 23.0
46 - Apex Legends Season 23.1 -> Season 24.1
*/


struct MilesBankHeaderShort_t
{
	int magic;
	int version;
	uint32_t dataSize;
};

struct MilesBankHeader_v28_t
{
	int magic;
	int version;
	uint32_t fileSize;

	int bankMagic;
	uint32_t unk_10;
	uint32_t unk_14;

	uint64_t reserved_project;
	char unk_20[16];

	OffsetPtr_t unk_offset_30;
	OffsetPtr_t unk_offset_38;
	OffsetPtr_t unk_offset_40;
	OffsetPtr_t sourceOffset;
	OffsetPtr_t localisedSourceOffset; // reserved
	OffsetPtr_t unk_offset_58;
	OffsetPtr_t eventOffset;
	OffsetPtr_t unk_offset_68;
	OffsetPtr_t stringTableOffset;
	OffsetPtr_t unk_offset_78;
	OffsetPtr_t unk_offset_80;
	OffsetPtr_t unk_offset_88;

	uint32_t unk_90;
	uint32_t localisedSourceCount;
	uint32_t sourceCount;
	uint32_t patchCount;
	uint32_t eventCount;

	char gap_a4[12];
	uint32_t buildTag;
	char gap_a8[12];
};
static_assert(offsetof(MilesBankHeader_v28_t, eventCount) == 0xa0);
static_assert(sizeof(MilesBankHeader_v28_t) == 0xc0);

//struct MilesBankHeader_v39_t
//{
//	int magic;
//	int version;
//	uint32_t fileSize;
//	int bankMagic;
//	uint32_t buildTag;
//	char gap_14[52];
//	OffsetPtr_t sourceIdsOffset;
//	OffsetPtr_t sourceOffset;
//	OffsetPtr_t localisedSourceOffset;
//	OffsetPtr_t unk_offset_60;
//	OffsetPtr_t unk_offset_68;
//	OffsetPtr_t eventOffset;
//	OffsetPtr_t unk_offset_78;
//	OffsetPtr_t stringTableOffset;
//	char gap_88[28];
//	uint32_t localisedSourceCount;
//	uint32_t sourceCount;
//	uint32_t patchCount;
//	uint32_t eventCount;
//	char gap_B4[20];
//};

struct MilesBankHeader_v45_t
{
	int magic;
	int version;
	uint32_t fileSize;

	int bankMagic;
	uint32_t buildTag;
	uint32_t bankHash;

	OffsetPtr_t bankNameOffset;

	// all zero
	uint64_t reserved_project; // reserved for a pointer to an internal project structure
	char unk28[16];

	OffsetPtr_t unk_offset_38;
	OffsetPtr_t unk_offset_40;
	OffsetPtr_t unk_offset_48; // SourceTableOffset
	OffsetPtr_t sourceOffset; // SourceEntryOffset
	OffsetPtr_t localisedSourceOffset; // reserved. null on disk
	OffsetPtr_t unk_offset_60;
	OffsetPtr_t eventOffset;
	OffsetPtr_t unk_offset_70;
	OffsetPtr_t unk_offset_78;
	OffsetPtr_t stringTableOffset;
	OffsetPtr_t unk_offset_88;
	OffsetPtr_t unk_offset_90;
	OffsetPtr_t unk_offset_98;

	uint32_t unk_a0;
	uint32_t localisedSourceCount; // localised source count?
	uint32_t sourceCount; // unlocalised source count

	// patchCount
	// this value determines how many *_patch_X.mstr files there need to be
	// for this bank to load correctly.
	// requires: general_stream_patch_1.mstr -> general_stream_patch_(patchCount-1).mstr to be present
	// and the equivalent language streams
	uint32_t patchCount;

	uint32_t eventCount;
	uint32_t unk_b4;
	uint32_t unk_b8;
	uint32_t unk_bc;
	uint32_t unk_c0;
	uint32_t unk_c4;
};

static_assert(offsetof(MilesBankHeader_v45_t, unk_offset_38) == 0x38);

class CMilesAudioBank : public CAssetContainer
{
public:
	CMilesAudioBank() {};
	~CMilesAudioBank() = default;

	const CAsset::ContainerType GetContainerType() const
	{
		return CAsset::ContainerType::AUDIO;
	}

	const bool ParseFile(const std::string& path);

	const std::string& GetFilePath() const { return m_filePath; }

	// the base name for the bank is always at the start of the string table
	const char* GetBankStem() const { return stringTable; };

	const std::vector<const char*>& GetLanguageNames() const { return m_languageNames; }

	std::string GetStreamingFileNameForSource(const MilesSource_t* source) const;

	const char* GetString(uint32_t offset) const
	{
		return reinterpret_cast<const char*>(stringTable) + offset;
	}

	bool IsValidSource(const MilesSource_t* source) const;
private:

	void DiscoverStreamingFiles();

	const bool ParseFromHeader();

	// Maps a language index to a bitfield that indicates if
	// the corresponding patch stream files exist.
	std::map<uint16_t, uint32_t> m_localisedStreamStates;
	uint32_t m_streamStates;

	std::string m_filePath;

	std::shared_ptr<char[]> m_fileBuf;

	std::vector<const char*> m_languageNames;

	uint32_t buildTag;
	uint32_t bankHash;

	uint32_t sourceCount;
	uint32_t eventCount;

	uint32_t languageCount; // not derived from the bank itself but useful info
	uint32_t localisedSourceCount;

	void* audioSources;
	void* audioEvents;
	const char* stringTable;


	int m_version;

	void Construct(const MilesBankHeader_v28_t* const header)
	{
		this->buildTag = header->buildTag;
		//this->bankHash = header->bankHash; // not sure if this var exists in v28

		// total source count including all languages
		this->sourceCount = header->sourceCount + (header->localisedSourceCount * (this->languageCount - 1));
		this->eventCount = header->eventCount;

		this->localisedSourceCount = header->localisedSourceCount;

		this->audioSources = m_fileBuf.get() + header->sourceOffset.offset;
		this->audioEvents = m_fileBuf.get() + header->eventOffset.offset;
		this->stringTable = m_fileBuf.get() + header->stringTableOffset.offset;
	}

	void Construct(const MilesBankHeader_v45_t* const header)
	{
		this->buildTag = header->buildTag;
		this->bankHash = header->bankHash;

		// total source count including all languages
		this->sourceCount = header->sourceCount + (header->localisedSourceCount * (this->languageCount - 1));
		this->eventCount = header->eventCount;

		this->localisedSourceCount = header->localisedSourceCount;
		
		this->audioSources = m_fileBuf.get() + header->sourceOffset.offset;
		this->audioEvents = m_fileBuf.get() + header->eventOffset.offset;
		this->stringTable = m_fileBuf.get() + header->stringTableOffset.offset;
	}
};

class CMilesAudioAsset : public CAsset
{
public:
	CMilesAudioAsset(const std::string& assetName, void* assetData, CMilesAudioBank* bank)
	{
		SetAssetName(assetName);
		m_assetGuid = 0;
		
		SetAssetVersion({});

		SetInternalAssetData(assetData);
		SetContainerFile(bank);
	}

	~CMilesAudioAsset()
	{
		delete m_assetData;
	};

	void SetContainerName(const std::string& containerName)
	{
		m_containerName = containerName;
	}

	void SetAssetGUID(uint64_t guid)
	{
		m_assetGuid = guid;
	}

	void SetAssetType(uint32_t type)
	{
		m_assetType = type;
	}

	uint32_t GetAssetType() const
	{
		return m_assetType;
	}

	const uint64_t GetAssetGUID() const
	{
		return m_assetGuid;
	}

	const ContainerType GetAssetContainerType() const
	{
		return ContainerType::AUDIO;
	};

	std::string GetContainerFileName() const
	{
		return m_containerName;
	}

	const void* GetInternalAssetData()
	{
		return m_assetData;
	}

private:
	std::string m_containerName;

	uint64_t m_assetGuid;
	uint32_t m_assetType;
};

// Decoders
MilesASIDecoder_t* GetRadAudioDecoder();
