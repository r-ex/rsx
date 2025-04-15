#pragma once

#define IVPS_ID			(('Y'<<24)+('H'<<16)+('P'<<8)+'V')
#define IVPS_LEGACY_ID	(('S'<<24)+('P'<<16)+('V'<<8)+'I')

#define IVPS_VERSION	0x100
#define IVPS_MAX_TRIS_PER_LEDGE	8192 // what is a ledge
#define IVPS_MAX_DIRECTIONS		3 // xyz

namespace ivps
{
	// taken from volt physics
	struct compactedge_t
	{
		unsigned int	start_point_index : 16; // point index
		int				opposite_index : 15; // rel to this // maybe extra array, 3 bits more than tri_index/pierce_index
		unsigned int	is_virtual : 1;
	};

	struct compacttriangle_t
	{
		unsigned int tri_index : 12; // used for upward navigation
		unsigned int pierce_index : 12;
		unsigned int material_index : 7;
		unsigned int is_virtual : 1;

		// three edges
		compactedge_t c_three_edges[3];
	};

	struct compactledgenode_t;

	struct compactledge_t
	{
		int c_point_offset; // byte offset from 'this' to (ledge) point array
		union {
			int ledgetree_node_offset; // 32bit ssize_t, offset into a node
			int client_data;	// if indicates a non terminal ledge, bone id
		};
		unsigned int has_chilren_flag : 2;
		int is_compact_flag : 2;  // if false than compact ledge uses points outside this piece of memory, tldr true for vertexes, false for none, IVP_BOOL
		unsigned int dummy : 4;
		unsigned int size_div_16 : 24;
		short n_triangles;
		short for_future_use;

		inline compacttriangle_t* const pTri(short i) const { assert(i >= 0 && i < n_triangles); return reinterpret_cast<compacttriangle_t*>((char*)this + sizeof(compactledge_t)) + i; }
		inline const Vector* pPoint(int i) const { return reinterpret_cast<const Vector*>((char*)this + c_point_offset + (static_cast<size_t>(16) * static_cast<size_t>(i))); } // aligned
		inline Vector GetPoint(int i) const { return *pPoint(i); }

		inline const compactledgenode_t* const pParentNode() const { return has_chilren_flag ? reinterpret_cast<compactledgenode_t*>((char*)this + ledgetree_node_offset) : nullptr; } // this should return parent node

		inline const int ClientData() const { return -1; } // recursively check children for this?
	};

	struct compactledgenode_t
	{
		int offset_right_node; // (if != 0 than children
		int offset_compact_ledge; //(if != 0, pointer to hull that contains all subelements
		Vector center;	// in object_coords
		float radius; // size of sphere
		unsigned char box_sizes[IVPS_MAX_DIRECTIONS];
		unsigned char free_0;

		inline compactledge_t* const pLedge() const { return offset_compact_ledge ? reinterpret_cast<compactledge_t*>((char*)this + offset_compact_ledge) : nullptr; }

		inline const compactledgenode_t* const pLeftNode() const { return offset_right_node ? this + 1 : nullptr; }
		inline const compactledgenode_t* const pRightNode() const { return offset_right_node ? (compactledgenode_t*)((char*)this + offset_right_node) : nullptr; }

		inline const bool HasChildren() const { return pLedge()->has_chilren_flag ? true : false; }

	};
	// end from volt physics

	struct legacysurfaceheader_t
	{
		//int		size;
		float	mass_center[3];
		float	rotation_inertia[3];
		float	upper_limit_radius;
		unsigned int	max_factor_surface_deviation : 8;
		int		byte_size : 24; // size of whole structure in bytes
		int		offset_ledgetree_root;	// offset to root node of internal ledgetree
		int		dummy[3]; // 16byte memory align

		// the the first 'ledge'
		compactledge_t* const pFirstLedge() const { return reinterpret_cast<compactledge_t*>((char*)this + sizeof(legacysurfaceheader_t)); }
		compactledgenode_t* const pRootNode() const { return reinterpret_cast<compactledgenode_t*>((char*)this + offset_ledgetree_root); }
		compactledgenode_t* const pNode(int i) const { return pRootNode() + i; }

		const int NodeCount() const { return (byte_size - offset_ledgetree_root) / static_cast<int>(sizeof(compactledgenode_t)); } // shouldn't be using this honestly
	};

	// poly checks out, what about the others? for 'modelType' field
	enum VPSModelType_t : int
	{
		TYPE_POLY,
		TYPE_MOPP,
		TYPE_BALL,
		TYPE_VIRTUAL
	};

	struct compactsurfaceheader_t
	{
		int		size; // size of the content after this byte
		int		vphysicsID;
		short	version;
		short	modelType;
		int		surfaceSize;
		Vector	dragAxisAreas;
		int		axisMapSize;

		// get associated legacy surface
		inline legacysurfaceheader_t* const pLegacySurface() const { return reinterpret_cast<legacysurfaceheader_t*>((char*)this + sizeof(compactsurfaceheader_t)); }
		inline compactsurfaceheader_t* const pNextSurface() const { return reinterpret_cast<compactsurfaceheader_t*>((char*)this + size + 4); } // add four as size is size after 'size' var
	};

	struct phyheader_t
	{
		int		size; // size of this data structure
		int		id; // 0 for valve, 1 for apex's new type
		int		solidCount; // number of surface headers
		int		checkSum;	// checksum of source .mdl file

		compactsurfaceheader_t* const pFirstSurface() const { return reinterpret_cast<compactsurfaceheader_t*>((char*)this + size); }
	};
}

// totally real
namespace irps
{
	struct side_t
	{
		char vertIndices[32];
	};

	struct edge_t
	{
		unsigned char vertIndices[2];
	};

	// bad very name
	struct edge_v10_t
	{
		unsigned char vertIndices[2];
		unsigned char sideIndices[2];
	};

	struct solid_t
	{
		float unk_0x0[3]; // first might not be float

		float unk_0xC;

		// Vector
		__int64 vertOffset; // 64bit for conversion into ptr
		__int64 vertCount;

		// side or face
		__int64 sideOffset; // 64bit for conversion into ptr
		__int64 sideCount;

		__int64 edgeOffset; // 64bit for conversion into ptr
		__int64 edgeCount;
	};

	struct solidgroup_t
	{
		__int64 solidOffset; // 64bit for conversion into ptr
		__int64 solidCount;

		float unk_0x10; // scale?? weird

		int unk_0x14[3];

		// idx 0, and 4 match the floats in solid_t
		struct {
			float unk0x0[3];
			int unk_0xC;
		} unk_0x20[5]; // weird

		// differs slightly from model but should be the mostly same assuming it is one solid.
		// this is PER SOLID so on multi-solid phys this will be smaller than the models
		Vector bbMin;
		Vector bbMax;

		char pad[8]; // padding for 16 byte alignment? never seen it filled
	};

	struct phyptrheader_t
	{
		// these are actually all 32 bit ints but with 4 extra bytes per to convert into pointers
		__int64 solidOffset;	// offset into surfaces
		__int64 solidCount;		// Number of solids in file
		__int64 unk_0x8;
		__int64 solidSize;		// size of all surfaces
	};

	struct phyheader_v8_t
	{
		int size;		// size of this data structure
		int id;			// 0 for valve, 1 for apex's new type
		int solidCount;	// number of surface headers
		int checkSum;	// checksum of source .mdl file

		int propertiesOffset;	// string block or size of data after this member
	};

	// how to handle versions for this
	struct phyheader_v16_t
	{
		unsigned short solidCount;			// number of surface headers
		unsigned short propertiesOffset;	// string block or size of data after this member
	};
}