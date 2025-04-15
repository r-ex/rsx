#pragma once
#include <game/rtech/utils/studio/studio_r1.h>

namespace r2
{
#pragma pack(push, 4)
	struct mstudiobone_t
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

		// compression scale
		Vector posscale; // scale muliplier for bone position in animations. depreciated in v53, as the posscale is stored in anim bone headers
		Vector rotscale; // scale muliplier for bone rotation in animations
		Vector scalescale; // scale muliplier for bone scale in animations

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

		// unknown phy related section
		short unkIndex; // index into this section
		short unkCount; // number of sections for this bone?? see: models\s2s\s2s_malta_gun_animated.mdl

		int unused[7]; // remove as appropriate
	};
	static_assert(sizeof(mstudiobone_t) == 0xF4);

	// These work as toggles, flag enabled = raw data, flag disabled = "pointers", with rotations
	enum RleFlags_t : uint8_t
	{
		STUDIO_ANIM_DELTA = 0x01, // delta animation
		STUDIO_ANIM_RAWPOS = 0x02, // Vector48
		STUDIO_ANIM_RAWROT = 0x04, // Quaternion64
		STUDIO_ANIM_RAWSCALE = 0x08, // Vector48, drone_frag.mdl for scale track usage
		STUDIO_ANIM_NOROT = 0x10  // this flag is used to check if there is 1. no static rotation and 2. no rotation track
		// https://github.com/ValveSoftware/source-sdk-2013/blob/master/sp/src/public/bone_setup.cpp#L393 used for this
		// this flag is used when the animation has no rotation data, it exists because it is not possible to check this as there are not separate flags for static/track rotation data
	};

	struct mstudio_rle_anim_t
	{
		float posscale; // does what posscale is used for

		uint8_t bone;
		uint8_t flags;

		inline char* pData() const { return ((char*)this + sizeof(mstudio_rle_anim_t)); } // gets start of animation data, this should have a '+2' if aligned to 4
		inline mstudioanim_valueptr_t* pRotV() const { return reinterpret_cast<mstudioanim_valueptr_t*>(pData()); } // returns rot as mstudioanim_valueptr_t
		inline mstudioanim_valueptr_t* pPosV() const { return reinterpret_cast<mstudioanim_valueptr_t*>(pData() + 8); } // returns pos as mstudioanim_valueptr_t
		inline mstudioanim_valueptr_t* pScaleV() const { return reinterpret_cast<mstudioanim_valueptr_t*>(pData() + 14); } // returns scale as mstudioanim_valueptr_t

		inline Quaternion64* pQuat64() const { return reinterpret_cast<Quaternion64*>(pData()); } // returns rot as static Quaternion64
		inline Vector48* pPos() const { return reinterpret_cast<Vector48*>(pData() + 8); } // returns pos as static Vector48
		inline Vector48* pScale() const { return reinterpret_cast<Vector48*>(pData() + 14); } // returns scale as static Vector48

		// points to next bone in the list
		inline int* pNextOffset() const { return reinterpret_cast<int*>(pData() + 20); }
		inline mstudio_rle_anim_t* pNext() const { return pNextOffset() ? reinterpret_cast<mstudio_rle_anim_t*>((char*)this + *pNextOffset()) : nullptr; }
	};
	static_assert(sizeof(mstudio_rle_anim_t) == 0x8);

	struct mstudioanimsections_t
	{
		int animindex;
	};

	struct mstudioanimdesc_t
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
		inline r1::mstudiomovement_t* const pMovement(int i) const { return reinterpret_cast<r1::mstudiomovement_t*>((char*)this + movementindex) + i; };

		int framemovementindex; // new in v52
		inline r1::mstudioframemovement_t* pFrameMovement() const { return reinterpret_cast<r1::mstudioframemovement_t*>((char*)this + framemovementindex); }

		int animindex; // non-zero when anim data isn't in sections

		int numikrules;
		int ikruleindex; // non-zero when IK data is stored in the mdl

		int numlocalhierarchy;
		int localhierarchyindex;

		int sectionindex;
		int sectionframes; // number of frames used in each fast lookup section, zero if not used
		inline const mstudioanimsections_t* pSection(int i) const { return reinterpret_cast<mstudioanimsections_t*>((char*)this + sectionindex) + i; }

		int unused[8];
	};

	struct mstudioseqdesc_t
	{
		int baseptr;

		int	szlabelindex;
		inline const char* pszLabel() const { return ((char*)this + szlabelindex); }

		int szactivitynameindex;
		inline const char* pszActivityName() const { return ((char*)this + szactivitynameindex); }

		int flags; // looping/non-looping flags

		int activity; // initialized at loadtime to game DLL values
		int actweight;

		int numevents;
		int eventindex;

		Vector bbmin; // per sequence bounding box
		Vector bbmax;

		int numblends;

		// Index into array of shorts which is groupsize[0] x groupsize[1] in length
		int animindexindex;
		inline const short* const pAnimIndex(const int i) const { return reinterpret_cast<short*>((char*)this + animindexindex) + i; }
		inline const short GetAnimIndex(const int i) const { return *pAnimIndex(i); }
		const int AnimCount() const { return  groupsize[0] * groupsize[1]; }

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

		int ikResetMask;
		int unk_C4;

		int unused[8];
	};

	struct mstudiomesh_t
	{
		int material;

		int modelindex;
		const r1::mstudiomodel_t* const pModel() const { return reinterpret_cast<r1::mstudiomodel_t*>((char*)this + modelindex); }

		int numvertices;	// number of unique vertices/normals/texcoords
		int vertexoffset;	// vertex mstudiovertex_t
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

		int unused[6];
	};
	static_assert(sizeof(mstudiomesh_t) == 0x74);

	struct mstudiotexture_t
	{
		int sznameindex;
		inline char* const pszName() const { return ((char*)this + sznameindex); }

		int unused_flags;
		int used;

		int unused[8];
	};
	static_assert(sizeof(mstudiotexture_t) == 0x2C);

	struct studiohdr_t
	{
		int id; // Model format ID, such as "IDST" (0x49 0x44 0x53 0x54)
		int version; // Format version number, such as 53 (0x35,0x00,0x00,0x00)
		int checksum; // This has to be the same in the phy and vtx files to load!
		int sznameindex; // This has been moved from studiohdr2_t to the front of the main header.
		inline char* const pszName() const { return ((char*)this + sznameindex); }
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

		// highest observed: 250
		// max is definitely 256 because 8bit uint limit
		int numbones; // bones
		int boneindex;
		inline mstudiobone_t* const pBone(int i) const { assert(i >= 0 && i < numbones); return reinterpret_cast<mstudiobone_t*>((char*)this + boneindex) + i; }

		int numbonecontrollers; // bone controllers
		int bonecontrollerindex;

		int numhitboxsets;
		int hitboxsetindex;

		int numlocalanim; // animations/poses
		int localanimindex; // animation descriptions
		inline mstudioanimdesc_t* const pAnimdesc(const int i) const { assert(i >= 0 && i < numlocalanim); return reinterpret_cast<mstudioanimdesc_t*>((char*)this + localanimindex) + i; }

		int numlocalseq; // sequences
		int	localseqindex;
		inline mstudioseqdesc_t* const pSeqdesc(const int i) const { assert(i >= 0 && i < numlocalseq); return reinterpret_cast<mstudioseqdesc_t*>((char*)this + localseqindex) + i; }

		int activitylistversion; // initialization flag - have the sequences been indexed? set on load
		int eventsindexed;

		// mstudiotexture_t
		// short rpak path
		// raw textures
		int numtextures; // the material limit exceeds 128, probably 256.
		int textureindex;
		inline mstudiotexture_t* const pTexture(int i) const { assert(i >= 0 && i < numtextures); return reinterpret_cast<mstudiotexture_t*>((char*)this + textureindex) + i; }

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
		inline mstudiobodyparts_t* const pBodypart(int i) const { assert(i >= 0 && i < numbodyparts); return reinterpret_cast<mstudiobodyparts_t*>((char*)this + bodypartindex) + i; }

		int numlocalattachments;
		int localattachmentindex;

		int numlocalnodes;
		int localnodeindex;
		int localnodenameindex;
		int localNodeUnk; // something about having nodes while include models also have nodes??? used only three times in r2, never used in apex, removed in rmdl v16. super_spectre_v1, titan_buddy, titan_buddy_skyway

		int deprecated_flexdescindex;

		int deprecated_numflexcontrollers;
		int deprecated_flexcontrollerindex;

		int deprecated_numflexrules;
		int deprecated_flexruleindex;

		int numikchains;
		int ikchainindex;

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

		// external animations, models, etc.
		int numincludemodels;
		int includemodelindex;

		// implementation specific back pointer to virtual data
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

		float fadeDistance; // set to -1 to never fade. set above 0 if you want it to fade out, distance is in feet.
		// player/titan models seem to inherit this value from the first model loaded in menus.
		// works oddly on entities, probably only meant for static props

		int deprecated_numflexcontrollerui;
		int deprecated_flexcontrolleruiindex;

		float flVertAnimFixedPointScale;
		int surfacepropLookup; // this index must be cached by the loader, not saved in the file

		// stored maya files from used dmx files, animation files are not added. for internal tools likely
		// in r1 this is a mixed bag, some are null with no data, some have a four byte section, and some actually have the files stored.
		int sourceFilenameOffset;
		inline char* const pszSourceFiles() const { return ((char*)this + sourceFilenameOffset); }

		int numsrcbonetransform;
		int srcbonetransformindex;

		int	illumpositionattachmentindex;

		int linearboneindex;
		inline r1::mstudiolinearbone_t* const pLinearBones() const { return linearboneindex ? reinterpret_cast<r1::mstudiolinearbone_t*>((char*)this + linearboneindex) : nullptr; }

		int m_nBoneFlexDriverCount;
		int m_nBoneFlexDriverIndex;

		// for static props (and maybe others)
		// Precomputed Per-Triangle AABB data
		int m_nPerTriAABBIndex;
		int m_nPerTriAABBNodeCount;
		int m_nPerTriAABBLeafCount;
		int m_nPerTriAABBVertCount;

		// always "" or "Titan", this is probably for internal tools
		int unkStringOffset;
		inline char* const pszUnkString() const { return ((char*)this + unkStringOffset); }

		// ANIs are no longer used and this is reflected in many structs
		// start of interal file data
		int vtxOffset; // VTX
		int vvdOffset; // VVD / IDSV
		int vvcOffset; // VVC / IDCV 
		int phyOffset; // VPHY / IVPS

		int vtxSize; // VTX
		int vvdSize; // VVD / IDSV
		int vvcSize; // VVC / IDCV 
		int phySize; // VPHY / IVPS

		inline OptimizedModel::FileHeader_t* const pVTX() const { return vtxSize > 0 ? reinterpret_cast<OptimizedModel::FileHeader_t*>((char*)this + vtxOffset) : nullptr; }
		inline vvd::vertexFileHeader_t* const pVVD() const { return vvdSize > 0 ? reinterpret_cast<vvd::vertexFileHeader_t*>((char*)this + vvdOffset) : nullptr; }
		inline vvc::vertexColorFileHeader_t* const pVVC() const { return vvcSize > 0 ? reinterpret_cast<vvc::vertexColorFileHeader_t*>((char*)this + vvcOffset) : nullptr; }
		inline ivps::phyheader_t* const pPHY() const { return phySize > 0 ? reinterpret_cast<ivps::phyheader_t*>((char*)this + phyOffset) : nullptr; }

		// this data block is related to the vphy, if it's not present the data will not be written
		// definitely related to phy, apex phy has this merged into it
		int unkOffset; // section between vphy and vtx.?
		int unkCount; // only seems to be used when phy has one solid

		// mostly seen on '_animated' suffixed models
		// manually declared bone followers are no longer stored in kvs under 'bone_followers', they are now stored in an array of ints with the bone index.
		int boneFollowerCount;
		int boneFollowerOffset; // index only written when numbones > 1, means whatever func writes this likely checks this (would make sense because bonefollowers need more than one bone to even be useful). maybe only written if phy exists

		int unused1[60];

	};
	static_assert(sizeof(studiohdr_t) == 0x2CC);
#pragma pack(pop)

	void CalcBoneQuaternion(int frame, float s,
		const mstudiobone_t* pBone,
		const r1::mstudiolinearbone_t* pLinearBones,
		const mstudio_rle_anim_t* panim, Quaternion& q);

	void CalcBonePosition(int frame, float s,
		const mstudiobone_t* pBone,
		const r1::mstudiolinearbone_t* pLinearBones,
		const mstudio_rle_anim_t* panim, Vector& pos);

	void CalcBoneScale(int frame, float s,
		const Vector& baseScale, const Vector& baseScaleScale,
		const mstudio_rle_anim_t* panim, Vector& scale);
}