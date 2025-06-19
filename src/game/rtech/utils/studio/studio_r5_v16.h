#pragma once
#include <game/rtech/utils/utils.h> // eCompressionType

namespace r5
{
	struct studio_hw_groupdata_v16_t
	{
		int dataOffset;				// offset to this section in compressed vg
		int dataSizeCompressed;		// compressed size of this lod buffer in hwData
		int dataSizeDecompressed;	// decompressed size of this lod buffer in hwData

		eCompressionType dataCompression; // none and oodle, haven't seen anything else used.

		//
		uint8_t lodIndex;		// base lod idx?
		uint8_t lodCount;		// number of lods contained within this group
		uint8_t lodMap;		// lods in this group, each bit is a lod
	};
	static_assert(sizeof(studio_hw_groupdata_v16_t) == 16);

	// aseq v11
	#pragma pack(push, 2)
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
	#pragma pack(pop)

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
		inline const char* pszName() const { return ((char*)this + FIX_OFFSET(sznameindex)); }

		uint16_t framemovementindex; // new in v52
		inline r5::mstudioframemovement_t* pFrameMovement() const { return reinterpret_cast<r5::mstudioframemovement_t*>((char*)this + FIX_OFFSET(framemovementindex)); }

		int animindex; // non-zero when anim data isn't in sections

		uint16_t numikrules;
		uint16_t ikruleindex; // non-zero when IK data is stored in the mdl
		inline const mstudioikrule_v16_t* const pIKRule(const int i) const { return reinterpret_cast<mstudioikrule_v16_t*>((char*)this + FIX_OFFSET(ikruleindex)) + i; };

		char* sectionDataExternal; // set on pak asset load
		uint16_t unk1; // maybe some sort of thread/mutic for the external data? set on pak asset load from unk_10

		uint16_t sectionindex;
		uint16_t sectionstallframes; // number of stall frames inside the animation, the reset excluding the final frame are stored externally. when external data is not loaded(?)/found(?) it falls back on the last frame of this as a stall
		uint16_t sectionframes; // number of frames used in each fast lookup section, zero if not used
		inline const mstudioanimsections_v16_t* pSection(int i) const { return reinterpret_cast<mstudioanimsections_v16_t*>((char*)this + FIX_OFFSET(sectionindex)) + i; }
	};

	struct mstudioseqdesc_v16_t
	{
		uint16_t szlabelindex;
		inline const char* pszLabel() const { return ((char*)this + FIX_OFFSET(szlabelindex)); }

		uint16_t szactivitynameindex;
		inline const char* pszActivity() const { return ((char*)this + FIX_OFFSET(szactivitynameindex)); }

		int flags; // looping/non-looping flags

		uint16_t activity; // initialized at loadtime to game DLL values
		uint16_t actweight;

		uint16_t numevents;
		uint16_t eventindex;

		Vector bbmin; // per sequence bounding box
		Vector bbmax;

		uint16_t numblends;

		// Index into array of shorts which is groupsize[0] x groupsize[1] in length
		uint16_t animindexindex;
		const int AnimIndex(const int i) const { return FIX_OFFSET(reinterpret_cast<uint16_t*>((char*)this + FIX_OFFSET(animindexindex))[i]); }
		const int AnimCount() const { return  groupsize[0] * groupsize[1]; }
		mstudioanimdesc_v16_t* pAnimDesc(const int i) const { return reinterpret_cast<mstudioanimdesc_v16_t*>((char*)this + AnimIndex(i)); }

		//short movementindex; // [blend] float array for blended movement
		short paramindex[2]; // X, Y, Z, XR, YR, ZR
		float paramstart[2]; // local (0..1) starting value
		float paramend[2]; // local (0..1) ending value
		//short paramparent;

		float fadeintime; // ideal cross fate in time (0.2 default)
		float fadeouttime; // ideal cross fade out time (0.2 default)

		// animseq/humans/class/medium/pilot_medium_bangalore/bangalore_freefall.rseq
		uint16_t localentrynode; // transition node at entry
		uint16_t localexitnode; // transition node at exit

		uint16_t numikrules;

		uint16_t numautolayers;
		uint16_t autolayerindex;

		uint16_t weightlistindex;

		uint8_t groupsize[2];

		// animseq/humans/class/medium/mp_pilot_medium_core/medium_bow_charge_pose.rseq
		uint16_t posekeyindex;

		// never used?
		uint16_t numiklocks;
		uint16_t iklockindex;

		// whar
		uint16_t unk_5C;

		// animseq/weapons/octane_epipen/ptpov_octane_epipen_held/drain_fluid_layer.rseq
		uint16_t cycleposeindex; // index of pose parameter to use as cycle index

		uint16_t activitymodifierindex;
		uint16_t numactivitymodifiers;

		int ikResetMask; // new in v52
		int unk1;

		uint16_t unkOffset;
		uint16_t unkCount;
	};

	// aseq v12
	struct mstudioseqdesc_v18_t
	{
		uint16_t szlabelindex;
		inline const char* pszLabel() const { return ((char*)this + FIX_OFFSET(szlabelindex)); }

		uint16_t szactivitynameindex;
		inline const char* pszActivity() const { return ((char*)this + FIX_OFFSET(szactivitynameindex)); }

		int flags; // looping/non-looping flags

		uint16_t activity; // initialized at loadtime to game DLL values
		uint16_t actweight;

		uint16_t numevents;
		uint16_t eventindex;

		Vector bbmin; // per sequence bounding box
		Vector bbmax;

		uint16_t numblends;

		// Index into array of shorts which is groupsize[0] x groupsize[1] in length
		uint16_t animindexindex;
		const int AnimIndex(const int i) const { return FIX_OFFSET(reinterpret_cast<uint16_t*>((char*)this + FIX_OFFSET(animindexindex))[i]); }
		const int AnimCount() const { return  groupsize[0] * groupsize[1]; }
		mstudioanimdesc_v16_t* pAnimDesc(const int i) const { return reinterpret_cast<mstudioanimdesc_v16_t*>((char*)this + AnimIndex(i)); }

		//short movementindex; // [blend] float array for blended movement
		short paramindex[2]; // X, Y, Z, XR, YR, ZR
		float paramstart[2]; // local (0..1) starting value
		float paramend[2]; // local (0..1) ending value
		//short paramparent;

		float fadeintime; // ideal cross fate in time (0.2 default)
		float fadeouttime; // ideal cross fade out time (0.2 default)

		// animseq/humans/class/medium/pilot_medium_bangalore/bangalore_freefall.rseq
		uint16_t localentrynode; // transition node at entry
		uint16_t localexitnode; // transition node at exit

		uint16_t numikrules;

		uint16_t numautolayers;
		uint16_t autolayerindex;

		uint16_t weightlistindex;

		uint8_t groupsize[2];

		// animseq/humans/class/medium/mp_pilot_medium_core/medium_bow_charge_pose.rseq
		uint16_t posekeyindex;

		// never used?
		uint16_t numiklocks;
		uint16_t iklockindex;

		// whar
		uint16_t unk_5C;

		// animseq/weapons/octane_epipen/ptpov_octane_epipen_held/drain_fluid_layer.rseq
		uint16_t cycleposeindex; // index of pose parameter to use as cycle index

		uint16_t activitymodifierindex;
		uint16_t numactivitymodifiers;

		int ikResetMask; // new in v52
		int unk1;

		uint16_t unkOffset;
		uint16_t unkCount;

		// points to an array of ints
		uint16_t unk_70; // offset
		uint16_t unk_72; // count
	};

	struct mstudiobonehdr_v16_t
	{
		int contents; // See BSPFlags.h for the contents flags

		uint8_t unk_0x4;

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

		uint16_t unk_0x76; // gotta be used because there's to 8 bit vars that could've fit here, still may be packing. previously 'unk1'

		int flags;

		uint8_t collisionIndex; // index into sections of collision, phy, bvh, etc. needs confirming

		uint8_t proctype;
		uint16_t procindex; // procedural rule
	};

	struct mstudiomesh_v16_t
	{
		uint16_t material;

		// a unique ordinal for this mesh
		uint16_t meshid;

		char unk[4];

		Vector center;
	};

	struct mstudiomodel_v16_t
	{
		uint16_t unkStringOffset; // goes to bones sometimes

		uint16_t meshCountTotal; // total number of meshes in this model
		uint16_t meshCountBase; // number of normal meshes in this model
		uint16_t meshCountBlends; // number of blend shape meshes

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

	struct studiohdr_v16_t
	{
		int flags;
		int checksum; // unsure if this is still checksum, there isn't any other files that have it still
		uint16_t sznameindex; // No longer stored in string block, uses string in header.
		char name[32]; // The internal name of the model, padding with null chars.
		char unk_0x2A; // always zero
		inline char* const pszName() const { return ((char*)this + FIX_OFFSET(sznameindex)); }

		uint8_t surfacepropLookup; // saved in the file

		float mass;

		int unk_0x30; // this is not version

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
		uint16_t unk_v54_v14[2]; // added in v13 -> v14

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

		// hw data lookup from rmdl
		uint16_t meshCount; // number of meshes per lod
		uint16_t meshOffset;

		uint16_t bonetablebynameindex;

		uint16_t boneStateOffset;
		uint16_t boneStateCount;
		inline const uint8_t* pBoneStates() const { return boneStateCount > 0 ? reinterpret_cast<uint8_t*>((char*)this + offsetof(studiohdr_v16_t, boneStateOffset) + FIX_OFFSET(boneStateOffset)) : nullptr; }

		// sets of lods
		uint16_t groupHeaderOffset;
		uint16_t groupHeaderCount;
		const studio_hw_groupdata_v16_t* pLODGroup(uint16_t i) const { return reinterpret_cast<studio_hw_groupdata_v16_t*>((char*)this + offsetof(studiohdr_v16_t, groupHeaderOffset) + FIX_OFFSET(groupHeaderOffset)) + i; }

		uint16_t lodOffset;
		uint16_t lodCount;  // check this
		const float* pLODThreshold(uint16_t i) const { return reinterpret_cast<float*>((char*)this + offsetof(studiohdr_v16_t, lodOffset) + FIX_OFFSET(lodOffset)) + i; }
		const float LODThreshold(uint16_t i) const { return *pLODThreshold(i); }

		// 
		float fadeDistance;
		float gathersize; // what. from r5r struct

		uint16_t numsrcbonetransform;
		uint16_t srcbonetransformindex;

		// asset bakery strings if it has any
		uint16_t sourceFilenameOffset;

		uint16_t linearboneindex;

		// unsure what this is for but it exists for jigglbones
		uint16_t procBoneCount;
		uint16_t procBoneOffset;
		uint16_t linearProcBoneOffset;

		// mostly seen on '_animated' suffixed models
		// manually declared bone followers are no longer stored in kvs under 'bone_followers', they are now stored in an array of ints with the bone index.
		uint16_t boneFollowerCount;
		uint16_t boneFollowerOffset;

		uint16_t bvhOffset;

		char bvhUnk[2]; // collision detail for bvh (?)

		short unk_0xDA;
		short unk_0xDC;
		short unk_0xDE;
	};

	// mdl_ v17
	struct studiohdr_v17_t
	{
		int flags;
		int checksum; // unsure if this is still checksum, there isn't any other files that have it still
		uint16_t sznameindex; // No longer stored in string block, uses string in header.
		char name[32]; // The internal name of the model, padding with null chars.
		char unk_0x2A; // always zero
		inline char* const pszName() const { return ((char*)this + FIX_OFFSET(sznameindex)); }

		uint8_t surfacepropLookup; // saved in the file

		float mass;

		int unk_0x30; // this is not version

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
		uint16_t unk_v54_v14[2]; // added in v13 -> v14

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

		// hw data lookup from rmdl
		uint16_t meshCount; // number of meshes per lod
		uint16_t meshOffset;

		uint16_t bonetablebynameindex;

		uint16_t boneStateOffset;
		uint16_t boneStateCount;
		inline const uint8_t* pBoneStates() const { return boneStateCount > 0 ? reinterpret_cast<uint8_t*>((char*)this + offsetof(studiohdr_v16_t, boneStateOffset) + FIX_OFFSET(boneStateOffset)) : nullptr; }

		// sets of lods
		uint16_t groupHeaderOffset;
		uint16_t groupHeaderCount;
		const studio_hw_groupdata_v16_t* pLODGroup(uint16_t i) const { return reinterpret_cast<studio_hw_groupdata_v16_t*>((char*)this + offsetof(studiohdr_v16_t, groupHeaderOffset) + FIX_OFFSET(groupHeaderOffset)) + i; }

		uint16_t lodOffset;
		uint16_t lodCount;  // check this
		const float* pLODThreshold(uint16_t i) const { return reinterpret_cast<float*>((char*)this + offsetof(studiohdr_v16_t, lodOffset) + FIX_OFFSET(lodOffset)) + i; }
		const float LODThreshold(uint16_t i) const { return *pLODThreshold(i); }

		// 
		float fadeDistance;
		float gathersize; // what. from r5r struct

		uint16_t numsrcbonetransform;
		uint16_t srcbonetransformindex;

		// asset bakery strings if it has any
		uint16_t sourceFilenameOffset;

		uint16_t linearboneindex;

		// unsure what this is for but it exists for jigglbones
		uint16_t procBoneCount;
		uint16_t procBoneOffset;
		uint16_t linearProcBoneOffset;

		// mostly seen on '_animated' suffixed models
		// manually declared bone followers are no longer stored in kvs under 'bone_followers', they are now stored in an array of ints with the bone index.
		uint16_t boneFollowerCount;
		uint16_t boneFollowerOffset;

		uint16_t bvhOffset;

		char bvhUnk[2]; // collision detail for bvh (?)

		short unk_0xDA;
		short unk_0xDC;
		short unk_0xDE;

		int unk_0xE0;
	};
}