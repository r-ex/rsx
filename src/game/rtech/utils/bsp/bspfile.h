#pragma once

#include "core/basetypes.h"
#include "branch/respawn/shared.h"
#include "branch/respawn/apex/lumps.h"

BEGIN_NAMESPACE(apex)

void DumpBSPFile(const fs::path& inputPath);

typedef respawn::BSPHeader_t BSPHeader_t;

using respawn::MAX_LUMPS;

// LUMP_MODELS (14 - 0x000e)
struct dmodel_t
{
	Vector3 mins;
	Vector3 maxs;
	int firstMesh;
	int meshCount;
	int bvhNodeIndex;
	int bvhLeafIndex;
	int vertexIndex;
	int vertexFlags;
	Vector3 origin;
	float decodeScale;
};

struct dbvhaxis_t
{
	short minChild0;
	short maxChild0;

	short minChild1;
	short maxChild1;

	short minChild2;
	short maxChild2;

	short minChild3;
	short maxChild3;
};

enum dbvhchildtype_e
{
	NODE,
	NO_CHILD,
	INVALID = 2,
	UNK_3,
	PRIM_TRI,         // triangle primitive using float3 verts
	PRIM_TRI_PACKED,  // triangle primitive using uint16 verts
	PRIM_QUAD,        // quad primitive using float3 verts
	PRIM_QUAD_PACKED, // quad primitive using uint16 verts
	UNK_8,
	STATIC_PROP,
};

struct dbvhleaf_poly_t
{
	uint32_t unk : 12;
	uint32_t polyCount : 4; // +1
	uint32_t baseVertexOffset : 16; // used when the leaf refers to verts beyond an index of 1024
};

// type 3 elements
struct dbvhsubleaf_t
{
	uint8_t cmIndex;
	uint8_t primitiveType;
	uint16_t length; // in dwords
};

// type 8 leaf
struct dbvhtype8_t
{
	uint8_t vertexCount;
	uint8_t faceCount;
	uint8_t triCount;
	uint8_t quadCount;
	Vector3 origin;
	float decodeScale;
};

struct dbvhtri_t
{
	uint32_t vertAIndex : 11;
	uint32_t vertBIndex : 9;
	uint32_t vertCIndex : 9;
	uint32_t edgeMask   : 3;
};


struct dbvhquad_t
{
	uint32_t vertAIndex : 10;
	uint32_t vertBIndex : 9;
	uint32_t vertCIndex : 9;
	uint32_t edgeMask : 4;
};

// LUMP_BVH_NODES (18 - 0x0012)
struct dbvhnode_t
{
	dbvhaxis_t x;
	dbvhaxis_t y;
	dbvhaxis_t z;

	uint32_t cmIndex    :  8; // 8-bit index into 32-bit collision mask array
	uint32_t index0     : 24;

	uint32_t pad        :  8;
	uint32_t index1     : 24;
	
	uint32_t child0Type :  4;
	uint32_t child1Type :  4;
	uint32_t index2     : 24;

	uint32_t child2Type :  4;
	uint32_t child3Type :  4;
	uint32_t index3     : 24;
};



END_NAMESPACE()