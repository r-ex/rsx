#include "pch.h"
#include "texture_anim.h"

#include <game/rtech/cpakfile.h>
#include <game/rtech/utils/utils.h>
#include <thirdparty/imgui/imgui.h>

extern ExportSettings_t g_ExportSettings;
static const char* const s_PathPrefixTXAN = s_AssetTypePaths.find(AssetType_t::TXAN)->second;

// [amos]: this information isn't stored in the rpak as the engine just animates
//         from the start slot to the end slot and wraps it around to the highest
//         of the 2, so we have to run over each layer and get the highest index
//         to determine the size of the texture slot array.
static int GetSlotCount(TextureAnimAssetHeader_v1_t* const hdr)
{
    int highestIndex = 0;

    for (int i = 0; i < hdr->layerCount; i++)
    {
        const TextureAnimLayer_t& layer = hdr->layers[i];
        const unsigned short layerSlotIndex = (std::max)(layer.startSlot, layer.endSlot);

        if (layerSlotIndex > highestIndex)
            highestIndex = layerSlotIndex;
    }

    return highestIndex;
}

bool ExportTextureAnimationAsset(CAsset* const asset, const int setting)
{
    UNUSED(asset);
    UNUSED(setting);

    CPakAsset* pakAsset = static_cast<CPakAsset*>(asset);

    TextureAnimAssetHeader_v1_t* const hdr = reinterpret_cast<TextureAnimAssetHeader_v1_t*>(pakAsset->header());

    //// Create exported path + asset path.
    std::filesystem::path exportPath = std::filesystem::current_path().append(EXPORT_DIRECTORY_NAME);
    const std::filesystem::path txanPath(asset->GetAssetName());

    // truncate paths?
    if (g_ExportSettings.exportPathsFull)
        exportPath.append(txanPath.parent_path().string());
    else
        exportPath.append(s_PathPrefixTXAN);

    if (!CreateDirectories(exportPath))
    {
        assertm(false, "Failed to create asset type directory.");
        return false;
    }

    exportPath.append(txanPath.filename().string());
    exportPath.replace_extension("txan");

    StreamIO txanOut(exportPath.string(), eStreamIOMode::Write);

    TextureAnimFileHeader_t fileHdr;

    fileHdr.magic = TXAN_FILE_MAGIC;
    fileHdr.fileVersion = TXAN_FILE_VERSION;
    fileHdr.assetVersion = static_cast<unsigned short>(pakAsset->data()->version);
    fileHdr.layerCount = hdr->layerCount;
    fileHdr.slotCount = GetSlotCount(hdr);

    txanOut.write(fileHdr);
    txanOut.write((const char*)hdr->layers, fileHdr.layerCount * sizeof(TextureAnimLayer_t));
    txanOut.write((const char*)hdr->slots, fileHdr.slotCount);

    txanOut.close();

    return true;
}

void InitTextureAnimationAssetType()
{
    AssetTypeBinding_t type =
    {
        .type = 'naxt',
        .headerAlignment = 8,
        .loadFunc = nullptr, // so far txan seems to have never changed, and is still version 1 in retail (R5pc_r5-230_J34_CL8040141_2024_11_27_12_52 as of time writing this).
        .postLoadFunc = nullptr,
        .previewFunc = nullptr,
        .e = { ExportTextureAnimationAsset, 0, nullptr, 0ull },
    };

    REGISTER_TYPE(type);
}