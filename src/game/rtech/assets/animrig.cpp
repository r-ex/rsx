#include <pch.h>
#include <game/rtech/assets/rson.h>
#include <game/rtech/assets/animrig.h>
#include <game/rtech/assets/animseq.h>

#include <core/mdl/stringtable.h>
#include <core/mdl/rmax.h>
#include <core/mdl/cast.h>

#include <thirdparty/imgui/imgui.h>
#include <thirdparty/imgui/misc/imgui_utility.h>

extern ExportSettings_t g_ExportSettings;

static void ParseRigBoneData_v8(CPakAsset* const asset, AnimRigAsset* const modelAsset)
{
    UNUSED(asset);

    const r5::mstudiobone_v8_t* const bones = reinterpret_cast<r5::mstudiobone_v8_t*>((char*)modelAsset->data + modelAsset->studiohdr.boneOffset);
    modelAsset->bones.resize(modelAsset->studiohdr.boneCount);

    for (int i = 0; i < modelAsset->studiohdr.boneCount; i++)
    {
        modelAsset->bones.at(i) = ModelBone_t(&bones[i]);
    }
}

static void ParseRigBoneData_v12_1(CPakAsset* const asset, AnimRigAsset* const modelAsset)
{
    UNUSED(asset);

    const r5::mstudiobone_v12_1_t* const bones = reinterpret_cast<r5::mstudiobone_v12_1_t*>((char*)modelAsset->data + modelAsset->studiohdr.boneOffset);
    modelAsset->bones.resize(modelAsset->studiohdr.boneCount);

    for (int i = 0; i < modelAsset->studiohdr.boneCount; i++)
    {
        modelAsset->bones.at(i) = ModelBone_t(&bones[i]);
    }
}

static void ParseRigBoneData_v16(CPakAsset* const asset, AnimRigAsset* const modelAsset)
{
    UNUSED(asset);

    const r5::mstudiobonehdr_v16_t* const bonehdrs = reinterpret_cast<r5::mstudiobonehdr_v16_t*>((char*)modelAsset->data + modelAsset->studiohdr.boneOffset);
    const r5::mstudiobonedata_v16_t* const bonedata = reinterpret_cast<r5::mstudiobonedata_v16_t*>((char*)modelAsset->data + modelAsset->studiohdr.boneDataOffset);

    modelAsset->bones.resize(modelAsset->studiohdr.boneCount);

    for (int i = 0; i < modelAsset->studiohdr.boneCount; i++)
    {
        modelAsset->bones.at(i) = ModelBone_t(&bonehdrs[i], &bonedata[i]);
    }
}

void LoadAnimRigAsset(CAssetContainer* const pak, CAsset* const asset)
{
    UNUSED(pak);
    AnimRigAsset* arigAsset = nullptr;

    CPakAsset* pakAsset = static_cast<CPakAsset*>(asset);

    switch (pakAsset->version())
    {
    case 4:
    {
        AnimRigAssetHeader_v4_t* const hdr = reinterpret_cast<AnimRigAssetHeader_v4_t*>(pakAsset->header());
        arigAsset = new AnimRigAsset(hdr);
        break;
    }
    case 5:
    case 6:
    {
        AnimRigAssetHeader_v5_t* const hdr = reinterpret_cast<AnimRigAssetHeader_v5_t*>(pakAsset->header());
        arigAsset = new AnimRigAsset(hdr);
        break;
    }
    default:
        return;
    }

    arigAsset->studioVersion = GetModelPakVersion(reinterpret_cast<const int* const>(arigAsset->data));
    switch (arigAsset->studioVersion)
    {
    case eMDLVersion::VERSION_8:
    case eMDLVersion::VERSION_9:
    case eMDLVersion::VERSION_10:
    case eMDLVersion::VERSION_11:
    case eMDLVersion::VERSION_12:
    {
        r5::studiohdr_v8_t* const pHdr = reinterpret_cast<r5::studiohdr_v8_t*>(arigAsset->data);
        arigAsset->studiohdr = studiohdr_generic_t(pHdr);

        ParseRigBoneData_v8(pakAsset, arigAsset);

        break;
    }
    case eMDLVersion::VERSION_12_1:
    {
        r5::studiohdr_v12_1_t* const pHdr = reinterpret_cast<r5::studiohdr_v12_1_t*>(arigAsset->data);
        arigAsset->studiohdr = studiohdr_generic_t(pHdr);

        ParseRigBoneData_v12_1(pakAsset, arigAsset);

        break;
    }
    case eMDLVersion::VERSION_12_2:
    {
        r5::studiohdr_v12_2_t* const pHdr = reinterpret_cast<r5::studiohdr_v12_2_t*>(arigAsset->data);
        arigAsset->studiohdr = studiohdr_generic_t(pHdr);

        ParseRigBoneData_v12_1(pakAsset, arigAsset);

        break;
    }
    case eMDLVersion::VERSION_12_3:
    case eMDLVersion::VERSION_13:
    case eMDLVersion::VERSION_13_1:
    {
        r5::studiohdr_v12_3_t* const pHdr = reinterpret_cast<r5::studiohdr_v12_3_t*>(arigAsset->data);
        arigAsset->studiohdr = studiohdr_generic_t(pHdr);

        ParseRigBoneData_v12_1(pakAsset, arigAsset);

        break;
    }
    case eMDLVersion::VERSION_14:
    case eMDLVersion::VERSION_14_1:
    case eMDLVersion::VERSION_15:
    {
        r5::studiohdr_v14_t* const pHdr = reinterpret_cast<r5::studiohdr_v14_t*>(arigAsset->data);
        arigAsset->studiohdr = studiohdr_generic_t(pHdr);

        ParseRigBoneData_v12_1(pakAsset, arigAsset);

        break;
    }
    case eMDLVersion::VERSION_16:
    {
        r5::studiohdr_v16_t* const pHdr = reinterpret_cast<r5::studiohdr_v16_t*>(arigAsset->data);
        arigAsset->dataSize = FIX_OFFSET(pHdr->boneDataOffset) + (pHdr->boneCount * sizeof(r5::mstudiobonedata_v16_t));
        arigAsset->studiohdr = studiohdr_generic_t(pHdr, 0, arigAsset->dataSize);

        ParseRigBoneData_v16(pakAsset, arigAsset);

        break;
    }
    case eMDLVersion::VERSION_17:
    case eMDLVersion::VERSION_18:
    {
        r5::studiohdr_v17_t* const pHdr = reinterpret_cast<r5::studiohdr_v17_t*>(arigAsset->data);
        arigAsset->dataSize = FIX_OFFSET(pHdr->boneDataOffset) + (pHdr->boneCount * sizeof(r5::mstudiobonedata_v16_t));
        arigAsset->studiohdr = studiohdr_generic_t(reinterpret_cast<r5::studiohdr_v16_t*>(pHdr), 0, arigAsset->dataSize);

        ParseRigBoneData_v16(pakAsset, arigAsset);

        break;
    }
    case eMDLVersion::VERSION_UNK:
    default:
        break;
    }

    assertm(arigAsset->name, "Rig had no name.");
    pakAsset->SetAssetName(arigAsset->name);
    pakAsset->setExtraData(arigAsset);
}

void PostLoadAnimRigAsset(CAssetContainer* const pak, CAsset* const asset)
{
    UNUSED(pak);

    CPakAsset* pakAsset = static_cast<CPakAsset*>(asset);

    AnimRigAsset* const arigAsset = reinterpret_cast<AnimRigAsset*>(pakAsset->extraData());
    assertm(nullptr != arigAsset, "extra data should be valid by this point.");

    // parse sequences for children
    const uint64_t* const guids = reinterpret_cast<const uint64_t*>(arigAsset->animSeqs);
    for (uint16_t seqIdx = 0; seqIdx < arigAsset->numAnimSeqs; seqIdx++)
    {
        const uint64_t guid = guids[seqIdx];
        CPakAsset* const animSeqAsset = g_assetData.FindAssetByGUID<CPakAsset>(guid);
        if (!animSeqAsset)
            continue;

        AnimSeqAsset* const animSeq = reinterpret_cast<AnimSeqAsset* const>(animSeqAsset->extraData());
        if (!animSeq)
        {
            assertm(false, "uuuuuuuhhhmm");
            continue;
        }

        animSeq->parentRig = !animSeq->parentRig ? arigAsset : animSeq->parentRig;
    }
}

static bool ExportRawAnimRigAsset(CPakAsset* const asset, const AnimRigAsset* const animRigAsset, std::filesystem::path& exportPath)
{
    UNUSED(asset);

    StreamIO rigOut(exportPath.string(), eStreamIOMode::Write);
    rigOut.write(reinterpret_cast<const char*>(animRigAsset->data), animRigAsset->dataSize);
    rigOut.close();

    // make a manifest of this assets dependencies
    exportPath.replace_extension(".rson");

    StreamIO depOut(exportPath.string(), eStreamIOMode::Write);
    WriteRSONDependencyArray(*depOut.W(), "seqs", animRigAsset->animSeqs, animRigAsset->numAnimSeqs);
    depOut.close();

    return true;
}

static bool ExportRMAXAnimRigAsset(CPakAsset* const asset, const AnimRigAsset* const animRigAsset, std::filesystem::path& exportPath)
{
    UNUSED(asset);

    const std::string fileNameBase = exportPath.stem().string();

    const std::string tmpName = std::format("{}.rmax", fileNameBase);
    exportPath.replace_filename(tmpName);

    rmax::RMAXExporter rmaxFile(exportPath, fileNameBase.c_str(), fileNameBase.c_str());

    // do bones
    rmaxFile.ReserveBones(animRigAsset->bones.size());
    for (auto& bone : animRigAsset->bones)
        rmaxFile.AddBone(bone.name, bone.parentIndex, bone.pos, bone.quat, bone.scale);

    rmaxFile.ToFile();

    return true;
}

static bool ExportCastAnimRigAsset(CPakAsset* const asset, const AnimRigAsset* const animRigAsset, std::filesystem::path& exportPath)
{
    const std::string fileNameBase = exportPath.stem().string();
    const std::string tmpName(std::format("{}.cast", fileNameBase));
    exportPath.replace_filename(tmpName);

    cast::CastExporter cast(exportPath.string());

    // cast
    cast::CastNode* const rootNode = cast.GetChild(0); // we only have one root node, no hash
    cast::CastNode* const modelNode = rootNode->AddChild(cast::CastId::Model, asset->data()->guid);

    // [rika]: we can predict how big this vector needs to be, however resizing it will make adding new members a pain.
    const size_t modelChildrenCount = 1; // skeleton (one)
    modelNode->ReserveChildren(modelChildrenCount);

    // do skeleton
    {
        const size_t boneCount = animRigAsset->bones.size();

        cast::CastNode* const skelNode = modelNode->AddChild(cast::CastId::Skeleton, RTech::StringToGuid(fileNameBase.c_str()));
        skelNode->ReserveChildren(boneCount);

        // uses hashes for lookup, still gets bone parents by index :clown:
        for (size_t i = 0; i < boneCount; i++)
        {
            const ModelBone_t* const boneData = &animRigAsset->bones.at(i);

            cast::CastNodeBone boneNode(skelNode);
            boneNode.MakeBone(boneData->name, boneData->parentIndex, &boneData->pos, &boneData->quat, false);
        }
    }

    cast.ToFile();

    return true;
}

static const char* const s_PathPrefixARIG = s_AssetTypePaths.find(AssetType_t::ARIG)->second;
bool ExportAnimRigAsset(CAsset* const asset, const int setting)
{
    CPakAsset* pakAsset = static_cast<CPakAsset*>(asset);
    const AnimRigAsset* const animRigAsset = reinterpret_cast<AnimRigAsset*>(pakAsset->extraData());
    assertm(animRigAsset, "Extra data should be valid at this point.");
    assertm(animRigAsset->name, "No name for anim rig.");

    // Create exported path + asset path.
    std::filesystem::path exportPath = std::filesystem::current_path().append(EXPORT_DIRECTORY_NAME);
    const std::filesystem::path rigPath(animRigAsset->name);
    const std::string rigStem(rigPath.stem().string());

    // truncate paths?
    if (g_ExportSettings.exportPathsFull)
        exportPath.append(rigPath.parent_path().string());
    else
        exportPath.append(std::format("{}/{}", s_PathPrefixARIG, rigStem));

    if (!CreateDirectories(exportPath))
    {
        assertm(false, "Failed to create asset directory.");
        return false;
    }

    // [rika]: needs setting to toggle this
    if (g_ExportSettings.exportRigSequences && animRigAsset->numAnimSeqs > 0)
    {
        auto aseqAssetBinding = g_assetData.m_assetTypeBindings.find('qesa');

        assertm(aseqAssetBinding != g_assetData.m_assetTypeBindings.end(), "Unable to find asset type binding for \"aseq\" assets");

        if (aseqAssetBinding != g_assetData.m_assetTypeBindings.end())
        {
            std::filesystem::path outputPath(exportPath);
            outputPath.append(std::format("anims_{}/temp", rigStem));

            if (!CreateDirectories(outputPath.parent_path()))
            {
                assertm(false, "Failed to create directory.");
                return false;
            }

            std::atomic<uint32_t> remainingSeqs = 0; // we don't actually need thread safe here
            const ProgressBarEvent_t* const seqExportProgress = g_pImGuiHandler->AddProgressBarEvent("Exporting Sequences..", static_cast<uint32_t>(animRigAsset->numAnimSeqs), &remainingSeqs, true);
            for (int i = 0; i < animRigAsset->numAnimSeqs; i++)
            {
                const uint64_t guid = animRigAsset->animSeqs[i].guid;

                CPakAsset* const animSeq = g_assetData.FindAssetByGUID<CPakAsset>(guid);

                if (nullptr == animSeq)
                {
                    Log("RRIG EXPORT: animseq asset 0x%llX was not loaded, skipping...\n", guid);

                    continue;
                }

                const AnimSeqAsset* const animSeqAsset = reinterpret_cast<AnimSeqAsset*>(animSeq->extraData());

                // skip this animation if for some reason it has not been parsed. if a loaded mdl/animrig has sequence children, it should always be parsed. possibly move this to an assert.
                if (!animSeqAsset->animationParsed)
                    continue;

                outputPath.replace_filename(std::filesystem::path(animSeqAsset->name).filename());

                ExportAnimSeqAsset(animSeq, aseqAssetBinding->second.e.exportSetting, animSeqAsset, outputPath, animRigAsset->name, &animRigAsset->bones);

                ++remainingSeqs;
            }
            g_pImGuiHandler->FinishProgressBarEvent(seqExportProgress);
        }
    }

    // rmax & cast just export the skeleton for now, perhaps in the future we could also export IK?

    exportPath.append(std::format("{}.rrig", rigStem));

    switch (setting)
    {
    case eAnimRigExportSetting::ANIMRIG_CAST:
    {
        return ExportCastAnimRigAsset(pakAsset, animRigAsset, exportPath);
    }
    case eAnimRigExportSetting::ANIMRIG_RMAX:
    {
        return ExportRMAXAnimRigAsset(pakAsset, animRigAsset, exportPath);
    }
    case eAnimRigExportSetting::ANIMRIG_RRIG:
    {
        return ExportRawAnimRigAsset(pakAsset, animRigAsset, exportPath);
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
        .type = 'gira',
        .headerAlignment = 8,
        .loadFunc = LoadAnimRigAsset,
        .postLoadFunc = PostLoadAnimRigAsset,
        .previewFunc = nullptr,
        .e = { ExportAnimRigAsset, 0, s_AnimRigExportSettingNames, ARRSIZE(s_AnimRigExportSettingNames) },
    };

    REGISTER_TYPE(type);
}