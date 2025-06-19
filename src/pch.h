#pragma once
#include <tchar.h>
#include <assert.h>
#include <cstdint>

#include <string>
#include <map>
#include <unordered_map>
#include <unordered_set>
#include <vector>
#include <memory>
#include <sstream>
#include <fstream>
#include <filesystem>
#include <format>
#include <algorithm>
#include <queue>
#include <stack>
#include <functional>
#include <ranges>
#include <mutex>

#include <core/utils/utils_general.h>
#include <core/utils/fileio.h>
#include <core/utils/thread.h>
#include <core/utils/ramen.h>
#include <core/utils/exportsettings.h>

#define MATH_ASSERTS 1
#define MATH_SIMD 1

#include <core/math/mathlib.h>
#include <core/math/vector.h>
#include <core/math/vector2d.h>
#include <core/math/vector4d.h>
#include <core/math/matrix.h>
#include <core/math/color32.h>
#include <core/math/float16.h>
#include <core/math/compressedvector.h>

#include <core/logging/logger.h>

#include <core/cache/cachedb.h>

#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <Windows.h>

#pragma warning (disable: 4201)

#define DISABLE_WARNING(num) \
    __pragma(warning(push)) \
    __pragma(warning(disable: num))

#define ENABLE_WARNING() \
    __pragma(warning(pop))

#define UNUSED(x) (void)(x)
#define ARRSIZE(a) (sizeof(a) / sizeof(*(a)))
#define DX_RELEASE_PTR(ptr) \
    if (ptr)                \
    {                       \
        ptr->Release();     \
        ptr = nullptr;      \
    }

#define unreachable() (__assume(false))
#define EXPORT_DIRECTORY_NAME "exported_files"

// DEFINE FOR DISABLING FEATURES OR DEBUGGING.
// DEBUG

#if defined(_DEBUG)
    #define ASSERTS
    #define PAKLOAD_DEBUG_LOG 1
#else
    #define SPLASHSCREEN
    #define EXCEPTION_HANDLER
    #define PAKLOAD_DEBUG_LOG 0
#endif // #ifdef _DEBUG

#if defined(ASSERTS)
    #define assertm(exp, msg) assert(((void)msg, exp))
#else
    #define assertm(exp, msg) UNUSED(exp);
#endif // #ifdef ASSERTS

// uses assertm
#include <core/utils/buffermanager.h>

#define PAKLOAD_DEBUG_VERBOSE 2
#define PAKLOAD_DEBUG PAKLOAD_DEBUG_LOG

// FEATURES
#define PAKLOAD_LOADING_V6
#define PAKLOAD_PATCHING_V7
#define PAKLOAD_PATCHING_V8

#if defined(PAKLOAD_PATCHING_V8) || defined(PAKLOAD_PATCHING_V7)
#define PAKLOAD_PATCHING_ANY
#endif

#define MILES_RADAUDIO
//#define XB_XECRPYT
//#define XB_XCOMPRESS
#define SWITCH_SWIZZLE
//#define ADVANCED_MODEL_PREVIEW