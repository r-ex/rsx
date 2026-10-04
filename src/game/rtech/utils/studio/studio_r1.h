#pragma once

#pragma pack(push, 4)
namespace r1
{
	struct studiohdr_t;

	//
	// Model Per Triangle Collision (for world models)
	//

	constexpr float perTriAABBScale = 0.000015259022f;

	// some notes: models do not need the static prop flag to have/use this data, and models with this data can have more than one bone.
	// gibs also use this. static props require this data, or at the bare minimum the header to be filled out (if no collision is desired)
	struct mstudiopertrivert_t
	{
		// to get float value:
		// axisValue * ((float)(bbmax.axis - bbmin.axis) * 0.000015259022)
		// where axis is x, y, or z. bbmax and bbmin are from the pertri header.
		uint16_t x, y, z;
	};
	static_assert(sizeof(mstudiopertrivert_t) == 0x6);

	struct mstudiopertritri_v1_t
	{
		uint16_t vertIndices[3];
	};

	struct mstudiopertritri_t
	{
		uint16_t vertIndices[3];
		uint16_t unk_6; // very odd, this value seems to match all tris on all leaves in file. varies per file. NOTE: this gets read and passed to function arg!
	};

	constexpr int MAX_PER_TRI_LEAF_TRIANGLES = 12;
	struct mstudiopertrileaf_v1_t
	{
		// bounds, calculated the same as vert pos
		uint16_t bbmin[3];
		uint16_t bbmax[3];

		int vertexIndex; // base index for triangle vertex indices

		mstudiopertritri_v1_t triangles[MAX_PER_TRI_LEAF_TRIANGLES];
	};

	struct mstudiopertrileaf_t
	{
		// bounds, calculated the same as vert pos
		uint16_t bbmin[3];
		uint16_t bbmax[3];

		int vertexIndex; // base index for triangle vertex indices

		mstudiopertritri_t triangles[MAX_PER_TRI_LEAF_TRIANGLES];
	};

	struct mstudiopertrinode_t
	{
		// bounds, calculated the same as vert pos
		uint16_t bbmin[3];
		uint16_t bbmax[3];

		// node / leaf indices, zero or greater is node, neg one or less is leaf
		int childIndex[2];

		inline const bool isLeaf(const int chldidx) const { return childIndex[chldidx] < 0; }
		inline const int ChildIndex(const int chldidx) const { return isLeaf(chldidx) ? ~childIndex[chldidx] : childIndex[chldidx]; }
		//inline const mstudiopertrileaf_t* pLeaf(const int chldidx) const { return isLeaf(chldidx) ? reinterpret_cast<mstudiopertrileaf_t*>((char*)this + ~childIndex[chldidx]) : nullptr; }
		//inline const mstudiopertrinode_t* pNode(const int chldidx) const { return !isLeaf(chldidx) ? reinterpret_cast<mstudiopertrinode_t*>((char*)this + childIndex[chldidx]) : nullptr; }
	};

	struct mstudiopertrihdr_t
	{
		// 0: unsupported or no pertri data, if set on static props it will cause issues
		// 1: supported in r1 (Titanfall) but not r2 (Titanfall 2), have not seen an example of this yet. know difference between v2 is 9 tris per leaf instead of 12
		// 2: supported by both r1 and r2, and is the expected version for models requiring per tri data
		int version;

		Vector bbmin;
		Vector bbmax;

		int unused[8];
	};


	//
	// Model Bones
	//

	// 'STUDIO_PROC_JIGGLE' the only one observed in respawn games
	#define STUDIO_PROC_AXISINTERP	1
	#define STUDIO_PROC_QUATINTERP	2
	#define STUDIO_PROC_AIMATBONE	3
	#define STUDIO_PROC_AIMATATTACH 4
	#define STUDIO_PROC_JIGGLE		5
	#define STUDIO_PROC_TWIST_MASTER	6
	#define STUDIO_PROC_TWIST_SLAVE		7 // Multiple twist bones are computed at once for the same parent/child combo so TWIST_NULL do nothing

// all except jiggle yet to be seen
	struct mstudioaxisinterpbone_t
	{
		int				control;// local transformation of this bone used to calc 3 point blend
		int				axis;	// axis to check
		Vector			pos[6];	// X+, X-, Y+, Y-, Z+, Z-
		Quaternion		quat[6];// X+, X-, Y+, Y-, Z+, Z-
	};

	struct mstudioquatinterpbone_t
	{
		int				control;// local transformation to check
		int				numtriggers;
		int				triggerindex;
	};

	struct mstudioaimatbone_t
	{
		int				parent;
		int				aim;		// Might be bone or attach
		Vector			aimvector;
		Vector			upvector;
		Vector			basepos;
	};

	struct mstudiotwistbonetarget_t
	{
		int				m_nBone;
		float			m_flWeight;
		Vector			m_vBaseTranslate;
		Quaternion		m_qBaseRotation;
	};

	struct mstudiotwistbone_t
	{
		bool			m_bInverse;				// False: Apply child rotation to twist targets True: Apply parent rotation to twist targets
		Vector			m_vUpVector;			// In parent space, projected into plane defined by vector between parent & child
		int				m_nParentBone;
		Quaternion		m_qBaseInv;	// The base rotation of the parent, used if m_bInverse is true
		int				m_nChildBone;

		int				m_nTargetCount;
		int				m_nTargetIndex;
	};

	struct mstudioquatinterpinfo_t
	{
		float			inv_tolerance;	// 1 / radian angle of trigger influence
		Quaternion		trigger;	// angle to match
		Vector			pos;		// new position
		Quaternion		quat;		// new angle
	};

	#define JIGGLE_IS_FLEXIBLE				0x01
	#define JIGGLE_IS_RIGID					0x02
	#define JIGGLE_HAS_YAW_CONSTRAINT		0x04
	#define JIGGLE_HAS_PITCH_CONSTRAINT		0x08
	#define JIGGLE_HAS_ANGLE_CONSTRAINT		0x10
	#define JIGGLE_HAS_LENGTH_CONSTRAINT	0x20
	#define JIGGLE_HAS_BASE_SPRING			0x40

	// this struct is the same in r1 and r2, and unchanged from p2
	struct mstudiojigglebone_t
	{
		int flags;

		// general params
		float length; // how far from bone base, along bone, is tip
		float tipMass;

		// flexible params
		float yawStiffness;
		float yawDamping;
		float pitchStiffness;
		float pitchDamping;
		float alongStiffness;
		float alongDamping;

		// angle constraint
		float angleLimit; // maximum deflection of tip in radians

		// yaw constraint
		float minYaw; // in radians
		float maxYaw; // in radians
		float yawFriction;
		float yawBounce;

		// pitch constraint
		float minPitch; // in radians
		float maxPitch; // in radians
		float pitchFriction;
		float pitchBounce;

		// base spring
		float baseMass;
		float baseStiffness;
		float baseDamping;
		float baseMinLeft;
		float baseMaxLeft;
		float baseLeftFriction;
		float baseMinUp;
		float baseMaxUp;
		float baseUpFriction;
		float baseMinForward;
		float baseMaxForward;
		float baseForwardFriction;
	};

	#define BONE_CALCULATE_MASK			0x1F	// 0x1F -> 0x2F (cannot be confirmed, unused)
	#define BONE_PHYSICALLY_SIMULATED	0x01	// bone is physically simulated when physics are active
	#define BONE_PHYSICS_PROCEDURAL		0x02	// procedural when physics is active
	#define BONE_ALWAYS_PROCEDURAL		0x04	// bone is always procedurally animated
	#define BONE_SCREEN_ALIGN_SPHERE	0x08	// bone aligns to the screen, not constrained in motion.
	#define BONE_SCREEN_ALIGN_CYLINDER	0x10	// bone aligns to the screen, constrained by it's own axis.
	#define BONE_IKCHAIN_INFLUENCE		0x20	// bone is influenced by IK chains, added in V52 (Titanfall 1)

	#define BONE_USED_MASK				0x0007FF00
	#define BONE_USED_BY_ANYTHING		0x0007FF00
	#define BONE_USED_BY_HITBOX			0x00000100	// bone (or child) is used by a hit box
	#define BONE_USED_BY_ATTACHMENT		0x00000200	// bone (or child) is used by an attachment point
	#define BONE_USED_BY_VERTEX_MASK	0x0003FC00
	#define BONE_USED_BY_VERTEX_LOD0	0x00000400	// bone (or child) is used by the toplevel model via skinned vertex
	#define BONE_USED_BY_VERTEX_LOD1	0x00000800	
	#define BONE_USED_BY_VERTEX_LOD2	0x00001000  
	#define BONE_USED_BY_VERTEX_LOD3	0x00002000
	#define BONE_USED_BY_VERTEX_LOD4	0x00004000
	#define BONE_USED_BY_VERTEX_LOD5	0x00008000
	#define BONE_USED_BY_VERTEX_LOD6	0x00010000
	#define BONE_USED_BY_VERTEX_LOD7	0x00020000
	#define BONE_USED_BY_BONE_MERGE		0x00040000	// bone is available for bone merge to occur against it
	#define BONE_FLAG_UNK_80000			0x00080000	// where? BONE_USED_BY_SOMETHING (what?)

	#define BONE_USED_BY_VERTEX_AT_LOD(lod) ( BONE_USED_BY_VERTEX_LOD0 << (lod) )
	#define BONE_USED_BY_ANYTHING_AT_LOD(lod) ( ( BONE_USED_BY_ANYTHING & ~BONE_USED_BY_VERTEX_MASK ) | BONE_USED_BY_VERTEX_AT_LOD(lod) )

	#define BONE_TYPE_MASK				0x00F00000
	#define BONE_FIXED_ALIGNMENT		0x00100000	// bone can't spin 360 degrees, all interpolation is normalized around a fixed orientation

	#define BONE_HAS_SAVEFRAME_POS		0x00200000	// Vector48
	#define BONE_HAS_SAVEFRAME_ROT64	0x00400000	// Quaternion64
	#define BONE_HAS_SAVEFRAME_ROT32	0x00800000	// Quaternion3
	#define BONE_FLAG_UNK_1000000		0x01000000	// where?

	struct mstudiobone_t
	{
		int sznameindex;
		inline const char* const pszName() const { return reinterpret_cast<const char* const>(this) + sznameindex; }

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
		inline const void* const pProcedure() const { return procindex ? reinterpret_cast<const char* const>(this) + procindex : nullptr; }

		int physicsbone;	// index into physically simulated bone
		// from what I can tell this is the section that is parented to this bone, and if this bone is not the parent of any sections, it goes up the bone chain to the nearest bone that does and uses that section index
		int surfacepropidx; // index into string tablefor property name
		inline const char* const pszSurfaceProp() const { return reinterpret_cast<const char* const>(this) + surfacepropidx; }

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
		inline const int* const pParent(int i) const
		{
			assert(i >= 0 && i < numbones);
			return reinterpret_cast<const int* const>((char*)this + parentindex) + i;
		}

		int	posindex;
		inline const Vector* const pPos(int i) const
		{
			assert(i >= 0 && i < numbones);
			return reinterpret_cast<const Vector* const>((char*)this + posindex) + i;
		}

		int quatindex;
		inline const Quaternion* const pQuat(int i) const
		{
			assert(i >= 0 && i < numbones);
			return reinterpret_cast<const Quaternion* const>((char*)this + quatindex) + i;
		}

		int rotindex;
		inline const RadianEuler* const pRot(int i) const
		{
			assert(i >= 0 && i < numbones);
			return reinterpret_cast<const RadianEuler* const>((char*)this + rotindex) + i;
		}

		int posetoboneindex;
		inline const matrix3x4_t* const pPoseToBone(int i) const
		{
			assert(i >= 0 && i < numbones);
			return reinterpret_cast<const matrix3x4_t* const>((char*)this + posetoboneindex) + i;
		}

		int	posscaleindex;
		inline const Vector* const pPosScale(int i) const { assert(i >= 0 && i < numbones); return reinterpret_cast<const Vector* const>((char*)this + posscaleindex) + i; }

		int	rotscaleindex;
		inline const Vector* const pRotScale(int i) const { assert(i >= 0 && i < numbones); return reinterpret_cast<const Vector* const>((char*)this + rotscaleindex) + i; }

		int	qalignmentindex;
		inline const Quaternion* const pQAlignment(int i) const { assert(i >= 0 && i < numbones); return reinterpret_cast<const Quaternion* const>((char*)this + qalignmentindex) + i; }

		int unused[6];
	};
	static_assert(sizeof(mstudiolinearbone_t) == 0x40);

	// this struct is the same in r1 and r2, and unchanged from p2
	struct mstudiosrcbonetransform_t
	{
		int			sznameindex;
		inline const char* const pszName() const { return reinterpret_cast<const char* const>(this) + sznameindex; }

		matrix3x4_t	pretransform;
		matrix3x4_t	posttransform;
	};

	// I have never seen the following structs used in Titanfall 1 but the game definitely has them as seen in datacache.dll
	// these cannot be confirmed for Titanfall 2 and as such will not be listed there
	struct mstudioboneflexdrivercontrol_t
	{
		int m_nBoneComponent;		// Bone component that drives flex, StudioBoneFlexComponent_t
		int m_nFlexControllerIndex;	// Flex controller to drive
		float m_flMin;				// Min value of bone component mapped to 0 on flex controller
		float m_flMax;				// Max value of bone component mapped to 1 on flex controller
	};

	struct mstudioboneflexdriver_t
	{
		int m_nBoneIndex;			// Bone to drive flex controller
		int m_nControlCount;		// Number of flex controllers being driven
		int m_nControlIndex;		// Index into data where controllers are (relative to this)

		int unused[3];
	};

	struct mstudiobonecontroller_t
	{
		int					bone;	// -1 == 0
		int					type;	// X, Y, Z, XR, YR, ZR, M
		float				start;
		float				end;
		int					rest;	// char index value at rest
		int					inputfield;	// 0-3 user set controller, 4 mouth
		int					unused[8];
	};

	#define	ATTACHMENT_FLAG_WORLD_ALIGN 0x10000

	// this struct is the same in r1 and r2, and unchanged from p2
	struct mstudioattachment_t
	{
		int					sznameindex;
		inline const char* const pszName() const { return reinterpret_cast<const char* const>(this) + sznameindex; }
		unsigned int		flags;
		int					localbone;
		matrix3x4_t			local; // attachment point
		int					unused[8];
	};

	// unchanged from p2
	struct mstudiobbox_t
	{
		int bone;
		int group;				// intersection group
		Vector bbmin;			// bounding box
		Vector bbmax;
		int szhitboxnameindex;	// offset to the name of the hitbox.
		inline const char* const pszHitboxName() const
		{
			if (szhitboxnameindex == 0)
				return "";

			return reinterpret_cast<const char* const>(this) + szhitboxnameindex;
		}

		int unused[8];
	};

	// this struct is the same in r1 and r2, and unchanged from p2
	struct mstudiohitboxset_t
	{
		int sznameindex;
		const char* const pszName() const { return reinterpret_cast<const char* const>(this) + sznameindex; }

		int numhitboxes;
		int hitboxindex;
		const mstudiobbox_t* const pHitbox(const int i) const { return reinterpret_cast<const mstudiobbox_t* const>((char*)this + hitboxindex) + i; }
	};


	//
	// Model IK Info
	//

	struct mstudioiklock_t
	{
		int chain;
		float flPosWeight;
		float flLocalQWeight;
		int flags;

		int unused[4];
	};

	struct mstudioiklink_t
	{
		int		bone;
		Vector	kneeDir;	// ideal bending direction (per link, if applicable)
		Vector	unused0;	// unused
	};

	struct mstudioikchain_t
	{
		int sznameindex;
		inline const char* const pszName() const { return reinterpret_cast<const char* const>(this) + sznameindex; }

		int linktype;
		int numlinks;
		int linkindex;
		const mstudioiklink_t* const pLink(int i) const { return reinterpret_cast<const mstudioiklink_t* const>((char*)this + linkindex) + i; }
	};

	// these should not be present in respawn games, as it was deprecated during hl2's development, despite this support for them remains even up to modern r5.
	struct mstudioikerror_t
	{
		Vector pos;
		Quaternion q;
	};

	struct mstudiocompressedikerror_t
	{
		float scale[6];
		short offset[6];
	};

	#define IK_SELF 1
	#define IK_WORLD 2
	#define IK_GROUND 3
	#define IK_RELEASE 4
	#define IK_ATTACHMENT 5
	#define IK_UNLATCH 6

	struct mstudioikrule_t
	{
		int index;

		int type;
		int chain;

		int	bone;

		int slot;	// iktarget slot.  Usually same as chain.
		float height;
		float radius;
		float floor;
		Vector pos;
		Quaternion q;

		int compressedikerrorindex;

		int unused2;

		int iStart;
		int ikerrorindex;

		float start;	// beginning of influence
		float peak;	// start of full influence
		float tail;	// end of full influence
		float end;	// end of all influence

		float unused3;	// 
		float contact;	// frame footstep makes ground concact
		float drop;		// how far down the foot should drop when reaching for IK
		float top;		// top of the foot box

		int unused6;
		int unused7;
		int unused8;

		int szattachmentindex; // name of world attachment
		inline const char* const pszAttachment() const { return szattachmentindex ? reinterpret_cast<const char* const>(this) + szattachmentindex : nullptr; }

		float endHeight; // new in v52

		int unused[6];
	};


	//
	// Model Animation
	// 

	// [rika]: issues with valid/total being negative? make these unsigned, they can exceed the 8bit integer limit (in r2).
	/*union mstudioanimvalue_t
	{
		struct
		{
			uint8_t	valid;
			uint8_t	total;
		} num;
		short value;
	};
	static_assert(sizeof(mstudioanimvalue_t) == 0x2);*/

	struct mstudioanim_valueptr_t
	{
		short	offset[3];
		inline mstudioanimvalue_t* pAnimvalue(int i) const { return (offset[i] > 0) ? reinterpret_cast<mstudioanimvalue_t*>((char*)this + offset[i]) : nullptr; }
	};
	static_assert(sizeof(mstudioanim_valueptr_t) == 0x6);

	enum RleFlags_t : uint8_t
	{
		STUDIO_ANIM_RAWPOS = 0x01, // Vector48
		STUDIO_ANIM_RAWROT = 0x02, // Quaternion48
		STUDIO_ANIM_ANIMPOS = 0x04, // mstudioanim_valueptr_t
		STUDIO_ANIM_ANIMROT = 0x08, // mstudioanim_valueptr_t
		STUDIO_ANIM_DELTA = 0x10,
		STUDIO_ANIM_RAWROT2 = 0x20, // Quaternion64
		STUDIO_ANIM_RAWSCALE = 0x40, // Vector48
		STUDIO_ANIM_ANIMSCALE = 0x80  // mstudioanim_valueptr_t 
		// gravestone_01_animated.mdl and leviathan models use scale in RLE
	};

	struct mstudio_rle_anim_t
	{
		uint8_t bone;
		uint8_t flags;		// weighing options

		__forceinline const int RotSize() const
		{
			const int sizeRaw = ((flags & STUDIO_ANIM_RAWROT) >> 1) * 6;
			const int sizeRaw2 = (flags & STUDIO_ANIM_RAWROT2) >> 2;
			const int sizeAnim = ((flags & STUDIO_ANIM_ANIMROT) >> 3) * 6;

			return sizeRaw + sizeRaw2 + sizeAnim;
		}

		__forceinline const int PosSize() const
		{
			const int sizeRaw = (flags & STUDIO_ANIM_RAWPOS) * 6;
			const int sizeAnim = ((flags & STUDIO_ANIM_ANIMPOS) >> 2) * 6;

			return sizeRaw + sizeAnim;
		}

		inline char* pData() const { return ((char*)this + sizeof(mstudio_rle_anim_t)); } // gets start of animation data, this should have a '+2' if aligned to 4
		inline mstudioanim_valueptr_t* pRotV() const { return reinterpret_cast<mstudioanim_valueptr_t*>(pData()); } // returns rot as mstudioanim_valueptr_t
		inline mstudioanim_valueptr_t* pPosV() const { return reinterpret_cast<mstudioanim_valueptr_t*>(pData() + RotSize()); } // returns pos as mstudioanim_valueptr_t
		inline mstudioanim_valueptr_t* pScaleV() const { return reinterpret_cast<mstudioanim_valueptr_t*>(pData() + RotSize() + PosSize()); } // returns scale as mstudioanim_valueptr_t

		inline Quaternion48* pQuat48() const { return reinterpret_cast<Quaternion48*>(pData()); } // returns rot as static Quaternion64
		inline Quaternion64* pQuat64() const { return reinterpret_cast<Quaternion64*>(pData()); } // returns rot as static Quaternion64
		inline Vector48* pPos() const { return reinterpret_cast<Vector48*>(pData() + RotSize()); } // returns pos as static Vector48
		inline Vector48* pScale() const { return reinterpret_cast<Vector48*>(pData() + RotSize() + PosSize()); } // returns scale as static Vector48

		// points to next bone in the list
		short nextoffset;
		inline mstudio_rle_anim_t* pNext() const { return nextoffset ? reinterpret_cast<mstudio_rle_anim_t*>((char*)this + nextoffset) : nullptr; }
	};
	static_assert(sizeof(mstudio_rle_anim_t) == 0x4);

	// used to seek directly to a bone's data instead of incrementing each time.
	struct mstudio_frame_anim_boneoffset_t
	{
		uint16_t frameOffset; // offset from start of frame data for this bone's frame data
		uint16_t constOffset; // offset from start of frame data for this bone's constant data
	};

	enum StudioFrameFlags_t : uint8_t
	{
		STUDIO_FRAME_RAWPOS = 0x01, // Vector48 in constants
		STUDIO_FRAME_RAWROT = 0x02, // Quaternion48 in constants
		STUDIO_FRAME_RAWSCALE = 0x04, // Vector48 in constants, atlas_mp_scripted.mdl at_hotdrop_loop at_hotdrop_01
		STUDIO_FRAME_ANIMPOS = 0x08, // Quaternion48 in framedata
		STUDIO_FRAME_ANIMROT = 0x10, // Vector48 in framedata
		STUDIO_FRAME_ANIMSCALE = 0x20, // Vector48 in framedata
		STUDIO_FRAME_FULLANIM = 0x40, // use full size vectors (12 bytes) for scale and position, and Quaternion48S for rotations. 
	};

	static constexpr uint8_t s_FrameAnimBoneSizeLUT[16] = { 0, 6, 6, 12, 6, 12, 12, 18, 0, 12, 6, 18, 12, 24, 18, 30 };
	static constexpr int s_FrameAnimFixedFlags = 1337; // I shit you not, this is the actual value

	struct mstudio_frame_anim_t
	{
		inline const uint8_t* const pBoneFlags() const { return reinterpret_cast<const uint8_t* const>(this) + sizeof(mstudio_frame_anim_t); }

		int constantsoffset;
		inline const uint8_t* const pConstantData() const { return reinterpret_cast<const uint8_t* const>(this) + constantsoffset; }

		int frameoffset;
		int framelength;
		inline const uint8_t* const pFrameData(const int iFrame) const { return reinterpret_cast<const uint8_t* const>(this) + frameoffset + (iFrame * framelength); }

		int fixedOldBoneflags; // set to '1337' if bone flags have been fixed
		inline const bool HasFixedFlags() const { return fixedOldBoneflags == s_FrameAnimFixedFlags ? true : false; }

		int boneLookupTableOffset; // table of offsets into a certain bone's data within a frame, if not preset use SkipBoneFrame() to progress track
		inline const mstudio_frame_anim_boneoffset_t* const pBoneLookup(const int bone) const { return reinterpret_cast<const mstudio_frame_anim_boneoffset_t* const>((char*)this + boneLookupTableOffset) + bone; }

		int unused;
	};
	static_assert(sizeof(mstudio_frame_anim_t) == 0x18);

	// never seen this used but we have it anyway
	struct mstudiolocalhierarchy_t
	{
		int iBone;			// bone being adjusted
		int iNewParent;		// the bones new parent

		float start;		// beginning of influence
		float peak;			// start of full influence
		float tail;			// end of full influence
		float end;			// end of all influence

		int iStart;			// first frame 

		int localanimindex;

		int unused[4];
	};

	#define STUDIO_X		0x00000001
	#define STUDIO_Y		0x00000002	
	#define STUDIO_Z		0x00000004
	#define STUDIO_XR		0x00000008
	#define STUDIO_YR		0x00000010
	#define STUDIO_ZR		0x00000020

	#define STUDIO_LX		0x00000040
	#define STUDIO_LY		0x00000080
	#define STUDIO_LZ		0x00000100
	#define STUDIO_LXR		0x00000200
	#define STUDIO_LYR		0x00000400
	#define STUDIO_LZR		0x00000800

	#define STUDIO_LINEAR			0x00001000
	#define STUDIO_QUADRATIC_MOTION 0x00002000 // tucked away in studiomdl.h

	#define STUDIO_TYPES	0x0003FFFF
	#define STUDIO_RLOOP	0x00040000	// controller that wraps shortest distance

	//struct mstudiomovement_t
	//{
	//	int		endframe;
	//	int		motionflags;
	//	float	v0;			// velocity at start of block
	//	float	v1;			// velocity at end of block
	//	float	angle;		// YAW rotation at end of this blocks movement
	//	Vector	vector;		// movement vector relative to this blocks initial angle
	//	Vector	position;	// relative to start of animation???
	//};

	// new in Titanfall 1
	// translation track for origin bone, used in lots of animated scenes, requires STUDIO_FRAMEMOVEMENT
	// pos_x, pos_y, pos_z, yaw
	// reSource calls this root motion (while having the struct actually called mstudioframemovement_t)
	// "Anim_EnableUseAnimatedRefAttachmentInsteadOfRootMotion"
	// "By default the REF attachment is checked only the first frame to get the initial offset. Then root motion is added onto that offset. Call this function to position the entity using REF every frame instead of using root motion."
	// as well as apex animation names having 'rm' in them (apex animations are named by command line)
	struct mstudioframemovement_t
	{
		float scale[4];
		short offset[4];
		inline mstudioanimvalue_t* pAnimvalue(int i) const { return (offset[i] > 0) ? reinterpret_cast<mstudioanimvalue_t*>((char*)this + offset[i]) : nullptr; }
	};

	struct mstudioanimblock_t
	{
		int datastart;
		int dataend;
	};

	struct mstudioanimsections_t
	{
		int animblock;
		int animindex;
	};

	// sequence and autolayer flags
	#define STUDIO_LOOPING	0x0001		// ending frame should be the same as the starting frame
	#define STUDIO_SNAP		0x0002		// do not interpolate between previous animation and this one
	#define STUDIO_DELTA	0x0004		// this sequence "adds" to the base sequences, not slerp blends
	#define STUDIO_AUTOPLAY	0x0008		// temporary flag that forces the sequence to always play
	#define STUDIO_POST		0x0010		// 
	#define STUDIO_ALLZEROS	0x0020		// this animation/sequence has no real animation data
	#define STUDIO_FRAMEANIM 0x0040		// animation is encoded as by frame x bone instead of RLE bone x frame
	#define STUDIO_CYCLEPOSE 0x0080		// cycle index is taken from a pose parameter index
	#define STUDIO_REALTIME	0x0100		// cycle index is taken from a real-time clock, not the animations cycle index
	#define STUDIO_LOCAL	0x0200		// sequence has a local context sequence
	#define STUDIO_HIDDEN	0x0400		// don't show in default selection views
	#define STUDIO_OVERRIDE	0x0800		// a forward declared sequence (empty)
	#define STUDIO_ACTIVITY	0x1000		// Has been updated at runtime to activity index
	#define STUDIO_EVENT	0x2000		// Has been updated at runtime to event index on server
	#define STUDIO_WORLD	0x4000		// sequence blends in worldspace
	#define STUDIO_NOFORCELOOP		0x8000	// do not force the animation loop
	#define STUDIO_EVENT_CLIENT		0x10000	// Has been updated at runtime to event index on client
	#define STUDIO_HAS_SCALE		0x20000 // only appears on anims with scale, used for quick lookup in ScaleBones, only reason this should be check is if there has been scale data parsed in, otherwise it is pointless (see ScaleBones checking if scale is 1.0f).
	#define STUDIO_FRAMEMOVEMENT    0x40000 // framemovements are only read if this flag is present
	#define STUDIO_SINGLEFRAME		0x80000 // this animation/sequence only has one frame of animation data
	#define STUDIO_BPANIM			0x100000 // animation uses bluepoint's custom animation data


// similar to p2 struct with some unused slots filled
	struct mstudioanimdesc_t
	{
		int baseptr;
		const studiohdr_t* const pStudiohdr() const { return reinterpret_cast<const studiohdr_t* const>((char*)this + baseptr); }

		int sznameindex;
		inline const char* const pszName() const { return reinterpret_cast<const char* const>(this) + sznameindex; }

		float fps; // frames per second	
		int flags; // looping/non-looping flags

		int numframes;

		// piecewise movement
		int	nummovements;
		int movementindex;
		inline const mstudiomovement_t* const pMovement(int i) const { return reinterpret_cast<const mstudiomovement_t* const>((char*)this + movementindex) + i; };

		int ikrulezeroframeindex;

		int framemovementindex; // new in v52
		inline const mstudioframemovement_t* const pFrameMovement() const { return reinterpret_cast<const mstudioframemovement_t* const>((char*)this + framemovementindex); }

		int unused1[4]; // remove as appropriate (and zero if loading older versions)	

		int animblock;
		int animindex; // non-zero when anim data isn't in sections
		const char* const pAnimBlock(int block, int index) const; // returns pointer to a specific anim block (local or external)
		const char* const pAnim(int* piFrame, float& flStall) const; // returns pointer to data and new frame index
		const char* const pAnim(int* piFrame) const; // returns pointer to data and new frame index

		int numikrules;
		int ikruleindex; // non-zero when IK data is stored in the mdl
		int animblockikruleindex; // non-zero when IK data is stored in animblock file
		const mstudioikrule_t* const pIKRule(const int i) const;

		int numlocalhierarchy;
		int localhierarchyindex;;

		int sectionindex;
		int sectionframes; // number of frames used in each fast lookup section, zero if not used
		inline const mstudioanimsections_t* const pSection(int i) const { return reinterpret_cast<const mstudioanimsections_t* const>((char*)this + sectionindex) + i; }

		short zeroframespan; // frames per span
		short zeroframecount; // number of spans
		int zeroframeindex;
		inline const char* const pZeroFrameData() const { return zeroframeindex ? reinterpret_cast<const char* const>(this) + zeroframeindex : nullptr; };
		float zeroframestalltime; // saved during read stalls
	};
	static_assert(sizeof(mstudioanimdesc_t) == 0x64);

	#define NEW_EVENT_STYLE ( 1 << 10 )

	struct mstudioevent_t
	{
		float cycle;
		int event;
		int type;
		char options[64];

		int szeventindex;
		inline const char* const pszEvent() const { return reinterpret_cast<const char* const>(this) + szeventindex; }
	};

	// autolayer flags
	//							0x0001
	//							0x0002
	//							0x0004
	//							0x0008
	#define STUDIO_AL_POST		0x0010		// 
	//							0x0020
	#define STUDIO_AL_SPLINE	0x0040		// convert layer ramp in/out curve is a spline instead of linear
	#define STUDIO_AL_XFADE		0x0080		// pre-bias the ramp curve to compense for a non-1 weight, assuming a second layer is also going to accumulate
	//							0x0100
	#define STUDIO_AL_NOBLEND	0x0200		// animation always blends at 1.0 (ignores weight)
	//							0x0400
	//							0x0800
	#define STUDIO_AL_LOCAL		0x1000		// layer is a local context sequence
	#define STUDIO_AL_2000		0x2000		// skips parsing in AddSequenceLayer and AddLocalLayer if set
	#define STUDIO_AL_POSE		0x4000		// layer blends using a pose parameter instead of parent cycle
	#define STUDIO_AL_REALTIME	0x8000		// treats the layer sequence as if it uses STUDIO_REALTIME (sub_1401D9AD0 in R5pc_r5launch_N1094_CL456479_2019_10_30_05_20_PM)


	struct mstudioautolayer_t
	{
		short iSequence;
		short iPose;

		int flags;
		float start;	// beginning of influence
		float peak;		// start of full influence
		float tail;		// end of full influence
		float end;		// end of all influence
	};

	struct mstudioactivitymodifier_t
	{
		int sznameindex;
		bool negate; // if true will return false when checking if this layer (sequence) has this activity modifier, however it still has this activity modifier when checked in other places (?)

		inline const char* const pszName() const { return (reinterpret_cast<const char* const>(this) + sznameindex); }
	};
	static_assert(sizeof(mstudioactivitymodifier_t) == 0x8);

	struct mstudioseqdesc_t
	{
		int baseptr;
		const studiohdr_t* const pStudiohdr() const { return reinterpret_cast<const studiohdr_t* const>((char*)this + baseptr); }

		int	szlabelindex;
		inline const char* const pszLabel() const { return reinterpret_cast<const char* const>(this) + szlabelindex; }

		int szactivitynameindex;
		inline const char* const pszActivityName() const { return reinterpret_cast<const char* const>(this) + szactivitynameindex; }

		int flags; // looping/non-looping flags

		int activity; // initialized at loadtime to game DLL values
		int actweight;

		int numevents;
		int eventindex;
		inline const mstudioevent_t* const pEvent(const int i) const { return reinterpret_cast<mstudioevent_t*>((char*)this + eventindex) + i; }

		Vector bbmin; // per sequence bounding box
		Vector bbmax;

		int numblends;

		// Index into array of shorts which is groupsize[0] x groupsize[1] in length
		int animindexindex;
		inline const int16_t* const pAnimIndex(const int i) const { return reinterpret_cast<int16_t*>((char*)this + animindexindex) + i; }
		inline const int16_t GetAnimIndex(const int i) const { return *pAnimIndex(i); }
		inline const int AnimCount() const { return  groupsize[0] * groupsize[1]; }

		int movementindex;	// unused as of v49
		int groupsize[2];	// width x height of blends
		int paramindex[2];	// X, Y, Z, XR, YR, ZR
		float paramstart[2];// local (0..1) starting value
		float paramend[2];	// local (0..1) ending value
		int paramparent;	// unused as of v49

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
		inline const mstudioautolayer_t* const pAutoLayer(const int i) const { return reinterpret_cast<const mstudioautolayer_t* const>((char*)this + autolayerindex) + i; }

		int weightlistindex;
		inline const float* const pBoneweight(const int i) const { return reinterpret_cast<float*>((char*)this + weightlistindex) + i; }
		inline const float weight(const int i) const { return *pBoneweight(i); }

		int posekeyindex;
		inline const float* const pPoseKey(int iParam, int iAnim) const { return reinterpret_cast<float*>((char*)this + posekeyindex) + (iParam * groupsize[0]) + iAnim; }
		inline float poseKey(int iParam, int iAnim) const { return *(pPoseKey(iParam, iAnim)); }

		int numiklocks;
		int iklockindex;
		inline const mstudioiklock_t* const pIKLock(const int i) const { return reinterpret_cast<const mstudioiklock_t* const>((char*)this + iklockindex) + i; }

		// Key values
		int	keyvalueindex;
		int keyvaluesize;
		inline const char* const pKeyValues() const { return  keyvaluesize ? (reinterpret_cast<const char* const>(this) + keyvalueindex) : nullptr; }

		int cycleposeindex; // index of pose parameter to use as cycle index

		int activitymodifierindex;
		int numactivitymodifiers;
		inline const mstudioactivitymodifier_t* const pActivityModifier(const int i) const { return reinterpret_cast<mstudioactivitymodifier_t*>((char*)this + activitymodifierindex) + i; }

		int ikResetMask; // mask this ik rule type for reset, can't find the code for this, but it would either prevent reset of this type, or only allow reset of this time. only ever observed as IK_GROUND
		int unk_C4;

		int unused[3];
	};
	static_assert(sizeof(mstudioseqdesc_t) == 0xD4);

	struct mstudioposeparamdesc_t
	{
		int		sznameindex;
		inline const char* const pszName() const { return reinterpret_cast<const char* const>(this) + sznameindex; }

		int		flags;	// ???? (may contain 'STUDIO_LOOPING' flag if looping is used)
		float	start;	// starting value
		float	end;	// ending value
		float	loop;	// looping range, 0 for no looping, 360 for rotations, etc.
	};

	struct mstudiomodelgroup_t
	{
		int szlabelindex;	// textual name (NOTE: this is never actually set or used anywhere in reSource, or valve source)
		inline const char* const pszLabel() const { return reinterpret_cast<const char* const>(this) + szlabelindex; }

		int sznameindex;	// file name
		inline const char* const pszName() const { return reinterpret_cast<const char* const>(this) + sznameindex; }
	};


	//
	// Model Bodyparts
	//

	struct mstudiomodel_t;

	struct mstudiomesh_t
	{
		int material;

		int modelindex;
		inline const mstudiomodel_t* const pModel() const { return reinterpret_cast<const mstudiomodel_t* const>((char*)this + modelindex); }

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

	struct mstudiomodel_t
	{
		char name[64];

		int type;

		float boundingradius;

		int nummeshes;
		int meshindex;
		inline const mstudiomesh_t* const pMesh(int i) const { return reinterpret_cast<const mstudiomesh_t* const>((char*)this + meshindex) + i; }

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

	struct mstudiobodyparts_t
	{
		int sznameindex;
		inline const char* const pszName() const { return reinterpret_cast<const char* const>(this) + sznameindex; }

		int nummodels;
		int base;
		int modelindex; // index into models array

		inline const mstudiomodel_t* const pModel(int i) const { return reinterpret_cast<const mstudiomodel_t* const>((char*)this + modelindex) + i; };
	};

	struct mstudiotexture_t
	{
		int sznameindex;
		inline const char* const pszName() const { return reinterpret_cast<const char* const>(this) + sznameindex; }

		int unused_flags;
		int used;
		int unused1;

		// these are turned into 64 bit ints on load and only filled in memory
		int material_RESERVED;
		int clientmaterial_RESERVED;

		int unused[10];
	};
	static_assert(sizeof(mstudiotexture_t) == 0x40);


	//
	// Studio Header
	//

	// This flag is set if no hitbox information was specified
	#define STUDIOHDR_FLAGS_AUTOGENERATED_HITBOX	0x1

	// NOTE:  This flag is set at loadtime, not mdl build time so that we don't have to rebuild
	// models when we change materials.
	#define STUDIOHDR_FLAGS_USES_ENV_CUBEMAP		0x2

	// Use this when there are translucent parts to the model but we're not going to sort it 
	#define STUDIOHDR_FLAGS_FORCE_OPAQUE			0x4

	// previously 'STUDIOHDR_FLAGS_TRANSLUCENT_TWOPASS', now handled by materials
	#define STUDIOHDR_FLAGS_UNUSED_8				0x8

	// This is set any time the .qc files has $staticprop in it
	// Means there's no bones and no transforms
	#define STUDIOHDR_FLAGS_STATIC_PROP				0x10

	// NOTE:  This flag is set at loadtime, not mdl build time so that we don't have to rebuild
	// models when we change materials.
	#define STUDIOHDR_FLAGS_USES_FB_TEXTURE		    0x20

	// This flag is set by studiomdl.exe if a separate "$shadowlod" entry was present
	//  for the .mdl (the shadow lod is the last entry in the lod list if present)
	#define STUDIOHDR_FLAGS_HASSHADOWLOD			0x40

	// NOTE:  This flag is set at loadtime, not mdl build time so that we don't have to rebuild
	// models when we change materials.
	#define STUDIOHDR_FLAGS_USES_BUMPMAPPING		0x80

	// NOTE:  This flag is set when we should use the actual materials on the shadow LOD
	// instead of overriding them with the default one (necessary for translucent shadows)
	#define STUDIOHDR_FLAGS_USE_SHADOWLOD_MATERIALS	0x100

	// NOTE:  This flag is set when we should use the actual materials on the shadow LOD
	// instead of overriding them with the default one (necessary for translucent shadows)
	#define STUDIOHDR_FLAGS_OBSOLETE				0x200

	#define STUDIOHDR_FLAGS_UNUSED					0x400

	// NOTE:  This flag is set at mdl build time
	#define STUDIOHDR_FLAGS_NO_FORCED_FADE			0x800

	// NOTE:  The npc will lengthen the viseme check to always include two phonemes
	#define STUDIOHDR_FLAGS_FORCE_PHONEME_CROSSFADE	0x1000

	// This flag is set when the .qc has $constantdirectionallight in it
	// If set, we use constantdirectionallightdot to calculate light intensity
	// rather than the normal directional dot product
	// only valid if STUDIOHDR_FLAGS_STATIC_PROP is also set
	#define STUDIOHDR_FLAGS_CONSTANT_DIRECTIONAL_LIGHT_DOT 0x2000

	// previously 'STUDIOHDR_FLAGS_FLEXES_CONVERTED' but flexes are unused/remmoved in r1/2
	#define STUDIOHDR_FLAGS_UNUSED_4000				0x4000

	// Indicates the studiomdl was built in preview mode
	#define STUDIOHDR_FLAGS_BUILT_IN_PREVIEW_MODE	0x8000

	// Ambient boost (runtime flag)
	#define STUDIOHDR_FLAGS_AMBIENT_BOOST			0x10000

	// Don't cast shadows from this model (useful on first-person models)
	#define STUDIOHDR_FLAGS_DO_NOT_CAST_SHADOWS		0x20000

	// alpha textures should cast shadows in vrad on this model (ONLY prop_static!)
	#define STUDIOHDR_FLAGS_CAST_TEXTURE_SHADOWS	0x40000

	// Model has a quad-only Catmull-Clark SubD cage
	#define STUDIOHDR_FLAGS_SUBDIVISION_SURFACE		0x80000

	// flagged on load to indicate no animation events on this model
	// might be a different thing on v54
	#define STUDIOHDR_FLAGS_NO_ANIM_EVENTS			0x100000

	// If flag is set then studiohdr_t.flVertAnimFixedPointScale contains the
	// scale value for fixed point vert anim data, if not set then the
	// scale value is the default of 1.0 / 4096.0.  Regardless use
	// studiohdr_t::VertAnimFixedPointScale() to always retrieve the scale value
	#define STUDIOHDR_FLAGS_VERT_ANIM_FIXED_POINT_SCALE	0x200000

	// used for checking legacy frame anim flags on models, likely set on older models during load? todo confirm
	#define STUDIOHDR_FLAGS_LEGACY_MODEL				0x800000

	// If this flag is present the model has vertex color, and by extension a VVC (IDVC) file.
	#define STUDIOHDR_FLAGS_USES_VERTEX_COLOR	        0x1000000

	// If this flag is present the model has a secondary UV layer, and by extension a VVC (IDVC) file.
	#define STUDIOHDR_FLAGS_USES_UV2			        0x2000000

	struct studiohdr2_t
	{
		int numsrcbonetransform;
		int srcbonetransformindex;

		int	illumpositionattachmentindex;
		inline const int IllumPositionAttachmentIndex() const { return illumpositionattachmentindex; }


		float flMaxEyeDeflection; // default to cos(30) if not set
		inline const float MaxEyeDeflection() const { return flMaxEyeDeflection != 0.0f ? flMaxEyeDeflection : 0.866f; }

		int linearboneindex;
		inline const mstudiolinearbone_t* const pLinearBones() const { return linearboneindex ? reinterpret_cast<const mstudiolinearbone_t* const>((char*)this + linearboneindex) : nullptr; }

		int sznameindex;
		inline const char* const pszName() const { return reinterpret_cast<const char* const>(this) + sznameindex; }

		int m_nBoneFlexDriverCount;
		int m_nBoneFlexDriverIndex;

		// for static props (and maybe others)
		// Precomputed Per-Triangle AABB data
		int m_nPerTriAABBIndex;
		int m_nPerTriAABBNodeCount;
		int m_nPerTriAABBLeafCount;
		int m_nPerTriAABBVertCount;
		inline const mstudiopertrihdr_t* const pPerTriHdr() const { return m_nPerTriAABBIndex ? reinterpret_cast<const mstudiopertrihdr_t*>((char*)this + m_nPerTriAABBIndex) : nullptr; }
		inline const mstudiopertrinode_t* const pPerTriNode(const int i) const { return reinterpret_cast<const mstudiopertrinode_t* const>(&pPerTriHdr()[1]) + i; }
		inline const mstudiopertrileaf_t* const pPerTriLeaf(const int i) const { return reinterpret_cast<const mstudiopertrileaf_t* const>(pPerTriNode(m_nPerTriAABBNodeCount)) + i; }
		inline const mstudiopertrivert_t* const pPerTriVert(const int i) const { return reinterpret_cast<const mstudiopertrivert_t* const>(pPerTriLeaf(m_nPerTriAABBLeafCount)) + i; }

		inline const mstudiopertrileaf_v1_t* const pPerTriLeaf_V1(const int i) const { return reinterpret_cast<const mstudiopertrileaf_v1_t* const>(pPerTriNode(m_nPerTriAABBNodeCount)) + i; }
		inline const mstudiopertrivert_t* const pPerTriVert_V1(const int i) const { return reinterpret_cast<const mstudiopertrivert_t* const>(pPerTriLeaf_V1(m_nPerTriAABBLeafCount)) + i; }

		// always "" or "Titan"
		int unkStringOffset;
		inline const char* const pszUnkString() const { return reinterpret_cast<const char* const>(this) + unkStringOffset; }

		int reserved[39];
	};
	static_assert(sizeof(studiohdr2_t) == 0xD0);

	struct studiohdr_t
	{
		int id; // Model format ID, such as "IDST" (0x49 0x44 0x53 0x54)
		int version; // Format version number, such as 52 (0x34,0x00,0x00,0x00)
		int checksum; // This has to be the same in the phy and vtx files to load!
		inline const char* const pszName() const { return studiohdr2index ? pStudioHdr2()->pszName() : nullptr; }
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
		inline const mstudiobone_t* const pBone(int i) const { assert(i >= 0 && i < numbones); return reinterpret_cast<mstudiobone_t*>((char*)this + boneindex) + i; }

		int numbonecontrollers; // bone controllers
		int bonecontrollerindex;

		int numhitboxsets;
		int hitboxsetindex;
		inline const mstudiohitboxset_t* const pHitboxSet(const int i) const
		{
			assert(i >= 0 && i < numhitboxsets);
			return reinterpret_cast<const mstudiohitboxset_t* const>((char*)this + hitboxsetindex) + i;
		};

		int numlocalanim; // animations/poses
		int localanimindex; // animation descriptions
		inline const mstudioanimdesc_t* const pAnimdesc(const int i) const { assert(i >= 0 && i < numlocalanim); return reinterpret_cast<mstudioanimdesc_t*>((char*)this + localanimindex) + i; }

		int numlocalseq; // sequences
		int	localseqindex;
		inline const mstudioseqdesc_t* const pSeqdesc(const int i) const { assert(i >= 0 && i < numlocalseq); return reinterpret_cast<mstudioseqdesc_t*>((char*)this + localseqindex) + i; }

		int activitylistversion; // initialization flag - have the sequences been indexed?
		int eventsindexed;

		// raw textures
		int numtextures;
		int textureindex;
		inline const mstudiotexture_t* const pTexture(int i) const { assert(i >= 0 && i < numtextures); return reinterpret_cast<mstudiotexture_t*>((char*)this + textureindex) + i; }

		// raw textures search paths
		int numcdtextures;
		int cdtextureindex;
		inline const char* const pCdtexture(const int i) const { return reinterpret_cast<const char* const>(this) + reinterpret_cast<const int* const>((char*)this + cdtextureindex)[i]; }

		// replaceable textures tables
		int numskinref;
		int numskinfamilies;
		int skinindex;
		inline const int16_t* const pSkinref(const int i) const { return reinterpret_cast<const int16_t* const>((char*)this + skinindex) + i; }
		inline const int16_t* const pSkinFamily(const int i) const { return pSkinref(numskinref * i); };

		int numbodyparts;
		int bodypartindex;
		inline const mstudiobodyparts_t* const pBodypart(int i) const { assert(i >= 0 && i < numbodyparts); return reinterpret_cast<mstudiobodyparts_t*>((char*)this + bodypartindex) + i; }

		int numlocalattachments;
		int localattachmentindex;
		inline const mstudioattachment_t* const pLocalAttachment(const int i) const { assert(i >= 0 && i < numlocalattachments); return reinterpret_cast<const mstudioattachment_t* const>((char*)this + localattachmentindex) + i; }

		int numlocalnodes;
		int localnodeindex;
		int localnodenameindex;
		inline const char* const pszLocalNodeName(const int iNode) const { return reinterpret_cast<const char* const>((char*)this + reinterpret_cast<const int* const>((char*)this + localnodenameindex)[iNode]); }
		inline const uint8_t* const pLocalTransition(const int i) const { return reinterpret_cast<const uint8_t* const>((char*)this + localnodeindex) + i; }

		int deprecated_numflexdesc;
		int deprecated_flexdescindex;

		int deprecated_numflexcontrollers;
		int deprecated_flexcontrollerindex;

		int deprecated_numflexrules;
		int deprecated_flexruleindex;

		int numikchains;
		int ikchainindex;
		inline const mstudioikchain_t* const pIKChain(const int i) const { assert(i >= 0 && i < numikchains); return reinterpret_cast<const mstudioikchain_t* const>((char*)this + ikchainindex) + i; }

		int deprecated_nummouths;
		int deprecated_mouthindex;

		int numlocalposeparameters;
		int localposeparamindex;
		inline const mstudioposeparamdesc_t* const pLocalPoseParameter(const int i) const { assert(i >= 0 && i < numlocalposeparameters); return reinterpret_cast<const mstudioposeparamdesc_t* const>((char*)this + localposeparamindex) + i; }

		int surfacepropindex;
		inline const char* const pszSurfaceProp() const { return reinterpret_cast<const char* const>(this) + surfacepropindex; }

		int keyvalueindex;
		int keyvaluesize;
		inline const char* const KeyValueText() const { return reinterpret_cast<const char* const>(this) + keyvalueindex; }

		int numlocalikautoplaylocks;
		int localikautoplaylockindex;
		inline const mstudioiklock_t* const pLocalIKAutoplayLock(const int i) const { assert(i >= 0 && i < numlocalikautoplaylocks); return reinterpret_cast<const mstudioiklock_t* const>((char*)this + localikautoplaylockindex) + i; }

		float mass;
		int contents;

		// external animations, models, etc.
		int numincludemodels;
		int includemodelindex;
		inline const mstudiomodelgroup_t* const pModelGroup(const int i) const { assert(i >= 0 && i < numincludemodels); return reinterpret_cast<const mstudiomodelgroup_t* const>((char*)this + includemodelindex) + i; }

		// implementation specific back pointer to virtual data
		int /* mutable void* */ virtualModel;

		// for demand loaded animation blocks
		int szanimblocknameindex;
		inline const char* const pszAnimBlockName() const { return szanimblocknameindex ? reinterpret_cast<const char* const>(this) + szanimblocknameindex : nullptr; }
		int numanimblocks;
		int animblockindex;
		inline const mstudioanimblock_t* const pAnimBlock(const int i) const { return reinterpret_cast<const mstudioanimblock_t* const>((char*)this + animblockindex) + i; };
		int /* mutable void* */ animblockModel;
		inline const char* const GetAnimBlock(const int i) const;

		int bonetablebynameindex;

		// used by tools only that don't cache, but persist mdl's peer data
		// engine uses virtualModel to back link to cache pointers
		int /* void* */ pVertexBase;
		int /* void* */ pIndexBase;

		// if STUDIOHDR_FLAGS_CONSTANT_DIRECTIONAL_LIGHT_DOT is set,
		// this value is used to calculate directional components of lighting 
		// on static props
		uint8_t constdirectionallightdot;

		// set during load of mdl data to track *desired* lod configuration (not actual)
		// the *actual* clamped root lod is found in studiohwdata
		// this is stored here as a global store to ensure the staged loading matches the rendering
		uint8_t rootLOD;

		// set in the mdl data to specify that lod configuration should only allow first numAllowRootLODs
		// to be set as root LOD:
		//	numAllowedRootLODs = 0	means no restriction, any lod can be set as root lod.
		//	numAllowedRootLODs = N	means that lod0 - lod(N-1) can be set as root lod, but not lodN or lower.
		uint8_t numAllowedRootLODs;

		uint8_t unused;

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
		inline const studiohdr2_t* const pStudioHdr2() const { return reinterpret_cast<studiohdr2_t*>((char*)this + studiohdr2index); }

		// stored maya files from used dmx files, animation files are not added. for internal tools likely
		// in r1 this is a mixed bag, some are null with no data, some have a four byte section, and some actually have the files stored.
		int sourceFilenameOffset;
		inline const char* const pszSourceFiles() const { return ((char*)this + sourceFilenameOffset); }

		inline const int NumSrcBoneTransforms() const { return studiohdr2index ? pStudioHdr2()->numsrcbonetransform : 0; }
		inline const mstudiosrcbonetransform_t* const SrcBoneTransform(int i) const { return studiohdr2index ? reinterpret_cast<const mstudiosrcbonetransform_t* const>((char*)this + pStudioHdr2()->srcbonetransformindex) + i : nullptr; }

		inline const int IllumPositionAttachmentIndex() const { return studiohdr2index ? pStudioHdr2()->IllumPositionAttachmentIndex() : 0; }

		inline const float MaxEyeDeflection() const { return studiohdr2index ? pStudioHdr2()->MaxEyeDeflection() : 0.866f; } // default to cos(30) if not set

		inline const mstudiolinearbone_t* const pLinearBones() const { return studiohdr2index ? pStudioHdr2()->pLinearBones() : nullptr; }

		inline const mstudiopertrihdr_t* const pPerTriHdr() const { return studiohdr2index ? pStudioHdr2()->pPerTriHdr() : nullptr; }
		inline const mstudiopertrinode_t* const pPerTriNode(const int i) const { return studiohdr2index ? pStudioHdr2()->pPerTriNode(i) : nullptr; }
		inline const mstudiopertrileaf_t* const pPerTriLeaf(const int i) const { return studiohdr2index ? pStudioHdr2()->pPerTriLeaf(i) : nullptr; }
		inline const mstudiopertrivert_t* const pPerTriVert(const int i) const { return studiohdr2index ? pStudioHdr2()->pPerTriVert(i) : nullptr; }

		inline const mstudiopertrileaf_v1_t* const pPerTriLeaf_V1(const int i) const { return studiohdr2index ? pStudioHdr2()->pPerTriLeaf_V1(i) : nullptr; }
		inline const mstudiopertrivert_t* const pPerTriVert_V1(const int i) const { return studiohdr2index ? pStudioHdr2()->pPerTriVert_V1(i) : nullptr; }

		inline const char* const pszUnkString() const { return studiohdr2index ? pStudioHdr2()->pszUnkString() : nullptr; }
	};
	static_assert(sizeof(studiohdr_t) == 0x198);

	// for everything r2 and before
	void ExtractAnimValue(int frame, const mstudioanimvalue_t* panimvalue, float scale, float& v1, float& v2);
	void ExtractAnimValue(int frame, const mstudioanimvalue_t* panimvalue, float scale, float& v1);
}
#pragma pack(pop)