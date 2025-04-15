#include <pch.h>
#include <game/rtech/utils/studio/studio.h>
#include <game/rtech/utils/studio/studio_r5.h>

namespace r5
{
	static char s_FrameBitCountLUT[4]{ 0, 2, 4, 0 }; // 0x1412FC154i64 (12FC154h)
	static float s_FrameValOffsetLUT[4]{ 0.0f, 3.0f, 15.0f, 0.0f }; // dword_1412FED60 (12FED60) (1 << s_FrameBitCountLUT[i] ==  s_FrameValOffsetLUT[i])
	static char s_AnimSeekLUT[60]
	{
		1,  15, 16, 2,  7,  8,  2,  15, 0,  3,  15, 0,  4,  15, 0,  5,
		15, 0,  6,  15, 0,  7,  15, 0,  2,  15, 2,  3,  15, 2,  4,  15,
		2,  5,  15, 2,  6,  15, 2,  7,  15, 2,  2,  15, 4,  3,  15, 4,
		4,  15, 4, 5, 15, 4, 6, 15, 4,  7,  15, 4
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

	inline void ExtractAnimValue(mstudioanimvalue_t* panimvalue, const int frame, const float scale, float& v1)
	{
		// note: in R5 'valid' is not really used the same way as traditional source.

		// '0' valid values is treated as the traditional source style track, having a 16bit signed integer per frame value. this will be used if the more compact tracks cannot be utilized
		if (!panimvalue->num.valid)
		{
			v1 = static_cast<float>(panimvalue[frame + 1].value) * scale; // + 1 to skip over the base animvalue.

			return;
		}

		// '1' valid values is a new type of compression, the track consists of one short, followed by a series of signed 8bit integers used to adjust the value after the first frame.
		if (panimvalue->num.valid == 1)
		{
			int16_t value = panimvalue[1].value;

			// there is a signed char for every frame after index 0 to adjust the basic value, this limits changes to be +-127 compressed
			if (frame > 0)
				value += reinterpret_cast<char*>(&panimvalue[2])[frame - 1]; // the char values start after the base

			v1 = static_cast<float>(value) * scale;

			return;
		}
		else
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

	void ExtractAnimValue(int frame, mstudioanimvalue_t* panimvalue, float scale, float& v1)
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

	void ExtractAnimValue(int frame, mstudioanimvalue_t* panimvalue, float scale, float& v1, float& v2)
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

				assert(angle.IsValid());
				AngleQuaternion(angle, q);
			}
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
}