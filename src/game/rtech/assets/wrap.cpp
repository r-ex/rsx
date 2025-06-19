#include <pch.h>
#include <game/rtech/assets/wrap.h>
#include <game/rtech/cpakfile.h>
#include <game/rtech/utils/utils.h>

void LoadWrapAsset(CAssetContainer* const pak, CAsset* const asset)
{
    UNUSED(pak);
    CPakAsset* pakAsset = static_cast<CPakAsset*>(asset);
    WrapAsset* wrapAsset = nullptr;

    switch (pakAsset->version())
    {
    case 7:
    {
        WrapAssetHeader_v7_t* v7 = reinterpret_cast<WrapAssetHeader_v7_t*>(pakAsset->header());
        wrapAsset = new WrapAsset(v7);
        break;
    }
    default:
    {
        assertm(false, "unaccounted asset version, will cause major issues!");
        return;
    }
    }

    const std::string name = std::string(wrapAsset->path + wrapAsset->skipFirstFolderPos);
    pakAsset->SetAssetName(name, true);
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

bool ExportWrapAsset(CAsset* const asset, const int setting)
{
    UNUSED(setting);
    CPakAsset* pakAsset = static_cast<CPakAsset*>(asset);

    const WrapAsset* const wrapAsset = reinterpret_cast<WrapAsset*>(pakAsset->extraData());
    assertm(wrapAsset, "Extra data should be valid at this point.");

    // Create exported path + asset path.
    std::filesystem::path exportPath = std::filesystem::current_path().append(std::format("{}\\{}", EXPORT_DIRECTORY_NAME, fourCCToString(asset->GetAssetType())));
    if (!CreateDirectories(exportPath))
    {
        assertm(false, "Failed to create asset type directory.");
        return false;
    }
    exportPath.append(asset->GetAssetName());
    if (!CreateDirectories(exportPath.parent_path()))
    {
        assertm(false, "Failed to create export directory.");
        return false;
    }

    StreamIO wrapOut;

    if (!wrapOut.open(exportPath.string(), eStreamIOMode::Write))
    {
        assertm(false, "Failed to open file for write.");
        return false;
    }

    const uint32_t wrapSize = wrapAsset->isCompressed ? wrapAsset->cmpSize : wrapAsset->dcmpSize;
    std::unique_ptr<char[]> wrapData = std::make_unique<char[]>(wrapSize);

    if (wrapAsset->isStreamed)
    {
        if (!GetStreamedDataForWrapAsset(pakAsset, wrapSize, wrapAsset->skipSize, wrapData))
        {
            assertm(false, "Failed to get streamed data for wrap asset.");
            return false;
        }
    }
    else
    {
        const int staticAsset = WRAP_FLAG_FILE_IS_COMPRESSED | WRAP_FLAG_FILE_IS_PERMANENT;
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

    wrapOut.write(wrapData.get(), wrapOutSize);
    wrapOut.close();

    return true;
}

void InitWrapAssetType()
{
    AssetTypeBinding_t type =
    {
        .type = 'parw',
        .headerAlignment = 8,
        .loadFunc = LoadWrapAsset,
        .postLoadFunc = nullptr,
        .previewFunc = nullptr,
        .e = { ExportWrapAsset, 0, nullptr, 0ull },
    };

    REGISTER_TYPE(type);
}