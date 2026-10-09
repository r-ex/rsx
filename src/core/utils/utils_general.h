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
#define FreeAllocVector(var) for (auto& v : var) { if (nullptr != v) delete v; }
#define FreeAllocPtrArray(var, count)	\
{										\
	for (int64_t i = 0; i < count; i++)	\
	{									\
		FreeAllocArray(var[i]);			\
	}									\
										\
	FreeAllocArray(var);				\
}

#define SWAP32(n) (((uint32_t)n & 0xff) << 24 | ((uint32_t)n & 0xff00) << 8 | ((uint32_t)n & 0xff0000) >> 8 | ((uint32_t)n >> 24))
#define ISWAP32(n) n = SWAP32(n)

inline const char* GetStringAfterLastSlash(const char* src)
{
	const char* lastSlash = strrchr(src, '/');
	const char* lastBackslash = strrchr(src, '\\');
	const char* lastSeparator = lastSlash > lastBackslash ? lastSlash : lastBackslash;

	if (!lastSeparator)
		return src;

	return lastSeparator + 1;
}

inline char* const removeExtension(char* const src)
{
	char* const extension = strrchr(src, '.');
	if (extension)
	{
		extension[0] = '\0';
	}

	return src;
}

inline void AppendSlash(std::string& svInput, const char separator = '\\')
{
	char lchar = svInput[svInput.size() - 1];

	if (lchar != '\\' && lchar != '/')
	{
		svInput.push_back(separator);
	}
}

// please do not call this with invalid pointers thank you
FORCEINLINE const bool IsStringZeroLength(const char* const str)
{
	assert(str);

	return str[0] == '\0';
}

inline constexpr size_t strlen_ct(const char* str)
{
	size_t length = 0ull;
	while (str[length] != '\0')
	{
		++length;
	}

	return length;
}

// use memcpy_s to copy strings (faster than normal strncpy_s)
// returns size of data wrote sans null terminator
FORCEINLINE const size_t strncpy_mem(char* dest, size_t destsz, const char* src, size_t count)
{
	const size_t length = strnlen_s(src, count);
	const errno_t result = memcpy_s(dest, destsz, src, length + 1ull);

	(void)(result); // guh
	assert(result == 0);

	// ensure null terminator, if memcpy_s did not fail, this should be valid memory
	dest[length] = '\0';

	return length;
}


inline std::string EscapeString(const std::string& str)
{
    std::stringstream outStream;

    for (int i = 0; i < str.length(); ++i)
    {
        unsigned char c = str.at(i);

        // if this char is over ascii then it's a multibyte sequence
        if (c > 0x7F)
        {
            // find the number of bytes to add to the stringstream
            int numBytes = 0;

            if (c <= 0xBF)
                numBytes = 1;
            else if (c >= 0xc2 && c <= 0xdf) // 0xC2 -> 0xDF - 2 byte sequence
                numBytes = 2;
            else if (c >= 0xe0 && c <= 0xef) // 0xE0 -> 0xEF - 3 byte sequence
                numBytes = 3;
            else if (c >= 0xf0 && c <= 0xf4) // 0xF0 -> 0xF4 - 4 byte sequence
                numBytes = 4;

            assert(numBytes != 0);

            for (int j = 0; j < numBytes; ++j)
            {
                outStream << str.at(i + j);
            }

            // add numBytes-1 to the char index
            // since one increment will already be handled by the for loop
            i += numBytes - 1;
            continue;
        }

        switch (c)
        {
        case '\0':
            break;
        case '\t':
            outStream << "\\t";
            break;
        case '\n':
            outStream << "\\n";
            break;
        case '\r':
            outStream << "\\r";
            break;
        case '\"':
            outStream << "\\\"";
            break;
        case '\\':
            outStream << "\\\\";
            break;
        default:
        {
            if (!std::isprint(c))
                outStream << std::hex << std::setfill('0') << std::setw(2) << "\\x" << (int)c;
            else
                outStream << c;

            break;
        }
        }
    }

    return outStream.str();
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

static const char* stristr(const char* haystack, const char* haystack_end, const char* needle, const char* needle_end)
{
    if (!needle_end)
        needle_end = needle + strlen(needle);

    const char un0 = (char)toupper(*needle);
    while ((!haystack_end && *haystack) || (haystack_end && haystack < haystack_end))
    {
        if (toupper(*haystack) == un0)
        {
            const char* b = needle + 1;
            for (const char* a = haystack + 1; b < needle_end; a++, b++)
                if (toupper(*a) != toupper(*b))
                    break;
            if (b == needle_end)
                return haystack;
        }
        haystack++;
    }
    return NULL;
}

// Adapted from ImGuiCustomTextFilter but minus the imgui part
class TextFilter
{
public:
    TextFilter()
    {
        grepCnt = 0;
        //Build();
    }

    bool PassFilter(const char* text, const char* text_end = nullptr) const
    {
        if (filters.size() == 0)
        {
            return true;
        }

        if (text == nullptr)
        {
            text = "";
            text_end = "";
        }

        for (const TxtRange& f : filters)
        {
            if (f.b == f.e)
            {
                continue;
            }

            if (f.b[0] == '-')
            {
                // exclude lookup
                if (stristr(text, text_end, f.b + 1, f.e) != NULL)
                {
                    return false;
                }
            }
            else
            {
                // normal grep lookup
                if (stristr(text, text_end, f.b, f.e) != NULL)
                {
                    return true;
                }
            }
        }

        // implicit * grep
        if (grepCnt == 0)
        {
            return true;
        }

        return false;
    }

    void Clear()
    {
        filters.clear();
        grepCnt = 0;
    };

    bool IsActive() const
    {
        return !filters.empty();
    };

    inline char charToUpper(const char c)
    {
        return (c >= 'a' && c <= 'z') ? (c & ~0x20) : c;
    }

    inline bool charIsBlank(const char c)
    {
        return c == ' ' || c == '\t';
    }

    struct TxtRange
    {
        const char* b;
        const char* e;

        TxtRange()
        {
            b = nullptr;
            e = nullptr;
        }

        TxtRange(const char* b, const char* e) : b(b), e(e)
        {
        };

        bool empty() const
        {
            return b == e;
        }

        void split(char separator, std::vector<TxtRange>* out) const
        {
            assert(out->size() == 0);

            const char* wb = b;
            const char* we = wb;
            while (we < e)
            {
                if (*we == separator)
                {
                    out->push_back(TxtRange(wb, we));
                    wb = (we + 1);
                }
                we++;
            }

            if (wb != we)
            {
                out->push_back(TxtRange(wb, we));
            }
        }
    };

    void Build(const char* input)
    {
        filters.clear();
        inputBuf = input;

        const size_t stringLen = strlen(input);
        TxtRange txtInputRange(input, input + stringLen);
        txtInputRange.split(',', &filters);

        grepCnt = 0;
        for (TxtRange& f : filters)
        {
            while (f.b < f.e && charIsBlank(f.b[0]))
            {
                f.b++;
            }

            while (f.e > f.b && charIsBlank(f.e[-1]))
            {
                f.e--;
            }

            if (f.empty())
            {
                continue;
            }

            if (f.b[0] != '-')
            {
                grepCnt += 1;
            }
        }
    }

    std::string inputBuf;
    std::vector<TxtRange> filters;
    int grepCnt;
};


using namespace std::chrono;

class CScopeTimer
{
public:
    CScopeTimer(const char* const name)
    {
        m_name = name;
        m_startTime = system_clock::now().time_since_epoch();
    }

    ~CScopeTimer()
    {
        system_clock::duration now = system_clock::now().time_since_epoch();
        printf("%s: finished in %.3fms.\n", m_name, duration_cast<microseconds>(now - m_startTime).count() / 1000.f);
    }

private:
    const char* m_name;
    system_clock::duration m_startTime;
};

#define XTIME_SCOPE2(x, y) CScopeTimer __timer_##y(x)
#define XTIME_SCOPE(x, y) XTIME_SCOPE2(x, y)
#define TIME_SCOPE(x) XTIME_SCOPE(x, __COUNTER__)
