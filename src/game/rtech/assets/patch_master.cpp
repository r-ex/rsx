#include <pch.h>
#include <game/rtech/assets/patch_master.h>
#include <game/rtech/cpakfile.h>
#include <game/rtech/utils/utils.h>

void LoadPatchMasterAsset(CAssetContainer* pak, CAsset* asset)
{
    UNUSED(pak);

    CPakAsset* pakAsset = static_cast<CPakAsset*>(asset);

    PatchAssetHeader_t* ptch = reinterpret_cast<PatchAssetHeader_t*>(pakAsset->header());

    for (int i = 0; i < ptch->patchCount; ++i)
    {
        const char* pakName = ptch->pakNames[i];

        const uint8_t pakVersion = ptch->pakPatchVersions[i];

        g_assetData.m_patchMasterEntries.emplace(pakName, pakVersion);
    }
}

void PostLoadPatchMasterAsset(CAssetContainer* const container, CAsset* const asset)
{
    UNUSED(container);

    CPakAsset* pakAsset = static_cast<CPakAsset*>(asset);

    // [rika]: has no name var
    pakAsset->SetAssetNameFromCache();
}

void InitPatchMasterAssetType()
{
    AssetTypeBinding_t type =
    {
        .type = 'hctP',
        .headerAlignment = 8,
        .loadFunc = LoadPatchMasterAsset,
        .postLoadFunc = PostLoadPatchMasterAsset,
        .previewFunc = nullptr,
        .e = { nullptr, 0, nullptr, 0ull },
    };

    REGISTER_TYPE(type);
}