#include <pch.h>
#include <game/rtech/utils/studio/studio.h>
#include <game/rtech/utils/studio/studio_r2.h>

namespace r2
{
	//-----------------------------------------------------------------------------
	// Purpose: return a sub frame rotation for a single bone
	//-----------------------------------------------------------------------------
	void CalcBoneQuaternion(int frame, float s, const Quaternion& baseQuat, const RadianEuler& baseRot, const Vector& baseRotScale, int iBaseFlags, const Quaternion& baseAlignment, const mstudio_rle_anim_t* panim, Quaternion& q)
	{
		if (panim->flags & STUDIO_ANIM_RAWROT)
		{
			q = *(panim->pQuat64());
			assert(q.IsValid());
			return;
		}

		if (panim->flags & STUDIO_ANIM_NOROT)
		{
			if (panim->flags & STUDIO_ANIM_DELTA)
			{
				q.Init(0.0f, 0.0f, 0.0f, 1.0f);
			}
			else
			{
				q = baseQuat;
			}
			return;
		}

		mstudioanim_valueptr_t* pValuesPtr = panim->pRotV();

		if (s > 0.001f)
		{
			Quaternion	q1, q2; // QuaternionAligned
			RadianEuler			angle1, angle2;

			r1::ExtractAnimValue(frame, pValuesPtr->pAnimvalue(0), baseRotScale.x, angle1.x, angle2.x);
			r1::ExtractAnimValue(frame, pValuesPtr->pAnimvalue(1), baseRotScale.y, angle1.y, angle2.y);
			r1::ExtractAnimValue(frame, pValuesPtr->pAnimvalue(2), baseRotScale.z, angle1.z, angle2.z);

			if (!(panim->flags & STUDIO_ANIM_DELTA))
			{
				angle1.x = angle1.x + baseRot.x;
				angle1.y = angle1.y + baseRot.y;
				angle1.z = angle1.z + baseRot.z;
				angle2.x = angle2.x + baseRot.x;
				angle2.y = angle2.y + baseRot.y;
				angle2.z = angle2.z + baseRot.z;
			}

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
		else
		{
			RadianEuler			angle;

			r1::ExtractAnimValue(frame, pValuesPtr->pAnimvalue(0), baseRotScale.x, angle.x);
			r1::ExtractAnimValue(frame, pValuesPtr->pAnimvalue(1), baseRotScale.y, angle.y);
			r1::ExtractAnimValue(frame, pValuesPtr->pAnimvalue(2), baseRotScale.z, angle.z);

			if (!(panim->flags & STUDIO_ANIM_DELTA))
			{
				angle.x = angle.x + baseRot.x;
				angle.y = angle.y + baseRot.y;
				angle.z = angle.z + baseRot.z;
			}

			assert(angle.IsValid());
			AngleQuaternion(angle, q);
		}

		assert(q.IsValid());

		// align to unified bone
		if (!(panim->flags & STUDIO_ANIM_DELTA) && (iBaseFlags & 0x100000))
		{
			QuaternionAlign(baseAlignment, q, q);
		}
	}

	void CalcBoneQuaternion(int frame, float s, const mstudiobone_t* pBone, const r1::mstudiolinearbone_t* pLinearBones, const mstudio_rle_anim_t* panim, Quaternion& q)
	{
		if (pLinearBones)
		{
			CalcBoneQuaternion(frame, s, *pLinearBones->pQuat(panim->bone), *pLinearBones->pRot(panim->bone), *pLinearBones->pRotScale(panim->bone), pLinearBones->flags(panim->bone), *pLinearBones->pQAlignment(panim->bone), panim, q);
		}
		else
		{
			CalcBoneQuaternion(frame, s, pBone->quat, pBone->rot, pBone->rotscale, pBone->flags, pBone->qAlignment, panim, q);
		}
	}

	void CalcBonePosition(int frame, float s, const Vector& basePos, const float& baseBoneScale, const mstudio_rle_anim_t* panim, Vector& pos)
	{
		if (panim->flags & STUDIO_ANIM_RAWPOS)
		{
			pos = *(panim->pPos());
			assert(pos.IsValid());

			return;
		}

		mstudioanim_valueptr_t* pPosV = panim->pPosV();
		int					j;

		if (s > 0.001f)
		{
			float v1, v2;
			for (j = 0; j < 3; j++)
			{
				r1::ExtractAnimValue(frame, pPosV->pAnimvalue(j), baseBoneScale, v1, v2);
				pos[j] = v1 * (1.0f - s) + v2 * s;
			}
		}
		else
		{
			for (j = 0; j < 3; j++)
			{
				r1::ExtractAnimValue(frame, pPosV->pAnimvalue(j), baseBoneScale, pos[j]);
			}
		}

		if (!(panim->flags & STUDIO_ANIM_DELTA))
		{
			pos.x = pos.x + basePos.x;
			pos.y = pos.y + basePos.y;
			pos.z = pos.z + basePos.z;
		}

		assert(pos.IsValid());
	}

	void CalcBonePosition(int frame, float s, const mstudiobone_t* pBone, const r1::mstudiolinearbone_t* pLinearBones, const mstudio_rle_anim_t* panim, Vector& pos)
	{
		if (pLinearBones)
		{
			CalcBonePosition(frame, s, *pLinearBones->pPos(panim->bone), panim->posscale, panim, pos);
		}
		else
		{
			CalcBonePosition(frame, s, pBone->pos, panim->posscale, panim, pos);
		}
	}

	void CalcBoneScale(int frame, float s, const Vector& baseScale, const Vector& baseScaleScale, const mstudio_rle_anim_t* panim, Vector& scale)
	{
		if (panim->flags & STUDIO_ANIM_RAWSCALE)
		{
			scale = *(panim->pScale());
			assert(scale.IsValid());

			return;
		}

		mstudioanim_valueptr_t* pScaleV = panim->pScaleV();
		int					j;

		if (s > 0.001f)
		{
			float v1, v2;
			for (j = 0; j < 3; j++)
			{
				r1::ExtractAnimValue(frame, pScaleV->pAnimvalue(j), baseScaleScale[j], v1, v2);
				scale[j] = v1 * (1.0f - s) + v2 * s;
			}
		}
		else
		{
			for (j = 0; j < 3; j++)
			{
				r1::ExtractAnimValue(frame, pScaleV->pAnimvalue(j), baseScaleScale[j], scale[j]);
			}
		}

		if (!(panim->flags & STUDIO_ANIM_DELTA))
		{
			scale.x = scale.x + baseScale.x;
			scale.y = scale.y + baseScale.y;
			scale.z = scale.z + baseScale.z;
		}

		assert(scale.IsValid());
	}
}