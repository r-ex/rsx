#include <pch.h>
#include <game/rtech/assets/wrap.h>
#include <game/rtech/cpakfile.h>
#include <game/rtech/utils/utils.h>
#include <game/bsp/bsp.h>
#include <thirdparty/imgui/imgui.h>
#include <thirdparty/imgui/misc/imgui_memory_editor.h>

void LoadWrapAsset(CAssetContainer* const pak, CAsset* const asset)
{
    UNUSED(pak);
    CPakAsset* pakAsset = static_cast<CPakAsset*>(asset);
    WrapAsset* wrapAsset = nullptr;

    switch (pakAsset->version())
    {
    case 1:
    {
        WrapAssetHeader_v1_t* hdr = reinterpret_cast<WrapAssetHeader_v1_t*>(pakAsset->header());
        wrapAsset = new WrapAsset(hdr);
        break;
    }
    case 7:
    {
        WrapAssetHeader_v7_t* hdr = reinterpret_cast<WrapAssetHeader_v7_t*>(pakAsset->header());
        wrapAsset = new WrapAsset(hdr);
        break;
    }
    default:
    {
        assertm(false, "unaccounted asset version, will cause major issues!");
        return;
    }
    }

    if (wrapAsset->path)
    {
        const std::string name = std::string(wrapAsset->path + wrapAsset->skipFirstFolderPos);
        pakAsset->SetAssetName(name, true);
    }

    pakAsset->setExtraData(wrapAsset);
}


static bool GetStreamedDataForWrapAsset(CPakAsset* const asset, const uint32_t size, const int32_t skipSize, std::unique_ptr<char[]>& wrapData)
{
    AssetPtr_t streamEntry = asset->getStarPakStreamEntry(false);
    if (!IS_ASSET_PTR_INVALID(streamEntry))
    {
        wrapData = asset->getStarPakData(streamEntry.offset + skipSize, size, false);
        return true;
    }

    streamEntry = asset->getStarPakStreamEntry(true);
    if (!IS_ASSET_PTR_INVALID(streamEntry))
    {
        wrapData = asset->getStarPakData(streamEntry.offset + skipSize, size, true);
        return true;
    }

    assertm(false, "GetStreamedDataForWrapAsset called but no streamed data?");
    return false;
}

std::unique_ptr<char[]> GetWrapAssetData(CAsset* const asset, uint64_t* outSize)
{
    CPakAsset* pakAsset = static_cast<CPakAsset*>(asset);

    const WrapAsset* const wrapAsset = reinterpret_cast<WrapAsset*>(pakAsset->extraData());

    const uint32_t wrapSize = wrapAsset->isCompressed ? wrapAsset->cmpSize : wrapAsset->dcmpSize;
    std::unique_ptr<char[]> wrapData = std::make_unique<char[]>(wrapSize);

    if (wrapAsset->isStreamed)
    {
        if (!GetStreamedDataForWrapAsset(pakAsset, wrapSize, wrapAsset->skipSize, wrapData))
        {
            assertm(false, "Failed to get streamed data for wrap asset.");
            return nullptr;
        }
    }
    else
    {
        constexpr uint16_t staticAsset = WRAP_FLAG_FILE_IS_COMPRESSED | WRAP_FLAG_FILE_IS_PERMANENT;
        const char* buf = (const char*)wrapAsset->data;

        // [amos]: if this condition is met, some internal header needs to be skipped.
        // only appears to happen on small files that appear to be marked for compress
        // but failed during build, e.g. mp_rr_arena_phase_runner.bsp.0005.bsp_lump in
        // release "build R5pc_r5-180_J25_CL4941853_2023_07_27_16_31". permanent assets
        // only
        if (!wrapAsset->isCompressed && (wrapAsset->flags & staticAsset) == staticAsset)
            buf += 2;

        memcpy_s(wrapData.get(), wrapSize, buf, wrapSize);
    }

    uint64_t wrapOutSize = wrapAsset->dcmpSize;
    if (wrapAsset->isCompressed)
    {
        wrapData = RTech::DecompressStreamedBuffer(std::move(wrapData), wrapOutSize, eCompressionType::OODLE);
    }

    if (outSize)
        *outSize = wrapOutSize;

    return wrapData;
}

// eugh
std::string Wrap_GetPathWithSwappedExtensions(const WrapAsset* const wrapAsset, const std::string& origAssetName)
{
    // size of the asset path excluding the first folder and the VM extension (ui, client, server)
    const uint64_t realPathSize = static_cast<uint64_t>(wrapAsset->pathSize) - wrapAsset->skipFirstFolderPos;

    if (origAssetName.length() == realPathSize)
        return origAssetName;

    const std::string vmExtension = origAssetName.substr(realPathSize);

    const std::string assetPathWithoutVMExt = origAssetName.substr(0, realPathSize);

    std::filesystem::path fsPath(assetPathWithoutVMExt);

    const std::string realExtension = fsPath.extension().string();

    fsPath.replace_extension(vmExtension);
    fsPath += realExtension;

    return fsPath.string();
}

// if there is a VM extension at the end of the file name, cut it off
// 
// usually, wrapAsset->pathSize will equal everything up until ".ui" or ".client"
// but the asset name already has the first folder cut off by this point, so we have to account for that in this check
inline void Wrap_StripVMExtensionFromPath(WrapAsset* const wrapAsset, std::string& assetPath)
{
    if (assetPath.length() != (wrapAsset->pathSize - wrapAsset->skipFirstFolderPos))
        assetPath = assetPath.substr(0, wrapAsset->pathSize - wrapAsset->skipFirstFolderPos);
}

void PostLoadWrapAsset(CAssetContainer* const pak, CAsset* const asset)
{
    UNUSED(pak);
    CPakAsset* pakAsset = static_cast<CPakAsset*>(asset);

    WrapAsset* const wrapAsset = reinterpret_cast<WrapAsset*>(pakAsset->extraData());

    // Default wrap asset type is "unknown"; basically if the file's extension isn't registered against a file type, it's previewed as binary data
    wrapAsset->type = VPKFileType_e::UNKNOWN;

    std::string assetName = asset->GetAssetName();
    
    Wrap_StripVMExtensionFromPath(wrapAsset, assetName);

    std::filesystem::path assetPath = std::filesystem::path(assetName);
    const std::string extension = assetPath.extension().string();

    // wrapped filesystem is the successor of vpk so we use VPK types, not the other way around
    if (auto it = s_vpkFileTypes.find(extension); it != s_vpkFileTypes.end())
        wrapAsset->type = it->second;

    switch (wrapAsset->type)
    {
    case VPKFileType_e::BSP:
    {
#if (HAS_BSP_SUPPORT)
        std::unique_ptr<char[]> wrapData = GetWrapAssetData(asset, nullptr);

        CBSPData* bspData = new CBSPData(assetPath.stem().string());
        bspData->PopulateFromPakAsset(pakAsset, wrapData.get());

        wrapAsset->parsedData = bspData;
#endif
        break;
    }
    }
}

bool ExportWrapAsset(CAsset* const asset, const int setting)
{
    UNUSED(setting);
    CPakAsset* pakAsset = static_cast<CPakAsset*>(asset);

    const WrapAsset* const wrapAsset = reinterpret_cast<WrapAsset*>(pakAsset->extraData());

    // Create exported path + asset path.
    std::filesystem::path exportPath = g_rsxSettings.GetExportDirectory() / fourCCToString(asset->GetAssetType());
    if (!CreateDirectories(exportPath))
    {
        assertm(false, "Failed to create asset type directory.");
        return false;
    }

    if (g_rsxSettings.useOrigScriptExportExtensions)
        exportPath.append(asset->GetAssetName());
    else
        exportPath.append(Wrap_GetPathWithSwappedExtensions(wrapAsset, asset->GetAssetName()));

    if (!CreateDirectories(exportPath.parent_path()))
    {
        assertm(false, "Failed to create export directory.");
        return false;
    }

    switch (wrapAsset->type)
    {
    case VPKFileType_e::UNKNOWN:
    case VPKFileType_e::TEXT:
    {
        StreamIO wrapOut;

        if (!wrapOut.open(exportPath.string(), eStreamIOMode::Write))
        {
            assertm(false, "Failed to open file for write.");
            return false;
        }

        uint64_t wrapOutSize = 0;
        std::unique_ptr<char[]> wrapData = GetWrapAssetData(asset, &wrapOutSize);

        if (!wrapData || wrapOutSize == 0)
            return false;

        // If the file has been detected as a text file and the last byte of the data is a null terminator, adjust the file size so we don't write it
        // In theory, the last byte will ALWAYS be a null byte, but it doesn't hurt to double check
        if (wrapAsset->type == VPKFileType_e::TEXT && wrapData[wrapOutSize - 1] == '\0')
            wrapOutSize--;

        wrapOut.write(wrapData.get(), wrapOutSize);
        wrapOut.close();
        break;
    }
#if (HAS_BSP_SUPPORT)
    case WrapAssetType_e::BSP:
    {
        StreamIO wrapOut;

        if (!wrapOut.open(exportPath.string(), eStreamIOMode::Write))
        {
            assertm(false, "Failed to open file for write.");
            return false;
        }

        CBSPData* bspData = reinterpret_cast<CBSPData*>(wrapAsset->parsedData);

        bspData->Export(wrapOut.W());

        wrapOut.close();

        break;
    }
#endif
    default:
    {
        unreachable();
        break;
    }
    }

    return true;
}

void* Wrap_PreviewTextOrBinary(CAsset* const asset, WrapAsset* const wrapAsset, const bool firstFrameForAsset)
{
    // this leaks memory
    if (firstFrameForAsset && !wrapAsset->rawData)
    {
        uint64_t wrapOutSize = 0;
        std::unique_ptr<char[]> wrapData = GetWrapAssetData(asset, &wrapOutSize);

        if (!wrapData || wrapOutSize == 0)
        {
            assert(0);
            return nullptr;
        }

        wrapAsset->rawData = std::move(wrapData);
    }

    if (ImGui::BeginChild("##Wrap Preview", ImVec2(-1, -1), true, ImGuiWindowFlags_HorizontalScrollbar))
    {
        if(wrapAsset->type == VPKFileType_e::TEXT)
            ImGui::TextUnformatted(reinterpret_cast<const char*>(wrapAsset->rawData.get()));
        else
        {
            static MemoryEditor wrapPreview;

            wrapPreview.ReadOnly = true;
            wrapPreview.OptShowDataPreview = true;
            //wrapPreview.UserData = &previewData;
            //wrapPreview.BgColorFn = UIArgData_BGColorCallback;

            wrapPreview.DrawContents(wrapAsset->rawData.get(), wrapAsset->dcmpSize);
        }
    }
    ImGui::EndChild();

    // there's nothing to put in the main scene window
    return nullptr;
}

void* PreviewWrapAsset(CAsset* const asset, const bool firstFrameForAsset)
{
    CPakAsset* const pakAsset = static_cast<CPakAsset*>(asset);

    WrapAsset* const wrapAsset = reinterpret_cast<WrapAsset*>(pakAsset->extraData());

    if (!wrapAsset) UNLIKELY
        return nullptr;

    switch (wrapAsset->type)
    {
#if !(HAS_BSP_SUPPORT) // if no bsp-specific support, just show it as a binary file
    case VPKFileType_e::BSP:
#endif
    case VPKFileType_e::TEXT:
    case VPKFileType_e::UNKNOWN:
        return Wrap_PreviewTextOrBinary(asset, wrapAsset, firstFrameForAsset);
#if (HAS_BSP_SUPPORT)
    case WrapAssetType_e::BSP:
        return reinterpret_cast<CBSPData*>(wrapAsset->parsedData)->ConstructPreviewData();
#endif
    default:
    {
        ImGui::Text("Preview for WRAP assets is currently not supported for BSP files");
        return nullptr;
    }
    }

    unreachable();
}

void InitWrapAssetType()
{
    AssetTypeBinding_t type =
    {
        .name = "Wrapped File",
        .type = 'parw',
        .headerAlignment = 8,
        .loadFunc = LoadWrapAsset,
        .postLoadFunc = PostLoadWrapAsset,
        .previewFunc = PreviewWrapAsset,
        .e = { ExportWrapAsset, 0, nullptr, 0ull },
    };

    REGISTER_TYPE(type);
}