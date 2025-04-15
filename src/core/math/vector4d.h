#pragma once

class Vector4D
{
public:
	float x, y, z, w;

	Vector4D() = default;
	//Vector4D(float inFl) : x(inFl), y(inFl), z(inFl) {};
	Vector4D(float inX, float inY, float inZ, float inW) : x(inX), y(inY), z(inZ), w(inW) {};
	Vector4D(const Vector4D& vecIn) : x(vecIn.x), y(vecIn.y), z(vecIn.z), w(vecIn.w) {};
	~Vector4D() {};

	inline void Init(float inFl = 0.0f) { x = inFl; y = inFl; z = inFl; w = inFl; } // init float members with a value of f
	inline void Init(float inX, float inY, float inZ, float inW) { x = inX; y = inY; z = inZ; w = inW; }
	inline bool IsValid() { return isfinite(x) && isfinite(y) && isfinite(z) && isfinite(w); } // check if floats are valid
	inline void Invalidate() { x = y = z = w = NAN; } // invalidate values
	inline void Negate(); // may not be correct

	inline const Vector& AsVector() const { return *reinterpret_cast<const Vector*>(this); };

	float operator[](int i) const;
	float& operator[](int i);

	Vector4D& operator=(const Vector4D& vecIn);

	bool operator==(const Vector4D& vecIn) const { return x == vecIn.x && y == vecIn.y && z == vecIn.z && w == vecIn.w; }
	bool operator!=(const Vector4D& vecIn) const { return x != vecIn.x || y != vecIn.y || z != vecIn.z || w != vecIn.w; } // check this one

	inline Vector4D& operator+=(const Vector4D& vecIn);
	inline Vector4D& operator+=(float inFl);
	inline Vector4D& operator-=(const Vector4D& vecIn);
	inline Vector4D& operator-=(float inFl);
	inline Vector4D& operator*=(const Vector4D& vecIn);
	inline Vector4D& operator*=(float scale);
	inline Vector4D& operator/=(const Vector4D& vecIn);
	inline Vector4D& operator/=(float scale);

	Vector4D operator+(const Vector4D& vecIn) const;
	Vector4D operator-(const Vector4D& vecIn) const;
	Vector4D operator*(const Vector4D& vecIn) const;
	Vector4D operator/(const Vector4D& vecIn) const;
	Vector4D operator*(float inFl) const;
	Vector4D operator/(float inFl) const;
};

#if !defined(VECTOR_NO_HLSL_TYPES)
typedef Vector4D float4;
#endif