#pragma once

#include <game/rtech/cpakfile.h>
#include <game/rtech/assets/texture.h>
#include <game/rtech/assets/material.h>
#include <core/mdl/animdata.h>

extern CBufferManager g_BufferManager;

//
// File contains data for exporting and storing 3D assets
//

// pre def structs
struct ModelMeshData_t;
class ModelParsedData_t;

struct VertexWeight_t
{
	float weight;
	int16_t bone;
};

struct VertexWeight_ForShader_t
{
	float weight;
	int bone;

	void operator=(VertexWeight_t& rhs)
	{
		weight = rhs.weight;
		bone = rhs.bone;
	}
};

#define VERT_PARSE_EXTRAWEIGHT	0x1
#define VERT_PARSE_BONES_1024	0x2

struct Vertex_t
{
	Vector position;
	Normal32 normalPacked;
	Color32 color;
	Vector2D texcoord; // texcoord0

	// [rika]: if this vertex buffer is used for rendering our shader doesn't support bones anyway.
	uint32_t weightCount : 8;
	uint32_t weightIndex : 24; // max weight count in a mesh is 1048576 (2^20), 24 bits gives plenty of headroom with a max value of 16777216 (2^24)

	uint64_t blendData; // opaque yippee!!!!! i fucking hate working on RSX

	Vertex_t(float x, float y, float z) : position(x, y, z), normalPacked(0), color(0xFF, 0xFF, 0xFF, 0xFF), texcoord(INFINITY, INFINITY), weightCount(0), weightIndex(0), blendData(0) {};

	static void ParseWeightFromVG_256(Vertex_t* const vert, VertexWeight_t* const weights, const char* const rawVertexData, const void* const boneMap, const vvw::mstudioboneweightextra_t* const weightExtra, const uint8_t parseFlags, int& weightIdx, int& offset);
	static void ParseWeightFromVG_1024(Vertex_t* const vert, VertexWeight_t* const weights, const char* const rawVertexData, const void* const boneMap, const vvw::mstudioboneweightextra_t* const weightExtra, const uint8_t parseFlags, int& weightIdx, int& offset);
	static bool ParseVertexFromVG(Vertex_t* const vert, VertexWeight_t* const weights, Vector2D* const texcoords, ModelMeshData_t* const mesh, const char* const rawVertexData, const void* const boneMap, const vvw::mstudioboneweightextra_t* const weightExtra, const uint8_t parseFlags, int& weightIdx);

	// Generic (basic data shared between them)
	static void ParseVertexFromVTX(Vertex_t* const vert, Vector2D* const texcoords, ModelMeshData_t* const mesh, const vvd::mstudiovertex_t* const pVerts, const Vector4D* const pTangs, const Color32* const pColors, const Vector2D* const pUVs, const int origId);

	// Basic Source
	static void ParseVertexFromVTX(Vertex_t* const vert, VertexWeight_t* const weights, Vector2D* const texcoords, ModelMeshData_t* mesh, const OptimizedModel::Vertex_t* const pVertex, const vvd::mstudiovertex_t* const pVerts, const Vector4D* const pTangs, const Color32* const pColors, const Vector2D* const pUVs,
		int& weightIdx, const bool isHwSkinned, const OptimizedModel::BoneStateChangeHeader_t* const pBoneStates);

	// Apex Legends
	static void ParseVertexFromVTX(Vertex_t* const vert, VertexWeight_t* const weights, Vector2D* const texcoords, ModelMeshData_t* mesh, const OptimizedModel::Vertex_t* const pVertex, const vvd::mstudiovertex_t* const pVerts, const Vector4D* const pTangs, const Color32* const pColors, const Vector2D* const pUVs,
		const vvw::vertexBoneWeightsExtraFileHeader_t* const pVVW, int& weightIdx);

	inline static void InvertTexcoord(Vector2D& texcoord)
	{
		texcoord.y = 1.0f - texcoord.y;
	}

	inline const Vector2D* const GetTexcoordForVertex(const uint32_t texcoordIdx, const uint32_t texcoordWidth, const Vector2D* const extraTexcoords, const uint32_t vertIdx) const
	{
		if (texcoordIdx == 0)
		{
			return &texcoord;
		}

		if (!extraTexcoords || texcoordIdx >= texcoordWidth)
		{
			return nullptr;
		}

		// only extra texcoords are stored, so subtract one for the base texcoord
		const uint32_t adjustedIndex = texcoordIdx - 1;
		const uint32_t adjustedWidth = texcoordWidth - 1;

		const Vector2D* const out = extraTexcoords + (vertIdx * adjustedWidth) + adjustedIndex;
		return out;
	}
};
static_assert(offsetof(Vertex_t, blendData) == 32);

//
// PARSEDDATA
//
struct ModelMeshData_t
{
	ModelMeshData_t() : meshVertexDataIndex(invalidNoodleIdx), rawVertexData(nullptr), rawVertexLayoutFlags(0ull), indexCount(0), vertCount(0), vertCacheSize(0),
		weightsPerVert(0), weightsCount(0), texcoordCount(0), texcoodIndices(0), materialId(0), materialAsset(nullptr),bodyPartIndex(-1), extraBoneWeights(nullptr), extraBoneWeightsSize(0) {};
	ModelMeshData_t(const ModelMeshData_t& mesh) : meshVertexDataIndex(mesh.meshVertexDataIndex), rawVertexData(mesh.rawVertexData), rawVertexLayoutFlags(mesh.rawVertexLayoutFlags), indexCount(mesh.indexCount), vertCount(mesh.vertCount), vertCacheSize(mesh.vertCacheSize),
		weightsPerVert(mesh.weightsPerVert), weightsCount(mesh.weightsCount), texcoordCount(mesh.texcoordCount), texcoodIndices(mesh.texcoodIndices), materialId(mesh.materialId), materialAsset(mesh.materialAsset), bodyPartIndex(mesh.bodyPartIndex), extraBoneWeights(nullptr), extraBoneWeightsSize(0) {
	};
	ModelMeshData_t(ModelMeshData_t& mesh) : meshVertexDataIndex(mesh.meshVertexDataIndex), rawVertexData(mesh.rawVertexData), rawVertexLayoutFlags(mesh.rawVertexLayoutFlags), indexCount(mesh.indexCount), vertCount(mesh.vertCount), vertCacheSize(mesh.vertCacheSize),
		weightsPerVert(mesh.weightsPerVert), weightsCount(mesh.weightsCount), texcoordCount(mesh.texcoordCount), texcoodIndices(mesh.texcoodIndices), materialId(mesh.materialId), materialAsset(mesh.materialAsset), bodyPartIndex(mesh.bodyPartIndex), extraBoneWeights(nullptr), extraBoneWeightsSize(0) {
	};

	~ModelMeshData_t()
	{
		// [rexx]: dw i'm uninstalling visual studio right now
		if (extraBoneWeights && extraBoneWeights != (char*)(0xcdcdcdcdcdcdcdcd))
			delete[] extraBoneWeights;
	};

	size_t meshVertexDataIndex;

	char* rawVertexData; // for model preview only
	uint64_t rawVertexLayoutFlags; // the flags from a 'vg' (baked hwData) mesh, identical to 'Vertex Layout Flags' (DX)

	uint32_t indexCount;

	uint32_t vertCount;
	uint16_t vertCacheSize;

	uint16_t weightsPerVert; // max number of weights per vertex
	uint32_t weightsCount; // the total number of weights in this mesh

	uint16_t texcoordCount;
	uint16_t texcoodIndices; // texcoord indices, e.g. texcoord0, texcoord2, etc

	// [rika]: swapped this to CPakAsset because in many cases the parsed asset would not exist yet
	int materialId; // the index of this material
	CPakAsset* materialAsset; // pointer to the material's asset (if loaded)

	char* extraBoneWeights;
	int64_t extraBoneWeightsSize;

	int bodyPartIndex;

	// gets the texcoord and indice based off rawVertexLayoutFlags
	void ParseTexcoords();
	void ParseMaterial(ModelParsedData_t* const parsed, const int material);

	inline MaterialAsset* const GetMaterialAsset() const { return reinterpret_cast<MaterialAsset* const>(materialAsset->extraData()); }

	void ParseMesh_VTX(const OptimizedModel::FileHeader_t* const pVTX, const int partIndex, const int flags)
	{
		// is this correct?
		rawVertexLayoutFlags |= (VERT_LEGACY | ((flags & STUDIOHDR_FLAGS_USES_VERTEX_COLOR) ? VERT_COLOR : 0x0));
		vertCacheSize = static_cast<uint16_t>(pVTX->vertCacheSize);

		// do we have a section texcoord
		rawVertexLayoutFlags |= (flags & STUDIOHDR_FLAGS_USES_UV2) ? VERT_TEXCOORDn_FMT(2, 0x2) : 0x0;

		// has to be parsed per strip
		vertCount = 0u;
		indexCount = 0u;

		bodyPartIndex = partIndex;

		// how to handle this here?
		extraBoneWeights = nullptr;
		extraBoneWeightsSize = 0;

		ParseTexcoords();
	}

	void ParseMesh_HW1(const vg::rev1::MeshHeader_t* pLODMesh, const int partIndex, const vg::rev1::VertexGroupHeader_t* const vgHdr)
	{
		rawVertexLayoutFlags |= pLODMesh->flags;

		vertCacheSize = static_cast<uint16_t>(pLODMesh->vertCacheSize);
		vertCount = pLODMesh->vertCount;
		indexCount = pLODMesh->indexCount;

		bodyPartIndex = partIndex;

		if (pLODMesh->extraBoneWeightSize)
		{
			char* ebw = new char[pLODMesh->extraBoneWeightSize];
			memcpy_s(ebw, pLODMesh->extraBoneWeightSize, pLODMesh->pBoneWeights(vgHdr), pLODMesh->extraBoneWeightSize);

			extraBoneWeights = ebw;
			extraBoneWeightsSize = pLODMesh->extraBoneWeightSize;
		}
		else
		{
			extraBoneWeights = nullptr;
			extraBoneWeightsSize = 0;
		}

		ParseTexcoords();
	}

	void ParseMesh_HW2(const vg::rev2::MeshHeader_t* pLODMesh, const int partIndex)
	{
		rawVertexLayoutFlags |= pLODMesh->flags;

		vertCacheSize = static_cast<uint16_t>(pLODMesh->vertCacheSize);
		vertCount = static_cast<uint32_t>(pLODMesh->vertCount);
		indexCount = static_cast<uint32_t>(pLODMesh->indexCount);

		bodyPartIndex = partIndex;

		if (pLODMesh->extraBoneWeightSize)
		{
			char* ebw = new char[pLODMesh->extraBoneWeightSize];
			memcpy_s(ebw, pLODMesh->extraBoneWeightSize, pLODMesh->pBoneWeights(), pLODMesh->extraBoneWeightSize);

			extraBoneWeights = ebw;
			extraBoneWeightsSize = pLODMesh->extraBoneWeightSize;
		}
		else
		{
			extraBoneWeights = nullptr;
			extraBoneWeightsSize = 0;
		}

		ParseTexcoords();
	}

	void ParseMesh_HW3(const vg::rev3::MeshHeader_t* pLODMesh, const int partIndex)
	{
		rawVertexLayoutFlags |= pLODMesh->flags;

		vertCacheSize = static_cast<uint16_t>(pLODMesh->vertCacheSize);
		vertCount = pLODMesh->vertCount;
		indexCount = pLODMesh->indexCount;

		bodyPartIndex = partIndex;

		if (pLODMesh->extraBoneWeightSize)
		{
			char* ebw = new char[pLODMesh->extraBoneWeightSize];
			memcpy_s(ebw, pLODMesh->extraBoneWeightSize, pLODMesh->pBoneWeights(), pLODMesh->extraBoneWeightSize);

			extraBoneWeights = ebw;
			extraBoneWeightsSize = pLODMesh->extraBoneWeightSize;
		}
		else
		{
			extraBoneWeights = nullptr;
			extraBoneWeightsSize = 0;
		}

		ParseTexcoords();
	}

	void ParseMesh_HW4(const vg::rev4::MeshHeader_t* pLODMesh, const uint16_t partIndex)
	{
		rawVertexLayoutFlags = pLODMesh->flags;

		vertCacheSize = pLODMesh->vertCacheSize;
		vertCount = pLODMesh->vertCount;
		indexCount = pLODMesh->indexCount;

		bodyPartIndex = partIndex;

		if (pLODMesh->extraBoneWeightSize)
		{
			char* ebw = new char[pLODMesh->extraBoneWeightSize];
			memcpy_s(ebw, pLODMesh->extraBoneWeightSize, pLODMesh->pBoneWeights(), pLODMesh->extraBoneWeightSize);

			extraBoneWeights = ebw;
			extraBoneWeightsSize = pLODMesh->extraBoneWeightSize;
		}
		else
		{
			extraBoneWeights = nullptr;
			extraBoneWeightsSize = 0;
		}

		ParseTexcoords();
	}
};

struct ModelModelData_t
{
	ModelModelData_t() : name(nullptr), nameInMem(false), meshes(nullptr), meshCount(0u), vertCount(0u) {}
	ModelModelData_t(ModelMeshData_t* const modelMeshes, uint32_t numMeshes) : name(nullptr), nameInMem(false), meshes(modelMeshes), meshCount(static_cast<uint16_t>(numMeshes)), vertCount(0u)
	{

	}
	~ModelModelData_t()
	{
		if (nameInMem)
		{
			FreeAllocArray(name);
		}
	}

	const char* name;
	ModelMeshData_t* meshes;
	uint16_t meshCount;

	bool nameInMem;

	uint32_t vertCount; // used to determine if this model is disabled in a LOD, as traditional methods (via VTX) will not work for most apex models

	ModelModelData_t& operator=(const ModelModelData_t&) = delete;
	ModelModelData_t& operator=(ModelModelData_t&& model) noexcept
	{
		if (this != &model)
		{
			name = model.name;
			meshes = model.meshes;
			meshCount = model.meshCount;

			nameInMem = model.nameInMem;

			vertCount = model.vertCount;

			model.name = nullptr;
			model.nameInMem = false;
		}

		return *this;
	}

	void GenerateName(const char* const part, const int localIndex, const int lodLevel)
	{
		constexpr size_t bufSize = 80ull;

		char nameBuf[bufSize];
		snprintf(nameBuf, bufSize, "%s_%i_LOD%i\0", part, localIndex, lodLevel);

		const size_t length = strnlen(nameBuf, bufSize) + 1ull;
		char* tmp = new char[length]{};
		strncpy_mem(tmp, length, nameBuf, bufSize);

		name = tmp;
	}
};

struct ModelHWGroup_t
{
	ModelHWGroup_t() = default;
	ModelHWGroup_t(const r5::studio_hw_groupdata_v16_t* const group) : dataOffset(group->dataOffset), dataSizeCompressed(group->dataSizeCompressed), dataSizeDecompressed(group->dataSizeDecompressed), dataCompression(group->dataCompression),
		lodIndex(group->lodIndex), lodCount(group->lodCount), lodMap(group->lodMap) {}
	ModelHWGroup_t(const r5::studio_hw_groupdata_v12_1_t* const group) : dataOffset(group->dataOffset), dataSizeCompressed(-1), dataSizeDecompressed(group->dataSize), dataCompression(eCompressionType::NONE),
		lodIndex(static_cast<uint8_t>(group->lodIndex)), lodCount(static_cast<uint8_t>(group->lodCount)), lodMap(static_cast<uint8_t>(group->lodMap)) {}

	int dataOffset;				// offset to this section in compressed vg
	int dataSizeCompressed;		// compressed size of this lod buffer in hwData
	int dataSizeDecompressed;	// decompressed size of this lod buffer in hwData

	eCompressionType dataCompression; // none and oodle, haven't seen anything else used.

	//
	uint8_t lodIndex;		// base lod idx?
	uint8_t lodCount;		// number of lods contained within this group
	uint8_t lodMap;		// lods in this group, each bit is a lod
};

struct ModelLODData_t
{
	ModelLODData_t() = default;
	~ModelLODData_t()
	{
		FreeAllocArray(models);
		FreeAllocArray(meshes);
	}

	ModelModelData_t* models;
	uint32_t numModels;

	uint32_t numMeshes;
	ModelMeshData_t* meshes;

	uint32_t vertexCount;
	uint32_t indexCount;
	float switchPoint;

	// for exporting
	uint16_t texcoordsPerVert; // max texcoords used in any mesh from this lod
	uint16_t weightsPerVert; // max weights used in any mesh from this lod

	inline const uint32_t GetMeshCount() const { return static_cast<int>(numMeshes); }
	inline const uint32_t GetModelCount() const { return static_cast<int>(numModels); }

	inline const ModelModelData_t* const pModel(const uint32_t i) const { return models + i; }
	inline const ModelMeshData_t* const pMesh(const uint32_t i) const { return meshes + i; }
	inline const ModelMeshData_t& Mesh(const uint32_t i) const { return meshes[i]; }

	void ParseLOD_VTX(const uint32_t modelCount, const uint32_t meshCount)
	{
		ParseLOD(0.0f, modelCount);

		numMeshes = meshCount;
		meshes = new ModelMeshData_t[numMeshes]{};
	}

	void ParseLOD_HW1(const vg::rev1::ModelLODHeader_t* const pLOD, const uint32_t modelCount, const vg::rev1::VertexGroupHeader_t* const vgHdr)
	{
		ParseLOD(pLOD->switchPoint, modelCount);

		numMeshes = 0u; // excludes empty ones, saves some memory
		for (uint8_t meshIdx = 0; meshIdx < pLOD->meshCount; meshIdx++)
		{
			if (pLOD->pMesh(vgHdr, meshIdx)->flags == 0)
			{
				continue;
			}

			numMeshes++;
		}

		meshes = new ModelMeshData_t[numMeshes]{};
	}

	void ParseLOD_HW2(const vg::rev2::ModelLODHeader_t* const pLOD, const uint32_t modelCount)
	{
		ParseLOD(pLOD->switchPoint, modelCount);

		numMeshes = 0u; // excludes empty ones, saves some memory
		for (uint8_t meshIdx = 0; meshIdx < pLOD->meshCount; meshIdx++)
		{
			if (pLOD->pMesh(meshIdx)->flags == 0)
			{
				continue;
			}

			numMeshes++;
		}

		meshes = new ModelMeshData_t[numMeshes]{};
	}

	// [rika]: this struct is the same
	void ParseLOD_HW3(const vg::rev3::ModelLODHeader_t* const pLOD, const uint32_t modelCount)
	{
		ParseLOD_HW2(reinterpret_cast<const vg::rev2::ModelLODHeader_t* const>(pLOD), modelCount);
	}

	void ParseLOD_HW4(const vg::rev4::ModelLODHeader_t* const pLOD, const float threshold, const uint32_t modelCount)
	{
		ParseLOD(threshold, modelCount);

		numMeshes = 0u; // excludes empty ones, saves some memory
		for (uint8_t meshIdx = 0; meshIdx < pLOD->meshCount; meshIdx++)
		{
			if (pLOD->pMesh(meshIdx)->flags == 0)
			{
				continue;
			}

			numMeshes++;
		}

		meshes = new ModelMeshData_t[numMeshes]{};
	}

	private:

		inline void ParseLOD(const float threshold, const uint32_t modelCount)
		{
			// set our lod up
			switchPoint = threshold;
			numModels = modelCount;
			models = new ModelModelData_t[numModels]{};

			vertexCount = 0u;
			indexCount = 0u;
			texcoordsPerVert = 0u;
			weightsPerVert = 0u;
		}
};

struct ModelBone_t
{
	ModelBone_t() = default;

	ModelBone_t(const r1::mstudiobone_t* const bone) : name(bone->pszName()), parent(bone->parent), flags(bone->flags), proctype(bone->proctype), procBone(bone->pProcedure()), physicsbone(bone->physicsbone), surfaceProp(bone->pszSurfaceProp()), contents(bone->contents),
		poseToBone(&bone->poseToBone), pos(bone->pos), quat(bone->quat), rot(bone->rot), scale(bone->scale) {};

	ModelBone_t(const r2::mstudiobone_t* const bone) : name(bone->pszName()), parent(bone->parent), flags(bone->flags), proctype(bone->proctype), procBone(bone->pProcedure()), physicsbone(bone->physicsbone), surfaceProp(bone->pszSurfaceProp()), contents(bone->contents),
		poseToBone(&bone->poseToBone), pos(bone->pos), quat(bone->quat), rot(bone->rot), scale(bone->scale) {};

	ModelBone_t(const r5::mstudiobone_v8_t* const bone) : name(bone->pszName()), parent(bone->parent), flags(bone->flags), proctype(bone->proctype), procBone(bone->pProcedure()), physicsbone(bone->physicsbone), surfaceProp(bone->pszSurfaceProp()), contents(bone->contents),
		poseToBone(&bone->poseToBone), pos(bone->pos), quat(bone->quat), rot(bone->rot), scale(bone->scale) {};

	ModelBone_t(const r5::mstudiobone_v12_1_t* const bone) : name(bone->pszName()), parent(bone->parent), flags(bone->flags), proctype(bone->proctype), procBone(bone->pProcedure()), physicsbone(bone->physicsbone), surfaceProp(bone->pszSurfaceProp()), contents(bone->contents),
		poseToBone(&bone->poseToBone), pos(bone->pos), quat(bone->quat), rot(bone->rot), scale(bone->scale) {};

	ModelBone_t(const r5::mstudiobonehdr_v16_t* const pBoneHdr, const r5::mstudiobonedata_v16_t* const pBoneData) : name(pBoneHdr->pszName()), parent(pBoneData->parent), flags(pBoneData->flags), proctype(pBoneData->proctype), procBone(pBoneData->pProcedure()),
		physicsbone(pBoneHdr->physicsbone), surfaceProp(pBoneHdr->pszSurfaceProp()), contents(pBoneHdr->contents),
		poseToBone(&pBoneData->poseToBone), pos(pBoneData->pos), quat(pBoneData->quat), rot(pBoneData->rot), scale(pBoneData->scale) {}

	ModelBone_t(const r5::mstudiobonehdr_v16_t* const pBoneHdr, const r5::mstudiobonedata_v19_t* const pBoneData, const r5::mstudiolinearbone_v19_t* const linearbone, const int bone) : name(pBoneHdr->pszName()), parent(pBoneData->parent), flags(pBoneData->flags), proctype(pBoneData->proctype), procBone(pBoneData->pProcedure()),
		physicsbone(pBoneHdr->physicsbone), surfaceProp(pBoneHdr->pszSurfaceProp()), contents(pBoneHdr->contents),
		poseToBone(linearbone->pPoseToBone(bone)), pos(*linearbone->pPos(bone)), quat(*linearbone->pQuat(bone)), rot(*linearbone->pRot(bone)), scale(*linearbone->pScale(bone)) {}

	const char* name;
	inline const char* const pszName() const { return name; }

	int parent;

	int physicsbone; // index into physically simulated bone
	int flags;
	int proctype;
	const void* procBone; // procedural rule offset
	inline const void* const pProcedure() const { return procBone; };

	const char* surfaceProp; // index into string tablefor property name
	inline const char* const pszSurfaceProp() const { return surfaceProp; }

	int contents; // See BSPFlags.h for the contents flags

	const Vector pos;
	const Quaternion quat;
	const RadianEuler rot;
	const Vector scale;

	const matrix3x4_t* poseToBone; // use a pointer for this type since it's very large

	ModelBone_t& operator=(const ModelBone_t& bone)
	{
		memcpy_s(this, sizeof(ModelBone_t), &bone, sizeof(ModelBone_t));

		return *this;
	}
};

struct ModelAttachment_t
{
	ModelAttachment_t() = default;
	ModelAttachment_t(const mstudioattachment_t* const attachment) : name(attachment->pszName()), flags(attachment->flags), localbone(attachment->localbone), localmatrix(&attachment->local) {}
	ModelAttachment_t(const r5::mstudioattachment_v8_t* const attachment) : name(attachment->pszName()), flags(attachment->flags), localbone(attachment->localbone), localmatrix(&attachment->local) {}
	ModelAttachment_t(const r5::mstudioattachment_v16_t* const attachment) : name(attachment->pszName()), flags(attachment->flags), localbone(attachment->localbone), localmatrix(&attachment->local) {}

	const char* name;
	int flags;

	int localbone;
	const matrix3x4_t* localmatrix;
};

struct ModelHitbox_t
{
	ModelHitbox_t() = default;
	ModelHitbox_t(const mstudiobbox_t* const bbox) : bone(bbox->bone), group(bbox->group), bbmin(&bbox->bbmin), bbmax(&bbox->bbmax), name(bbox->pszHitboxName()), forceCritPoint(0) {}
	ModelHitbox_t(const r2::mstudiobbox_t* const bbox) : bone(bbox->bone), group(bbox->group), bbmin(&bbox->bbmin), bbmax(&bbox->bbmax), name(bbox->pszHitboxName()), forceCritPoint(bbox->forceCritPoint) {}
	ModelHitbox_t(const r5::mstudiobbox_v8_t* const bbox) : bone(bbox->bone), group(bbox->group), bbmin(&bbox->bbmin), bbmax(&bbox->bbmax), name(bbox->pszHitboxName()), forceCritPoint(bbox->forceCritPoint) {}
	ModelHitbox_t(const r5::mstudiobbox_v16_t* const bbox) : bone(bbox->bone), group(bbox->group), bbmin(&bbox->bbmin), bbmax(&bbox->bbmax), name(bbox->pszHitboxName()), forceCritPoint(0) {}

	int bone;
	int group;
	const Vector* bbmin;
	const Vector* bbmax;
	const char* name;

	int forceCritPoint;
};

struct ModelHitboxSet_t
{
	ModelHitboxSet_t() = default;
	ModelHitboxSet_t(const mstudiohitboxset_t* const hitboxset, const mstudiobbox_t* const bboxes) : name(hitboxset->pszName()), hitboxes(nullptr), numHitboxes(hitboxset->numhitboxes)
	{
		if (!numHitboxes)
			return;

		hitboxes = new ModelHitbox_t[numHitboxes]{};

		for (int i = 0; i < numHitboxes; i++)
		{
			hitboxes[i] = ModelHitbox_t(bboxes + i);
		}
	};
	ModelHitboxSet_t(const mstudiohitboxset_t* const hitboxset, const r2::mstudiobbox_t* const bboxes) : name(hitboxset->pszName()), hitboxes(nullptr), numHitboxes(hitboxset->numhitboxes)
	{
		if (!numHitboxes)
			return;

		hitboxes = new ModelHitbox_t[numHitboxes]{};

		for (int i = 0; i < numHitboxes; i++)
		{
			hitboxes[i] = ModelHitbox_t(bboxes + i);
		}
	};
	ModelHitboxSet_t(const mstudiohitboxset_t* const hitboxset, const r5::mstudiobbox_v8_t* const bboxes) : name(hitboxset->pszName()), hitboxes(nullptr), numHitboxes(hitboxset->numhitboxes)
	{
		if (!numHitboxes)
			return;

		hitboxes = new ModelHitbox_t[numHitboxes]{};

		for (int i = 0; i < numHitboxes; i++)
		{
			hitboxes[i] = ModelHitbox_t(bboxes + i);
		}
	};
	ModelHitboxSet_t(const r5::mstudiohitboxset_v16_t* const hitboxset) : name(hitboxset->pszName()), hitboxes(nullptr), numHitboxes(hitboxset->numhitboxes)
	{
		if (!numHitboxes)
			return;

		hitboxes = new ModelHitbox_t[numHitboxes]{};

		for (int i = 0; i < numHitboxes; i++)
		{
			hitboxes[i] = ModelHitbox_t(hitboxset->pHitbox(i));
		}
	};

	~ModelHitboxSet_t()
	{
		FreeAllocArray(hitboxes);
	}

	ModelHitboxSet_t& operator=(const ModelHitboxSet_t&) = delete;
	ModelHitboxSet_t& operator=(ModelHitboxSet_t&& set) noexcept
	{
		if (this != &set)
		{
			name = set.name;
			hitboxes = set.hitboxes;
			numHitboxes = set.numHitboxes;

			set.hitboxes = nullptr;
		}

		return *this;
	}

	const char* name;
	ModelHitbox_t* hitboxes;
	int numHitboxes;
	inline const ModelHitbox_t* const pHitbox(const int i) const { return hitboxes + i; }
};

struct ModelMaterialData_t
{
	~ModelMaterialData_t()
	{
		FreeAllocArray(stored);
	}

	// pointer to the referenced material asset
	CPakAsset* asset;

	// also store guid and name just in case the material is not loaded
	uint64_t guid;
	const char* name;
	const char* stored;

	inline MaterialAsset* const GetMaterialAsset() const { return asset ? reinterpret_cast<MaterialAsset* const>(asset->extraData()) : nullptr; }

	// use 'true' to prefer studio model name, use 'false' for material asset name
	const char* const GetName(const bool biasStudio) const
	{
		// [rika]: if we don't prefer studio name, or studio name is generated, return material name if it exists
		if ((!biasStudio || IsAllocated()) && GetMaterialAsset())
			return GetMaterialAsset()->name;

		// [rika]: prefer studio name, or return generated name if the material did not exist
		return name;
	}

	// [rika]: originally had this as one function but it's wasted performance to check in cases it'd never happen
	inline void SetName(const char* const str)
	{
		name = str;
		stored = nullptr;
	}

	inline void StoreName(const char* const str)
	{
		const size_t length = strnlen_s(str, MAX_PATH) + 1;
		char* buf = new char[length] {};
		strncpy_s(buf, length, str, length - 1);

		stored = buf;
		name = stored;
	}

	inline const bool IsAllocated() const { return stored ? true : false; }
};

struct ModelSkinData_t
{
	ModelSkinData_t() = default;
	ModelSkinData_t(const char* nameIn, const int16_t* indiceIn) : name(nameIn), indices(indiceIn) {};

	const char* name;
	const int16_t* indices;
};

struct ModelBodyPart_t
{
	ModelBodyPart_t() : name(nullptr), modelIndex(-1), numModels(0) {};
	ModelBodyPart_t(const char* const bodypart, const int modelIdx, const int modelCount) : name(bodypart), modelIndex(modelIdx), numModels(modelCount) {};

	const char* name;

	int modelIndex;
	int numModels;

	inline const char* const GetName() const { return name; }
	inline const int GetModelCount() const { return numModels; }
};

struct ModelPoseParam_t
{
	ModelPoseParam_t() : name(nullptr), flags(0), start(0.0f), end(0.0f), loop(0.0f) {}
	ModelPoseParam_t(const mstudioposeparamdesc_t* const poseparam) : name(poseparam->pszName()), flags(poseparam->flags), start(poseparam->start), end(poseparam->end), loop(poseparam->loop) {}
	ModelPoseParam_t(const r5::mstudioposeparamdesc_v16_t* const poseparam) : name(poseparam->pszName()), flags(poseparam->flags), start(poseparam->start), end(poseparam->end), loop(poseparam->loop) {}

	const char* name;

	int flags;
	float start;
	float end;
	float loop;
};

struct ModelIKLock_t
{
	ModelIKLock_t() = default;
	ModelIKLock_t(const mstudioiklock_t* const iklock) : chain(iklock->chain), flPosWeight(iklock->flPosWeight), flLocalQWeight(iklock->flLocalQWeight), flags(iklock->flags) {}
	ModelIKLock_t(const r2::mstudioiklock_t* const iklock) : chain(iklock->chain), flPosWeight(iklock->flPosWeight), flLocalQWeight(iklock->flLocalQWeight), flags(iklock->flags) {}
	ModelIKLock_t(const r5::mstudioiklock_v8_t* const iklock) : chain(iklock->chain), flPosWeight(iklock->flPosWeight), flLocalQWeight(iklock->flLocalQWeight), flags(iklock->flags) {}
	ModelIKLock_t(const r5::mstudioiklock_v16_t* const iklock) : chain(iklock->chain), flPosWeight(iklock->flPosWeight), flLocalQWeight(iklock->flLocalQWeight), flags(iklock->flags) {}

	int chain;
	float flPosWeight;
	float flLocalQWeight;
	int flags;
};

struct ModelIKLink_t
{
	ModelIKLink_t() = default;
	ModelIKLink_t(const mstudioiklink_t* const iklink) : bone(iklink->bone), kneeDir(iklink->kneeDir) {}
	ModelIKLink_t(const r2::mstudioiklink_t* const iklink) : bone(iklink->bone), kneeDir(iklink->kneeDir) {}
	ModelIKLink_t(const r5::mstudioiklink_v8_t* const iklink) : bone(iklink->bone), kneeDir(iklink->kneeDir) {}
	ModelIKLink_t(const r5::mstudioiklink_v16_t* const iklink) : bone(iklink->bone), kneeDir(iklink->kneeDir) {}

	int bone;
	Vector kneeDir;
};

struct ModelIKChain_t
{
	ModelIKChain_t() = default;
	ModelIKChain_t(const mstudioikchain_t* const ikchain) : name(ikchain->pszName()), unk_10(0.0f)
	{
		assertm(ikchain->numlinks == 3, "ikchain was abnormal");
		assertm(ikchain->linktype == 0, "ikchain was abnormal");

		links[0] = ModelIKLink_t(ikchain->pLink(0));
		links[1] = ModelIKLink_t(ikchain->pLink(1));
		links[2] = ModelIKLink_t(ikchain->pLink(2));
	}
	ModelIKChain_t(const r2::mstudioikchain_t* const ikchain) : name(ikchain->pszName()), unk_10(ikchain->unk_10)
	{
		assertm(ikchain->numlinks == 3, "ikchain was abnormal");
		assertm(ikchain->linktype == 0, "ikchain was abnormal");

		links[0] = ModelIKLink_t(ikchain->pLink(0));
		links[1] = ModelIKLink_t(ikchain->pLink(1));
		links[2] = ModelIKLink_t(ikchain->pLink(2));
	}
	ModelIKChain_t(const r5::mstudioikchain_v8_t* const ikchain) : name(ikchain->pszName()), unk_10(ikchain->unk_10)
	{
		assertm(ikchain->numlinks == 3, "ikchain was abnormal");
		assertm(ikchain->linktype == 0, "ikchain was abnormal");

		links[0] = ModelIKLink_t(ikchain->pLink(0));
		links[1] = ModelIKLink_t(ikchain->pLink(1));
		links[2] = ModelIKLink_t(ikchain->pLink(2));
	}
	ModelIKChain_t(const r5::mstudioikchain_v16_t* const ikchain) : name(ikchain->pszName()), unk_10(ikchain->unk_10)
	{
		assertm(ikchain->numlinks == 3, "ikchain was abnormal");
		assertm(ikchain->linktype == 0, "ikchain was abnormal");

		links[0] = ModelIKLink_t(ikchain->pLink(0));
		links[1] = ModelIKLink_t(ikchain->pLink(1));
		links[2] = ModelIKLink_t(ikchain->pLink(2));
	}

	enum LinkType_t
	{
		IKLINK_THIGH,
		IKLINK_KNEE,
		IKLINK_FOOT,

		IKLINK_COUNT
	};

	const char* name;
	float unk_10;

	// while this could be dynamic, it's hardcoded throughout all of source and reSource to assume it's 3
	// I'd imagine the original intent was to add more types with varied links, but that never happened
	ModelIKLink_t links[IKLINK_COUNT];
};

// [rika]: cool stuff to eliminate storing the indices!
constexpr int STORE_INDEX_FLAG = 1 << 31;
constexpr int STORE_INDEX_MASK = ~STORE_INDEX_FLAG;
#define INDEX_STORE_16_OFS(ptr, num, srcIndex, srcOfs, srcNum, idx) assertm(idx >= 0 && idx < 2, "out of bounds"); reinterpret_cast<uint32_t*>(&ptr)[idx] = (FIX_OFFSET(srcIndex) + srcOfs); num = srcNum // [rika]: because of fix offset, it will give bad values otherwise
#define INDEX_STORE_16(ptr, num, srcIndex, srcNum, idx) assertm(idx >= 0 && idx < 2, "out of bounds"); reinterpret_cast<uint32_t*>(&ptr)[idx] = FIX_OFFSET(srcIndex); num = srcNum
#define INDEX_STORE_32(ptr, num, srcIndex, srcNum, idx) assertm(idx >= 0 && idx < 2, "out of bounds"); reinterpret_cast<uint32_t*>(&ptr)[idx] = srcIndex; num = srcNum
#define INDEX_GET(ptr, idx) reinterpret_cast<uint32_t*>(&ptr)[idx]
#define INDEX_TO_PTR(ptr, num, data) ptr = new data[num]{} 
#define INDEX_TO_NULL(ptr) ptr = nullptr 

class ModelParsedData_t
{
public:
	ModelParsedData_t() = default;
	ModelParsedData_t(const r1::studiohdr_t* const pHdr, const AssetVersion_t& fileVersion, StudioLooseData_t* const looseData);
	ModelParsedData_t(const r2::studiohdr_t* const pHdr, const AssetVersion_t& fileVersion);
	ModelParsedData_t(const r5::studiohdr_v8_t* const pHdr, const AssetVersion_t& fileVersion);
	ModelParsedData_t(const r5::studiohdr_v12_1_t* const pHdr, const AssetVersion_t& fileVersion);
	ModelParsedData_t(const r5::studiohdr_v12_2_t* const pHdr, const AssetVersion_t& fileVersion);
	ModelParsedData_t(const r5::studiohdr_v12_4_t* const pHdr, const AssetVersion_t& fileVersion);
	ModelParsedData_t(const r5::studiohdr_v14_t* const pHdr, const AssetVersion_t& fileVersion);
	ModelParsedData_t(const r5::studiohdr_v16_t* const pHdr, const AssetVersion_t& fileVersion, const int dataSizePhys, const int dataSizeModel);
	ModelParsedData_t(const r5::studiohdr_v17_t* const pHdr, const AssetVersion_t& fileVersion, const int dataSizePhys, const int dataSizeModel);
	ModelParsedData_t(const r5::studiohdr_v19_2_t* const pHdr, const AssetVersion_t& fileVersion, const int dataSizePhys, const int dataSizeModel);

	~ModelParsedData_t()
	{
		FreeAllocArray(bones);
		FreeAllocArray(attachments);
		FreeAllocArray(hitboxSets);

		FreeAllocArray(materials);
		FreeAllocArray(skins);
		FreeAllocArray(cdMaterials);

		FreeAllocArray(hwGroups);
		FreeAllocArray(lods);
		FreeAllocArray(bodyparts);

		FreeAllocArray(localSequences);
		FreeAllocArray(localNodeNames);
		FreeAllocArray(poseParams);
		FreeAllocArray(ikChains);
		FreeAllocArray(ikLocks);
	}

	/*ModelParsedData_t& operator=(ModelParsedData_t&& parsed)
	{
		if (this != &parsed)
		{
			this->meshVertexData.move(parsed.meshVertexData);
			this->bones.swap(parsed.bones);
			this->attachments.swap(parsed.attachments);
			this->hitboxsets.swap(parsed.hitboxsets);

			this->lods.swap(parsed.lods);
			this->materials.swap(parsed.materials);
			this->skins.swap(parsed.skins);

			this->bodyParts.swap(parsed.bodyParts);

			localSequences = parsed.localSequences;
			numLocalSequences = parsed.numLocalSequences;
			numExternalSequences = parsed.numExternalSequences;
			externalSequences = parsed.externalSequences;

			externalIncludeModels = parsed.externalIncludeModels;
			numExternalIncludeModels = parsed.numExternalIncludeModels;
			numLocalNodes = parsed.numLocalNodes;
			localNodeNames = parsed.localNodeNames;

			this->poseparams = parsed.poseparams;
			this->ikchains = parsed.ikchains;
			this->iklocks = parsed.iklocks;

			parsed.localSequences = nullptr;
			parsed.externalSequences = nullptr;

			parsed.poseparams = nullptr;
			parsed.ikchains = nullptr;
			parsed.iklocks = nullptr;

			this->studiohdr = parsed.studiohdr;
		}

		return *this;
	}*/

	const char* baseptr;
	AssetVersion_t version;

	int length;
	int flags;
	inline const bool IsStaticProp() const { return (flags & STUDIOHDR_FLAGS_STATIC_PROP) ? true : false; }

	const char* name; // The internal name of the model, padding with null chars. last byte always null

	float mass;
	int contents;

	Vector eyeposition;		// ideal eye position
	Vector illumposition;	// illumination center

	Vector hull_min;	// ideal movement hull size
	Vector hull_max;	// ideal movement hull size

	Vector view_bbmin;	// clipping bounding box
	Vector view_bbmax;	// clipping bounding box

	const char* linearBone;
	ModelBone_t* bones;
	int numBones;
	inline const int BoneCount() const { return numBones; }
	inline const ModelBone_t* const pBone(const int i) const { return bones + i; }

	int numSrcBoneTransforms;
	const mstudiosrcbonetransform_t* srcBoneTransforms;

	ModelAttachment_t* attachments;
	int numAttachments;
	inline const ModelAttachment_t* const pAttachment(const size_t i) const { return attachments + i; }

	int numHitboxSets;
	ModelHitboxSet_t* hitboxSets;
	inline const int HitboxSetCount() const { return numHitboxSets; }
	inline const ModelHitboxSet_t* const pHitboxSet(const size_t i) const { return hitboxSets + i; }

	ModelLODData_t* lods;
	int numLODs;
	inline const int LODCount() const { return numLODs; }
	inline const ModelLODData_t* const pLOD(const size_t i) const { return lods + i; }

	int numHwGroups;
	ModelHWGroup_t* hwGroups;
	inline const int GroupCount() const { return numHwGroups; }
	inline const ModelHWGroup_t* const pLODGroup(const int i) const { return hwGroups + i; }

	const void* boneStates; // uint8_t or uint16_t
	int numBoneStates;

	int numBodyparts;
	ModelBodyPart_t* bodyparts;
	inline const int BodypartCount() const { return numBodyparts; }
	inline const ModelBodyPart_t* const pBodypart(const size_t i) const { return bodyparts + i; }

	ModelMaterialData_t* materials;
	int numMaterials;
	inline const int MaterialCount() const { return numMaterials; }
	inline const ModelMaterialData_t* const pMaterial(const int i) const { return materials + i; }

	int numSkins;
	ModelSkinData_t* skins;
	inline const int SkinCount() const { return numSkins; }
	inline const ModelSkinData_t* const pSkin(const int i) const { return skins + i; }

	inline const int16_t* const pSkinref(const int i, int skinindex) const { return reinterpret_cast<const int16_t* const>(baseptr + skinindex) + i; }
	inline const int16_t* const pSkinFamily(const int i, int skinindex) const { return pSkinref(numMaterials * i, skinindex); };
	template<typename T>
	const char* const pSkinName(const int i, int skinindex) const
	{
		// only stored for index 1 and up
		// [rika]: in code this actually returns '\0'
		if (i == 0)
		{
			return STUDIO_DEFAULT_SKIN_NAME;
		}

		const T skinnameindex = *(reinterpret_cast<const T* const>(pSkinFamily(numSkins, skinindex)) + (i - 1));
		const char* const skinname = baseptr + FIX_OFFSET(skinnameindex);

		if (IsStringZeroLength(skinname))
		{
			return STUDIO_NULL_SKIN_NAME;
		}

		return skinname;
	}

	const char** cdMaterials;
	int numCdMaterials;
	inline const int CDMaterialCount() const { return numCdMaterials; }
	inline const char* const CDMaterial(const int i) const { return cdMaterials[i]; }

	int numLocalSequences;
	ModelSeq_t* localSequences;
	inline const int LocalSeqCount() const { return numLocalSequences; }
	inline const ModelSeq_t* const pLocalSeq(const int i) const { return localSequences + i; }

	const AssetGuid_t* externalSequences;
	int numExternalSequences;
	inline const int ExternalSeqCount() const { return numExternalSequences; }

	int numExternalIncludeModels;
	const AssetGuid_t* externalIncludeModels;

	const mstudiomodelgroup_t* includeModels;
	int numIncludeModels;
	inline const int IncludeModelCount() const { return numIncludeModels; }
	inline const mstudiomodelgroup_t* const pIncludeModel(const int i) const { return includeModels + i; }

	int numLocalNodes;
	const char** localNodeNames;
	inline const int NodeCount() const { return numLocalNodes; }
	inline const char* const pszNodeName(const int i) const { return localNodeNames[i]; }

	ModelPoseParam_t* poseParams;
	int numPoseParm;
	inline const int PoseParamCount() const { return numPoseParm; }
	inline const ModelPoseParam_t* const pPoseParam(const int i) const { return poseParams + i; }

	int numIkChains;
	ModelIKChain_t* ikChains;
	inline const int IKChainCount() const { return numIkChains; };
	inline const ModelIKChain_t* const pIKChain(const int i) const { return ikChains + i; }

	ModelIKLock_t* ikLocks;
	int numIkLocks;
	inline const int IKLockCount() const { return numIkLocks; }
	inline const ModelIKLock_t* const pIKLock(const int i) const { return ikLocks + i; }

	uint8_t constdirectionallightdot;
	uint8_t rootLOD;
	uint8_t numAllowedRootLODs;

	uint8_t pad;

	float fadeDistance; // set to -1 to never fade. set above 0 if you want it to fade out, distance is in feet.
	float gatherSize;

	int	illumpositionattachmentindex;

	float flMaxEyeDeflection;

	const char* surfaceProp;	// offset to surface prop string

	const char* keyValues;		// offset to keyvalues
	int keyValueSize;		// removed in later rmdl, keyvalues are null terminated

	int vtxOffset; // VTX
	int vvdOffset; // VVD / IDSV
	int vvcOffset; // VVC / IDCV
	int vvwOffset; // index will come last after other vertex files
	int phyOffset; // VPHY / IVPS

	int vtxSize;
	int vvdSize;
	int vvcSize;
	int vvwSize;
	int phySize; // still used in models using vg

	size_t hwDataSize;

	const void* bvhData;

	CRamen meshVertexData;

	ModelParsedData_t& operator=(const ModelParsedData_t&) = delete;
	ModelParsedData_t& operator=(ModelParsedData_t&& parsed) noexcept
	{
		if (this != &parsed)
		{
			memcpy_s(this, sizeof(ModelParsedData_t), &parsed, sizeof(ModelParsedData_t));

			parsed.bones = nullptr;
			parsed.attachments = nullptr;
			parsed.hitboxSets = nullptr;
			parsed.materials = nullptr;
			parsed.skins = nullptr;

			parsed.lods = nullptr;
			parsed.bodyparts = nullptr;
			parsed.hwGroups = nullptr;

			parsed.localSequences = nullptr;
			parsed.localNodeNames = nullptr;

			parsed.poseParams = nullptr;
			parsed.ikChains = nullptr;
			parsed.ikLocks = nullptr;
		}

		return *this;
	}

	template<typename mstudiobone_t> void ParseModelBoneData();
	void ParseModelBoneData_v16();
	void ParseModelBoneData_v19();

	template<typename mstudioattachment> void ParseModelAttachmentData();

	template<typename mstudiobbox_t> void ParseModelHitboxData();
	void ParseModelHitboxData_v16();

	void ParseModelTextureData_v8();
	void ParseModelTextureData_v16();

	template<typename mstudiomodel_t, typename mstudiomesh_t> void ParseModelVertexData_VTX(StudioLooseData_t* const looseData);
	void ParseModelVertexData_v9(const char* const vertexData);
	void ParseModelVertexData_v12_1(const char* const vertexData);
	void ParseModelVertexData_v14(const char* const vertexData);
	void ParseModelVertexData_v16(const char* const vertexData, const uint8_t parseFlags = 0x0);

	void ParseModelAnimTypes_V8();
	void ParseModelAnimTypes_V16();

	// [rika]: this is for model internal sequence data (r5)
	void ParseModelSequenceData_NoStall();
	void ParseModelSequenceData_Stall_V8();
	void ParseModelSequenceData_Stall_V16();
	void ParseModelSequenceData_Stall_V18();
	void ParseModelSequenceData_Stall_V19_1(const uint32_t flagWidth);

private:
	void GetHWBuffer(char* const dcmpBuf, const char* const hwBuf, const int groupIndex);
	void ParseMeshData_VTX(ModelMeshData_t* const pMeshData, Vertex_t* const parseVertices, Vector2D* const parseTexcoords, uint16_t* const parseIndices, VertexWeight_t* const parseWeights, char* const meshBuffer);
	void ParseHWVertices(ModelMeshData_t* const pMeshData, char* const meshParseBuffer, const char* const vertexData, const vvw::mstudioboneweightextra_t* const extraWeightData, const uint16_t* const indiceData, const int material, const uint8_t vertexParseFlags);
};

// bones
template<typename mstudiobone_t>
void ModelParsedData_t::ParseModelBoneData()
{
	if (numBones == 0)
	{
		INDEX_TO_NULL(bones);

		return;
	}

	const r5::mstudiobone_v8_t* const pBones = reinterpret_cast<const r5::mstudiobone_v8_t* const>(baseptr + INDEX_GET(bones, 0));

	INDEX_TO_PTR(bones, numBones, ModelBone_t);

	for (uint16_t i = 0; i < numBones; i++)
	{
		bones[i] = ModelBone_t(pBones + i);
	}
}

template<typename mstudioattachment_t>
void ModelParsedData_t::ParseModelAttachmentData()
{
	if (numAttachments == 0)
	{
		INDEX_TO_NULL(attachments);

		return;
	}

	const mstudioattachment_t* const pAttachments = reinterpret_cast<const mstudioattachment_t* const>(baseptr + INDEX_GET(attachments, 0));

	INDEX_TO_PTR(attachments, numAttachments, ModelAttachment_t);

	for (int i = 0; i < numAttachments; i++)
	{
		attachments[i] = ModelAttachment_t(pAttachments + i);
	}
}

template<typename mstudiobbox_t>
void ModelParsedData_t::ParseModelHitboxData()
{
	if (numHitboxSets == 0)
	{
		INDEX_TO_NULL(hitboxSets);

		return;
	}

	const mstudiohitboxset_t* const pHitboxSets = reinterpret_cast<const mstudiohitboxset_t* const>(baseptr + INDEX_GET(hitboxSets, 0));

	INDEX_TO_PTR(hitboxSets, numHitboxSets, ModelHitboxSet_t);

	for (int i = 0; i < numHitboxSets; i++)
	{
		hitboxSets[i] = ModelHitboxSet_t(pHitboxSets + i, pHitboxSets[i].pHitbox<mstudiobbox_t>(0));
	}
}

// vertex
template<typename mstudiomodel_t, typename mstudiomesh_t>
void ModelParsedData_t::ParseModelVertexData_VTX(StudioLooseData_t* const looseData)
{
	const OptimizedModel::FileHeader_t* const pVTX = looseData->GetVTX();
	const vvd::vertexFileHeader_t* const pVVD = looseData->GetVVD();
	const vvc::vertexColorFileHeader_t* const pVVC = looseData->GetVVC();
	const vvw::vertexBoneWeightsExtraFileHeader_t* const pVVW = looseData->GetVVW();

	numLODs = pVTX->numLODs;

	INDEX_TO_NULL(hwGroups);

	if (numLODs == 0 || numBodyparts == 0)
	{
		assertm(false, "model without LODs");

		INDEX_TO_NULL(lods);
		INDEX_TO_NULL(bodyparts);

		return;
	}

	// no valid vertex data
	if (!pVTX || !pVVD)
		return;

	if (looseData->VerifyFileIntegrity(reinterpret_cast<const studiohdr_short_t* const>(baseptr)) == false)
	{
		assertm(false, "loose data had mismatched files");
		return;
	}

	// [rika]: handle bodypart parsing and setup lods
	const mstudiobodyparts_t* const pBodyparts = reinterpret_cast<const mstudiobodyparts_t* const>(baseptr + INDEX_GET(bodyparts, 0));

	INDEX_TO_PTR(bodyparts, numBodyparts, ModelBodyPart_t);

	uint32_t numModels = 0u;
	uint32_t numMeshes = 0u;

	for (int i = 0; i < numBodyparts; i++)
	{
		const mstudiobodyparts_t* const pBodypart = pBodyparts + i;

		bodyparts[i] = ModelBodyPart_t(pBodypart->pszName(), numModels, pBodypart->nummodels);

		numModels += bodyparts[i].numModels;

		for (int j = 0; j < pBodypart->nummodels; j++)
		{
			const mstudiomodel_t* const pModel = pBodypart->pModel<mstudiomodel_t>(j);

			numMeshes += pModel->nummeshes;
		}
	}

	INDEX_TO_PTR(lods, numLODs, ModelLODData_t);

	CManagedBuffer* const meshBuffer = g_BufferManager.ClaimBuffer();

	// [rika]: fixed sizes per vertex
	constexpr size_t maxVertexDataSize = sizeof(vvd::mstudiovertex_t) + sizeof(Vector4D) + sizeof(Vector2D) + sizeof(Color32);
	constexpr size_t maxVertexBufferSize = maxVertexDataSize * s_MaxStudioVerts;

	// needed due to how vtx is parsed!
	CManagedBuffer* const   parseBuf = g_BufferManager.ClaimBuffer();

	Vertex_t* const         parseVertices = reinterpret_cast<Vertex_t*>         (parseBuf->Buffer() + maxVertexBufferSize);
	Vector2D* const         parseTexcoords = reinterpret_cast<Vector2D*>        (&parseVertices[s_MaxStudioVerts]);
	uint16_t* const         parseIndices = reinterpret_cast<uint16_t*>          (&parseTexcoords[s_MaxStudioVerts * 2]);
	VertexWeight_t* const   parseWeights = reinterpret_cast<VertexWeight_t*>    (&parseIndices[s_MaxStudioTriIndices]); // ~8mb for weights

	for (uint16_t lodLevel = 0; lodLevel < numLODs; lodLevel++)
	{
		ModelLODData_t* const pLODData = lods + lodLevel;

		pLODData->ParseLOD_VTX(numModels, numMeshes);

		uint32_t currentMeshIndex = 0u;

		// parse models via bodyparts
		for (int bodypartIdx = 0; bodypartIdx < numBodyparts; bodypartIdx++)
		{
			const mstudiobodyparts_t* const pBodypart = pBodyparts + bodypartIdx; // [rika]: messy but do what you got to
			const OptimizedModel::BodyPartHeader_t* const pVertBodyPart = pVTX->pBodyPart(bodypartIdx);
			const ModelBodyPart_t& bodypart = bodyparts[bodypartIdx];

			for (int modelIdx = 0; modelIdx < pBodypart->nummodels; modelIdx++)
			{
				const mstudiomodel_t* const pStudioModel = pBodypart->pModel<mstudiomodel_t>(modelIdx);
				const OptimizedModel::ModelHeader_t* const pVertModel = pVertBodyPart->pModel(modelIdx);
				ModelModelData_t* const pModel = pLODData->models + (bodypart.modelIndex + modelIdx);

				const OptimizedModel::ModelLODHeader_t* const pVertLOD = pVertModel->pLOD(lodLevel);
				pLODData->switchPoint = pVertLOD->switchPoint;

				*pModel = ModelModelData_t(pLODData->meshes + currentMeshIndex, pStudioModel->nummeshes);
				pModel->GenerateName(pBodypart->pszName(), modelIdx, lodLevel);

				for (int meshIdx = 0; meshIdx < pStudioModel->nummeshes; ++meshIdx)
				{
					const mstudiomesh_t* const pStudioMesh = pStudioModel->pMesh(meshIdx);
					const OptimizedModel::MeshHeader_t* const pVertMesh = pVertLOD->pMesh(meshIdx);

					const int baseVertexOffset = (pStudioModel->vertexindex / sizeof(vvd::mstudiovertex_t)) + pStudioMesh->vertexoffset;
					const int studioVertCount = pStudioMesh->vertexloddata.numLODVertexes[lodLevel];

					if (pVertMesh->numStripGroups == 0)
					{
						pModel->meshCount--; // this mesh was skipped so remove it from our total
						continue;
					}

					// [rika]: grabs the vertex data
					vvd::mstudiovertex_t* rawVertices = reinterpret_cast<vvd::mstudiovertex_t*>(parseBuf->Buffer());
					Vector4D* rawTangents = reinterpret_cast<Vector4D*>(&rawVertices[studioVertCount]);
					Color32* rawColors = reinterpret_cast<Color32*>(&rawTangents[studioVertCount]);
					Vector2D* rawTexcoords = reinterpret_cast<Vector2D*>(&rawColors[studioVertCount]);

					pVVD->PerLODVertexBuffer(lodLevel, rawVertices, rawTangents, baseVertexOffset, baseVertexOffset + studioVertCount);

					if (pVVC)
					{
						pVVC->PerLODVertexBuffer(lodLevel, pVVD->numFixups, pVVD->GetFixupData(0), rawColors, rawTexcoords, baseVertexOffset, baseVertexOffset + studioVertCount);
					}

					ModelMeshData_t* const pMeshData = pLODData->meshes + currentMeshIndex;
					pMeshData->ParseMesh_VTX(pVTX, bodypartIdx, flags);

					// parsing more than one is unfun and not a single model from respawn has two
					int weightIdx = 0;
					assertm(pVertMesh->numStripGroups == 1, "model had more than one strip group");
					for (int stripGrpIdx = 0; stripGrpIdx < 1; stripGrpIdx++)
					{
						OptimizedModel::StripGroupHeader_t* pStripGrp = pVertMesh->pStripGroup(stripGrpIdx);
						const bool isHwSkinned = pStripGrp->IsHWSkinned();

						pMeshData->vertCount += pStripGrp->numVerts;
						pLODData->vertexCount += pStripGrp->numVerts;

						pMeshData->indexCount += pStripGrp->numIndices;
						pLODData->indexCount += pStripGrp->numIndices;

						assertm(s_MaxStudioTriIndices >= pMeshData->indexCount, "too many triangles");

						for (int stripIdx = 0; stripIdx < pStripGrp->numStrips; stripIdx++)
						{
							OptimizedModel::StripHeader_t* pStrip = pStripGrp->pStrip(stripIdx);
							const OptimizedModel::BoneStateChangeHeader_t* const pBoneStates = pStrip->pBoneStateChange(0);

							for (int vertIdx = 0; vertIdx < pStrip->numVerts; vertIdx++)
							{
								OptimizedModel::Vertex_t* pVert = pStripGrp->pVertex(pStrip->vertOffset + vertIdx);

								Vector2D* const texcoords = pMeshData->texcoordCount > 1 ? &parseTexcoords[(pStrip->vertOffset + vertIdx) * (pMeshData->texcoordCount - 1)] : nullptr;

								// [rika]: basically check if it's r5 using this instance
								if constexpr (sizeof(mstudiomodel_t) == sizeof(r5::mstudiomodel_v8_t))
								{
									Vertex_t::ParseVertexFromVTX(&parseVertices[pStrip->vertOffset + vertIdx], &parseWeights[weightIdx], texcoords, pMeshData, pVert, rawVertices, rawTangents, rawColors, rawTexcoords, pVVW, weightIdx);
								}
								else
								{
									Vertex_t::ParseVertexFromVTX(&parseVertices[pStrip->vertOffset + vertIdx], &parseWeights[weightIdx], texcoords, pMeshData, pVert, rawVertices, rawTangents, rawColors, rawTexcoords, weightIdx, isHwSkinned, pBoneStates);
								}

							}

							memcpy(&parseIndices[pStrip->indexOffset], pStripGrp->pIndex(pStrip->indexOffset), pStrip->numIndices * sizeof(uint16_t));
						}

					}
					pMeshData->weightsCount = weightIdx;

					pMeshData->ParseMaterial(this, pStudioMesh->material);

					pModel->vertCount += pMeshData->vertCount;

					ParseMeshData_VTX(pMeshData, parseVertices, parseTexcoords, parseIndices, parseWeights, meshBuffer->Buffer());

					// for export
					pLODData->weightsPerVert = pMeshData->weightsPerVert > pLODData->weightsPerVert ? pMeshData->weightsPerVert : pLODData->weightsPerVert;
					pLODData->texcoordsPerVert = pMeshData->texcoordCount > pLODData->texcoordsPerVert ? pMeshData->texcoordCount : pLODData->texcoordsPerVert;

					currentMeshIndex++;
				}

			}
		}
	}

	g_BufferManager.RelieveBuffer(meshBuffer);
	g_BufferManager.RelieveBuffer(parseBuf);
}

void CreateBuffersForModelDrawData(ModelParsedData_t* const parsedData, CDXDrawData* const drawData, const uint64_t lod);


// 
// COMPDATA
//
class CMeshData
{
public:
	CMeshData() : indiceOffset(0ll), vertexOffset(0ll), weightOffset(0ll), texcoordOffset(0ll), size(0ll), writer(nullptr) {};
	~CMeshData() {};

	inline void InitWriter() { writer = (char*)this + IALIGN16(sizeof(CMeshData)); memset(&indiceOffset, 0, sizeof(CMeshData) - sizeof(writer)); };
	inline void DestroyWriter() { size = writer - reinterpret_cast<char*>(this); writer = nullptr; };

	void AddIndices(const uint16_t* const indices, const size_t indiceCount);
	void AddVertices(const Vertex_t* const vertices, const size_t vertexCount);
	void AddWeights(const VertexWeight_t* const weights, const size_t weightCount);
	void AddTexcoords(const Vector2D* const texcoords, const size_t texcoordCount);

	inline uint16_t* const GetIndices() const { return indiceOffset > 0 ? reinterpret_cast<uint16_t*>((char*)this + indiceOffset) : nullptr; };
	inline Vertex_t* const GetVertices() const { return vertexOffset > 0 ? reinterpret_cast<Vertex_t*>((char*)this + vertexOffset) : nullptr; };
	inline VertexWeight_t* const GetWeights() const { return weightOffset > 0 ? reinterpret_cast<VertexWeight_t*>((char*)this + weightOffset) : nullptr; };
	inline Vector2D* const GetTexcoords() const { return texcoordOffset > 0 ? reinterpret_cast<Vector2D*>((char*)this + texcoordOffset) : nullptr; };

	inline int64_t GetWeightCount() const { return weightCount; };
	inline const int64_t GetSize() const { return size; };

private:
	int64_t indiceOffset;
	int64_t vertexOffset;
	int64_t weightOffset;
	int64_t weightCount;
	int64_t texcoordOffset;

	int64_t size; // size of this data

	char* writer; // for writing only
};

// for parsing the animation data
class CAnimDataBone
{
public:
	CAnimDataBone(const size_t frameCount) : flags(0)
	{
		positions.resize(frameCount);
		rotations.resize(frameCount);
		scales.resize(frameCount);
	};

	inline void SetFlags(const uint8_t& flagsIn) { flags |= flagsIn; };
	inline void SetFrame(const int frameIdx, const Vector& pos, const Quaternion& quat, const Vector& scale)
	{
		positions.at(frameIdx) = pos;
		rotations.at(frameIdx) = quat;
		scales.at(frameIdx) = scale;
	}

	enum BoneFlags
	{
		ANIMDATA_POS = 0x1, // bone has pos values
		ANIMDATA_ROT = 0x2, // bone has rot values
		ANIMDATA_SCL = 0x4, // bone has scale values
		ANIMDATA_DATA = (ANIMDATA_POS | ANIMDATA_ROT | ANIMDATA_SCL), // bone has animation data
	};

	inline const uint8_t GetFlags() const { return flags; };
	inline const Vector* GetPosPtr() const { return positions.data(); };
	inline const Quaternion* GetRotPtr() const { return rotations.data(); };
	inline const Vector* GetSclPtr() const { return scales.data(); };

private:
	uint8_t flags;

	std::vector<Vector> positions;
	std::vector<Quaternion> rotations;
	std::vector<Vector> scales;
};

static const int s_AnimDataBoneSizeLUT[8] =
{
	0,
	sizeof(Vector),								// ANIMDATA_POS
	sizeof(Quaternion),							// ANIMDATA_ROT
	sizeof(Vector) + sizeof(Quaternion),		// ANIMDATA_POS | ANIMDATA_ROT
	sizeof(Vector),								// ANIMDATA_SCL
	sizeof(Vector) * 2,							// ANIMDATA_POS | ANIMDATA_SCL
	sizeof(Vector) + sizeof(Quaternion),		// ANIMDATA_ROT | ANIMDATA_SCL
	(sizeof(Vector) * 2) + sizeof(Quaternion),	// ANIMDATA_POS | ANIMDATA_ROT | ANIMDATA_SCL
};

class CAnimData
{
public:
	CAnimData(const int boneCount, const int frameCount) : numBones(boneCount), numFrames(frameCount), pBuffer(nullptr), pOffsets(nullptr), pFlags(nullptr) {};
	CAnimData(char* const buf);

	inline void ReserveVector() { bones.resize(numBones, numFrames); };
	CAnimDataBone& GetBone(const size_t idx) { return bones.at(idx); };

	// mem
	inline const uint8_t GetFlag(const size_t idx) const { return pFlags[idx]; };
	inline const uint8_t GetFlag(const int idx) const { return pFlags[idx]; };
	inline const char* const GetData(const size_t idx) const { return GetFlag(idx) & CAnimDataBone::ANIMDATA_DATA ? reinterpret_cast<const char* const>(pBuffer + pOffsets[idx]) : nullptr; }

	const Vector* const GetBonePosForFrame(const int bone, const int frame) const;
	const Quaternion* const GetBoneQuatForFrame(const int bone, const int frame) const;
	const Vector* const GetBoneScaleForFrame(const int bone, const int frame) const;

	// returns allocated buffer
	const size_t ToMemory(char* const buf);

private:
	int numBones;
	int numFrames;

	bool memory; // memory format

	// mem
	char* const pBuffer;
	const size_t* pOffsets;
	const uint8_t* pFlags;

	std::vector<CAnimDataBone> bones;
};

//
// EXPORT FORMATS
//
enum eModelExportSetting : int
{
	MODEL_CAST,
	MODEL_RMAX,
	MODEL_SMD,
	MODEL_FMT_3D_COUNT,

	MODEL_RMDL = MODEL_FMT_3D_COUNT,

	// rmdl only for now, but can support sourcemodelasset in the future
	MODEL_STL_VALVE_PHYSICS,
	MODEL_STL_RESPAWN_PHYSICS,
	
	MODEL_HITBOXES,

	MODEL_FMT_COUNT,
};

static const char* s_ModelExportSettingNames[] =
{
	"CAST",
	"RMAX",
	"SMD",

	"RMDL",

	"STL (Valve Physics)", 
	"STL (Respawn Physics)",
	"OBJ (Hitboxes only)"
};

static const char* s_ModelExportExtensions[] =
{
	".cast",
	".rmax",
	".smd",

	".rmdl",
};

enum eAnimRigExportSetting : int
{
	ANIMRIG_CAST,
	ANIMRIG_RMAX,
	ANIMRIG_SMD,

	ANIMRIG_RRIG,

	ANIMRIG_FMT_COUNT,
};

static const char* s_AnimRigExportSettingNames[] =
{
	"CAST",
	"RMAX",
	"SMD",

	"RRIG",
};

enum eAnimSeqExportSetting : int
{
	ANIMSEQ_CAST,
	ANIMSEQ_RMAX,
	ANIMSEQ_SMD,

	ANIMSEQ_RSEQ,

	ANIMSEQ_FMT_COUNT,
};

static const char* s_AnimSeqExportSettingNames[] =
{
	"CAST",
	"RMAX",
	"SMD",

	"RSEQ",
};

bool ExportModelMeshes(const ModelParsedData_t* const parsedData, std::filesystem::path& exportPath, const int setting, const int version);
bool ExportModelQC(const ModelParsedData_t* const parsedData, std::filesystem::path& exportPath, const int setting, const int version);

bool ExportSeqDesc(const int setting, const ModelSeq_t* const seqdesc, std::filesystem::path& exportPath, const char* const skelName, const ModelParsedData_t* const rig, const uint64_t guid);
bool ExportSeqQC(const ModelParsedData_t* const parsedData, const ModelSeq_t* const sequence, std::filesystem::path& exportPath, const int setting, const int version);

void UpdateModelBoneMatrix(CDXDrawData* const drawData);
void InitModelBoneMatrix(CDXDrawData* const drawData, const ModelParsedData_t* const parsedData);

enum class PreviewSeqType_e : uint8_t
{
	SEQ_LOCAL, // stored directly inside the rmdl
	SEQ_ASEQ,  // rmdl -> aseq
	SEQ_ARIG,   // rmdl -> arig -> aseq
};

struct SeqPreviewEntry_t
{
	std::string name;
	uint64_t guid; // local seqs do not have a guid so this is zero

	const ModelSeq_t* seqdesc;
	const ModelParsedData_t* srcBones; // the skeleton that belongs to this sequence's parent (i.e., model or rig)

	PreviewSeqType_e type;

	bool parsed; // if this is from an external sequence then this is true if the asset was loaded (i.e., not in a diff pak)
				 // local sequences are always considered parsed
};

struct AnimState_t
{
	int selectedSeqIndex; // seqdesc
	int selectedAnimIndex; // animdesc

	int activeSeqIdx;
	int activeAnimIdx;

	float frame;

	std::unique_ptr<char[]> dcmpNoodle; // noodle decomp
	std::vector<int> boneRemap; // model bone index -> index into the seq's source skeleton (and its anim data), -1 if the bone isn't in it

	bool playing;
	bool looping;

	void TogglePlay() { playing = !playing; };
	void Play() { playing = true; };
	void Stop() { playing = false; frame = 0.f; };
	void Restart() { Play();  frame = 0.f; };
};

struct ModelPreviewInfo_t
{
	ModelPreviewInfo_t() : lastSelectedBodypartIndex(0u), selectedBodypartIndex(0u), lastSelectedSkinIndex(0u), selectedSkinIndex(0u), selectedLODLevel(0u), minLODIndex(0u), maxLODIndex(0u)
	{

	}

	std::vector<size_t> bodygroupModelSelected;
	size_t lastSelectedBodypartIndex = 0;
	size_t selectedBodypartIndex = 0;

	size_t lastSelectedSkinIndex = 0;
	size_t selectedSkinIndex = 0;

	// for controlling and handling lod changes
	uint8_t selectedLODLevel = 0u;
	uint8_t minLODIndex = 0u;
	uint8_t maxLODIndex = 0u;

	std::vector<SeqPreviewEntry_t> sequences;
	AnimState_t animState;
};

void* PreviewParsedData(ModelPreviewInfo_t* const info, ModelParsedData_t* const parsedData, const char* const assetName, const uint64_t assetGUID, const bool firstFrameForAsset);
void PreviewSeqDesc(const ModelSeq_t* const seqdesc);

// returns true if the user requested a refresh of the sequence list
bool Preview_SequencesSection(ModelPreviewInfo_t* const info, const ModelParsedData_t* const parsedData, CDXDrawData* const drawData);