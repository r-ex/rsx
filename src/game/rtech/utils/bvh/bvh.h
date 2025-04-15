#pragma once
#include <set>

struct Triangle
{
	Vector a;
	Vector b;
	Vector c;

	uint32_t flags;
};

struct Quad
{
	Vector a;
	Vector b;
	Vector c;
	Vector d;

	uint32_t flags;
};

struct PackedVector // TODO: move elsewhere
{
	uint16_t x, y, z;
};

// intermediate data for exporting a model of some bsp data
struct CollisionModel_t
{
	std::vector<Triangle> tris;
	std::vector<Quad> quads;

	bool exportSTL(const std::filesystem::path& out);
	bool exportOBJ(const std::filesystem::path& out);
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
	Vector origin;
	float decodeScale;
};

struct dbvhtri_t
{
	uint32_t vertAIndex : 11;
	uint32_t vertBIndex : 9;
	uint32_t vertCIndex : 9;
	uint32_t edgeMask : 3;
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

	uint32_t cmIndex : 8; // 8-bit index into 32-bit collision mask array
	uint32_t index0 : 24;

	uint32_t pad : 8;
	uint32_t index1 : 24;

	uint32_t child0Type : 4;
	uint32_t child1Type : 4;
	uint32_t index2 : 24;

	uint32_t child2Type : 4;
	uint32_t child3Type : 4;
	uint32_t index3 : 24;
};

struct BVHModel_t
{
	const dbvhnode_t* nodes;
	const Vector* verts;
	const PackedVector* packedVerts;
	const char* leafs;
	const uint32_t* masks;
	const Vector* origin;
	float scale;
	uint32_t maskFilter;
	bool filterExclusive;
	bool filterAND;
};

extern void Coll_HandleNodeChildType(CollisionModel_t& colModel, uint32_t contents, int parentNodeIndex, int nodeType, int index, const BVHModel_t* pModel);