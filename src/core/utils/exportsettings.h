#pragma once

// bleh
struct ExportSettings_t
{
    uint32_t exportNormalRecalcSetting;

    uint32_t exportPhysicsContentsFilter;
    bool exportPhysicsFilterExclusive;
    bool exportPhysicsFilterAND;

    bool exportPathsFull;
    bool exportAssetDeps;
    bool exportSeqsWithRig;
    bool exportTxtrWithMat;
    bool useSemanticTextureNames;
};

enum eNormalExportRecalc : uint32_t
{
    NML_RECALC_NONE,
    NML_RECALC_DX,
    NML_RECALC_OGL,
};

static const char* s_NormalExportRecalcSetting[] =
{
    "None",
    "DirectX",
    "OpenGL",
};