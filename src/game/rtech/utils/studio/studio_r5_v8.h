#pragma once

// containts versions:
// 8, 9, 10, 11, 12
namespace r5
{
	// [rika]: awesome, please do not remove this, thank you.
	#pragma pack(push, 1)
	struct mstudiotexture_v8_t
	{
		int sznameindex;
		uint64_t guid;

		inline const char* pszName() const { return reinterpret_cast<char*>((char*)this + sznameindex); }
	};
	static_assert(sizeof(mstudiotexture_v8_t) == 0xC);

	struct mstudiomesh_v8_t
	{
		int material;

		int modelindex;

		int numvertices; // number of unique vertices/normals/texcoords
		int vertexoffset; // vertex mstudiovertex_t
		// offset by vertexoffset number of verts into vvd vertexes, relative to the models offset

		// Access thin/fat mesh vertex data (only one will return a non-NULL result)

		int deprecated_numflexes; // vertex animation
		int deprecated_flexindex;

		// special codes for material operations
		int deprecated_materialtype;
		int deprecated_materialparam;

		// a unique ordinal for this mesh
		int meshid;

		Vector center;

		mstudio_meshvertexloddata_t vertexloddata;

		void* unkptr; // unknown memory pointer, probably one of the older vertex pointers but moved
	};
	#pragma pack(pop)
	static_assert(sizeof(mstudiomesh_v8_t) == 0x5C);

	struct mstudiomodel_v8_t
	{
		char name[64];

		int unkStringOffset; // goes to bones sometimes

		int type;

		float boundingradius;

		int nummeshes;
		int meshindex;
		const mstudiomesh_v8_t* const pMesh(int i) const { return reinterpret_cast<mstudiomesh_v8_t*>((char*)this + meshindex) + i; }

		// cache purposes
		int numvertices;	// number of unique vertices/normals/texcoords
		int vertexindex;	// vertex Vector
		// offset by vertexindex number of chars into vvd verts
		int tangentsindex;	// tangents Vector
		// offset by tangentsindex number of chars into vvd tangents

		int numattachments;
		int attachmentindex;

		int deprecated_numeyeballs;
		int deprecated_eyeballindex;

		int pad[4];

		int colorindex; // vertex color
		// offset by colorindex number of chars into vvc vertex colors
		int uv2index;	// vertex second uv map
		// offset by uv2index number of chars into vvc secondary uv map
	};
	static_assert(sizeof(mstudiomodel_v8_t) == 0x88);

	struct mstudiobone_v8_t
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

		// not required for our uses
		int unk[2];
	};

	// aseq v7
	struct mstudioanimsections_v8_t
	{
		int animindex;
	};

	struct mstudioanimdesc_v12_1_t;
	struct mstudioanimdesc_v8_t
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

		int framemovementindex;
		inline mstudioframemovement_t* pFrameMovement() const { return reinterpret_cast<mstudioframemovement_t*>((char*)this + framemovementindex); }

		int animindex; // non-zero when anim data isn't in sections

		int numikrules;
		int ikruleindex; // non-zero when IK data is stored in the mdl

		int sectionindex;
		int sectionframes; // number of frames used in each fast lookup section, zero if not used
		inline const mstudioanimsections_v8_t* pSection(const int i) const { return reinterpret_cast<mstudioanimsections_v8_t*>((char*)this + sectionindex) + i; }
	};
	static_assert(sizeof(mstudioanimdesc_v8_t) == 0x34);

	// aseq v7
	struct mstudioseqdesc_v8_t
	{
		int baseptr;

		int	szlabelindex;
		const char* pszLabel() const { return ((char*)this + szlabelindex); }

		int szactivitynameindex;
		const char* pszActivity() const { return ((char*)this + szactivitynameindex); }

		int flags; // looping/non-looping flags

		int activity; // initialized at loadtime to game DLL values
		int actweight;

		int numevents;
		int eventindex;

		Vector bbmin; // per sequence bounding box
		Vector bbmax;

		int numblends;

		// Index into array of ints which is groupsize[0] x groupsize[1] in length
		int animindexindex;
		const int AnimIndex(const int i) const { return reinterpret_cast<int*>((char*)this + animindexindex)[i]; }
		const int AnimCount() const { return  groupsize[0] * groupsize[1]; }
		mstudioanimdesc_v8_t* pAnimDescV8(const int i) const { return reinterpret_cast<mstudioanimdesc_v8_t*>((char*)this + AnimIndex(i)); }
		mstudioanimdesc_v12_1_t* pAnimDescV12_1(const int i) const { return reinterpret_cast<mstudioanimdesc_v12_1_t*>((char*)this + AnimIndex(i)); }

		int movementindex; // [blend] float array for blended movement
		int groupsize[2];
		int paramindex[2]; // X, Y, Z, XR, YR, ZR
		float paramstart[2]; // local (0..1) starting value
		float paramend[2]; // local (0..1) ending value
		int paramparent;

		float fadeintime; // ideal cross fate in time (0.2 default)
		float fadeouttime; // ideal cross fade out time (0.2 default)

		int localentrynode; // transition node at entry
		int localexitnode; // transition node at exit
		int nodeflags; // transition rules

		float entryphase; // used to match entry gait
		float exitphase; // used to match exit gait

		float lastframe; // frame that should generation EndOfSequence

		int nextseq; // auto advancing sequences
		int pose; // index of delta animation between end and nextseq

		int numikrules;

		int numautolayers;
		int autolayerindex;

		int weightlistindex;

		int posekeyindex;

		int numiklocks;
		int iklockindex;

		// Key values
		int keyvalueindex;
		int keyvaluesize;

		int cycleposeindex; // index of pose parameter to use as cycle index

		int activitymodifierindex;
		int numactivitymodifiers;

		int ikResetMask;	// new in v52
		// titan_buddy_mp_core.mdl
		// reset all ikrules with this type???
		int unk1;	// count?
		// mayhaps this is the ik type applied to the mask if what's above it true

		int unkOffset;
		int unkCount;
	};

	struct studiohdr_v8_t
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

		inline const mstudiotexture_v8_t* pTextures() const
		{
			return reinterpret_cast<const mstudiotexture_v8_t*>(reinterpret_cast<const char*>(this) + textureindex);
		}

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

		int unkNodeCount; // ???
		int nodeDataOffsetsOffset; // index into an array of int sized offsets that read into the data for each node

		int meshOffset; // hard offset to the start of this models meshes

		// all flex related model vars and structs are stripped in respawn source
		int deprecated_numflexcontrollers;
		int deprecated_flexcontrollerindex;

		int deprecated_numflexrules;
		int deprecated_flexruleindex;

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
		// technically still functional though I am unsure why you'd want to use it
		int numincludemodels;
		int includemodelindex;

		int /* mutable void* */ virtualModel;

		int bonetablebynameindex;

		// if STUDIOHDR_FLAGS_CONSTANT_DIRECTIONAL_LIGHT_DOT is set,
		// this value is used to calculate directional components of lighting 
		// on static props
		char constdirectionallightdot;

		// set during load of mdl data to track *desired* lod configuration (not actual)
		// the *actual* clamped root lod is found in studiohwdata
		// this is stored here as a global store to ensure the staged loading matches the rendering
		char rootLOD;

		// set in the mdl data to specify that lod configuration should only allow first numAllowRootLODs
		// to be set as root LOD:
		//	numAllowedRootLODs = 0	means no restriction, any lod can be set as root lod.
		//	numAllowedRootLODs = N	means that lod0 - lod(N-1) can be set as root lod, but not lodN or lower.
		char numAllowedRootLODs;

		char unused;

		float fadeDistance;	// set to -1 to never fade. set above 0 if you want it to fade out, distance is in feet.
		// player/titan models seem to inherit this value from the first model loaded in menus.
		// works oddly on entities, probably only meant for static props

		float gatherSize; // what. from r5r struct. no clue what this does, seemingly for early versions of apex bsp but stripped in release apex (season 0)
		// bad name, frustum culling

		int deprecated_numflexcontrollerui;
		int deprecated_flexcontrolleruiindex;

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

		// depreciated as they are removed in 12.1
		int deprecated_m_nBoneFlexDriverCount;
		int deprecated_m_nBoneFlexDriverIndex;

		int deprecated_m_nPerTriAABBIndex;
		int deprecated_m_nPerTriAABBNodeCount;
		int deprecated_m_nPerTriAABBLeafCount;
		int deprecated_m_nPerTriAABBVertCount;

		// always "" or "Titan"
		int unkStringOffset;

		// this is now used for combined files in rpak, vtx, vvd, and vvc are all combined while vphy is separate.
		// the indexes are added to the offset in the rpak mdl_ header.
		// vphy isn't vphy, looks like a heavily modified vphy.
		// as of s2/3 these are no unused except phy
		int vtxOffset; // VTX
		int vvdOffset; // VVD / IDSV
		int vvcOffset; // VVC / IDCV 
		int phyOffset; // VPHY / IVPS

		int vtxSize;
		int vvdSize;
		int vvcSize;
		int phySize; // still used in models using vg

		// unused in apex, gets cut in 12.1
		int deprecated_unkOffset; // deprecated_imposterIndex
		int deprecated_unkCount; // deprecated_numImposters

		// mostly seen on '_animated' suffixed models
		// manually declared bone followers are no longer stored in kvs under 'bone_followers', they are now stored in an array of ints with the bone index.
		int boneFollowerCount;
		int boneFollowerOffset;

		// BVH4 size (?)
		Vector bvhMin;
		Vector bvhMax; // seem to be the same as hull size

		Vector unk3_v54;

		int bvhOffset; // bvh4 tree

		short bvhUnk[2]; // collision detail for bvh (?)

		// new in apex vertex weight file for verts that have more than three weights
		// vvw is a 'fake' extension name, we do not know the proper name.
		int vvwOffset; // index will come last after other vertex files
		int vvwSize;
	};
}
