#pragma once

#pragma pack(push, 4) // required for quite a few structs
enum MaterialShaderType_t : uint8_t;
namespace r5
{
	//
	// Model BVH Collision
	// 

	// leaf and node data is located in bvh.h (not in this project currently)

	// headers
	struct dsurfaceproperty_t
	{
		short unk_0;
		uint8_t surfacePropId;
		uint8_t contentMaskOffset;
		int surfaceNameOffset;
	};

	struct mstudiocollmodel_v8_t
	{
		int contentMasksIndex;
		int surfacePropsIndex;
		int surfaceNamesIndex;
		int headerCount;

		const dsurfaceproperty_t* const pSurfaceProp(const int idx) const { return reinterpret_cast<const dsurfaceproperty_t* const>((char*)this + surfacePropsIndex) + idx; }
	};

	struct mstudiocollheader_v8_t
	{
		int unk;
		int bvhNodeIndex;
		int vertIndex;
		int bvhLeafIndex;

		float origin[3];
		float scale;
	};

	// headers
	struct dsurfacepropertydata_t
	{
		uint8_t surfacePropId1;
		uint8_t surfacePropId2;
		uint16_t flags;
	};

	struct mstudiocollheader_v12_t
	{
		int unk_0;
		int bvhNodeIndex;
		int vertIndex;
		int bvhLeafIndex;

		// [amos]
		// A new index that indexes into a buffer that contains surfacePropId's
		// The number of surfacePropId's is determined by surfacePropCount.
		// surfacePropArrayCount determines how many of these buffers we have,
		// they are the same size but sometimes with different surfacePropId's.
		int surfacePropDataIndex;

		uint8_t surfacePropArrayCount;
		uint8_t surfacePropCount;
		const dsurfacepropertydata_t* const pSurfaceProp(const int idx) const { return reinterpret_cast<const dsurfacepropertydata_t* const>((char*)this + surfacePropDataIndex) + idx; }

		short padding_maybe;

		float origin[3];
		float scale;
	};


	//
	// Model Bones
	//

	#define JIGGLE_IS_FLEXIBLE				0x01 // JIGGLE_IS_RIGID if not set
	#define JIGGLE_HAS_FLEX					0x02 // (JIGGLE_IS_FLEXIBLE | JIGGLE_IS_RIGID) in old code, see note below
	#define JIGGLE_HAS_YAW_CONSTRAINT		0x04
	#define JIGGLE_HAS_PITCH_CONSTRAINT		0x08
	#define JIGGLE_HAS_ANGLE_CONSTRAINT		0x10
	#define JIGGLE_HAS_LENGTH_CONSTRAINT	0x20 // having this flag should mean is_flexible or is_rigid is used, however it's used regardless of if 'JIGGLE_HAS_FLEX', which would skip any code for this, and rigid or flexible
	#define JIGGLE_HAS_BASE_SPRING			0x40
	// JIGGLE_HAS_FLEX: this implementation allows it to be independent of type, despite not being utilized in code (flex/rigid codepaths are skipped if this is not set).
	// JIGGLE_HAS_FLEX: some cases of this not being set while flex is, how does that work?

// apex changed this a bit, 'is_rigid' cut
	struct mstudiojigglebone_v8_t
	{
		uint8_t flags;
		uint8_t bone; // id of bone, might be single byte

		uint8_t pad[2];

		// general params
		float length; // how far from bone base, along bone, is tip
		float tipMass;
		float tipFriction; // friction applied to tip velocity, 0-1

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

		int unk_B0;

		int collisionIndex; // index into sections of collision, phy, bvh, etc. needs confirming
	};

	// this struct is the same in r1 and r2
	struct mstudiolinearbone_v8_t
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
	};

	struct mstudioattachment_v8_t
	{
		int sznameindex;
		inline const char* const pszName() const { return reinterpret_cast<const char* const>(this) + sznameindex; }
		int flags;
		int localbone; // parent bone
		matrix3x4_t local; // attachment point
	};

	struct mstudiobbox_v8_t
	{
		int bone;
		int group; // intersection group
		Vector bbmin; // bounding box
		Vector bbmax;
		int szhitboxnameindex; // offset to the name of the hitbox.
		inline const char* const pszHitboxName() const
		{
			if (szhitboxnameindex == 0)
				return "";

			return reinterpret_cast<const char* const>(this) + szhitboxnameindex;
		}

		int forceCritPoint;	// value of one causes this to act as if it was group of 'head', may either be crit override or group override. mayhaps aligned boolean, look into this
		int hitdataGroupOffset;	// hit_data group in keyvalues this hitbox uses.
	};

	// this struct is the same in r1, r2, and early r5, and unchanged from p2
	struct mstudiohitboxset_v8_t
	{
		int	sznameindex;
		int	numhitboxes;
		int	hitboxindex;
	};


	//
	// Model IK Info
	//

	struct mstudioiklock_v8_t
	{
		int chain;
		float flPosWeight;
		float flLocalQWeight;
		int flags;
	};

	struct mstudioiklink_v8_t
	{
		int bone;
		Vector kneeDir;	// ideal bending direction (per link, if applicable)
	};

	struct mstudioikchain_v8_t
	{
		int sznameindex;
		inline const char* const pszName() const { return reinterpret_cast<const char* const>(this) + sznameindex; }

		int	linktype;
		int	numlinks;
		int	linkindex;
		const mstudioiklink_v8_t* const pLink(int i) const { return reinterpret_cast<const mstudioiklink_v8_t* const>((char*)this + linkindex) + i; }

		float unk_10;	// tried, and tried to find what this does, but it doesn't seem to get hit in any normal IK code paths
						// however, in Apex Legends this value is 0.866 which happens to be cos(30) (https://github.com/NicolasDe/AlienSwarm/blob/master/src/public/studio.h#L2173)
						// and in Titanfall 2 it's set to 0.707 or alternatively cos(45).
						// TLDR: likely set in QC $ikchain command from a degrees entry, or set with a default value in studiomdl when not defined.
	};

	struct mstudiocompressedikerror_v8_t
	{
		float scale[6]; // these values are the same as what posscale (if it was used) and rotscale are.
		int sectionframes; // frames per section, may not match animdesc
	};

	/*#define IK_SELF 1
	#define IK_WORLD 2
	#define IK_GROUND 3
	#define IK_RELEASE 4
	#define IK_ATTACHMENT 5
	#define IK_UNLATCH 6*/

	struct mstudioikrule_v8_t
	{
		int index;
		int type;
		int chain;
		int bone; // gets it from ikchain now pretty sure

		int slot; // iktarget slot. Usually same as chain.
		float height;
		float radius;
		float floor;
		Vector pos;
		Quaternion q;

		// apex does this oddly
		mstudiocompressedikerror_v8_t compressedikerror;
		int compressedikerrorindex;

		int iStart;
		int ikerrorindex;

		float start; // beginning of influence
		float peak; // start of full influence
		float tail; // end of full influence
		float end; // end of all influence

		float contact; // frame footstep makes ground concact
		float drop; // how far down the foot should drop when reaching for IK
		float top; // top of the foot box

		int szattachmentindex; // name of world attachment
		inline const char* const pszAttachment() const { return szattachmentindex ? reinterpret_cast<const char* const>(this) + szattachmentindex : nullptr; }

		float endHeight; // new in v52   
	};


	//
	// Model Animation
	// 

	enum ValuePtrFlags_t : uint8_t
	{
		STUDIO_ANIMPTR_Z = 0x01,
		STUDIO_ANIMPTR_Y = 0x02,
		STUDIO_ANIMPTR_X = 0x04,
	};

	// to get mstudioanim_value_t for each axis:
	// if 1 flag, only read from offset
	// if 2 flags (e.g. y,z):
	// y @ offset;
	// z @ offset + (idx1 * sizeof(mstudioanimvalue_t));

	// if 3 flags:
	// x @ offset;
	// y @ offset + (idx1 * sizeof(mstudioanimvalue_t));
	// z @ offset + (idx2 * sizeof(mstudioanimvalue_t));
	struct mstudioanim_valueptr_t
	{
		uint16_t offset : 13;
		uint16_t flags : 3;
		uint8_t axisIdx1; // these two are definitely unsigned
		uint8_t axisIdx2;

		inline mstudioanimvalue_t* pAnimvalue() const { return reinterpret_cast<mstudioanimvalue_t*>((char*)this + offset); }
	};
	static_assert(sizeof(mstudioanim_valueptr_t) == 0x4);

	// flags for the per bone animation headers
	// "mstudioanim_valueptr_t" indicates it has a set of offsets into anim tracks
	enum RleFlags_t : uint8_t
	{
		STUDIO_ANIM_ANIMSCALE = 0x01, // mstudioanim_valueptr_t
		STUDIO_ANIM_ANIMROT = 0x02, // mstudioanim_valueptr_t
		STUDIO_ANIM_ANIMPOS = 0x04, // mstudioanim_valueptr_t
	};

	// flags for the per bone array, in 4 bit sections (two sets of flags per char), aligned to two chars
	enum RleBoneFlags_t :uint8_t
	{
		STUDIO_ANIM_POS		= 0x1,	// bone has pos values
		STUDIO_ANIM_ROT		= 0x2,	// bone has rot values
		STUDIO_ANIM_SCALE	= 0x4,	// bone has scale values
		STUDIO_ANIM_UNK8	= 0x8,	// used in virtual models for new anim type
		STUDIO_ANIM_UNK10	= 0x10,	// TBD
		STUDIO_ANIM_UNK20	= 0x20,	// TBD

		STUDIO_ANIM_DATA	= (STUDIO_ANIM_POS | STUDIO_ANIM_ROT | STUDIO_ANIM_SCALE), // bone has animation data
		STUDIO_ANIM_MASK_RELEASE	= (STUDIO_ANIM_POS | STUDIO_ANIM_ROT | STUDIO_ANIM_SCALE | STUDIO_ANIM_UNK8),
		STUDIO_ANIM_MASK_RETAIL		= (STUDIO_ANIM_POS | STUDIO_ANIM_ROT | STUDIO_ANIM_SCALE | STUDIO_ANIM_UNK8 | STUDIO_ANIM_UNK10 | STUDIO_ANIM_UNK20),
	};

	// from release to season 29
	#define ANIM_BONEFLAG_BITS_4			4 // a nibble even
	#define ANIM_BONEFLAG_SIZE_4(count)		(IALIGN2((ANIM_BONEFLAG_BITS_4 * count + 7) / 8)) // size in bytes of the bone flag array. pads 7 bits for truncation, then align to 2 bytes
	#define ANIM_BONEFLAG_SHIFT_4(idx)		(ANIM_BONEFLAG_BITS_4 * (idx % 2)) // return four (bits) if this index is odd, as there are four bits per bone
	#define ANIM_BONEFLAGS_FLAG_4(ptr, idx)	(static_cast<uint8_t>(ptr[idx / 2] >> ANIM_BONEFLAG_SHIFT_4(idx)) & r5::RleBoneFlags_t::STUDIO_ANIM_MASK_RELEASE) // get byte offset, then shift if needed, mask to four bits

	// starting in season 30 we have six bits for bone flags
	#define ANIM_BONEFLAG_BITS_6			6 // a morsel ?
	#define ANIM_BONEFLAG_SIZE_6(count)		(IALIGN2((ANIM_BONEFLAG_BITS_6 * count + 7) / 8))	// size in bytes of the bone flag array. pads 7 bits for truncation, then align to 2 bytes !!! TBD if this works the same !!!
	#define ANIM_BONEFLAG_SHIFT_6(idx)		(ANIM_BONEFLAG_BITS_6 * (idx % 4))					// return four (bits) if this index is odd, as there are four bits per bone
	#define ANIM_BONEFLAGS_FLAG_6(ptr, idx)	(static_cast<uint8_t>(*reinterpret_cast<const uint32_t* const>(ptr + ((idx / 4) * 3)) >> ANIM_BONEFLAG_SHIFT_6(idx)) & r5::RleBoneFlags_t::STUDIO_ANIM_MASK_RETAIL) // get byte offset, then shift if needed, mask to four bits

	#define ANIM_BONEFLAG_SIZE(count, width) (width == ANIM_BONEFLAG_BITS_6 ? ANIM_BONEFLAG_SIZE_6(count) : ANIM_BONEFLAG_SIZE_4(count)) 
	#define ANIM_BONEFLAGS_FLAG(ptr, idx, width) (width == ANIM_BONEFLAG_BITS_6 ? ANIM_BONEFLAGS_FLAG_6(ptr, idx) : ANIM_BONEFLAGS_FLAG_4(ptr, idx)) 

	struct mstudio_rle_anim_t
	{
	public:
		uint16_t size : 13; // total size of all animation data, not next offset because even the last one has it
		uint16_t flags : 3;

		inline char* pData() const { return ((char*)this + sizeof(mstudio_rle_anim_t)); } // gets start of animation data

		// we have to input the bone flags so we can get the proper offset of each data, pos is always first so it's fine
		// valid for animating data only
		inline mstudioanim_valueptr_t* pRotV(const char boneFlags) const { return reinterpret_cast<mstudioanim_valueptr_t*>(pData() + PosSize(boneFlags)); } // returns rot as mstudioanim_valueptr_t
		inline mstudioanim_valueptr_t* pPosV() const { return reinterpret_cast<mstudioanim_valueptr_t*>(pData() + 4); } // returns pos as mstudioanim_valueptr_t
		inline mstudioanim_valueptr_t* pScaleV(const char boneFlags) const { return reinterpret_cast<mstudioanim_valueptr_t*>(pData() + PosSize(boneFlags) + RotSize(boneFlags)); } // returns scale as mstudioanim_valueptr_t

		// valid if animation unvaring over timeline
		inline Quaternion64* pQuat64(const char boneFlags) const { return reinterpret_cast<Quaternion64*>(pData() + PosSize(boneFlags)); } // returns rot as static Quaternion64
		inline Vector48* pPos() const { return reinterpret_cast<Vector48*>(pData()); } // returns pos as static Vector48
		inline Vector48* pScale(const char boneFlags) const { return reinterpret_cast<Vector48*>(pData() + PosSize(boneFlags) + RotSize(boneFlags)); } // returns scale as static Vector48

		inline const float PosScale() const { return *reinterpret_cast<float*>(pData()); } // pos scale

		inline int Size() const { return static_cast<int>(size); } // size of all data including RLE header

		// points to next bone in the list
		inline mstudio_rle_anim_t* pNext() const { return reinterpret_cast<mstudio_rle_anim_t*>((char*)this + Size()); } // can't return nullptr

	private:
		inline const int PosSize(const char boneFlags) const { return (flags & RleFlags_t::STUDIO_ANIM_ANIMPOS ? 8 : 6) * (boneFlags & RleBoneFlags_t::STUDIO_ANIM_POS); }
		inline const int RotSize(const char boneFlags) const { return (flags & RleFlags_t::STUDIO_ANIM_ANIMROT ? 4 : 8) * (boneFlags & RleBoneFlags_t::STUDIO_ANIM_ROT) >> 1; } // shift one bit for half size
	};
	static_assert(sizeof(mstudio_rle_anim_t) == 0x2);

	// new in Titanfall 1
	// translation track for origin bone, used in lots of animated scenes, requires STUDIO_FRAMEMOVEMENT
	// pos_x, pos_y, pos_z, yaw
	struct mstudioframemovement_t
	{
		float scale[4];
		int sectionframes;

		inline const int SectionCount(const int numframes) const { return (numframes - 1) / sectionframes + 1; } // this might be wrong :(
		inline int SectionIndex(const int frame) const { return frame / sectionframes; } // get the section index for this frame
		inline int SectionOffset(const int frame) const { return reinterpret_cast<int*>((char*)this + sizeof(mstudioframemovement_t))[SectionIndex(frame)]; } // offset is int
		inline int SectionOffsetRetail(const int frame) const { return FIX_OFFSET(reinterpret_cast<uint16_t*>((char*)this + sizeof(mstudioframemovement_t))[SectionIndex(frame)]); } // offset is uint16_t
		inline uint16_t* pSection(const int frame, const bool retail) const { return reinterpret_cast<uint16_t*>((char*)this + (retail ? SectionOffsetRetail(frame) : SectionOffset(frame))); }

		inline mstudioanimvalue_t* pAnimvalue(const int i, const uint16_t* section) const { return (section[i] > 0) ? reinterpret_cast<mstudioanimvalue_t*>((char*)section + section[i]) : nullptr; }

		inline const uint8_t* const pDatapointTrack() const
		{
			constexpr int baseOffset = sizeof(mstudioframemovement_t) + sizeof(uint16_t);
			return reinterpret_cast<const uint8_t* const>(this) + baseOffset;
		}
	};

	struct mstudioanimsections_v8_t
	{
		int animindex;
	};

	// sequence and autolayer flags
	//#define STUDIO_LOOPING		0x0001		// ending frame should be the same as the starting frame
	//#define STUDIO_SNAP			0x0002		// do not interpolate between previous animation and this one
	//#define STUDIO_DELTA		0x0004		// this sequence "adds" to the base sequences, not slerp blends
	//#define STUDIO_AUTOPLAY		0x0008		// temporary flag that forces the sequence to always play
	//#define STUDIO_POST			0x0010		// 
	//#define STUDIO_ALLZEROS		0x0020		// this animation/sequence has no real animation data
	//#define STUDIO_ANIM_UNK40	0x0040		// suppgest ?
	//#define STUDIO_CYCLEPOSE	0x0080		// cycle index is taken from a pose parameter index
	//#define STUDIO_REALTIME		0x0100		// cycle index is taken from a real-time clock, not the animations cycle index
	//#define STUDIO_LOCAL		0x0200		// sequence has a local context sequence
	//#define STUDIO_HIDDEN		0x0400		// don't show in default selection views
	//#define STUDIO_OVERRIDE		0x0800		// a forward declared sequence (empty)
	//#define STUDIO_ACTIVITY		0x1000		// Has been updated at runtime to activity index
	//#define STUDIO_EVENT		0x2000		// Has been updated at runtime to event index on server
	//#define STUDIO_WORLD		0x4000		// sequence blends in worldspace
	//#define STUDIO_NOFORCELOOP		0x8000	// do not force the animation loop
	//#define STUDIO_EVENT_CLIENT		0x10000	// Has been updated at runtime to event index on client
	//#define STUDIO_HAS_SCALE		0x20000 // only appears on anims with scale, used for quick lookup in ScaleBones, only reason this should be check is if there has been scale data parsed in, otherwise it is pointless (see ScaleBones checking if scale is 1.0f). note: in r5 this is only set on sequence
	//#define STUDIO_HAS_ANIM			0x20000 // if not set there is no useable anim data
	//#define STUDIO_FRAMEMOVEMENT    0x40000 // framemovements are only read if this flag is present
	//#define STUDIO_SINGLE_FRAME		0x80000 // this animation/sequence only has one frame of animation data
	//#define STUDIO_ANIM_UNK100000	0x100000	// reactive animations? seemingly only used on sequences for reactive skins, speicifcally the idle sequence
	//#define STUDIO_DATAPOINTANIM	0x200000

	struct mstudioanimdesc_v8_t
	{
		int baseptr;

		// in apex the name seems to be generated when baking, it follows a format: 'name__cmd_hash'
		// where:
		// name is the name of the dmx file,
		// cmd is the qc commands on the $animation qc line
		// hash is an unknown number that trails it all
		int sznameindex;
		inline const char* const pszName() const { return reinterpret_cast<const char* const>(this) + sznameindex; }

		float fps; // frames per second	
		int flags; // looping/non-looping flags

		int numframes;

		// piecewise movement
		int nummovements;
		int movementindex;
		inline const mstudiomovement_t* const pMovement(int i) const { return reinterpret_cast<const mstudiomovement_t* const>((char*)this + movementindex) + i; };

		int framemovementindex; // new in v52
		inline const mstudioframemovement_t* const pFrameMovement() const { return reinterpret_cast<const mstudioframemovement_t* const>((char*)this + framemovementindex); }

		int animindex; // non-zero when anim data isn't in sections
		//char* pAnimdata(int* piFrame) const; // data array, starting with per bone flags
		//mstudio_rle_anim_t* pAnim(int* piFrame, const int boneCount) const; // returns pointer to data and new frame index

		int numikrules;
		int ikruleindex; // non-zero when IK data is stored in the mdl
		inline const mstudioikrule_v8_t* const pIKRule(const int i) const { return reinterpret_cast<const mstudioikrule_v8_t* const>((char*)this + ikruleindex) + i; }

		int sectionindex;
		int sectionframes; // number of frames used in each fast lookup section, zero if not used
		inline const mstudioanimsections_v8_t* const pSection(const int i) const { return reinterpret_cast<const mstudioanimsections_v8_t* const>((char*)this + sectionindex) + i; }
		// numsections = ((numframes - 1) / sectionframes) + 2
	};
	static_assert(sizeof(mstudioanimdesc_v8_t) == 0x34);

	struct mstudioevent_v8_t
	{
		float	cycle;
		int		event;
		int		type;
		char	options[256];

		int		szeventindex;
		inline const char* const pszEvent() const { return reinterpret_cast<const char* const>(this) + szeventindex; }
	};

	struct mstudioautolayer_v8_t
	{
		uint64_t sequence; // hashed aseq guid asset, this needs to have a guid descriptor in rpak

		int iPose;

		int flags;
		float start;	// beginning of influence
		float peak;	// start of full influence
		float tail;	// end of full influence
		float end;	// end of all influence
	};

	// todo: check for iklock struct even though it's never used

	struct mstudioactivitymodifier_v8_t
	{
		int sznameindex;
		bool negate; // negate all other activity modifiers when this one is active?

		inline const char* const pszName() const { return (reinterpret_cast<const char* const>(this) + sznameindex); }
	};
	static_assert(sizeof(mstudioactivitymodifier_v8_t) == 0x8);

	// weight fixups, mostly for procbones, uses linearprocbones
	// might not be correct, does some weird stuff
	// 01401D9F80
	struct mstudioseqweightfixup_v8_t
	{
		// generally 0-1
		float unk_0; // used to scale the result

		int bone; // bone idx

		// used to adjust cycle
		float unk_8;
		float unk_C;
		float unk_10;
		float unk_14;
	};

	struct mstudioanimdesc_v12_1_t;
	struct mstudioevent_v12_3_t;
	struct mstudioseqdesc_v8_t
	{
		int baseptr;

		int	szlabelindex;
		inline const char* const pszLabel() const { return reinterpret_cast<const char* const>(this) + szlabelindex; }

		int szactivitynameindex;
		inline const char* const pszActivityName() const { return reinterpret_cast<const char* const>(this) + szactivitynameindex; }

		int flags; // looping/non-looping flags

		int activity; // initialized at loadtime to game DLL values
		int actweight;

		int numevents;
		int eventindex;
		inline const mstudioevent_v8_t* const pEvent_V8(const int i) const { return reinterpret_cast<mstudioevent_v8_t*>((char*)this + eventindex) + i; }
		inline const mstudioevent_v12_3_t* const pEvent_V12_3(const int i) const;

		Vector bbmin; // per sequence bounding box
		Vector bbmax;

		int numblends;

		// Index into array of ints which is groupsize[0] x groupsize[1] in length
		int animindexindex;
		const int AnimIndex(const int i) const { return reinterpret_cast<int*>((char*)this + animindexindex)[i]; }
		const int AnimCount() const { return  groupsize[0] * groupsize[1]; }
		const mstudioanimdesc_v8_t* const pAnimDesc_V8(const int i) const { return reinterpret_cast<const mstudioanimdesc_v8_t* const>((char*)this + AnimIndex(i)); }
		const mstudioanimdesc_v12_1_t* const pAnimDesc_V12_1(const int i) const;

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
		inline const mstudioautolayer_v8_t* const pAutoLayer(const int i) const { return reinterpret_cast<mstudioautolayer_v8_t*>((char*)this + autolayerindex) + i; }

		int weightlistindex;
		inline const float* const pBoneweight(const int i) const { return reinterpret_cast<float*>((char*)this + weightlistindex) + i; }
		inline const float weight(const int i) const { return *pBoneweight(i); }

		int posekeyindex;
		inline const float* const pPoseKey(int iParam, int iAnim) const { return reinterpret_cast<float*>((char*)this + posekeyindex) + (iParam * groupsize[0]) + iAnim; }
		inline float poseKey(int iParam, int iAnim) const { return *(pPoseKey(iParam, iAnim)); }

		int numiklocks;
		int iklockindex;
		inline const mstudioiklock_v8_t* const pIKLock(const int i) const { return reinterpret_cast<const mstudioiklock_v8_t* const>((char*)this + iklockindex) + i; }

		// Key values
		int keyvalueindex;
		int keyvaluesize;
		inline const char* const pKeyValues() const { return  keyvaluesize ? (reinterpret_cast<const char* const>(this) + keyvalueindex) : nullptr; }

		int cycleposeindex; // index of pose parameter to use as cycle index

		int activitymodifierindex;
		int numactivitymodifiers;
		inline const mstudioactivitymodifier_v8_t* const pActivityModifier(const int i) const { return reinterpret_cast<mstudioactivitymodifier_v8_t*>((char*)this + activitymodifierindex) + i; }

		int ikResetMask; // mask this ik rule type for reset, can't find the code for this, but it would either prevent reset of this type, or only allow reset of this time. only ever observed as IK_GROUND
		int unk_C4;

		// for adjusting bone weights, mostly used for procbones (jiggle)
		int weightFixupOffset;
		int weightFixupCount;
	};

	//struct mstudioposeparamdesc_v8_t
	//{
	//	int		sznameindex;
	//	inline char* const pszName() const { return ((char*)this + sznameindex); }

	//	int		flags;	// ????
	//	float	start;	// starting value
	//	float	end;	// ending value
	//	float	loop;	// looping range, 0 for no looping, 360 for rotations, etc.
	//};

	//struct mstudiomodelgroup_v8_t
	//{
	//	int	szlabelindex;	// textual name
	//	inline char* const pszLabel() const { return ((char*)this + szlabelindex); }

	//	int	sznameindex;	// file name
	//	inline char* const pszName() const { return ((char*)this + sznameindex); }
	//};


	//
	// Model Bodyparts
	// 

	struct mstudiomodel_v8_t;

	// pack here for our silly little unknown pointer
	struct mstudiomesh_v8_t
	{
		int material;

		int modelindex;
		mstudiomodel_v8_t* const pModel() const { return reinterpret_cast<mstudiomodel_v8_t*>((char*)this + modelindex); }

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

		void* unk_54; // unknown memory pointer, probably one of the older vertex pointers but moved
	};
	static_assert(sizeof(mstudiomesh_v8_t) == 0x5C);

	struct mstudiomodel_v8_t
	{
		char name[64];

		int unkStringOffset; // goes to bones sometimes

		int type;

		float boundingradius;

		int nummeshes;
		int meshindex;

		const mstudiomesh_v8_t* const pMesh(int i) const
		{
			return reinterpret_cast<mstudiomesh_v8_t*>((char*)this + meshindex) + i;
		}

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

	//struct mstudiobodyparts_v8_t
	//{
	//	int	sznameindex;
	//	inline char* const pszName() const { return ((char*)this + sznameindex); }

	//	int	nummodels;
	//	int	base;
	//	int	modelindex; // index into models array

	//	mstudiomodel_v8_t* pModel(int i) { return reinterpret_cast<mstudiomodel_v8_t*>((char*)this + modelindex) + i; };
	//};

	struct mstudiotexture_v8_t
	{
		int sznameindex;
		inline char* const pszName() const { return ((char*)this + sznameindex); }

		uint64_t texture; // guid/hash of this material
	};
	static_assert(sizeof(mstudiotexture_v8_t) == 0xC);


	//
	// Studio Header
	//

	struct studiohdr_v8_t
	{
		int id; // Model format ID, such as "IDST" (0x49 0x44 0x53 0x54)
		int version; // Format version number, such as 54 (0x36,0x00,0x00,0x00)
		int checksum; // This has to be the same in the phy and vtx files to load!
		int sznameindex; // This has been moved from studiohdr2 to the front of the main header.
		inline char* const pszName() const { return ((char*)this + sznameindex); }
		char name[64]; // The internal name of the model, padding with null chars.
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
		inline mstudiobone_v8_t* const pBone(int i) const { assert(i >= 0 && i < numbones); return reinterpret_cast<mstudiobone_v8_t*>((char*)this + boneindex) + i; }

		int numbonecontrollers; // bone controllers
		int bonecontrollerindex;

		int numhitboxsets;
		int hitboxsetindex;

		// seemingly unused now, as animations are per sequence
		int numlocalanim; // animations/poses
		int localanimindex; // animation descriptions
		inline mstudioanimdesc_v8_t* const pAnimdesc(const int i) const { assert(i >= 0 && i < numlocalanim); return reinterpret_cast<mstudioanimdesc_v8_t*>((char*)this + localanimindex) + i; }

		int numlocalseq; // sequences
		int	localseqindex;
		inline mstudioseqdesc_v8_t* const pSeqdesc(const int i) const { assert(i >= 0 && i < numlocalseq); return reinterpret_cast<mstudioseqdesc_v8_t*>((char*)this + localseqindex) + i; }

		int activitylistversion; // initialization flag - have the sequences been indexed?

		// mstudiotexture_t
		// short rpak path
		// raw textures
		// todo: stops getting written at somepoint before being outright removed, when?
		int materialtypesindex; // index into an array of MaterialShaderType_t type enums for each material used by the model
		int numtextures; // the material limit exceeds 128, probably 256.
		int textureindex;
		inline const MaterialShaderType_t pMatShaderType(int i) const { assert(i >= 0 && i < numtextures); return reinterpret_cast<MaterialShaderType_t*>((char*)this + materialtypesindex)[i]; }
		inline mstudiotexture_v8_t* const pTexture(int i) const { assert(i >= 0 && i < numtextures); return reinterpret_cast<mstudiotexture_v8_t*>((char*)this + textureindex) + i; }

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
		//inline mstudiobodyparts_v8_t* const pBodypart(int i) const { assert(i >= 0 && i < numbodyparts); return reinterpret_cast<mstudiobodyparts_v8_t*>((char*)this + bodypartindex) + i; }
		inline mstudiobodyparts_t* const pBodypart(int i) const { assert(i >= 0 && i < numbodyparts); return reinterpret_cast<mstudiobodyparts_t*>((char*)this + bodypartindex) + i; }

		int numlocalattachments;
		int localattachmentindex;

		int numlocalnodes;
		int localnodeindex;			// legacy node data
		int localnodenameindex;
		int localNodeUnk;			// used sparsely in r2, unused in apex, removed in v16 rmdl
		int localNodeDataOffset;	// offset into an array of int sized offsets that read into the data for each node (note: bad name, this should be named something different to reflect it being a new node storage type)

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
		//inline UIPanelHeader_s* const pUiPanel(int i) const { assert(i >= 0 && i < uiPanelCount); return reinterpret_cast<UIPanelHeader_s*>((char*)this + uiPanelOffset) + i; }

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

		float gatherSize;	// what. from r5r struct. no clue what this does, seemingly for early versions of apex bsp but stripped in release apex (season 0)
		// bad name, frustum culling
		// never written on disk, memory tbd

		int deprecated_numflexcontrollerui;
		int deprecated_flexcontrolleruiindex;

		float flVertAnimFixedPointScale; // to be verified
		int surfacepropLookup; // saved in the file

		// this is in most shipped models, probably part of their asset bakery.
		// doesn't actually need to be written pretty sure, only four chars when not present.
		// this is not completely true as some models simply have nothing, such as animation models.
		int sourceFilenameOffset;
		inline char* const pszSourceFiles() const { return ((char*)this + sourceFilenameOffset); }

		int numsrcbonetransform;
		int srcbonetransformindex;

		int	illumpositionattachmentindex;

		int linearboneindex;
		inline mstudiolinearbone_v8_t* const pLinearBones() const { return linearboneindex ? reinterpret_cast<mstudiolinearbone_v8_t*>((char*)this + linearboneindex) : nullptr; }

		// used for adjusting weights in sequences, quick lookup into bones that have procbones, unsure what else uses this.
		int procBoneCount;
		int procBoneOffset; // in order array of procbones and their parent bone indice
		int linearProcBoneOffset; // byte per bone with indices into each bones procbone, 0xff if no procbone is present

		// depreciated as they are removed in 12.1
		int deprecated_m_nBoneFlexDriverCount;
		int deprecated_m_nBoneFlexDriverIndex;

		int deprecated_m_nPerTriAABBIndex;
		int deprecated_m_nPerTriAABBNodeCount;
		int deprecated_m_nPerTriAABBLeafCount;
		int deprecated_m_nPerTriAABBVertCount;

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

		// // removed from header in version 12.1
		int deprecated_collisionOffset;			// deprecated_imposterIndex
		int deprecated_staticCollisionCount;	// deprecated_numImposters

		// mostly seen on '_animated' suffixed models
		// manually declared bone followers are no longer stored in kvs under 'bone_followers', they are now stored in an array of ints with the bone index.
		int boneFollowerCount;
		int boneFollowerOffset;
		inline const int* const pBoneFollowers(int i) const { assert(i >= 0 && i < boneFollowerCount); return reinterpret_cast<int*>((char*)this + boneFollowerOffset) + i; }
		inline const int BoneFollower(int i) const { return *pBoneFollowers(i); }

		// BVH4 size (?)
		Vector bvhMin;
		Vector bvhMax; // seem to be the same as hull size

		int unk_208[3]; // never written on disk, memory tbd

		int bvhOffset; // bvh4 tree

		short bvhUnk[2]; // collision detail for bvh (?)

		// new in apex vertex weight file for verts that have more than three weights
		// vvw is a 'fake' extension name, we do not know the proper name.
		int vvwOffset; // index will come last after other vertex files
		int vvwSize;
	};
};
#pragma pack(pop)

namespace r5
{
	int GetAnimValueOffset(const mstudioanimvalue_t* const panimvalue);
	void ExtractAnimValue(int frame, const mstudioanimvalue_t* panimvalue, float scale, float& v1);
	void ExtractAnimValue(int frame, const mstudioanimvalue_t* panimvalue, float scale, float& v1, float& v2);

	void CalcBoneQuaternion(int frame, float s, const mstudio_rle_anim_t* panim, const RadianEuler& baseRot, Quaternion& q, const uint8_t boneFlags);
	void CalcBonePosition(int frame, float s, const mstudio_rle_anim_t* panim, Vector& pos);
	void CalcBoneScale(int frame, float s, const mstudio_rle_anim_t* panim, Vector& scale, const uint8_t boneFlags);

	// uses 'datapoints' and interpolates from them
	void CalcBoneQuaternion_DP(int sectionlength, const uint8_t** panimtrack, float fFrame, Quaternion& q);
	void CalcBonePosition_DP(int sectionlength, const uint8_t** panimtrack, float fFrame, Vector& pos);
	void CalcBonePositionVirtual_DP(const int sectionlength, const uint8_t** panimtrack, const float fFrame, Vector& pos);
	void CalcBoneScale_DP(const int sectionlength, const uint8_t** panimtrack, const float fFrame, Vector& scale);

	template<class PackedType>
	__forceinline void CalcBoneSeek_DP(const PackedType* const pPackedData, int& validIdx, uint32_t& remainingFrames, const uint32_t targetFrame)
	{
		// [rika]: seek to the correct quaterion index for this frame
		/*if (pPackedData[validIdx].numInterpFrames < static_cast<uint32_t>(prevFrame))
		{
			// todo function
			uint32_t numInterpFrames = pPackedData[validIdx].numInterpFrames;

			// [rika]: does this quaternion cover our desired frame
			do
			{
				validIdx++;
				assertm(validIdx < valid, "invalid index");

				remainingFrames += -1 - numInterpFrames;
				numInterpFrames = pPackedData[validIdx].numInterpFrames;

			} while (numInterpFrames < remainingFrames);
		}*/

		/*uint32_t numInterpFrames = pPackedData[validIdx].numInterpFrames;
		for (uint32_t i = nextFrame + remainingFrames - prevFrame; numInterpFrames < i; numInterpFrames = pPackedData[validIdx].numInterpFrames)
		{
			++validIdx;
			assertm(validIdx < valid, "invalid index");

			i += -1 - numInterpFrames;
		}*/

		remainingFrames = targetFrame;

		// [rika]: seek to the correct data index for this frame
		if (pPackedData[validIdx].numInterpFrames < targetFrame)
		{
			uint32_t numInterpFrames = pPackedData[validIdx].numInterpFrames;

			// [rika]: does this quaternion cover our desired frame
			while (numInterpFrames < remainingFrames)
			{
				validIdx++;
				//assertm(validIdx < valid, "invalid index");

				remainingFrames += -1 - numInterpFrames;
				numInterpFrames = pPackedData[validIdx].numInterpFrames;

			}
		}
	}

	template<class IndexType, class PackedType>
	__forceinline void CalcBoneInterpFrames_DP(int& prevFrame, int& nextFrame, float& s, const float fFrame, const IndexType total, const int sectionlength, const IndexType* const pFrameIndices, const PackedType** const pPackedData)
	{
		if (total >= sectionlength)
		{
			const int maxFrameIndex = sectionlength - 1; // the max index of any frame in this section

			// [rika]: if the frame is interpolated, the truncated value would be the previous frame
			prevFrame = static_cast<int>(fminf(truncf(fFrame), (static_cast<float>(sectionlength) - 1.0f)));

			// [rika]: extract the 's' or interp value from fFrameInterp value
			s = fFrame - static_cast<float>(prevFrame);

			nextFrame = prevFrame + 1; // the next frame would be the next indice

			// [rika]: if the interp is too small, or the previous frame is the last frame in our section, the next frame should be equal to previous frame
			if (s < 0.00000011920929 || prevFrame == maxFrameIndex)
				nextFrame = prevFrame;

			// [rika]: set the pointers for our data
			*pPackedData = reinterpret_cast<const PackedType* const>(pFrameIndices);
			return;
		}
		else
		{
			prevFrame = 0;
			nextFrame = total - 1;

			// [rika]: pretty sure this is thinning down frames
			// [rika]: only do this if we have more than two frames
			if (nextFrame > 1)
			{
				while (true)
				{
					if (1 >= nextFrame - prevFrame)
						break;

					// [rika]: this should be the frame between our two end points
					const int interpFrame = (nextFrame + prevFrame) >> 1;
					const float fIndice = static_cast<float>(pFrameIndices[interpFrame]);

					if (fIndice <= fFrame)
					{
						prevFrame = interpFrame; // move the previous frame closer to the next frame

						if (fFrame > fIndice)
							continue;
					}

					nextFrame = interpFrame; // move the next frame closer to the previous frame
				}
			}

			const float prevFrameIndice = static_cast<float>(pFrameIndices[prevFrame]);
			const float nextFrameIndice = static_cast<float>(pFrameIndices[nextFrame]);

			if (prevFrameIndice > fFrame)
			{
				nextFrame = prevFrame;
				s = 0.0f;
			}
			else if (fFrame <= nextFrameIndice)
			{
				// [rika]: same frame, don't interp
				if (prevFrame == nextFrame)
					s = 0.0f;
				else
					s = (fFrame - prevFrameIndice) / static_cast<float>(pFrameIndices[nextFrame] - pFrameIndices[prevFrame]);

				/*if (prevFrame != nextFrame)
					s = (fFrame - prevFrameIndice) / static_cast<float>(pFrameIndices[nextFrame] - pFrameIndices[prevFrame]);*/
			}
			else
			{
				prevFrame = nextFrame;
				s = 0.0f;
			}

			// [rika]: set the pointers for our data
			*pPackedData = reinterpret_cast<const PackedType* const>(pFrameIndices + total);
			return;
		}
	}
}