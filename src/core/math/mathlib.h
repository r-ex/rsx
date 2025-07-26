#pragma once
#include <DirectXMath.h>
using namespace DirectX;

#define DEG2RAD(x) (static_cast<float>(x) * (XM_PI / 180.f))
#define RAD2DEG(x) (static_cast<float>(x) * (180.f / XM_PI))

#if MATH_SIMD

// [rika]: I do not like these
#define SubFloat(m, i) (m.m128_f32[i])
#define SubInt(m, i) (m.m128_i32[i])

#define BINOP(op) 														\
	__m128 retVal;                                          				\
	SubFloat( retVal, 0 ) = ( SubFloat( a, 0 ) op SubFloat( b, 0 ) );	\
	SubFloat( retVal, 1 ) = ( SubFloat( a, 1 ) op SubFloat( b, 1 ) );	\
	SubFloat( retVal, 2 ) = ( SubFloat( a, 2 ) op SubFloat( b, 2 ) );	\
	SubFloat( retVal, 3 ) = ( SubFloat( a, 3 ) op SubFloat( b, 3 ) );	\
    return retVal;

#define IBINOP(op) 														\
	__m128 retVal;														\
	SubInt( retVal, 0 ) = ( SubInt( a, 0 ) op SubInt ( b, 0 ) );		\
	SubInt( retVal, 1 ) = ( SubInt( a, 1 ) op SubInt ( b, 1 ) );		\
	SubInt( retVal, 2 ) = ( SubInt( a, 2 ) op SubInt ( b, 2 ) );		\
	SubInt( retVal, 3 ) = ( SubInt( a, 3 ) op SubInt ( b, 3 ) );		\
    return retVal;

__forceinline __m128 OrSIMD(const __m128& a, const __m128& b)				// a | b
{
	IBINOP(| );
}

__forceinline __m128 AndSIMD(const __m128& a, const __m128& b)				// a & b
{
	IBINOP(&);
}

__forceinline __m128 AndNotSIMD(const __m128& a, const __m128& b)			// ~a & b
{
	__m128 retVal;
	SubInt(retVal, 0) = ~SubInt(a, 0) & SubInt(b, 0);
	SubInt(retVal, 1) = ~SubInt(a, 1) & SubInt(b, 1);
	SubInt(retVal, 2) = ~SubInt(a, 2) & SubInt(b, 2);
	SubInt(retVal, 3) = ~SubInt(a, 3) & SubInt(b, 3);
	return retVal;
}

static constexpr __m128 simd_Four_Zeros = { 0.0f, 0.0f, 0.0f, 0.0f };
static constexpr __m128 simd_Four_NegZeroes = { -0.0f, -0.0f, -0.0f, -0.0f };
static constexpr __m128 simd_Four_PointFives = { 0.5f, 0.5f, 0.5f, 0.5f };
static constexpr __m128 simd_Four_Ones = { 1.0f, 1.0f, 1.0f, 1.0f };
static constexpr __m128 simd_Four_Threes = { 3.0f, 3.0f, 3.0f, 3.0f };

static constexpr __m128 simd_NegativeMask = { .m128_u32 = { 0x80000000, 0x80000000, 0x80000000, 0x80000000 } };

__forceinline __m128 MaskedAssign(const __m128& ReplacementMask, const __m128& NewValue, const __m128& OldValue)
{
	//return _mm_or_ps( _mm_add_ps(ReplacementMask, NewValue), _mm_andnot_ps(ReplacementMask, OldValue));
	return OrSIMD(AndSIMD(ReplacementMask, NewValue), AndNotSIMD(ReplacementMask, OldValue));
}

__forceinline __m128 ReplicateX4(float flValue)
{
	__m128 value = _mm_set_ss(flValue);
	return _mm_shuffle_ps(value, value, _MM_SHUFFLE(0, 0, 0, 0));
}

__forceinline __m128 MulSIMD(const __m128& a, const __m128& b)
{
	return _mm_mul_ps(a, b);
}

__forceinline __m128 AddSIMD(const __m128& a, const __m128& b)
{
	return _mm_add_ps(a, b);
}

__forceinline __m128 SubSIMD(const __m128& a, const __m128& b)
{
	return _mm_sub_ps(a, b);
}

// a*b + c
__forceinline __m128 MaddSIMD(const __m128& a, const __m128& b, const __m128& c)
{
	return AddSIMD(MulSIMD(a, b), c);
}

// 1/sqrt(a), more or less
__forceinline __m128 ReciprocalSqrtEstSIMD(const __m128& a)
{
	return _mm_rsqrt_ps(a);
}

// uses newton iteration for higher precision results than ReciprocalSqrtEstSIMD
// 1/sqrt(a)
__forceinline __m128 ReciprocalSqrtSIMD(const __m128& a)
{
	__m128 guess = ReciprocalSqrtEstSIMD(a);
	// newton iteration for 1/sqrt(a) : y(n+1) = 1/2 (y(n)*(3-a*y(n)^2));
	guess = MulSIMD(guess, SubSIMD(simd_Four_Threes, MulSIMD(a, MulSIMD(guess, guess))));
	guess = MulSIMD(simd_Four_PointFives, guess);
	return guess;
}

__forceinline const float DotSIMD(const __m128& a, const __m128& b)
{
	const __m128 product = _mm_mul_ps(a, b);
	const __m128 shfl0 = _mm_shuffle_ps(product, product, _MM_SHUFFLE(2, 3, 0, 1));

	const __m128 sum = _mm_add_ps(shfl0, product);
	const __m128 shfl1 = _mm_shuffle_ps(sum, sum, _MM_SHUFFLE(0, 1, 2, 3));

	const float flDot = shfl1.m128_f32[0] + sum.m128_f32[0];

	return flDot;
}

__forceinline __m128 Dot4SIMD(const __m128& a, const __m128& b)
{
	/*__m128 m = _mm_mul_ps(a, b);
	float flDot = SubFloat(m, 0) + SubFloat(m, 1) + SubFloat(m, 2) + SubFloat(m, 3);*/

	// [rika]: accurate within 0.0001, from inital tests
	const float flDot = DotSIMD(a, b);

	return ReplicateX4(flDot);
}

#endif // MATH_SIMD


enum Axis_t
{
	PITCH = 0,	// up / down
	YAW,		// left / right
	ROLL		// fall over
};

inline void SinCos(float x, float* fsin, float* fcos)
{
	*fsin = sin(x);
	*fcos = cos(x);
}

static __forceinline float __vectorcall FastSqrtFast(float x)
{
	__m128 root = _mm_sqrt_ss(_mm_load_ss(&x));
	return _mm_cvtss_f32(root);
};

static __forceinline float __vectorcall FastSqrtFast(const __m128& x)
{
	__m128 root = _mm_sqrt_ss(x);
	return _mm_cvtss_f32(root);
};

// reciprocal
static __forceinline float __vectorcall FastRSqrtFast(float x)
{
	__m128 rroot = _mm_rsqrt_ss(_mm_load_ss(&x));
	return _mm_cvtss_f32(rroot);
};

static __forceinline float __vectorcall FastRSqrtFast(const __m128& x)
{
	__m128 rroot = _mm_rsqrt_ss(x);
	return _mm_cvtss_f32(rroot);
};

float AngleDiff(float destAngle, float srcAngle);

class Vector;
class Quaternion;
class RadianEuler;
class QAngle;
struct matrix3x4_t;

// lovely chunk of quaternion functions from source
// https://github.com/ValveSoftware/source-sdk-2013/blob/master/sp/src/public/mathlib/mathlib.h#L594


void QuaternionBlend(const Quaternion& p, const Quaternion& q, float t, Quaternion& qt);
void QuaternionBlendNoAlign(const Quaternion& p, const Quaternion& q, float t, Quaternion& qt);



void QuaternionAlign(const Quaternion& p, const Quaternion& q, Quaternion& qt);



float QuaternionNormalize(Quaternion& q);


void QuaternionMatrix(const Quaternion& q, matrix3x4_t& matrix);
void QuaternionMatrix(const Quaternion& q, const Vector& pos, matrix3x4_t& matrix);
void QuaternionAngles(const Quaternion& q, QAngle& angles);
void AngleQuaternion(const QAngle& angles, Quaternion& qt);
void QuaternionAngles(const Quaternion& q, RadianEuler& angles);
void AngleQuaternion(RadianEuler const& angles, Quaternion& qt);