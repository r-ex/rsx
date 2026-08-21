#pragma once
#include <game/rtech/utils/utils.h> // eCompressionType

#pragma pack(push, 2)
namespace r5
{
	//
	// VERSION 16
	//

	//
	// Model HWDATA
	//

	struct studio_hw_groupdata_v16_t
	{
		int dataOffset; // offset to this section in compressed vg
		int dataSizeCompressed; // compressed size of data in vg
		int dataSizeDecompressed; // decompressed size of data in vg
		eCompressionType dataCompression; // compressionType_t

		uint8_t lodIndex;
		uint8_t lodCount;	// number of lods contained within this group
		uint8_t lodMap;		// lods in this group, each bit is a lod
	};


	//
	// Model Bones
	//

	struct mstudiobonehdr_v16_t
	{
		int contents; // See BSPFlags.h for the contents flags

		uint8_t unk_4;

		uint8_t surfacepropLookup; // written on compile in v54
		uint16_t surfacepropidx; // index into string table for property name

		uint16_t physicsbone; // index into physically simulated bone

		uint16_t sznameindex;
		inline char* const pszName() const { return ((char*)this + FIX_OFFSET(sznameindex)); }
	};

	struct mstudiobonedata_v16_t
	{
		matrix3x4_t poseToBone;
		Quaternion qAlignment;

		// default values
		Vector pos;
		Quaternion quat;
		RadianEuler rot;
		Vector scale; // bone scale(?)

		short parent; // parent bone;

		uint16_t unk_76; // gotta be used because there's to 8 bit vars that could've fit here, still may be packing. previously 'unk1'

		int flags;

		uint8_t collisionIndex; // index into sections of collision, phy, bvh, etc. needs confirming

		uint8_t proctype;
		uint16_t procindex; // procedural rule
	};

	struct mstudiolinearbone_v16_t
	{
		// they cut pos and rot scale, understandable since posscale was never used it tf|2 and they do anims different in apex
		uint16_t numbones;

		uint16_t flagsindex;
		inline int* pFlags(int i) const { assert(i >= 0 && i < numbones); return reinterpret_cast<int*>((char*)this + FIX_OFFSET(flagsindex)) + i; }
		inline int flags(int i) const { return *pFlags(i); }

		uint16_t parentindex;
		inline int* pParent(int i)
			const {
			assert(i >= 0 && i < numbones);
			return reinterpret_cast<int*>((char*)this + FIX_OFFSET(parentindex)) + i;
		}

		uint16_t posindex;
		inline const Vector* pPos(int i)
			const {
			assert(i >= 0 && i < numbones);
			return reinterpret_cast<Vector*>((char*)this + FIX_OFFSET(posindex)) + i;
		}

		uint16_t quatindex;
		inline const Quaternion* pQuat(int i)
			const {
			assert(i >= 0 && i < numbones);
			return reinterpret_cast<Quaternion*>((char*)this + FIX_OFFSET(quatindex)) + i;
		}

		uint16_t rotindex;
		inline const RadianEuler* pRot(int i)
			const {
			assert(i >= 0 && i < numbones);
			return reinterpret_cast<RadianEuler*>((char*)this + FIX_OFFSET(rotindex)) + i;
		}

		uint16_t posetoboneindex;
		inline const matrix3x4_t* pPoseToBone(int i)
			const {
			assert(i >= 0 && i < numbones);
			return reinterpret_cast<matrix3x4_t*>((char*)this + FIX_OFFSET(posetoboneindex)) + i;
		}
	};

	struct mstudioattachment_v16_t
	{
		uint16_t sznameindex;
		inline const char* const pszName() const { return reinterpret_cast<const char* const>(this) + FIX_OFFSET(sznameindex); }
		uint16_t localbone; // parent bone
		int flags;

		matrix3x4_t local; // attachment point
	};

	struct mstudiobbox_v16_t
	{
		uint16_t bone;
		uint16_t group; // intersection group

		Vector bbmin; // bounding box
		Vector bbmax;

		uint16_t szhitboxnameindex; // offset to the name of the hitbox.
		inline const char* const pszHitboxName() const
		{
			if (szhitboxnameindex == 0)
				return "";

			return reinterpret_cast<const char* const>(this) + FIX_OFFSET(szhitboxnameindex);
		}

		uint16_t hitdataGroupOffset;	// hit_data group in keyvalues this hitbox uses.
	};

	struct mstudiohitboxset_v16_t
	{
		uint16_t sznameindex;
		inline const char* const pszName() const { return reinterpret_cast<const char* const>(this) + FIX_OFFSET(sznameindex); }

		uint16_t numhitboxes;
		uint16_t hitboxindex;
		const mstudiobbox_v16_t* const pHitbox(const int i) const { return reinterpret_cast<const mstudiobbox_v16_t* const>((char*)this + FIX_OFFSET(hitboxindex)) + i; }
	};


	//
	// Model IK Info
	//

	struct mstudioiklock_v16_t
	{
		uint16_t chain;
		uint16_t flags; // possibly unused/padding now
		float flPosWeight;
		float flLocalQWeight;
	};

	struct mstudioiklink_v16_t
	{
		uint16_t bone;
		uint16_t pad;
		Vector kneeDir;
	};

	struct mstudioikchain_v16_t
	{
		uint16_t sznameindex;
		inline const char* const pszName() const { return reinterpret_cast<const char* const>(this) + FIX_OFFSET(sznameindex); }

		uint16_t linktype;
		uint16_t numlinks;
		uint16_t linkindex;
		inline const mstudioiklink_v16_t* const pLink(const int i) const { return reinterpret_cast<const mstudioiklink_v16_t* const>((const char*)this + FIX_OFFSET(linkindex)) + i; }

		float unk_10;	// tried, and tried to find what this does, but it doesn't seem to get hit in any normal IK code paths
		// however, in Apex Legends this value is 0.866 which happens to be cos(30) (https://github.com/NicolasDe/AlienSwarm/blob/master/src/public/studio.h#L2173)
		// and in Titanfall 2 it's set to 0.707 or alternatively cos(45).
		// TLDR: likely set in QC $ikchain command from a degrees entry, or set with a default value in studiomdl when not defined.
	};


	//
	// Model Animation
	//

	struct mstudiocompressedikerror_v16_t
	{
		uint16_t sectionframes;
		float scale[6];
	};
	static_assert(sizeof(mstudiocompressedikerror_v16_t) == 0x1A);

	// 'index' member removed, as it was unused anywhere.
	struct mstudioikrule_v16_t
	{
		short chain;
		short bone;
		char type;
		char slot;

		mstudiocompressedikerror_v16_t compressedikerror;
		uint16_t compressedikerrorindex;
		inline const int SectionCount(const int numframes) const { return (numframes - 1) / compressedikerror.sectionframes + 1; } // this might be wrong :(
		inline int SectionIndex(const int frame) const { return frame / compressedikerror.sectionframes; } // get the section index for this frame
		inline uint16_t SectionOffset(const int frame) const { return reinterpret_cast<uint16_t*>((char*)this + compressedikerrorindex)[SectionIndex(frame)]; }
		inline uint16_t* pSection(const int frame) const { return reinterpret_cast<uint16_t*>((char*)this + SectionOffset(frame)); }

		inline mstudioanimvalue_t* pAnimvalue(const int i, const uint16_t* section) const { return (section[i] > 0) ? reinterpret_cast<mstudioanimvalue_t*>((char*)section + section[i]) : nullptr; }

		short iStart;
		uint16_t ikerrorindex;

		uint16_t szattachmentindex; // name of world attachment
		inline const char* const pszAttachment() const { return szattachmentindex ? reinterpret_cast<const char* const>(this) + FIX_OFFSET(szattachmentindex) : nullptr; }

		float start; // beginning of influence
		float peak; // start of full influence
		float tail; // end of full influence
		float end; // end of all influence

		float contact; // frame footstep makes ground concact
		float drop; // how far down the foot should drop when reaching for IK
		float top; // top of the foot box

		float height;
		float endHeight;
		float radius;
		float floor;

		// these are not used, but fit here perfectly so.
		Vector pos;
		Quaternion q;
	};
	static_assert(sizeof(mstudioikrule_v16_t) == 0x70);

	struct mstudioanimsections_v16_t
	{
		int animindex;  // negative bit set if external
		inline const bool isExternal() const { return animindex < 0; } // check if negative
		inline const int Index() const { return isExternal() ? ~animindex : animindex; } // flip bits if negative
	};

	struct mstudioanimdesc_v16_t
	{
		float fps; // frames per second	
		int flags; // looping/non-looping flags

		int numframes;

		uint16_t sznameindex;
		inline const char* const pszName() const { return reinterpret_cast<const char* const>(this) + FIX_OFFSET(sznameindex); }

		uint16_t framemovementindex; // new in v52
		inline const mstudioframemovement_t* const pFrameMovement() const { return reinterpret_cast<const mstudioframemovement_t* const>((char*)this + FIX_OFFSET(framemovementindex)); }

		int animindex; // non-zero when anim data isn't in sections
		//char* pAnim(int* piFrame) const; // returns pointer to data and new frame index

		uint16_t numikrules;
		uint16_t ikruleindex; // non-zero when IK data is stored in the mdl
		inline const mstudioikrule_v16_t* const pIKRule(const int i) const;

		char* sectionDataExternal; // set on pak asset load
		uint16_t unk1; // maybe some sort of thread/mutic for the external data? set on pak asset load from unk_10

		uint16_t sectionindex;
		uint16_t sectionstallframes; // number of stall frames inside the animation, the reset excluding the final frame are stored externally. when external data is not loaded(?)/found(?) it falls back on the last frame of this as a stall
		uint16_t sectionframes; // number of frames used in each fast lookup section, zero if not used
		inline const mstudioanimsections_v16_t* const pSection(int i) const { return reinterpret_cast<mstudioanimsections_v16_t*>((char*)this + FIX_OFFSET(sectionindex)) + i; }
		// numsections = ((numframes - sectionstallframes - 1) / sectionframes) + 2;
		// numsections after stall section, if stall frames > zero add one section
	};

	struct mstudioevent_v16_t
	{
		float cycle;
		int	event;
		int type; // no flag if using 'event' var with event id

		int unk_C;

		// bad names, "optionsOffset" and "eventOffset"?
		uint16_t optionsindex;
		uint16_t szeventindex;
		inline const char* const pszOptions() const { return (reinterpret_cast<const char* const>(this) + FIX_OFFSET(optionsindex)); }
		inline const char* const pszEvent() const { return (reinterpret_cast<const char* const>(this) + FIX_OFFSET(szeventindex)); }
	};

	struct mstudioactivitymodifier_v16_t
	{
		uint16_t sznameindex;
		bool negate; // 0 or 1 observed.

		inline const char* const pszName() const { return (reinterpret_cast<const char* const>(this) + FIX_OFFSET(sznameindex)); }
	};

	struct mstudioseqdesc_v16_t
	{
		uint16_t szlabelindex;
		inline const char* const pszLabel() const { return reinterpret_cast<const char* const>(this) + FIX_OFFSET(szlabelindex); }

		uint16_t szactivitynameindex;
		inline const char* const pszActivityName() const { return reinterpret_cast<const char* const>(this) + FIX_OFFSET(szactivitynameindex); }

		int flags; // looping/non-looping flags

		uint16_t activity; // initialized at loadtime to game DLL values
		uint16_t actweight;

		uint16_t numevents;
		uint16_t eventindex;
		inline const mstudioevent_v16_t* const pEvent(const int i) const { return reinterpret_cast<mstudioevent_v16_t*>((char*)this + FIX_OFFSET(eventindex)) + i; }

		Vector bbmin; // per sequence bounding box
		Vector bbmax;

		uint16_t numblends;

		// Index into array of shorts which is groupsize[0] x groupsize[1] in length
		uint16_t animindexindex;
		const int AnimIndex(const uint16_t i) const { return FIX_OFFSET(reinterpret_cast<const uint16_t* const>((const char* const)this + FIX_OFFSET(animindexindex))[i]); }
		const int AnimCount() const { return  groupsize[0] * groupsize[1]; }
		mstudioanimdesc_v16_t* pAnimDesc(const uint16_t i) const { return reinterpret_cast<mstudioanimdesc_v16_t*>((char*)this + AnimIndex(i)); }

		short paramindex[2]; // X, Y, Z, XR, YR, ZR
		float paramstart[2]; // local (0..1) starting value
		float paramend[2]; // local (0..1) ending value

		float fadeintime; // ideal cross fate in time (0.2 default)
		float fadeouttime; // ideal cross fade out time (0.2 default)

		// animseq/humans/class/medium/pilot_medium_bangalore/bangalore_freefall.rseq
		uint16_t localentrynode; // transition node at entry
		uint16_t localexitnode; // transition node at exit

		uint16_t numikrules;

		uint16_t numautolayers;
		uint16_t autolayerindex;
		inline const mstudioautolayer_v8_t* const pAutoLayer(const int i) const { return reinterpret_cast<mstudioautolayer_v8_t*>((char*)this + FIX_OFFSET(autolayerindex)) + i; }

		uint16_t weightlistindex;
		inline const float* const pBoneweight(const int i) const { return reinterpret_cast<float*>((char*)this + FIX_OFFSET(weightlistindex)) + i; }
		inline const float weight(const int i) const { return *pBoneweight(i); }

		uint8_t groupsize[2]; // width x height of blends

		// animseq/humans/class/medium/mp_pilot_medium_core/medium_bow_charge_pose.rseq
		uint16_t posekeyindex;
		inline const float* const pPoseKey(int iParam, int iAnim) const { return reinterpret_cast<float*>((char*)this + FIX_OFFSET(posekeyindex)) + (iParam * groupsize[0]) + iAnim; }
		inline float poseKey(int iParam, int iAnim) const { return *(pPoseKey(iParam, iAnim)); }

		// never used?
		uint16_t numiklocks;
		uint16_t iklockindex;
		inline const mstudioiklock_v16_t* const pIKLock(const int i) const { return reinterpret_cast<const mstudioiklock_v16_t* const>((char*)this + FIX_OFFSET(iklockindex)) + i; }

		// whar
		uint16_t unk_5C;

		// animseq/weapons/octane_epipen/ptpov_octane_epipen_held/drain_fluid_layer.rseq
		uint16_t cycleposeindex; // index of pose parameter to use as cycle index

		uint16_t activitymodifierindex;
		uint16_t numactivitymodifiers;
		inline const mstudioactivitymodifier_v16_t* const pActivityModifier(const int i) const { return reinterpret_cast<mstudioactivitymodifier_v16_t*>((char*)this + FIX_OFFSET(activitymodifierindex)) + i; }

		int ikResetMask; // mask this ik rule type for reset, can't find the code for this, but it would either prevent reset of this type, or only allow reset of this time. only ever observed as IK_GROUND
		int unk_68;	// unk_C4

		uint16_t weightFixupOffset;
		uint16_t weightFixupCount;
	};

	struct mstudioposeparamdesc_v16_t
	{
		uint16_t	sznameindex;
		inline const char* const pszName() const { return reinterpret_cast<const char* const>(this) + FIX_OFFSET(sznameindex); }

		uint16_t	flags;		// ???? (may contain 'STUDIO_LOOPING' flag if looping is used)
		float		start;		// starting value
		float		end;		// ending value
		float		loop;		// looping range, 0 for no looping, 360 for rotations, etc.
	};


	//
	// Model Bodyparts
	//

	struct mstudiomesh_v16_t
	{
		uint16_t material;

		// a unique ordinal for this mesh
		uint16_t meshid;

		char unk_4[4];

		Vector center;
	};

	struct mstudiomodel_v16_t
	{
		uint16_t unkStringOffset; // goes to bones sometimes; 'small' 'large'
		inline char* const pszString() const { return ((char*)this + FIX_OFFSET(unkStringOffset)); }

		uint16_t meshCountTotal; // total number of meshes in this mesh
		uint16_t meshCountBase; // number of normal meshes in this model
		uint16_t meshCountBlend; // number of blend shape meshes, todo how does this play with more than one base mesh?
		uint16_t meshOffset;
		inline mstudiomesh_v16_t* const pMesh(const uint16_t meshIdx) const { return reinterpret_cast<mstudiomesh_v16_t*>((char*)this + FIX_OFFSET(meshOffset)) + meshIdx; }
	};

	struct mstudiobodyparts_v16_t
	{
		uint16_t sznameindex;
		inline char* const pszName() const { return ((char*)this + FIX_OFFSET(sznameindex)); }

		uint16_t modelindex;
		inline mstudiomodel_v16_t* const pModel(const uint16_t modelIdx) const { return reinterpret_cast<mstudiomodel_v16_t*>((char*)this + FIX_OFFSET(modelindex)) + modelIdx; }

		int base;
		int nummodels;
		int meshOffset; // might be short
	};

	struct mstudiotexture_v16_t
	{
		uint64_t texture; // hash of material path
	};


	//
	// Studio Header
	//

	struct studiohdr_v16_t
	{
		int flags;
		int checksum; // unsure if this is still checksum, there isn't any other files that have it still
		uint16_t sznameindex; // No longer stored in string block, uses string in header.
		inline const char* const pszName() const { return reinterpret_cast<const char* const>(this) + FIX_OFFSET(sznameindex); }
		char name[33]; // The internal name of the model, padding with null chars. last byte always null

		uint8_t surfacepropLookup; // saved in the file (unsigned dl/fx/ferrofluid_ult_base_puddle)

		float mass;

		int contents;

		uint16_t hitboxsetindex;
		uint8_t numhitboxsets;

		uint8_t illumpositionattachmentindex;

		Vector illumposition;	// illumination center

		Vector hull_min;		// ideal movement hull size
		Vector hull_max;

		Vector view_bbmin;		// clipping bounding box
		Vector view_bbmax;

		uint16_t boneCount; // bones
		uint16_t boneHdrOffset;
		uint16_t boneDataOffset;

		uint16_t numlocalseq; // sequences
		uint16_t localseqindex;

		// needs to be confirmed
		uint16_t unk_7E[2]; // added in v13 -> v14

		// needs to be confirmed
		char activitylistversion; // initialization flag - have the sequences been indexed?

		uint8_t numlocalattachments;
		uint16_t localattachmentindex;

		uint16_t numlocalnodes;
		uint16_t localnodenameindex;
		uint16_t localNodeDataOffset; // offset into an array of int sized offsets that read into the data for each node

		uint16_t numikchains;
		uint16_t ikchainindex;

		uint16_t numtextures; // the material limit exceeds 128, probably 256.
		uint16_t textureindex;

		// replaceable textures tables
		uint16_t numskinref;
		uint16_t numskinfamilies;
		uint16_t skinindex;

		uint16_t numbodyparts;
		uint16_t bodypartindex;
		inline const mstudiobodyparts_v16_t* const pBodypart(const uint16_t i) const { assert(i >= 0 && i < numbodyparts); return reinterpret_cast<mstudiobodyparts_v16_t*>((char*)this + FIX_OFFSET(bodypartindex)) + i; }

		// this is rui meshes
		uint16_t uiPanelCount;
		uint16_t uiPanelOffset;

		uint16_t numlocalposeparameters;
		uint16_t localposeparamindex;

		uint16_t surfacepropindex;

		uint16_t keyvalueindex;

		uint16_t virtualModel;

		// hw data lookup from rmdl
		uint16_t meshCount; // number of meshes per lod

		uint16_t bonetablebynameindex;

		uint16_t boneStateOffset;
		uint16_t boneStateCount;
		inline const uint8_t* pBoneStates() const { return boneStateCount > 0 ? reinterpret_cast<uint8_t*>((char*)this + offsetof(studiohdr_v16_t, boneStateOffset) + FIX_OFFSET(boneStateOffset)) : nullptr; }

		// sets of lods
		uint16_t groupHeaderOffset;
		uint16_t groupHeaderCount;
		const studio_hw_groupdata_v16_t* const pLODGroup(const uint16_t i) const { return reinterpret_cast<const studio_hw_groupdata_v16_t* const>((char*)this + offsetof(studiohdr_v16_t, groupHeaderOffset) + FIX_OFFSET(groupHeaderOffset)) + i; }

		uint16_t lodOffset;
		uint16_t lodCount;
		const float* const pLODThreshold(const uint16_t i) const { return reinterpret_cast<const float* const>((char*)this + offsetof(studiohdr_v16_t, lodOffset) + FIX_OFFSET(lodOffset)) + i; }
		const float LODThreshold(const uint16_t i) const { return *pLODThreshold(i); }

		// 
		float fadeDistance;
		float gatherSize; // what. from r5r struct

		uint16_t numsrcbonetransform;
		uint16_t srcbonetransformindex;

		// asset bakery strings if it has any
		uint16_t sourceFilenameOffset;

		uint16_t linearboneindex;

		// used for adjusting weights in sequences, quick lookup into bones that have procbones, unsure what else uses this.
		uint16_t procBoneCount;
		uint16_t procBoneOffset; // in order array of procbones and their parent bone indice
		uint16_t linearProcBoneOffset; // byte per bone with indices into each bones procbone, 0xff if no procbone is present

		// mostly seen on '_animated' suffixed models
		// manually declared bone followers are no longer stored in kvs under 'bone_followers', they are now stored in an array of ints with the bone index.
		uint16_t boneFollowerCount;
		uint16_t boneFollowerOffset;

		uint16_t bvhOffset;

		char bvhUnk[2]; // collision detail for bvh (?)

		// perhaps these are t he same varibles added in v12.3? cannot find any models that use them previously (pre-v16).
		// UnkDataType_0_t
		uint16_t unkDataCount; // unk_0xDA
		uint16_t unkDataOffset; // unk_0xDC
		// UnkDataType_1_t
		uint16_t unkStrcOffset; // unk_0xDE
	};


	//
	// VERSION 17
	//

	//
	// Studio Header
	//

	struct studiohdr_v17_t
	{
		int flags;
		int checksum; // unsure if this is still checksum, there isn't any other files that have it still
		uint16_t sznameindex; // No longer stored in string block, uses string in header.
		inline const char* const pszName() const { return reinterpret_cast<const char* const>(this) + FIX_OFFSET(sznameindex); }
		char name[33]; // The internal name of the model, padding with null chars. last byte always null

		uint8_t surfacepropLookup; // saved in the file (unsigned dl/fx/ferrofluid_ult_base_puddle)

		float mass;

		int contents;

		uint16_t hitboxsetindex;
		uint8_t numhitboxsets;

		uint8_t illumpositionattachmentindex;

		Vector illumposition;	// illumination center

		Vector hull_min;		// ideal movement hull size
		Vector hull_max;

		Vector view_bbmin;		// clipping bounding box
		Vector view_bbmax;

		uint16_t boneCount; // bones
		uint16_t boneHdrOffset;
		uint16_t boneDataOffset;

		uint16_t numlocalseq; // sequences
		uint16_t localseqindex;

		// needs to be confirmed
		uint16_t unk_7E[2]; // added in v13 -> v14

		// needs to be confirmed
		char activitylistversion; // initialization flag - have the sequences been indexed?

		uint8_t numlocalattachments;
		uint16_t localattachmentindex;

		uint16_t numlocalnodes;
		uint16_t localnodenameindex;
		uint16_t localNodeDataOffset; // offset into an array of int sized offsets that read into the data for each node

		uint16_t numikchains;
		uint16_t ikchainindex;

		uint16_t numtextures; // the material limit exceeds 128, probably 256.
		uint16_t textureindex;

		// replaceable textures tables
		uint16_t numskinref;
		uint16_t numskinfamilies;
		uint16_t skinindex;

		uint16_t numbodyparts;
		uint16_t bodypartindex;
		inline const mstudiobodyparts_v16_t* const pBodypart(const uint16_t i) const { assert(i >= 0 && i < numbodyparts); return reinterpret_cast<mstudiobodyparts_v16_t*>((char*)this + FIX_OFFSET(bodypartindex)) + i; }

		// this is rui meshes
		uint16_t uiPanelCount;
		uint16_t uiPanelOffset;

		uint16_t numlocalposeparameters;
		uint16_t localposeparamindex;

		uint16_t surfacepropindex;

		uint16_t keyvalueindex;

		uint16_t virtualModel;

		// hw data lookup from rmdl
		uint16_t meshCount; // number of meshes per lod

		uint16_t bonetablebynameindex;

		uint16_t boneStateOffset;
		uint16_t boneStateCount;
		inline const uint8_t* pBoneStates() const { return boneStateCount > 0 ? reinterpret_cast<uint8_t*>((char*)this + offsetof(studiohdr_v16_t, boneStateOffset) + FIX_OFFSET(boneStateOffset)) : nullptr; }

		// sets of lods
		uint16_t groupHeaderOffset;
		uint16_t groupHeaderCount;
		const studio_hw_groupdata_v16_t* const pLODGroup(const uint16_t i) const { return reinterpret_cast<const studio_hw_groupdata_v16_t* const>((char*)this + offsetof(studiohdr_v16_t, groupHeaderOffset) + FIX_OFFSET(groupHeaderOffset)) + i; }

		uint16_t lodOffset;
		uint16_t lodCount;
		const float* const pLODThreshold(const uint16_t i) const { return reinterpret_cast<const float* const>((char*)this + offsetof(studiohdr_v16_t, lodOffset) + FIX_OFFSET(lodOffset)) + i; }
		const float LODThreshold(const uint16_t i) const { return *pLODThreshold(i); }

		// 
		float fadeDistance;
		float gatherSize; // what. from r5r struct

		uint16_t numsrcbonetransform;
		uint16_t srcbonetransformindex;

		// asset bakery strings if it has any
		uint16_t sourceFilenameOffset;

		uint16_t linearboneindex;

		// used for adjusting weights in sequences, quick lookup into bones that have procbones, unsure what else uses this.
		uint16_t procBoneCount;
		uint16_t procBoneOffset; // in order array of procbones and their parent bone indice
		uint16_t linearProcBoneOffset; // byte per bone with indices into each bones procbone, 0xff if no procbone is present

		// mostly seen on '_animated' suffixed models
		// manually declared bone followers are no longer stored in kvs under 'bone_followers', they are now stored in an array of ints with the bone index.
		uint16_t boneFollowerCount;
		uint16_t boneFollowerOffset;

		uint16_t bvhOffset;

		char bvhUnk[2]; // collision detail for bvh (?)

		// perhaps these are t he same varibles added in v12.3? cannot find any models that use them previously (pre-v16).
		// UnkDataType_0_t
		uint16_t unkDataCount; // unk_0xDA
		uint16_t unkDataOffset; // unk_0xDC
		// UnkDataType_1_t
		uint16_t unkStrcOffset; // unk_0xDE

		int unk_E0;
	};


	//
	// VERSION 18
	//

	//
	// Model Animation
	//

	struct mstudio_nointerpframes_t
	{
		int firstFrame;
		int lastFrame;
	};

	struct mstudioanimdesc_v19_1_t;
	struct mstudioseqdesc_v18_t
	{
		uint16_t szlabelindex;
		inline const char* const pszLabel() const { return reinterpret_cast<const char* const>(this) + FIX_OFFSET(szlabelindex); }

		uint16_t szactivitynameindex;
		inline const char* const pszActivityName() const { return reinterpret_cast<const char* const>(this) + FIX_OFFSET(szactivitynameindex); }

		int flags; // looping/non-looping flags

		uint16_t activity; // initialized at loadtime to game DLL values
		uint16_t actweight;

		uint16_t numevents;
		uint16_t eventindex;
		inline const mstudioevent_v16_t* const pEvent(const int i) const { return reinterpret_cast<mstudioevent_v16_t*>((char*)this + FIX_OFFSET(eventindex)) + i; }

		Vector bbmin; // per sequence bounding box
		Vector bbmax;

		uint16_t numblends;

		// Index into array of shorts which is groupsize[0] x groupsize[1] in length
		uint16_t animindexindex;
		const int AnimIndex(const uint16_t i) const { return FIX_OFFSET(reinterpret_cast<const uint16_t* const>((const char* const)this + FIX_OFFSET(animindexindex))[i]); }
		const int AnimCount() const { return  groupsize[0] * groupsize[1]; }
		const mstudioanimdesc_v16_t* const pAnimDesc(const uint16_t i) const { return reinterpret_cast<const mstudioanimdesc_v16_t* const>((char*)this + AnimIndex(i)); }
		const mstudioanimdesc_v19_1_t* const pAnimDesc_V19_1(const uint16_t i) const;

		short paramindex[2]; // X, Y, Z, XR, YR, ZR
		float paramstart[2]; // local (0..1) starting value
		float paramend[2]; // local (0..1) ending value

		float fadeintime; // ideal cross fate in time (0.2 default)
		float fadeouttime; // ideal cross fade out time (0.2 default)

		// animseq/humans/class/medium/pilot_medium_bangalore/bangalore_freefall.rseq
		uint16_t localentrynode; // transition node at entry
		uint16_t localexitnode; // transition node at exit

		uint16_t numikrules;

		uint16_t numautolayers;
		uint16_t autolayerindex;
		inline const mstudioautolayer_v8_t* const pAutoLayer(const int i) const { return reinterpret_cast<mstudioautolayer_v8_t*>((char*)this + FIX_OFFSET(autolayerindex)) + i; }

		uint16_t weightlistindex;
		inline const float* const pWeightList() const;
		inline const float* const pBoneweight(const int i) const { return pWeightList() + i; }
		inline const float weight(const int i) const { return *pBoneweight(i); }

		uint8_t groupsize[2]; // width x height of blends

		// animseq/humans/class/medium/mp_pilot_medium_core/medium_bow_charge_pose.rseq
		uint16_t posekeyindex;
		inline const float* const pPoseKey(int iParam, int iAnim) const { return reinterpret_cast<float*>((char*)this + FIX_OFFSET(posekeyindex)) + (iParam * groupsize[0]) + iAnim; }
		inline float poseKey(int iParam, int iAnim) const { return *(pPoseKey(iParam, iAnim)); }

		// never used?
		uint16_t numiklocks;
		uint16_t iklockindex;
		inline const mstudioiklock_v16_t* const pIKLock(const int i) const { return reinterpret_cast<const mstudioiklock_v16_t* const>((char*)this + FIX_OFFSET(iklockindex)) + i; }

		// whar
		uint16_t unk_5C;

		// animseq/weapons/octane_epipen/ptpov_octane_epipen_held/drain_fluid_layer.rseq
		uint16_t cycleposeindex; // index of pose parameter to use as cycle index

		uint16_t activitymodifierindex;
		uint16_t numactivitymodifiers;
		inline const mstudioactivitymodifier_v16_t* const pActivityModifier(const int i) const { return reinterpret_cast<mstudioactivitymodifier_v16_t*>((char*)this + FIX_OFFSET(activitymodifierindex)) + i; }

		int ikResetMask; // mask this ik rule type for reset, can't find the code for this, but it would either prevent reset of this type, or only allow reset of this time. only ever observed as IK_GROUND
		int unk_68;	// unk_C4

		uint16_t weightFixupOffset;
		uint16_t weightFixupCount;

		// disables interp for certain frames (from what I can tell)
		uint16_t noInterpFrameOffset;
		uint16_t noInterpFrameCount;
		const int NoInterpFrameSingle(const int i) const { return reinterpret_cast<int*>((char*)this + FIX_OFFSET(noInterpFrameOffset))[i]; }
		const mstudio_nointerpframes_t* NoInterpFramePair(const int i) const { return &reinterpret_cast<mstudio_nointerpframes_t*>((char*)this + FIX_OFFSET(noInterpFrameOffset))[i]; }
	};


	//
	// VERSION 19
	//

	//
	// Model Bones
	//

	struct mstudiobonedata_v19_t
	{
		short parent; // parent bone;

		uint16_t unk_76; // gotta be used because there's to 8 bit vars that could've fit here, still may be packing. previously 'unk1'

		int flags;

		uint8_t collisionIndex; // index into sections of collision, phy, bvh, etc. needs confirming

		uint8_t proctype;
		uint16_t procindex; // procedural rule

		int unk_C; // chance this is alignment for 16 bytes
	};

	struct mstudiolinearbone_v19_t
	{
		// they cut pos and rot scale, understandable since posscale was never used it tf|2 and they do anims different in apex
		uint16_t numbones;

		uint16_t flagsindex;
		inline int* pFlags(int i) const { assert(i >= 0 && i < numbones); return reinterpret_cast<int*>((char*)this + FIX_OFFSET(flagsindex)) + i; }
		inline int flags(int i) const { return *pFlags(i); }

		uint16_t parentindex;
		inline int* pParent(int i)
			const {
			assert(i >= 0 && i < numbones);
			return reinterpret_cast<int*>((char*)this + FIX_OFFSET(parentindex)) + i;
		}

		uint16_t posindex;
		inline const Vector* pPos(int i)
			const {
			assert(i >= 0 && i < numbones);
			return reinterpret_cast<Vector*>((char*)this + FIX_OFFSET(posindex)) + i;
		}

		uint16_t quatindex;
		inline const Quaternion* pQuat(int i)
			const {
			assert(i >= 0 && i < numbones);
			return reinterpret_cast<Quaternion*>((char*)this + FIX_OFFSET(quatindex)) + i;
		}

		uint16_t rotindex;
		inline const RadianEuler* pRot(int i)
			const {
			assert(i >= 0 && i < numbones);
			return reinterpret_cast<RadianEuler*>((char*)this + FIX_OFFSET(rotindex)) + i;
		}

		uint16_t posetoboneindex;
		inline const matrix3x4_t* pPoseToBone(int i)
			const {
			assert(i >= 0 && i < numbones);
			return reinterpret_cast<matrix3x4_t*>((char*)this + FIX_OFFSET(posetoboneindex)) + i;
		}

		uint16_t qalignmentindex;
		inline const Quaternion* pQAlignment(int i)
			const {
			assert(i >= 0 && i < numbones);
			return reinterpret_cast<Quaternion*>((char*)this + FIX_OFFSET(qalignmentindex)) + i;
		}

		uint16_t scaleindex;
		inline const Vector* pScale(int i)
			const {
			assert(i >= 0 && i < numbones);
			return reinterpret_cast<Vector*>((char*)this + FIX_OFFSET(scaleindex)) + i;
		}
	};


	//
	// Model Anim
	//

	// for datapoint animations
	struct AxisFixup_t
	{
		int8_t adjustment[3]; // per indice

		inline const Vector ToVector(const float scale) const
		{
			Vector vec;

			vec.x = static_cast<float>(adjustment[0]) * scale;
			vec.y = static_cast<float>(adjustment[1]) * scale;
			vec.z = static_cast<float>(adjustment[2]) * scale;

			return vec;
		}
	};
	static_assert(sizeof(AxisFixup_t) == 0x3);

	struct AnimQuat32
	{
		inline uint32_t AsUint32() const { return *reinterpret_cast<const uint32_t*>(this); }
		inline uint32_t* AsUint32Ptr() { return reinterpret_cast<uint32_t*>(this); }
		inline const uint32_t* AsUint32Ptr() const { return reinterpret_cast<const uint32_t*>(this); }

		// copy constructors
		AnimQuat32(AnimQuat32& in)
		{
			*this->AsUint32Ptr() = in.AsUint32();
		}

		AnimQuat32(const AnimQuat32& in)
		{
			*this->AsUint32Ptr() = in.AsUint32();
		}

		// equals
		AnimQuat32& operator=(const AnimQuat32& in)
		{
			*this->AsUint32Ptr() = in.AsUint32();

			return *this;
		}

		// saved components
		int value0 : 7;
		int value1 : 7;
		int value2 : 7;

		uint32_t scaleFactor : 3; // used to set two different scaling floats for unpacking components
		uint32_t droppedAxis : 2; // axis/indice of the dropped component

		uint32_t numInterpFrames : 6; // frame gap between this and the next valid data

		static void Unpack(Quaternion& quat, const AnimQuat32 packedQuat, const AxisFixup_t* const axisFixup);
	};
	static_assert(sizeof(AnimQuat32) == 0x4);

	struct AnimPos64
	{
		inline uint64_t AsUint64() const { return *reinterpret_cast<const uint64_t*>(this); }
		inline uint64_t* AsUint64Ptr() { return reinterpret_cast<uint64_t*>(this); }
		inline const uint64_t* AsUint64Ptr() const { return reinterpret_cast<const uint64_t*>(this); }

		// copy constructors
		AnimPos64(AnimPos64& in)
		{
			*this->AsUint64Ptr() = in.AsUint64();
		}

		AnimPos64(const AnimPos64& in)
		{
			*this->AsUint64Ptr() = in.AsUint64();
		}

		// equals
		AnimPos64& operator=(const AnimPos64& in)
		{
			*this->AsUint64Ptr() = in.AsUint64();

			return *this;
		}

		// saved components
		int16_t values[3]; // x, y, z

		uint16_t scaleFactor : 9; // used to scale the values
		uint16_t numInterpFrames : 7; // frame gap between this and the next valid data

		static void Unpack(Vector& pos, const AnimPos64 packedPos, const AxisFixup_t* const axisFixup);
	};


	//
	// VERSION 19.1
	//

	struct mstudioanimdesc_v19_1_t
	{
		float fps; // frames per second	
		int flags; // looping/non-looping flags

		int numframes;

		uint16_t sznameindex;
		inline const char* const pszName() const { return reinterpret_cast<const char* const>(this) + FIX_OFFSET(sznameindex); }

		uint16_t framemovementindex; // new in v52
		inline const mstudioframemovement_t* const pFrameMovement() const { return reinterpret_cast<const mstudioframemovement_t* const>((char*)this + FIX_OFFSET(framemovementindex)); }

		uint16_t numikrules;

		uint8_t unused_12[4]; // pad? unused? what the hell man

		uint16_t ikruleindex; // non-zero when IK data is stored in the mdl
		inline const mstudioikrule_v16_t* const pIKRule(const int i) const;

		uint64_t animDataAsset; // guid in pak, should be pointer to asset on load. not set if STUDIO_HAS_ANIM is missing

		char* sectionDataExternal; // set on pak asset load
		uint16_t unk1; // maybe some sort of thread/mutic for the external data? set on pak asset load from unk_10

		uint16_t sectionindex;
		uint16_t sectionstallframes; // number of stall frames inside the animation, the reset excluding the final frame are stored externally. when external data is not loaded(?)/found(?) it falls back on the last frame of this as a stall
		uint16_t sectionframes; // number of frames used in each fast lookup section, zero if not used
		inline const mstudioanimsections_v16_t* const pSection(int i) const { return reinterpret_cast<const mstudioanimsections_v16_t* const>((char*)this + FIX_OFFSET(sectionindex)) + i; }
		// numsections = ((numframes - sectionstallframes - 1) / sectionframes) + 2;
		// numsections after stall section, if stall frames > zero add one section
	};


	//
	// VERSION GENERAL
	//

	//
	// Animation Static Data
	// 

	static const float s_StudioWeightList_1[256] = {};	// all zeroes
	static const float s_StudioWeightList_3[256] = {	// all ones
		1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f,
		1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f,
		1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f,
		1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f,
		1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f,
		1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f,
		1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f,
		1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f,
		1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f,
		1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f,
		1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f,
		1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f,
		1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f,
		1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f,
		1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f,
		1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f
	};

	static const mstudioikrule_v16_t s_StudioIKRule_3[1]{
		{
			.chain = 0,
			.bone = 0,
			.type = 4,
			.slot = 0,

			.compressedikerror {},
			.compressedikerrorindex = sizeof(mstudioikrule_v16_t),

			.iStart = 0,
			.ikerrorindex = 0,

			.szattachmentindex = 0,

			.start = 0.0f,
			.peak = 0.0f,
			.tail = 1.0f,
			.end = 1.0f,

			.contact = 0.0f,
			.drop = 0.0f,
			.top = 0.0f,

			.height = 0.0f,
			.endHeight = 0.0f,
			.radius = 0.0f,
			.floor = 0.0f,

			.pos {},
			.q {},
		}
	};

	static const mstudioikrule_v16_t s_StudioIKRule_5[2]{
		{
			.chain = 0,
			.bone = 0,
			.type = 4,
			.slot = 0,

			.compressedikerror {},
			.compressedikerrorindex = sizeof(mstudioikrule_v16_t) * 2,

			.iStart = 0,
			.ikerrorindex = 0,

			.szattachmentindex = 0,

			.start = 0.0f,
			.peak = 0.0f,
			.tail = 1.0f,
			.end = 1.0f,

			.contact = 0.0f,
			.drop = 0.0f,
			.top = 0.0f,

			.height = 0.0f,
			.endHeight = 0.0f,
			.radius = 0.0f,
			.floor = 0.0f,

			.pos {},
			.q {},
		},

		{
			.chain = 1,
			.bone = 0,
			.type = 4,
			.slot = 1,

			.compressedikerror {},
			.compressedikerrorindex = sizeof(mstudioikrule_v16_t),

			.iStart = 0,
			.ikerrorindex = 0,

			.szattachmentindex = 0,

			.start = 0.0f,
			.peak = 0.0f,
			.tail = 1.0f,
			.end = 1.0f,

			.contact = 0.0f,
			.drop = 0.0f,
			.top = 0.0f,

			.height = 0.0f,
			.endHeight = 0.0f,
			.radius = 0.0f,
			.floor = 0.0f,

			.pos {},
			.q {},
		}
	};

	inline const mstudioikrule_v16_t* const mstudioanimdesc_v16_t::pIKRule(const int i) const
	{ 
		switch (ikruleindex)
		{
		case 3:
		{
			return s_StudioIKRule_3;
		}
		case 5:
		{
			return s_StudioIKRule_5;
		}
		default:
		{
			return reinterpret_cast<mstudioikrule_v16_t*>((char*)this + FIX_OFFSET(ikruleindex)) + i;
		}
		}
	}

	inline const mstudioikrule_v16_t* const mstudioanimdesc_v19_1_t::pIKRule(const int i) const
	{ 
		switch (ikruleindex)
		{
		case 3:
		{
			assertm(i < 1, "out of range");
			return s_StudioIKRule_3 + i;
		}
		case 5:
		{
			assertm(i < 2, "out of range");
			return s_StudioIKRule_5 + i;
		}
		default:
		{
			return reinterpret_cast<mstudioikrule_v16_t*>((char*)this + FIX_OFFSET(ikruleindex)) + i;
		}
		}
	}

	inline const float* const mstudioseqdesc_v18_t::pWeightList() const
	{
		switch (weightlistindex)
		{
		case 1:
		{
			return s_StudioWeightList_1;
		}
		case 3:
		{
			return s_StudioWeightList_3;
		}
		default:
		{
			return reinterpret_cast<float*>((char*)this + FIX_OFFSET(weightlistindex));
		}
		}
	}

	// VERSION 19.2

	struct studiohdr_v19_2_t
	{
		int flags;
		int checksum; // unsure if this is still checksum, there isn't any other files that have it still
		uint16_t sznameindex; // No longer stored in string block, uses string in header.
		inline const char* const pszName() const { return reinterpret_cast<const char* const>(this) + FIX_OFFSET(sznameindex); }
		char name[33]; // The internal name of the model, padding with null chars. last byte always null

		uint8_t surfacepropLookup; // saved in the file (unsigned dl/fx/ferrofluid_ult_base_puddle)

		float mass;

		int contents;

		uint16_t hitboxsetindex;
		uint8_t numhitboxsets;

		uint8_t illumpositionattachmentindex;

		Vector illumposition;	// illumination center

		Vector hull_min;		// ideal movement hull size
		Vector hull_max;

		Vector view_bbmin;		// clipping bounding box
		Vector view_bbmax;

		uint16_t boneCount; // bones
		uint16_t boneHdrOffset;
		uint16_t boneDataOffset;

		uint16_t numlocalseq; // sequences
		uint16_t localseqindex;

		// needs to be confirmed
		uint16_t unk_7E[2]; // added in v13 -> v14

		// needs to be confirmed
		char activitylistversion; // initialization flag - have the sequences been indexed?

		uint8_t numlocalattachments;
		uint16_t localattachmentindex;

		uint16_t numlocalnodes;
		uint16_t localnodenameindex;
		uint16_t localNodeDataOffset; // offset into an array of int sized offsets that read into the data for each node

		uint16_t numikchains;
		uint16_t ikchainindex;

		uint16_t numtextures; // the material limit exceeds 128, probably 256.
		uint16_t textureindex;

		// replaceable textures tables
		uint16_t numskinref;
		uint16_t numskinfamilies;
		uint16_t skinindex;

		uint16_t numbodyparts;
		uint16_t bodypartindex;
		inline const mstudiobodyparts_v16_t* const pBodypart(const uint16_t i) const { assert(i >= 0 && i < numbodyparts); return reinterpret_cast<mstudiobodyparts_v16_t*>((char*)this + FIX_OFFSET(bodypartindex)) + i; }

		// this is rui meshes
		uint16_t uiPanelCount;
		uint16_t uiPanelOffset;

		uint16_t numlocalposeparameters;
		uint16_t localposeparamindex;

		uint16_t surfacepropindex;

		uint16_t keyvalueindex;

		uint16_t virtualModel;

		// hw data lookup from rmdl
		uint16_t meshCount; // number of meshes per lod

		uint16_t bonetablebynameindex; // bonetable is u16[] since v19.2

		uint16_t boneStateOffset;
		uint16_t boneStateCount;
		inline const uint16_t* pBoneStates() const { return boneStateCount > 0 ? reinterpret_cast<uint16_t*>((char*)this + offsetof(studiohdr_v19_2_t, boneStateOffset) + FIX_OFFSET(boneStateOffset)) : nullptr; }
		uint16_t boneStatePerLOD[MAX_NUM_LODS]; // number of bones, size of bonesates, per lod

		// sets of lods
		uint16_t groupHeaderOffset;
		uint16_t groupHeaderCount;
		const studio_hw_groupdata_v16_t* const pLODGroup(const uint16_t i) const { return reinterpret_cast<const studio_hw_groupdata_v16_t* const>((char*)this + offsetof(studiohdr_v19_2_t, groupHeaderOffset) + FIX_OFFSET(groupHeaderOffset)) + i; }

		uint16_t lodOffset;
		uint16_t lodCount;
		const float* const pLODThreshold(const uint16_t i) const { return reinterpret_cast<const float* const>((char*)this + offsetof(studiohdr_v19_2_t, lodOffset) + FIX_OFFSET(lodOffset)) + i; }
		const float LODThreshold(const uint16_t i) const { return *pLODThreshold(i); }

		// 
		float fadeDistance;
		float gatherSize; // what. from r5r struct

		uint16_t numsrcbonetransform;
		uint16_t srcbonetransformindex;

		// asset bakery strings if it has any
		uint16_t sourceFilenameOffset;

		uint16_t linearboneindex;

		// used for adjusting weights in sequences, quick lookup into bones that have procbones, unsure what else uses this.
		uint16_t procBoneCount;
		uint16_t procBoneOffset; // in order array of procbones and their parent bone indice
		uint16_t linearProcBoneOffset; // byte per bone with indices into each bones procbone, 0xff if no procbone is present

		// mostly seen on '_animated' suffixed models
		// manually declared bone followers are no longer stored in kvs under 'bone_followers', they are now stored in an array of ints with the bone index.
		uint16_t boneFollowerCount;
		uint16_t boneFollowerOffset;

		uint16_t bvhOffset;

		char bvhUnk[2]; // collision detail for bvh (?)

		// perhaps these are t he same varibles added in v12.3? cannot find any models that use them previously (pre-v16).
		// UnkDataType_0_t
		uint16_t unkDataCount; // unk_0xDA
		uint16_t unkDataOffset; // unk_0xDC
		// UnkDataType_1_t
		uint16_t unkStrcOffset; // unk_0xDE

		int unk_E0;
	};

	static_assert(offsetof(studiohdr_v19_2_t, groupHeaderOffset) == 0xC4);
}
#pragma pack(pop)