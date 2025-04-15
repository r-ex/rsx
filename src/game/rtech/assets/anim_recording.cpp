#include "pch.h"
#include "anim_recording.h"

#include <game/rtech/cpakfile.h>
#include <game/rtech/utils/utils.h>
#include <thirdparty/imgui/imgui.h>

extern ExportSettings_t g_ExportSettings;
static const char* const s_PathPrefixANIR = s_AssetTypePaths.find(AssetType_t::ANIR)->second;

static bool ExportAnimRecordingAsset(CAsset* const asset, const int setting)
{
    UNUSED(setting);

    CPakAsset* pakAsset = static_cast<CPakAsset*>(asset);
    const int assetVersion = pakAsset->version();

    if (assetVersion != 1)
        return false; // 1 is currently the only supported version.

    // Create exported path + asset path.
    std::filesystem::path exportPath = std::filesystem::current_path().append(EXPORT_DIRECTORY_NAME);
    const std::filesystem::path anirPath(pakAsset->GetAssetName());

    // truncate paths?
    if (g_ExportSettings.exportPathsFull)
        exportPath.append(anirPath.parent_path().string());
    else
        exportPath.append(s_PathPrefixANIR);

    if (!CreateDirectories(exportPath))
    {
        assertm(false, "Failed to create asset type directory.");
        return false;
    }

    exportPath.append(anirPath.filename().string());
    exportPath.replace_extension("anir");

    StreamIO anirOut(exportPath.string(), eStreamIOMode::Write);

    AnimRecordingAssetHeader_v0_t* const hdr = reinterpret_cast<AnimRecordingAssetHeader_v0_t*>(pakAsset->header());
    AnimRecordingFileHeader_s fileHdr;

    fileHdr.magic = ANIR_FILE_MAGIC;
    fileHdr.fileVersion = ANIR_FILE_VERSION;
    fileHdr.assetVersion = (unsigned short)pakAsset->version();

    fileHdr.startPos = hdr->startPos;
    fileHdr.startAngles = hdr->startAngles;

    fileHdr.stringBufSize = 0;

    fileHdr.numElements = 0;
    fileHdr.numSequences = 0;

    fileHdr.numRecordedFrames = hdr->numRecordedFrames;
    fileHdr.numRecordedOverlays = hdr->numRecordedOverlays;

    fileHdr.animRecordingId = hdr->animRecordingId;
    anirOut.write(fileHdr);

    // This information can only be retrieved by counting the number
    // of valid pose parameter names.
    int numElems = 0;
    int stringBufLen = 0;

    // Write out the pose parameters.
    for (int i = 0; i < ANIR_MAX_ELEMENTS; i++)
    {
        const char* const poseParamName = hdr->poseParamNames[i];

        if (poseParamName)
            numElems++;
        else
            break;

        const size_t strLen = strlen(poseParamName);
        anirOut.writeString(std::string(poseParamName, strLen));

        stringBufLen += (int)strLen + 1; // Include the null too.
    }

    // Write out the pose values.
    for (int i = 0; i < numElems; i++)
    {
        anirOut.write(hdr->poseParamValues[i]);
    }

    int numSeqs = 0;

    // Write out the animation sequence names.
    for (int i = 0; i < ANIR_MAX_SEQUENCES; i++)
    {
        const char* const animSequenceName = hdr->animSequences[i];

        if (animSequenceName)
            numSeqs++;
        else
            break;

        const size_t strLen = strlen(animSequenceName);
        anirOut.writeString(std::string(animSequenceName, strLen));

        stringBufLen += (int)strLen + 1; // Include the null too.
    }

    // Write out the recorded frames.
    for (int i = 0; i < hdr->numRecordedFrames; i++)
    {
        assert(hdr->recordedFrames);

        const AnimRecordingFrame_s* const frame = &hdr->recordedFrames[i];
        anirOut.write(*frame);
    }

    // Write out the recorded overlays.
    for (int i = 0; i < hdr->numRecordedOverlays; i++)
    {
        assert(hdr->recordedOverlays);

        const AnimRecordingOverlay_s* const overlay = &hdr->recordedOverlays[i];
        anirOut.write(*overlay);
    }

    // Update the data in the header if we ended up writing
    // elements and sequences.
    if (numElems > 0 || numSeqs > 0)
    {
        anirOut.seek(offsetof(AnimRecordingFileHeader_s, stringBufSize));

        anirOut.write(stringBufLen);
        anirOut.write(numElems);
        anirOut.write(numSeqs);
    }

    return true;
}

void InitAnimRecordingAssetType()
{
    AssetTypeBinding_t type =
    {
        .type = 'rina',
        .headerAlignment = 8,
        .loadFunc = nullptr,
        .postLoadFunc = nullptr,
        .previewFunc = nullptr,
        .e = { ExportAnimRecordingAsset, 0, nullptr, 0ull },
    };

    REGISTER_TYPE(type);
}
