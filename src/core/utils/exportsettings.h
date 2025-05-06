#pragma once

// bleh
struct ExportSettings_t
{
    int previewedSkinIndex;

    uint32_t exportNormalRecalcSetting;
    uint32_t exportTextureNameSetting;

    bool exportPathsFull;
    bool exportAssetDeps;
    bool exportRigSequences;
    bool exportModelSkin; 
    bool exportMaterialTextures;

    uint32_t exportPhysicsContentsFilter;
    bool exportPhysicsFilterExclusive;
    bool exportPhysicsFilterAND;
};

enum eNormalExportRecalc : uint32_t
{
    NML_RECALC_NONE,
    NML_RECALC_DX,
    NML_RECALC_OGL,

    NML_RECALC_COUNT,
};

static const char* s_NormalExportRecalcSetting[eNormalExportRecalc::NML_RECALC_COUNT] =
{
    "None",
    "DirectX",
    "OpenGL",
};

enum eTextureExportName : uint32_t
{
    TXTR_NAME_GUID,
    TXTR_NAME_REAL, // only use real values for name export (will fall back on guid if texture asset's name is nullptr)
    TXTR_NAME_TEXT, // use a text name for textures (will fall back on a generated semantic if texture asset's name is nullptr)
    TXTR_NAME_SMTC, // always use a generated name for textures (for exporting to things such as cast)

    TXTR_NAME_COUNT,
};

static const char* s_TextureExportNameSetting[eTextureExportName::TXTR_NAME_COUNT] =
{
    "GUID",
    "Real",
    "Text",
    "Semantic",
};

// preview settings
#define PREVIEW_CULL_DEFAULT    1000.0f
#define PREVIEW_CULL_MIN        256.0f // map max size
#define PREVIEW_CULL_MAX        16384.0f // quarter of max map size

#define PREVIEW_SPEED_DEFAULT   10.0f
#define PREVIEW_SPEED_MIN       1.0f
#define PREVIEW_SPEED_MAX       200.0f
#define PREVIEW_SPEED_MULT      5.0f

struct PreviewSettings_t
{
    float previewCullDistance; // currently can only be set on load, which is not ideal
    float previewMovementSpeed;
};