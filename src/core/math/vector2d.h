#pragma once

#define VALVE_RAND_MAX 0x7fff

class Vector2D
{
public:
	float x, y;

	Vector2D() = default;
	Vector2D(float inFl) : x(inFl), y(inFl) {};
	Vector2D(float inX, float inY) : x(inX), y(inY) {};
	Vector2D(const Vector2D& vecIn) : x(vecIn.x), y(vecIn.y) {};
	~Vector2D() {};

	inline void Init(float inFl = 0.0f) { x = inFl; y = inFl; } // init float members with a value of f
	inline void Init(float inX, float inY) { x = inX; y = inY; }
	inline bool IsValid() { return isfinite(x) && isfinite(y); } // check if floats are valid
	inline void Invalidate() { x = y = NAN; } // invalidate values
	inline void Negate(); // may not be correct

	float operator[](int i) const;
	float& operator[](int i);

	Vector2D& operator=(const Vector2D& vecIn);

	bool operator==(const Vector2D& vecIn) const { return x == vecIn.x && y == vecIn.y; }
	bool operator!=(const Vector2D& vecIn) const { return x != vecIn.x || y != vecIn.y; } // check this one

	inline Vector2D& operator+=(const Vector2D& vecIn);
	inline Vector2D& operator+=(float inFl);
	inline Vector2D& operator-=(const Vector2D& vecIn);
	inline Vector2D& operator-=(float inFl);
	inline Vector2D& operator*=(const Vector2D& vecIn);
	inline Vector2D& operator*=(float scale);
	inline Vector2D& operator/=(const Vector2D& vecIn);
	inline Vector2D& operator/=(float scale);

	Vector2D operator+(const Vector2D& vecIn) const;
	Vector2D operator-(const Vector2D& vecIn) const;
	Vector2D operator*(const Vector2D& vecIn) const;
	Vector2D operator/(const Vector2D& vecIn) const;
	Vector2D operator*(float inFl) const;
	Vector2D operator/(float inFl) const;

	inline void Random(float minFl, float maxFl);
};

#if !defined(VECTOR_NO_HLSL_TYPES)
typedef Vector2D float2;
#endif;