#pragma once

struct UIAssetArgCluster_t
{
    uint16_t argIndex;
    uint16_t argCount;

    uint8_t byte_4;
    uint8_t byte_5;

    char gap_6[4];

    uint16_t word_A;

    char gap_C[6];
};

enum UIAssetArgType_t : uint8_t
{
    UI_ARG_TYPE_NONE = 0,
    UI_ARG_TYPE_STRING = 0x1,
    UI_ARG_TYPE_ASSET = 0x2,
    UI_ARG_TYPE_BOOL = 0x3,
    UI_ARG_TYPE_INT = 0x4,
    UI_ARG_TYPE_FLOAT = 0x5,
    UI_ARG_TYPE_FLOAT2 = 0x6,
    UI_ARG_TYPE_FLOAT3 = 0x7,
    UI_ARG_TYPE_COLOR_ALPHA = 0x8,
    UI_ARG_TYPE_GAMETIME = 0x9,
    UI_ARG_TYPE_WALLTIME = 0xA,
    UI_ARG_TYPE_UIHANDLE = 0xB,
    UI_ARG_TYPE_IMAGE = 0xC,
    UI_ARG_TYPE_FONT_FACE = 0xD,
    UI_ARG_TYPE_FONT_HASH = 0xE,
    UI_ARG_TYPE_ARRAY = 0xF,
};

static const char* const s_UIArgTypeNames[] = {
    "NONE",
    "STRING",
    "ASSET",
    "BOOL",
    "INT",
    "FLOAT",
    "FLOAT2",
    "FLOAT3",
    "COLOR ALPHA",
    "GAMETIME",
    "WALLTIME",
    "UIHANDLE",
    "IMAGE",
    "FONT FACE",
    "FONT HASH",
    "ARRAY",
};

union UIAssetArgValue_t
{
    char* rawptr;
    char** string;
    bool* boolean;
    int* integer;
    float* fpn;
    int64_t* integer64;
};

struct UIAssetArg_t
{
    UIAssetArgType_t type;

    uint8_t unk_1;

    uint16_t dataOffset;
    uint16_t nameOffset;

    uint16_t shortHash;
};

struct UIAssetStyleDescriptor_t
{
    uint16_t type;
    uint16_t color_red;
    uint16_t color_green;
    uint16_t color_blue;
    uint16_t color_alpha;

    uint16_t unk_A[10];

    uint16_t fontIndex_1E;

    uint16_t unk_20[4];

    uint16_t textSize; // word_28
    uint16_t textStretch; //  word_2A

    uint16_t unk_2C[4];
};

struct UIAssetMapping_t
{
    uint32_t dataCount;

    uint16_t unk_4;
    uint16_t unk_6;

    float* data;
};

struct UIAssetHeader_t
{
    const char* name;

    void* defaultValues;
    uint8_t* transformData;

    float elementWidth;
    float elementHeight;
    float elementWidthRatio;
    float elementHeightRatio;

    const char* argNames;
    UIAssetArgCluster_t* argClusters;
    UIAssetArg_t* args;
    int16_t argCount;

    int16_t unk_10; // count

    uint16_t ruiDataStructSize;
    uint16_t defaultValuesSize;
    uint16_t styleDescriptorCount;

    uint16_t unk_A4;

    uint16_t renderJobCount;
    uint16_t argClusterCount;

    UIAssetStyleDescriptor_t* styleDescriptors;
    uint8_t* renderJobs;
    UIAssetMapping_t* mappingData;
};

class UIAsset
{
public:
    UIAsset(UIAssetHeader_t* hdr) : name(hdr->name), elementWidth(hdr->elementWidth), elementHeight(hdr->elementHeight), elementWidthRatio(hdr->elementWidthRatio), elementHeightRatio(hdr->elementHeightRatio),
        argNames(hdr->argNames),  args(hdr->args), argClusters(hdr->argClusters), argDefaultValues(hdr->defaultValues), argCount(hdr->argCount), argClusterCount(hdr->argClusterCount), argDefaultValueSize(hdr->defaultValuesSize) {};

    const char* name;

    float elementWidth;
    float elementHeight;
    float elementWidthRatio;
    float elementHeightRatio;

    const char* argNames;
    UIAssetArg_t* args;
    UIAssetArgCluster_t* argClusters;
    void* argDefaultValues;

    int16_t argCount;
    uint16_t argClusterCount;
    uint16_t argDefaultValueSize;
};