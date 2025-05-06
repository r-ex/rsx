#include <pch.h>
#include <game/rtech/assets/map.h>
#include <game/rtech/cpakfile.h>

extern ExportSettings_t g_ExportSettings;

static const char* const s_PathPrefixRTK = s_AssetTypePaths.find(AssetType_t::RTK)->second;

void LoadMapAsset(CAssetContainer* container, CAsset* asset)
{
    CPakFile* const pak = static_cast<CPakFile* const>(container);
    CPakAsset* const pakAsset = static_cast<CPakAsset* const>(asset);

    std::string assetName = "map/" + pak->getPakStem() + ".rmap";
    const uint64_t guidFromString = RTech::StringToGuid(assetName.c_str());

    if (guidFromString == asset->GetAssetGUID())
        pakAsset->SetAssetName(assetName);
}

void InitMapAssetType()
{
    AssetTypeBinding_t type =
    {
        .type = 'pamr',
        .headerAlignment = 8,
        .loadFunc = LoadMapAsset,
        .postLoadFunc = nullptr,
        .previewFunc = nullptr,
        .e = { nullptr, 0, nullptr, 0ull },
    };

    REGISTER_TYPE(type);
}