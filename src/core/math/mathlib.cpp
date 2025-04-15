#include <pch.h>

#include <core/math/mathlib.h>

// https://github.com/ValveSoftware/source-sdk-2013/blob/master/sp/src/mathlib/mathlib_base.cpp#L3466
float AngleDiff(float destAngle, float srcAngle)
{
	float delta = fmodf(destAngle - srcAngle, 360.0f);
	if (destAngle > srcAngle)
	{
		if (delta >= 180)
			delta -= 360;
	}
	else
	{
		if (delta <= -180)
			delta += 360;
	}
	return delta;
}