#pragma once

//#define _CRTDBG_MAP_ALLOC
//#include <stdlib.h>
//#include <crtdbg.h>

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
#include <variant>

#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <Windows.h>
#include <io.h>

#include <core/features.h>

#include <core/utils/crc32.h>
#include <core/utils/utils_general.h>
#include <core/utils/fileio.h>
#include <core/utils/thread.h>
#include <core/utils/ramen.h>

#define STREAMIO

#define MATH_USE_DX
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

#include <rapidjson/rapidjson.h>
#include "rapidjson/document.h"
#include "rapidjson/writer.h"
#include "rapidjson/stringbuffer.h"
#include "rapidjson/prettywriter.h"
#include "rapidjson/error/en.h"

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
    //#define SPLASHSCREEN
    #define EXCEPTION_HANDLER
    #define PAKLOAD_DEBUG_LOG 0
#endif // #ifdef _DEBUG

#if defined(ASSERTS)
    #define assertm(exp, msg) assert(((void)msg, exp))
#else
    #define assertm(exp, msg) UNUSED(exp);
#endif // #ifdef ASSERTS

// [rika]: uses some of the above defines
#include <core/utils/buffermanager.h>
#include <core/utils/exportsettings.h>
#include <core/utils/textbuffer.h>
#include <core/utils/keyvalue_parser.h>

#define PAKLOAD_DEBUG_VERBOSE 2
#define PAKLOAD_DEBUG PAKLOAD_DEBUG_LOG

// FEATURES
#define PAKLOAD_LOADING_V6
#define PAKLOAD_PATCHING_V7
#define PAKLOAD_PATCHING_V8

#if defined(PAKLOAD_PATCHING_V8) || defined(PAKLOAD_PATCHING_V7)
#define PAKLOAD_PATCHING_ANY
#endif

//#define XB_XECRPYT
//#define XB_XCOMPRESS
#define SWITCH_SWIZZLE

#if defined(BUILD_NOGUI)
#define IS_NOGUI(...) (true)
#else
#define IS_NOGUI(cli) ((cli) && (cli)->HasParam("-nogui"))
#endif

#define DO_ASSET_LOAD() !g_assetData.m_validate || g_assetData.m_validateAssetLoading


#define CONCAT(a, b) XCONCAT(a, b)
#define XCONCAT(a, b) a ## b
#define UNIQUE_VAR() CONCAT(__unique_, __COUNTER__)

#include "core/features.h"
