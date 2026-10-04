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

    CPakFile* const pak = static_cast<CPakFile* const>(container);
    const std::unordered_map<uint32_t, PakLoadedAssetTypeInfo_t> loadedAssetInfo = pak->GetLoadedAssetTypeInfo();

    if (loadedAssetInfo.contains(static_cast<uint32_t>(AssetType_t::MDL_)) == 0)
    {
        assertm(false, "in practice there should be at least one model in an rpak with an animrig");
    }

    const PakLoadedAssetTypeInfo_t& loadedModelInfo = loadedAssetInfo.at(static_cast<uint32_t>(AssetType_t::MDL_));
    assertm((loadedModelInfo.inconsistentHeaderSize || loadedModelInfo.inconsistentVersions) == false, "pak had a hodgepodge of versions");

    switch (pakAsset->version())
    {
    case 4:
    {
        AnimRigAssetHeader_v4_t* const hdr = reinterpret_cast<AnimRigAssetHeader_v4_t*>(pakAsset->header());

        const eMDLVersion version = GetModelVersionFromAsset(pak, hdr->data, loadedModelInfo.version, loadedModelInfo.headerSize);

        arigAsset = new AnimRigAsset(hdr, version);
        break;
    }
    case 5:
    case 6:
    {
        AnimRigAssetHeader_v5_t* const hdr = reinterpret_cast<AnimRigAssetHeader_v5_t*>(pakAsset->header());

        const eMDLVersion version = GetModelVersionFromAsset(pak, hdr->data, loadedModelInfo.version, loadedModelInfo.headerSize);

        arigAsset = new AnimRigAsset(hdr, version);
        break;
    }
    case 7:
    {
        AnimRigAssetHeader_v5_t* const hdr = reinterpret_cast<AnimRigAssetHeader_v5_t*>(pakAsset->header());

        const eMDLVersion version = GetModelVersionFromAsset(pak, hdr->data, loadedModelInfo.version, loadedModelInfo.headerSize);

        arigAsset = new AnimRigAsset(hdr, version);
        break;
    }
    default:
        return;
    }

    ModelParsedData_t* const parsedData = arigAsset->GetParsedData();

    switch (arigAsset->studioVersion)
    {
    case eMDLVersion::VERSION_8:
    case eMDLVersion::VERSION_9:
    case eMDLVersion::VERSION_10:
    case eMDLVersion::VERSION_11:
    case eMDLVersion::VERSION_12:
    {
        parsedData->ParseModelBoneData<r5::mstudiobone_v8_t>();
        parsedData->ParseModelAttachmentData<r5::mstudioattachment_v8_t>();
        parsedData->ParseModelHitboxData<r5::mstudiobbox_v8_t>();
        parsedData->ParseModelAnimTypes_V8();

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
        parsedData->ParseModelBoneData<r5::mstudiobone_v12_1_t>();
        parsedData->ParseModelAttachmentData<r5::mstudioattachment_v8_t>();
        parsedData->ParseModelHitboxData<r5::mstudiobbox_v8_t>();
        parsedData->ParseModelAnimTypes_V8();

        break;
    }
    case eMDLVersion::VERSION_16:
    case eMDLVersion::VERSION_17:
    case eMDLVersion::VERSION_18:
    {
        parsedData->ParseModelBoneData_v16();
        parsedData->ParseModelAttachmentData<r5::mstudioattachment_v16_t>();
        parsedData->ParseModelHitboxData_v16();
        parsedData->ParseModelAnimTypes_V16();

        break;
    }
    case eMDLVersion::VERSION_19:
    case eMDLVersion::VERSION_19_1:
    case eMDLVersion::VERSION_19_2:
    case eMDLVersion::VERSION_19_3:
    case eMDLVersion::VERSION_20:
    {
        parsedData->ParseModelBoneData_v19();
        parsedData->ParseModelAttachmentData<r5::mstudioattachment_v16_t>();
        parsedData->ParseModelHitboxData_v16();
        parsedData->ParseModelAnimTypes_V16();

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

    ModelParsedData_t* const parsedData = arigAsset->GetParsedData();

    // parse sequences for children
    ParseExternalSequences(parsedData, arigAsset->numAnimSeqs, arigAsset->animSeqs);

    // [rika]: this should never get hit
    if (parsedData->LocalSeqCount() == 0)
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
        parsedData->ParseModelSequenceData_NoStall();

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
        parsedData->ParseModelSequenceData_Stall_V8();

        break;
    }
    case eMDLVersion::VERSION_16:
    case eMDLVersion::VERSION_17:
    {
        parsedData->ParseModelSequenceData_Stall_V16();

        break;
    }
    case eMDLVersion::VERSION_18:
    case eMDLVersion::VERSION_19:
    {
        parsedData->ParseModelSequenceData_Stall_V18();

        break;
    }
    case eMDLVersion::VERSION_19_1:
    case eMDLVersion::VERSION_19_2:
    {
        parsedData->ParseModelSequenceData_Stall_V19_1(ANIM_BONEFLAG_BITS_4);

        break;
    }
    case eMDLVersion::VERSION_19_3:
    case eMDLVersion::VERSION_20:
    {
        parsedData->ParseModelSequenceData_Stall_V19_1(ANIM_BONEFLAG_BITS_6);

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
    rigOut.write(reinterpret_cast<const char*>(animRigAsset->data), animRigAsset->parsedData.length);
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

    if (g_rsxSettings.exportRigSequences && parsedData->LocalSeqCount() > 0)
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

        for (int i = 0; i < parsedData->LocalSeqCount(); i++)
        {
            const ModelSeq_t* const seqdesc = parsedData->pLocalSeq(i);

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