#include <pch.h>
#include <core/math/vector4d.h>

inline void Vector4D::Negate()
{
	x = -x;
	y = -y;
	z = -z;
	w = -w;
}

// access
float Vector4D::operator[](int i) const
{
	assert(i >= 0 && 4 > i);
	return ((float*)this)[i];
}

float& Vector4D::operator[](int i)
{
	assert(i >= 0 && 4 > i);
	return ((float*)this)[i];
}

Vector4D& Vector4D::operator=(const Vector4D& vecIn)
{
	x = vecIn.x;
	y = vecIn.y;
	z = vecIn.z;
	w = vecIn.w;

	return *this;
}

// math
inline Vector4D& Vector4D::operator+=(const Vector4D& vecIn)
{
	x += vecIn.x;
	y += vecIn.y;
	z += vecIn.z;
	w += vecIn.w;

	return *this;
}

inline Vector4D& Vector4D::operator+=(float inFl)
{
	x += inFl;
	y += inFl;
	z += inFl;
	w += inFl;

	return *this;
}

inline Vector4D& Vector4D::operator-=(const Vector4D& vecIn)
{
	x -= vecIn.x;
	y -= vecIn.y;
	z -= vecIn.z;
	z -= vecIn.w;

	return *this;
}

inline Vector4D& Vector4D::operator-=(float inFl)
{
	x -= inFl;
	y -= inFl;
	z -= inFl;
	w -= inFl;

	return *this;
}

inline Vector4D& Vector4D::operator*=(const Vector4D& vecIn)
{
	x *= vecIn.x;
	y *= vecIn.y;
	z *= vecIn.z;
	w *= vecIn.w;

	return *this;
}

inline Vector4D& Vector4D::operator*=(float scale)
{
	x *= scale;
	y *= scale;
	z *= scale;
	w *= scale;

	return *this;
}

inline Vector4D& Vector4D::operator/=(const Vector4D& vecIn)
{
	x /= vecIn.x;
	y /= vecIn.y;
	z /= vecIn.z;
	w /= vecIn.w;

	return *this;
}

inline Vector4D& Vector4D::operator/=(float scale)
{
	x /= scale;
	y /= scale;
	z /= scale;
	w /= scale;

	return *this;
}

// math but with copying
Vector4D Vector4D::operator+(const Vector4D& vecIn) const
{
	Vector4D out;

	out.x = x + vecIn.x;
	out.y = y + vecIn.y;
	out.z = z + vecIn.z;
	out.w = w + vecIn.w;

	return out;
}

Vector4D Vector4D::operator-(const Vector4D& vecIn) const
{
	Vector4D out;

	out.x = x - vecIn.x;
	out.y = y - vecIn.y;
	out.z = z - vecIn.z;
	out.w = w - vecIn.w;

	return out;
}

Vector4D Vector4D::operator*(const Vector4D& vecIn) const
{
	Vector4D out;

	out.x = x * vecIn.x;
	out.y = y * vecIn.y;
	out.z = z * vecIn.z;
	out.w = w * vecIn.w;

	return out;
}

Vector4D Vector4D::operator/(const Vector4D& vecIn) const
{
	Vector4D out;

	out.x = x / vecIn.x;
	out.y = y / vecIn.y;
	out.z = z / vecIn.z;
	out.w = w / vecIn.w;

	return out;
}

Vector4D Vector4D::operator*(float inFl) const
{
	Vector4D out;

	out.x = x * inFl;
	out.y = y * inFl;
	out.z = z * inFl;
	out.w = w * inFl;

	return out;
}

Vector4D Vector4D::operator/(float inFl) const
{
	Vector4D out;

	out.x = x / inFl;
	out.y = y / inFl;
	out.z = z / inFl;
	out.w = w / inFl;

	return out;
}