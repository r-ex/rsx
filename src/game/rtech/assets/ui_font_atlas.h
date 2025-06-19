#pragma once
#include <d3d11.h>
#include <game/rtech/cpakfile.h>

class UIFontAtlasAsset;

// v6 r2tt
// v7 r2 + r5 (s0 - s15.1)
// v8 (skipped)
// v9 (skipped)
// v10 s16
// v11 (skipped)
// v12 r5 (s16.1 - retail)

constexpr uint32_t FONT_TEXTURE_IDX_INVALID = 0xffffffff;
constexpr int FONT_UTF16_QMARK = L'?';
constexpr int FONT_UTF16_BOX = 0x25A1;
constexpr int FONT_MAX_PROPORTIONS = 8;

#define MAX_FONTS 32

// guh
struct UIFont_UNK_t
{
    int unk_0;
    float unk_4;
};

// character proportions
// for different sizes and shapes of characters
struct UIFontProportion_v7_t
{
    float scaleBounds; // for the addition size added on to of the base size
    float scaleSize; // for the basic character size
};

struct UIFontProportion_v12_t
{
    float scaleBounds; // for the addition size added on to of the base size
    float scaleSize; // for the basic character size

    int unk_8;
    float unk_C;
    int unk_10;
    float unk_14;
};

struct UIFontProportion
{
    UIFontProportion() = default;
    UIFontProportion(const float bounds, const float size) : scaleBounds(bounds), scaleSize(size) {};
    UIFontProportion(const UIFontProportion_v7_t* const prop) : scaleBounds(prop->scaleBounds), scaleSize(prop->scaleSize) {};
    UIFontProportion(const UIFontProportion_v12_t* const prop) : scaleBounds(prop->scaleBounds), scaleSize(prop->scaleSize) {};

    float scaleBounds; // for the addition size added on to of the base size
    float scaleSize; // for the basic character size
};

// textures
struct UIFontTexture_v6_t
{
    // for kerning?
    float unk_0;
    uint16_t unk_4;
    uint8_t unk_6;
    uint8_t unk_7;

    // approximate, fixed with other values. maybe this is into a chunk/sector?
    float posBaseX;
    float posBaseY;

    // local to posBase, these get scaled by vector2d from the font, likely to preserve some precision
    float posMinX;
    float posMinY;

    float posMaxX;
    float posMaxY;
};

struct UIFontTexture_v7_t
{
    // for kerning?
    float unk_0;
    uint16_t unk_4;
    uint8_t unk_6;

    uint8_t proportionIndex; // index into an array of two floats for different proportioned characters

    // approximate, fixed with other values. maybe this is into a chunk/sector?
    float posBaseX;
    float posBaseY;

    // local to posBase, these get scaled by vector2d from the font, likely to preserve some precision
    float posMinX;
    float posMinY;

    float posMaxX;
    float posMaxY;
};

struct UIFontTexture_v12_t
{
    // for kerning?
    float unk_0;
    uint16_t unk_4;

    uint16_t unk_6;

    char unk_8;

    char proportionIndex; // index into an array of fixed up Vector2D from FontHeader_t unk_48. max of 29 (only 30 vectors)

    uint8_t unk_A[2];

    // approximate, fixed with other values.
    float posBaseX;
    float posBaseY;

    // local to posBase, these get scaled by vector2d from the font, likely to preserve some precision
    float posMinX;
    float posMinY;

    float posMaxX;
    float posMaxY;
};

struct UIFontTexture
{
    UIFontTexture() = default;
    UIFontTexture(UIFontTexture_v6_t* tex) : proportionIndex(0), posBaseX(tex->posBaseX), posBaseY(tex->posBaseY), posMinX(tex->posMinX), posMinY(tex->posMinY), posMaxX(tex->posMaxX), posMaxY(tex->posMaxY) {};
    UIFontTexture(UIFontTexture_v7_t* tex) : proportionIndex(tex->proportionIndex), posBaseX(tex->posBaseX), posBaseY(tex->posBaseY), posMinX(tex->posMinX), posMinY(tex->posMinY), posMaxX(tex->posMaxX), posMaxY(tex->posMaxY) {};
    UIFontTexture(UIFontTexture_v12_t* tex) : proportionIndex(tex->proportionIndex), posBaseX(tex->posBaseX), posBaseY(tex->posBaseY), posMinX(tex->posMinX), posMinY(tex->posMinY), posMaxX(tex->posMaxX), posMaxY(tex->posMaxY) {};

    uint8_t proportionIndex; // index into an array of two floats for different proportioned characters

    // approximate, fixed with other values. maybe this is into a chunk/sector?
    float posBaseX;
    float posBaseY;

    // local to posBase, these get scaled by vector2d from the font, likely to preserve som precision
    float posMinX;
    float posMinY;

    float posMaxX;
    float posMaxY;
};

// unicode map (v6)
struct UIFontUnicodeMap_t
{
    int unicodeFirst; // first valid unicode in this block
    int unicodeLast;  // last valid unicode in this block
    int textureIndex; // index into textures
};

// font headers
#define GetFontIndex(hash) ((hash ^ (hash >> 11)) & 1f) // [rika]: random note we could probably use this and fonts from rtk files to name fonts (in retail)

struct UIFontHeader_v6_t
{
    char* name;

    uint16_t fontIndex; // font index within the global font list, gotten via GetFontIndex(hash) where hash is the output of RTech::StringToGuid when provided with this font's name

    uint16_t numUnicodeMaps;
    uint32_t numTextures; // used for mem alloc

    // for scaling the character proportions 
    float proportionScaleX;
    float proportionScaleY;

    float unk_18[2];

    float proportionSizeScale;

    uint16_t numGlyphChunks; // also the count for unk_38 (which is why the name carries over)

    // base index for texture count
    uint16_t textureIndex;

    UIFontUnicodeMap_t* unicodeMap; // unicodes?
    UIFontTexture_v6_t* textures; // array of UIFontTexture_v6_t

    UIFont_UNK_t* unk_38; // for kerning?
};
static_assert(sizeof(UIFontHeader_v6_t) == 0x40);

struct UIFontHeader_v7_t
{
    char* name;

    uint16_t fontIndex; // font index within the global font list, gotten via GetFontIndex(hash) where hash is the output of RTech::StringToGuid when provided with this font's name

    uint16_t numProportions; // number of character proportions

    // number of chunks for glyph and unicode lookup, 64 unique glyphs/unicodes per chunk
    uint16_t numGlyphChunks; // unused
    uint16_t numUnicodeChunks;

    int glyphIndex; // base glyph index, unused
    int unicodeIndex; // base unicode character index, this gets subtracted off input characters, everything before this index is invalid.

    uint32_t numTextures; // used for mem alloc

    // for scaling the character proportions
    float proportionScaleX;
    float proportionScaleY;

    float unk_24[2];

    // base index for texture count
    uint32_t textureIndex; // used for mem alloc

    // for getting a texture from a provided unicode/glyph
    uint16_t* unicodeChunks; // index per 64 unicodes, index into other arrays to get assigned texture (unicodeBaseIndex & unicodeIndexMask)
    uint16_t* unicodeChunksIndex; // the base texture index, added with popcount from unicodeIndexMask
    uint64_t* unicodeChunksMask; // each bit represents a single unicode

    UIFontProportion_v7_t* proportions; // array of UIFontProportion_v7_t
    UIFontTexture_v7_t* textures; // array of UIFontTexture_v7_t

    UIFont_UNK_t* unk_58; // for kerning?
};
static_assert(sizeof(UIFontHeader_v7_t) == 0x60);

struct UIFontHeader_v12_t
{
    char* name;

    uint16_t fontIndex; // font index within the global font list, gotten via GetFontIndex(hash) where hash is the output of RTech::StringToGuid when provided with this font's name

    uint16_t numProportions; // number of character proportions

    // number of chunks for glyph and unicode lookup, 64 unique glyphs/unicodes per chunk
    uint16_t numGlyphChunks; // not count for unk_88??
    uint16_t numUnicodeChunks;

    int glyphIndex; // base glyph index
    int unicodeIndex; // base unicode character index, this gets subtracted off input characters, everything before this index is invalid.

    uint32_t numGlyphTextures; // used for mem alloc TODO: is this just number of glyphs
    uint32_t numUnicodeTextures;

    // for scaling the character proportions
    float proportionScaleX;
    float proportionScaleY;

    float unk_28[2];

    // base index for texture count
    uint32_t textureIndex; // used for mem alloc

    int errorGlyph;

    // for getting a texture from a provided unicode/glyph
    uint16_t* glyphChunks;
    uint16_t* unicodeChunks; // index per 64 unicodes, index into other arrays to get assigned texture (unicodeBaseIndex & unicodeIndexMask)
    uint16_t* glyphChunksIndex;
    uint16_t* unicodeChunksIndex; // the base texture index, added with popcount from unicodeIndexMask
    uint64_t* glyphChunksMask;
    uint64_t* unicodeChunksMask; // each bit represents a single unicode

    UIFontProportion_v12_t* proportions; // array of vector2d
    UIFontTexture_v12_t* glyphTextures; // per texture, 32 bytes
    UIFontTexture_v12_t* unicodeTextures;

    void* unk_80;

    UIFont_UNK_t* unk_88;
};
static_assert(sizeof(UIFontHeader_v12_t) == 0x90);

// asset headers
struct UIFontAtlasAssetHeader_v6_t
{
    uint16_t fontCount;
    uint16_t unk_2; // count for the data at unk_18

    uint16_t width;
    uint16_t height;

    // like ui image atlas
    float widthRatio;
    float heightRatio;

    void* fonts;
    void* unk_18;

    uint64_t atlasGUID; // guid
};
static_assert(sizeof(UIFontAtlasAssetHeader_v6_t) == 0x28);

struct UIFontAtlasAssetHeader_v12_t
{
    uint16_t fontCount;
    uint16_t unk_2; // count for the data at unk_40

    uint16_t width;
    uint16_t height;

    // like ui image atlas
    float widthRatio;
    float heightRatio;

    char unk_10[40];

    void* fonts;
    void* unk_40; // previously unk_18

    uint64_t atlasGUID; // guid

    void* unk_50;
    void* unk_58;
    int unk_60[2];
};
static_assert(sizeof(UIFontAtlasAssetHeader_v12_t) == 0x68);

// generic
class UIFontHeader
{
public:
    UIFontHeader(const UIFontHeader_v6_t* const font) : name(font->name), fontIndex(font->fontIndex), textureIndex(font->textureIndex), proportionScaleX(font->proportionScaleX), proportionScaleY(font->proportionScaleY),
        glyphIndex(0u), unicodeIndex(0u), errorGlyph(0), numProportions(1u), numGlyphTextures(0u), numUnicodeTextures(font->numTextures), proportions(), glyphTextures(nullptr), unicodeTextures(nullptr),
        unicodeMap(font->unicodeMap), glyphChunks(nullptr), glyphChunksIndex(nullptr), glyphChunksMask(nullptr), unicodeChunks(nullptr), unicodeChunksIndex(nullptr), unicodeChunksMask(nullptr), numGlyphChunks(font->numGlyphChunks), numUnicodeChunks(0u), numUnicodeMaps(font->numUnicodeMaps)
    {
        proportions[0] = UIFontProportion(1.0f, font->proportionSizeScale);

        if (numUnicodeTextures)
        {
            unicodeTextures = new UIFontTexture[numUnicodeTextures]{};

            for (uint32_t i = 0u; i < numUnicodeTextures; i++)
                unicodeTextures[i] = UIFontTexture(&font->textures[i]);
        }
    };

    UIFontHeader(const UIFontHeader_v7_t* const font) : name(font->name), fontIndex(font->fontIndex), textureIndex(font->textureIndex), proportionScaleX(font->proportionScaleX), proportionScaleY(font->proportionScaleY),
        glyphIndex(font->glyphIndex), unicodeIndex(font->unicodeIndex), errorGlyph(0), numProportions(font->numProportions), numGlyphTextures(0u), numUnicodeTextures(font->numTextures), proportions(), glyphTextures(nullptr), unicodeTextures(nullptr),
        unicodeMap(nullptr), glyphChunks(nullptr), glyphChunksIndex(nullptr), glyphChunksMask(nullptr), unicodeChunks(font->unicodeChunks), unicodeChunksIndex(font->unicodeChunksIndex), unicodeChunksMask(font->unicodeChunksMask), numGlyphChunks(font->numGlyphChunks), numUnicodeChunks(font->numUnicodeChunks), numUnicodeMaps(0u)
    {
        for (uint16_t i = 0; i < font->numProportions; i++)
            proportions[i] = UIFontProportion(&font->proportions[i]);

        if (numUnicodeTextures)
        {
            unicodeTextures = new UIFontTexture[numUnicodeTextures]{};

            for (uint32_t i = 0u; i < numUnicodeTextures; i++)
                unicodeTextures[i] = UIFontTexture(&font->textures[i]);
        }
    };

    UIFontHeader(const UIFontHeader_v12_t* const font) : name(font->name), fontIndex(font->fontIndex), textureIndex(font->textureIndex), proportionScaleX(font->proportionScaleX), proportionScaleY(font->proportionScaleY),
        glyphIndex(font->glyphIndex), unicodeIndex(font->unicodeIndex), errorGlyph(font->errorGlyph), numProportions(font->numProportions), numGlyphTextures(font->numGlyphTextures), numUnicodeTextures(font->numUnicodeTextures), proportions(), glyphTextures(nullptr), unicodeTextures(nullptr),
        unicodeMap(nullptr), glyphChunks(font->glyphChunks), glyphChunksIndex(font->glyphChunksIndex), glyphChunksMask(font->glyphChunksMask), unicodeChunks(font->unicodeChunks), unicodeChunksIndex(font->unicodeChunksIndex), unicodeChunksMask(font->unicodeChunksMask), numGlyphChunks(font->numGlyphChunks), numUnicodeChunks(font->numUnicodeChunks), numUnicodeMaps(0u)
    {
        assertm(fontIndex < MAX_FONTS, "assure max number of fonts is unchanged");

        for (uint16_t i = 0; i < font->numProportions; i++)
            proportions[i] = UIFontProportion(&font->proportions[i]);

        if (numUnicodeTextures)
        {
            unicodeTextures = new UIFontTexture[numUnicodeTextures]{};

            for (uint32_t i = 0u; i < numUnicodeTextures; i++)
                unicodeTextures[i] = UIFontTexture(&font->unicodeTextures[i]);
        }

        // [rika]: turns out this can be nullptr in switch!
        if (numGlyphTextures && font->glyphTextures)
        {
            glyphTextures = new UIFontTexture[numGlyphTextures]{};

            for (uint32_t i = 0u; i < numGlyphTextures; i++)
                glyphTextures[i] = UIFontTexture(&font->glyphTextures[i]);
        }
    };

    ~UIFontHeader()
    {
        FreeAllocArray(glyphTextures)
        FreeAllocArray(unicodeTextures)
    }

    char* name;

    uint32_t textureIndex; // the base index for the textures in this font

    uint32_t numGlyphTextures; // used for mem alloc TODO: is this just number of glyphs
    uint32_t numUnicodeTextures;

    float proportionScaleX;
    float proportionScaleY;

    int glyphIndex;
    int unicodeIndex;
    
    int errorGlyph;

    UIFontProportion proportions[FONT_MAX_PROPORTIONS]; // array of vector2d
    UIFontTexture* glyphTextures; // per texture, 32 bytes
    UIFontTexture* unicodeTextures;

    // for getting textures from a provided unicode via blocks of indices
    UIFontUnicodeMap_t* unicodeMap;

    // for getting a texture from a provided glyph via bits
    uint16_t* glyphChunks;
    uint16_t* glyphChunksIndex;
    uint64_t* glyphChunksMask;

    // for getting a texture from a provided unicode via bits
    uint16_t* unicodeChunks;
    uint16_t* unicodeChunksIndex;
    uint64_t* unicodeChunksMask;

    // number of maps
    uint16_t numGlyphChunks;
    uint16_t numUnicodeChunks;
    uint16_t numUnicodeMaps;

    uint16_t numProportions;

    uint16_t fontIndex;

    const uint32_t TextureFromUnicodeMap(int unicode) const;
    const uint32_t TextureFromUnicode(int unicode) const;
    const uint32_t TextureFromGlyph(int glyph) const;

    void ParseProportions(Vector2D* const imageBoundsScale, Vector2D* const imageSizeBase) const;

    void SetCharacterForImages(const UIFontAtlasAsset* const fontAsset, const int maxUnicode, const uint32_t numTextures, const uint32_t(UIFontHeader::*TextureFromCharacter)(int) const) const;

private:
    inline const uint32_t GetTextureIndexFromUnicodeMap(int unicode) const;
};

struct UIFontCharacter_t
{
    Vector2D mins;
    Vector2D maxs;

    uint16_t posX;
    uint16_t posY;

    uint16_t width;
    uint16_t height;

    int utf16;
    int glyph;

    uint16_t font; // index to which font owns this character

    void SetBounds(const UIFontAtlasAsset* const fontAsset, const UIFontTexture* const texture, const Vector2D* const imageBoundsScale, const Vector2D* const imageSizeBase);
};

class CTexture;
class UIFontAtlasAsset
{
public:
    UIFontAtlasAsset(const UIFontAtlasAssetHeader_v6_t* const hdr, const int version) : fontCount(hdr->fontCount), fonts(hdr->fonts), width(hdr->width), height(hdr->height), widthRatio(hdr->widthRatio), heightRatio(hdr->heightRatio), atlasGUID(hdr->atlasGUID), imageCount(0ll), images(nullptr), txtrFormat(DXGI_FORMAT_UNKNOWN)
    {
        fontData.reserve(fontCount);
        switch (version)
        {
        case 6:
        {
            for (uint16_t i = 0; i < fontCount; i++)
                fontData.emplace_back(&reinterpret_cast<UIFontHeader_v6_t*>(hdr->fonts)[i]);

            break;
        }
        case 7:
        case 10:
        {
            for (uint16_t i = 0; i < fontCount; i++)
                fontData.emplace_back(&reinterpret_cast<UIFontHeader_v7_t*>(hdr->fonts)[i]);

            break;
        }
        default:
        {
            break;
        }
        }
    };

    UIFontAtlasAsset(const UIFontAtlasAssetHeader_v12_t* const hdr) : fontCount(hdr->fontCount), fonts(hdr->fonts), width(hdr->width), height(hdr->height), widthRatio(hdr->widthRatio), heightRatio(hdr->heightRatio), atlasGUID(hdr->atlasGUID), imageCount(0ll), images(nullptr), txtrFormat(DXGI_FORMAT_UNKNOWN)
    {
        fontData.reserve(fontCount);

        for (uint16_t i = 0; i < fontCount; i++)
            fontData.emplace_back(&reinterpret_cast<UIFontHeader_v12_t*>(hdr->fonts)[i]);
    };

    ~UIFontAtlasAsset()
    {
        FreeAllocArray(images);
    };

    uint16_t fontCount;

    uint16_t width;
    uint16_t height;

    // like ui image atlas
    float widthRatio;
    float heightRatio;

    void* fonts;

    uint64_t atlasGUID; // guid

    UIFontCharacter_t* images;
    int imageCount;

    std::vector<UIFontHeader> fontData;

    std::shared_ptr<CTexture> txtrRaw;
    std::shared_ptr<CTexture> txtrConverted;
    DXGI_FORMAT txtrFormat;
};
