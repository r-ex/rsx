#pragma once

namespace r1
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

		// compression scale
		Vector posscale; // scale muliplier for bone position in animations. depreciated in v53, as the posscale is stored in anim bone headers
		Vector rotscale; // scale muliplier for bone rotation in animations

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

		Vector scale; // base bone scale
		Vector scalescale; // scale muliplier for bone scale in animations

		int unused; // remove as appropriate
	};
	static_assert(sizeof(mstudiobone_t) == 0xD8);

	// this struct is the same in r1 and r2
	struct mstudiolinearbone_t
	{
		int numbones;

		int flagsindex;
		inline int* pFlags(int i) const { assert(i >= 0 && i < numbones); return reinterpret_cast<int*>((char*)this + flagsindex) + i; }
		inline int flags(int i) const { return *pFlags(i); }

		int	parentindex;
		inline int* pParent(int i)
			const {
			assert(i >= 0 && i < numbones);
			return reinterpret_cast<int*>((char*)this + parentindex) + i;
		}

		int	posindex;
		inline const Vector* pPos(int i)
			const {
			assert(i >= 0 && i < numbones);
			return reinterpret_cast<Vector*>((char*)this + posindex) + i;
		}

		int quatindex;
		inline const Quaternion* pQuat(int i)
			const {
			assert(i >= 0 && i < numbones);
			return reinterpret_cast<Quaternion*>((char*)this + quatindex) + i;
		}

		int rotindex;
		inline const RadianEuler* pRot(int i)
			const {
			assert(i >= 0 && i < numbones);
			return reinterpret_cast<RadianEuler*>((char*)this + rotindex) + i;
		}

		int posetoboneindex;
		inline const matrix3x4_t* pPoseToBone(int i)
			const {
			assert(i >= 0 && i < numbones);
			return reinterpret_cast<matrix3x4_t*>((char*)this + posetoboneindex) + i;
		}

		int	posscaleindex;
		inline const Vector* pPosScale(int i) const { assert(i >= 0 && i < numbones); return reinterpret_cast<Vector*>((char*)this + posscaleindex) + i; }

		int	rotscaleindex;
		inline const Vector* pRotScale(int i) const { assert(i >= 0 && i < numbones); return reinterpret_cast<Vector*>((char*)this + rotscaleindex) + i; }

		int	qalignmentindex;
		inline const Quaternion* pQAlignment(int i)
			const {
			assert(i >= 0 && i < numbones);
			return reinterpret_cast<Quaternion*>((char*)this + qalignmentindex) + i;
		}

		int unused[6];
	};
	static_assert(sizeof(mstudiolinearbone_t) == 0x40);

	//// These work as toggles, flag enabled = raw data, flag disabled = "pointers", with rotations
	//enum RleFlags_v53_t : uint8_t
	//{
	//	STUDIO_ANIM_DELTA = 0x01, // delta animation
	//	STUDIO_ANIM_RAWPOS = 0x02, // Vector48
	//	STUDIO_ANIM_RAWROT = 0x04, // Quaternion64
	//	STUDIO_ANIM_RAWSCALE = 0x08, // Vector48, drone_frag.mdl for scale track usage
	//	STUDIO_ANIM_NOROT = 0x10  // this flag is used to check if there is 1. no static rotation and 2. no rotation track
	//	// https://github.com/ValveSoftware/source-sdk-2013/blob/master/sp/src/public/bone_setup.cpp#L393 used for this
	//	// this flag is used when the animation has no rotation data, it exists because it is not possible to check this as there are not separate flags for static/track rotation data
	//};
	//
	//struct mstudio_rle_anim_v53_t
	//{
	//	float posscale; // does what posscale is used for
	//
	//	uint8_t bone;
	//	uint8_t flags;
	//
	//	inline char* pData() const { return ((char*)this + sizeof(mstudio_rle_anim_t)); } // gets start of animation data, this should have a '+2' if aligned to 4
	//	inline mstudioanim_valueptr_t* pRotV() const { return reinterpret_cast<mstudioanim_valueptr_t*>(pData()); } // returns rot as mstudioanim_valueptr_t
	//	inline mstudioanim_valueptr_t* pPosV() const { return reinterpret_cast<mstudioanim_valueptr_t*>(pData() + 8); } // returns pos as mstudioanim_valueptr_t
	//	inline mstudioanim_valueptr_t* pScaleV() const { return reinterpret_cast<mstudioanim_valueptr_t*>(pData() + 14); } // returns scale as mstudioanim_valueptr_t
	//
	//	inline Quaternion64* pQuat64() const { return reinterpret_cast<Quaternion64*>(pData()); } // returns rot as static Quaternion64
	//	inline Vector48* pPos() const { return reinterpret_cast<Vector48*>(pData() + 8); } // returns pos as static Vector48
	//	inline Vector48* pScale() const { return reinterpret_cast<Vector48*>(pData() + 14); } // returns scale as static Vector48
	//
	//	// points to next bone in the list
	//	inline int* pNextOffset() const { return reinterpret_cast<int*>(pData() + 20); }
	//	inline mstudio_rle_anim_t* pNext() const { return pNextOffset() ? reinterpret_cast<mstudio_rle_anim_t*>((char*)this + *pNextOffset()) : nullptr; }
	//};
	//static_assert(sizeof(mstudio_rle_anim_v53_t) == 0x8);

	struct mstudiomovement_t
	{
		int					endframe;
		int					motionflags;
		float				v0;			// velocity at start of block
		float				v1;			// velocity at end of block
		float				angle;		// YAW rotation at end of this blocks movement
		Vector				vector;		// movement vector relative to this blocks initial angle
		Vector				position;	// relative to start of animation???
	};

	// new in Titanfall 1
	// translation track for origin bone, used in lots of animated scenes, requires STUDIO_FRAMEMOVEMENT
	// pos_x, pos_y, pos_z, yaw
	struct mstudioframemovement_t
	{
		float scale[4];
		short offset[4];
		inline mstudioanimvalue_t* pAnimvalue(int i) const { return (offset[i] > 0) ? reinterpret_cast<mstudioanimvalue_t*>((char*)this + offset[i]) : nullptr; }
	};
	static_assert(sizeof(mstudioframemovement_t) == 0x18);

	//struct mstudioanimdesc_v53_t
	//{
	//	int baseptr;
	//
	//	int sznameindex;
	//	inline const char* pszName() const { return ((char*)this + sznameindex); }
	//
	//	float fps; // frames per second	
	//	int flags; // looping/non-looping flags
	//
	//	int numframes;
	//
	//	// piecewise movement
	//	int nummovements;
	//	int movementindex;
	//	inline mstudiomovement_t* const pMovement(int i) const { return reinterpret_cast<mstudiomovement_t*>((char*)this + movementindex) + i; };
	//
	//	int framemovementindex; // new in v52
	//	inline const mstudioframemovement_t* pFrameMovement() const { return reinterpret_cast<mstudioframemovement_t*>((char*)this + framemovementindex); }
	//
	//	int animindex; // non-zero when anim data isn't in sections
	//	mstudio_rle_anim_v53_t* pAnim(int* piFrame) const; // returns pointer to data and new frame index
	//
	//	int numikrules;
	//	int ikruleindex; // non-zero when IK data is stored in the mdl
	//
	//	int numlocalhierarchy;
	//	int localhierarchyindex;
	//
	//	int sectionindex;
	//	int sectionframes; // number of frames used in each fast lookup section, zero if not used
	//	inline const mstudioanimsections_v8_t* pSection(int i) const { return reinterpret_cast<mstudioanimsections_v8_t*>((char*)this + sectionindex) + i; }
	//
	//	int unused[8];
	//};

	struct mstudiomodel_t;
	struct mstudiomesh_t
	{
		int material;

		int modelindex;
		const mstudiomodel_t* const pModel() const { return reinterpret_cast<mstudiomodel_t*>((char*)this + modelindex); }

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

		int unused[8]; // remove as appropriate
	};
	static_assert(sizeof(mstudiomesh_t) == 0x74);

	struct mstudiomesh_v53_t;
	struct mstudiomodel_t
	{
		char name[64];

		int type;

		float boundingradius;

		int nummeshes;
		int meshindex;

		template<typename mstudiomesh_t>
		const mstudiomesh_t* const pMesh(int i) const { return reinterpret_cast<mstudiomesh_t*>((char*)this + meshindex) + i; }

		// cache purposes
		int numvertices; // number of unique vertices/normals/texcoords
		int vertexindex; // vertex Vector
		// offset by vertexindex number of chars into vvd verts
		int tangentsindex; // tangents Vector
		// offset by tangentsindex number of chars into vvd tangents

		int numattachments;
		int attachmentindex;

		int deprecated_numeyeballs;
		int deprecated_eyeballindex;

		int pad[4];

		int colorindex; // vertex color
		// offset by colorindex number of chars into vvc vertex colors
		int uv2index; // vertex second uv map
		// offset by uv2index number of chars into vvc secondary uv map

		int unused[4];
	};
	static_assert(sizeof(mstudiomodel_t) == 0x94);

	struct mstudiotexture_t
	{
		int sznameindex;
		inline char* const pszName() const { return ((char*)this + sznameindex); }

		int unused_flags;
		int used;
		int unused1;

		// these are turned into 64 bit ints on load and only filled in memory
		int material_RESERVED;
		int clientmaterial_RESERVED;

		int unused[10];
	};
	static_assert(sizeof(mstudiotexture_t) == 0x40);

	struct studiohdr2_t
	{
		int numsrcbonetransform;
		int srcbonetransformindex;

		int	illumpositionattachmentindex;

		float flMaxEyeDeflection; // default to cos(30) if not set

		int linearboneindex;
		inline mstudiolinearbone_t* const pLinearBones() const { return linearboneindex ? reinterpret_cast<mstudiolinearbone_t*>((char*)this + linearboneindex) : nullptr; }

		int sznameindex;
		inline char* const pszName() const { return ((char*)this + sznameindex); }

		int m_nBoneFlexDriverCount;
		int m_nBoneFlexDriverIndex;

		// for static props (and maybe others)
		// Precomputed Per-Triangle AABB data
		int m_nPerTriAABBIndex;
		int m_nPerTriAABBNodeCount;
		int m_nPerTriAABBLeafCount;
		int m_nPerTriAABBVertCount;

		// always "" or "Titan"
		int unkStringOffset;
		inline char* const pszUnkString() const { return ((char*)this + unkStringOffset); }

		int reserved[39];
	};
	static_assert(sizeof(studiohdr2_t) == 0xD0);

	struct studiohdr_t
	{
		int id; // Model format ID, such as "IDST" (0x49 0x44 0x53 0x54)
		int version; // Format version number, such as 52 (0x34,0x00,0x00,0x00)
		int checksum; // This has to be the same in the phy and vtx files to load!
		char name[64]; // The internal name of the model, padding with null chars.
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

		int numlocalseq; // sequences
		int	localseqindex;

		int activitylistversion; // initialization flag - have the sequences been indexed?
		int eventsindexed;

		// raw textures
		int numtextures;
		int textureindex;
		inline mstudiotexture_t* const pTexture(int i) const { assert(i >= 0 && i < numtextures); return reinterpret_cast<mstudiotexture_t*>((char*)this + textureindex) + i; }

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

		int deprecated_numflexdesc;
		int deprecated_flexdescindex;

		int deprecated_numflexcontrollers;
		int deprecated_flexcontrollerindex;

		int deprecated_numflexrules;
		int deprecated_flexruleindex;

		int numikchains;
		int ikchainindex;

		int deprecated_nummouths;
		int deprecated_mouthindex;

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

		// for demand loaded animation blocks
		int szanimblocknameindex;

		int numanimblocks;
		int animblockindex;

		int /* mutable void* */ animblockModel;

		int bonetablebynameindex;

		// used by tools only that don't cache, but persist mdl's peer data
		// engine uses virtualModel to back link to cache pointers
		int /* void* */ pVertexBase;
		int /* void* */ pIndexBase;

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

		int deprecated_numflexcontrollerui;
		int deprecated_flexcontrolleruiindex;

		float flVertAnimFixedPointScale;
		int surfacepropLookup;	// this index must be cached by the loader, not saved in the file

		// NOTE: No room to add stuff? Up the .mdl file format version 
		// [and move all fields in studiohdr2_t into studiohdr_t and kill studiohdr2_t],
		// or add your stuff to studiohdr2_t. See NumSrcBoneTransforms/SrcBoneTransform for the pattern to use.
		int studiohdr2index;
		inline studiohdr2_t* const pStudioHdr2() const { return reinterpret_cast<studiohdr2_t*>((char*)this + studiohdr2index); }

		// stored maya files from used dmx files, animation files are not added. for internal tools likely
		// in r1 this is a mixed bag, some are null with no data, some have a four byte section, and some actually have the files stored.
		int sourceFilenameOffset;
		inline char* const pszSourceFiles() const { return ((char*)this + sourceFilenameOffset); }

		inline mstudiolinearbone_t* const pLinearBones() const { return studiohdr2index ? pStudioHdr2()->pLinearBones() : nullptr; }
		inline char* const pszName() const { return studiohdr2index ? pStudioHdr2()->pszName() : nullptr; }

		inline char* const pszUnkString() const { return studiohdr2index ? pStudioHdr2()->pszUnkString() : nullptr; }
	};
	static_assert(sizeof(studiohdr_t) == 0x198);
#pragma pack(pop)

	// for everything r2 and before
	void ExtractAnimValue(int frame, mstudioanimvalue_t* panimvalue, float scale, float& v1, float& v2);
	void ExtractAnimValue(int frame, mstudioanimvalue_t* panimvalue, float scale, float& v1);

	bool Studio_AnimPosition(const animdesc_t* const panim, float flCycle, Vector& vecPos, QAngle& vecAngle);
}