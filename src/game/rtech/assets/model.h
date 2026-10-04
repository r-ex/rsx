#pragma once
#include <game/rtech/cpakfile.h>
#include <game/rtech/utils/utils.h>
#include <game/rtech/assets/material.h>

#include <core/mdl/modeldata.h>

class CDXDrawData;

enum RSXSettings_RMDL_e
{
	SET_EXPORT_SEQUENCES = 0,
};

// pak data
struct ModelAssetHeader_v8_t
{
	void* data; // ptr to studiohdr & rmdl buffer
	char* name;

	char unk_10[8];

	void* physics;
	char* vertexComponentData; // non-streamable vertex components

	AssetGuid_t* animRigs;
	uint32_t numAnimRigs;

	uint32_t componentDataSize; // size of individual mdl components pre-baking (vtx, vvd, etc.)

	uint32_t unk_38;

	uint32_t numAnimSeqs;
	AssetGuid_t* animSeqs;

	char unk_48[8];
};

struct ModelAssetHeader_v9_t
{
	void* data; // ptr to studiohdr & rmdl buffer
	void* info; // set on load
	char* name;

	char gap_18[8];

	void* physics;
	char* vertexComponentData; // non-streamable vertex components
	char* staticStreamingData; // baked (streamable) vertex data used for static prop cache

	AssetGuid_t* animRigs;
	uint32_t numAnimRigs;

	uint32_t componentDataSize; // size of individual mdl components pre-baking (vtx, vvd, etc.)
	uint32_t streamingDataSize; // size of VG data post-baking

	uint64_t unk_4C;
	uint64_t unk_54;
	uint32_t unk_5C;

	uint32_t numAnimSeqs;
	AssetGuid_t* animSeqs;

	char gap_6C[8];
};
static_assert(sizeof(ModelAssetHeader_v9_t) == 120);

struct ModelAssetHeader_v12_1_t
{
	void* data; // ptr to studiohdr & rmdl buffer
	void* info; // set on load
	char* name;

	char gap_18[8];

	void* physics;
	char* vertexComponentData; // non-streamable vertex components
	char* staticStreamingData; // baked (streamable) vertex data used for static prop cache

	AssetGuid_t* animRigs;
	uint32_t numAnimRigs;

	uint32_t componentDataSize; // size of individual mdl components pre-baking (vtx, vvd, etc.)
	uint32_t streamingDataSize; // size of VG data post-baking

	char gap_4C[8];

	// number of anim sequences directly associated with this model
	uint32_t numAnimSeqs;
	AssetGuid_t* animSeqs;

	char gap_60[8];
};
static_assert(sizeof(ModelAssetHeader_v12_1_t) == 104);

struct ModelAssetHeader_v13_t
{
	void* data; // ptr to studiohdr & rmdl buffer
	void* info; // set on load
	char* name;

	char gap_18[8];

	void* physics;
	char* vertexComponentData; // non-streamable vertex components
	char* staticStreamingData; // baked (streamable) vertex data used for static prop cache

	AssetGuid_t* animRigs;
	uint32_t numAnimRigs;

	uint32_t componentDataSize; // size of individual mdl components pre-baking (vtx, vvd, etc.)
	uint32_t streamingDataSize; // size of VG data post-baking

	Vector bbox_min;
	Vector bbox_max;

	char gap_64[8];

	// number of anim sequences directly associated with this model
	uint32_t numAnimSeqs;
	AssetGuid_t* animSeqs;

	char gap_78[8];
};
static_assert(sizeof(ModelAssetHeader_v13_t) == 128);

struct ModelAssetHeader_v16_t
{
	void* data; // ptr to studiohdr & rmdl buffer
	char* name;

	char gap_10[8];

	char* staticStreamingData; // baked (streamable) vertex data used for static prop cache

	AssetGuid_t* animRigs;
	uint32_t numAnimRigs;

	uint32_t streamingDataSize; // size of VG data post-baking

	Vector bbox_min;
	Vector bbox_max;

	uint16_t gap_48;

	uint16_t numAnimSeqs;

	char gap_4C[4];

	AssetGuid_t* animSeqs;

	char gap_58[8];
};
static_assert(sizeof(ModelAssetHeader_v16_t) == 96);

struct ModelAssetCPU_v16_t
{
	void* physics;
	int dataSizePhys;
	int dataSizeModel;
};

// [rika]: this will be more painful than you think
// more accurate version that asset version, respawn did changes without increasing version
enum class eMDLVersion : int
{
	VERSION_UNK = -1,
	VERSION_8,
	VERSION_9,
	VERSION_10,
	VERSION_11,
	VERSION_12,
	VERSION_12_1,
	VERSION_12_2,
	VERSION_12_3, // changes mstudioevent_t (animseq 10)
	VERSION_12_4,
	VERSION_12_5,
	VERSION_13,
	VERSION_13_1,
	VERSION_14,
	VERSION_14_1, // minor changes but should be recorded here (known is related to string offsets)
	VERSION_15,
	VERSION_16,
	VERSION_17,
	VERSION_18,
	VERSION_19,
	VERSION_19_1,
	VERSION_19_2,
	VERSION_19_3,
	VERSION_20,

	VERSION_PAK_COUNT,

	// bleh
	VERSION_52,
	VERSION_53,
};

static const std::map<int, eMDLVersion> s_mdlVersionFromPak
{
	{ 8, eMDLVersion::VERSION_8 },
	{ 9, eMDLVersion::VERSION_9 },
	{ 10, eMDLVersion::VERSION_10 },
	{ 11, eMDLVersion::VERSION_11 },
	{ 12, eMDLVersion::VERSION_12 },
	{ 13, eMDLVersion::VERSION_13 },
	{ 14, eMDLVersion::VERSION_14 },
	{ 15, eMDLVersion::VERSION_15 },
	{ 16, eMDLVersion::VERSION_16 },
	{ 17, eMDLVersion::VERSION_17 },
	{ 18, eMDLVersion::VERSION_18 },
	{ 19, eMDLVersion::VERSION_19 },
	{ 20, eMDLVersion::VERSION_20 },
};

static const int s_mdlVersionToPak_Major[static_cast<int>(eMDLVersion::VERSION_PAK_COUNT)] =
{
	8,	// eMDLVersion::VERSION_8
	9,	// eMDLVersion::VERSION_9
	10,	// eMDLVersion::VERSION_10
	11,	// eMDLVersion::VERSION_11
	12,	// eMDLVersion::VERSION_12
	12,	// eMDLVersion::VERSION_12_1
	12,	// eMDLVersion::VERSION_12_2
	12,	// eMDLVersion::VERSION_12_3
	12,	// eMDLVersion::VERSION_12_4
	12,	// eMDLVersion::VERSION_12_5
	13,	// eMDLVersion::VERSION_13
	13,	// eMDLVersion::VERSION_13_1
	14,	// eMDLVersion::VERSION_14
	14,	// eMDLVersion::VERSION_14_1
	15,	// eMDLVersion::VERSION_15
	16,	// eMDLVersion::VERSION_16
	17,	// eMDLVersion::VERSION_17
	18,	// eMDLVersion::VERSION_18
	19,	// eMDLVersion::VERSION_19
	19,	// eMDLVersion::VERSION_19_1
	19,	// eMDLVersion::VERSION_19_2
	19,	// eMDLVersion::VERSION_19_3
	20,	// eMDLVersion::VERSION_20
};

static const int s_mdlVersionToPak_Minor[static_cast<int>(eMDLVersion::VERSION_PAK_COUNT)] =
{
	0,	// eMDLVersion::VERSION_8
	0,	// eMDLVersion::VERSION_9
	0,	// eMDLVersion::VERSION_10
	0,	// eMDLVersion::VERSION_11
	0,	// eMDLVersion::VERSION_12
	1,	// eMDLVersion::VERSION_12_1
	2,	// eMDLVersion::VERSION_12_2
	3,	// eMDLVersion::VERSION_12_3
	4,	// eMDLVersion::VERSION_12_4
	5,	// eMDLVersion::VERSION_12_5
	0,	// eMDLVersion::VERSION_13
	1,	// eMDLVersion::VERSION_13_1
	0,	// eMDLVersion::VERSION_14
	1,	// eMDLVersion::VERSION_14_1
	0,	// eMDLVersion::VERSION_15
	0,	// eMDLVersion::VERSION_16
	0,	// eMDLVersion::VERSION_17
	0,	// eMDLVersion::VERSION_18
	0,	// eMDLVersion::VERSION_19
	1,	// eMDLVersion::VERSION_19_1
	2,	// eMDLVersion::VERSION_19_2
	3,	// eMDLVersion::VERSION_19_3
	0,	// eMDLVersion::VERSION_20
};

inline const AssetVersion_t GetAssetVersionFromMDL(const eMDLVersion version)
{
	const AssetVersion_t asset(s_mdlVersionToPak_Major[static_cast<int>(version)], s_mdlVersionToPak_Minor[static_cast<int>(version)]);

	return asset;
}

constexpr uint64_t s_MdlTimeStamp_V19_1 = 0x01DC1DF805C28000; // 09/05/2025 00:00:00
constexpr uint64_t s_MdlTimeStamp_V19_3 = 0x01DD1EED32D6C000; // 07/29/2026 00:00:00

const eMDLVersion GetModelVersionFromAsset(CPakFile* const pak, const void* const studioBuffer, const int version, const int headerStructSize);
inline const eMDLVersion GetModelVersionFromAsset(CPakAsset* const asset, CPakFile* const pak)
{
	return GetModelVersionFromAsset(pak, reinterpret_cast<void**>(asset->header())[0], asset->version(), asset->data()->headerStructSize);
}

// generic
inline const eMDLVersion GetModelPakVersion(const int* const pHdr)
{
	if (pHdr[97] == sizeof(r5::studiohdr_v8_t) || pHdr[41] == sizeof(r5::studiohdr_v8_t))
		return  eMDLVersion::VERSION_8;

	if (pHdr[101] == sizeof(r5::studiohdr_v12_1_t) || pHdr[41] == sizeof(r5::studiohdr_v12_1_t))
		return eMDLVersion::VERSION_12_1;

	if (pHdr[102] == sizeof(r5::studiohdr_v12_2_t) || pHdr[41] == sizeof(r5::studiohdr_v12_2_t))
		return eMDLVersion::VERSION_12_2;

	if (pHdr[102] == sizeof(r5::studiohdr_v12_4_t) || pHdr[41] == sizeof(r5::studiohdr_v12_4_t))
		return eMDLVersion::VERSION_12_4;

	if (pHdr[102] == sizeof(r5::studiohdr_v12_5_t) || pHdr[41] == sizeof(r5::studiohdr_v12_5_t))
		return eMDLVersion::VERSION_12_5;
	
	if (pHdr[104] == sizeof(r5::studiohdr_v14_t) || pHdr[41] == sizeof(r5::studiohdr_v14_t))
		return eMDLVersion::VERSION_14;

	const uint16_t* const pHdrNew = reinterpret_cast<const uint16_t* const>(pHdr);

	if (pHdrNew[100] == sizeof(r5::studiohdr_v16_t) || pHdrNew[59] == sizeof(r5::studiohdr_v16_t))
		return eMDLVersion::VERSION_16;

	// v18
	if (pHdrNew[100] == sizeof(r5::studiohdr_v17_t) || pHdrNew[59] == sizeof(r5::studiohdr_v17_t))
		return eMDLVersion::VERSION_17;

	if (pHdrNew[54] == sizeof(r5::studiohdr_v19_2_t))
		return eMDLVersion::VERSION_19_2;

	return eMDLVersion::VERSION_UNK;
}

class ModelAsset
{
public:
	ModelAsset() = default;
	ModelAsset(ModelAssetHeader_v8_t* hdr, AssetPtr_t streamedData, eMDLVersion ver, const AssetVersion_t assetVersion) : name(hdr->name), data(hdr->data), vertexComponentData(hdr->vertexComponentData), staticStreamingData(nullptr), vertexStreamingData(streamedData), physics(hdr->physics),
		componentDataSize(hdr->componentDataSize), streamingDataSize(0u), animRigs(hdr->animRigs), numAnimRigs(hdr->numAnimRigs),
		numAnimSeqs(hdr->numAnimSeqs), animSeqs(hdr->animSeqs), version(ver), parsedData(reinterpret_cast<const r5::studiohdr_v8_t* const>(data), assetVersion) {};

	ModelAsset(ModelAssetHeader_v9_t* hdr, AssetPtr_t streamedData, eMDLVersion ver, const AssetVersion_t assetVersion) : name(hdr->name), data(hdr->data), vertexComponentData(hdr->vertexComponentData), staticStreamingData(hdr->staticStreamingData), vertexStreamingData(streamedData), physics(hdr->physics),
		componentDataSize(hdr->componentDataSize), streamingDataSize(hdr->streamingDataSize), animRigs(hdr->animRigs), numAnimRigs(hdr->numAnimRigs),
		numAnimSeqs(hdr->numAnimSeqs), animSeqs(hdr->animSeqs), version(ver), parsedData(reinterpret_cast<const r5::studiohdr_v8_t* const>(data), assetVersion) {};

	ModelAsset(ModelAssetHeader_v12_1_t* hdr, AssetPtr_t streamedData, eMDLVersion ver, const AssetVersion_t assetVersion) : name(hdr->name), data(hdr->data), vertexComponentData(hdr->vertexComponentData), staticStreamingData(hdr->staticStreamingData), vertexStreamingData(streamedData), physics(hdr->physics),
		componentDataSize(hdr->componentDataSize), streamingDataSize(hdr->streamingDataSize), animRigs(hdr->animRigs), numAnimRigs(hdr->numAnimRigs),
		numAnimSeqs(hdr->numAnimSeqs), animSeqs(hdr->animSeqs), version(ver)
	{
		switch (ver)
		{
		case eMDLVersion::VERSION_12_1:
		{
			parsedData = ModelParsedData_t(reinterpret_cast<const r5::studiohdr_v12_1_t* const>(data), assetVersion);
			break;
		}
		case eMDLVersion::VERSION_12_2:
		case eMDLVersion::VERSION_12_3:
		{
			parsedData = ModelParsedData_t(reinterpret_cast<const r5::studiohdr_v12_2_t* const>(data), assetVersion);
			break;
		}
		case eMDLVersion::VERSION_12_4:
		case eMDLVersion::VERSION_12_5:
		{
			parsedData = ModelParsedData_t(reinterpret_cast<const r5::studiohdr_v12_4_t* const>(data), assetVersion);
			break;
		}
		}
	};

	ModelAsset(ModelAssetHeader_v13_t* hdr, AssetPtr_t streamedData, eMDLVersion ver, const AssetVersion_t assetVersion) : name(hdr->name), data(hdr->data), vertexComponentData(hdr->vertexComponentData), staticStreamingData(hdr->staticStreamingData), vertexStreamingData(streamedData), physics(hdr->physics),
		componentDataSize(hdr->componentDataSize), streamingDataSize(hdr->streamingDataSize), animRigs(hdr->animRigs), numAnimRigs(hdr->numAnimRigs),
		numAnimSeqs(hdr->numAnimSeqs), animSeqs(hdr->animSeqs), version(ver)
	{
		switch (ver)
		{
		case eMDLVersion::VERSION_13:
		case eMDLVersion::VERSION_13_1:
		{
			parsedData = ModelParsedData_t(reinterpret_cast<const r5::studiohdr_v12_4_t* const>(data), assetVersion);
			break;
		}
		case eMDLVersion::VERSION_14:
		case eMDLVersion::VERSION_14_1:
		case eMDLVersion::VERSION_15:
		{
			parsedData = ModelParsedData_t(reinterpret_cast<const r5::studiohdr_v14_t* const>(data), assetVersion);
			break;
		}
		}
	};

	ModelAsset(ModelAssetHeader_v16_t* hdr, ModelAssetCPU_v16_t* cpu, AssetPtr_t streamedData, eMDLVersion ver, const AssetVersion_t assetVersion) :name(hdr->name), data(hdr->data), physics(cpu->physics),
		vertexComponentData(nullptr), staticStreamingData(hdr->staticStreamingData), vertexStreamingData(streamedData), componentDataSize(0u), streamingDataSize(hdr->streamingDataSize),
		animRigs(hdr->animRigs), numAnimRigs(hdr->numAnimRigs), numAnimSeqs(hdr->numAnimSeqs), animSeqs(hdr->animSeqs),
		version(ver)
	{
		switch (version)
		{
		case eMDLVersion::VERSION_16:
		{
			parsedData = ModelParsedData_t(reinterpret_cast<const r5::studiohdr_v16_t* const>(data), assetVersion, cpu->dataSizePhys, cpu->dataSizeModel);
			break;
		}
		case eMDLVersion::VERSION_17:
		case eMDLVersion::VERSION_18:
		case eMDLVersion::VERSION_19:
		case eMDLVersion::VERSION_19_1:
		{
			parsedData = ModelParsedData_t(reinterpret_cast<const r5::studiohdr_v17_t* const>(data), assetVersion, cpu->dataSizePhys, cpu->dataSizeModel);
			break;
		}
		case eMDLVersion::VERSION_19_2:
		case eMDLVersion::VERSION_19_3:
		case eMDLVersion::VERSION_20:
		{
			parsedData = ModelParsedData_t(reinterpret_cast<const r5::studiohdr_v19_2_t* const>(data), assetVersion, cpu->dataSizePhys, cpu->dataSizeModel);
			break;
		}
		}
	};

	~ModelAsset()
	{

	}

	char* name;
	void* data; // ptr to studiohdr & rmdl buffer

	void* physics;
	char* vertexComponentData; // non-streamable vertex components
	char* staticStreamingData; // baked (streamable) vertex data used for static prop cache
	AssetPtr_t vertexStreamingData;  // baked (streamable) vertex data (stored in starpak)

	uint32_t componentDataSize; // size of individual mdl components pre-baking (vtx, vvd, etc.)
	uint32_t streamingDataSize; // size of VG data post-baking, hw size is probably a better name as it also has static data

	AssetGuid_t* animRigs;
	AssetGuid_t* animSeqs;

	uint32_t numAnimRigs;
	uint32_t numAnimSeqs;

	ModelParsedData_t parsedData;

	eMDLVersion version; // like asset version, but takes between version revisions into consideration

	inline ModelParsedData_t* const GetParsedData() { return &parsedData; }
	inline const ModelParsedData_t* const GetParsedData() const { return &parsedData; }
	inline const ModelParsedData_t* const GetRig() const { return &parsedData; } // slerp them bones
};

// [rika]: sets the external sequences variables in ModelParsedData_t and cycles through all the animseqs to set their rig
void ParseExternalSequences(ModelParsedData_t* const parsedData, const uint32_t numAnimSeqs, AssetGuid_t* const animSeqs);