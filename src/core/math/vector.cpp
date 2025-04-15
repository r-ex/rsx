#include <pch.h>

#include <core/math/vector.h>

//========
// Vector
//========

// access
float Vector::operator[](int i) const
{
	assert(i >= 0 && 3 > i);
	return ((float*)this)[i];
}

float& Vector::operator[](int i)
{
	assert(i >= 0 && 3 > i);
	return ((float*)this)[i];
}

Vector& Vector::operator=(const Vector& vecIn)
{
	x = vecIn.x;
	y = vecIn.y;
	z = vecIn.z;

	return *this;
}

// math but with copying
Vector Vector::operator+(const Vector& vecIn) const
{
	Vector out;

	out.x = x + vecIn.x;
	out.y = y + vecIn.y;
	out.z = z + vecIn.z;

	return out;
}

Vector Vector::operator-(const Vector& vecIn) const
{
	Vector out;

	out.x = x - vecIn.x;
	out.y = y - vecIn.y;
	out.z = z - vecIn.z;

	return out;
}

Vector Vector::operator*(const Vector& vecIn) const
{
	Vector out;

	out.x = x * vecIn.x;
	out.y = y * vecIn.y;
	out.z = z * vecIn.z;

	return out;
}

Vector Vector::operator/(const Vector& vecIn) const
{
	Vector out;

	out.x = x / vecIn.x;
	out.y = y / vecIn.y;
	out.z = z / vecIn.z;

	return out;
}

Vector Vector::operator*(float inFl) const
{
	Vector out;

	out.x = x * inFl;
	out.y = y * inFl;
	out.z = z * inFl;

	return out;
}

Vector Vector::operator/(float inFl) const
{
	Vector out;

	out.x = x / inFl;
	out.y = y / inFl;
	out.z = z / inFl;

	return out;
}

Vector Vector::operator*(int in) const
{
	Vector out;

	out.x = x * in;
	out.y = y * in;
	out.z = z * in;

	return out;
}

Vector Vector::operator/(int in) const
{
	Vector out;

	out.x = x / in;
	out.y = y / in;
	out.z = z / in;

	return out;
}

const float Vector::Dot(const Vector& a, const Vector& b)
{
	return a.x * b.x + a.y * b.y + a.z * b.z;
}


//============
// Quaternion
//============

// access
float Quaternion::operator[](int i) const
{
	assert(i >= 0 && 4 > i);
	return ((float*)this)[i];
}

float& Quaternion::operator[](int i)
{
	assert(i >= 0 && 4 > i);
	return ((float*)this)[i];
}

Quaternion& Quaternion::operator=(const Quaternion& quatIn)
{
	x = quatIn.x;
	y = quatIn.y;
	z = quatIn.z;
	w = quatIn.w;

	return *this;
}


//=============
// RadianEuler
//=============

// access
float RadianEuler::operator[](int i) const
{
	assert(i >= 0 && 3 > i);
	return ((float*)this)[i];
}

float& RadianEuler::operator[](int i)
{
	assert(i >= 0 && 3 > i);
	return ((float*)this)[i];
}

RadianEuler& RadianEuler::operator=(const RadianEuler& angIn)
{
	x = angIn.x;
	y = angIn.y;
	z = angIn.z;

	return *this;
}


//========
// QAngle
//========

// access
float QAngle::operator[](int i) const
{
	assert(i >= 0 && 3 > i);
	return ((float*)this)[i];
}

float& QAngle::operator[](int i)
{
	assert(i >= 0 && 3 > i);
	return ((float*)this)[i];
}

QAngle& QAngle::operator=(const QAngle& qangIn)
{
	x = qangIn.x;
	y = qangIn.y;
	z = qangIn.z;

	return *this;
}

QAngle& QAngle::operator+=(const QAngle& qangIn)
{
	x += qangIn.x;
	y += qangIn.y;
	z += qangIn.z;

	return *this;
}

QAngle& QAngle::operator-=(const QAngle& qangIn)
{
	x -= qangIn.x;
	y -= qangIn.y;
	z -= qangIn.z;

	return *this;
}

QAngle& QAngle::operator*=(float scale)
{
	x *= scale;
	y *= scale;
	z *= scale;

	return *this;
}

QAngle& QAngle::operator/=(float scale)
{
	x /= scale;
	y /= scale;
	z /= scale;

	return *this;
}

QAngle QAngle::operator-() const
{
	QAngle out(-x, -y, -z);
	return out;
}

QAngle QAngle::operator+(const QAngle& qangIn) const
{
	QAngle out;

	out.x = x + qangIn.x;
	out.y = y + qangIn.y;
	out.z = z + qangIn.z;

	return out;
}

QAngle QAngle::operator-(const QAngle& qangIn) const
{
	QAngle out;

	out.x = x - qangIn.x;
	out.y = y - qangIn.y;
	out.z = z - qangIn.z;

	return out;
}

QAngle QAngle::operator*(float inFl) const
{
	QAngle out;

	out.x = x * inFl;
	out.y = y * inFl;
	out.z = z * inFl;

	return out;
}

QAngle QAngle::operator/(float inFl) const
{
	QAngle out;

	out.x = x / inFl;
	out.y = y / inFl;
	out.z = z / inFl;

	return out;
}


//======
// Post
//======

// this is all stuff that can only be setup once the classes have actually be declared. mostly 1:1 from source to match functionality\
// radian angle to quaternion
static const __m128 aqScaleRadians = _mm_set_ps1(0.5f);
void AngleQuaternion(const RadianEuler& angles, Quaternion& outQuat)
{
#if MATH_ASSERTS
	assert(angles.IsValid());
#endif // MATH_ASSERTS

	// pitch = x, yaw = y, roll = z
	float sr, sp, sy, cr, cp, cy;

#if MATH_SIMD
	__m128 radians, /*scale,*/ sine, cosine;
	radians = _mm_load_ps(angles.Base());
	//scale = ReplicateX4(0.5f);
	radians = _mm_mul_ps(radians, aqScaleRadians);

	sine = _mm_sincos_ps(&cosine, radians);

	// NOTE: The ordering here is *different* from the AngleQuaternion below
	// because p, y, r are not in the same locations in QAngle + RadianEuler. Yay!
	// [rika]: honestly this is so pain.. thinking about an enum for each
	sr = SubFloat(sine, 0);	sp = SubFloat(sine, 1);	sy = SubFloat(sine, 2);
	cr = SubFloat(cosine, 0);	cp = SubFloat(cosine, 1);	cy = SubFloat(cosine, 2);
#else
	SinCos(angles.z * 0.5f, &sy, &cy);
	SinCos(angles.y * 0.5f, &sp, &cp);
	SinCos(angles.x * 0.5f, &sr, &cr);
#endif

	float srXcp = sr * cp, crXsp = cr * sp;
	outQuat.x = srXcp * cy - crXsp * sy; // X
	outQuat.y = crXsp * cy + srXcp * sy; // Y

	float crXcp = cr * cp, srXsp = sr * sp;
	outQuat.z = crXcp * sy - srXsp * cy; // Z
	outQuat.w = crXcp * cy + srXsp * sy; // W (real component)

#if MATH_ASSERTS
	assert(outQuat.IsValid());
#endif // MATH_ASSERTS
}

// degree angle to quaternion
static const __m128 aqScaleDegrees = _mm_set_ps1(DEG2RAD(0.5f));
void AngleQuaternion(const QAngle& angles, Quaternion& outQuat)
{
#if MATH_ASSERTS
	assert(angles.IsValid());
#endif // MATH_ASSERTS

	// pitch = x, yaw = y, roll = z
	float sr, sp, sy, cr, cp, cy;

#if MATH_SIMD
	__m128 degrees, /*scale,*/ sine, cosine;
	degrees = _mm_load_ps(angles.Base());
	//scale = ReplicateX4(DEG2RAD(0.5f));
	degrees = _mm_mul_ps(degrees, aqScaleDegrees);

	sine = _mm_sincos_ps(&cosine, degrees);

	// NOTE: The ordering here is *different* from the AngleQuaternion above
	// because p, y, r are not in the same locations in QAngle + RadianEuler. Yay!
	sp = SubFloat(sine, 0);	sy = SubFloat(sine, 1);	sr = SubFloat(sine, 2);
	cp = SubFloat(cosine, 0);	cy = SubFloat(cosine, 1);	cr = SubFloat(cosine, 2);
#else
	SinCos(DEG2RAD(angles.y) * 0.5f, &sy, &cy);
	SinCos(DEG2RAD(angles.x) * 0.5f, &sp, &cp);
	SinCos(DEG2RAD(angles.z) * 0.5f, &sr, &cr);
#endif

	float srXcp = sr * cp, crXsp = cr * sp;
	outQuat.x = srXcp * cy - crXsp * sy; // X
	outQuat.y = crXsp * cy + srXcp * sy; // Y

	float crXcp = cr * cp, srXsp = sr * sp;
	outQuat.z = crXcp * sy - srXsp * cy; // Z
	outQuat.w = crXcp * cy + srXsp * sy; // W (real component)

#if MATH_ASSERTS
	assert(outQuat.IsValid());
#endif // MATH_ASSERTS
}

// quaternion to radian angle
void QuaternionAngles(const Quaternion& q, QAngle& angles)
{
#if MATH_ASSERTS
	assert(q.IsValid());
#endif // MATH_ASSERTS

	// LOOK HERE FOR QUATERNION ISSUES!!! ;3333
#if 1
	// FIXME: doing it this way calculates too much data, needs to do an optimized version...
	// [rika]: see above, kept running into issues with the other style though..
	matrix3x4_t matrix;
	QuaternionMatrix(q, matrix);
	MatrixAngles(matrix, angles);
#else
	float m11, m12, m13, m23, m33;

	m11 = (2.0f * q.w * q.w) + (2.0f * q.x * q.x) - 1.0f;
	m12 = (2.0f * q.x * q.y) + (2.0f * q.w * q.z);
	m13 = (2.0f * q.x * q.z) - (2.0f * q.w * q.y);
	m23 = (2.0f * q.y * q.z) + (2.0f * q.w * q.x);
	m33 = (2.0f * q.w * q.w) + (2.0f * q.z * q.z) - 1.0f;

	// FIXME: this code has a singularity near PITCH +-90
	angles[YAW] = RAD2DEG(atan2(m12, m11));
	angles[PITCH] = RAD2DEG(asin(-m13));
	angles[ROLL] = RAD2DEG(atan2(m23, m33));
#endif

#if MATH_ASSERTS
	assert(angles.IsValid());
#endif // MATH_ASSERTS
}

// quaternion to degree angle
void QuaternionAngles(const Quaternion& q, RadianEuler& angles)
{
#if MATH_ASSERTS
	assert(q.IsValid());
#endif // MATH_ASSERTS

	// FIXME: doing it this way calculates too much data, needs to do an optimized version...
	// [rika]: see above, kept running into issues with the other style though..
	matrix3x4_t matrix;
	QuaternionMatrix(q, matrix);
	MatrixAngles(matrix, angles);

#if MATH_ASSERTS
	assert(angles.IsValid());
#endif // MATH_ASSERTS
}


// Quaternion
Quaternion::Quaternion(RadianEuler const& angle)
{
	Init();
	AngleQuaternion(angle, *this);
}

// RadianEuler
RadianEuler::RadianEuler(Quaternion const& q)
{
	Init();
	QuaternionAngles(q, *this);
}

RadianEuler::RadianEuler(QAngle const& angles)
{
	Init(
		DEG2RAD(angles.z),
		DEG2RAD(angles.x),
		DEG2RAD(angles.y));
}

QAngle RadianEuler::ToQAngle() const
{
	return QAngle(
		RAD2DEG(y),
		RAD2DEG(z),
		RAD2DEG(x));
}

// QAngle
QAngle::QAngle(RadianEuler const& angIn)
{
	Init(
		RAD2DEG(angIn.y),
		RAD2DEG(angIn.z),
		RAD2DEG(angIn.x));
}

QAngle::QAngle(Quaternion const& quatIn)
{
	Init();
	QuaternionAngles(quatIn, *this);
}

RadianEuler QAngle::ToEuler() const
{
	return RadianEuler(
		DEG2RAD(z),
		DEG2RAD(x),
		DEG2RAD(y));
}



// Rotate a vector around the Z axis (YAW)
void VectorYawRotate(const Vector& in, float flYaw, Vector& out)
{
	if (&in == &out)
	{
		Vector tmp;
		tmp = in;
		VectorYawRotate(tmp, flYaw, out);
		return;
	}

	float sy, cy;

	SinCos(DEG2RAD(flYaw), &sy, &cy);

	out.x = in.x * cy - in.y * sy;
	out.y = in.x * sy + in.y * cy;
	out.z = in.z;
}

// these two should go in future math lib
void NormalizeAngles(QAngle& angles)
{
	int i;

	// Normalize angles to -180 to 180 range
	for (i = 0; i < 3; i++)
	{
		if (angles[i] > 180.0)
		{
			angles[i] -= 360.0;
		}
		else if (angles[i] < -180.0)
		{
			angles[i] += 360.0;
		}
	}
}

// do this better!
float QuaternionNormalize(Quaternion& q)
{
	float radius, iradius;

	assert(q.IsValid());

	radius = q[0] * q[0] + q[1] * q[1] + q[2] * q[2] + q[3] * q[3];

	if (radius) // > FLT_EPSILON && ((radius < 1.0f - 4*FLT_EPSILON) || (radius > 1.0f + 4*FLT_EPSILON))
	{
		__m128 _rad = _mm_load_ss(&radius);

		iradius = FastRSqrtFast(_rad);
		radius = FastSqrtFast(_rad);
		//iradius = 1.0f / radius;
		q[3] *= iradius;
		q[2] *= iradius;
		q[1] *= iradius;
		q[0] *= iradius;
	}
	return radius;
}

void QuaternionBlendNoAlign(const Quaternion& p, const Quaternion& q, float t, Quaternion& qt)
{
	float sclp, sclq;
	int i;

	// 0.0 returns p, 1.0 return q.
	sclp = 1.0f - t;
	sclq = t;
	for (i = 0; i < 4; i++) {
		qt[i] = sclp * p[i] + sclq * q[i];
	}
	QuaternionNormalize(qt);
}

void QuaternionAlign(const Quaternion& p, const Quaternion& q, Quaternion& qt)
{
	int i;
	// decide if one of the quaternions is backwards
	float a = 0;
	float b = 0;
	for (i = 0; i < 4; i++)
	{
		a += (p[i] - q[i]) * (p[i] - q[i]);
		b += (p[i] + q[i]) * (p[i] + q[i]);
	}
	if (a > b)
	{
		for (i = 0; i < 4; i++)
		{
			qt[i] = -q[i];
		}
	}
	else if (&qt != &q)
	{
		for (i = 0; i < 4; i++)
		{
			qt[i] = q[i];
		}
	}
}

#if MATH_SIMD
__forceinline __m128 QuaternionAlignSIMD(const __m128& p, const __m128& q)
{
	// decide if one of the quaternions is backwards
	__m128 a = _mm_sub_ps(p, q);
	__m128 b = _mm_add_ps(p, q);
	a = Dot4SIMD(a, a);
	b = Dot4SIMD(b, b);
	__m128 cmp = _mm_cmpgt_ps(a, b);
	__m128 result = MaskedAssign(cmp, DirectX::XMVectorNegate(q), q);
	return result;
}

FORCEINLINE __m128 QuaternionNormalizeSIMD(const __m128& q)
{
	__m128 radius, result, mask;
	radius = Dot4SIMD(q, q);
	mask = _mm_cmpeq_ps(radius, DirectX::XMVectorZero()); // all ones iff radius = 0
	result = _mm_rsqrt_ps(radius);
	result = _mm_mul_ps(result, q);
	return MaskedAssign(mask, q, result);	// if radius was 0, just return q
}

__forceinline __m128 QuaternionBlendNoAlignSIMD(const __m128& p, const __m128& q, float t)
{
	__m128 sclp, sclq, result;
	sclq = ReplicateX4(t);
	sclp = _mm_sub_ps(DirectX::XMVectorSplatOne(), sclq); // splat is such a silly name for this
	result = _mm_mul_ps(sclp, p);
	result = MaddSIMD(sclq, q, result);
	return QuaternionNormalizeSIMD(result);
}

__forceinline __m128 QuaternionBlendSIMD(const __m128& p, const __m128& q, float t)
{
	__m128 q2, result;
	q2 = QuaternionAlignSIMD(p, q);
	result = QuaternionBlendNoAlignSIMD(p, q2, t);
	return result;
}
#endif // USE_SIMD

void QuaternionBlend(const Quaternion& p, const Quaternion& q, float t, Quaternion& qt)
{
#if MATH_SIMD
	__m128 psimd = _mm_load_ps(p.Base());
	__m128 qsimd = _mm_load_ps(q.Base());
	__m128 qtsimd = QuaternionBlendSIMD(psimd, qsimd, t);
	_mm_store_ps(qt.Base(), qtsimd);
#else
	// decide if one of the quaternions is backwards
	Quaternion q2;
	QuaternionAlign(p, q, q2);
	QuaternionBlendNoAlign(p, q2, t, qt);
#endif // USE_SIMD
}