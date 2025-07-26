#pragma once

#include <pch.h>

#define VALVE_RAND_MAX 0x7fff

//=========================================================
// 64 bit Quaternion
//=========================================================

class Quaternion64
{
public:
	// Construction/destruction:
	Quaternion64() = default;
	Quaternion64(const Quaternion& quat) { *this = quat; }

	// assignment
	Quaternion64& operator=(const Quaternion& vOther);
	operator Quaternion ();

private:
	uint64_t x : 21;
	uint64_t y : 21;
	uint64_t z : 21;
	uint64_t wneg : 1;
};

inline Quaternion64::operator Quaternion ()
{
	Quaternion tmp;

	// shift to -1048576, + 1048575, then round down slightly to -1.0 < x < 1.0
	tmp.x = (static_cast<int>(x) - 1048576) * (1 / 1048576.5f);
	tmp.y = (static_cast<int>(y) - 1048576) * (1 / 1048576.5f);
	tmp.z = (static_cast<int>(z) - 1048576) * (1 / 1048576.5f);

	// floating percision moment
	const float dprem = 1.0f - ((tmp.x * tmp.x) + (tmp.y * tmp.y) + (tmp.z * tmp.z));
	//const float dprem = 1 - tmp.x * tmp.x - tmp.y * tmp.y - tmp.z * tmp.z;

	// don't do fast square root if dprem < 0 so nans get handled properly
	if (dprem < 0.0f)
		tmp.w = sqrtf(dprem); // this is just here to return a nan lol
	else
		tmp.w = FastSqrtFast(dprem); // fsqrt
	
	if (wneg)
		tmp.w = -tmp.w;

#if MATH_ASSERTS
	assert(tmp.IsValid());
#endif

	return tmp;
}

inline Quaternion64& Quaternion64::operator=(const Quaternion& vOther)
{
	x = std::clamp(static_cast<int>(vOther.x * 1048576) + 1048576, 0, 2097151);
	y = std::clamp(static_cast<int>(vOther.y * 1048576) + 1048576, 0, 2097151);
	z = std::clamp(static_cast<int>(vOther.z * 1048576) + 1048576, 0, 2097151);
	wneg = (vOther.w < 0);
	return *this;
}



//=========================================================
// 48 bit Quaternion
//=========================================================

class Quaternion48
{
public:
	// Construction/destruction:
	Quaternion48() = default;
	Quaternion48(const Quaternion& quat) { *this = quat; }

	// assignment
	Quaternion48& operator=(const Quaternion& vOther);
	operator Quaternion ();

private:
	uint16_t x : 16;
	uint16_t y : 16;
	uint16_t z : 15;
	uint16_t wneg : 1;
};


inline Quaternion48::operator Quaternion ()
{
	Quaternion tmp;

	tmp.x = (static_cast<int>(x) - 32768) * (1 / 32768.5f);
	tmp.y = (static_cast<int>(y) - 32768) * (1 / 32768.5f);
	tmp.z = (static_cast<int>(z) - 16384) * (1 / 16384.5f);
	tmp.w = FastSqrtFast(1 - tmp.x * tmp.x - tmp.y * tmp.y - tmp.z * tmp.z);
	if (wneg)
		tmp.w = -tmp.w;

#if MATH_ASSERTS
	assert(tmp.IsValid());
#endif

	return tmp;
}

inline Quaternion48& Quaternion48::operator=(const Quaternion& vOther)
{
	x = std::clamp(static_cast<int>(vOther.x * 32768) + 32768, 0, 65535);
	y = std::clamp(static_cast<int>(vOther.y * 32768) + 32768, 0, 65535);
	z = std::clamp(static_cast<int>(vOther.z * 16384) + 16384, 0, 32767);
	wneg = (vOther.w < 0);
	return *this;
}



//=========================================================
// 32 bit Quaternion
//=========================================================

class Quaternion32
{
public:
	// Construction/destruction:
	Quaternion32() = default;
	Quaternion32(const Quaternion& quat) { *this = quat; }

	// assignment
	Quaternion32& operator=(const Quaternion& vOther);
	operator Quaternion ();
private:
	uint32_t x : 11;
	uint32_t y : 10;
	uint32_t z : 10;
	uint32_t wneg : 1;
};


inline Quaternion32::operator Quaternion ()
{
	Quaternion tmp;

	tmp.x = (static_cast<int>(x) - 1024) * (1 / 1024.0f);
	tmp.y = (static_cast<int>(y) - 512) * (1 / 512.0f);
	tmp.z = (static_cast<int>(z) - 512) * (1 / 512.0f);
	tmp.w = FastSqrtFast(1 - tmp.x * tmp.x - tmp.y * tmp.y - tmp.z * tmp.z);
	if (wneg)
		tmp.w = -tmp.w;

#if MATH_ASSERTS
	assert(tmp.IsValid());
#endif

	return tmp;
}

inline Quaternion32& Quaternion32::operator=(const Quaternion& vOther)
{
	x = std::clamp(static_cast<int>(vOther.x * 1024) + 1024, 0, 2047);
	y = std::clamp(static_cast<int>(vOther.y * 512) + 512, 0, 1023);
	z = std::clamp(static_cast<int>(vOther.z * 512) + 512, 0, 1023);
	wneg = (vOther.w < 0);
	return *this;
}



//=========================================================
// Fit a 3D vector in 48 bits
//=========================================================

class Vector48
{
public:
	// Construction/destruction:
	Vector48() = default;
	Vector48(float X, float Y, float Z) { x.SetFloat(X); y.SetFloat(Y); z.SetFloat(Z); }

	// assignment
	Vector48& operator=(const Vector& vOther);
	operator Vector ();

	const float operator[](int i) const { return (((float16*)this)[i]).GetFloat(); }

	// [rika]: todo surely a better solution for this
	inline Vector AsVector() const;

	float16 x;
	float16 y;
	float16 z;
};

inline Vector48& Vector48::operator=(const Vector& vOther)
{
	x.SetFloat(vOther.x);
	y.SetFloat(vOther.y);
	z.SetFloat(vOther.z);
	return *this;
}

inline Vector Vector48::AsVector() const
{
	Vector tmp;

	tmp.x = x.GetFloat();
	tmp.y = y.GetFloat();
	tmp.z = z.GetFloat();

#if MATH_ASSERTS
	assert(tmp.IsValid());
#endif

	return tmp;
}

inline Vector48::operator Vector ()
{
	Vector tmp;

	tmp.x = x.GetFloat();
	tmp.y = y.GetFloat();
	tmp.z = z.GetFloat();

#if MATH_ASSERTS
	assert(tmp.IsValid());
#endif

	return tmp;
}



//=========================================================
// Fit a 3D vector in 64 bits
//=========================================================

class Vector64
{
public:
	Vector64() = default;
	Vector64(const Vector& v) { *this = v; }

	Vector64& operator=(const Vector& v);
	operator Vector();

	Vector Unpack() const;

private:
	uint64_t x : 21;
	uint64_t y : 21;
	uint64_t z : 22;
};

inline Vector64& Vector64::operator=(const Vector& v)
{
	x = static_cast<uint64_t>((v.x + 1024.0) / 0.0009765625);
	y = static_cast<uint64_t>((v.y + 1024.0) / 0.0009765625);
	z = static_cast<uint64_t>((v.z + 2048.0) / 0.0009765625);

	return *this;
}

inline Vector64::operator Vector()
{
	Vector tmp;

	tmp.x = (x * 0.0009765625f) - 1024.f;
	tmp.y = (y * 0.0009765625f) - 1024.f;
	tmp.z = (z * 0.0009765625f) - 2048.f;

#if MATH_ASSERTS
	assert(tmp.IsValid());
#endif

	return tmp;
}

inline Vector Vector64::Unpack() const
{
	Vector tmp;

	tmp.x = (this->x * 0.0009765625f) - 1024.f;
	tmp.y = (this->y * 0.0009765625f) - 1024.f;
	tmp.z = (this->z * 0.0009765625f) - 2048.f;

	return tmp;
}



//=========================================================
// Fit a Normal + Tangent into 32 bits
//=========================================================

class Normal32
{
public:
	Normal32() : packedValue(0) {};
	Normal32(const uint32_t in) : packedValue(in) {};
	Normal32(const Vector& norm, const Vector4D& tang) : packedValue(0)
	{
		PackNormal(norm, tang);
	};

	inline const uint32_t PackedValue() const { return packedValue; };
	inline void UnpackNormal(Vector& out) const; // we need tangent too!
	inline void PackNormal(const Vector& normal, const Vector4D& tangent);

	inline Normal32& operator=(const Normal32& nml);

private:
	uint32_t packedValue;
};

// tangent unpack
void Normal32::UnpackNormal(Vector& out) const
{
	// check sign bit on first component
	bool sign = (packedValue >> 28) & 1;

	float val1 = sign ? -255.f : 255.f; // normal value 1
	float val2 = ((packedValue >> 19) & 0x1FF) - 256.f;
	float val3 = ((packedValue >> 10) & 0x1FF) - 256.f;

	int idx1 = (packedValue >> 29) & 3;
	int idx2 = (0x124u >> (2 * idx1 + 2)) & 3;
	int idx3 = (0x124u >> (2 * idx1 + 4)) & 3;

	// normalise the normal
	float normalised = FastRSqrtFast((255.f * 255.f) + (val2 * val2) + (val3 * val3));

	out[idx1] = val1 * normalised;
	out[idx2] = val2 * normalised;
	out[idx3] = val3 * normalised;
}

// https://github.com/r-ex/rmdlconv/blob/a0bf7044e9942a0d579c89c092996a1f8a3a7d6a/rmdlconv/mdl/studio.cpp#L93
void Normal32::PackNormal(const Vector& normal, const Vector4D& tangent)
{
	Vector absNml = normal;
	absNml.ABS();

	uint8_t idx1 = 0;

	// dropped component. seems to be the most significant axis?
	if (absNml.x >= absNml.y && absNml.x >= absNml.z)
		idx1 = 0;
	else if (absNml.y >= absNml.x && absNml.y >= absNml.z)
		idx1 = 1;
	else
		idx1 = 2;

	// index of the second and third components of the normal
	// see 140455D12
	int idx2 = (0x124u >> (2 * idx1 + 2)) & 3; // (2 * idx1 + 2) % 3
	int idx3 = (0x124u >> (2 * idx1 + 4)) & 3; // (2 * idx1 + 4) % 3

	// changed from (sign ? -255 : 255) because when -255 is used, the result in game appears very wrong
	float s = 255 / absNml[idx1];
	float val2 = (normal[idx2] * s) + 256.f;
	float val3 = (normal[idx3] * s) + 256.f;


	/*  --- cleaned up tangent unpacking from hlsl ---
		r2.y = -1 / (1 + nml.z);

		if (nml.z < -0.999899983)
		{
			r2.yzw = float3(0, -1, 0);
			r3.xyz = float3(-1, 0, 0);
		}
		else
		{
			r3.x = r2.y * (nml.x * nml.y);

			r3.z = r2.y * (nml.x * nml.x) + 1;
			r4.x = r2.y * (nml.y * nml.y) + 1;

			r2.yzw = float3(r3.z, r3.x, -nml.x);
			r3.xyz = float3(r3.x, r4.x, -nml.y);
		}

		r1.w = 0.00614192151 * (uint)r2.x; // 0.00614192151 * (_Value & 0x3FF)
		sincos(r1.w, r2.x, r4.x);

		// tangent
		r2.xyz = (r2.yzw * r4.xxx) + (r3.xyz * r2.xxx);
		// binorm sign
		r0.w = r0.w ? -1 : 1;
	*/

	// seems to calculate a modified orthonormal basis?
	// might be based on this: https://github.com/NVIDIA/Q2RTX/blob/9d987e755063f76ea86e426043313c2ba564c3b7/src/refresh/vkpt/shader/utils.glsl#L240-L255
	// we need to calculate this here so we can get the components of the tangent
	Vector a, b;
	if (normal.z < -0.999899983)
	{
		a = Vector{ 0, -1, 0 };
		b = Vector{ -1, 0, 0 };
	}
	else
	{
		float v1 = -1 / (1 + normal.z);
		float v2 = v1 * (normal.x * normal.y);
		float v3 = v1 * (normal.x * normal.x) + 1;
		float v4 = v1 * (normal.y * normal.y) + 1;

		a = Vector{ v3, v2, -normal.x };
		b = Vector{ v2, v4, -normal.y };
	}

	// get angle that gives the x and y components of our tangent
	float angle = atan2f(Vector::Dot(tangent.AsVector(), b), Vector::Dot(tangent.AsVector(), a));

	// add 360deg to the angle so it is always positive (can't store negative angles in 10 bits)
	if (angle < 0)
		angle += DEG2RAD(360.f);

	angle /= 0.00614192151f;

	bool sign = normal[idx1] < 0; // sign on the dropped normal component
	char binormSign = tangent.w < 1 ? 1 : 0; // sign on tangent w component

	packedValue = (binormSign << 31)
				| (idx1 << 29)
				| (sign << 28)
				| (static_cast<unsigned short>(val2) << 19)
				| (static_cast<unsigned short>(val3) << 10)
				| (static_cast<unsigned short>(angle) & 0x3FF);
}

Normal32& Normal32::operator=(const Normal32& nml)
{
	this->packedValue = nml.PackedValue();

	return *this;
}