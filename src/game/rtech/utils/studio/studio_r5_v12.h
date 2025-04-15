#pragma once

// contains versions:
// 12.1, 12.2, 12.3, 13, 13.1, 14, 14.1, 15
namespace r5
{
	struct studio_hw_groupdata_v12_1_t
	{
		int dataOffset; // offset to this section in vg
		int dataSize;	// size of data in vg

		short lodIndex;		// base lod idx?
		short lodCount;		// number of lods contained within this group
		short lodMap;		// lods in this group, each bit is a lod
	};
	static_assert(sizeof(studio_hw_groupdata_v12_1_t) == 16);

	struct mstudioanimsections_v12_1_t
	{
		int animindex;
		bool isExternal; // 0 or 1, if 1 section is not in local data
	};
	static_assert(sizeof(mstudioanimsections_v12_1_t) == 0x8);

	struct mstudioanimdesc_v12_1_t
	{
		int baseptr;

		int sznameindex;
		inline const char* pszName() const { return ((char*)this + sznameindex); }

		float fps; // frames per second	
		int flags; // looping/non-looping flags

		int numframes;

		// piecewise movement
		int nummovements;
		int movementindex;

		int framemovementindex; // new in v52
		inline r5::mstudioframemovement_t* pFrameMovement() const { return reinterpret_cast<r5::mstudioframemovement_t*>((char*)this + framemovementindex); }

		int animindex; // non-zero when anim data isn't in sections

		int numikrules;
		int ikruleindex; // non-zero when IK data is stored in the mdl

		int sectionindex;
		int sectionstallframes; // number of stall frames inside the animation, the reset excluding the final frame are stored externally. when external data is not loaded(?)/found(?) it falls back on the last frame of this as a stall
		int sectionframes; // number of frames used in each fast lookup section, zero if not used
		inline const mstudioanimsections_v12_1_t* pSection(int i) const { return reinterpret_cast<mstudioanimsections_v12_1_t*>((char*)this + sectionindex) + i; }

		// both of these are used in pAnim
		// todo: figure these out
		int64_t unk1;
		int64_t unk2; // gets converted to an offset on load?
	};

	#pragma pack(push, 1)
	struct mstudiomesh_v12_1_t
	{
		uint16_t material;

		uint16_t unk;	// unused in v14 as string gets written over it (mdl/vehicles_r5\land_med\msc_freight_tortus_mod\veh_land_msc_freight_tortus_mod_cargo_holder_v1_static.rmdl)
		// seemingly added in v14, but unused in v14/15, maybe this carried over to v16?
		int modelindex;

		int numvertices; // number of unique vertices/normals/texcoords
		int vertexoffset; // vertex mstudiovertex_t
		// offset by vertexoffset number of verts into vvd vertexes, relative to the models offset

		// a unique ordinal for this mesh
		int meshid;

		Vector center;

		mstudio_meshvertexloddata_t vertexloddata;

		void* unkptr; // unknown memory pointer, probably one of the older vertex pointers but moved
	};
	#pragma pack(pop)
	static_assert(sizeof(mstudiomesh_v12_1_t) == 0x4C);

	struct mstudiomodel_v12_1_t
	{
		char name[64];

		int unkStringOffset; // byte before string block

		// it looks like they write the entire name
		// then write over it with other values where needed
		// why.
		int type;

		float boundingradius;

		int nummeshes;
		int meshindex;
		const mstudiomesh_v12_1_t* const pMesh(int i) const { return reinterpret_cast<mstudiomesh_v12_1_t*>((char*)this + meshindex) + i; }

		// cache purposes
		int numvertices; // number of unique vertices/normals/texcoords
		int vertexindex; // vertex Vector
		int tangentsindex; // tangents Vector

		int numattachments;
		int attachmentindex;

		int colorindex; // vertex color
		// offset by colorindex number of bytes into vvc vertex colors
		int uv2index; // vertex second uv map
		// offset by uv2index number of bytes into vvc secondary uv map
	};
	static_assert(sizeof(mstudiomodel_v12_1_t) == 0x70);

	// mdl_ v13.1
	struct mstudiomodel_v13_1_t
	{
		inline const mstudiomodel_v12_1_t* const AsV12_1() const { return reinterpret_cast<const mstudiomodel_v12_1_t* const>(this); }

		char name[64];

		int unkStringOffset; // byte before string block

		// it looks like they write the entire name
		// then write over it with other values where needed
		// why.
		int type;

		float boundingradius;

		int nummeshes;
		int meshindex;

		// cache purposes
		int numvertices; // number of unique vertices/normals/texcoords
		int vertexindex; // vertex Vector
		int tangentsindex; // tangents Vector

		int numattachments;
		int attachmentindex;

		int colorindex; // vertex color
		// offset by colorindex number of bytes into vvc vertex colors
		int uv2index; // vertex second uv map
		// offset by uv2index number of bytes into vvc secondary uv map

		int uv3index; // same as uv2index, probably replaces uv2 data
	};
	static_assert(sizeof(mstudiomodel_v13_1_t) == 0x74);

	// mdl_ v14
	struct mstudiomodel_v14_t
	{
		char name[64];

		int unkStringOffset; // byte before string block

		// they write over these two when it's the default
		int type;

		float boundingradius;

		int meshCountTotal;
		int meshCountBase;
		int meshCountBlends;

		int meshindex;

		mstudiomesh_v12_1_t* pMesh(int i) const
		{
			return reinterpret_cast<mstudiomesh_v12_1_t*>((char*)this + meshindex) + i;
		}

		// most of these vtx, vvd, vvc, and vg indexes are depreciated after v14.1 (s14)

		// cache purposes
		int numvertices; // number of unique vertices/normals/texcoords
		int vertexindex; // vertex Vector
		int tangentsindex; // tangents Vector

		int numattachments;
		int attachmentindex;

		int colorindex; // vertex color
		int uv2index; // vertex second uv map
		int unkOffset;
	};

	// mdl_ v15
	struct mstudiobodyparts_v15_t
	{
		int sznameindex;
		int nummodels;
		int base;
		int modelindex; // index into models array

		int unk;
		int meshOffset; // offset to this bodyparts meshes

		inline const mstudiobodyparts_t* const AsV8() const { return reinterpret_cast<const mstudiobodyparts_t* const>(this); }
	};

	struct mstudiobone_v12_1_t
	{
		int sznameindex;
		inline char* const pszName() const { return ((char*)this + sznameindex); }

		int parent; // parent bone
		int bonecontroller[6]; // bone controller index, -1 == none

		// default values
		Vector pos; // base bone position
		Quaternion quat;
		RadianEuler rot; // base bone rotation
		Vector scale; // base bone scale

		matrix3x4_t poseToBone;
		Quaternion qAlignment;

		int flags;
		int proctype;
		int procindex; // procedural rule offset
		int physicsbone; // index into physically simulated bone
		// from what I can tell this is the section that is parented to this bone, and if this bone is not the parent of any sections, it goes up the bone chain to the nearest bone that does and uses that section index
		int surfacepropidx; // index into string tablefor property name
		inline char* const pszSurfaceProp() const { return ((char*)this + surfacepropidx); }

		int contents; // See BSPFlags.h for the contents flags

		int surfacepropLookup; // this index must be cached by the loader, not saved in the file

		uint8_t collisionIndex; // index into sections of collision, phy, bvh, etc. needs confirming

		uint8_t unk_B1[3]; // maybe this is 'unk'?
	};
	static_assert(sizeof(mstudiobone_v12_1_t) == 0xB4);

	struct studiohdr_v12_1_t
	{
		int id; // Model format ID, such as "IDST" (0x49 0x44 0x53 0x54)
		int version; // Format version number, such as 54 (0x36,0x00,0x00,0x00)
		int checksum; // This has to be the same in the phy and vtx files to load!
		int sznameindex; // This has been moved from studiohdr2 to the front of the main header.
		char name[64]; // The internal name of the model, padding with null chars.
		// Typically "my_model.mdl" will have an internal name of "my_model"
		int length; // Data size of MDL file in chars.

		Vector eyeposition;	// ideal eye position

		Vector illumposition;	// illumination center

		Vector hull_min;		// ideal movement hull size
		Vector hull_max;

		Vector view_bbmin;		// clipping bounding box
		Vector view_bbmax;

		int flags;

		int numbones; // bones
		int boneindex;

		int numbonecontrollers; // bone controllers
		int bonecontrollerindex;

		int numhitboxsets;
		int hitboxsetindex;

		// seemingly unused now, as animations are per sequence
		int numlocalanim; // animations/poses
		int localanimindex; // animation descriptions

		int numlocalseq; // sequences
		int	localseqindex;

		int activitylistversion; // initialization flag - have the sequences been indexed?

		// mstudiotexture_t
		// short rpak path
		// raw textures
		int materialtypesindex; // index into an array of char sized material type enums for each material used by the model
		int numtextures; // the material limit exceeds 128, probably 256.
		int textureindex;

		// this should always only be one, unless using vmts.
		// raw textures search paths
		int numcdtextures;
		int cdtextureindex;

		// replaceable textures tables
		int numskinref;
		int numskinfamilies;
		int skinindex;

		int numbodyparts;
		int bodypartindex;
		inline const mstudiobodyparts_t* pBodypart(int i) const { return reinterpret_cast<const mstudiobodyparts_t*>(reinterpret_cast<const char*>(this) + bodypartindex) + i; }

		int numlocalattachments;
		int localattachmentindex;

		int numlocalnodes;
		int localnodeindex;
		int localnodenameindex;
		int localNodeUnk; // used sparsely in r2, unused in apex, removed in v16 rmdl
		int localNodeDataOffset; // offset into an array of int sized offsets that read into the data for each nod

		int numikchains;
		int ikchainindex;

		// mesh panels for using rui on models, primarily for weapons
		int uiPanelCount;
		int uiPanelOffset;

		int numlocalposeparameters;
		int localposeparamindex;

		int surfacepropindex;

		int keyvalueindex;
		int keyvaluesize;

		int numlocalikautoplaylocks;
		int localikautoplaylockindex;

		float mass;
		int contents;

		// unused for packed models
		int numincludemodels;
		int includemodelindex;

		int /* mutable void* */ virtualModel;

		int bonetablebynameindex;

		/// hw data lookup from rmdl
		int meshCount; // number of meshes per lod
		int meshOffset;

		int boneStateOffset;
		int boneStateCount;
		inline const uint8_t* pBoneStates() const { return boneStateCount > 0 ? reinterpret_cast<uint8_t*>((char*)this + offsetof(studiohdr_v12_1_t, boneStateOffset) + boneStateOffset) : nullptr; }

		int unk_v12_1; // related to vg likely

		int hwDataSize;

		// weird hw data stuff
		// duplicates?
		// NAMES NEEDED
		short vgUnk; // same as padding in vg header
		short vgLODCount; // same as lod count in vg header

		int lodMap; // lods in this group, each bit is a lod

		// sets of lods
		int groupHeaderOffset;
		int groupHeaderCount;
		const studio_hw_groupdata_v12_1_t* const pLODGroup(int i) const { return reinterpret_cast<studio_hw_groupdata_v12_1_t*>((char*)this + offsetof(studiohdr_v12_1_t, groupHeaderOffset) + groupHeaderOffset) + i; }

		int lodOffset;
		int lodCount;  // check this

		float fadeDistance;
		float gatherSize; // what. from r5r struct

		float flVertAnimFixedPointScale; // to be verified
		int surfacepropLookup; // saved in the file

		// this is in most shipped models, probably part of their asset bakery.
		// doesn't actually need to be written pretty sure, only four chars when not present.
		// this is not completely true as some models simply have nothing, such as animation models.
		int sourceFilenameOffset;

		int numsrcbonetransform;
		int srcbonetransformindex;

		int	illumpositionattachmentindex;

		int linearboneindex;

		// unsure what this is for but it exists for jigglbones
		int procBoneCount;
		int procBoneOffset;
		int linearProcBoneOffset;

		// always "" or "Titan"
		int unkStringOffset;

		// offsets into vertex buffer for component files, suzes are per file if course
		// unused besides phy starting in rmdl v9
		int vtxOffset; // VTX
		int vvdOffset; // VVD / IDSV
		int vvcOffset; // VVC / IDCV 
		int phyOffset; // VPHY / IVPS

		int vtxSize;
		int vvdSize;
		int vvcSize;
		int phySize; // still used in models using vg

		// mostly seen on '_animated' suffixed models
		// manually declared bone followers are no longer stored in kvs under 'bone_followers', they are now stored in an array of ints with the bone index.
		int boneFollowerCount;
		int boneFollowerOffset;

		// BVH4 size (?)
		Vector bvhMin;
		Vector bvhMax; // seem to be the same as hull size

		int bvhOffset; // bvh4 tree

		short bvhUnk[2]; // collision detail for bvh (?)

		// new in apex vertex weight file for verts that have more than three weights
		// vvw is a 'fake' extension name, we do not know the proper name.
		int vvwOffset; // index will come last after other vertex files
		int vvwSize;
	};

	struct studiohdr_v12_2_t
	{
		int id; // Model format ID, such as "IDST" (0x49 0x44 0x53 0x54)
		int version; // Format version number, such as 54 (0x36,0x00,0x00,0x00)
		int checksum; // This has to be the same in the phy and vtx files to load!
		int sznameindex; // This has been moved from studiohdr2 to the front of the main header.
		char name[64]; // The internal name of the model, padding with null chars.
		// Typically "my_model.mdl" will have an internal name of "my_model"
		int length; // Data size of MDL file in chars.

		Vector eyeposition;	// ideal eye position

		Vector illumposition;	// illumination center

		Vector hull_min;		// ideal movement hull size
		Vector hull_max;

		Vector view_bbmin;		// clipping bounding box
		Vector view_bbmax;

		int flags;

		int numbones; // bones
		int boneindex;

		int numbonecontrollers; // bone controllers
		int bonecontrollerindex;

		int numhitboxsets;
		int hitboxsetindex;

		// seemingly unused now, as animations are per sequence
		int numlocalanim; // animations/poses
		int localanimindex; // animation descriptions

		int numlocalseq; // sequences
		int	localseqindex;

		int activitylistversion; // initialization flag - have the sequences been indexed?

		// mstudiotexture_t
		// short rpak path
		// raw textures
		int materialtypesindex; // index into an array of char sized material type enums for each material used by the model
		int numtextures; // the material limit exceeds 128, probably 256.
		int textureindex;

		// this should always only be one, unless using vmts.
		// raw textures search paths
		int numcdtextures;
		int cdtextureindex;

		// replaceable textures tables
		int numskinref;
		int numskinfamilies;
		int skinindex;

		int numbodyparts;
		int bodypartindex;

		int numlocalattachments;
		int localattachmentindex;

		int numlocalnodes;
		int localnodeindex;
		int localnodenameindex;
		int localNodeUnk; // used sparsely in r2, unused in apex, removed in v16 rmdl
		int localNodeDataOffset; // offset into an array of int sized offsets that read into the data for each nod

		int numikchains;
		int ikchainindex;

		// mesh panels for using rui on models, primarily for weapons
		int uiPanelCount;
		int uiPanelOffset;

		int numlocalposeparameters;
		int localposeparamindex;

		int surfacepropindex;

		int keyvalueindex;
		int keyvaluesize;

		int numlocalikautoplaylocks;
		int localikautoplaylockindex;

		float mass;
		int contents;

		// unused for packed models
		int numincludemodels;
		int includemodelindex;

		int /* mutable void* */ virtualModel;

		int bonetablebynameindex;

		// hw data lookup from rmdl
		int meshCount; // number of meshes per lod
		int meshOffset;

		int boneStateOffset;
		int boneStateCount;

		int unk_v12_1; // related to vg likely

		int hwDataSize;

		// weird hw data stuff
		// duplicates?
		// NAMES NEEDED
		short vgUnk; // same as padding in vg header
		short vgLODCount; // same as lod count in vg header

		int lodMap; // lods in this group, each bit is a lod

		// sets of lods
		int groupHeaderOffset;
		int groupHeaderCount;
		const studio_hw_groupdata_v12_1_t* const pLODGroup(int i) const { return reinterpret_cast<studio_hw_groupdata_v12_1_t*>((char*)this + offsetof(studiohdr_v12_2_t, groupHeaderOffset) + groupHeaderOffset) + i; }

		int lodOffset;
		int lodCount;  // check this

		float fadeDistance;
		float gatherSize; // what. from r5r struct

		float flVertAnimFixedPointScale; // to be verified
		int surfacepropLookup; // saved in the file

		int unk_v12_2; // added in transition version

		// this is in most shipped models, probably part of their asset bakery.
		// doesn't actually need to be written pretty sure, only four chars when not present.
		// this is not completely true as some models simply have nothing, such as animation models.
		int sourceFilenameOffset;

		int numsrcbonetransform;
		int srcbonetransformindex;

		int	illumpositionattachmentindex;

		int linearboneindex;

		// unsure what this is for but it exists for jigglbones
		int procBoneCount;
		int procBoneOffset;
		int linearProcBoneOffset;

		// always "" or "Titan"
		int unkStringOffset;

		// offsets into vertex buffer for component files, suzes are per file if course
		// unused besides phy starting in rmdl v9
		int vtxOffset; // VTX
		int vvdOffset; // VVD / IDSV
		int vvcOffset; // VVC / IDCV 
		int phyOffset; // VPHY / IVPS

		int vtxSize;
		int vvdSize;
		int vvcSize;
		int phySize; // still used in models using vg

		// mostly seen on '_animated' suffixed models
		// manually declared bone followers are no longer stored in kvs under 'bone_followers', they are now stored in an array of ints with the bone index.
		int boneFollowerCount;
		int boneFollowerOffset;

		// BVH4 size (?)
		Vector bvhMin;
		Vector bvhMax; // seem to be the same as hull size

		int bvhOffset; // bvh4 tree

		short bvhUnk[2]; // collision detail for bvh (?)

		// new in apex vertex weight file for verts that have more than three weights
		// vvw is a 'fake' extension name, we do not know the proper name.
		int vvwOffset; // index will come last after other vertex files
		int vvwSize;
	};

	struct studiohdr_v12_3_t
	{
		int id; // Model format ID, such as "IDST" (0x49 0x44 0x53 0x54)
		int version; // Format version number, such as 54 (0x36,0x00,0x00,0x00)
		int checksum; // This has to be the same in the phy and vtx files to load!
		int sznameindex; // This has been moved from studiohdr2 to the front of the main header.
		char name[64]; // The internal name of the model, padding with null chars.
		int length; // Data size of MDL file in chars.
		inline const char* pszName() const { return ((char*)this + sznameindex); }

		Vector eyeposition;	// ideal eye position

		Vector illumposition;	// illumination center

		Vector hull_min;		// ideal movement hull size
		Vector hull_max;

		Vector view_bbmin;		// clipping bounding box
		Vector view_bbmax;

		int flags;

		int numbones; // bones
		int boneindex;

		int numbonecontrollers; // bone controllers
		int bonecontrollerindex;

		int numhitboxsets;
		int hitboxsetindex;

		// seemingly unused now, as animations are per sequence
		int numlocalanim; // animations/poses
		int localanimindex; // animation descriptions

		int numlocalseq; // sequences
		int	localseqindex;

		int activitylistversion; // initialization flag - have the sequences been indexed?

		// mstudiotexture_t
		// short rpak path
		// raw textures
		int materialtypesindex; // index into an array of char sized material type enums for each material used by the model
		int numtextures; // the material limit exceeds 128, probably 256.
		int textureindex;

		// this should always only be one, unless using vmts.
		// raw textures search paths
		int numcdtextures;
		int cdtextureindex;

		// replaceable textures tables
		int numskinref;
		int numskinfamilies;
		int skinindex;

		int numbodyparts;
		int bodypartindex;
		inline const mstudiobodyparts_t* pBodypart(int i) const { return reinterpret_cast<const mstudiobodyparts_t*>(reinterpret_cast<const char*>(this) + bodypartindex) + i; }

		int numlocalattachments;
		int localattachmentindex;

		int numlocalnodes;
		int localnodeindex;
		int localnodenameindex;
		int localNodeUnk; // used sparsely in r2, unused in apex, removed in v16 rmdl
		int localNodeDataOffset; // offset into an array of int sized offsets that read into the data for each nod

		int numikchains;
		int ikchainindex;

		// mesh panels for using rui on models, primarily for weapons
		int uiPanelCount;
		int uiPanelOffset;

		int numlocalposeparameters;
		int localposeparamindex;

		int surfacepropindex;

		int keyvalueindex;
		int keyvaluesize;

		int numlocalikautoplaylocks;
		int localikautoplaylockindex;

		float mass;
		int contents;

		// unused for packed models
		int numincludemodels;
		int includemodelindex;

		int /* mutable void* */ virtualModel;

		int bonetablebynameindex;

		// hw data lookup from rmdl
		int meshCount; // number of meshes per lod
		int meshOffset;

		int boneStateOffset;
		int boneStateCount;

		int unk_v12_1; // related to vg likely

		int hwDataSize;

		// weird hw data stuff
		// duplicates?
		// NAMES NEEDED
		short vgUnk; // same as padding in vg header
		short vgLODCount; // same as lod count in vg header

		int lodMap; // lods in this group, each bit is a lod

		// sets of lods
		int groupHeaderOffset;
		int groupHeaderCount;
		const studio_hw_groupdata_v12_1_t* const pLODGroup(int i) const { return reinterpret_cast<studio_hw_groupdata_v12_1_t*>((char*)this + offsetof(studiohdr_v12_3_t, groupHeaderOffset) + groupHeaderOffset) + i; }

		int lodOffset;
		int lodCount;  // check this

		float fadeDistance;
		float gatherSize; // what. from r5r struct

		float flVertAnimFixedPointScale; // to be verified
		int surfacepropLookup; // saved in the file

		int unk_v12_2; // added in v12.2

		// this is in most shipped models, probably part of their asset bakery.
		// doesn't actually need to be written pretty sure, only four chars when not present.
		// this is not completely true as some models simply have nothing, such as animation models.
		int sourceFilenameOffset;

		int numsrcbonetransform;
		int srcbonetransformindex;

		int	illumpositionattachmentindex;

		int linearboneindex;

		// unsure what this is for but it exists for jigglbones
		int procBoneCount;
		int procBoneOffset;
		int linearProcBoneOffset;

		// always "" or "Titan"
		int unkStringOffset;

		// offsets into vertex buffer for component files, suzes are per file if course
		// unused besides phy starting in rmdl v9
		int vtxOffset; // VTX
		int vvdOffset; // VVD / IDSV
		int vvcOffset; // VVC / IDCV 
		int phyOffset; // VPHY / IVPS

		int vtxSize;
		int vvdSize;
		int vvcSize;
		int phySize; // still used in models using vg

		// mostly seen on '_animated' suffixed models
		// manually declared bone followers are no longer stored in kvs under 'bone_followers', they are now stored in an array of ints with the bone index.
		int boneFollowerCount;
		int boneFollowerOffset;

		// BVH4 size (?)
		Vector bvhMin;
		Vector bvhMax; // seem to be the same as hull size

		int bvhOffset; // bvh4 tree

		short bvhUnk[2]; // collision detail for bvh (?)

		// new in apex vertex weight file for verts that have more than three weights
		// vvw is a 'fake' extension name, we do not know the proper name.
		int vvwOffset; // index will come last after other vertex files
		int vvwSize;

		int unk_v12_3[3]; // added in rmdl v12.3
	};

	struct studiohdr_v14_t
	{
		int id; // Model format ID, such as "IDST" (0x49 0x44 0x53 0x54)
		int version; // Format version number, such as 54 (0x36,0x00,0x00,0x00)
		int checksum; // This has to be the same in the phy and vtx files to load!
		int sznameindex; // This has been moved from studiohdr2 to the front of the main header.
		char name[64]; // The internal name of the model, padding with null chars.
		int length; // Data size of MDL file in chars.
		inline const char* pszName() const { return ((char*)this + sznameindex); }

		Vector eyeposition;	// ideal eye position

		Vector illumposition;	// illumination center

		Vector hull_min;		// ideal movement hull size
		Vector hull_max;

		Vector view_bbmin;		// clipping bounding box
		Vector view_bbmax;

		int flags;

		int numbones; // bones
		int boneindex;

		int numbonecontrollers; // bone controllers
		int bonecontrollerindex;

		int numhitboxsets;
		int hitboxsetindex;

		// seemingly unused now, as animations are per sequence
		int numlocalanim; // animations/poses
		int localanimindex; // animation descriptions

		int numlocalseq; // sequences
		int	localseqindex;

		int unk_v14[2]; // added in v13 -> v14

		int activitylistversion; // initialization flag - have the sequences been indexed?

		// mstudiotexture_t
		// short rpak path
		// raw textures
		int materialtypesindex; // index into an array of char sized material type enums for each material used by the model
		int numtextures; // the material limit exceeds 128, probably 256.
		int textureindex;

		// this should always only be one, unless using vmts.
		// raw textures search paths
		int numcdtextures;
		int cdtextureindex;

		// replaceable textures tables
		int numskinref;
		int numskinfamilies;
		int skinindex;

		int numbodyparts;
		int bodypartindex;
		inline const mstudiobodyparts_t* pBodypart(int i) const { return reinterpret_cast<const mstudiobodyparts_t*>(reinterpret_cast<const char*>(this) + bodypartindex) + i; }
		inline const mstudiobodyparts_v15_t* pBodypart_V15(int i) const { return reinterpret_cast<const mstudiobodyparts_v15_t*>(reinterpret_cast<const char*>(this) + bodypartindex) + i; }

		int numlocalattachments;
		int localattachmentindex;

		int numlocalnodes;
		int localnodeindex;
		int localnodenameindex;
		int localNodeUnk; // used sparsely in r2, unused in apex, removed in v16 rmdl
		int localNodeDataOffset; // offset into an array of int sized offsets that read into the data for each nod

		int numikchains;
		int ikchainindex;

		// mesh panels for using rui on models, primarily for weapons
		int uiPanelCount;
		int uiPanelOffset;

		int numlocalposeparameters;
		int localposeparamindex;

		int surfacepropindex;

		int keyvalueindex;
		int keyvaluesize;

		int numlocalikautoplaylocks;
		int localikautoplaylockindex;

		float mass;
		int contents;

		// unused for packed models
		int numincludemodels;
		int includemodelindex;

		int /* mutable void* */ virtualModel;

		int bonetablebynameindex;

		// hw data lookup from rmdl
		int meshCount; // number of meshes per lod
		int meshOffset;

		int boneStateOffset;
		int boneStateCount;
		inline const uint8_t* pBoneStates() const { return boneStateCount > 0 ? reinterpret_cast<uint8_t*>((char*)this + offsetof(studiohdr_v14_t, boneStateOffset) + boneStateOffset) : nullptr; }

		int unk_v12_1; // related to vg likely

		int hwDataSize;

		// weird hw data stuff
		// duplicates?
		// NAMES NEEDED
		short vgUnk; // same as padding in vg header
		short vgLODCount; // same as lod count in vg header

		int lodMap; // lods in this group, each bit is a lod

		// sets of lods
		int groupHeaderOffset;
		int groupHeaderCount;
		const studio_hw_groupdata_v12_1_t* const pLODGroup(int i) const { return reinterpret_cast<studio_hw_groupdata_v12_1_t*>((char*)this + offsetof(studiohdr_v14_t, groupHeaderOffset) + groupHeaderOffset) + i; }

		int lodOffset;
		int lodCount;  // check this

		float fadeDistance;
		float gatherSize; // what. from r5r struct

		float flVertAnimFixedPointScale; // to be verified
		int surfacepropLookup; // saved in the file

		int unk_v12_2; // added in v12.2

		// this is in most shipped models, probably part of their asset bakery.
		// doesn't actually need to be written pretty sure, only four chars when not present.
		// this is not completely true as some models simply have nothing, such as animation models.
		int sourceFilenameOffset;

		int numsrcbonetransform;
		int srcbonetransformindex;

		int	illumpositionattachmentindex;

		int linearboneindex;

		// unsure what this is for but it exists for jigglbones
		int procBoneCount;
		int procBoneOffset;
		int linearProcBoneOffset;

		// always "" or "Titan"
		int unkStringOffset;

		// offsets into vertex buffer for component files, suzes are per file if course
		// unused besides phy starting in rmdl v9
		int vtxOffset; // VTX
		int vvdOffset; // VVD / IDSV
		int vvcOffset; // VVC / IDCV 
		int phyOffset; // VPHY / IVPS

		int vtxSize;
		int vvdSize;
		int vvcSize;
		int phySize; // still used in models using vg

		// mostly seen on '_animated' suffixed models
		// manually declared bone followers are no longer stored in kvs under 'bone_followers', they are now stored in an array of ints with the bone index.
		int boneFollowerCount;
		int boneFollowerOffset;

		// BVH4 size (?)
		Vector bvhMin;
		Vector bvhMax; // seem to be the same as hull size

		int bvhOffset; // bvh4 tree

		short bvhUnk[2]; // collision detail for bvh (?)

		// new in apex vertex weight file for verts that have more than three weights
		// vvw is a 'fake' extension name, we do not know the proper name.
		int vvwOffset; // index will come last after other vertex files
		int vvwSize;

		int unk_v12_3[3]; // added in rmdl v12.3
	};
}