#pragma once
#include <d3d11.h>
#include <game/rtech/cpakfile.h>
#include <game/rtech/utils/utils.h>
#include <core/render/dx.h>

class CRenderTexture;

enum eTextureFormat : uint16_t
{
    TEX_FMT_BC1_UNORM,
    TEX_FMT_BC1_UNORM_SRGB,
    TEX_FMT_BC2_UNORM,
    TEX_FMT_BC2_UNORM_SRGB,
    TEX_FMT_BC3_UNORM,
    TEX_FMT_BC3_UNORM_SRGB,
    TEX_FMT_BC4_UNORM,
    TEX_FMT_BC4_SNORM,
    TEX_FMT_BC5_UNORM,
    TEX_FMT_BC5_SNORM,
    TEX_FMT_BC6H_UF16,
    TEX_FMT_BC6H_SF16,
    TEX_FMT_BC7_UNORM,
    TEX_FMT_BC7_UNORM_SRGB,
    TEX_FMT_R32G32B32A32_FLOAT,
    TEX_FMT_R32G32B32A32_UINT,
    TEX_FMT_R32G32B32A32_SINT,
    TEX_FMT_R32G32B32_FLOAT,
    TEX_FMT_R32G32B32_UINT,
    TEX_FMT_R32G32B32_SINT,
    TEX_FMT_R16G16B16A16_FLOAT,
    TEX_FMT_R16G16B16A16_UNORM,
    TEX_FMT_R16G16B16A16_UINT,
    TEX_FMT_R16G16B16A16_SNORM,
    TEX_FMT_R16G16B16A16_SINT,
    TEX_FMT_R32G32_FLOAT,
    TEX_FMT_R32G32_UINT,
    TEX_FMT_R32G32_SINT,
    TEX_FMT_R10G10B10A2_UNORM,
    TEX_FMT_R10G10B10A2_UINT,
    TEX_FMT_R11G11B10_FLOAT,
    TEX_FMT_R8G8B8A8_UNORM,
    TEX_FMT_R8G8B8A8_UNORM_SRGB,
    TEX_FMT_R8G8B8A8_UINT,
    TEX_FMT_R8G8B8A8_SNORM,
    TEX_FMT_R8G8B8A8_SINT,
    TEX_FMT_R16G16_FLOAT,
    TEX_FMT_R16G16_UNORM,
    TEX_FMT_R16G16_UINT,
    TEX_FMT_R16G16_SNORM,
    TEX_FMT_R16G16_SINT,
    TEX_FMT_R32_FLOAT,
    TEX_FMT_R32_UINT,
    TEX_FMT_R32_SINT,
    TEX_FMT_R8G8_UNORM,
    TEX_FMT_R8G8_UINT,
    TEX_FMT_R8G8_SNORM,
    TEX_FMT_R8G8_SINT,
    TEX_FMT_R16_FLOAT,
    TEX_FMT_R16_UNORM,
    TEX_FMT_R16_UINT,
    TEX_FMT_R16_SNORM,
    TEX_FMT_R16_SINT,
    TEX_FMT_R8_UNORM,
    TEX_FMT_R8_UINT,
    TEX_FMT_R8_SNORM,
    TEX_FMT_R8_SINT,
    TEX_FMT_A8_UNORM,
    TEX_FMT_R9G9B9E5_SHAREDEXP,
    TEX_FMT_R10G10B10_XR_BIAS_A2_UNORM,
    TEX_FMT_D32_FLOAT,
    TEX_FMT_D16_UNORM,

    // todo: confirm this order
    TEX_FMT_ASTC_4x4_UNORM,
    TEX_FMT_ASTC_5x4_UNORM,
    TEX_FMT_ASTC_5x5_UNORM,
    TEX_FMT_ASTC_6x5_UNORM,
    TEX_FMT_ASTC_6x6_UNORM,
    TEX_FMT_ASTC_8x5_UNORM,
    TEX_FMT_ASTC_8x6_UNORM,
    TEX_FMT_ASTC_10x5_UNORM,
    TEX_FMT_ASTC_10x6_UNORM,
    TEX_FMT_ASTC_8x8_UNORM,
    TEX_FMT_ASTC_10x8_UNORM,
    TEX_FMT_ASTC_10x10_UNORM,
    TEX_FMT_ASTC_12x10_UNORM,
    TEX_FMT_ASTC_12x12_UNORM,
    TEX_FMT_ASTC_4x4_UNORM_SRGB,
    TEX_FMT_ASTC_5x4_UNORM_SRGB,
    TEX_FMT_ASTC_5x5_UNORM_SRGB,
    TEX_FMT_ASTC_6x5_UNORM_SRGB,
    TEX_FMT_ASTC_6x6_UNORM_SRGB,
    TEX_FMT_ASTC_8x5_UNORM_SRGB,
    TEX_FMT_ASTC_8x6_UNORM_SRGB,
    TEX_FMT_ASTC_10x5_UNORM_SRGB,
    TEX_FMT_ASTC_10x6_UNORM_SRGB,
    TEX_FMT_ASTC_8x8_UNORM_SRGB,
    TEX_FMT_ASTC_10x8_UNORM_SRGB,
    TEX_FMT_ASTC_10x10_UNORM_SRGB,
    TEX_FMT_ASTC_12x10_UNORM_SRGB,
    TEX_FMT_ASTC_12x12_UNORM_SRGB,

    TEX_FMT_D24_UNORM_S8_UINT,

    TEX_FMT_UNKNOWN, // has weird values in previous versions ? this might not actually be unknown!
    TEX_FMT_COUNT,
};

inline const bool IsATSC(const eTextureFormat fmt)
{
    return fmt >= eTextureFormat::TEX_FMT_ASTC_4x4_UNORM && eTextureFormat::TEX_FMT_ASTC_12x12_UNORM_SRGB >= fmt;
}

/*#define U8(x) static_cast<uint8_t>(x)
static const std::pair<uint8_t, uint8_t> s_pBytesPerPixel[eTextureFormat::TEX_FMT_COUNT] =
{
    { U8(8u), U8(4u) },
    { U8(8u), U8(4u) },
    { U8(16u), U8(4u) },
    { U8(16u), U8(4u) },
    { U8(16u), U8(4u) },
    { U8(16u), U8(4u) },
    { U8(8u), U8(4u) },
    { U8(8u), U8(4u) },
    { U8(16u), U8(4u) },
    { U8(16u), U8(4u) },
    { U8(16u), U8(4u) },
    { U8(16u), U8(4u) },
    { U8(16u), U8(4u) },
    { U8(16u), U8(4u) },
    { U8(16u), U8(1u) },
    { U8(16u), U8(1u) },
    { U8(16u), U8(1u) },
    { U8(12u), U8(1u) },
    { U8(12u), U8(1u) },
    { U8(12u), U8(1u) },
    { U8(8u), U8(1u) },
    { U8(8u), U8(1u) },
    { U8(8u), U8(1u) },
    { U8(8u), U8(1u) },
    { U8(8u), U8(1u) },
    { U8(8u), U8(1u) },
    { U8(8u), U8(1u) },
    { U8(8u), U8(1u) },
    { U8(4u), U8(1u) },
    { U8(4u), U8(1u) },
    { U8(4u), U8(1u) },
    { U8(4u), U8(1u) },
    { U8(4u), U8(1u) },
    { U8(4u), U8(1u) },
    { U8(4u), U8(1u) },
    { U8(4u), U8(1u) },
    { U8(4u), U8(1u) },
    { U8(4u), U8(1u) },
    { U8(4u), U8(1u) },
    { U8(4u), U8(1u) },
    { U8(4u), U8(1u) },
    { U8(4u), U8(1u) },
    { U8(4u), U8(1u) },
    { U8(4u), U8(1u) },
    { U8(2u), U8(1u) },
    { U8(2u), U8(1u) },
    { U8(2u), U8(1u) },
    { U8(2u), U8(1u) },
    { U8(2u), U8(1u) },
    { U8(2u), U8(1u) },
    { U8(2u), U8(1u) },
    { U8(2u), U8(1u) },
    { U8(2u), U8(1u) },
    { U8(1u), U8(1u) },
    { U8(1u), U8(1u) },
    { U8(1u), U8(1u) },
    { U8(1u), U8(1u) },
    { U8(1u), U8(1u) },
    { U8(4u), U8(1u) },
    { U8(4u), U8(1u) },
    { U8(4u), U8(1u) },
    { U8(2u), U8(1u) },
    { U8(0u), U8(0u) },
    { U8(0u), U8(0u) },
    { U8(5u), U8(0u) },
    { U8(0u), U8(0u) },
    { U8(5u), U8(0u) },
    { U8(0u), U8(0u) },
    { U8(1u), U8(0u) },
    { U8(0u), U8(0u) },
    { U8(2u), U8(0u) },
    { U8(0u), U8(0u) },
    { U8(0u), U8(0u) },
    { U8(0u), U8(0u) },
    { U8(1u), U8(0u) },
    { U8(0u), U8(0u) },
};*/

static const uint8_t s_pBytesPerPixel[304 * 3] =
{
    8, 4, 4,
    8, 4, 4,
    16, 4, 4,
    16, 4, 4,
    16, 4, 4,
    16, 4, 4,
    8, 4, 4,
    8, 4, 4,
    16, 4, 4,
    16, 4, 4,
    16, 4, 4,
    16, 4, 4,
    16, 4, 4,
    16, 4, 4,
    16, 1, 1,
    16, 1, 1,
    16, 1, 1,
    12, 1, 1,
    12, 1, 1,
    12, 1, 1,
    8, 1, 1,
    8, 1, 1,
    8, 1, 1,
    8, 1, 1,
    8, 1, 1,
    8, 1, 1,
    8, 1, 1,
    8, 1, 1,
    4, 1, 1,
    4, 1, 1,
    4, 1, 1,
    4, 1, 1,
    4, 1, 1,
    4, 1, 1,
    4, 1, 1,
    4, 1, 1,
    4, 1, 1,
    4, 1, 1,
    4, 1, 1,
    4, 1, 1,
    4, 1, 1,
    4, 1, 1,
    4, 1, 1,
    4, 1, 1,
    2, 1, 1,
    2, 1, 1,
    2, 1, 1,
    2, 1, 1,
    2, 1, 1,
    2, 1, 1,
    2, 1, 1,
    2, 1, 1,
    2, 1, 1,
    1, 1, 1,
    1, 1, 1,
    1, 1, 1,
    1, 1, 1,
    1, 1, 1,
    4, 1, 1,
    4, 1, 1,
    4, 1, 1,
    2, 1, 1,
    16, 4, 4,
    16, 5, 4,
    16, 5, 5,
    16, 6, 5,
    16, 6, 6,
    16, 8, 5,
    16, 8, 6,
    16, 10, 5,
    16, 10, 6,
    16, 8, 8,
    16, 10, 8,
    16, 10, 10,
    16, 12, 10,
    16, 12, 12,
    16, 4, 4,
    16, 5, 4,
    16, 5, 5,
    16, 6, 5,
    16, 6, 6,
    16, 8, 5,
    16, 8, 6,
    16, 10, 5,
    16, 10, 6,
    16, 8, 8,
    16, 10, 8,
    16, 10, 10,
    16, 12, 10,
    16, 12, 12,
    4, 1, 1,
    0, 0, 0,
    0, 0, 0,
    0, 0, 0,
    0, 0, 0,
    0, 0, 0,
    5, 6, 5,
    1, 0, 0,
    5, 6, 5,
    1, 0, 0,
    5, 6, 5,
    4, 0, 0,
    5, 6, 5,
    4, 0, 0,
    5, 6, 5,
    8, 0, 0,
    5, 6, 5,
    8, 0, 0,
    8, 0, 0,
    0, 0, 0,
    8, 0, 0,
    0, 0, 0,
    8, 8, 0,
    0, 0, 0,
    8, 8, 0,
    0, 0, 0,
    16, 16, 16,
    0, 0, 0,
    16, 16, 16,
    0, 0, 0,
    0, 0, 0,
    0, 0, 0,
    0, 0, 0,
    0, 0, 0,
    32, 32, 32,
    32, 0, 0,
    32, 32, 32,
    32, 0, 0,
    32, 32, 32,
    32, 0, 0,
    32, 32, 32,
    0, 0, 0,
    32, 32, 32,
    0, 0, 0,
    32, 32, 32,
    0, 0, 0,
    16, 16, 16,
    16, 0, 0,
    16, 16, 16,
    16, 0, 0,
    16, 16, 16,
    16, 0, 0,
    16, 16, 16,
    16, 0, 0,
    16, 16, 16,
    16, 0, 0,
    32, 32, 0,
    0, 0, 0,
    32, 32, 0,
    0, 0, 0,
    32, 32, 0,
    0, 0, 0,
    10, 10, 10,
    2, 0, 0,
    10, 10, 10,
    2, 0, 0,
    11, 11, 10,
    0, 0, 0,
    8, 8, 8,
    8, 0, 0,
    8, 8, 8,
    8, 0, 0,
    8, 8, 8,
    8, 0, 0,
    8, 8, 8,
    8, 0, 0,
    8, 8, 8,
    8, 0, 0,
    16, 16, 0,
    0, 0, 0,
    16, 16, 0,
    0, 0, 0,
    16, 16, 0,
    0, 0, 0,
    16, 16, 0,
    0, 0, 0,
    16, 16, 0,
    0, 0, 0,
    32, 0, 0,
    0, 0, 0,
    32, 0, 0,
    0, 0, 0,
    32, 0, 0,
    0, 0, 0,
    8, 8, 0,
    0, 0, 0,
    8, 8, 0,
    0, 0, 0,
    8, 8, 0,
    0, 0, 0,
    8, 8, 0,
    0, 0, 0,
    16, 0, 0,
    0, 0, 0,
    16, 0, 0,
    0, 0, 0,
    16, 0, 0,
    0, 0, 0,
    16, 0, 0,
    0, 0, 0,
    16, 0, 0,
    0, 0, 0,
    8, 0, 0,
    0, 0, 0,
    8, 0, 0,
    0, 0, 0,
    8, 0, 0,
    0, 0, 0,
    8, 0, 0,
    0, 0, 0,
    8, 0, 0,
    0, 0, 0,
    9, 9, 9,
    5, 0, 0,
    10, 10, 10,
    2, 0, 0,
    4, 1, 1,
    0, 32, 0,
    2, 1, 1,
    0, 16, 0,
    0, 0, 0,
    0, 0, 0,
    0, 0, 0,
    0, 0, 0,
    0, 0, 0,
    0, 0, 0,
    0, 0, 0,
    0, 0, 0,
    0, 0, 0,
    0, 0, 0,
    0, 0, 0,
    0, 0, 0,
    0, 0, 0,
    0, 0, 0,
    0, 0, 0,
    0, 0, 0,
    0, 0, 0,
    0, 0, 0,
    0, 0, 0,
    0, 0, 0,
    0, 0, 0,
    0, 0, 0,
    0, 0, 0,
    0, 0, 0,
    0, 0, 0,
    0, 0, 0,
    0, 0, 0,
    0, 0, 0,
    0, 0, 0,
    0, 0, 0,
    0, 0, 0,
    0, 0, 0,
    0, 0, 0,
    0, 0, 0,
    0, 0, 0,
    0, 0, 0,
    0, 0, 0,
    0, 0, 0,
    0, 0, 0,
    0, 0, 0,
    0, 0, 0,
    0, 0, 0,
    0, 0, 0,
    0, 0, 0,
    0, 0, 0,
    0, 0, 0,
    0, 0, 0,
    0, 0, 0,
    0, 0, 0,
    0, 0, 0,
    0, 0, 0,
    0, 0, 0,
    0, 0, 0,
    0, 0, 0,
    0, 0, 0,
    0, 0, 0,
    0, 0, 0,
    0, 24, 8,
    0, 0, 0,
    0, 0, 0,
    0, 0, 0,
    0, 0, 0,
    0, 0, 0,
    0, 0, 0,
    0, 0, 0,
    0, 0, 0,
    0, 0, 0,
    0, 0, 0,
    0, 0, 0,
    0, 0, 0,
    0, 0, 0,
    0, 0, 0,
    0, 0, 0,
    0, 0, 0,
    0, 0, 0,
    0, 0, 0,
    0, 0, 0,
    0, 0, 0,
    0, 0, 0,
    0, 0, 0,
    0, 0, 0,
    0, 0, 0,
    0, 0, 0,
    0, 0, 0,
};

static const DXGI_FORMAT s_PakToDxgiFormat[eTextureFormat::TEX_FMT_COUNT] =
{
    DXGI_FORMAT_BC1_UNORM,
    DXGI_FORMAT_BC1_UNORM_SRGB,
    DXGI_FORMAT_BC2_UNORM,
    DXGI_FORMAT_BC2_UNORM_SRGB,
    DXGI_FORMAT_BC3_UNORM,
    DXGI_FORMAT_BC3_UNORM_SRGB,
    DXGI_FORMAT_BC4_UNORM,
    DXGI_FORMAT_BC4_SNORM,
    DXGI_FORMAT_BC5_UNORM,
    DXGI_FORMAT_BC5_SNORM,
    DXGI_FORMAT_BC6H_UF16,
    DXGI_FORMAT_BC6H_SF16,
    DXGI_FORMAT_BC7_UNORM,
    DXGI_FORMAT_BC7_UNORM_SRGB,
    DXGI_FORMAT_R32G32B32A32_FLOAT,
    DXGI_FORMAT_R32G32B32A32_UINT,
    DXGI_FORMAT_R32G32B32A32_SINT,
    DXGI_FORMAT_R32G32B32_FLOAT,
    DXGI_FORMAT_R32G32B32_UINT,
    DXGI_FORMAT_R32G32B32_SINT,
    DXGI_FORMAT_R16G16B16A16_FLOAT,
    DXGI_FORMAT_R16G16B16A16_UNORM,
    DXGI_FORMAT_R16G16B16A16_UINT,
    DXGI_FORMAT_R16G16B16A16_SNORM,
    DXGI_FORMAT_R16G16B16A16_SINT,
    DXGI_FORMAT_R32G32_FLOAT,
    DXGI_FORMAT_R32G32_UINT,
    DXGI_FORMAT_R32G32_SINT,
    DXGI_FORMAT_R10G10B10A2_UNORM,
    DXGI_FORMAT_R10G10B10A2_UINT,
    DXGI_FORMAT_R11G11B10_FLOAT,
    DXGI_FORMAT_R8G8B8A8_UNORM,
    DXGI_FORMAT_R8G8B8A8_UNORM_SRGB,
    DXGI_FORMAT_R8G8B8A8_UINT,
    DXGI_FORMAT_R8G8B8A8_SNORM,
    DXGI_FORMAT_R8G8B8A8_SINT,
    DXGI_FORMAT_R16G16_FLOAT,
    DXGI_FORMAT_R16G16_UNORM,
    DXGI_FORMAT_R16G16_UINT,
    DXGI_FORMAT_R16G16_SNORM,
    DXGI_FORMAT_R16G16_SINT,
    DXGI_FORMAT_R32_FLOAT,
    DXGI_FORMAT_R32_UINT,
    DXGI_FORMAT_R32_SINT,
    DXGI_FORMAT_R8G8_UNORM,
    DXGI_FORMAT_R8G8_UINT,
    DXGI_FORMAT_R8G8_SNORM,
    DXGI_FORMAT_R8G8_SINT,
    DXGI_FORMAT_R16_FLOAT,
    DXGI_FORMAT_R16_UNORM,
    DXGI_FORMAT_R16_UINT,
    DXGI_FORMAT_R16_SNORM,
    DXGI_FORMAT_R16_SINT,
    DXGI_FORMAT_R8_UNORM,
    DXGI_FORMAT_R8_UINT,
    DXGI_FORMAT_R8_SNORM,
    DXGI_FORMAT_R8_SINT,
    DXGI_FORMAT_A8_UNORM,
    DXGI_FORMAT_R9G9B9E5_SHAREDEXP,
    DXGI_FORMAT_R10G10B10_XR_BIAS_A2_UNORM,
    DXGI_FORMAT_D32_FLOAT,
    DXGI_FORMAT_D16_UNORM,

    // astc
    DXGI_FORMAT_UNKNOWN,
    DXGI_FORMAT_UNKNOWN,
    DXGI_FORMAT_UNKNOWN,
    DXGI_FORMAT_UNKNOWN,
    DXGI_FORMAT_UNKNOWN,
    DXGI_FORMAT_UNKNOWN,
    DXGI_FORMAT_UNKNOWN,
    DXGI_FORMAT_UNKNOWN,
    DXGI_FORMAT_UNKNOWN,
    DXGI_FORMAT_UNKNOWN,
    DXGI_FORMAT_UNKNOWN,
    DXGI_FORMAT_UNKNOWN,
    DXGI_FORMAT_UNKNOWN,
    DXGI_FORMAT_UNKNOWN,
    DXGI_FORMAT_UNKNOWN,
    DXGI_FORMAT_UNKNOWN,
    DXGI_FORMAT_UNKNOWN,
    DXGI_FORMAT_UNKNOWN,
    DXGI_FORMAT_UNKNOWN,
    DXGI_FORMAT_UNKNOWN,
    DXGI_FORMAT_UNKNOWN,
    DXGI_FORMAT_UNKNOWN,
    DXGI_FORMAT_UNKNOWN,
    DXGI_FORMAT_UNKNOWN,
    DXGI_FORMAT_UNKNOWN,
    DXGI_FORMAT_UNKNOWN,
    DXGI_FORMAT_UNKNOWN,
    DXGI_FORMAT_UNKNOWN,

    DXGI_FORMAT_D24_UNORM_S8_UINT,

    DXGI_FORMAT_UNKNOWN,
};

// for switch, seems to be nvn, can't find the enum for nvn formats
/*static const int s_PakToNVNFormat[eTextureFormat::TEX_FMT_COUNT] =
{
    0x42,
    0x46,
    0x43,
    0x47,
    0x44,
    0x48,
    0x49,
    0x4A,
    0x4B,
    0x4C,
    0x50,
    0x4F,
    0x4D,
    0x4E,
    0x2E,
    0x2F,
    0x30,
    0x22,
    0x23,
    0x24,
    0x29,
    0x2A,
    0x2C,
    0x2B,
    0x2D,
    0x16,
    0x17,
    0x18,
    0x3D,
    0x3E,
    0x3F,
    0x25,
    0x38,
    0x27,
    0x26,
    0x28,
    0x11,
    0x12,
    0x14,
    0x13,
    0x15,
    0xA,
    0xB,
    0xC,
    0xD,
    0xF,
    0xE,
    0x10,
    0x5,
    0x6,
    0x8,
    0x7,
    0x9,
    0x1,
    0x3,
    0x2,
    0x4,
    0x1,
    0x0,
    0x0,
    0x34,
    0x32,
    0x79,
    0x7A,
    0x7B,
    0x7C,
    0x7D,
    0x7E,
    0x7F,
    0x81,
    0x82,
    0x80,
    0x83,
    0x84,
    0x85,
    0x86,
    0x87,
    0x88,
    0x89,
    0x8A,
    0x8B,
    0x8C,
    0x8D,
    0x8F,
    0x90,
    0x8E,
    0x91,
    0x92,
    0x93,
    0x94,
    0x35,

    0x0, // don't have a build with this yet
};*/

inline const bool IsSRGB(DXGI_FORMAT format)
{
    switch (format)
    {
    case DXGI_FORMAT_R8G8B8A8_UNORM_SRGB:
    case DXGI_FORMAT_BC1_UNORM_SRGB:
    case DXGI_FORMAT_BC2_UNORM_SRGB:
    case DXGI_FORMAT_BC3_UNORM_SRGB:
    case DXGI_FORMAT_B8G8R8A8_UNORM_SRGB:
    case DXGI_FORMAT_B8G8R8X8_UNORM_SRGB:
    case DXGI_FORMAT_BC7_UNORM_SRGB:
        return true;

    default:
        return false;
    }
}

enum eTextureType : uint8_t
{
    UNK_0 = 0, // combined. seen with vxd/opa (0x180ACCBA656EED38) and bm/gls/bm (0x2E93F226C59E8035)
    COL = 4,
    UNK_5 = 5,  // mostly used for _opa, _alpha, _msk
    DET = 6,  // no clear pattern. maybe detail? _det, _detail, _dcol
    OPA = 8,
    UNK_9 = 9, // combined _opa _msk. e.g. texture\\combined\\[104c458f]bloodhound_lgnd_v19_halloween_gear_opa[7ca33a66]bloodhound_lgnd_v19_halloween_rt01_gear_msk.rpak
    NML = 12,
    SPC = 13,
    ASD = 14, // anisotropic specular direction
    ASA_15 = 15, // sometimes used for _asa textures
    ASA = 16, // i assume anisotropic specular amount?
    GLS = 17,
    ILM = 19,
    EHM_22 = 22, // sometimes used for _ehm textures
    EHL = 23,
    EHM = 24, // emissive heightmap ramp
    AO = 25,
    CAV = 26,
    TNT = 27, // yea idk. tint?
    SCTR = 28,

    THK_30 = 30, // sometimes used for _thk textures
    THK = 31,
    VXD = 33, // vertex displace

    UNK_34 = 34, // ui related ?
    FONT_ATLAS = 35, // used for texture/ui_atlas/fontAtlas.rpak and asianFontAtlas.rpak

    UNK_37 = 37, // sometimes used for _bm textures, otherwise not sure
    UNK_39 = 39, // opacityMultiplyTexture
    UNK_44 = 44, // _opa?
    UNK_45 = 45, // used for skin _col textures and particle textures

    _UNUSED = 0xFF, // for v8 init
};

static const std::map<uint8_t, const char* const> s_TextureTypeMap
{
    { eTextureType::COL, "_col" },      // albedoTexture, albedo2Texture, iridescenceRampTexture, emissiveTexture
    { eTextureType::UNK_5, "_msk" },    // albedoTexture, opacityMultiplyTexture
    { eTextureType::DET, "_det" },      // detailTexture
    { eTextureType::OPA, "_opa" },      // opacityMultiplyTexture
    { eTextureType::NML, "_nml" },      // normalTexture, normal2Texture, detailNormalTexture, uvDistortionTexture, uvDistortion2Texture
    { eTextureType::SPC, "_spc" },      // specTexture, spec2Texture
    { eTextureType::ASD, "_asd" },      // anisoSpecDirTexture
    { eTextureType::ASA_15, "_asa" },   // anisoSpecDirTexture
    { eTextureType::ASA, "_asa" },      // anisoSpecDirTexture
    { eTextureType::GLS, "_gls" },      // glossTexture, gloss2Texture
    { eTextureType::ILM, "_ilm" },      // emissiveTexture, emissiveMultiplyTexture, emissive2Texture, albedoTexture, albedoMultiplyTexture
    { eTextureType::EHM_22, "_ehm" },   // emissiveMultiplyTexture
    { eTextureType::EHL, "_ehl" },      // emissiveTexture
    { eTextureType::EHM, "_ehm" },      // emissiveMultiplyTexture
    { eTextureType::AO, "_ao" },        // aoTexture, ao2Texture
    { eTextureType::CAV, "_cav" },      // cavityTexture
    { eTextureType::TNT, "_tnt" },
    { eTextureType::SCTR, "_sctr" },    // scatterThicknessTexture
    { eTextureType::THK_30, "_thk" },   // scatterThicknessTexture
    { eTextureType::THK, "_thk" },      // scatterThicknessTexture
    { eTextureType::VXD, "_vxd" },
    { eTextureType::UNK_34, "_col" }, // albedoTexture
    { eTextureType::UNK_37, "_bm" },    // layerBlendTexture
    { eTextureType::UNK_44, "_opa" },   // opacityMultiplyTexture
    { eTextureType::UNK_45, "_col" },   // albedoTexture, albedoMultiplyTexture, emissiveTexture, emissiveMultiplyTexture, colorRampTexture, opacityMultiplyTexture, detailTexture
};

inline std::string GetTextureSuffixByType(uint8_t type)
{
    if (s_TextureTypeMap.count(type) != 0)
        return s_TextureTypeMap.at(type);

    return "_type" + std::to_string(type);
}

enum class eTextureMipType : uint8_t
{
    RPak, // PERM
    StarPak, // STREAMED
    OptStarPak, // OPTSTREAMED

    _COUNT,
};

// [rika]: this really should be called texture tiling, 'ps4' is also used by xb1
enum eTextureSwizzle : uint8_t
{
    SWIZZLE_NONE = 0,
    SWIZZLE_PS4 = 8,
    SWIZZLE_SWITCH = 9,

    _SWIZZLECOUNT,
};

static constexpr int s_MipAligment[eTextureSwizzle::_SWIZZLECOUNT] =
{
    16, // pc
    -1,
    -1,
    -1,
    -1,
    -1,
    -1,
    -1,
    512, // ps4
    512, // switch
};

static constexpr int s_SwizzleChunkSizePS4 = 8;
#ifdef SWITCH_SWIZZLE
static constexpr int s_SwizzleChunkSizeSwitchX = 4;
static constexpr int s_SwizzleChunkSizeSwitchY = 8;

// lut for switch 'morton'
static const int s_SwitchSwizzleLUT[32] =
{
    0,
    4,
    1,
    5,
    8,
    12,
    9,
    13,
    16,
    20,
    17,
    21,
    24,
    28,
    25,
    29,
    2,
    6,
    3,
    7,
    10,
    14,
    11,
    15,
    18,
    22,
    19,
    23,
    26,
    30,
    27,
    31,
};
#endif

struct TextureMip_t
{
    AssetPtr_t assetPtr;
    eTextureMipType type;
    eCompressionType compType;

    uint8_t level; // level of this mip, for later reference

    bool isLoaded;

    uint16_t width;
    uint16_t height;

    uint32_t pitch;
    uint32_t slicePitch;
    size_t sizeSingle; // size of a single entry (arrays of textures)
    size_t sizeAbsolute; // size on disk

    eTextureSwizzle swizzle;
};

struct TextureAssetHeader_v8_t
{
    uint64_t guid;
    char* name;

    uint16_t width;
    uint16_t height;
    uint16_t depth;

    eTextureFormat imgFormat;
    uint32_t dataSize;

    uint8_t unk_1C; // used for a minimum data 'chunk size', 1 << unk_1C. 'dataAlignment' ? but it doesn't line up with the disk alignment, research further.
    uint8_t optStreamedMipLevels;
    uint8_t arraySize; // createtexture2d desc param

    // [amos]: are we sure this is layerCount? this looks like resourceFlags.
    // It is used in the CreateTexture function to check if a texture is a
    // cubemap (layerCount & 2). That is the only usage I've seen so far. It
    // is being used since S3.1.
    uint8_t layerCount;
    uint8_t usageFlags; // ?
    uint8_t permanentMipLevels;
    uint8_t streamedMipLevels;

    uint8_t unk_23[13];
    __int64 numPixels; // reserved
};
static_assert(sizeof(TextureAssetHeader_v8_t) == 56);

struct TextureAssetHeader_v9_t
{
    char* name;

    eTextureFormat imgFormat;
    uint16_t width;
    uint16_t height;
    uint16_t depth;

    uint8_t arraySize;

    // [amos]: are we sure this is layerCount? this looks like resourceFlags.
    // It is used in the CreateTexture function to check if a texture is a
    // cubemap (layerCount & 2). That is the only usage I've seen so far. It
    // is being used since S3.1.
    uint8_t layerCount;

    uint8_t unk_12; // unk0
    uint8_t usageFlags; // "mipFlags"

    uint32_t dataSize;
    uint8_t permanentMipLevels;
    uint8_t streamedMipLevels;
    uint8_t optStreamedMipLevels;

    eTextureType type;

    uint16_t compTypePacked;
    uint16_t compressedBytes[7];

    uint16_t unk_2C[2];
    uint8_t unk_30[8]; // filled per streamed mip
};
static_assert(sizeof(TextureAssetHeader_v9_t) == 56);

struct TextureAssetHeader_v10_t
{
    char* name;

    eTextureFormat imgFormat;
    uint16_t width;
    uint16_t height;
    uint16_t depth;

    uint8_t arraySize;

    // [amos]: are we sure this is layerCount? this looks like resourceFlags.
    // It is used in the CreateTexture function to check if a texture is a
    // cubemap (layerCount & 2). That is the only usage I've seen so far. It
    // is being used since S3.1.
    uint8_t layerCount;

    uint8_t unk_12;

    // This is still usage flags, but it now indexes into an array
    // (see dword_14128A858 in 180_J25_CL4941853_2023_07_27_16_31)
    // to get the D3D11_USAGE flags. However, it is still the same
    // as earlier versions, earlier versions however only nulled
    // the usage flags if this field was 2.
    uint8_t usageFlags;

    uint32_t dataSize;
    uint8_t permanentMipLevels;
    uint8_t streamedMipLevels;
    uint8_t optStreamedMipLevels;
    uint8_t unk_1B; // related to mips, used oddly
    uint8_t unkMipLevels; // new type of streamed mip, gets added on to opt stream & streamed mip count. V9 does not have this.

    eTextureType type;

    // this is packed eCompressionType (2 bits per enum), not all streamed mips are compressed.
    // (compTypePacked >> (2 * mipIndex)) & 3
    uint16_t compTypePacked;
    uint16_t compressedBytes[7];

    uint16_t unk_2E; // unk_2C but uint8_t probably
    uint8_t unk_30[8]; // filled per streamed mip
};
static_assert(sizeof(TextureAssetHeader_v10_t) == 56);

enum class eTextureMipSorting : uint8_t
{
    SORT_BOTTOM_TOP, // pc
    SORT_TOP_BOTTOM, // consoles
    SORT_MIXED, // retail pc, perm mips bottom to top, stream mips top to bottom
};

class TextureAsset
{
public:
    TextureAsset() = default;
    TextureAsset(TextureAssetHeader_v8_t* const hdr) : guid(hdr->guid), name(hdr->name),
        width(hdr->width), height(hdr->height), imgFormat(hdr->imgFormat),
        dataSize(hdr->dataSize), dataSizeUnaligned(0), arraySize(hdr->arraySize), layerCount(hdr->layerCount), usageFlags(hdr->usageFlags),
        permanentMipLevels(hdr->permanentMipLevels), streamedMipLevels(hdr->streamedMipLevels), optStreamedMipLevels(hdr->optStreamedMipLevels), unkMipLevels(0u), totalMipLevels(0u), mipSorting(eTextureMipSorting::SORT_BOTTOM_TOP),
        type(eTextureType::_UNUSED), legacy(true), compTypePacked(0), compressedBytes(), swizzle(static_cast<eTextureSwizzle>(hdr->unk_1C))
    {
        memcpy_s(unkPerMip_Legacy, sizeof(unkPerMip_Legacy), hdr->unk_23, sizeof(hdr->unk_23));
    };

    TextureAsset(TextureAssetHeader_v9_t* const hdr, const uint64_t guid) : guid(guid), name(hdr->name),
        width(hdr->width), height(hdr->height), imgFormat(hdr->imgFormat),
        dataSize(hdr->dataSize), dataSizeUnaligned(0), arraySize(hdr->arraySize), layerCount(hdr->layerCount), usageFlags(hdr->usageFlags),
        permanentMipLevels(hdr->permanentMipLevels), streamedMipLevels(hdr->streamedMipLevels), optStreamedMipLevels(hdr->optStreamedMipLevels), unkMipLevels(0u), totalMipLevels(0u), mipSorting(eTextureMipSorting::SORT_MIXED),
        type(hdr->type), legacy(false), compTypePacked(hdr->compTypePacked), swizzle(static_cast<eTextureSwizzle>(hdr->unk_12))
    {
        memcpy_s(compressedBytes, sizeof(compressedBytes), hdr->compressedBytes, sizeof(hdr->compressedBytes));
    };

    TextureAsset(TextureAssetHeader_v10_t* const hdr, const uint64_t guid) : guid(guid), name(hdr->name),
        width(hdr->width), height(hdr->height), imgFormat(hdr->imgFormat),
        dataSize(hdr->dataSize), dataSizeUnaligned(0), arraySize(hdr->arraySize), layerCount(hdr->layerCount), usageFlags(hdr->usageFlags),
        permanentMipLevels(hdr->permanentMipLevels), streamedMipLevels(hdr->streamedMipLevels), optStreamedMipLevels(hdr->optStreamedMipLevels), unkMipLevels(hdr->unkMipLevels), totalMipLevels(0u), mipSorting(eTextureMipSorting::SORT_MIXED),
        type(hdr->type), legacy(false), compTypePacked(hdr->compTypePacked), swizzle(static_cast<eTextureSwizzle>(hdr->unk_12))
    {
        memcpy_s(compressedBytes, sizeof(compressedBytes), hdr->compressedBytes, sizeof(hdr->compressedBytes));
    };

    //
    uint64_t guid;
    char* name;

    uint16_t width;
    uint16_t height;
    eTextureFormat imgFormat;

    uint32_t dataSize;
    uint32_t dataSizeUnaligned; // size of the 'raw' texture data without an asset's applied alignment

    uint8_t arraySize;
    uint8_t layerCount;
    uint8_t usageFlags;

    uint8_t permanentMipLevels;
    uint8_t streamedMipLevels;
    uint8_t optStreamedMipLevels;
    uint8_t unkMipLevels; // treated like a streamed mip
    uint8_t totalMipLevels;
    eTextureMipSorting mipSorting;

    // used for consoles?
    eTextureSwizzle swizzle;

    eTextureType type;

    // Set if this is a V8 texture.
    bool legacy;

    uint16_t compTypePacked;
    uint16_t compressedBytes[7];
    inline const eCompressionType StreamMipCompression(const uint8_t streamMipIdx) const
    {
        assertm(streamMipIdx < (totalMipLevels - permanentMipLevels), "mip index was not streamed.");
        return static_cast<eCompressionType>((compTypePacked >> (2 * streamMipIdx)) & 3);
    }

    uint8_t unkPerMip_Legacy[13];

    std::vector<TextureMip_t> mipArray;
};

enum eTextureExportSetting
{
    PNG_HM,  // PNG (Highest Mip)
    PNG_AM,  // PNG (All Mips)
    DDS_HM,  // DDS (Highest Mip)
    DDS_AM,  // DDS (All Mips)
    DDS_MM,  // DDS (Mip Mapped)
    DDS_MD,  // DDS (Meta Data)
};

bool ExportPngTextureAsset(CPakAsset* const asset, const TextureAsset* const txtrAsset, std::filesystem::path& exportPath, const int setting, const bool isNormal);
bool ExportDdsTextureAsset(CPakAsset* const asset, const TextureAsset* const txtrAsset, std::filesystem::path& exportPath, const int setting, const bool isNormal);
std::unique_ptr<char[]> GetTextureDataForMip(CPakAsset* const asset, const TextureMip_t* const mip, const DXGI_FORMAT format, const size_t arrayIndex = 0);
std::shared_ptr<CTexture> CreateTextureFromMip(CPakAsset* const asset, const TextureMip_t* const mip, const DXGI_FORMAT format, const size_t arrayIdx = 0);