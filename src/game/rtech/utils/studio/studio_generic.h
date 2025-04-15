#pragma once
#include <game/rtech/utils/studio/studio.h>
#include <game/rtech/utils/studio/studio_r1.h>
#include <game/rtech/utils/studio/studio_r2.h>
#include <game/rtech/utils/studio/studio_r5.h>
#include <game/rtech/utils/studio/studio_r5_v8.h>
#include <game/rtech/utils/studio/studio_r5_v12.h>
#include <game/rtech/utils/studio/studio_r5_v16.h>

struct animmovement_t
{
	animmovement_t(const animmovement_t& movement);
	animmovement_t(r5::mstudioframemovement_t* movement, const int frameCount, const bool indexType);
	animmovement_t(r1::mstudioframemovement_t* movement);
	~animmovement_t()
	{
		if (nullptr != sections) delete[] sections;
	}

	void* baseptr;
	float scale[4];

	union
	{
		int* sections;
		short offset[4];
	};
	int sectionframes;
	int sectioncount;

	inline int SectionIndex(const int frame) const { return frame / sectionframes; } // get the section index for this frame
	inline int SectionOffset(const int frame) const { return sections[SectionIndex(frame)]; }
	inline uint16_t* pSection(const int frame) const { return reinterpret_cast<uint16_t*>((char*)baseptr + SectionOffset(frame)); }

	inline mstudioanimvalue_t* pAnimvalue(const int i, const uint16_t* section) const { return (section[i] > 0) ? reinterpret_cast<mstudioanimvalue_t*>((char*)section + section[i]) : nullptr; }
	inline mstudioanimvalue_t* pAnimvalue(int i) const { return (offset[i] > 0) ? reinterpret_cast<mstudioanimvalue_t*>((char*)this + offset[i]) : nullptr; }
};

struct animsection_t
{
	int animindex;
	bool isExternal;
};

struct animdesc_t
{
	animdesc_t(const animdesc_t& animdesc);
	animdesc_t(r5::mstudioanimdesc_v8_t* animdesc);
	animdesc_t(r5::mstudioanimdesc_v12_1_t* animdesc, char* ext);
	animdesc_t(r5::mstudioanimdesc_v16_t* animdesc, char* ext);
	animdesc_t(r2::mstudioanimdesc_t* animdesc);
	~animdesc_t()
	{
		if (nullptr != movement) delete movement;
	}

	void* baseptr; // for getting to the animations
	const char* name;

	float fps;
	int flags;

	int numframes;

	int nummovements;
	int movementindex;
	int framemovementindex;
	animmovement_t* movement;
	inline r1::mstudiomovement_t* const pMovement(int i) const { return reinterpret_cast<r1::mstudiomovement_t*>((char*)this + movementindex) + i; };

	int animindex;
	// data array, starting with per bone flags
	char* pAnimdataStall(int* const piFrame) const;
	char* pAnimdataNoStall(int* const piFrame) const;

	int sectionindex; // can be safely removed
	int sectionstallframes; // number of static frames inside the animation, the reset excluding the final frame are stored externally. when external data is not loaded(?)/found(?) it falls back on the last frame of this as a stall
	int sectionframes; // number of frames used in each fast lookup section, zero if not used
	std::vector<animsection_t> sections;
	inline const bool HasStall() const { return sectionstallframes > 0; }
	inline const animsection_t* pSection(const int i) const { return &sections.at(i); }
	inline const int SectionCount() const { return ((numframes - sectionstallframes - 1) / sectionframes) + 2 + (static_cast<int>(HasStall())); }

	char* extraData;

	size_t parsedBufferIndex;
};

struct seqdesc_t
{
	seqdesc_t() = default;
	seqdesc_t(r5::mstudioseqdesc_v8_t* seqdesc);
	seqdesc_t(r5::mstudioseqdesc_v8_t* seqdesc, char* ext);
	seqdesc_t(r5::mstudioseqdesc_v16_t* seqdesc, char* ext);
	seqdesc_t(r2::mstudioseqdesc_t* seqdesc);

	void* baseptr;

	const char* szlabel;
	const char* szactivityname;

	int flags; // looping/non-looping flags

	int weightlistindex;
	const float* pWeight(const int i) const { return reinterpret_cast<const float*>((char*)baseptr + weightlistindex) + i; };
	const float Weight(const int i) const { return *pWeight(i); };

	std::vector<animdesc_t> anims;
	CRamen parsedData;

	const int AnimCount() const { return static_cast<int>(anims.size()); }
};

struct studio_hw_groupdata_t
{
	studio_hw_groupdata_t() = default;
	studio_hw_groupdata_t(const r5::studio_hw_groupdata_v16_t* const group);
	studio_hw_groupdata_t(const r5::studio_hw_groupdata_v12_1_t* const group);

	int dataOffset;				// offset to this section in compressed vg
	int dataSizeCompressed;		// compressed size of this lod buffer in hwData
	int dataSizeDecompressed;	// decompressed size of this lod buffer in hwData

	eCompressionType dataCompression; // none and oodle, haven't seen anything else used.

	//
	uint8_t lodIndex;		// base lod idx?
	uint8_t lodCount;		// number of lods contained within this group
	uint8_t lodMap;		// lods in this group, each bit is a lod
};
static_assert(sizeof(studio_hw_groupdata_t) == 16);

struct studiohdr_generic_t
{
	studiohdr_generic_t() = default;
	studiohdr_generic_t(const r5::studiohdr_v8_t* const pHdr);
	studiohdr_generic_t(const r5::studiohdr_v12_1_t* const pHdr);
	studiohdr_generic_t(const r5::studiohdr_v12_2_t* const pHdr);
	studiohdr_generic_t(const r5::studiohdr_v12_3_t* const pHdr);
	studiohdr_generic_t(const r5::studiohdr_v14_t* const pHdr);
	studiohdr_generic_t(const r5::studiohdr_v16_t* const pHdr, int dataSizePhys, int dataSizeModel);
	studiohdr_generic_t(const r1::studiohdr_t* const pHdr, StudioLooseData_t* const pData);
	studiohdr_generic_t(const r2::studiohdr_t* const pHdr);

	const char* baseptr;

	int length; // size of rmdl file in bytes
	int flags; // studio flags
	int contents; // contents mask, see bspflags.h

	int vtxOffset; // VTX
	int vvdOffset; // VVD / IDSV
	int vvcOffset; // VVC / IDCV
	int vvwOffset; // index will come last after other vertex files
	int phyOffset; // VPHY / IVPS

	int vtxSize;
	int vvdSize;
	int vvcSize;
	int vvwSize;
	int phySize; // still used in models using vg
	size_t hwDataSize;

	int boneOffset;
	int boneDataOffset; // guh
	int boneCount;

	int textureOffset;
	int textureCount;

	int numSkinRef;
	int numSkinFamilies;
	int skinOffset;
	inline const int16_t* const pSkinFamily(const int i) const { return reinterpret_cast<const int16_t*>(baseptr + skinOffset) + (numSkinRef * i); };
	inline const char* const pSkinName(const int i) const
	{
		// only stored for index 1 and up
		if (i == 0)
			return STUDIO_DEFAULT_SKIN_NAME;

		const int index = reinterpret_cast<const int* const>(pSkinFamily(0) + (IALIGN2(numSkinRef * numSkinFamilies)))[i - 1];

		const char* const name = baseptr + index;
		if (IsStringZeroLength(name))
			return STUDIO_NULL_SKIN_NAME;

		return name;
	}

	inline const char* const pSkinName_V16(const int i) const
	{
		// only stored for index 1 and up
		if (i == 0)
			return STUDIO_DEFAULT_SKIN_NAME;

		const uint16_t index = *(reinterpret_cast<const uint16_t* const>(pSkinFamily(numSkinFamilies)) + (i - 1));

		const char* const name = baseptr + FIX_OFFSET(index);
		if (IsStringZeroLength(name))
			return STUDIO_NULL_SKIN_NAME;

		return name;
	}

	int boneStateOffset;
	int boneStateCount;
	inline const uint8_t* const pBoneStates() const { return boneStateCount > 0 ? reinterpret_cast<uint8_t*>((char*)baseptr + boneStateOffset) : nullptr; }

	int groupCount;
	studio_hw_groupdata_t groups[8]; // early 'vg' will only have one group

	int bvhOffset;
};