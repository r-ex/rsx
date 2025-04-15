#include <pch.h>
#include <game/rtech/assets/ui_rtk.h>
#include <game/rtech/cpakfile.h>

#include <game/rtech/utils/utils.h>
#include <thirdparty/imgui/imgui.h>

extern ExportSettings_t g_ExportSettings;

static const char* const s_PathPrefixRTK = s_AssetTypePaths.find(AssetType_t::RTK)->second;

void LoadRTKAsset(CAssetContainer* pak, CAsset* asset)
{
    UNUSED(pak);
    RTKAsset* rtkAsset = nullptr;

    CPakAsset* pakAsset = static_cast<CPakAsset*>(asset);
    switch (pakAsset->data()->headerStructSize)
    {
    case 0x20:
    {
        RTKAssetHeader_v2_t* hdr = reinterpret_cast<RTKAssetHeader_v2_t*>(pakAsset->header());
        rtkAsset = new RTKAsset(hdr);

        break;
    }
    case 0x28:
    {
        RTKAssetHeader_v2_1_t* hdr = reinterpret_cast<RTKAssetHeader_v2_1_t*>(pakAsset->header());
        rtkAsset = new RTKAsset(hdr);

        break;
    }
    case 0x30:
    {
        RTKAssetHeader_v2_2_t* hdr = reinterpret_cast<RTKAssetHeader_v2_2_t*>(pakAsset->header());
        rtkAsset = new RTKAsset(hdr);

        break;
    }
    default:
        static bool hasWarned = false;

        if (!hasWarned)
        {
            Log("Failed to load RTK asset %llX with unknown header size. Header Size: %u, Version: %u\n",
                pakAsset->guid(), pakAsset->data()->headerStructSize, pakAsset->version());

            hasWarned = true;
        }
        return;
    }

    std::string assetName = "ui_rtk/" + rtkAsset->getName() + ".rpak";
    pakAsset->SetAssetName(assetName);
    pakAsset->setExtraData(rtkAsset);
}

void* PreviewRTKAsset(CAsset* const asset, const bool firstFrameForAsset)
{
    UNUSED(firstFrameForAsset);

    CPakAsset* pakAsset = static_cast<CPakAsset*>(asset);
    const RTKAsset* const hdr = reinterpret_cast<RTKAsset*>(pakAsset->extraData());
    ImGui::Text("Name: %s", hdr->elementName);
    ImGui::Text("Element Size: %d", hdr->elementDataSize);

    if (ImGui::BeginChild("RTK Preview", ImVec2(-1, -1), true, ImGuiWindowFlags_HorizontalScrollbar))
    {
        ImGui::TextUnformatted(hdr->elementData);
    }
    ImGui::EndChild();

    return nullptr;
}

bool ExportRTKAsset(CAsset* const asset, const int setting)
{
    UNUSED(setting);      

    CPakAsset* pakAsset = static_cast<CPakAsset*>(asset);
    const RTKAsset* const hdr = reinterpret_cast<RTKAsset*>(pakAsset->extraData());

    // Create exported path + asset path.
    std::filesystem::path exportPath = std::filesystem::current_path().append(EXPORT_DIRECTORY_NAME);
    const std::filesystem::path rtkPath(pakAsset->GetAssetName());

    // truncate paths?
    if (g_ExportSettings.exportPathsFull)
        exportPath.append(rtkPath.parent_path().string());
    else
        exportPath.append(s_PathPrefixRTK);

    if (!CreateDirectories(exportPath))
    {
        assertm(false, "Failed to create asset type directory.");
        return false;
    }

    exportPath.append(rtkPath.filename().string());
    exportPath.replace_extension("rtk");

    StreamIO rtkOut(exportPath.string(), eStreamIOMode::Write);
    rtkOut.write(hdr->elementData, hdr->elementDataSize);
    rtkOut.close();

    return true;
}

void InitRTKAssetType()
{
    AssetTypeBinding_t type =
    {
        .type = '\0ktr',
        .headerAlignment = 8,
        .loadFunc = LoadRTKAsset,
        .postLoadFunc = nullptr,
        .previewFunc = PreviewRTKAsset,
        .e = { ExportRTKAsset, 0, nullptr, 0ull },
    };

    REGISTER_TYPE(type);
}