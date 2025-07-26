#include <pch.h>
#include <game/rtech/assets/lcd_screen_effect.h>
#include <game/rtech/cpakfile.h>

static bool ExportLcdScreenEffectAsset(CAsset* const asset, const int setting)
{
    CPakAsset* pakAsset = static_cast<CPakAsset*>(asset);
    UNUSED(setting);

    LcdScreenEffect_s rlcd;
    bool set = false;

    switch (pakAsset->version())
    {
    case 0:
    {
        LcdScreenEffect_v0_s* const hdr = reinterpret_cast<LcdScreenEffect_v0_s*>(pakAsset->header());

        rlcd.pixelScaleX1 = hdr->pixelScaleX1;
        rlcd.pixelScaleX2 = hdr->pixelScaleX2;
        rlcd.pixelScaleY = hdr->pixelScaleY;
        rlcd.brightness = hdr->brightness;
        rlcd.contrast = hdr->contrast;
        rlcd.waveScale = hdr->waveScale;
        rlcd.waveOffset = hdr->waveOffset;
        rlcd.waveSpeed = hdr->waveSpeed;
        rlcd.wavePeriod = hdr->wavePeriod;
        rlcd.bloomAdd = hdr->bloomAdd;
        rlcd.reserved = hdr->reserved;
        rlcd.pixelFlicker = hdr->pixelFlicker;

        set = true;
    }
    }

    if (!set)
    {
        assert(0); // Unsupported rlcd version.
        return false;
    }

    std::filesystem::path exportPath = std::filesystem::current_path().append(EXPORT_DIRECTORY_NAME);
    std::filesystem::path rlcdPath = asset->GetAssetName();

    exportPath.append(rlcdPath.parent_path().string());

    if (!CreateDirectories(exportPath))
    {
        assertm(false, "Failed to create asset type directory.");
        return false;
    }

    exportPath.append(rlcdPath.filename().string());
    exportPath.replace_extension(".json");

    StreamIO out;
    if (!out.open(exportPath.string(), eStreamIOMode::Write))
    {
        assertm(false, "Failed to open file for write.");
        return false;
    }

    std::string outBuf("{\n");

    outBuf.append(std::format("\t\"pixelScaleX1\": {:f},\n", rlcd.pixelScaleX1));
    outBuf.append(std::format("\t\"pixelScaleX2\": {:f},\n", rlcd.pixelScaleX2));
    outBuf.append(std::format("\t\"pixelScaleY\": {:f},\n", rlcd.pixelScaleY));
    outBuf.append(std::format("\t\"brightness\": {:f},\n", rlcd.brightness));
    outBuf.append(std::format("\t\"contrast\": {:f},\n", rlcd.contrast));
    outBuf.append(std::format("\t\"waveScale\": {:f},\n", rlcd.waveScale));
    outBuf.append(std::format("\t\"waveOffset\": {:f},\n", rlcd.waveOffset));
    outBuf.append(std::format("\t\"waveSpeed\": {:f},\n", rlcd.waveSpeed));
    outBuf.append(std::format("\t\"wavePeriod\": {:f},\n", rlcd.wavePeriod));
    outBuf.append(std::format("\t\"bloomAdd\": {:f},\n", rlcd.bloomAdd));
    outBuf.append(std::format("\t\"reserved\": {:d},\n", rlcd.reserved));
    outBuf.append(std::format("\t\"pixelFlicker\": {:f}\n}}", rlcd.pixelFlicker));

    out.write(outBuf.data(), outBuf.size());
    return true;
}

void InitLcdScreenEffectAssetType()
{
    AssetTypeBinding_t type =
    {
        .type = 'dclr',
        .headerAlignment = 4,
        .loadFunc = nullptr,
        .postLoadFunc = nullptr,
        .previewFunc = nullptr,
        .e = { ExportLcdScreenEffectAsset, 0, nullptr, 0ull },
    };

    REGISTER_TYPE(type);
}
