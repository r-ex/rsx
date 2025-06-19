#include <pch.h>
#include <game/rtech/assets/material_snapshot.h>

#include <thirdparty/imgui/imgui.h>

extern CDXParentHandler* g_dxHandler;
extern ExportSettings_t g_ExportSettings;


#undef max
void LoadMaterialSnapshotAsset(CAssetContainer* const container, CAsset* const asset)
{
    UNUSED(container);
    CPakAsset* const pakAsset = static_cast<CPakAsset*>(asset);

    MaterialSnapshotAsset* snapshotAsset = nullptr;

    switch (pakAsset->version())
    {
    case 1:
    {
        assertm(pakAsset->data()->headerStructSize == sizeof(MaterialSnapshotAssetHeader_v1_t), "incorrect header");

        const MaterialSnapshotAssetHeader_v1_t* const hdr = reinterpret_cast<const MaterialSnapshotAssetHeader_v1_t* const>(pakAsset->header());
        snapshotAsset = new MaterialSnapshotAsset(hdr);
        break;
    }
    default:
    {
        assertm(false, "unaccounted asset version, will cause major issues!");
        return;
    }
    }

    pakAsset->setExtraData(snapshotAsset);
}

void PostLoadMaterialSnapshotAsset(CAssetContainer* const container, CAsset* const asset)
{
    UNUSED(container);

    CPakAsset* const pakAsset = static_cast<CPakAsset*>(asset);

    MaterialSnapshotAsset* const snapshotAsset = reinterpret_cast<MaterialSnapshotAsset*>(pakAsset->extraData());
    assertm(snapshotAsset, "Extra data should be valid at this point.");

    CPakAsset* const shaderSetAsset = g_assetData.FindAssetByGUID<CPakAsset>(snapshotAsset->shaderSet);
    if (shaderSetAsset)
        snapshotAsset->shaderSetAsset = shaderSetAsset;

    // [rika]: has no name var
    pakAsset->SetAssetNameFromCache();
}

void* PreviewMaterialSnapshotAsset(CAsset* const asset, const bool firstFrameForAsset)
{
    UNUSED(firstFrameForAsset);

    CPakAsset* pakAsset = static_cast<CPakAsset*>(asset);

    const MaterialSnapshotAsset* const snapshotAsset = reinterpret_cast<const MaterialSnapshotAsset* const>(pakAsset->extraData());
    assertm(snapshotAsset, "Extra data should be valid at this point.");

    ImGui::Text("Shaderset: %s (0x%llx)", snapshotAsset->shaderSetAsset ? snapshotAsset->shaderSetAsset->GetAssetName().c_str() : "unloaded", snapshotAsset->shaderSet);

    if (ImGui::TreeNode("DX States"))
    {
        MatPreview_DXState(snapshotAsset->dxStates, 0, D3D11_SIMULTANEOUS_RENDER_TARGET_COUNT);

        ImGui::TreePop();
    }

    return nullptr;
}

void InitMaterialSnapshotAssetType()
{
    AssetTypeBinding_t type =
    {
        .type = 'pnsm',
        .headerAlignment = 16,
        .loadFunc = LoadMaterialSnapshotAsset,
        .postLoadFunc = PostLoadMaterialSnapshotAsset,
        .previewFunc = PreviewMaterialSnapshotAsset,
        .e = { nullptr, 0, nullptr, 0ull },
    };

    REGISTER_TYPE(type);
}