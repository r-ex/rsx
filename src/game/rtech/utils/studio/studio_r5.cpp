#include <pch.h>
#include <game/rtech/utils/studio/studio.h>
#include <game/rtech/utils/studio/studio_r5.h>
#include <game/rtech/utils/studio/studio_r5_v12.h>
#include <game/rtech/utils/studio/studio_r5_v16.h>

namespace r5
{
	static char s_FrameBitCountLUT[4]{ 0, 2, 4, 0 }; // 0x1412FC154i64 (12FC154h)
	static float s_FrameValOffsetLUT[4]{ 0.0f, 3.0f, 15.0f, 0.0f }; // dword_1412FED60 (12FED60) (1 << s_FrameBitCountLUT[i] ==  s_FrameValOffsetLUT[i])
	static char s_AnimSeekLUT[60] // [rika]: may be larger in newer versions
	{
		1,  15, 16, 2,  7,  8,  2,  15, 0,  3,  15, 0,  4,  15, 0,  5,
		15, 0,  6,  15, 0,  7,  15, 0,  2,  15, 2,  3,  15, 2,  4,  15,
		2,  5,  15, 2,  6,  15, 2,  7,  15, 2,  2,  15, 4,  3,  15, 4,
		4,  15, 4, 5, 15, 4, 6, 15, 4,  7,  15, 4,
	};

	// this LUT will Just Work within reason, no real need to fashion values specially
	#define FIXED_ROT_SCALE 0.00019175345f
	#define FIXED_SCALE_SCALE 0.0030518509f

	// literal magic
	int GetAnimValueOffset(const mstudioanimvalue_t* const panimvalue)
	{
		const int lutBaseIdx = panimvalue->num.valid * 3;

		return (s_AnimSeekLUT[lutBaseIdx] + ((s_AnimSeekLUT[lutBaseIdx + 1] + panimvalue->num.total * s_AnimSeekLUT[lutBaseIdx + 2]) >> 4));
	}

	inline void ExtractAnimValue(const mstudioanimvalue_t* panimvalue, const int frame, const float scale, float& v1)
	{
		// note: in R5 'valid' is not really used the same way as traditional source.
		switch (panimvalue->num.valid)
		{
			// '0' valid values is treated as the traditional source style track, having a 16bit signed integer per frame value. this will be used if the more compact tracks cannot be utilized
		case 0:
		{
			v1 = static_cast<float>(panimvalue[frame + 1].value) * scale; // + 1 to skip over the base animvalue.

			return;
		}
		// '1' valid values is a new type of compression, the track consists of one short, followed by a series of signed 8bit integers used to adjust the value after the first frame.
		case 1:
		{
			int16_t value = panimvalue[1].value;

			// there is a signed char for every frame after index 0 to adjust the basic value, this limits changes to be +-127 compressed
			if (frame > 0)
				value += reinterpret_cast<const char*>(&panimvalue[2])[frame - 1]; // the char values start after the base

			v1 = static_cast<float>(value) * scale;

			return;
		}
		// 'other' valid values is a new type of compression, but can also be used to have a single value across many frames due to how it is setup
		default:
		{
			float v7 = 1.0f;

			// note: we are subtracting two, as if it isn't above two, we should only have the single base value
			const int bitLUTIndex = (panimvalue->num.valid - 2) / 6; // how many bits are we storing per frame, if any.
			const int numUnkValue = (panimvalue->num.valid - 2) % 6; // number of 16 bit values on top of the base value, unsure what these are

			float value = static_cast<float>(panimvalue[1].value); // the base value after our animvalue

			const float cycle = static_cast<float>(frame) / static_cast<float>(panimvalue->num.total - 1); // progress through this block

			// this is very cursed and I am unsure of why it works, the 16 bit values are all over the place, perhaps these are a curve of sorts?
			for (int i = 0; i < numUnkValue; i++)
			{
				v7 *= cycle;
				value += static_cast<float>(panimvalue[i + 2].value) * v7; // + 2, skipping the animvalue, and the first base value, trailing values are for modification
			}

			// this is pretty straight forward, like with '1', we store adjustments to the base value saving space by packing them into a smaller value. the adjustments seem to be micro tweaks on top of how whatever unknown 16 bit values do
			if (bitLUTIndex)
			{
				const int16_t* pbits = &panimvalue[numUnkValue + 2].value; // + 2, skipping the animvalue, the first base value, and the unknown values, placing us at the bits

				value -= s_FrameValOffsetLUT[bitLUTIndex]; // subtract the max value that can be contained within our bitfield

				// number of bits per value adjustment
				const uint8_t maskBitCount = s_FrameBitCountLUT[bitLUTIndex];

				// data mask for value bits
				const uint8_t mask = (1 << maskBitCount) - 1;

				// get the bit offset to the value for this frame
				const int frameValueBitOffset = frame * maskBitCount;

				// number of bits stored in a "mstudioanimvalue_t" value
				constexpr int animValueBitSize = (8 * sizeof(mstudioanimvalue_t));
				static_assert(animValueBitSize == 16);

				// offset must be contained within the 16 bits of mstudioanimvalue_t::value, so only the lower 4 bits of the value are kept (bitmask 0xF)
				const int bitOffset = frameValueBitOffset & (animValueBitSize - 1);

				value += 2.0f * (mask & (pbits[frameValueBitOffset >> 4] >> bitOffset)); // we shift frameValueBitOffset to index for an int16_t pointer, then we offset into that int16_t and mask off the bits we wish to keep. some accuracy loss here considering we mult by two
			}

			v1 = value * scale;

			return;
		}
		}
	}

	void ExtractAnimValue(int frame, const mstudioanimvalue_t* panimvalue, float scale, float& v1)
	{
		int k = frame;

		// [rika]: for obvious reasons this func gets inlined in CalcBone functions
		while (panimvalue->num.total <= k)
		{
			k -= panimvalue->num.total;
			panimvalue += GetAnimValueOffset(panimvalue);
		}

		ExtractAnimValue(panimvalue, k, scale, v1);
	}

	void ExtractAnimValue(int frame, const mstudioanimvalue_t* panimvalue, float scale, float& v1, float& v2)
	{
		int k = frame;

		// find the data list that has the frame
		while (panimvalue->num.total <= k)
		{
			k -= panimvalue->num.total;
			panimvalue += GetAnimValueOffset(panimvalue); // [rika]: this is a macro because I thought it was used more than once initally
		}

		// [rika]: check if our track index is the last value, if it is, our second value (v2) gets taken from the first frame of the next track
		if (k >= panimvalue->num.total - 1)
		{
			ExtractAnimValue(panimvalue, k, scale, v1);
			ExtractAnimValue(panimvalue + GetAnimValueOffset(panimvalue), 0, scale, v2); // index 0 for the first frame, seek to next track with macro
		}
		else
		{
			// inlined in apex but it's actually just two ExtractAnimValue functions
			ExtractAnimValue(panimvalue, k, scale, v1);
			ExtractAnimValue(panimvalue, k + 1, scale, v2);
		}
	}

	void CalcBoneQuaternion(int frame, float s, const mstudio_rle_anim_t* panim, const RadianEuler& baseRot, Quaternion& q, const uint8_t boneFlags)
	{
		if (!(panim->flags & RleFlags_t::STUDIO_ANIM_ANIMROT))
		{
			q = *(panim->pQuat64(boneFlags));
			assert(q.IsValid());
			return;
		}

		mstudioanim_valueptr_t* pRotV = panim->pRotV(boneFlags);
		int j;

		mstudioanimvalue_t* values[3] = { pRotV->pAnimvalue(), pRotV->pAnimvalue() + pRotV->axisIdx1, pRotV->pAnimvalue() + pRotV->axisIdx2 };

		// don't interpolate if first frame
		if (s == 0.0f)
		{
			RadianEuler angle;

			for (j = 0; j < 3; j++)
			{
				if (pRotV->flags & (ValuePtrFlags_t::STUDIO_ANIMPTR_X >> j))
				{
					ExtractAnimValue(frame, values[j], FIXED_ROT_SCALE, angle[j]);
				}
				else
				{
					// check this
					angle[j] = baseRot[j];
				}
			}

			assert(angle.IsValid());
			AngleQuaternion(angle, q);
		}
		else
		{
			Quaternion	q1, q2; // QuaternionAligned
			RadianEuler angle1, angle2;

			for (j = 0; j < 3; j++)
			{
				// extract anim value doesn't catch nullptrs like normal source, so we have to be careful
				if (pRotV->flags & (ValuePtrFlags_t::STUDIO_ANIMPTR_X >> j))
				{
					ExtractAnimValue(frame, values[j], FIXED_ROT_SCALE, angle1[j], angle2[j]);
				}
				else
				{
					// unsure
					angle1[j] = baseRot[j];
					angle2[j] = baseRot[j];
				}
			}

			// disassembly is flipped
			assert(angle1.IsValid() && angle2.IsValid());
			if (angle1.x != angle2.x || angle1.y != angle2.y || angle1.z != angle2.z)
			{
				AngleQuaternion(angle1, q1);
				AngleQuaternion(angle2, q2);

				QuaternionBlend(q1, q2, s, q);
			}
			else
			{
				AngleQuaternion(angle1, q);
			}
		}

		assert(q.IsValid());
	}

	void CalcBonePosition(int frame, float s, const mstudio_rle_anim_t* panim, Vector& pos)
	{
		if (!(panim->flags & RleFlags_t::STUDIO_ANIM_ANIMPOS))
		{
			pos = *(panim->pPos());
			assert(pos.IsValid());

			return;
		}

		mstudioanim_valueptr_t* pPosV = panim->pPosV();
		int j;

		mstudioanimvalue_t* values[3] = { pPosV->pAnimvalue(), pPosV->pAnimvalue() + pPosV->axisIdx1, pPosV->pAnimvalue() + pPosV->axisIdx2 };
		Vector tmp(0.0f); // temp to load our values into, pos is passed in with data in apex (delta is checked externally)

		// don't interpolate if first frame
		if (s == 0.0f)
		{
			for (j = 0; j < 3; j++)
			{
				if (pPosV->flags & (ValuePtrFlags_t::STUDIO_ANIMPTR_X >> j))
				{
					ExtractAnimValue(frame, values[j], panim->PosScale(), tmp[j]);
				}
			}
		}
		else
		{
			float v1, v2;
			for (j = 0; j < 3; j++)
			{
				// extract anim value doesn't catch nullptrs like normal source, so we have to be careful
				if (pPosV->flags & (ValuePtrFlags_t::STUDIO_ANIMPTR_X >> j))
				{
					ExtractAnimValue(frame, values[j], panim->PosScale(), v1, v2);
					tmp[j] = v1 * (1.0f - s) + v2 * s;
				}
			}
		}

		pos += tmp; // add our values onto the existing pos

		assert(pos.IsValid());
	}

	void CalcBoneScale(int frame, float s, const mstudio_rle_anim_t* panim, Vector& scale, const uint8_t boneFlags)
	{
		// basically the same as pos with the exception of a 'boneFlags' var so we can see what data this rle header has
		// apex just passes a pointer to the data but I am using funcs like normal source
		if (!(panim->flags & RleFlags_t::STUDIO_ANIM_ANIMSCALE))
		{
			scale = *(panim->pScale(boneFlags));
			assert(scale.IsValid());

			return;
		}

		mstudioanim_valueptr_t* pScaleV = panim->pScaleV(boneFlags);
		int j;

		mstudioanimvalue_t* values[3] = { pScaleV->pAnimvalue(), pScaleV->pAnimvalue() + pScaleV->axisIdx1, pScaleV->pAnimvalue() + pScaleV->axisIdx2 };
		Vector tmp(0.0f); // temp to load our values into, scale is passed in with data in apex (delta is checked externally)

		// don't interpolate if first frame
		if (s == 0.0f)
		{
			for (j = 0; j < 3; j++)
			{
				if (pScaleV->flags & (ValuePtrFlags_t::STUDIO_ANIMPTR_X >> j))
				{
					ExtractAnimValue(frame, values[j], FIXED_SCALE_SCALE, tmp[j]);
				}
			}
		}
		else
		{
			float v1, v2;
			for (j = 0; j < 3; j++)
			{
				// extract anim value doesn't catch nullptrs like normal source, so we have to be careful
				if (pScaleV->flags & (ValuePtrFlags_t::STUDIO_ANIMPTR_X >> j))
				{
					ExtractAnimValue(frame, values[j], FIXED_SCALE_SCALE, v1, v2);
					tmp[j] = v1 * (1.0f - s) + v2 * s;
				}
			}
		}

		scale += tmp; // what?? are the scale values stored differently in apex? delta vs absolute

		assert(scale.IsValid());
	}

	//
	// DATAPOINT ANIMATION (Model V19)
	//

	// uses base datapoints with offsets and interps to get the proper values
	void CalcBoneQuaternion_DP(const int sectionlength, const uint8_t** panimtrack, const float fFrame, Quaternion& q)
	{
		const uint8_t* ptrack = *reinterpret_cast<const uint8_t** const>(panimtrack);

		const uint8_t valid = ptrack[0];
		const uint8_t total = ptrack[1];

		const uint8_t* pFrameIndices = ptrack + 2;

		int prevFrame = 0, nextFrame = 0;
		float s = 0.0f; // always init as 0!

		// [rika]: get data pointers
		const AnimQuat32* pPackedData = nullptr;
		const AxisFixup_t* pAxisFixup = nullptr;

		CalcBoneInterpFrames_DP(prevFrame, nextFrame, s, fFrame, total, sectionlength, pFrameIndices, &pPackedData);

		pAxisFixup = reinterpret_cast<const AxisFixup_t*>(pPackedData + valid);

		// [rika]: see to our datapoint
		int validIdx = 0;
		uint32_t remainingFrames = 0;

		const uint32_t prevTarget = prevFrame;
		CalcBoneSeek_DP(pPackedData, validIdx, remainingFrames, /*valid,*/ prevTarget);

		Quaternion q1;
		AnimQuat32::Unpack(q1, pPackedData[validIdx], &pAxisFixup[prevFrame]);

		// [rika]: check if we need interp or not
		if (prevFrame == nextFrame)
		{
			q = q1;
		}
		else
		{
			// [rika]: see to our datapoint (the sequal)
			const uint32_t nextTarget = (nextFrame - prevFrame) + remainingFrames; // to check if our current data will contain this interp data, seek until we have it
			CalcBoneSeek_DP(pPackedData, validIdx, remainingFrames, /*valid,*/ nextTarget);

			Quaternion q2;
			AnimQuat32::Unpack(q2, pPackedData[validIdx], &pAxisFixup[nextFrame]);

			QuaternionSlerp(q1, q2, s, q);
		}

		assertm(q.IsValid(), "invalid quaternion");

		// [rika]: advance the data ptr for other functions
		*panimtrack = reinterpret_cast<const uint8_t*>(pAxisFixup + total);
	}

	void CalcBonePosition_DP(const int sectionlength, const uint8_t** panimtrack, const float fFrame, Vector& pos)
	{
		const uint8_t* ptrack = *reinterpret_cast<const uint8_t** const>(panimtrack);

		const uint8_t valid = ptrack[0];
		const uint8_t total = ptrack[1];

		const uint8_t* pFrameIndices = ptrack + 2;
		*panimtrack = reinterpret_cast<const uint8_t*>(pFrameIndices);

		// [rika]: return zeros if no data
		if (!total)
		{
			pos.Init(0.0f, 0.0f, 0.0f);
			return;
		}

		int prevFrame = 0, nextFrame = 0;
		float s = 0.0f; // always init as 0!

		// [rika]: get data pointers
		const AnimPos64* pPackedData = nullptr;
		const AxisFixup_t* pAxisFixup = nullptr;

		CalcBoneInterpFrames_DP(prevFrame, nextFrame, s, fFrame, total, sectionlength, pFrameIndices, &pPackedData);

		pAxisFixup = reinterpret_cast<const AxisFixup_t*>(pPackedData + valid);

		// [rika]: see to our datapoint
		int validIdx = 0;
		uint32_t remainingFrames = 0;

		const uint32_t prevTarget = prevFrame;
		CalcBoneSeek_DP(pPackedData, validIdx, remainingFrames, /*valid,*/ prevTarget);

		Vector pos1;
		AnimPos64::Unpack(pos1, pPackedData[validIdx], &pAxisFixup[prevFrame]);

		// [rika]: check if we need interp or not
		if (prevFrame == nextFrame)
		{
			pos = pos1;
		}
		else
		{
			// [rika]: see to our datapoint (the sequal)
			const uint32_t nextTarget = (nextFrame - prevFrame) + remainingFrames; // to check if our current data will contain this interp data, seek until we have it
			CalcBoneSeek_DP(pPackedData, validIdx, remainingFrames, /*valid,*/ nextTarget);

			Vector pos2;
			AnimPos64::Unpack(pos2, pPackedData[validIdx], &pAxisFixup[nextFrame]);

			pos = pos1 * (1.0f - s) + pos2 * s;
		}

		// [rika]: advance the data ptr for other functions
		*panimtrack = reinterpret_cast<const uint8_t*>(pAxisFixup + total);
	}

	void CalcBonePositionVirtual_DP(const int sectionlength, const uint8_t** panimtrack, const float fFrame, Vector& pos)
	{
		const uint8_t* ptrack = *reinterpret_cast<const uint8_t** const>(panimtrack);

		const uint8_t total = ptrack[0];

		const float16 packedscale = *reinterpret_cast<const float16* const>(ptrack + 1);
		const float posscale = packedscale.GetFloat() / 127.0f;

		const uint8_t* pFrameIndices = ptrack + 3;

		int prevFrame = 0, nextFrame = 0;
		float s = 0.0f; // always init as 0!

		// [rika]: get data pointers
		const AxisFixup_t* pAxisFixup = nullptr;

		CalcBoneInterpFrames_DP(prevFrame, nextFrame, s, fFrame, total, sectionlength, pFrameIndices, &pAxisFixup);

		// [rika]: check if we need interp or not
		if (prevFrame == nextFrame)
		{
			pos = pAxisFixup[prevFrame].ToVector(posscale);
		}
		else
		{
			const Vector pos1(pAxisFixup[prevFrame].ToVector(posscale));
			const Vector pos2(pAxisFixup[nextFrame].ToVector(posscale));

			pos = pos1 * (1.0f - s) + pos2 * s;
		}

		// [rika]: advance the data ptr for other functions
		*panimtrack = reinterpret_cast<const uint8_t*>(pAxisFixup + total);
	}

	void CalcBoneScale_DP(const int sectionlength, const uint8_t** panimtrack, const float fFrame, Vector& scale)
	{
		const uint8_t* ptrack = *reinterpret_cast<const uint8_t** const>(panimtrack);

		const uint8_t total = ptrack[0];

		const uint8_t* pFrameIndices = ptrack + 1;

		int prevFrame = 0, nextFrame = 0;
		float s = 0.0f; // always init as 0!

		// [rika]: get data pointers
		const Vector48* pPackedData = nullptr;

		CalcBoneInterpFrames_DP(prevFrame, nextFrame, s, fFrame, total, sectionlength, pFrameIndices, &pPackedData);

		// [rika]: check if we need interp or not
		if (prevFrame == nextFrame)
		{
			scale = pPackedData[prevFrame].AsVector();
		}
		else
		{
			Vector scale1(pPackedData[prevFrame].AsVector());
			Vector scale2(pPackedData[nextFrame].AsVector());

			scale = scale1 * (1.0f - s) + scale2 * s;
		}

		// [rika]: advance the data ptr for other functions
		*panimtrack = reinterpret_cast<const uint8_t*>(pPackedData + total);
	}

	// newer revision of functions, seemingly more 'optimized' by skipping certain interp parsing if unneeded
	void CalcBoneQuaternion_DP(const bool hasFlagTmp, const int sectionlength, const uint8_t** panimtrack, const float fFrame, Quaternion& q)
	{
		const uint8_t* ptrack = *reinterpret_cast<const uint8_t** const>(panimtrack);

		uint8_t valid = 1;
		uint8_t total = 1;

		int prevFrame = 0, nextFrame = 0;
		float s = 1.0f; // previously init as 0.0f, new version is 1.0f!

		// [rika]: get data pointers
		const AnimQuat32* pPackedData = reinterpret_cast<const AnimQuat32* const>(ptrack + 2);

		if (hasFlagTmp == false)
		{
			valid = ptrack[0];
			total = ptrack[1];

			if (total > 1)
			{
				const uint8_t* const pFrameIndices = ptrack + 2;

				CalcBoneInterpFrames_DP(prevFrame, nextFrame, s, fFrame, total, sectionlength, pFrameIndices, &pPackedData);
			}
		}

		const AxisFixup_t* pAxisFixup = reinterpret_cast<const AxisFixup_t*>(pPackedData + valid);

		// [rika]: see to our datapoint
		int validIdx = 0;
		uint32_t remainingFrames = 0;

		const uint32_t prevTarget = prevFrame;
		CalcBoneSeek_DP(pPackedData, validIdx, remainingFrames, /*valid,*/ prevTarget);

		Quaternion q1;
		AnimQuat32::Unpack(q1, pPackedData[validIdx], &pAxisFixup[prevFrame]);

		// [rika]: check if we need interp or not
		if (prevFrame == nextFrame)
		{
			q = q1;
		}
		else
		{
			// [rika]: see to our datapoint (the sequal)
			const uint32_t nextTarget = (nextFrame - prevFrame) + remainingFrames; // to check if our current data will contain this interp data, seek until we have it
			CalcBoneSeek_DP(pPackedData, validIdx, remainingFrames, /*valid,*/ nextTarget);

			Quaternion q2;
			AnimQuat32::Unpack(q2, pPackedData[validIdx], &pAxisFixup[nextFrame]);

			QuaternionSlerp(q1, q2, s, q);
		}

		assertm(q.IsValid(), "invalid quaternion");

		// [rika]: advance the data ptr for other functions
		*panimtrack = reinterpret_cast<const uint8_t*>(pAxisFixup + total);
	}

	void CalcBonePosition_DP(const bool hasFlagTmp, const int sectionlength, const uint8_t** panimtrack, const float fFrame, Vector& pos)
	{
		const uint8_t* ptrack = *reinterpret_cast<const uint8_t** const>(panimtrack);

		const uint8_t valid = ptrack[0];
		uint8_t total = 1;

		int prevFrame = 0, nextFrame = 0;
		float s = 1.0f; // previously init as 0.0f, new version is 1.0f!

		// [rika]: get data pointers
		const AnimPos64* pPackedData = reinterpret_cast<const AnimPos64* const>(ptrack + 1);

		if (hasFlagTmp == false)
		{
			total = ptrack[1];

			// [rika]: return zeros if no data
			if (!total)
			{
				pos.Init(0.0f, 0.0f, 0.0f);
				return;
			}

			if (total > 1)
			{
				const uint8_t* const pFrameIndices = ptrack + 2;

				CalcBoneInterpFrames_DP(prevFrame, nextFrame, s, fFrame, total, sectionlength, pFrameIndices, &pPackedData);
			}
		}

		if (valid)
		{
			const AxisFixup_t* pAxisFixup = reinterpret_cast<const AxisFixup_t*>(pPackedData + valid);

			// [rika]: see to our datapoint
			int validIdx = 0;
			uint32_t remainingFrames = 0;

			const uint32_t prevTarget = prevFrame;
			CalcBoneSeek_DP(pPackedData, validIdx, remainingFrames, /*valid,*/ prevTarget);

			Vector pos1;
			AnimPos64::Unpack(pos1, pPackedData[validIdx], &pAxisFixup[prevFrame]);

			// [rika]: check if we need interp or not
			if (prevFrame == nextFrame)
			{
				pos = pos1;
			}
			else
			{
				// [rika]: see to our datapoint (the sequal)
				const uint32_t nextTarget = (nextFrame - prevFrame) + remainingFrames; // to check if our current data will contain this interp data, seek until we have it
				CalcBoneSeek_DP(pPackedData, validIdx, remainingFrames, /*valid,*/ nextTarget);

				Vector pos2;
				AnimPos64::Unpack(pos2, pPackedData[validIdx], &pAxisFixup[nextFrame]);

				pos = pos1 * (1.0f - s) + pos2 * s;
			}

			// [rika]: advance the data ptr for other functions
			*panimtrack = reinterpret_cast<const uint8_t*>(pAxisFixup + total);
		}
		else
		{
			const AnimPos48* const pPosValues = reinterpret_cast<const AnimPos48* const>(pPackedData);

			const Vector pos1(pPosValues[prevFrame].values[0], pPosValues[prevFrame].values[1], pPosValues[prevFrame].values[2]);

			if (prevFrame == nextFrame)
			{
				pos = pos1;
			}
			else
			{
				Vector pos2(pPosValues[nextFrame].values[0], pPosValues[nextFrame].values[1], pPosValues[nextFrame].values[2]);

				pos = pos1 + ((pos2 - pos1) * s);
			}

			pos *= 0.0099999998f;

			*panimtrack = reinterpret_cast<const uint8_t*>(pPosValues + total);
		}
	}

	void CalcBonePositionVirtual_DP(const bool hasFlagTmp, const int sectionlength, const uint8_t** panimtrack, const float fFrame, Vector& pos)
	{
		const uint8_t* ptrack = *reinterpret_cast<const uint8_t** const>(panimtrack);

		uint8_t total = 1;
		float posscale = 0.0f;


		int prevFrame = 0, nextFrame = 0;
		float s = 1.0f; // always init as 0!

		// [rika]: get data pointers
		const AxisFixup_t* pAxisFixup = nullptr;

		if (hasFlagTmp)
		{
			const int packedscale = ptrack[0];
			posscale = static_cast<float>(packedscale) / 127.0f;
			pAxisFixup = reinterpret_cast<const AxisFixup_t* const>(ptrack + 1);
		}
		else
		{
			total = ptrack[0];

			const int packedscale = ptrack[1];
			posscale = static_cast<float>(packedscale) / 127.0f;
			pAxisFixup = reinterpret_cast<const AxisFixup_t* const>(ptrack + 2);

			if (total > 1)
			{
				const uint8_t* pFrameIndices = ptrack + 2;

				CalcBoneInterpFrames_DP(prevFrame, nextFrame, s, fFrame, total, sectionlength, pFrameIndices, &pAxisFixup);
			}
		}

		// [rika]: check if we need interp or not
		if (prevFrame == nextFrame)
		{
			pos = pAxisFixup[prevFrame].ToVector(posscale);
		}
		else
		{
			const Vector pos1(pAxisFixup[prevFrame].ToVector(posscale));
			const Vector pos2(pAxisFixup[nextFrame].ToVector(posscale));

			pos = pos1 * (1.0f - s) + pos2 * s;
		}

		// [rika]: advance the data ptr for other functions
		*panimtrack = reinterpret_cast<const uint8_t*>(pAxisFixup + total);
	}

	void CalcBoneScale_DP(const bool hasFlagTmp, const int sectionlength, const uint8_t** panimtrack, const float fFrame, Vector& scale)
	{
		const uint8_t* ptrack = *reinterpret_cast<const uint8_t** const>(panimtrack);

		uint8_t total = 1;

		int prevFrame = 0, nextFrame = 0;
		float s = 1.0f; // always init as 0!

		// [rika]: get data pointers
		const Vector48* pPackedData = reinterpret_cast<const Vector48* const>(ptrack + 1);

		if (hasFlagTmp == false)
		{
			total = ptrack[0];

			if (total > 1)
			{
				const uint8_t* pFrameIndices = ptrack + 1;

				CalcBoneInterpFrames_DP(prevFrame, nextFrame, s, fFrame, total, sectionlength, pFrameIndices, &pPackedData);
			}
		}

		// [rika]: check if we need interp or not
		if (prevFrame == nextFrame)
		{
			scale = pPackedData[prevFrame].AsVector();
		}
		else
		{
			Vector scale1(pPackedData[prevFrame].AsVector());
			Vector scale2(pPackedData[nextFrame].AsVector());

			scale = scale1 * (1.0f - s) + scale2 * s;
		}

		// [rika]: advance the data ptr for other functions
		*panimtrack = reinterpret_cast<const uint8_t*>(pPackedData + total);
	}

	void AnimQuat32::Unpack(Quaternion& quat, const AnimQuat32 packedQuat, const AxisFixup_t* const axisFixup)
	{
		const int scaleFac = packedQuat.scaleFactor;

		const float scaleComponent = scaleFac ? 0.011048543f : 0.0055242716f;
		const float scaleFixup = static_cast<float>(1 << scaleFac) * 0.000021924432f;

		const float axis0 = ((static_cast<float>(packedQuat.value0) + 0.5f) * scaleComponent) + (static_cast<float>(axisFixup->adjustment[0]) * scaleFixup);
		const float axis1 = ((static_cast<float>(packedQuat.value1) + 0.5f) * scaleComponent) + (static_cast<float>(axisFixup->adjustment[1]) * scaleFixup);
		const float axis2 = ((static_cast<float>(packedQuat.value2) + 0.5f) * scaleComponent) + (static_cast<float>(axisFixup->adjustment[2]) * scaleFixup);

		float droppedComponent = 0.0f;

		const float dotProduct = (axis0 * axis0) + (axis1 * axis1) + (axis2 * axis2);

		if (dotProduct < 1.0f)
		{
			const float dprem = 1.0f - dotProduct;
			if (dprem < 0.0)
				droppedComponent = sqrtf(dprem);
			else
				droppedComponent = FastSqrtFast(dprem); // fsqrt
		}

		switch (packedQuat.droppedAxis)
		{
		case 0:
		{
			quat.x = droppedComponent;
			quat.y = axis0;
			quat.z = axis1;
			quat.w = axis2;

			break;
		}
		case 1:
		{
			quat.x = axis0;
			quat.y = droppedComponent;
			quat.z = axis1;
			quat.w = axis2;

			break;
		}
		case 2:
		{
			quat.x = axis0;
			quat.y = axis1;
			quat.z = droppedComponent;
			quat.w = axis2;

			break;
		}
		case 3:
		{
			quat.x = axis0;
			quat.y = axis1;
			quat.z = axis2;
			quat.w = droppedComponent;

			break;
		}
		}
	}

	void AnimPos64::Unpack(Vector& pos, const AnimPos64 packedPos, const AxisFixup_t* const axisFixup)
	{
		const float scaleFac = static_cast<float>(packedPos.scaleFactor + 1);

		pos.x = ((static_cast<float>(axisFixup->adjustment[0]) / 100.0f) + static_cast<float>(packedPos.values[0])) * scaleFac;
		pos.y = ((static_cast<float>(axisFixup->adjustment[1]) / 100.0f) + static_cast<float>(packedPos.values[1])) * scaleFac;
		pos.z = ((static_cast<float>(axisFixup->adjustment[2]) / 100.0f) + static_cast<float>(packedPos.values[2])) * scaleFac;
	}

	// not definted when declared
	const mstudioanimdesc_v12_1_t* const mstudioseqdesc_v8_t::pAnimDesc_V12_1(const int i) const { return reinterpret_cast<const mstudioanimdesc_v12_1_t* const>((char*)this + AnimIndex(i)); }
	const mstudioanimdesc_v19_1_t* const mstudioseqdesc_v18_t::pAnimDesc_V19_1(const uint16_t i) const { return reinterpret_cast<const mstudioanimdesc_v19_1_t* const>((char*)this + AnimIndex(i)); }
}