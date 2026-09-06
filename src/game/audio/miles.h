#pragma once
#include <game/asset.h>
#include <misc/imgui_utility.h>

constexpr int MILES_DECODER_BINKA = 2;
constexpr int MILES_DECODER_RADA = 6;

constexpr int DECODE_FORMAT_F32 = 0b10;

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
	void* ASI_stream_seek_to_frame; // 32 - not used
	void* ASI_stream_seek_direct; // 40 - not used
	void* ASI_decode_block; // 48
	void* ASI_get_block_size; // 56
	void* ASI_dealloc; // 64 - not used
};

typedef uint32_t(*ASI_read_stream_f)(char*, uint64_t, void*); // buffer, readAmount, userData
typedef bool(*ASI_parse_metadata_f)(void*, size_t, uint16_t*, uint32_t*, uint32_t*, int*, uint32_t*);
typedef uint8_t(*ASI_open_stream_f)(void*, size_t*, void*, void*);
typedef void(*ASI_notify_seek_f)(void*);
typedef size_t(*ASI_stream_seek_to_frame_f)(void*, size_t, size_t*, uint32_t*);
typedef size_t(*ASI_stream_seek_direct_f)(const char*, size_t, size_t, size_t*, size_t*);
typedef size_t(*ASI_decode_block_f)(void*, const char*, size_t, void*, size_t, uint32_t*, uint32_t*);
typedef void(*ASI_get_block_size_f)(void*, const char*, size_t, uint32_t*, uint32_t*, uint32_t*);

typedef void(*ASI_dealloc_f)(void*);

struct MilesAudioMarker_t
{
	uint32_t nameOffset;
	uint32_t framePosition;
	char unk_8[8];
};

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

struct EventName_s
{
	uint32_t nameOffset;
	uint32_t dataOffset; // relative to event actions data
};

inline float MilesLinearToDb(float value)
{
	if (value <= 0.f || value <= 0.000099999997)
		return -96.f;
	
	return log(value) * 14.4764827301f;
}

inline float MilesLevelToDb(float value)
{
	if (value <= 0.f || value <= 0.000099999997)
		return -96.0;

	return log10(value) * 20.f;
}

inline float ConvertGraphRange(bool invert, uint8_t type, float fraction)
{
	if (invert) fraction = 1.f - fraction;

	switch (type)
	{
	case 3: // sqr
	{
		fraction = fraction * fraction;
		break;
	}
	case 4: // dB
	{
		fraction = (MilesLinearToDb(fraction) + 96.f) / 96.f;
		break;
	}
	case 5:
	{
		fraction = (MilesLevelToDb(sqrtf(fraction)) + 96.f) / 96.f;
		break;
	}
	case 6:
	{
		fraction = fraction * fraction * fraction;
		break;
	}
	}

	if (fraction < 0.0000023841858)
		fraction = 0.f;

	fraction = fminf(1.f, fraction);

	return invert ? (1 - fraction) : fraction;
}

/*
	Graph Data

	At the graphData pointer in a MBNK file, there is the following structure:

	u8 numValues;
	u8 unk;
	u16 unk;
	u32 unkStrOffset;
*/
// valid in r2 and r5
struct MilesGraphSegment_s
{
	float start; // LHS
	float end; // RHS (the start value of the next segment, or == start if this is the final segment)

	float paramA;
	float paramB;
	char type;

	char pad[3]; // can't find what (if anything) these do

	float Sample(float x, float min, float max) const
	{
		if (type == 2)
			return start;

		const float range = max - min;

		if (range < 0.0000023841858f)
			return start;

		const float delta = x - min;

		float frac = delta / range;

		if (frac < 0.0000023841858f)
			return start;

		if (frac >= 1.f)
			return end;

		// linear
		switch (type)
		{
		case 0:
			return std::lerp(start, end, frac);
		case 1:
		{
			// paramA/B are angles for the start/end control handles
			const float sinA = sinf(paramA);
			const float cosA = cosf(paramA);

			const float sinB = sinf(paramB);
			const float cosB = cosf(paramB);

			const float segmentRange = end - start;

			float v17 = ((sinA / 3.f) + start) - start;
			float v19 = v17 / (((cosA / 3.f) + min) - min);
			float v21 = end - (end - (sinB / 3.f));

			return (float)((float)((float)((float)((float)((float)((float)((float)((float)((float)(segmentRange + segmentRange)
				+ segmentRange)
				- (float)(v19 * range))
				- (float)(v19 * range))
				- (float)((float)(v21
					/ (float)(max
						- (float)(max - (float)(cosB / 3.0))))
					* range))
				* (float)(1.0 / (float)(range * range)))
				+ (float)((float)((float)((float)((float)((float)((float)((float)(v21 / (float)(max - (float)(max - (float)(cosB / 3.0))))
					* range)
					+ (float)(v19 * range))
					- segmentRange)
					- segmentRange)
					* (float)(1.0 / (float)(range * range)))
					/ range)
					* delta))
				* delta)
				+ v19)
				* delta)
				+ start;

			break;
		}
		case 3:
		case 4:
		case 5:
		case 6:
		{
			// start/end override?
			if (paramA != 0.f || paramB != 0.f)
				frac = std::lerp(paramA, paramB, frac);

			float adjustedFraction = ConvertGraphRange(type & 0x10, type & 0xF, frac);

			if (paramA != 0.f || paramB != 0.f)
			{
				float a = ConvertGraphRange(type & 0x10, type & 0xF, paramA);
				float b = ConvertGraphRange(type & 0x10, type & 0xF, paramB);

				adjustedFraction = (a - adjustedFraction) / (a - b);
			}

			return ((end - start) * adjustedFraction) + start;
		}
		default:
			return start;
		}
	}

};
static_assert(sizeof(MilesGraphSegment_s) == 20);

struct MilesValueGraph_s
{
	char numPoints;
	char unk_1;
	uint16_t unk_2;
	uint32_t baseControllerNameOffset;

	// x-axis values for each point on the graph
	const float* XValues() const
	{
		return reinterpret_cast<const float*>(&this[1]);
	}

	const MilesGraphSegment_s* Segments() const
	{
		return reinterpret_cast<const MilesGraphSegment_s*>(XValues() + numPoints);
	}

	const size_t Size() const
	{
		return sizeof(MilesValueGraph_s) + (numPoints * sizeof(float)) + (numPoints * sizeof(MilesGraphSegment_s));
	}

	const std::pair<Vector2D, Vector2D> MinsMaxs() const
	{
		Vector2D min(FLT_MAX, FLT_MAX);
		Vector2D max(FLT_MIN, FLT_MIN);
		for (int i = 0; i < numPoints; ++i)
		{
			float x = XValues()[i];
			float y = Segments()[i].start;

			if (x < min.x) min.x = x;
			if (y < min.y) min.y = y;

			if (x > max.x) max.x = x;
			if (y > max.y) max.y = y;
		}

		return { min, max };
	}
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
	uint32_t fileSize;
};

// largely taken from RoyalBlue's Kilometer
struct MilesBankHeader_v13_t
{
	int magic;
	int version;
	uint32_t fileSize;

	int bankMagic;
	
	OffsetPtr_t bankName;

	uint64_t nextBank;

	uint64_t sharedState;
	uint64_t bankLoader;

	OffsetPtr_t loadedStreamFiles; // runtime only?
	OffsetPtr_t loadedLanguageStreamFiles; // runtime only?
	OffsetPtr_t sourceNames;

	OffsetPtr_t sources;
	OffsetPtr_t localisedSources;
	OffsetPtr_t markers;

	OffsetPtr_t eventNames;
	OffsetPtr_t eventData;
	OffsetPtr_t strings;
	OffsetPtr_t graphData;
	OffsetPtr_t sourceSelectors;
	OffsetPtr_t filters;
	OffsetPtr_t unk_90;

	uint32_t unk_98;

	uint32_t localisedSourceCount;
	uint32_t sourceCount;
	uint32_t patchCount;
	uint32_t eventCount;
	uint32_t stringsSize;
	uint32_t dword_B0;
	uint32_t graphDataSize;
	uint32_t unkSize;
	uint32_t buildTag;
};

static_assert(offsetof(MilesBankHeader_v13_t, sources) == 0x48);
static_assert(offsetof(MilesBankHeader_v13_t, markers) == 0x58);
static_assert(offsetof(MilesBankHeader_v13_t, eventData) == 0x68);
static_assert(offsetof(MilesBankHeader_v13_t, strings) == 0x70);
static_assert(offsetof(MilesBankHeader_v13_t, unk_90) == 0x90);
static_assert(offsetof(MilesBankHeader_v13_t, sourceCount) == 0xA0);
static_assert(offsetof(MilesBankHeader_v13_t, dword_B0) == 0xB0);
static_assert(offsetof(MilesBankHeader_v13_t, buildTag) == 0xBC);

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
	OffsetPtr_t sources;
	OffsetPtr_t localisedSourceOffset; // reserved
	OffsetPtr_t unk_offset_58;
	OffsetPtr_t eventNames;
	OffsetPtr_t eventData;
	OffsetPtr_t strings;
	OffsetPtr_t graphData;
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
	OffsetPtr_t sources; // SourceEntryOffset
	OffsetPtr_t localisedSourceOffset; // reserved. null on disk
	OffsetPtr_t unk_offset_60;
	OffsetPtr_t eventNames;
	OffsetPtr_t eventData;
	OffsetPtr_t unk_offset_78;
	OffsetPtr_t strings;
	OffsetPtr_t graphData;
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


struct MilesBankHeader_v49_t
{
	int magic;
	int version;
	uint32_t fileSize;

	int bankMagic;
	uint32_t buildTag;
	uint32_t bankHash;

	uint8_t bankIdx;

	char gap_19[3];

	uint32_t eventCount;
		
	uint32_t unk_20[3];

	uint32_t eventDataOffset; // compressed event data
	uint32_t unkOffset;

	uint32_t sources;

	uint32_t sourceCount; // unlocalised
	uint32_t localisedSourceCount;

	char gap_40[8];

	OffsetPtr_t namePtr;

	OffsetPtr_t unk_50;
	OffsetPtr_t audioMarkers;

	char gap_60[8];

	OffsetPtr_t eventNames; // just the names and offset into event data

	OffsetPtr_t strings;
	OffsetPtr_t graphData;

	char gap_80[24];

	void* reserved_memoryBank; // memory bank lmao (pointer to the memory instance that wraps around this file's data)
};

static_assert(offsetof(MilesBankHeader_v49_t, reserved_memoryBank) == 0x98);
static_assert(offsetof(MilesBankHeader_v49_t, graphData) == 0x78);
static_assert(sizeof(MilesBankHeader_v49_t) == 0xA0);

struct MilesSource_t;
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

	int GetVersion() const { return m_version; };
	uint32_t GetSourceCount() const { return sourceCount; };
	uint32_t GetEventCount() const { return eventCount; };


	// the base name for the bank is always at the start of the string table
	const char* GetBankStem() const { return stringTable; };

	const std::vector<const char*>& GetLanguageNames() const { return languageNames; }

	const char* GetString(uint64_t offset) const
	{
		return reinterpret_cast<const char*>(stringTable) + offset;
	}

	template <typename T>
	const T* GetPtr(uint64_t offset) const
	{
		return reinterpret_cast<const T*>(m_fileBuf.get() + offset);
	}

	template <typename T>
	T* GetPtr(uint64_t offset)
	{
		return reinterpret_cast<T*>(m_fileBuf.get() + offset);
	}

	template <typename T>
	const T* GetPtr(const OffsetPtr_t& ptr) const
	{
		return reinterpret_cast<const T*>(m_fileBuf.get() + ptr.offset);
	}

	template <typename T>
	T* GetPtr(const OffsetPtr_t& ptr)
	{
		return reinterpret_cast<T*>(m_fileBuf.get() + ptr.offset);
	}

	const MilesAudioMarker_t* GetMarkers() const
	{
		return audioMarkers;
	}

	const void* GetSourceData() const
	{
		return audioSources;
	}

	const EventName_s* GetEventNamesData() const
	{
		return audioEventNames;
	}

	const void* GetEventActionsData() const
	{
		return audioEventData;
	}

	const void* GetGraphData() const
	{
		return graphData;
	}

	std::string GetStreamingFileNameForSource(const MilesSource_t* source) const;

	bool IsValidSource(const MilesSource_t* source) const;
private:

	void DiscoverStreamingFiles();

	const bool ParseFromHeader();

	// Maps a language index to a bitfield that indicates if
	// the corresponding patch stream files exist.
	std::map<uint16_t, uint32_t> m_localisedStreamStates;
	uint32_t m_streamStates;

	std::shared_ptr<char[]> m_fileBuf;

	std::vector<const char*> languageNames;

	uint32_t buildTag;
	uint32_t bankHash;

	uint32_t sourceCount;
	uint32_t eventCount;

	uint32_t localisedSourceCount;

	void* graphData;
	void* audioSources;
	EventName_s* audioEventNames;
	void* audioEventData;
	MilesAudioMarker_t* audioMarkers;
	const char* stringTable;

	int m_version;


	void Construct(const MilesBankHeader_v13_t* const header)
	{
		this->buildTag = header->buildTag;
		//this->bankHash = header->bankHash; // not sure if this var exists in v28

		// total source count including all languages
		this->sourceCount = header->sourceCount + (header->localisedSourceCount * (static_cast<uint32_t>(this->languageNames.size()) - 1));
		this->eventCount = header->eventCount;

		this->localisedSourceCount = header->localisedSourceCount;

		this->graphData = GetPtr<void>(header->graphData);
		this->audioSources = GetPtr<void>(header->sources);
		this->audioEventNames = GetPtr<EventName_s>(header->eventNames);
		this->audioEventData = GetPtr<void>(header->eventData);
		this->stringTable = GetPtr<char>(header->strings);
	}

	void Construct(const MilesBankHeader_v28_t* const header)
	{
		this->buildTag = header->buildTag;
		//this->bankHash = header->bankHash; // not sure if this var exists in v28

		// total source count including all languages
		this->sourceCount = header->sourceCount + (header->localisedSourceCount * (static_cast<uint32_t>(this->languageNames.size()) - 1));
		this->eventCount = header->eventCount;

		this->localisedSourceCount = header->localisedSourceCount;

		this->graphData = GetPtr<void>(header->graphData);
		this->audioSources = GetPtr<void>(header->sources);
		this->audioEventNames = GetPtr<EventName_s>(header->eventNames);
		this->audioEventData = GetPtr<void>(header->eventData);
		this->stringTable = GetPtr<char>(header->strings);
	}

	void Construct(const MilesBankHeader_v45_t* const header)
	{
		this->buildTag = header->buildTag;
		this->bankHash = header->bankHash;

		// total source count including all languages
		this->sourceCount = header->sourceCount + (header->localisedSourceCount * (static_cast<uint32_t>(this->languageNames.size()) - 1));
		this->eventCount = header->eventCount;

		this->localisedSourceCount = header->localisedSourceCount;

		this->graphData = GetPtr<void>(header->graphData);
		this->audioSources = GetPtr<void>(header->sources);
		this->audioEventNames = GetPtr<EventName_s>(header->eventNames);
		this->audioEventData = GetPtr<void>(header->eventData);
		this->stringTable = GetPtr<char>(header->strings);
	}

	void Construct(const MilesBankHeader_v49_t* const header)
	{
		this->buildTag = header->buildTag;
		this->bankHash = header->bankHash;

		// total source count including all languages
		this->sourceCount = header->sourceCount + (header->localisedSourceCount * (static_cast<uint32_t>(this->languageNames.size()) - 1));
		this->eventCount = header->eventCount;

		this->localisedSourceCount = header->localisedSourceCount;

		this->graphData = GetPtr<void>(header->graphData);
		this->audioSources = GetPtr<void>(header->sources);
		this->audioEventNames = GetPtr<EventName_s>(header->eventNames);
		this->audioEventData = GetPtr<void>(header->eventDataOffset);
		this->audioMarkers = GetPtr<MilesAudioMarker_t>(header->audioMarkers);
		this->stringTable = GetPtr<char>(header->strings);
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

	~CMilesAudioAsset();

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
MilesASIDecoder_t* GetBinkAudioDecoder();
