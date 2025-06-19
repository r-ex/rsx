#pragma once

// misc for pch so rtech utils (teheheh) doesn't have to be included in 9000 files
#define IALIGN(a, b) (((a)+((b)-1)) & ~((b)-1))

#define IALIGN2(a)   IALIGN(a,2)
#define IALIGN4(a)   IALIGN(a,4)
#define IALIGN8(a)   IALIGN(a,8)
#define IALIGN16(a)  IALIGN(a,16)
#define IALIGN32(a)  IALIGN(a,32)
#define IALIGN64(a)  IALIGN(a,64)

#define FORCEINLINE __forceinline
#define UNLIKELY [[unlikely]]
#define LIKELY [[likely]]

// one liners to make it Look Fancy
#define FreeAllocArray(var) if (nullptr != var) { delete[] var; }
#define FreeAllocVar(var) if (nullptr != var) { delete var; }

#define SWAP32(n) (((uint32_t)n & 0xff) << 24 | ((uint32_t)n & 0xff00) << 8 | ((uint32_t)n & 0xff0000) >> 8 | ((uint32_t)n >> 24))
#define ISWAP32(n) n = SWAP32(n)

inline const char* keepAfterLastSlashOrBackslash(const char* src)
{
	const char* lastSlash = strrchr(src, '/');
	const char* lastBackslash = strrchr(src, '\\');
	const char* lastSeparator = lastSlash > lastBackslash ? lastSlash : lastBackslash;

	if (!lastSeparator)
		return src;

	return lastSeparator + 1;
}

// please do not call this with invalid pointers thank you
FORCEINLINE const bool IsStringZeroLength(const char* const str)
{
	assert(str);

	return str[0] == '\0';
}

struct Color4
{
	Color4() = default;
	Color4(const uint8_t r, const uint8_t g, const uint8_t b, const uint8_t a)
	{
		this->r = static_cast<float>(r) / 255.0f;
		this->g = static_cast<float>(g) / 255.0f;
		this->b = static_cast<float>(b) / 255.0f;
		this->a = static_cast<float>(a) / 255.0f;
	}

	Color4(const uint8_t r, const uint8_t g, const uint8_t b)
	{
		this->r = static_cast<float>(r) / 255.0f;
		this->g = static_cast<float>(g) / 255.0f;
		this->b = static_cast<float>(b) / 255.0f;
		this->a = 1.0f;
	}

	float r, g, b, a;
};

void WaitForDebugger();