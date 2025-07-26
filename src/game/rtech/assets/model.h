#pragma once
#include <game/rtech/cpakfile.h>
#include <game/rtech/utils/utils.h>
#include <game/rtech/utils/studio/studio_generic.h>
#include <game/rtech/assets/material.h>

#include <core/mdl/modeldata.h>

class CDXDrawData;

// pak data
struct ModelAssetHeader_v8_t
{
	void* data; // ptr to studiohdr & rmdl buffer

	char* name;

	char unk_10[8];

	void* physics;

	char* permVertData; // permanent vertex components

	AssetGuid_t* animRigs;
	int numAnimRigs;

	int componentDataSize; // size of individual mdl components pre-baking (vtx, vvd, etc.)
	int streamingDataSize; // size of VG data post-baking. probably not real but we don't need to change packing

	int numAnimSeqs;
	AssetGuid_t* animSeqs;

	char unk_48[8];
};

struct ModelAssetHeader_v9_t
{
	void* data; // ptr to studiohdr & rmdl buffer
	void* reserved_1;

	char* name;
	char gap_18[8];

	void* physics;
	void* unk_28;

	char* permVertData; // used for static prop cache

	AssetGuid_t* animRigs;
	int numAnimRigs;

	int componentDataSize; // size of individual mdl components pre-baking (vtx, vvd, etc.)
	int streamingDataSize; // size of VG data post-baking

	uint64_t unk_4C;
	uint64_t unk_54;
	uint32_t unk_5C;

	int numAnimSeqs;
	AssetGuid_t* animSeqs;

	char gap_6C[8];
};
static_assert(sizeof(ModelAssetHeader_v9_t) == 120);

struct ModelAssetHeader_v12_1_t
{
	void* data; // ptr to studiohdr & rmdl buffer
	void* reserved_1;

	char* name;
	char gap_18[8];

	void* physics;
	void* unk_28;

	char* permVertData; // used for static prop cache

	AssetGuid_t* animRigs;
	int numAnimRigs;

	int componentDataSize; // size of individual mdl components pre-baking (vtx, vvd, etc.)
	int streamingDataSize; // size of VG data post-baking

	char gap_4C[8];

	// number of anim sequences directly associated with this model
	int numAnimSeqs;
	AssetGuid_t* animSeqs;

	char gap_60[8];
};
static_assert(sizeof(ModelAssetHeader_v12_1_t) == 104);

struct ModelAssetHeader_v13_t
{
	void* data; // ptr to studiohdr & rmdl buffer
	void* reserved_1;

	char* name;
	char gap_18[8];

	void* physics;
	void* unk_28;

	char* permVertData; // used for static prop cache

	AssetGuid_t* animRigs;
	int numAnimRigs;

	int componentDataSize; // size of individual mdl components pre-baking (vtx, vvd, etc.)
	int streamingDataSize; // size of VG data post-baking

	Vector bbox_min;
	Vector bbox_max;

	char gap_64[8];

	// number of anim sequences directly associated with this model
	int numAnimSeqs;
	AssetGuid_t* animSeqs;

	char gap_78[8];
};
static_assert(sizeof(ModelAssetHeader_v13_t) == 128);

struct ModelAssetHeader_v16_t
{
	void* data; // ptr to studiohdr & rmdl buffer
	char* name;
	char gap_10[8];

	char* permVertData; // used for static prop cache

	AssetGuid_t* animRigs;
	int numAnimRigs;

	int streamingDataSize; // size of VG data post-baking

	Vector bbox_min;
	Vector bbox_max;

	short gap_48;

	short numAnimSeqs;

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

	// bleh
	VERSION_52,
	VERSION_53,
};

static const std::map<int, eMDLVersion> s_mdlVersionMap
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
};

inline const eMDLVersion GetModelVersionFromAsset(CPakAsset* const asset)
{
	eMDLVersion out = eMDLVersion::VERSION_UNK;

	if (s_mdlVersionMap.count(asset->version()) == 1u)
		out = s_mdlVersionMap.at(asset->version());

	// pointer to the studiohdr is always the first entry in ModelAssetHeader regardless of versions (if this changes it won't affect this anyway)
	// so get that pointer for our studiohdr pointer, probably a better way to snag this but if it works it works
	const int* const pMDL = reinterpret_cast<int*>(reinterpret_cast<void**>(asset->header())[0]);

	switch (out)
	{
	case eMDLVersion::VERSION_12:
	{
		// [rika]: love to see it
		// each of these index to the position of sourceFilenameOffset, we check what value it has (should point to end of header) to see which iteration it is, and then verify the asset header's size is correct
		if ((pMDL[97] == sizeof(r5::studiohdr_v8_t) || pMDL[41] == sizeof(r5::studiohdr_v8_t)) && asset->data()->headerStructSize == sizeof(ModelAssetHeader_v9_t))
			return eMDLVersion::VERSION_12;

		if ((pMDL[101] == sizeof(r5::studiohdr_v12_1_t) || pMDL[41] == sizeof(r5::studiohdr_v8_t)) && asset->data()->headerStructSize == sizeof(ModelAssetHeader_v12_1_t))
			return eMDLVersion::VERSION_12_1;

		if ((pMDL[102] == sizeof(r5::studiohdr_v12_2_t) || pMDL[41] == sizeof(r5::studiohdr_v12_2_t)) && asset->data()->headerStructSize == sizeof(ModelAssetHeader_v12_1_t))
			return eMDLVersion::VERSION_12_2;

		if ((pMDL[102] == sizeof(r5::studiohdr_v12_4_t) || pMDL[41] == sizeof(r5::studiohdr_v12_4_t)) && asset->data()->headerStructSize == sizeof(ModelAssetHeader_v12_1_t))
			return eMDLVersion::VERSION_12_4;

		if ((pMDL[102] == sizeof(r5::studiohdr_v12_5_t) || pMDL[41] == sizeof(r5::studiohdr_v12_5_t)) && asset->data()->headerStructSize == sizeof(ModelAssetHeader_v12_1_t))
			return eMDLVersion::VERSION_12_5;

		return eMDLVersion::VERSION_UNK;
	}
	case eMDLVersion::VERSION_13:
	{
		const r5::studiohdr_v12_5_t* const pHdr = reinterpret_cast<const r5::studiohdr_v12_5_t* const>(pMDL);

		if (pHdr->numbodyparts == 0)
			return out;

		const mstudiobodyparts_t* const pBodypart0 = pHdr->pBodypart(0);
		const r5::mstudiomodel_v12_1_t* const pModel = pBodypart0->pModel<r5::mstudiomodel_v12_1_t>(0);

		if (pModel->meshindex == 0)
		{
			assertm(false, "could not properly check version");
			return out;
		}

		// get the start and end point for mstudiomodel_t structs
		const int modelStart = static_cast<int>(reinterpret_cast<const char*>(pModel) - reinterpret_cast<const char*>(pMDL));
		const int modelEnd = modelStart + pModel->meshindex;
		const int modelSize = modelEnd - modelStart;
		int modelCount = 0;

		for (int i = 0; i < pHdr->numbodyparts; i++)
			modelCount += pHdr->pBodypart(i)->nummodels;

		const int modelSizeSingle = modelSize / modelCount;

		if (modelSizeSingle == static_cast<int>(sizeof(r5::mstudiomodel_v13_1_t)))
			return eMDLVersion::VERSION_13_1;

		return out;
	}
	default:
	{
		return out;
	}
	}
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

	return eMDLVersion::VERSION_UNK;
}

class ModelAsset
{
public:
	ModelAsset() = default;
	ModelAsset(ModelAssetHeader_v8_t* hdr, AssetPtr_t streamedData, eMDLVersion ver) : name(hdr->name), data(hdr->data), vertDataPermanent(hdr->permVertData), vertDataStreamed(streamedData), physics(hdr->physics),
		componentDataSize(hdr->componentDataSize), streamingDataSize(hdr->streamingDataSize), animRigs(hdr->animRigs), numAnimRigs(hdr->numAnimRigs),
		numAnimSeqs(hdr->numAnimSeqs), animSeqs(hdr->animSeqs), version(ver), parsedData(reinterpret_cast<r5::studiohdr_v8_t*>(data)) {};

	ModelAsset(ModelAssetHeader_v9_t* hdr, AssetPtr_t streamedData, eMDLVersion ver) : name(hdr->name), data(hdr->data), vertDataPermanent(hdr->permVertData), vertDataStreamed(streamedData), physics(hdr->physics),
		componentDataSize(hdr->componentDataSize), streamingDataSize(hdr->streamingDataSize), animRigs(hdr->animRigs), numAnimRigs(hdr->numAnimRigs),
		numAnimSeqs(hdr->numAnimSeqs), animSeqs(hdr->animSeqs), version(ver), parsedData(reinterpret_cast<r5::studiohdr_v8_t*>(data)) {};

	ModelAsset(ModelAssetHeader_v12_1_t* hdr, AssetPtr_t streamedData, eMDLVersion ver) : name(hdr->name), data(hdr->data), vertDataPermanent(hdr->permVertData), vertDataStreamed(streamedData), physics(hdr->physics),
		componentDataSize(hdr->componentDataSize), streamingDataSize(hdr->streamingDataSize), animRigs(hdr->animRigs), numAnimRigs(hdr->numAnimRigs),
		numAnimSeqs(hdr->numAnimSeqs), animSeqs(hdr->animSeqs), version(ver)
	{
		switch (ver)
		{
		case eMDLVersion::VERSION_12_1:
		{
			parsedData = ModelParsedData_t(reinterpret_cast<r5::studiohdr_v12_1_t*>(data));
			break;
		}
		case eMDLVersion::VERSION_12_2:
		case eMDLVersion::VERSION_12_3:
		{
			parsedData = ModelParsedData_t(reinterpret_cast<r5::studiohdr_v12_2_t*>(data));
			break;
		}
		case eMDLVersion::VERSION_12_4:
		case eMDLVersion::VERSION_12_5:
		{
			parsedData = ModelParsedData_t(reinterpret_cast<r5::studiohdr_v12_4_t*>(data));
			break;
		}
		}
	};

	ModelAsset(ModelAssetHeader_v13_t* hdr, AssetPtr_t streamedData, eMDLVersion ver) : name(hdr->name), data(hdr->data), vertDataPermanent(hdr->permVertData), vertDataStreamed(streamedData), physics(hdr->physics),
		componentDataSize(hdr->componentDataSize), streamingDataSize(hdr->streamingDataSize), animRigs(hdr->animRigs), numAnimRigs(hdr->numAnimRigs),
		numAnimSeqs(hdr->numAnimSeqs), animSeqs(hdr->animSeqs), version(ver)
	{
		switch (ver)
		{
		case eMDLVersion::VERSION_13:
		case eMDLVersion::VERSION_13_1:
		{
			parsedData = ModelParsedData_t(reinterpret_cast<r5::studiohdr_v12_4_t*>(data));
			break;
		}
		case eMDLVersion::VERSION_14:
		case eMDLVersion::VERSION_14_1:
		case eMDLVersion::VERSION_15:
		{
			parsedData = ModelParsedData_t(reinterpret_cast<r5::studiohdr_v14_t*>(data));
			break;
		}
		}
	};

	ModelAsset(ModelAssetHeader_v16_t* hdr, ModelAssetCPU_v16_t* cpu, AssetPtr_t streamedData, eMDLVersion ver) : name(hdr->name), data(hdr->data), vertDataPermanent(hdr->permVertData), vertDataStreamed(streamedData), physics(cpu->physics),
		componentDataSize(-1), streamingDataSize(hdr->streamingDataSize), animRigs(hdr->animRigs), numAnimRigs(hdr->numAnimRigs),
		numAnimSeqs(hdr->numAnimSeqs), animSeqs(hdr->animSeqs), version(ver)
	{
		switch (ver)
		{
		case eMDLVersion::VERSION_16:
		{
			parsedData = ModelParsedData_t(reinterpret_cast<r5::studiohdr_v16_t*>(data), cpu->dataSizePhys, cpu->dataSizeModel);
			break;
		}
		case eMDLVersion::VERSION_17:
		case eMDLVersion::VERSION_18:
		case eMDLVersion::VERSION_19:
		{
			parsedData = ModelParsedData_t(reinterpret_cast<r5::studiohdr_v17_t*>(data), cpu->dataSizePhys, cpu->dataSizeModel);
			break;
		}
		}
	};

	~ModelAsset()
	{
		delete drawData;
	}

	//void InitBoneMatrix();
	//void UpdateBoneMatrix();

	char* name;
	void* data; // ptr to studiohdr & rmdl buffer
	AssetPtr_t vertDataStreamed;  // vertex data (valve or hwdata)
	char* vertDataPermanent; // vertex data (valve or hwdata)  
	void* physics;

	int componentDataSize; // size of individual mdl components pre-baking (vtx, vvd, etc.)
	int streamingDataSize; // size of VG data post-baking, hw size is probably a better name as it also has static data

	AssetGuid_t* animRigs;
	AssetGuid_t* animSeqs;

	int numAnimRigs;
	int numAnimSeqs;

	CDXDrawData* drawData;

	ModelParsedData_t parsedData;

	eMDLVersion version; // like asset version, but takes between version revisions into consideration

	inline const studiohdr_generic_t& StudioHdr() const { return parsedData.studiohdr; }
	inline const studiohdr_generic_t* const pStudioHdr() const { return &parsedData.studiohdr; }
	inline ModelParsedData_t* const GetParsedData() { return &parsedData; }
	inline const std::vector<ModelBone_t>* const GetRig() const { return &parsedData.bones; } // slerp them bones

	// get loose files from vertDataPermanent
	const OptimizedModel::FileHeader_t* const GetVTX() const { return StudioHdr().vtxSize > 0 ? reinterpret_cast<const OptimizedModel::FileHeader_t* const>(vertDataPermanent + StudioHdr().vtxOffset) : nullptr; }
	const vvd::vertexFileHeader_t* const GetVVD() const { return StudioHdr().vvdSize > 0 ? reinterpret_cast<const vvd::vertexFileHeader_t* const>(vertDataPermanent + StudioHdr().vvdOffset) : nullptr; }
	const vvc::vertexColorFileHeader_t* const GetVVC() const { return StudioHdr().vvcSize > 0 ? reinterpret_cast<const vvc::vertexColorFileHeader_t* const>(vertDataPermanent + StudioHdr().vvcOffset) : nullptr; }
	const vvw::vertexBoneWeightsExtraFileHeader_t* const GetVVW() const { return StudioHdr().vvwSize > 0 ? reinterpret_cast<const vvw::vertexBoneWeightsExtraFileHeader_t* const>(vertDataPermanent + StudioHdr().vvwOffset) : nullptr; }
};