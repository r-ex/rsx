#pragma once

typedef unsigned char uint8;

class Color32
{
public:

	Color32() = default;
	Color32(const Color32& inCol);
	Color32(uint8 inRGB, uint8 inA);
	Color32(uint8 inR, uint8 inG, uint8 inB, uint8 inA);
	//Color32(float inR, float inG, float inB, float inA) : r(inR), g(inG), b(inB), a(inA) {}; // 0-1 or 0-255??

	uint8 r, g, b, a;

	// source has these, probably more efficient
	unsigned int AsUint32() const;
	unsigned int* AsUint32Ptr();
	const unsigned int* AsUint32Ptr() const;

	Color32& operator=(const Color32& inCol);
	Color32& operator=(const Vector4D& inCol);

	bool operator==(const Color32& inCol) const { return r == inCol.r && g == inCol.g && b == inCol.b && a == inCol.a; }
	bool operator!=(const Color32& inCol) const { return r != inCol.r || g != inCol.g || b != inCol.b || a != inCol.a; }

	Vector4D ToVector4D() const;
};

