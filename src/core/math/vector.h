#pragma once

#define VALVE_RAND_MAX 0x7fff

//========
// Vector
//========

class Vector
{
public:
	float x, y, z;

	Vector() = default;
	Vector(float inFl) : x(inFl), y(inFl), z(inFl) {};
	Vector(float inX, float inY, float inZ) : x(inX), y(inY), z(inZ) {};
	Vector(const Vector& vecIn) : x(vecIn.x), y(vecIn.y), z(vecIn.z) {};
	~Vector() {};

	void Init(float inFl = 0.0f) { x = inFl; y = inFl; z = inFl; } // init float members with a value of f
	void Init(float inX, float inY, float inZ) { x = inX; y = inY; z = inZ; }
	inline bool IsValid() const { return isfinite(x) && isfinite(y) && isfinite(z); } // check if floats are valid
	inline void Invalidate() { x = y = z = NAN; } // invalidate values
	inline void Negate(); // may not be correct
	inline void ABS() { x = fabs(x), y = fabs(y), z = fabs(z); };

	float operator[](int i) const;
	float& operator[](int i);

	Vector& operator=(const Vector& vecIn);

	bool operator==(const Vector& vecIn) const { return x == vecIn.x && y == vecIn.y && z == vecIn.z; }
	bool operator!=(const Vector& vecIn) const { return x != vecIn.x || y != vecIn.y || z != vecIn.z; } // check this one

	inline Vector& operator+=(const Vector& vecIn);
	inline Vector& operator+=(float inFl);
	inline Vector& operator-=(const Vector& vecIn);
	inline Vector& operator-=(float inFl);
	inline Vector& operator*=(const Vector& vecIn);
	inline Vector& operator*=(float scale);
	inline Vector& operator/=(const Vector& vecIn);
	inline Vector& operator/=(float scale);

	Vector operator+(const Vector& vecIn) const;
	Vector operator-(const Vector& vecIn) const;
	Vector operator*(const Vector& vecIn) const;
	Vector operator/(const Vector& vecIn) const;
	Vector operator*(float inFl) const;
	Vector operator/(float inFl) const;
	Vector operator*(int in) const;
	Vector operator/(int in) const;

	inline void Random(float minFl, float maxFl);

	inline float* AsFloat() { return reinterpret_cast<float*>(this); }

	float* Base() { return reinterpret_cast<float*>(this); }
	float const* Base() const { return reinterpret_cast<float const*>(this); }

	static const float Dot(const Vector& a, const Vector& b); // add simd option

	inline XMVECTOR AsXMVector() const { return XMVECTOR{ x,y,z }; };
};

inline void Vector::Negate()
{
	x = -x;
	y = -y;
	z = -z;
}

// math
inline Vector& Vector::operator+=(const Vector& vecIn)
{
	x += vecIn.x;
	y += vecIn.y;
	z += vecIn.z;

	return *this;
}

inline Vector& Vector::operator+=(float inFl)
{
	x += inFl;
	y += inFl;
	z += inFl;

	return *this;
}

inline Vector& Vector::operator-=(const Vector& vecIn)
{
	x -= vecIn.x;
	y -= vecIn.y;
	z -= vecIn.z;

	return *this;
}

inline Vector& Vector::operator-=(float inFl)
{
	x -= inFl;
	y -= inFl;
	z -= inFl;

	return *this;
}

inline Vector& Vector::operator*=(const Vector& vecIn)
{
	x *= vecIn.x;
	y *= vecIn.y;
	z *= vecIn.z;

	return *this;
}

inline Vector& Vector::operator*=(float scale)
{
	x *= scale;
	y *= scale;
	z *= scale;

	return *this;
}

inline Vector& Vector::operator/=(const Vector& vecIn)
{
	x /= vecIn.x;
	y /= vecIn.y;
	z /= vecIn.z;

	return *this;
}

inline Vector& Vector::operator/=(float scale)
{
	x /= scale;
	y /= scale;
	z /= scale;

	return *this;
}

// functionality copied from source for consistency
inline void Vector::Random(float minFl, float maxFl)
{
	x = minFl + (static_cast<float>(rand()) / VALVE_RAND_MAX) * (maxFl - minFl);
	y = minFl + (static_cast<float>(rand()) / VALVE_RAND_MAX) * (maxFl - minFl);
	z = minFl + (static_cast<float>(rand()) / VALVE_RAND_MAX) * (maxFl - minFl);
}

// sorse
inline float DotProduct(const Vector& a, const Vector& b)
{
	assert(!a.IsValid());
	assert(!b.IsValid());
	return(a.x * b.x + a.y * b.y + a.z * b.z);
}

inline float DotProduct(const float* a, const float* b)
{
	return(a[0] * b[0] + a[1] * b[1] + a[2] * b[2]);
}

#if !defined(VECTOR_NO_HLSL_TYPES)
typedef Vector float3;
#endif


//============
// Quaternion
//============

class RadianEuler;
class Quaternion
{
public:
	float x, y, z, w;

	Quaternion() = default;
	Quaternion(float inFl) : x(inFl), y(inFl), z(inFl), w(inFl) {};
	Quaternion(float xIn, float yIn, float zIn, float wIn) : x(xIn), y(yIn), z(zIn), w(wIn) {};
	Quaternion(RadianEuler const &inEul);
	~Quaternion() {};

	void Init(float inFl = 0.0f) { x = inFl; y = inFl; z = inFl; w = inFl; } // init float members with a value of f
	void Init(float inX, float inY, float inZ, float wIn) { x = inX; y = inY; z = inZ; w = wIn; }
	inline bool IsValid() const { return isfinite(x) && isfinite(y) && isfinite(z) && isfinite(w); } // check if floats are valid
	inline void Invalidate() { x = y = z = w = NAN; } // invalidate values
	inline const float* Base() const { return reinterpret_cast<const float*>(this); }
	inline float* Base() { return reinterpret_cast<float*>(this); }

	float operator[](int i) const;
	float& operator[](int i);

	Quaternion& operator=(const Quaternion& quatIn);

	bool operator==(const Quaternion& quatIn) const { return x == quatIn.x && y == quatIn.y && z == quatIn.z && w == quatIn.w; }
	bool operator!=(const Quaternion& quatIn) const { return x != quatIn.x || y != quatIn.y || z != quatIn.z || w != quatIn.w; } // check this one
};


//=============
// RadianEuler
//=============

class QAngle;
// Radian Euler angle aligned to axis (NOT ROLL/PITCH/YAW)
class RadianEuler
{
public:
	float x, y, z;

	RadianEuler() = default;
	RadianEuler(float inFl) : x(inFl), y(inFl), z(inFl) {};
	RadianEuler(float inX, float inY, float inZ) : x(inX), y(inY), z(inZ) {};
	RadianEuler(Quaternion const &quatIn);
	RadianEuler(QAngle const &qangIn);

	void Init(float inFl = 0.0f) { x = inFl; y = inFl; z = inFl; } // init float members with a value of f
	void Init(float inX, float inY, float inZ) { x = inX; y = inY; z = inZ; }
	inline bool IsValid() const { return isfinite(x) && isfinite(y) && isfinite(z); } // check if floats are valid
	inline void Invalidate() { x = y = z = NAN; } // invalidate values
	inline const float* Base() const { return reinterpret_cast<const float*>(this); }
	inline float* Base() { return reinterpret_cast<float*>(this); }

	float operator[](int i) const;
	float& operator[](int i);

	RadianEuler& operator=(const RadianEuler& angIn);

	bool operator==(const RadianEuler& angIn) const { return x == angIn.x && y == angIn.y && z == angIn.z; }
	bool operator!=(const RadianEuler& angIn) const { return x != angIn.x || y != angIn.y || z != angIn.z; } // check this one

	QAngle ToQAngle() const;
};


//========
// QAngle
//========

// Degree Euler pitch, yaw, roll
class QAngle
{
public:
	float x, y, z;

	QAngle() = default;
	QAngle(float inFl) : x(inFl), y(inFl), z(inFl) {};
	QAngle(float inX, float inY, float inZ) : x(inX), y(inY), z(inZ) {};
	QAngle(RadianEuler const &angIn);
	QAngle(Quaternion const& quatIn);

	void Init(float inFl = 0.0f) { x = inFl; y = inFl; z = inFl; } // init float members with a value of f
	void Init(float inX, float inY, float inZ) { x = inX; y = inY; z = inZ; }
	inline bool IsValid() const { return isfinite(x) && isfinite(y) && isfinite(z); } // check if floats are valid
	inline void Invalidate() { x = y = z = NAN; } // invalidate values
	inline const float* Base() const { return reinterpret_cast<const float*>(this); }
	inline float* Base() { return reinterpret_cast<float*>(this); }

	float operator[](int i) const;
	float& operator[](int i);

	QAngle& operator=(const QAngle& qangIn);

	bool operator==(const QAngle& qangIn) const { return x == qangIn.x && y == qangIn.y && z == qangIn.z; }
	bool operator!=(const QAngle& qangIn) const { return x != qangIn.x || y != qangIn.y || z != qangIn.z; } // check this one

	QAngle& operator+=(const QAngle& qangIn);
	QAngle& operator-=(const QAngle& qangIn);
	QAngle& operator*=(float scale);
	QAngle& operator/=(float scale);

	QAngle	operator-() const;
	QAngle	operator+(const QAngle& qangIn) const;
	QAngle	operator-(const QAngle& qangIn) const;
	QAngle	operator*(float inFl) const;
	QAngle	operator/(float inFl) const;

	RadianEuler ToEuler() const;
};

void VectorYawRotate(const Vector& in, float flYaw, Vector& out);

void NormalizeAngles(QAngle& angles);