#include <pch.h>
#include <game/rtech/assets/rson.h>
#include <game/rtech/assets/animrig.h>
#include <game/rtech/assets/animseq.h>

#include <core/mdl/stringtable.h>
#include <core/mdl/rmax.h>
#include <core/mdl/cast.h>

#include <thirdparty/imgui/imgui.h>
#include <thirdparty/imgui/misc/imgui_utility.h>

extern RSXSettings_t g_rsxSettings;

void LoadAnimRigAsset(CAssetContainer* const container, CAsset* const asset)
{
    CPakAsset* pakAsset = static_cast<CPakAsset*>(asset);
    AnimRigAsset* arigAsset = nullptr;

    switch (pakAsset->version())
    {
    case 4:
    {
        AnimRigAssetHeader_v4_t* const hdr = reinterpret_cast<AnimRigAssetHeader_v4_t*>(pakAsset->header());
        arigAsset = new AnimRigAsset(hdr, GetModelPakVersion(reinterpret_cast<const int* const>(hdr->data)));
        break;
    }
    case 5:
    case 6:
    {
        AnimRigAssetHeader_v5_t* const hdr = reinterpret_cast<AnimRigAssetHeader_v5_t*>(pakAsset->header());
        arigAsset = new AnimRigAsset(hdr, GetModelPakVersion(reinterpret_cast<const int* const>(hdr->data)));
        break;
    }
    case 7:
    {
        CPakFile* const pak = static_cast<CPakFile* const>(container);
        eMDLVersion ver = pak->header()->createdTime >= s_AnimSeqTimeStamp_V12_1 ? eMDLVersion::VERSION_19_1 : eMDLVersion::VERSION_19;

        AnimRigAssetHeader_v5_t* const hdr = reinterpret_cast<AnimRigAssetHeader_v5_t*>(pakAsset->header());

        // [rika]: model version unchanged reasonably, but animdata has changed so.
        if (pak->header()->createdTime > s_AnimSeqTimeStamp_V13)
            ver = eMDLVersion::VERSION_19_3;

        // i HAAAAATE this tool man
        if (pak->header()->createdTime > s_AnimRigTimeStamp_V7_V19_2)
            ver = eMDLVersion::VERSION_19_2;

        arigAsset = new AnimRigAsset(hdr, ver);
        break;
    }
    default:
        return;
    }

    switch (arigAsset->studioVersion)
    {
    case eMDLVersion::VERSION_8:
    case eMDLVersion::VERSION_9:
    case eMDLVersion::VERSION_10:
    case eMDLVersion::VERSION_11:
    case eMDLVersion::VERSION_12:
    {
        ParseModelBoneData_v8(arigAsset->GetParsedData());
        ParseModelAttachmentData_v8(arigAsset->GetParsedData());
        ParseModelHitboxData_v8(arigAsset->GetParsedData());
        ParseModelAnimTypes_V8(arigAsset->GetParsedData());

        break;
    }
    case eMDLVersion::VERSION_12_1:
    case eMDLVersion::VERSION_12_2:
    case eMDLVersion::VERSION_12_3:
    case eMDLVersion::VERSION_12_4:
    case eMDLVersion::VERSION_12_5:
    case eMDLVersion::VERSION_13:
    case eMDLVersion::VERSION_13_1:
    case eMDLVersion::VERSION_14:
    case eMDLVersion::VERSION_14_1:
    case eMDLVersion::VERSION_15:
    {
        ParseModelBoneData_v12_1(arigAsset->GetParsedData());
        ParseModelAttachmentData_v8(arigAsset->GetParsedData());
        ParseModelHitboxData_v8(arigAsset->GetParsedData());
        ParseModelAnimTypes_V8(arigAsset->GetParsedData());

        break;
    }
    case eMDLVersion::VERSION_16:
    case eMDLVersion::VERSION_17:
    {
        ParseModelBoneData_v16(arigAsset->GetParsedData());
        ParseModelAttachmentData_v16(arigAsset->GetParsedData());
        ParseModelHitboxData_v16(arigAsset->GetParsedData());
        ParseModelAnimTypes_V16(arigAsset->GetParsedData());

        break;
    }
    case eMDLVersion::VERSION_18:
    {
        ParseModelBoneData_v16(arigAsset->GetParsedData());
        ParseModelAttachmentData_v16(arigAsset->GetParsedData());
        ParseModelHitboxData_v16(arigAsset->GetParsedData());
        ParseModelAnimTypes_V16(arigAsset->GetParsedData());

        break;
    }
    case eMDLVersion::VERSION_19:
    {
        ParseModelBoneData_v19(arigAsset->GetParsedData());
        ParseModelAttachmentData_v16(arigAsset->GetParsedData());
        ParseModelHitboxData_v16(arigAsset->GetParsedData());
        ParseModelAnimTypes_V16(arigAsset->GetParsedData());

        break;
    }
    case eMDLVersion::VERSION_19_1:
    case eMDLVersion::VERSION_19_2:
    case eMDLVersion::VERSION_19_3:
    {
        ParseModelBoneData_v19(arigAsset->GetParsedData());
        ParseModelAttachmentData_v16(arigAsset->GetParsedData());
        ParseModelHitboxData_v16(arigAsset->GetParsedData());
        ParseModelAnimTypes_V16(arigAsset->GetParsedData());

        break;
    }
    case eMDLVersion::VERSION_UNK:
    default:
    {
        assertm(false, "Unknown AnimRig asset version");
        break;
    }
    }

    assertm(arigAsset->name, "Rig had no name.");
    pakAsset->SetAssetName(arigAsset->name, true);
    pakAsset->setExtraData(arigAsset);
}

void PostLoadAnimRigAsset(CAssetContainer* const pak, CAsset* const asset)
{
    UNUSED(pak);

    CPakAsset* pakAsset = static_cast<CPakAsset*>(asset);

    AnimRigAsset* const arigAsset = reinterpret_cast<AnimRigAsset*>(pakAsset->extraData());

    if (!arigAsset)
        return;

    // parse sequences for children
    if (arigAsset->numAnimSeqs)
    {
        ModelParsedData_t* const parsedData = arigAsset->GetParsedData();

        parsedData->numExternalSequences = arigAsset->numAnimSeqs;
        parsedData->externalSequences = arigAsset->animSeqs;

        const uint64_t* guids = reinterpret_cast<const uint64_t*>(arigAsset->animSeqs);

        for (uint16_t seqIdx = 0; seqIdx < arigAsset->numAnimSeqs; seqIdx++)
        {
            const uint64_t guid = guids[seqIdx];

            CPakAsset* const animSeqAsset = g_assetData.FindAssetByGUID<CPakAsset>(guid);

            if (nullptr == animSeqAsset)
                continue;

            if (!animSeqAsset->hasExtraData())
                continue;

            AnimSeqAsset* const animSeq = reinterpret_cast<AnimSeqAsset* const>(animSeqAsset->extraData());

            if (nullptr == animSeq)
            {
                continue;
            }

            animSeq->parentRig = !animSeq->parentRig ? arigAsset : animSeq->parentRig;
        }
    }

    // [rika]: this should never get hit
    if (arigAsset->GetParsedData()->NumLocalSeq() == 0)
        return;

    assertm(false, "arig had internal sequences");

    switch (arigAsset->studioVersion)
    {
    case eMDLVersion::VERSION_8:
    case eMDLVersion::VERSION_9:
    case eMDLVersion::VERSION_10:
    case eMDLVersion::VERSION_11:
    case eMDLVersion::VERSION_12:
    {
        ParseModelSequenceData_NoStall(arigAsset->GetParsedData(), reinterpret_cast<char* const>(arigAsset->data));

        break;
    }
    case eMDLVersion::VERSION_12_1:
    case eMDLVersion::VERSION_12_2:
    case eMDLVersion::VERSION_12_3:
    case eMDLVersion::VERSION_12_4:
    case eMDLVersion::VERSION_12_5:
    case eMDLVersion::VERSION_13:
    case eMDLVersion::VERSION_13_1:
    case eMDLVersion::VERSION_14:
    case eMDLVersion::VERSION_14_1:
    case eMDLVersion::VERSION_15:
    {
        ParseModelSequenceData_Stall_V8(arigAsset->GetParsedData(), reinterpret_cast<char* const>(arigAsset->data));

        break;
    }
    case eMDLVersion::VERSION_16:
    case eMDLVersion::VERSION_17:
    {
        ParseModelSequenceData_Stall_V16(arigAsset->GetParsedData(), reinterpret_cast<char* const>(arigAsset->data));

        break;
    }
    case eMDLVersion::VERSION_18:
    case eMDLVersion::VERSION_19:
    {
        ParseModelSequenceData_Stall_V18(arigAsset->GetParsedData(), reinterpret_cast<char* const>(arigAsset->data));

        break;
    }
    case eMDLVersion::VERSION_19_1:
    case eMDLVersion::VERSION_19_2:
    {
        ParseModelSequenceData_Stall_V19_1(arigAsset->GetParsedData(), reinterpret_cast<char* const>(arigAsset->data), ANIM_BONEFLAG_BITS_4);

        break;
    }
    case eMDLVersion::VERSION_19_3:
    {
        ParseModelSequenceData_Stall_V19_1(arigAsset->GetParsedData(), reinterpret_cast<char* const>(arigAsset->data), ANIM_BONEFLAG_BITS_6);

        break;
    }
    case eMDLVersion::VERSION_UNK:
    default:
    {
        assertm(false, "unaccounted asset version, will cause major issues!");
        break;
    }
    }
}

static bool ExportRawAnimRigAsset(CPakAsset* const asset, const AnimRigAsset* const animRigAsset, std::filesystem::path& exportPath)
{
    UNUSED(asset);

    StreamIO rigOut(exportPath.string(), eStreamIOMode::Write);
    rigOut.write(reinterpret_cast<const char*>(animRigAsset->data), animRigAsset->pStudioHdr()->length);
    rigOut.close();

    // make a manifest of this assets dependencies
    exportPath.replace_extension(".rson");

    StreamIO depOut(exportPath.string(), eStreamIOMode::Write);
    WriteRSONDependencyArray(*depOut.W(), "seqs", animRigAsset->animSeqs, animRigAsset->numAnimSeqs);
    depOut.close();

    return true;
}

static const char* const s_PathPrefixARIG = s_AssetTypePaths.find(AssetType_t::ARIG)->second;
bool ExportAnimRigAsset(CAsset* const asset, const int setting)
{
    CPakAsset* pakAsset = static_cast<CPakAsset*>(asset);
    const AnimRigAsset* const animRigAsset = reinterpret_cast<AnimRigAsset*>(pakAsset->extraData());

    if (!animRigAsset)
        return false;

    assertm(animRigAsset->name, "No name for anim rig.");

    // Create exported path + asset path.
    std::filesystem::path exportPath = g_rsxSettings.GetExportDirectory();
    const std::filesystem::path rigPath(animRigAsset->name);
    const std::string rigStem(rigPath.stem().string());

    // truncate paths?
    if (g_rsxSettings.exportPathsFull)
        exportPath.append(rigPath.parent_path().string());
    else
        exportPath.append(std::format("{}/{}", s_PathPrefixARIG, rigStem));

    if (!CreateDirectories(exportPath))
    {
        assertm(false, "Failed to create asset directory.");
        return false;
    }

    const ModelParsedData_t* const parsedData = &animRigAsset->parsedData;

    if (g_rsxSettings.exportRigSequences && animRigAsset->numAnimSeqs > 0)
    {
        if (!ExportAnimSeqFromAsset(exportPath, rigStem, animRigAsset->name, animRigAsset->numAnimSeqs, animRigAsset->animSeqs, animRigAsset->GetRig()))
            return false;
    }

    if (g_rsxSettings.exportRigSequences && parsedData->NumLocalSeq() > 0)
    {
        std::filesystem::path outputPath(exportPath);
        outputPath.append(std::format("anims_{}/temp", rigStem));

        if (!CreateDirectories(outputPath.parent_path()))
        {
            assertm(false, "Failed to create directory.");
            return false;
        }

        auto aseqAssetBinding = g_assetData.m_assetTypeBindings.find('qesa');
        assertm(aseqAssetBinding != g_assetData.m_assetTypeBindings.end(), "Unable to find asset type binding for \"aseq\" assets");

        for (int i = 0; i < parsedData->NumLocalSeq(); i++)
        {
            const ModelSeq_t* const seqdesc = parsedData->LocalSeq(i);

            outputPath.replace_filename(seqdesc->szlabel);

            ExportSeqDesc(aseqAssetBinding->second.e.exportSetting, seqdesc, outputPath, animRigAsset->name, animRigAsset->GetRig(), RTech::StringToGuid(seqdesc->szlabel));
        }
    }

    // rmax & cast just export the skeleton for now, perhaps in the future we could also export IK?

    exportPath.append(std::format("{}.rrig", rigStem));

    switch (setting)
    {
    case eAnimRigExportSetting::ANIMRIG_CAST:
    {
        return ExportModelCast(parsedData, exportPath, asset->GetAssetGUID());
    }
    case eAnimRigExportSetting::ANIMRIG_RMAX:
    {
        return ExportModelRMAX(parsedData, exportPath);
    }
    case eAnimRigExportSetting::ANIMRIG_RRIG:
    {
        return ExportRawAnimRigAsset(pakAsset, animRigAsset, exportPath);
    }
    case eAnimRigExportSetting::ANIMRIG_SMD:
    {
        return ExportModelSMD(parsedData, exportPath) && ExportModelQC(parsedData, exportPath, setting, 54);
    }
    default:
    {
        assertm(false, "Export setting is not handled.");
        return false;
    }
    }

    unreachable();
}

void InitAnimRigAssetType()
{
    AssetTypeBinding_t type =
    {
        .name = "Animation Rig",
        .type = 'gira',
        .headerAlignment = 8,
        .loadFunc = LoadAnimRigAsset,
        .postLoadFunc = PostLoadAnimRigAsset,
        .previewFunc = nullptr,
        .e = { ExportAnimRigAsset, 0, s_AnimRigExportSettingNames, ARRSIZE(s_AnimRigExportSettingNames) },
    };

    REGISTER_TYPE(type);
}