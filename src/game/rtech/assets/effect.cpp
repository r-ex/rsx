#include "pch.h"
#include "effect.h"

#include <game/rtech/cpakfile.h>
#include <game/rtech/utils/utils.h>
#include <thirdparty/imgui/imgui.h>

extern ExportSettings_t g_ExportSettings;
static const char* const s_PathPrefixEFCT = s_AssetTypePaths.find(AssetType_t::EFCT)->second;

static bool ExportEffectAsset(CAsset* const asset, const int setting)
{
    CPakAsset* pakAsset = static_cast<CPakAsset*>(asset);
    UNUSED(setting);

    EffectHeader* const hdr = reinterpret_cast<EffectHeader*>(pakAsset->header());
    UNUSED(hdr);

    return true;
}

void InitEffectAssetType()
{
    AssetTypeBinding_t type =
    {
        .type = 'tcfe',
        .headerAlignment = 8,
        .loadFunc = nullptr,
        .postLoadFunc = nullptr,
        .previewFunc = nullptr,
        .e = { ExportEffectAsset, 0, nullptr, 0ull },
    };

    REGISTER_TYPE(type);
}
