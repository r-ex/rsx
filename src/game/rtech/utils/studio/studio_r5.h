#pragma once

namespace r5
{
	// r5 bvh data
	struct mstudiocollmodel_v8_t
	{
		int contentMasksIndex;
		int surfacePropsIndex;
		int surfaceNamesIndex;
		int headerCount;
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

	struct mstudiocollheader_v12_1_t
	{
		int unk;
		int bvhNodeIndex;
		int vertIndex;
		int bvhLeafIndex;
		int unkIndex;
		int unkNew;
		float origin[3];
		float scale;
	};

	// r5 anim data
	enum ValuePtrFlags_t : char
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
		short offset : 13;
		short flags : 3;
		uint8_t axisIdx1; // these two are definitely unsigned
		uint8_t axisIdx2;

		inline mstudioanimvalue_t* pAnimvalue() const { return reinterpret_cast<mstudioanimvalue_t*>((char*)this + offset); }
	};

	// flags for the per bone animation headers
	// "mstudioanim_valueptr_t" indicates it has a set of offsets into anim tracks
	enum RleFlags_t : char
	{
		STUDIO_ANIM_ANIMSCALE = 0x01, // mstudioanim_valueptr_t
		STUDIO_ANIM_ANIMROT = 0x02, // mstudioanim_valueptr_t
		STUDIO_ANIM_ANIMPOS = 0x04, // mstudioanim_valueptr_t
	};

	// flags for the per bone array, in 4 bit sections (two sets of flags per char), aligned to two chars
	enum RleBoneFlags_t
	{
		STUDIO_ANIM_POS		= 0x1, // bone has pos values
		STUDIO_ANIM_ROT		= 0x2, // bone has rot values
		STUDIO_ANIM_SCALE	= 0x4, // bone has scale values
		STUDIO_ANIM_UNK8	= 0x8, // used in virtual models for new anim type

		STUDIO_ANIM_DATA	= (STUDIO_ANIM_POS | STUDIO_ANIM_ROT | STUDIO_ANIM_SCALE), // bone has animation data
		STUDIO_ANIM_MASK	= (STUDIO_ANIM_POS | STUDIO_ANIM_ROT | STUDIO_ANIM_SCALE | STUDIO_ANIM_UNK8),
	};

	#define ANIM_BONEFLAG_BITS				4 // a nibble even
	#define ANIM_BONEFLAG_SIZE(count)		(IALIGN2((ANIM_BONEFLAG_BITS * count + 7) / 8)) // size in bytes of the bone flag array. pads 7 bits for truncation, then align to 2 bytes
	#define ANIM_BONEFLAG_SHIFT(idx)		(ANIM_BONEFLAG_BITS * (idx % 2)) // return four (bits) if this index is odd, as there are four bits per bone
	#define ANIM_BONEFLAGS_FLAG(ptr, idx)	(static_cast<uint8_t>(ptr[idx / 2] >> ANIM_BONEFLAG_SHIFT(idx)) & 0xf) // get byte offset, then shift if needed, mask to four bits

	struct mstudio_rle_anim_t
	{
	public:
		short size : 13; // total size of all animation data, not next offset because even the last one has it
		short flags : 3;

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

	struct mstudioframemovement_t
	{
		float scale[4];
		int sectionframes;

		inline const int SectionCount(const int numframes) const { return (numframes - 1) / sectionframes + 1; } // this might be wrong :(
		inline int SectionIndex(const int frame) const { return frame / sectionframes; } // get the section index for this frame
		inline int SectionOffset(const int frame) const { return reinterpret_cast<int*>((char*)this + sizeof(mstudioframemovement_t))[SectionIndex(frame)]; }
		inline int SectionOffset_V16(const int frame) const { return FIX_OFFSET(reinterpret_cast<uint16_t*>((char*)this + sizeof(r5::mstudioframemovement_t))[SectionIndex(frame)]); }
		inline uint16_t* pSection(const int frame, const bool retail) const { return reinterpret_cast<uint16_t*>((char*)this + (retail ? SectionOffset_V16(frame) : SectionOffset(frame))); }

		inline mstudioanimvalue_t* pAnimvalue(const int i, const uint16_t* section) const { return (section[i] > 0) ? reinterpret_cast<mstudioanimvalue_t*>((char*)section + section[i]) : nullptr; }

		inline const uint8_t* const pDatapointTrack() const
		{
			constexpr int baseOffset = sizeof(mstudioframemovement_t) + sizeof(uint16_t);
			return reinterpret_cast<const uint8_t* const>(this) + baseOffset;
		}
	};
	static_assert(sizeof(mstudioframemovement_t) == 0x14);

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

	int GetAnimValueOffset(const mstudioanimvalue_t* const panimvalue);
	void ExtractAnimValue(int frame, mstudioanimvalue_t* panimvalue, float scale, float& v1);
	void ExtractAnimValue(int frame, mstudioanimvalue_t* panimvalue, float scale, float& v1, float& v2);

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

	// movements
	bool Studio_AnimPosition(const animdesc_t* const panim, float flCycle, Vector& vecPos, QAngle& vecAngle);
}