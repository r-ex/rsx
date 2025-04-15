#include <pch.h>
#include <core/math/color32.h>

Color32::Color32(uint8 inRGB, uint8 inA = 255) : r(inRGB), g(inRGB), b(inRGB), a(inA)
{

};

Color32::Color32(uint8 inR, uint8 inG, uint8 inB, uint8 inA) : r(inR), g(inG), b(inB), a(inA)
{

};

Color32::Color32(const Color32& inCol)
{
	*AsUint32Ptr() = inCol.AsUint32();
}

inline unsigned int Color32::AsUint32() const
{
	return *reinterpret_cast<const unsigned int*>(this);
}

inline unsigned int* Color32::AsUint32Ptr()
{
	return reinterpret_cast<unsigned int*>(this);
}

inline const unsigned int* Color32::AsUint32Ptr() const
{
	return reinterpret_cast<const unsigned int*>(this);
}

Color32& Color32::operator=(const Color32& inCol)
{
	r = inCol.r;
	g = inCol.g;
	b = inCol.b;
	a = inCol.a;

	return *this;
}

Color32& Color32::operator=(const Vector4D& inCol)
{
	r = static_cast<uint8_t>(inCol.x * 255.f);
	g = static_cast<uint8_t>(inCol.y * 255.f);
	b = static_cast<uint8_t>(inCol.z * 255.f);
	a = static_cast<uint8_t>(inCol.w * 255.f);

	return *this;
}

Vector4D Color32::ToVector4D() const
{
	Vector4D out(this->r / 255.f, this->g / 255.f, this->b / 255.f, this->a / 255.f);
	return out;
}