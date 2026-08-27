#pragma once


// r2
struct MilesSource_v13_t
{
	char gap0[16];
	uint32_t nameOffset;

	uint16_t sampleRate;
	uint16_t bitRate;

	// unverified offsets
	uint8_t channelCount;

	char gap_19[2];
	uint8_t markerCount;
	char gap_1C[19];

	uint32_t streamHeaderSize;
	uint32_t sampleCount;
	uint64_t streamHeaderOffset;
	uint64_t streamDataOffset;

	uint32_t markerOffset;
	uint32_t unk_4C;
	short languageIdx;
	short patchIdx;
	char gap_54[4];
};
static_assert(offsetof(MilesSource_v13_t, nameOffset) == 0x10);
static_assert(offsetof(MilesSource_v13_t, sampleRate) == 0x14);
static_assert(offsetof(MilesSource_v13_t, languageIdx) == 0x50);
//static_assert(offsetof(MilesSource_v13_t, gap_48) == 0x48);
static_assert(sizeof(MilesSource_v13_t) == 0x58);

// s0
struct MilesSource_v28_t
{
	char gap0[12];
	uint16_t languageIdx; // sound language ID
	uint16_t patchIdx; // index of the patch file that contains this sound
	uint32_t nameOffset; // relative to string table
	uint16_t sampleRate;
	uint16_t bitRate;

	char unk;
	uint8_t markerCount;

	uint8_t channelCount;
	char gap_1B[21];

	uint32_t streamHeaderSize;
	uint32_t sampleCount;
	uint64_t streamHeaderOffset;
	uint64_t streamDataOffset;
	uint32_t markerOffset;
	uint32_t unkMinusOne;

	char gap_end[8];
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

	char unk;
	uint8_t markerCount;

	char gap[8];

	uint16_t bpm;
	char gap_20[4];

	uint32_t streamHeaderSize;
	uint32_t sampleCount;
	uint64_t streamHeaderOffset;
	uint64_t streamDataOffset;
	uint32_t markerOffset;
	uint32_t unkMinusOne;
	char gap8[8];
};
static_assert(offsetof(MilesSource_v39_t, streamDataOffset) == 48);
static_assert(sizeof(MilesSource_v39_t) == 72);

// v48, v49
struct MilesSource_v48_t
{
	uint64_t nameOffset; // relative to Something
	uint16_t languageIdx; // sound language ID
	uint16_t patchIdx; // index of the patch file that contains this sound
	uint32_t unk8;
	uint16_t sampleRate;
	uint16_t bitRate;

	char unk;
	uint8_t markerCount;

	char gap[8];

	uint16_t bpm;
	char gap_20[4];

	uint32_t streamHeaderSize;
	uint64_t sampleCount;
	uint64_t streamHeaderOffset;
	uint64_t streamDataOffset;
	uint32_t markerOffset;
	uint32_t unkMinusOne;
	char gap8[8];
};
static_assert(sizeof(MilesSource_v48_t) == 80);

struct MilesSource_v49_t : public MilesSource_v48_t { };

struct MilesSource_t
{
	MilesSource_t(const MilesSource_v13_t* const a) :
		streamDataOffset(a->streamDataOffset), streamHeaderOffset(a->streamHeaderOffset),
		sampleCount(a->sampleCount), streamHeaderSize(a->streamHeaderSize),
		nameOffset(a->nameOffset),
		languageIdx(a->languageIdx), patchIdx(a->patchIdx), bpm(0), sampleRate(a->sampleRate)
	{
	};

	MilesSource_t(const MilesSource_v28_t* const a) :
		streamDataOffset(a->streamDataOffset), streamHeaderOffset(a->streamHeaderOffset),
		sampleCount(a->sampleCount), streamHeaderSize(a->streamHeaderSize),
		nameOffset(a->nameOffset),
		languageIdx(a->languageIdx), patchIdx(a->patchIdx), bpm(0), sampleRate(a->sampleRate)
	{
	};

	MilesSource_t(const MilesSource_v39_t* const a) :
		streamDataOffset(a->streamDataOffset), streamHeaderOffset(a->streamHeaderOffset),
		sampleCount(a->sampleCount), streamHeaderSize(a->streamHeaderSize),
		nameOffset(a->nameOffset),
		languageIdx(a->languageIdx), patchIdx(a->patchIdx), bpm(a->bpm), sampleRate(a->sampleRate)
	{
	};

	MilesSource_t(const MilesSource_v48_t* const a) :
		streamDataOffset(a->streamDataOffset), streamHeaderOffset(a->streamHeaderOffset),
		sampleCount(a->sampleCount), streamHeaderSize(a->streamHeaderSize),
		nameOffset(a->nameOffset),
		languageIdx(a->languageIdx), patchIdx(a->patchIdx), bpm(a->bpm), sampleRate(a->sampleRate)
	{
	};

	uint64_t nameOffset;
	uint64_t streamDataOffset;
	uint64_t streamHeaderOffset;
	uint64_t sampleCount;
	uint32_t streamHeaderSize;

	uint16_t languageIdx;
	uint16_t patchIdx;

	uint16_t bpm;
	uint16_t sampleRate;

	std::vector<ImGuiExt::AudioMarker_s> audioMarkers;

	float duration() const
	{
		return sampleCount / (float)sampleRate;
	}
};
