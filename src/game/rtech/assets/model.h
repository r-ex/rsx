#pragma once
#include <game/rtech/cpakfile.h>
#include <game/rtech/utils/utils.h>
#include <game/rtech/utils/studio/studio_generic.h>
#include <game/rtech/assets/material.h>

#include <core/utils/ramen.h>
#include <core/mdl/compdata.h>

class CDXDrawData;

// pak data
struct ModelAssetHeader_v8_t
{
	void* data; // ptr to studiohdr & rmdl buffer

	char* name;

	char unk_10[8];

	void* physics;

	char* vertData; // permanent vertex components

	AssetGuid_t* animRigs;
	int numAnimRigs;

	int componentDataSize; // size of individual mdl components pre-baking (vtx, vvd, etc.)
	int streamingDataSize; // size of VG data post-baking. probably not real but we don't need to change packing

	int numAnimSeqs;
	AssetGuid_t* animSeqs;

	char unk_48[8];
};

// [rika]: something in this struct is off?
// [amos]: probably the animseq stuff that was off? fixed in cl946
struct ModelAssetHeader_v9_t
{
	void* data; // ptr to studiohdr & rmdl buffer
	void* reserved_1;

	char* name;
	char gap_18[8];

	void* physics;
	void* unk_28;

	char* unkVertData; // used for static prop cache

	AssetGuid_t* animRigs;
	int numAnimRigs;

	int componentDataSize; // size of individual mdl components pre-baking (vtx, vvd, etc.)
	int streamingDataSize; // size of VG data post-baking

	uint64_t unk3;
	uint64_t unk4;
	uint32_t unk5;

	int numAnimSeqs;
	AssetGuid_t* animSeqs;

	char gap_unk6[8];
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

	char* unkVertData; // used for static prop cache

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

// size: 0x80
struct ModelAssetHeader_v13_t
{
	void* data; // ptr to studiohdr & rmdl buffer
	void* reserved_1;

	char* name;
	char gap_18[8];

	void* physics;
	void* unk_28;

	char* unkVertData; // used for static prop cache

	AssetGuid_t* animRigs;
	int numAnimRigs;

	int componentDataSize; // size of individual mdl components pre-baking (vtx, vvd, etc.)
	int streamingDataSize; // size of VG data post-baking

	// [rika]: we need a vector class
	//Vector bbox_min;
	//Vector bbox_max;
	float bbox_min[3];
	float bbox_max[3];

	char gap_unk[8];

	// number of anim sequences directly associated with this model
	int numAnimSeqs;
	AssetGuid_t* animSeqs;

	char gap_unk1[8];
};

struct ModelAssetHeader_v16_t
{
	void* data; // ptr to studiohdr & rmdl buffer
	char* name;
	char gap_10[8];

	char* unkVertData; // used for static prop cache

	AssetGuid_t* animRigs;
	int numAnimRigs;

	int streamingDataSize; // size of VG data post-baking

	//Vector bbox_min;
	//Vector bbox_max;
	float bbox_min[3];
	float bbox_max[3];

	short gap_unk;

	short numAnimSeqs;

	char gap_unk1[4];

	AssetGuid_t* animSeqs;

	char gap_unk2[8];
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
	VERSION_12_3,
	VERSION_13,
	VERSION_13_1,
	VERSION_14,
	VERSION_14_1, // minor changes but should be recorded here (known is related to string offsets)
	VERSION_15,
	VERSION_16,
	VERSION_17,
	VERSION_18,

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

		if ((pMDL[102] == sizeof(r5::studiohdr_v12_3_t) || pMDL[41] == sizeof(r5::studiohdr_v12_3_t)) && asset->data()->headerStructSize == sizeof(ModelAssetHeader_v12_1_t))
			return eMDLVersion::VERSION_12_3;

		return out;
	}
	case eMDLVersion::VERSION_13:
	{
		const r5::studiohdr_v12_3_t* const pHdr = reinterpret_cast<const r5::studiohdr_v12_3_t* const>(pMDL);

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

	if (pHdr[102] == sizeof(r5::studiohdr_v12_3_t) || pHdr[41] == sizeof(r5::studiohdr_v12_3_t))
		return eMDLVersion::VERSION_12_3;
	
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

class ModelAsset;
struct ModelMeshData_t
{
	ModelMeshData_t() : meshVertexDataIndex(invalidNoodleIdx), rawVertexData(nullptr), rawVertexLayoutFlags(0ull), indexCount(0), vertCount(0), vertCacheSize(0), weightsPerVert(0), weightsCount(0), texcoordCount(0), texcoodIndices(0), materialId(0), material(nullptr), bodyPartIndex(-1) {};
	ModelMeshData_t(const ModelMeshData_t& mesh) : meshVertexDataIndex(mesh.meshVertexDataIndex), rawVertexData(mesh.rawVertexData), rawVertexLayoutFlags(mesh.rawVertexLayoutFlags), indexCount(mesh.indexCount), vertCount(mesh.vertCount), vertCacheSize(mesh.vertCacheSize),
		weightsPerVert(mesh.weightsPerVert), weightsCount(mesh.weightsCount), texcoordCount(mesh.texcoordCount), texcoodIndices(mesh.texcoodIndices), materialId(mesh.materialId), material(mesh.material), bodyPartIndex(mesh.bodyPartIndex) {};
	ModelMeshData_t(ModelMeshData_t& mesh) : meshVertexDataIndex(mesh.meshVertexDataIndex), rawVertexData(mesh.rawVertexData), rawVertexLayoutFlags(mesh.rawVertexLayoutFlags), indexCount(mesh.indexCount), vertCount(mesh.vertCount), vertCacheSize(mesh.vertCacheSize),
		weightsPerVert(mesh.weightsPerVert), weightsCount(mesh.weightsCount), texcoordCount(mesh.texcoordCount), texcoodIndices(mesh.texcoodIndices), materialId(mesh.materialId), material(mesh.material), bodyPartIndex(mesh.bodyPartIndex) {};

	~ModelMeshData_t() { }

	size_t meshVertexDataIndex;

	char* rawVertexData; // for model preview only
	uint64_t rawVertexLayoutFlags; // the flags from a 'vg' (baked hwData) mesh, identical to 'Vertex Layout Flags' (DX)
	
	uint32_t indexCount;

	uint32_t vertCount;
	uint16_t vertCacheSize;

	uint16_t weightsPerVert; // max number of weights per vertex
	uint32_t weightsCount; // the total number of weights in this mesh
	
	int16_t texcoordCount;
	int16_t texcoodIndices; // texcoord indices, e.g. texcoord0, texcoord2, etc

	int materialId; // the index of this material
	MaterialAsset* material; // pointer to the material's asset (if loaded)

	int bodyPartIndex;

	// gets the texcoord and indice based off rawVertexLayoutFlags
	inline void ParseTexcoords();
	inline void ParseMaterial(ModelAsset* const modelAsset, const int materialIdx, const int meshid);
};

struct ModelModelData_t
{
	std::string name;
	ModelMeshData_t* meshes;
	size_t meshIndex;
	uint32_t meshCount;
};

struct ModelLODData_t
{
	std::vector<ModelModelData_t> models;
	std::vector<ModelMeshData_t> meshes;
	size_t vertexCount;
	size_t indexCount;
	float switchPoint;

	// for exporting
	uint16_t texcoordsPerVert; // max texcoords used in any mesh from this lod
	uint16_t weightsPerVert; // max weights used in any mesh from this lod

	inline const int GetMeshCount() const { return static_cast<int>(meshes.size()); }
	inline const int GetModelCount() const { return static_cast<int>(models.size()); }
};

struct ModelBone_t
{
	ModelBone_t() = default;
	ModelBone_t(const r5::mstudiobone_v8_t* bone) : name(bone->pszName()), parentIndex(bone->parent), flags(bone->flags), poseToBone(bone->poseToBone), pos(bone->pos), quat(bone->quat), rot(bone->rot), scale(bone->scale) {};
	ModelBone_t(const r5::mstudiobone_v12_1_t* bone) : name(bone->pszName()), parentIndex(bone->parent), flags(bone->flags), poseToBone(bone->poseToBone), pos(bone->pos), quat(bone->quat), rot(bone->rot), scale(bone->scale) {};
	ModelBone_t(const r5::mstudiobonehdr_v16_t* bonehdr, const r5::mstudiobonedata_v16_t* bonedata) : name(bonehdr->pszName()), parentIndex(bonedata->parent), flags(bonedata->flags), poseToBone(bonedata->poseToBone), pos(bonedata->pos), quat(bonedata->quat), rot(bonedata->rot), scale(bonedata->scale) {};
	ModelBone_t(const r1::mstudiobone_t* bone) : name(bone->pszName()), parentIndex(bone->parent), flags(bone->flags), poseToBone(bone->poseToBone), pos(bone->pos), quat(bone->quat), rot(bone->rot), scale(bone->scale) {};
	ModelBone_t(const r2::mstudiobone_t* bone) : name(bone->pszName()), parentIndex(bone->parent), flags(bone->flags), poseToBone(bone->poseToBone), pos(bone->pos), quat(bone->quat), rot(bone->rot), scale(bone->scale) {};

	const char* name;
	int parentIndex;

	int flags;

	matrix3x4_t poseToBone;

	Vector pos;
	Quaternion quat;
	RadianEuler rot;
	Vector scale;
};

struct ModelMaterialData_t
{
	// pointer to the referenced material asset
	CPakAsset* materialAsset;

	// also store guid and name just in case the material is not loaded
	uint64_t materialGuid;
	std::string materialName;
};

struct ModelSkinData_t
{
	ModelSkinData_t(const char* nameIn, const int16_t* indiceIn) : name(nameIn), indices(indiceIn) {};

	const char* name;
	const int16_t* indices;
};

struct ModelBodyPart_t
{
	ModelBodyPart_t() : partName(), modelIndex(-1), numModels(0), previewEnabled(true) {};

	std::string partName;

	int modelIndex;
	int numModels;

	bool previewEnabled;

	FORCEINLINE void SetName(const std::string& name) { partName = name; };
	FORCEINLINE std::string GetName() const { return partName; };
	FORCEINLINE const char* GetNameCStr() const { return partName.c_str(); };
	FORCEINLINE bool IsPreviewEnabled() const { return previewEnabled; };
};

struct ModelAnimSequence_t
{
	enum class eType : uint8_t
	{
		UNLOADED = 0,
		DIRECT,
		RIG,
	};

	CPakAsset* asset;
	uint64_t guid;
	eType type;

	FORCEINLINE const bool IsLoaded() const { return type != eType::UNLOADED; };
};

class ModelAsset
{
public:
	ModelAsset() = default;
	ModelAsset(ModelAssetHeader_v8_t* hdr, AssetPtr_t streamedData, eMDLVersion ver) : name(hdr->name), data(hdr->data), vertDataPermanent(hdr->vertData), vertDataStreamed(streamedData), physics(hdr->physics),
		componentDataSize(hdr->componentDataSize), streamingDataSize(hdr->streamingDataSize), animRigs(hdr->animRigs), numAnimRigs(hdr->numAnimRigs),
		numAnimSeqs(hdr->numAnimSeqs), animSeqs(hdr->animSeqs), version(ver), studiohdr(studiohdr_generic_t(reinterpret_cast<r5::studiohdr_v8_t*>(data))) {};

	ModelAsset(ModelAssetHeader_v9_t* hdr, AssetPtr_t streamedData, eMDLVersion ver) : name(hdr->name), data(hdr->data), vertDataPermanent(hdr->unkVertData), vertDataStreamed(streamedData), physics(hdr->physics),
		componentDataSize(hdr->componentDataSize), streamingDataSize(hdr->streamingDataSize), animRigs(hdr->animRigs), numAnimRigs(hdr->numAnimRigs),
		numAnimSeqs(hdr->numAnimSeqs), animSeqs(hdr->animSeqs), version(ver), studiohdr(studiohdr_generic_t(reinterpret_cast<r5::studiohdr_v8_t*>(data))) {};

	ModelAsset(ModelAssetHeader_v12_1_t* hdr, AssetPtr_t streamedData, eMDLVersion ver) : name(hdr->name), data(hdr->data), vertDataPermanent(hdr->unkVertData), vertDataStreamed(streamedData), physics(hdr->physics),
		componentDataSize(hdr->componentDataSize), streamingDataSize(hdr->streamingDataSize), animRigs(hdr->animRigs), numAnimRigs(hdr->numAnimRigs),
		numAnimSeqs(hdr->numAnimSeqs), animSeqs(hdr->animSeqs), version(ver)
	{
		switch (ver)
		{
		case eMDLVersion::VERSION_12_1:
		{
			studiohdr = studiohdr_generic_t(reinterpret_cast<r5::studiohdr_v12_1_t*>(data));
			break;
		}
		case eMDLVersion::VERSION_12_2:
		{
			studiohdr = studiohdr_generic_t(reinterpret_cast<r5::studiohdr_v12_2_t*>(data));
			break;
		}
		case eMDLVersion::VERSION_12_3:
		{
			studiohdr = studiohdr_generic_t(reinterpret_cast<r5::studiohdr_v12_3_t*>(data));
			break;
		}
		}
	};

	ModelAsset(ModelAssetHeader_v13_t* hdr, AssetPtr_t streamedData, eMDLVersion ver) : name(hdr->name), data(hdr->data), vertDataPermanent(hdr->unkVertData), vertDataStreamed(streamedData), physics(hdr->physics),
		componentDataSize(hdr->componentDataSize), streamingDataSize(hdr->streamingDataSize), animRigs(hdr->animRigs), numAnimRigs(hdr->numAnimRigs),
		numAnimSeqs(hdr->numAnimSeqs), animSeqs(hdr->animSeqs), version(ver)
	{
		switch (ver)
		{
		case eMDLVersion::VERSION_13:
		case eMDLVersion::VERSION_13_1:
		{
			studiohdr = studiohdr_generic_t(reinterpret_cast<r5::studiohdr_v12_3_t*>(data));
			break;
		}
		case eMDLVersion::VERSION_14:
		case eMDLVersion::VERSION_14_1:
		case eMDLVersion::VERSION_15:
		{
			studiohdr = studiohdr_generic_t(reinterpret_cast<r5::studiohdr_v14_t*>(data));
			break;
		}
		}
	};

	ModelAsset(ModelAssetHeader_v16_t* hdr, ModelAssetCPU_v16_t* cpu, AssetPtr_t streamedData, eMDLVersion ver) : name(hdr->name), data(hdr->data), vertDataPermanent(hdr->unkVertData), vertDataStreamed(streamedData), physics(cpu->physics),
		componentDataSize(-1), streamingDataSize(hdr->streamingDataSize), animRigs(hdr->animRigs), numAnimRigs(hdr->numAnimRigs),
		numAnimSeqs(hdr->numAnimSeqs), animSeqs(hdr->animSeqs), version(ver), studiohdr(studiohdr_generic_t(reinterpret_cast<r5::studiohdr_v16_t*>(data), cpu->dataSizePhys, cpu->dataSizeModel)) {};

	// oh ya this is gaming
	ModelAsset(r1::studiohdr_t* hdr, StudioLooseData_t* data) : name(hdr->name), data(reinterpret_cast<void*>(hdr)), vertDataPermanent(data->vertexDataBuffer), vertDataStreamed(), physics(data->physicsDataBuffer),
		componentDataSize(data->vertexDataSize[StudioLooseData_t::SLD_VTX] + data->vertexDataSize[StudioLooseData_t::SLD_VVD] + data->vertexDataSize[StudioLooseData_t::SLD_VVC]), streamingDataSize(0), animRigs(nullptr), numAnimRigs(0),
		numAnimSeqs(0), animSeqs(nullptr), version(eMDLVersion::VERSION_52), studiohdr(hdr, data) {};

	ModelAsset(r2::studiohdr_t* hdr) : name(hdr->name), data(reinterpret_cast<void*>(hdr)), vertDataPermanent(reinterpret_cast<char*>(hdr)), vertDataStreamed(), physics(hdr->pPHY()),
		componentDataSize(hdr->vtxSize + hdr->vvdSize + hdr->vvcSize), streamingDataSize(0), animRigs(nullptr), numAnimRigs(0),
		numAnimSeqs(0), animSeqs(nullptr), version(eMDLVersion::VERSION_53), studiohdr(hdr) {};

	~ModelAsset()
	{
		delete drawData;
	}

	void InitBoneMatrix();
	void UpdateBoneMatrix();

	FORCEINLINE void SetupBodyPart(int i, const char* partName, const int modelIndex, const int numModels)
	{
		ModelBodyPart_t& part = bodyParts.at(i);
		if (part.partName.empty())
		{
			part.SetName(partName);
			part.modelIndex = modelIndex;
			part.numModels = numModels;
		};
	};

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
	CRamen meshVertexData;

	std::vector<ModelBone_t> bones;

	std::vector<ModelLODData_t> lods;
	std::vector<ModelMaterialData_t> materials;
	std::vector<ModelSkinData_t> skins;

	std::vector<ModelBodyPart_t> bodyParts;

	// Resolved vector of animation sequences for use in model preview
	std::vector<ModelAnimSequence_t> animSequences;

	studiohdr_generic_t studiohdr;

	eMDLVersion version; // like asset version, but takes between version revisions into consideration

	// get loose files from vertDataPermanent
	const OptimizedModel::FileHeader_t* const GetVTX() const
	{
		return studiohdr.vtxSize > 0 ? reinterpret_cast<const OptimizedModel::FileHeader_t* const>(vertDataPermanent + studiohdr.vtxOffset) : nullptr;
	}
	const vvd::vertexFileHeader_t* const GetVVD() const
	{
		return studiohdr.vvdSize > 0 ? reinterpret_cast<const vvd::vertexFileHeader_t* const>(vertDataPermanent + studiohdr.vvdOffset) : nullptr;
	}
	const vvc::vertexColorFileHeader_t* const GetVVC() const
	{
		return studiohdr.vvcSize > 0 ? reinterpret_cast<const vvc::vertexColorFileHeader_t* const>(vertDataPermanent + studiohdr.vvcOffset) : nullptr;
	}
	const vvw::vertexBoneWeightsExtraFileHeader_t* const GetVVW() const
	{
		return studiohdr.vvwSize > 0 ? reinterpret_cast<const vvw::vertexBoneWeightsExtraFileHeader_t* const>(vertDataPermanent + studiohdr.vvwOffset) : nullptr;
	}
};

// define after ModelAsset
void ModelMeshData_t::ParseTexcoords()
{
	if (rawVertexLayoutFlags & VERT_TEXCOORD0)
	{
		int texCoordIdx = 0;
		int texCoordShift = 24;

		uint64_t inputFlagsShifted = rawVertexLayoutFlags >> texCoordShift;
		do
		{
			inputFlagsShifted = rawVertexLayoutFlags >> texCoordShift;

			int8_t texCoordFormat = inputFlagsShifted & VERT_TEXCOORD_MASK;

			assertm(texCoordFormat == 2 || texCoordFormat == 0, "invalid texcoord format");

			if (texCoordFormat != 0)
			{
				texcoordCount++;
				texcoodIndices |= (1 << texCoordIdx);
			}

			texCoordShift += VERT_TEXCOORD_BITS;
			texCoordIdx++;
		} while (inputFlagsShifted >= (1 << VERT_TEXCOORD_BITS)); // while the flag value is large enough that there is more than just one 
	}
}

void ModelMeshData_t::ParseMaterial(ModelAsset* const modelAsset, const int materialIdx, const int meshid)
{
	UNUSED(meshid);
	// [rika]: handling mesh's material here since we've already looped through everything
	assertm(materialIdx < modelAsset->materials.size() && materialIdx >= 0, "invalid mesh material index");
	materialId = materialIdx;

	ModelMaterialData_t& matlData = modelAsset->materials.at(materialIdx);
	if (matlData.materialAsset)
		material = reinterpret_cast<MaterialAsset*>(matlData.materialAsset->extraData());
	//else
	//	Log("%s %d: null material\n", modelAsset->name, meshid);
}