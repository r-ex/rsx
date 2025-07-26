#pragma once
#include <game/rtech/utils/studio/optimize.h>
#include <game/rtech/utils/studio/phyfile.h>

#define FIX_OFFSET(offset) static_cast<int>(static_cast<int>(offset & 0xFFFE) << (4 * (offset & 1))) // C6297 please fix!!
#define MAX_NUM_LODS 8 // consistent across games

// Name of the default skin (idx 0) for studio models. This is not stored in the MDL file.
constexpr const char* STUDIO_DEFAULT_SKIN_NAME = "default";
// Fallback name of any skin that has a zero-length string stored in place of an actual name in the MDL file.
constexpr const char* STUDIO_NULL_SKIN_NAME = "null";

static constexpr int s_MaxStudioVerts		= 65536;
static constexpr int s_MaxStudioTriangles	= 131072;
#define MAXSTUDIOSRCVERTS		(8*65536)

#define MODEL_FILE_ID				MAKEFOURCC('I', 'D', 'S', 'T') // little-endian "IDST"
#define MODEL_ANIM_FILE_ID			MAKEFOURCC('I', 'D', 'A', 'G') // little-endian "IDAG"

#define UNPACKWEIGHT(w) static_cast<float>(w / 32767.f) // weights in vvw and vg are packed into a signed 16 bit value that uses the entire number range

//===================
// STUDIO VERTEX DATA
//===================

#define MODEL_VERTEX_FILE_ID			MAKEFOURCC('I', 'D', 'S', 'V') // little-endian "IDSV"
#define MODEL_VERTEX_FILE_VERSION		4
#define MODEL_VERTEX_COLOR_FILE_ID		MAKEFOURCC('I', 'D', 'C', 'V') // little-endian "IDCV"
#define MODEL_VERTEX_COLOR_FILE_VERSION	1

namespace vvd
{
	struct mstudioweightextra_t
	{
		short weight[3]; // value divided by 32767.0

		short pad; // likely alignment

		int extraweightindex; // base index for vvw, add (weightIdx - 3)

		inline float Weight(const int i) const { return static_cast<float>(weight[i] / 32767.f); }
	};

	struct mstudioboneweight_t
	{
		union
		{
			float	weight[MAX_NUM_BONES_PER_VERT];
			mstudioweightextra_t weightextra; // only in apex (v54)
		};

		unsigned char bone[MAX_NUM_BONES_PER_VERT]; // set to unsigned so we can read it
		char	numbones;
	};

	struct mstudiovertex_t
	{
		mstudioboneweight_t	m_BoneWeights;
		Vector m_vecPosition;
		Vector m_vecNormal;
		Vector2D m_vecTexCoord;
	};

	struct vertexFileFixup_t
	{
		int		lod;				// used to skip culled root lod
		int		sourceVertexID;		// absolute index from start of vertex/tangent blocks
		int		numVertexes;
	};

	struct vertexFileHeader_t
	{
		int id; // MODEL_VERTEX_FILE_ID
		int version; // MODEL_VERTEX_FILE_VERSION
		int checksum; // same as studiohdr_t, ensures sync

		int numLODs; // num of valid lods
		int numLODVertexes[MAX_NUM_LODS]; // num verts for desired root lod

		int numFixups; // num of vertexFileFixup_t

		int fixupTableStart; // offset from base to fixup table
		int vertexDataStart; // offset from base to vertex block
		int tangentDataStart; // offset from base to tangent block

		const vertexFileFixup_t* const GetFixupData(int i) const
		{
			return reinterpret_cast<vertexFileFixup_t*>((char*)this + fixupTableStart) + i;
		}

		const mstudiovertex_t* const GetVertexData(int i) const
		{
			return reinterpret_cast<mstudiovertex_t*>((char*)this + vertexDataStart) + i;
		}

		const Vector4D* const GetTangentData(int i) const
		{
			return reinterpret_cast<Vector4D*>((char*)this + tangentDataStart) + i;
		}

		//vertexFileFixup_t* pFixup() const { return reinterpret_cast<vertexFileFixup_t*>((char*)this + fixupTableStart); }

		int PerLODVertexCount(int lod) const
		{
			int vertexCount = 0;

			if (!numFixups)
				return numLODVertexes[lod];

			for (int i = 0; i < numFixups; i++)
			{
				const vertexFileFixup_t* const pFixup = GetFixupData(i);

				if (pFixup->lod < lod)
					continue;

				vertexCount += pFixup->numVertexes;
			}

			return vertexCount;
		}

		// max / min vars for limiting what verts we want to load (tldr limit to a specific model)
		void PerLODVertexBuffer(int lod, mstudiovertex_t* verts, Vector4D* tangs, int minVert = 0, int maxVert = MAXSTUDIOSRCVERTS) const
		{
			maxVert = maxVert > numLODVertexes[0] ? numLODVertexes[0] : maxVert; // prevent overflow, cap to vertice count in this vvd.
			minVert = minVert < 0 ? 0 : minVert; // prevent underflow

			if (!numFixups)
			{
				const size_t vertexCount = static_cast<size_t>(maxVert) - static_cast<size_t>(minVert);

				std::memcpy(verts, GetVertexData(minVert), vertexCount * sizeof(mstudiovertex_t));
				std::memcpy(tangs, GetTangentData(minVert), vertexCount * sizeof(Vector4D));

				return;
			}

			// we use this instead of sourceVertexID as it is unreliable in normal source models, this entire system is a hack and it's not how you're actually suppose to make vertex buffers (supposed to do per lod not per model lod)
			// in reSource all fixups have their vertex data sequentially, for example: fixup 2's vertices are after fixup 1's, and fixup 3's is after fixup 2's, this means all sourceVertexIDs would be in order.
			// in normal source fixup vertices seem to be sorted by lod, so: fixup 4's (lod 1) vertices are after fixup 1's (lod 1), and fixup 5's (lod 2) vertices are after fixup 2's (lod 2).
			// [rika]: if vertices seem mixed between meshes, look here.
			int curVertexIndex = 0; // I don't like this

			for (int i = 0; i < numFixups; i++)
			{
				const vertexFileFixup_t* const fixup = GetFixupData(i);

				if (fixup->lod < lod || curVertexIndex < minVert)
				{
					curVertexIndex += fixup->numVertexes;
					continue;
				}

				if (curVertexIndex >= maxVert)
					break;

				curVertexIndex += fixup->numVertexes;

				std::memcpy(verts, GetVertexData(fixup->sourceVertexID), fixup->numVertexes * sizeof(mstudiovertex_t));
				std::memcpy(tangs, GetTangentData(fixup->sourceVertexID), fixup->numVertexes * sizeof(Vector4D));

				verts += fixup->numVertexes;
				tangs += fixup->numVertexes;
			}

		}
	};
}

namespace vvc
{
	struct vertexColorFileHeader_t
	{
		int id; // IDCV
		int version; // 1
		int checksum; // same as studiohdr_t, ensures sync

		int numLODs; // num of valid lods
		int numLODVertexes[MAX_NUM_LODS]; // num verts for desired root lod

		int colorDataStart;
		int uv2DataStart;

		const Color32* const GetColorData(int i) const
		{
			return reinterpret_cast<Color32*>((char*)this + colorDataStart) + i;
		}

		const Vector2D* const GetUVData(int i) const
		{
			return reinterpret_cast<Vector2D*>((char*)this + uv2DataStart) + i;
		}

		void PerLODVertexBuffer(int lod, const int numFixups, const vvd::vertexFileFixup_t* const pFixup, Color32* colors, Vector2D* uv2s, int minVert = 0, int maxVert = MAXSTUDIOSRCVERTS) const
		{
			maxVert = maxVert > numLODVertexes[0] ? numLODVertexes[0] : maxVert; // prevent overflow, cap to vertice count in this vvd.
			minVert = minVert < 0 ? 0 : minVert; // prevent underflow

			if (!numFixups)
			{
				const size_t vertexCount = static_cast<size_t>(maxVert) - static_cast<size_t>(minVert);

				std::memcpy(colors, GetColorData(minVert), vertexCount * sizeof(Color32));
				std::memcpy(uv2s, GetUVData(minVert), vertexCount * sizeof(Vector2D));

				return;
			}

			int curVertexIndex = 0; // I don't like this

			for (int i = 0; i < numFixups; i++)
			{
				const vvd::vertexFileFixup_t* const fixup = &pFixup[i];

				if (fixup->lod < lod || curVertexIndex < minVert)
				{
					curVertexIndex += fixup->numVertexes;
					continue;
				}

				if (curVertexIndex >= maxVert)
					break;

				curVertexIndex += fixup->numVertexes;

				std::memcpy(colors, GetColorData(fixup->sourceVertexID), fixup->numVertexes * sizeof(Color32));
				std::memcpy(uv2s, GetUVData(fixup->sourceVertexID), fixup->numVertexes * sizeof(Vector2D));

				colors += fixup->numVertexes;
				uv2s += fixup->numVertexes;
			}

		}
	};
}

namespace vvw
{
	struct mstudioboneweightextra_t
	{
		short	weight; // weight = this / 32767.0
		short   bone;

		inline float Weight() const { return UNPACKWEIGHT(weight); }
	};

	struct vertexBoneWeightsExtraFileHeader_t
	{
		int checksum; // same as studiohdr_t, ensures sync
		int version; // should be 1

		int numLODVertexes[MAX_NUM_LODS]; // maybe this but the others don't get filled?

		int weightDataStart; // index into mstudioboneweightextra_t array

		const mstudioboneweightextra_t* const GetWeightData(int i) const
		{
			return reinterpret_cast<mstudioboneweightextra_t*>((char*)this + weightDataStart) + i;
		}
	};
}

namespace vg
{
	// mesh flags, these are the same across all versions
	// mimics shader input flags
	#define VERT_POSITION_UNPACKED		0x1		// size of 12
	#define VERT_POSITION_PACKED		0x2		// size of 8
	#define VERT_POSITION				0x3		// starting with rmdl 13 position is treated more like an enum, thankfully reverse compatible

	#define VERT_COLOR					0x10	// size of 4

	#define VERT_UNK					0x40	// size of 8??? flag gets ignored, is this flag for having mesh data? color2?

	#define VERT_NORMAL_UNPACKED		0x100	// size of 12
	#define VERT_NORMAL_PACKED			0x200	// size of 4
	#define VERT_TANGENT_FLOAT3			0x400	// size of 12
	#define VERT_TANGENT_FLOAT4			0x800	// size of 16, size is ignored for vertex size calculation

	#define VERT_BLENDINDICES			0x1000	// size of 4, bone indices and count
	#define VERT_BLENDWEIGHTS_UNPACKED	0x2000	// size of 8, two floats, presumably weights
	#define VERT_BLENDWEIGHTS_PACKED	0x4000	// size of 4, two uint16_ts packed with a weight

	// thanks rexx
	#define VERT_TEXCOORD_BITS			4 // nibble
	#define VERT_TEXCOORD_MASK			((1 << VERT_TEXCOORD_BITS) - 1)
	#define VERT_TEXCOORD_SHIFT(n)		(24 + (n * VERT_TEXCOORD_BITS))
	#define VERT_TEXCOORDn(n)			(static_cast<uint64_t>(VERT_TEXCOORD_MASK) << VERT_TEXCOORD_SHIFT(n))
	#define VERT_TEXCOORDn_FMT(n, fmt)	(static_cast<uint64_t>(fmt) << VERT_TEXCOORD_SHIFT(n))

	// size depends on format, probably can only be certain formats (limited by size lookup micro lut)
	#define VERT_TEXCOORD0				VERT_TEXCOORDn(0) // commonly used as the first uv 
	#define VERT_TEXCOORD2				VERT_TEXCOORDn(2) // commonly used as the second uv
	#define VERT_TEXCOORD3				VERT_TEXCOORDn(3) // used in 'wind'/folliage models, the vertex shader uses the first value as a non float field
	
	// for vtx based models
	#define VERT_LEGACY (VERT_POSITION_UNPACKED | VERT_NORMAL_UNPACKED | VERT_TANGENT_FLOAT4 | VERT_BLENDINDICES | VERT_BLENDWEIGHTS_UNPACKED | VERT_TEXCOORDn_FMT(0, 0x2))

	enum eVertPositionType
	{
		VG_POS_NONE,
		VG_POS_UNPACKED,	// Vector
		VG_POS_PACKED64,	// Vector64
		VG_POS_PACKED48,	// UNKNOWN
	};

	const int64_t VertexSizeFromFlags_V9(const uint64_t flags);
	const int64_t VertexSizeFromFlags_V13(const uint64_t flags);

	struct BlendWeightsPacked_s
	{
		uint16_t weight[2];	// packed weight with a max value of 32767, divide value by 32767 to get weight. weights will always correspond to first and second bone
		// if the mesh has extra bone weights the second value will be used as an index into the array of extra bone weights, max value of 65535.
		inline float Weight(const int i) const { return UNPACKWEIGHT(weight[i]); }
		inline const uint16_t Index() const { return weight[1]; }
	};

	struct BlendWeightIndices_s
	{
		uint8_t bone[3];	// when the model doesn't have extra bone weights all three are used for bone indices, otherwise in order they will be used for: first bone, last bone (assumes vvd->vg), unused.
		uint8_t boneCount;	// number of bones this vertex is weighted to excluding the base weight (value of 0 if only one weight, max of 15 with 16 weights)
	};

	struct Vertex_t
	{
		Vector positionsUnpacked;
		Vector64 positionPacked;
		BlendWeightsPacked_s blendWeightsPacked;
		BlendWeightIndices_s blendIndices;
		uint32_t normal;
		Color32 color;
		Vector2D texCoords0;
		Vector2D texCoords2;
	};

	// added in rev 3
	// used for making shapes to blend between, good for stuff that can't be animated with bones (ferrofluid, static props, etc)
	// FROM: uberVcolTnBnInterpm0InterpInst_rgbs_vs (0x4D2DB5174F10FEFA)
	/*
	struct BlendShapeVertex_s
	{
		uint2 position;                // Offset:    0
		float2 texCoords;              // Offset:    8
		float2 texCoords1;             // Offset:   16
		uint normal;                   // Offset:   24
		uint color;                    // Offset:   28
	};
	*/
	struct BlendShapeVertex_s
	{
		Vector64 position;		// position changes
		Vector2D texCoords;
		Vector2D texCoords1;	// null if not used
		uint32_t normal;		// same normal as base, tangent is different, related to position obviously
		Color32 color;
	};

	// seasons 0-6
	// rmdl versions 9-12
	namespace rev1
	{
		struct MeshHeader_t;
		struct ModelLODHeader_t;
		struct VertexGroupHeader_t
		{
			int id;			// 0x47567430	'0tVG'
			int version;	// 1
			int unk;		// Usually 0, checksum? header index? data offset?
			int dataSize;	// Total size of data + header in starpak

			// unsigned char
			int64_t boneStateChangeOffset; // offset to bone remap buffer
			int64_t boneStateChangeCount;  // number of "bone remaps" (size: 1)
			inline const uint8_t* const pBoneMap() const { return reinterpret_cast<uint8_t*>((char*)this + boneStateChangeOffset); };

			// MeshHeader_t
			int64_t meshOffset;   // offset to mesh buffer
			int64_t meshCount;    // number of meshes (size: 0x48)
			inline const MeshHeader_t* const pMesh(const int64_t i) const;

			// unsigned short
			int64_t indexOffset;     // offset to index buffer
			int64_t indexCount;      // number of indices (size: 2 (uint16_t))
			inline const uint16_t* const pIndices() const { return indexCount > 0 ? reinterpret_cast<uint16_t*>((char*)this + indexOffset) : nullptr; };

			// uses dynamic sized struct, similar to RLE animations
			int64_t vertOffset;    // offset to vertex buffer
			int64_t vertCount;     // number of chars in vertex buffer
			inline const char* const pVertices() const { return vertCount > 0 ? reinterpret_cast<char*>((char*)this + vertOffset) : nullptr; };

			// vvw::mstudioboneweightextra_t
			int64_t extraBoneWeightOffset;   // offset to extended weights buffer
			int64_t extraBoneWeightSize;    // number of chars in extended weights buffer
			inline const vvw::mstudioboneweightextra_t* const pBoneWeights() const { return extraBoneWeightSize > 0 ? reinterpret_cast<vvw::mstudioboneweightextra_t*>((char*)this + extraBoneWeightOffset) : nullptr; };

			// there is one for every LOD mesh
			// i.e, unknownCount == lod.meshCount for all LODs
			int64_t unknownOffset;   // offset to buffer
			int64_t unknownCount;    // count (size: 0x30)

			// ModelLODHeader_t
			int64_t lodOffset;       // offset to LOD buffer
			int64_t lodCount;        // number of LODs (size: 0x8)
			inline const ModelLODHeader_t* const pLOD(const int64_t i) const;

			// vvd::mstudioboneweight_t
			int64_t legacyWeightOffset;	// seems to be an offset into the "external weights" buffer for this mesh
			int64_t legacyWeightCount;   // seems to be the number of "external weights" that this mesh uses

			// vtx::StripHeader_t
			int64_t stripOffset;    // offset to strips buffer
			int64_t stripCount;     // number of strips (size: 0x23)

			int64_t unused[8];
		};

		struct MeshHeader_t
		{
			int64_t flags;	// mesh flags

			// uses dynamic sized struct, similar to RLE animations
			uint32_t vertOffset;	// start offset for this mesh's vertices
			uint32_t vertCacheSize;	// size of the vertex structure
			uint32_t vertCount;		// number of vertices
			inline const char* const pVertices(const VertexGroupHeader_t* const hdr) const { return vertCount > 0 ? (hdr->pVertices() + vertOffset) : nullptr; };

			int unk_14;

			// vvw::mstudioboneweightextra_t
			int extraBoneWeightOffset;	// start offset for this mesh's extra bone weights
			int extraBoneWeightSize;	// size or count of extra bone weights
			inline const vvw::mstudioboneweightextra_t* const pBoneWeight(const VertexGroupHeader_t* const hdr) const { return extraBoneWeightSize > 0 ? hdr->pBoneWeights() + (extraBoneWeightOffset / sizeof(vvw::mstudioboneweightextra_t)) : nullptr; };

			// unsigned short
			int indexOffset;		// index into indices
			int indexCount;			// number of indices
			inline const uint16_t* const pIndices(const VertexGroupHeader_t* const hdr) const { return indexCount > 0 ? (hdr->pIndices() + indexOffset) : nullptr; };

			// vvd::mstudioboneweight_t
			int legacyWeightOffset;	// index into legacy weights (vvd)
			int legacyWeightCount;	// number of legacy weights

			// vtx::StripHeader_t
			int stripOffset;        // index into strips
			int stripCount;			// number of strips

			// might be stuff like topologies and or bonestates, definitely unused
			int unk_38[4];

			/*int numBoneStateChanges;
			int boneStateChangeOffset;

			// MDL Version 49 and up only
			int numTopologyIndices;
			int topologyOffset;*/
		};

		struct ModelLODHeader_t
		{
			uint16_t meshIndex;
			uint16_t meshCount;
			inline const MeshHeader_t* const pMesh(const VertexGroupHeader_t* const hdr, const int64_t i) const { return meshCount > 0 ? hdr->pMesh(meshIndex + i) : nullptr; };

			float switchPoint;
		};

		struct UnkVgData_t
		{
			int64_t unk;
			float unk1;

			char data[0x24];
		};

		inline const MeshHeader_t* const VertexGroupHeader_t::pMesh(const int64_t i) const { return meshCount > 0 ? (reinterpret_cast<MeshHeader_t*>((char*)this + meshOffset) + i) : nullptr; };
		inline const ModelLODHeader_t* const VertexGroupHeader_t::pLOD(const int64_t i) const { return lodCount > 0 ? (reinterpret_cast<ModelLODHeader_t*>((char*)this + lodOffset) + i) : nullptr; };
	}

	// seasons 7-13
	// rmdl versions 12.1-13
	// starts of the trend of offsets starting from their variable
	namespace rev2
	{
		struct MeshHeader_t
		{
			int64_t flags;

			int vertCacheSize;		        // number of bytes used from the vertex buffer
			int vertCount;			        // number of vertices

			int64_t indexOffset;			// start offset for this mesh's "indices"
			int64_t indexCount : 56;        // number of indices
			int64_t indexType : 8;			// tri quad? StripHeaderFlags_t. added in rmdl version 12.2, 12.1 does not have this but making a new struct would be silly
			inline const uint16_t* const pIndices() const { return indexCount > 0 ? reinterpret_cast<uint16_t*>((char*)this + offsetof(MeshHeader_t, indexOffset) + indexOffset) : nullptr; }

			int64_t vertOffset;             // start offset for this mesh's vertices
			int64_t vertBufferSize;         // TOTAL size of vertex buffer
			inline char* const pVertices() const { return vertBufferSize > 0 ? reinterpret_cast<char*>((char*)this + offsetof(MeshHeader_t, vertOffset) + vertOffset) : nullptr; }

			int64_t extraBoneWeightOffset;	// start offset for this mesh's extra bone weights
			int64_t extraBoneWeightSize;    // size or count of extra bone weights
			inline const vvw::mstudioboneweightextra_t* const pBoneWeights() const { return extraBoneWeightSize > 0 ? reinterpret_cast<vvw::mstudioboneweightextra_t*>((char*)this + offsetof(MeshHeader_t, extraBoneWeightOffset) + extraBoneWeightOffset) : nullptr; }

			int64_t legacyWeightOffset;	    // seems to be an offset into the "external weights" buffer for this mesh
			int64_t legacyWeightCount;       // seems to be the number of "external weights" that this mesh uses

			int64_t stripOffset;            // Index into the strips structs
			int64_t stripCount;
		};

		struct ModelLODHeader_t
		{
			int dataOffset; // stolen from rmdl
			int dataSize;

			// this is like the section in rmdl, but backwards for some reason
			uint8_t meshCount;
			uint8_t meshIndex;	// for lod, probably 0 in most cases
			uint8_t lodLevel;		// lod level that this header represents
			uint8_t groupIndex;	// vertex group index

			float switchPoint;

			int64_t meshOffset;
			inline const MeshHeader_t* const pMesh(const int meshIdx) const { return meshCount > 0 ? reinterpret_cast<MeshHeader_t*>((char*)this + offsetof(ModelLODHeader_t, meshOffset) + meshOffset) + meshIdx : nullptr; }
		};

		// potential for several per file
		struct VertexGroupHeader_t
		{
			int id;			// 0x47567430	'0tVG'
			int version;	// 0x1

			int lodIndex;	// 
			int lodCount;	// number of lods contained within this group
			int groupIndex;	// vertex group index
			int lodMap;		// lods in this group, each bit is a lod

			int64_t lodOffset;
			inline const ModelLODHeader_t* const pLod(const int lodIdx) const { return lodCount > 0 ? reinterpret_cast<ModelLODHeader_t*>((char*)this + offsetof(VertexGroupHeader_t, lodOffset) + lodOffset) + lodIdx : nullptr; }
		};
	}

	// seasons 13.1(?)-14
	// rmdl versions 14-15
	// only major struct change over rev2 is the addition of blend shapes
	namespace rev3
	{
		struct MeshHeader_t
		{
			uint64_t flags;

			uint32_t vertCacheSize;			// number of bytes used from the vertex buffer
			uint32_t vertCount;				// number of vertices

			int64_t indexOffset;			// start offset for this mesh's "indices"
			int64_t indexCount : 56;        // number of indices
			int64_t indexType : 8;			// tri quad? StripHeaderFlags_t
			inline const uint16_t* const pIndices() const { return indexCount > 0 ? reinterpret_cast<uint16_t*>((char*)this + offsetof(MeshHeader_t, indexOffset) + indexOffset) : nullptr; }

			int64_t vertOffset;             // start offset for this mesh's vertices
			int64_t vertBufferSize;         // TOTAL size of vertex buffer
			inline char* const pVertices() const { return vertBufferSize > 0 ? reinterpret_cast<char*>((char*)this + offsetof(MeshHeader_t, vertOffset) + vertOffset) : nullptr; }

			int64_t extraBoneWeightOffset;	// start offset for this mesh's extra bone weights
			int64_t extraBoneWeightSize;    // size or count of extra bone weights
			inline const vvw::mstudioboneweightextra_t* const pBoneWeights() const { return extraBoneWeightSize > 0 ? reinterpret_cast<vvw::mstudioboneweightextra_t*>((char*)this + offsetof(MeshHeader_t, extraBoneWeightOffset) + extraBoneWeightOffset) : nullptr; }

			// unused now
			int64_t deprecated_legacyWeightOffset;	// seems to be an offset into the "external weights" buffer for this mesh
			int64_t deprecated_legacyWeightCount;	// seems to be the number of "external weights" that this mesh uses

			int64_t deprecated_stripOffset;			// Index into the strips structs
			int64_t deprecated_stripCount;

			int64_t blendShapeVertOffset;
			int64_t blendShapeVertBufferSize;		// shapeCount = meshCountBlends in mstudiomodel_t
		};

		struct ModelLODHeader_t
		{
			int dataOffset;
			int dataSize;

			// this is like the section in rmdl, but backwards for some reason
			uint8_t meshCount;
			uint8_t meshIndex;	// for lod, probably 0 in most cases
			uint8_t lodLevel;	// lod level that this header represents
			uint8_t groupIndex;	// vertex group index

			float switchPoint;

			int64_t meshOffset;
			inline const MeshHeader_t* const pMesh(const int meshIdx) const { return meshCount > 0 ? reinterpret_cast<MeshHeader_t*>((char*)this + offsetof(ModelLODHeader_t, meshOffset) + meshOffset) + meshIdx : nullptr; }
		};

		struct VertexGroupHeader_t
		{
			int id;			// 0x47567430	'0tVG'
			int version;	// 0x1

			int lodIndex;
			int lodCount;	// number of lods contained within this group
			int groupIndex;	// vertex group ind
			int lodMap;		// lods in this group, each bit is a lod

			int64_t lodOffset;
			inline const ModelLODHeader_t* const pLod(const int lodIdx) const { return lodCount > 0 ? reinterpret_cast<ModelLODHeader_t*>((char*)this + offsetof(VertexGroupHeader_t, lodOffset) + lodOffset) + lodIdx : nullptr; }
		};
	}

	// seasons 15-current
	// rmdl versions 16
	namespace rev4
	{
		struct MeshHeader_t
		{
			uint64_t flags;

			uint32_t vertCount;					// number of vertices
			uint16_t vertCacheSize;				// number of bytes used from the vertex buffer
			uint16_t vertBoneCount;				// how many bones are used by vertices in this mesh, this will be from StripHeader_t

			uint32_t indexOffset;				// start offset for this mesh's "indices"
			uint32_t indexCount : 28;			// number of indices
			uint32_t indexType : 4;				// tri quad? StripHeaderFlags_t, might be important. indexGroups?
			inline const uint16_t* const pIndices() const { return indexCount > 0 ? reinterpret_cast<uint16_t*>((char*)this + offsetof(MeshHeader_t, indexOffset) + indexOffset) : nullptr; }

			uint32_t vertOffset;				// start offset for this mesh's vertices
			uint32_t vertBufferSize;			// TOTAL size of vertex buffer
			inline char* const pVertices() const { return vertBufferSize > 0 ? reinterpret_cast<char*>((char*)this + offsetof(MeshHeader_t, vertOffset) + vertOffset) : nullptr; }

			uint32_t extraBoneWeightOffset;		// start offset for this mesh's extra bone weights
			uint32_t extraBoneWeightSize;		// size or count of extra bone weights
			inline const vvw::mstudioboneweightextra_t* const pBoneWeights() const { return extraBoneWeightSize > 0 ? reinterpret_cast<vvw::mstudioboneweightextra_t*>((char*)this + offsetof(MeshHeader_t, extraBoneWeightOffset) + extraBoneWeightOffset) : nullptr; }

			uint32_t blendShapeVertOffset;
			uint32_t blendShapeVertBufferSize;	// shapeCount = meshCountBlends in mstudiomodel_t
		};

		struct ModelLODHeader_t
		{
			uint8_t meshCount;
			uint8_t meshIndex;	// for lod, probably 0 in most cases
			uint8_t lodLevel;	// lod level that this header represents
			uint8_t groupIndex;	// vertex group index

			uint32_t meshOffset;
			inline const MeshHeader_t* const pMesh(const uint8_t meshIdx) const { return meshCount > 0 ? reinterpret_cast<MeshHeader_t*>((char*)this + offsetof(ModelLODHeader_t, meshOffset) + meshOffset) + meshIdx : nullptr; }
		};

		struct VertexGroupHeader_t
		{
			uint8_t lodIndex;
			uint8_t lodCount;	// number of lods contained within this group
			uint8_t groupIndex;	// vertex group ind
			uint8_t lodMap;		// lods in this group, each bit is a lod

			uint32_t lodOffset;
			inline const ModelLODHeader_t* const pLod(const uint8_t lodIdx) const { return lodCount > 0 ? reinterpret_cast<ModelLODHeader_t*>((char*)this + offsetof(VertexGroupHeader_t, lodOffset) + lodOffset) + lodIdx : nullptr; }
		};
	}
}

//===================
// STUDIO MODEL DATA
//===================

struct mstudio_meshvertexloddata_t
{
	int modelvertexdataUnusedPad; // likely has none of the funny stuff because unused

	int numLODVertexes[MAX_NUM_LODS]; // depreciated starting with rmdl v14(?)
};

struct mstudiobodyparts_t
{
	int					sznameindex;
	inline char* const pszName() const { return ((char*)this + sznameindex); }

	int					nummodels;
	int					base;
	int					modelindex; // index into models array

	template<typename T>
	inline const T* pModel(int i) const
	{
		return reinterpret_cast<T*>((char*)this + modelindex) + i;
	};
};
static_assert(sizeof(mstudiobodyparts_t) == 0x10);

union mstudioanimvalue_t
{
	struct
	{
		uint8_t	valid;
		uint8_t	total;
	} num;
	short value;
};

struct mstudioanim_valueptr_t
{
	short offset[3];
	inline mstudioanimvalue_t* pAnimvalue(int i) const { return (offset[i] > 0) ? reinterpret_cast<mstudioanimvalue_t*>((char*)this + offset[i]) : nullptr; }
};
static_assert(sizeof(mstudioanim_valueptr_t) == 0x6);

enum eStudioAnimFlags
{
	ANIM_LOOPING		= 0x1,
	ANIM_DELTA			= 0x4,
	ANIM_ALLZEROS		= 0x20,		// this animation/sequence has no real animation data
	ANIM_VALID			= 0x20000,	// if not set this anim has no data
	ANIM_SCALE			= 0x20000,	// set in mstudioseqdesc_t if this animation uses scale data
	ANIM_FRAMEMOVEMENT	= 0x40000,
	ANIM_DATAPOINT		= 0x200000,	// uses 'datapoint' style compression instead of 'rle' style
};

// This is set any time the .qc files has $staticprop in it
// Means there's no bones and no transforms
#define STUDIOHDR_FLAGS_STATIC_PROP				0x10

// If this flag is present the model has vertex color, and by extension a VVC (IDVC) file.
#define STUDIOHDR_FLAGS_USES_VERTEX_COLOR	        0x1000000

// If this flag is present the model has a secondary UV layer.
#define STUDIOHDR_FLAGS_USES_UV2			        0x2000000

// small studio header for checking file id and version.
struct studiohdr_short_t
{
	int id;			// should be 'IDST' in all supported versions.
	int version;	// versions 52, and 53 are valid, version 54 should be exported via RPak.
	int checksum;	// this should be the same as other attached files, otherwise exporting should be aborted.

	inline const bool UseHdr2() const { return (version == 53 || version == 54) ? false : true; }

	const int length() const
	{
		const int index = UseHdr2() ? 19 : 20; // non hdr2 files have sznameindex after checksum, which puts this one int later

		return reinterpret_cast<const int* const>(this)[index];
	}

	const char* pszName() const;
};

// for non pak models with loose data (r1)
class StudioLooseData_t
{
public:
	StudioLooseData_t(const std::filesystem::path& path, const char* name, char* buffer); // DO NOT call this without managing the allocated buffers.
	StudioLooseData_t(char* file);

	enum LooseDataType : int8_t
	{
		SLD_VTX,
		SLD_VVD,
		SLD_VVC,
		SLD_VVW,

		SLD_COUNT
	};

	inline char* const VertBuf() const { return vertexDataBuffer; }
	inline const int VertOffset(const LooseDataType type) const { return vertexDataOffset[type]; }
	inline const int VertSize(const LooseDataType type) const { return vertexDataSize[type]; }

	inline char* const PhysBuf() const { return physicsDataBuffer; }
	inline const int PhysOffset() const { return physicsDataOffset; }
	inline const int PhysSize() const { return physicsDataSize; }

	inline const OptimizedModel::FileHeader_t*				const GetVTX() const { return vertexDataSize[SLD_VTX] ? reinterpret_cast<const OptimizedModel::FileHeader_t*			const>(vertexDataBuffer + vertexDataOffset[SLD_VTX]) : nullptr; }
	inline const vvd::vertexFileHeader_t*					const GetVVD() const { return vertexDataSize[SLD_VVD] ? reinterpret_cast<const vvd::vertexFileHeader_t*					const>(vertexDataBuffer + vertexDataOffset[SLD_VVD]) : nullptr; }
	inline const vvc::vertexColorFileHeader_t*				const GetVVC() const { return vertexDataSize[SLD_VVC] ? reinterpret_cast<const vvc::vertexColorFileHeader_t*			const>(vertexDataBuffer + vertexDataOffset[SLD_VVC]) : nullptr; }
	inline const vvw::vertexBoneWeightsExtraFileHeader_t*	const GetVVW() const { return vertexDataSize[SLD_VVW] ? reinterpret_cast<const vvw::vertexBoneWeightsExtraFileHeader_t*	const>(vertexDataBuffer + vertexDataOffset[SLD_VVW]) : nullptr; }

private:

	char* vertexDataBuffer;
	int vertexDataOffset[LooseDataType::SLD_COUNT];
	int vertexDataSize[LooseDataType::SLD_COUNT];

	char* physicsDataBuffer;
	int physicsDataOffset;
	int physicsDataSize;
};

static const char* s_StudioLooseDataExtensions[StudioLooseData_t::LooseDataType::SLD_COUNT] = {
	".dx11.vtx",
	".vvd",
	".vvc",
	".vvw",
};

// generic pre def
struct animmovement_t;
struct animdesc_t;
struct seqdesc_t;
struct studiohdr_generic_t;