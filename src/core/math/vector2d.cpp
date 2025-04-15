#include <pch.h>
#include <core/math/vector2d.h>

inline void Vector2D::Negate()
{
	x = -x;
	y = -y;
}

// access
float Vector2D::operator[](int i) const
{
	assert(i >= 0 && 2 > i);
	return ((float*)this)[i];
}

float& Vector2D::operator[](int i)
{
	assert(i >= 0 && 2 > i);
	return ((float*)this)[i];
}

Vector2D& Vector2D::operator=(const Vector2D& vecIn)
{
	x = vecIn.x;
	y = vecIn.y;

	return *this;
}

// math
inline Vector2D& Vector2D::operator+=(const Vector2D& vecIn)
{
	x += vecIn.x;
	y += vecIn.y;

	return *this;
}

inline Vector2D& Vector2D::operator+=(float inFl)
{
	x += inFl;
	y += inFl;

	return *this;
}

inline Vector2D& Vector2D::operator-=(const Vector2D& vecIn)
{
	x -= vecIn.x;
	y -= vecIn.y;

	return *this;
}

inline Vector2D& Vector2D::operator-=(float inFl)
{
	x -= inFl;
	y -= inFl;

	return *this;
}

inline Vector2D& Vector2D::operator*=(const Vector2D& vecIn)
{
	x *= vecIn.x;
	y *= vecIn.y;

	return *this;
}

inline Vector2D& Vector2D::operator*=(float scale)
{
	x *= scale;
	y *= scale;

	return *this;
}

inline Vector2D& Vector2D::operator/=(const Vector2D& vecIn)
{
	x /= vecIn.x;
	y /= vecIn.y;

	return *this;
}

inline Vector2D& Vector2D::operator/=(float scale)
{
	x /= scale;
	y /= scale;

	return *this;
}

// math but with copying
Vector2D Vector2D::operator+(const Vector2D& vecIn) const
{
	Vector2D out;

	out.x = x + vecIn.x;
	out.y = y + vecIn.y;

	return out;
}

Vector2D Vector2D::operator-(const Vector2D& vecIn) const
{
	Vector2D out;

	out.x = x - vecIn.x;
	out.y = y - vecIn.y;

	return out;
}

Vector2D Vector2D::operator*(const Vector2D& vecIn) const
{
	Vector2D out;

	out.x = x * vecIn.x;
	out.y = y * vecIn.y;

	return out;
}

Vector2D Vector2D::operator/(const Vector2D& vecIn) const
{
	Vector2D out;

	out.x = x / vecIn.x;
	out.y = y / vecIn.y;

	return out;
}

Vector2D Vector2D::operator*(float inFl) const
{
	Vector2D out;

	out.x = x * inFl;
	out.y = y * inFl;

	return out;
}

Vector2D Vector2D::operator/(float inFl) const
{
	Vector2D out;

	out.x = x / inFl;
	out.y = y / inFl;

	return out;
}

// functionality copied from source for consistency
inline void Vector2D::Random(float minFl, float maxFl)
{
	x = minFl + (static_cast<float>(rand()) / VALVE_RAND_MAX) * (maxFl - minFl);
	y = minFl + (static_cast<float>(rand()) / VALVE_RAND_MAX) * (maxFl - minFl);
}