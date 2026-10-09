#include <pch.h>
#include <core/mdl/modeldata.h>
#include <game/model/sourcemodel.h>

#include <core/render/dx.h>
#include <thirdparty/imgui/imgui.h>
#include <thirdparty/imgui/misc/imgui_utility.h>

extern CBufferManager g_BufferManager;
extern RSXSettings_t g_rsxSettings;

static const char* const s_PathPrefixMDL = s_AssetTypePaths.find(AssetType_t::MDL)->second;
static const char* const s_PathPrefixSEQ = s_AssetTypePaths.find(AssetType_t::SEQ)->second;

void CSourceModelAsset::FixupSkinData()
{
    const int skinCount = static_cast<int>(m_modelParsed->SkinCount());

    // [rika]: for models with only animations basically.
    if (skinCount == 0)
        return;

    ModelSkinData_t* const skinData = m_modelParsed->skins;
    skinData[0].name = STUDIO_DEFAULT_SKIN_NAME; // [rika]: use default name for first skin

    // [rika]: only the default skin
    if (skinCount == 1)
        return;

    char fmtbuf[16]{};

    //
    m_numModelSkinNames = skinCount - 1;
    m_modelSkinNames = new char*[m_numModelSkinNames] {};
    for (int i = 0; i < m_numModelSkinNames; i++)
    {
        snprintf(fmtbuf, sizeof(fmtbuf), "skin_%i\0", i);
        const size_t length = strnlen_s(fmtbuf, sizeof(fmtbuf)) + 1;
        
        char* tmp = new char[length] {};
        strcpy_s(tmp, length, fmtbuf);

        m_modelSkinNames[i] = tmp;
        skinData[i + 1].name = tmp;
    }
}

template<typename mstudiotexture_t>
void ParseSourceModelTextureData(ModelParsedData_t* const parsedData)
{
    if (parsedData->numMaterials == 0)
    {
        INDEX_TO_NULL(parsedData->materials);
        INDEX_TO_NULL(parsedData->skins);
        INDEX_TO_NULL(parsedData->cdMaterials);

        return;
    }

    const mstudiotexture_t* const pTextures = reinterpret_cast<const mstudiotexture_t* const>(parsedData->baseptr + INDEX_GET(parsedData->materials, 0));

    INDEX_TO_PTR(parsedData->materials, parsedData->numMaterials, ModelMaterialData_t);

    for (int i = 0; i < parsedData->numMaterials; ++i)
    {
        ModelMaterialData_t* const pMaterial = parsedData->materials + i;
        const mstudiotexture_t* const texture = pTextures + i;

        const std::string skn = std::format("material/{}_skn.rpak", texture->pszName());
        const std::string fix = std::format("material/{}_fix.rpak", texture->pszName());

        const uint64_t sknGUID = RTech::StringToGuid(skn.c_str());
        const uint64_t fixGUID = RTech::StringToGuid(fix.c_str());

        CPakAsset* const sknAsset = g_assetData.FindAsset<CPakAsset>(skn);
        CPakAsset* const fixAsset = g_assetData.FindAsset<CPakAsset>(fix);

        if (sknAsset)
        {
            pMaterial->asset = sknAsset;
            pMaterial->guid = sknGUID;
        }
        else if (fixAsset)
        {
            pMaterial->asset = sknAsset;
            pMaterial->guid = fixGUID;
        }
        else
        {
            pMaterial->asset = nullptr;
            pMaterial->guid = 0;
        }

        pMaterial->SetName(texture->pszName());
    }
    
    const int skinindex = INDEX_GET(parsedData->skins, 0);

    INDEX_TO_PTR(parsedData->skins, parsedData->numSkins, ModelSkinData_t);

    // [rika]: skin names will be fixed up in the load function
    for (int i = 0; i < parsedData->numSkins; i++)
    {
        parsedData->skins[i] = ModelSkinData_t(STUDIO_NULL_SKIN_NAME, parsedData->pSkinFamily(i, skinindex));
    }

    if (parsedData->numCdMaterials)
    {
        const int* const cdMaterialIndices = reinterpret_cast<const int* const>(parsedData->baseptr + INDEX_GET(parsedData->cdMaterials, 0));
        INDEX_TO_PTR(parsedData->cdMaterials, parsedData->numCdMaterials, const char*);

        for (int i = 0; i < parsedData->numCdMaterials; i++)
        {
            parsedData->cdMaterials[i] = parsedData->baseptr + cdMaterialIndices[i];
        }
    }
    else
    {
        INDEX_TO_NULL(parsedData->cdMaterials);
    }
}

template<typename mstudioikchain_t>
void ParseSourceModelAnimTypes(ModelParsedData_t* const parsedData)
{
    if (parsedData->numIkChains > 0)
    {
        const mstudioikchain_t* const pIKChains = reinterpret_cast<const mstudioikchain_t* const>(parsedData->baseptr + INDEX_GET(parsedData->ikChains, 0));
        INDEX_TO_PTR(parsedData->ikChains, parsedData->numIkChains, ModelIKChain_t);

        for (int i = 0; i < parsedData->numIkChains; i++)
        {
            const ModelIKChain_t ikchain(pIKChains + i);
            memcpy_s(parsedData->ikChains + i, sizeof(ModelIKChain_t), &ikchain, sizeof(ModelIKChain_t));
        }
    }
    else
    {
        INDEX_TO_NULL(parsedData->ikChains);
    }

    if (parsedData->numPoseParm > 0)
    {
        const mstudioposeparamdesc_t* const pPoseParams = reinterpret_cast<const mstudioposeparamdesc_t* const>(parsedData->baseptr + INDEX_GET(parsedData->poseParams, 0));
        INDEX_TO_PTR(parsedData->poseParams, parsedData->numPoseParm, ModelPoseParam_t);

        for (int i = 0; i < parsedData->numPoseParm; i++)
        {
            const ModelPoseParam_t poseparam(pPoseParams + i);
            memcpy_s(parsedData->poseParams + i, sizeof(ModelPoseParam_t), &poseparam, sizeof(ModelPoseParam_t));
        }
    }
    else
    {
        INDEX_TO_NULL(parsedData->poseParams);
    }

    if (parsedData->numLocalNodes > 0)
    {
        const int localNodeNameIndex = INDEX_GET(parsedData->localNodeNames, 0);
        const int* const nodeNameIndices = reinterpret_cast<const int* const>(parsedData->baseptr + localNodeNameIndex);
        INDEX_TO_PTR(parsedData->localNodeNames, parsedData->numLocalNodes, const char*);

        for (int i = 0; i < parsedData->numLocalNodes; i++)
        {
            parsedData->localNodeNames[i] = parsedData->baseptr + nodeNameIndices[i];
        }
    }
    else
    {
        INDEX_TO_NULL(parsedData->localNodeNames);
    }

    // technically supported but never used
    if (parsedData->numIkLocks > 0)
    {
        //printf("wooowowww~~!! iklocks in: %s\n", pStudioHdr->pszName());

        const mstudioiklock_t* const pIKLocks = reinterpret_cast<const mstudioiklock_t* const>(parsedData->baseptr + INDEX_GET(parsedData->ikLocks, 0));
        INDEX_TO_PTR(parsedData->ikLocks, parsedData->numIkLocks, ModelIKLock_t);

        for (int i = 0; i < parsedData->numIkLocks; i++)
        {
            const ModelIKLock_t iklock(pIKLocks + i);
            memcpy_s(parsedData->ikLocks + i, sizeof(ModelIKLock_t), &iklock, sizeof(ModelIKLock_t));
        }
    }
    else
    {
        INDEX_TO_NULL(parsedData->ikLocks);
    }
}

void LoadSourceModelAsset(CAssetContainer* container, CAsset* asset)
{
    CSourceModelAsset* const srcMdlAsset = static_cast<CSourceModelAsset* const>(asset);
    CSourceModelSource* const srcMdlSource = static_cast<CSourceModelSource* const>(container);

    ModelParsedData_t* parsedData = nullptr;
    StudioLooseData_t* looseData = nullptr;

    switch (srcMdlAsset->GetAssetVersion().majorVer)
    {
    case 52:
    {
        r1::studiohdr_t* const pStudioHdr = reinterpret_cast<r1::studiohdr_t* const>(srcMdlAsset->GetAssetData());

        CManagedBuffer* buffer = g_BufferManager.ClaimBuffer();

        looseData = new StudioLooseData_t(srcMdlSource->GetFilePath(), pStudioHdr->pszName(), buffer->Buffer(), managedBufferSize);

        g_BufferManager.RelieveBuffer(buffer);

        // these are now managed by the asset
        srcMdlAsset->SetExtraData(looseData->VertBuf(), CSourceModelAsset::SRCMDL_VERT);
        srcMdlAsset->SetExtraData(looseData->PhysBuf(), CSourceModelAsset::SRCMDL_PHYS);

        // parsed data
        parsedData = new ModelParsedData_t(pStudioHdr, asset->GetAssetVersion(), looseData);

        parsedData->ParseModelBoneData<r1::mstudiobone_t>();
        parsedData->ParseModelAttachmentData<mstudioattachment_t>();
        parsedData->ParseModelHitboxData<mstudiobbox_t>();
        ParseSourceModelTextureData<r1::mstudiotexture_t>(parsedData);
        ParseSourceModelAnimTypes<mstudioikchain_t>(parsedData);

        parsedData->ParseModelVertexData_VTX<r1::mstudiomodel_t, r1::mstudiomesh_t>(looseData);

        srcMdlAsset->SetName(pStudioHdr->pszName());

        break;
    }
    case 53:
    {
        r2::studiohdr_t* const pStudioHdr = reinterpret_cast<r2::studiohdr_t* const>(srcMdlAsset->GetAssetData());
        looseData = new StudioLooseData_t(reinterpret_cast<char*>(pStudioHdr));

        // parsed data
        parsedData = new ModelParsedData_t(pStudioHdr, asset->GetAssetVersion());

        parsedData->ParseModelBoneData<r2::mstudiobone_t>();
        parsedData->ParseModelAttachmentData<mstudioattachment_t>();
        parsedData->ParseModelHitboxData<r2::mstudiobbox_t>();
        ParseSourceModelTextureData<r2::mstudiotexture_t>(parsedData);
        ParseSourceModelAnimTypes<r2::mstudioikchain_t>(parsedData);

        parsedData->ParseModelVertexData_VTX<r2::mstudiomodel_t, r2::mstudiomesh_t>(looseData);

        srcMdlAsset->SetName(pStudioHdr->pszName());

        // include models

        break;
    }
    default:
    {
        assertm(false, "unaccounted asset version, will cause major issues!");
        return;
    }
    }

    assertm(srcMdlAsset->GetName(), "model should have name, invalid model.");

    const std::string name = std::format("{}/{}", s_PathPrefixMDL, srcMdlAsset->GetName());

    srcMdlAsset->SetAssetName(name);
    srcMdlAsset->SetParsedData(parsedData);
    srcMdlAsset->SetLooseData(looseData);

    srcMdlSource->SetFileName(GetStringAfterLastSlash(srcMdlAsset->GetName()));

    //srcMdlAsset->FixupSkinData();
}

void PostLoadSourceModelAsset(CAssetContainer* container, CAsset* asset)
{
    UNUSED(container);
    UNUSED(asset);
}

// [rika]: todo remove duplicate code and make one function (PreviewModelAsset)
void* PreviewSourceModelAsset(CAsset* const asset, const bool firstFrameForAsset)
{
    CSourceModelAsset* const srcMdlAsset = static_cast<CSourceModelAsset*>(asset);
    assertm(srcMdlAsset, "Asset should be valid.");

    ModelParsedData_t* const parsedData = srcMdlAsset->GetParsedData();

    static ModelPreviewInfo_t previewInfo;

    if (firstFrameForAsset)
    {
        previewInfo.bodygroupModelSelected.clear();

        previewInfo.bodygroupModelSelected.resize(parsedData->BodypartCount(), 0ull);

        previewInfo.selectedBodypartIndex = previewInfo.selectedBodypartIndex > parsedData->BodypartCount() ? 0 : previewInfo.selectedBodypartIndex;
        previewInfo.selectedSkinIndex = previewInfo.selectedSkinIndex > parsedData->SkinCount() ? 0 : previewInfo.selectedSkinIndex;

        // [rika]: lod handling
        assertm(parsedData->LODCount(), "no lods in preview?");
        previewInfo.maxLODIndex = static_cast<uint8_t>(parsedData->LODCount()) - 1;
        previewInfo.selectedLODLevel = previewInfo.selectedLODLevel > previewInfo.maxLODIndex ? previewInfo.maxLODIndex : previewInfo.selectedLODLevel; // clamp it
    }

    return PreviewParsedData(&previewInfo, parsedData, srcMdlAsset->GetName(), asset->GetAssetGUID(), firstFrameForAsset);
}

bool ExportSourceModelAsset(CAsset* const asset, const int setting)
{
    UNUSED(setting);

    // fix our setting
    if (!g_assetData.m_assetTypeBindings.count(static_cast<uint32_t>(AssetType_t::MDL_)))
    {
        assertm(false, "bad export setting");
        return false;
    }

    const int settingFixup = g_assetData.m_assetTypeBindings.find(static_cast<uint32_t>(AssetType_t::MDL_))->second.e.exportSetting;

    CSourceModelAsset* const srcMdlAsset = static_cast<CSourceModelAsset* const>(asset);
    assertm(srcMdlAsset, "Asset should be valid.");

    // Create exported path + asset path.
    std::filesystem::path exportPath = g_rsxSettings.GetExportDirectory();
    const std::filesystem::path modelPath(srcMdlAsset->GetAssetName());
    const std::string modelStem(modelPath.stem().string());

    // truncate paths?
    if (g_rsxSettings.exportPathsFull)
    {
        exportPath.append(modelPath.parent_path().string());
    }
    else
    {
        exportPath.append(s_PathPrefixMDL);
        exportPath.append(modelStem);
    }

    if (!CreateDirectories(exportPath))
    {
        assertm(false, "Failed to create asset directory.");
        return false;
    }

    // [rika]: handle sequence exporting
    if (g_rsxSettings.exportRigSequences && srcMdlAsset->GetSequenceCount() > 0)
    {
        const uint32_t type = static_cast<uint32_t>(AssetType_t::SEQ);

        assertm(g_assetData.m_assetTypeBindings.contains(type), "did not contain asset binding for source sequences");

        const AssetTypeBinding_t& binding = g_assetData.m_assetTypeBindings.find(type)->second;

        for (int i = 0; i < srcMdlAsset->GetSequenceCount(); i++)
        {
            const uint64_t guid = srcMdlAsset->GetSequenceGUID(i);

            CSourceSequenceAsset* const sequence = static_cast<CSourceSequenceAsset*>(g_assetData.FindAssetByGUID(guid));
            if (!sequence)
                continue;

            binding.e.exportFunc(sequence, binding.e.exportSetting);
        }
    }

    exportPath.append(std::format("{}.mdl", modelStem));    

    const ModelParsedData_t* const parsedData = srcMdlAsset->GetParsedData();

    switch (settingFixup)
    {
    case eModelExportSetting::MODEL_CAST:
    case eModelExportSetting::MODEL_RMAX:
    case eModelExportSetting::MODEL_SMD:
    {
        return ExportModelMeshes(parsedData, exportPath, settingFixup, 54);
    }
    default:
    {
        assertm(false, "Export setting is not handled.");
        return false;
    }
    }

    unreachable();
}

void InitSourceModelAssetType()
{
    AssetTypeBinding_t type =
    {
        .name = "Source Model",
        .type = 'ldm',
        .headerAlignment = 8,
        .loadFunc = LoadSourceModelAsset,
        .postLoadFunc = PostLoadSourceModelAsset,
        .previewFunc = PreviewSourceModelAsset,
        .e = { ExportSourceModelAsset, 0, nullptr, 0ull },
    };

    REGISTER_TYPE(type);
}

void LoadSourceSequenceAsset(CAssetContainer* container, CAsset* asset)
{
    UNUSED(container);

    CSourceSequenceAsset* const srcSeqAsset = static_cast<CSourceSequenceAsset* const>(asset);
    ModelSeq_t* sequence = nullptr;

    switch (srcSeqAsset->GetAssetVersion().majorVer)
    {
    case 52:
    {
        break;
    }
    case 53:
    {
        const r2::mstudioseqdesc_t* const pSeqdesc = reinterpret_cast<const r2::mstudioseqdesc_t* const>(srcSeqAsset->GetSequenceData());
        sequence = new ModelSeq_t(pSeqdesc);

        break;
    }
    default:
    {
        assertm(false, "unaccounted asset version, will cause major issues!");
        return;
    }
    }

    srcSeqAsset->SetSequence(sequence);
}

void PostLoadSourceSequenceAsset(CAssetContainer* const container, CAsset* const asset)
{
    UNUSED(container);

    CSourceSequenceAsset* const srcSeqAsset = static_cast<CSourceSequenceAsset* const>(asset);

    const CSourceModelAsset* const srcMdlAsset = reinterpret_cast<const CSourceModelAsset* const>(g_assetData.FindAssetByGUID(srcSeqAsset->GetRigGUID()));
    if (srcMdlAsset == nullptr)
    {
        assertm(false, "no bones ?");
        return;
    }

    srcSeqAsset->SetRig(srcMdlAsset->GetRig()); // bone
    const ModelParsedData_t* const rig = srcSeqAsset->GetRig();
    assertm(rig->BoneCount(), "we should have bones at this point.");

    ModelSeq_t* const seqdesc = srcSeqAsset->GetSequence();
    switch (srcSeqAsset->GetAssetVersion().majorVer)
    {
    case 52:
    {
        break;
    }
    case 53:
    {
        ParseSequence(seqdesc, rig, reinterpret_cast<const r2::studiohdr_t* const>(srcMdlAsset->GetAssetData()));

        break;
    }
    default:
        break;
    }

    // the sequence has been parsed for exporting
    srcSeqAsset->SetParsed();
}

void* PreviewSourceSequenceAsset(CAsset* const asset, const bool firstFrameForAsset)
{
    UNUSED(firstFrameForAsset);

    const CSourceSequenceAsset* const srcSeqAsset = static_cast<CSourceSequenceAsset*>(asset);

    PreviewSeqDesc(srcSeqAsset->GetSequence());

    return nullptr;
}

bool ExportSourceSequenceAsset(CAsset* const asset, const int setting)
{
    UNUSED(setting);

    // fix our setting
    if (!g_assetData.m_assetTypeBindings.count(static_cast<uint32_t>(AssetType_t::ASEQ)))
    {
        assertm(false, "bad export setting");
        return false;
    }

    const int settingFixup = g_assetData.m_assetTypeBindings.find(static_cast<uint32_t>(AssetType_t::ASEQ))->second.e.exportSetting;

    // [rika]: don't support exporting the raw data as it should be stored in the model file
    if (settingFixup == eAnimSeqExportSetting::ANIMSEQ_RSEQ)
        return true;

    CSourceSequenceAsset* const srcSeqAsset = static_cast<CSourceSequenceAsset* const>(asset);

    if (!srcSeqAsset->IsParsed())
        return false;

    const CSourceModelAsset* const srcMdlAsset = reinterpret_cast<const CSourceModelAsset* const>(g_assetData.FindAssetByGUID(srcSeqAsset->GetRigGUID()));
    if (srcMdlAsset == nullptr)
    {
        assertm(false, "no bones ?");
        return false;
    }

    // Create exported path + asset path.
    std::filesystem::path exportPath = g_rsxSettings.GetExportDirectory();
    const std::filesystem::path seqPath(srcSeqAsset->GetAssetName());
    const std::string seqStem(seqPath.stem().string());
    const std::string srcStem(std::filesystem::path(srcSeqAsset->GetContainerFileName()).stem().string());

    // truncate paths?
    if (g_rsxSettings.exportPathsFull)
    {
        exportPath.append(seqPath.parent_path().string());
    }
    else
    {
        exportPath.append(s_PathPrefixSEQ);
        exportPath.append(srcStem);
    }

    if (!CreateDirectories(exportPath))
    {
        assertm(false, "Failed to create asset directory.");
        return false;
    }

    exportPath.append(std::format("{}.seq", seqStem));

    const ModelParsedData_t* const rig = srcSeqAsset->GetRig();
    assertm(rig->BoneCount(), "we should have bones at this point.");

    std::filesystem::path exportPathCop = exportPath;

    return ExportSeqDesc(settingFixup, srcSeqAsset->GetSequence(), exportPath, srcMdlAsset->GetAssetName().c_str(), rig, asset->GetAssetGUID());
}

void InitSourceSequenceAssetType()
{
    AssetTypeBinding_t type =
    {
        .name = "Source Sequence",
        .type = 'qes',
        .headerAlignment = 8,
        .loadFunc = LoadSourceSequenceAsset,
        .postLoadFunc = PostLoadSourceSequenceAsset,
        .previewFunc = PreviewSourceSequenceAsset,
        .e = { ExportSourceSequenceAsset, 0, nullptr, 0ull },
    };

    REGISTER_TYPE(type);
}