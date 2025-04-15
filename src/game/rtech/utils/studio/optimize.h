#pragma once


#define MAX_NUM_BONES_PER_STRIP 512
#define OPTIMIZED_MODEL_FILE_VERSION 7

#define MAX_NUM_BONES_PER_VERT 3
#define MAX_NUM_EXTRA_BONE_WEIGHTS	16 // for apex legends


#pragma pack(push, 1)
namespace OptimizedModel
{
	struct BoneStateChangeHeader_t
	{
		int hardwareID;
		int newBoneID;
	};
	static_assert(sizeof(BoneStateChangeHeader_t) == 0x8);

	struct Vertex_t
	{
		// these index into the mesh's vert[origMeshVertID]'s bones
		unsigned char boneWeightIndex[MAX_NUM_BONES_PER_VERT];
		unsigned char numBones;

		unsigned short origMeshVertID;

		// for sw skinned verts, these are indices into the global list of bones
		// for hw skinned verts, these are hardware bone indices
		unsigned char boneID[MAX_NUM_BONES_PER_VERT];
	};
	static_assert(sizeof(Vertex_t) == 0x9);

	enum StripHeaderFlags_t
	{
		STRIP_IS_TRILIST = 0x01,
		// STRIP_IS_TRISTRIP	= 0x02 // https://github.com/ValveSoftware/source-sdk-2013/blob/master/sp/src/public/optimize.h#L56
		STRIP_IS_QUADLIST_REG = 0x02,		// Regular sub-d quads
		STRIP_IS_QUADLIST_EXTRA = 0x04		// Extraordinary sub-d quads
	};


	// A strip is a piece of a stripgroup which is divided by bones 
	struct StripHeader_t
	{
		int numIndices;
		int indexOffset;

		int numVerts;
		int vertOffset;

		short numBones;

		unsigned char flags;

		int numBoneStateChanges;
		int boneStateChangeOffset;

		const BoneStateChangeHeader_t* const pBoneStateChange(int i) const
		{
			return numBoneStateChanges ? reinterpret_cast<BoneStateChangeHeader_t*>((char*)this + boneStateChangeOffset) + i : nullptr;
		};

		// MDL Version 49 and up only
		int numTopologyIndices;
		int topologyOffset;
	};
	static_assert(sizeof(StripHeader_t) == 0x23);

	enum StripGroupFlags_t
	{
		STRIPGROUP_IS_HWSKINNED = 0x02,
		STRIPGROUP_IS_DELTA_FLEXED = 0x04,
		STRIPGROUP_SUPPRESS_HW_MORPH = 0x08,	// NOTE: This is a temporary flag used at run time.
	};

	struct StripGroupHeader_t
	{
		// These are the arrays of all verts and indices for this mesh.  strips index into this.
		int numVerts;
		int vertOffset;

		Vertex_t* pVertex(int i) const
		{
			return reinterpret_cast<Vertex_t*>((char*)this + vertOffset) + i;
		}

		int numIndices;
		int indexOffset;

		unsigned short* pIndex(int i) const { return reinterpret_cast<unsigned short*>((char*)this + indexOffset) + i; }
		int Index(int i) { return static_cast<int>(*pIndex(i)); }


		int numStrips;
		int stripOffset;

		StripHeader_t* pStrip(int i) const
		{
			return reinterpret_cast<StripHeader_t*>((char*)this + stripOffset) + i;
		}

		unsigned char flags;
		inline const bool IsHWSkinned() const { return flags & StripGroupFlags_t::STRIPGROUP_IS_HWSKINNED ? true : false; }

		// The following fields are only present if MDL version is >=49
		// Points to an array of unsigned shorts (16 bits each)
		int numTopologyIndices;
		int topologyOffset;

		unsigned short* pTopologyIndex(int i) const
		{
			return reinterpret_cast<unsigned short*>((char*)this + topologyOffset) + i;
		}
	};
	static_assert(sizeof(StripGroupHeader_t) == 0x21);

	// likely unused in Respawn games as mouths and eyes are cut.
	enum MeshFlags_t {
		// these are both material properties, and a mesh has a single material.
		MESH_IS_TEETH = 0x01,
		MESH_IS_EYES = 0x02
	};

	struct MeshHeader_t
	{
		int numStripGroups;
		int stripGroupHeaderOffset;

		unsigned char flags; // never read these as eyeballs and mouths are depreciated

		StripGroupHeader_t* pStripGroup(int i) const
		{
			return reinterpret_cast<StripGroupHeader_t*>((char*)this + stripGroupHeaderOffset) + i;
		}
	};
	static_assert(sizeof(MeshHeader_t) == 0x9);

	struct ModelLODHeader_t
	{
		//Mesh array
		int numMeshes;
		int meshOffset;

		float switchPoint;

		MeshHeader_t* pMesh(int i) const
		{
			return reinterpret_cast<MeshHeader_t*>((char*)this + meshOffset) + i;
		}
	};
	static_assert(sizeof(ModelLODHeader_t) == 0xC);

	struct ModelHeader_t
	{
		//LOD mesh array
		int numLODs;   //This is also specified in FileHeader_t
		int lodOffset;

		ModelLODHeader_t* pLOD(int i) const
		{
			return reinterpret_cast<ModelLODHeader_t*>((char*)this + lodOffset) + i;
		}
	};
	static_assert(sizeof(ModelHeader_t) == 0x8);

	struct BodyPartHeader_t
	{
		// Model array
		int numModels;
		int modelOffset;

		ModelHeader_t* pModel(int i) const
		{
			return reinterpret_cast<ModelHeader_t*>((char*)this + modelOffset) + i;
		}
	};
	static_assert(sizeof(BodyPartHeader_t) == 0x8);

	struct MaterialReplacementHeader_t
	{
		short materialID;
		int replacementMaterialNameOffset;
		char* pMaterialReplacementName() const
		{
			return ((char*)this + replacementMaterialNameOffset);
		}
	};
	static_assert(sizeof(MaterialReplacementHeader_t) == 0x6);

	struct MaterialReplacementListHeader_t
	{
		int numReplacements;
		int replacementOffset;

		MaterialReplacementHeader_t* pMaterialReplacement(int i) const
		{
			return reinterpret_cast<MaterialReplacementHeader_t*>((char*)this + replacementOffset) + i;
		}
	};
	static_assert(sizeof(MaterialReplacementListHeader_t) == 0x8);

	struct FileHeader_t
	{
		// file version as defined by OPTIMIZED_MODEL_FILE_VERSION (currently 7)
		int version;

		// hardware params that affect how the model is to be optimized.
		int vertCacheSize;
		unsigned short maxBonesPerStrip;
		unsigned short maxBonesPerFace;
		int maxBonesPerVert; // max of 16 in apex

		// must match checkSum in the .mdl
		int checkSum;

		int numLODs; // Also specified in ModelHeader_t's and should match

		// Offset to materialReplacementList Array. one of these for each LOD, 8 in total
		int materialReplacementListOffset;

		MaterialReplacementListHeader_t* pMaterialReplacementList(int lodID) const
		{
			return reinterpret_cast<MaterialReplacementListHeader_t*>((char*)this + materialReplacementListOffset) + lodID;
		}

		// Defines the size and location of the body part array
		int numBodyParts;
		int bodyPartOffset;

		BodyPartHeader_t* pBodyPart(int i) const
		{
			return reinterpret_cast<BodyPartHeader_t*>((char*)this + bodyPartOffset) + i;
		}
	};
	static_assert(sizeof(FileHeader_t) == 0x24);
}
#pragma pack(pop)
