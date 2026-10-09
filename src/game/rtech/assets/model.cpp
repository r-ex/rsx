#include <pch.h>

#include <game/rtech/assets/model.h>
#include <game/rtech/assets/animrig.h>
#include <game/rtech/assets/animseq.h>
#include <game/rtech/assets/texture.h>
#include <game/rtech/assets/material.h>
#include <game/rtech/assets/rson.h>
#include <game/rtech/utils/bvh/bvh.h>
#include <game/rtech/utils/bsp/bspflags.h>

#include <core/render/dx.h>
#include <thirdparty/imgui/imgui.h>
#include <thirdparty/imgui/misc/imgui_utility.h>

#include <immintrin.h>

extern CBufferManager g_BufferManager;
extern RSXSettings_t g_rsxSettings;

void LoadModelAsset(CAssetContainer* const pak, CAsset* const asset)
{
    UNUSED(pak);

    CPakAsset* const pakAsset = static_cast<CPakAsset*>(asset);

    ModelAsset* mdlAsset = nullptr;
    const AssetPtr_t streamEntry = pakAsset->getStarPakStreamEntry(false); // vertex data is never opt streamed (I hope)

    const eMDLVersion ver = GetModelVersionFromAsset(pakAsset, static_cast<CPakFile* const>(pak));

    // [rika]: go and set our subversion
    switch (ver)
    {
    case eMDLVersion::VERSION_12_1:
    {
        asset->SetAssetVersion({ 12, 1 });
        break;
    }
    case eMDLVersion::VERSION_12_2:
    {
        asset->SetAssetVersion({ 12, 2 });
        break;
    }
    case eMDLVersion::VERSION_12_3:
    {
        asset->SetAssetVersion({ 12, 3 });
        break;
    }
    case eMDLVersion::VERSION_12_4:
    {
        asset->SetAssetVersion({ 12, 4 });
        break;
    }
    case eMDLVersion::VERSION_12_5:
    {
        asset->SetAssetVersion({ 12, 5 });
        break;
    }
    case eMDLVersion::VERSION_13_1:
    {
        asset->SetAssetVersion({ 13, 1 });
        break;
    }
    case eMDLVersion::VERSION_14_1:
    {
        asset->SetAssetVersion({ 14, 1 });
        break;
    }
    case eMDLVersion::VERSION_19_1:
    {
        asset->SetAssetVersion({ 19, 1 });
        break;
    }
    case eMDLVersion::VERSION_19_2:
    {
        asset->SetAssetVersion({ 19, 2 });
        break;
    }
    case eMDLVersion::VERSION_19_3:
    {
        asset->SetAssetVersion({ 19, 3 });
        break;
    }
    default:
    {
        break;
    }
    }

    switch (ver)
    {
    case eMDLVersion::VERSION_8:
    {
        ModelAssetHeader_v8_t* hdr = reinterpret_cast<ModelAssetHeader_v8_t*>(pakAsset->header());
        mdlAsset = new ModelAsset(hdr, streamEntry, ver, asset->GetAssetVersion());

        ModelParsedData_t* const parsedData = mdlAsset->GetParsedData();

        parsedData->ParseModelBoneData<r5::mstudiobone_v8_t>();
        parsedData->ParseModelAttachmentData<r5::mstudioattachment_v8_t>();
        parsedData->ParseModelHitboxData<r5::mstudiobbox_v8_t>();
        parsedData->ParseModelTextureData_v8();
        parsedData->ParseModelAnimTypes_V8();
        break;
    }
    case eMDLVersion::VERSION_9:
    case eMDLVersion::VERSION_10:
    case eMDLVersion::VERSION_11:
    case eMDLVersion::VERSION_12:
    {
        ModelAssetHeader_v9_t* hdr = reinterpret_cast<ModelAssetHeader_v9_t*>(pakAsset->header());
        mdlAsset = new ModelAsset(hdr, streamEntry, ver, asset->GetAssetVersion());

        ModelParsedData_t* const parsedData = mdlAsset->GetParsedData();

        parsedData->ParseModelBoneData<r5::mstudiobone_v8_t>();
        parsedData->ParseModelAttachmentData<r5::mstudioattachment_v8_t>();
        parsedData->ParseModelHitboxData<r5::mstudiobbox_v8_t>();
        parsedData->ParseModelTextureData_v8();
        parsedData->ParseModelAnimTypes_V8();
        break;
    }
    case eMDLVersion::VERSION_12_1: // has to have its own vertex func
    case eMDLVersion::VERSION_12_2:
    case eMDLVersion::VERSION_12_3:
    case eMDLVersion::VERSION_12_4:
    case eMDLVersion::VERSION_12_5:
    {
        ModelAssetHeader_v12_1_t* hdr = reinterpret_cast<ModelAssetHeader_v12_1_t*>(pakAsset->header());
        mdlAsset = new ModelAsset(hdr, streamEntry, ver, asset->GetAssetVersion());

        ModelParsedData_t* const parsedData = mdlAsset->GetParsedData();

        parsedData->ParseModelBoneData<r5::mstudiobone_v12_1_t>();
        parsedData->ParseModelAttachmentData<r5::mstudioattachment_v8_t>();
        parsedData->ParseModelHitboxData<r5::mstudiobbox_v8_t>();
        parsedData->ParseModelTextureData_v8();
        parsedData->ParseModelAnimTypes_V8();
        break;
    }
    case eMDLVersion::VERSION_13:
    case eMDLVersion::VERSION_13_1:
    case eMDLVersion::VERSION_14:
    case eMDLVersion::VERSION_14_1:
    case eMDLVersion::VERSION_15:
    {
        ModelAssetHeader_v13_t* hdr = reinterpret_cast<ModelAssetHeader_v13_t*>(pakAsset->header());
        mdlAsset = new ModelAsset(hdr, streamEntry, ver, asset->GetAssetVersion());

        ModelParsedData_t* const parsedData = mdlAsset->GetParsedData();

        parsedData->ParseModelBoneData<r5::mstudiobone_v12_1_t>();
        parsedData->ParseModelAttachmentData<r5::mstudioattachment_v8_t>();
        parsedData->ParseModelHitboxData<r5::mstudiobbox_v8_t>();
        parsedData->ParseModelTextureData_v8();
        parsedData->ParseModelAnimTypes_V8();
        break;
    }
    case eMDLVersion::VERSION_16:
    case eMDLVersion::VERSION_17:
    case eMDLVersion::VERSION_18:
    {
        ModelAssetHeader_v16_t* hdr = reinterpret_cast<ModelAssetHeader_v16_t*>(pakAsset->header());
        ModelAssetCPU_v16_t* cpu = reinterpret_cast<ModelAssetCPU_v16_t*>(pakAsset->cpu());
        mdlAsset = new ModelAsset(hdr, cpu, streamEntry, ver, asset->GetAssetVersion());

        ModelParsedData_t* const parsedData = mdlAsset->GetParsedData();

        parsedData->ParseModelBoneData_v16();
        parsedData->ParseModelAttachmentData<r5::mstudioattachment_v16_t>();
        parsedData->ParseModelHitboxData_v16();
        parsedData->ParseModelTextureData_v16();
        parsedData->ParseModelAnimTypes_V16();
        break;
    }
    case eMDLVersion::VERSION_19:
    case eMDLVersion::VERSION_19_1:
    case eMDLVersion::VERSION_19_2:
    case eMDLVersion::VERSION_19_3:
    case eMDLVersion::VERSION_20:
    {
        ModelAssetHeader_v16_t* hdr = reinterpret_cast<ModelAssetHeader_v16_t*>(pakAsset->header());
        ModelAssetCPU_v16_t* cpu = reinterpret_cast<ModelAssetCPU_v16_t*>(pakAsset->cpu());
        mdlAsset = new ModelAsset(hdr, cpu, streamEntry, ver, asset->GetAssetVersion());

        ModelParsedData_t* const parsedData = mdlAsset->GetParsedData();

        parsedData->ParseModelBoneData_v19();
        parsedData->ParseModelAttachmentData<r5::mstudioattachment_v16_t>();
        parsedData->ParseModelHitboxData_v16();
        parsedData->ParseModelTextureData_v16();
        parsedData->ParseModelAnimTypes_V16();
        break;
    }
    default:
    {
        assertm(false, "unaccounted asset version, will cause major issues!");
        return;
    }
    }

    assertm(mdlAsset->name, "Model had no name.");
    pakAsset->SetAssetName(mdlAsset->name, true);
    pakAsset->setExtraData(mdlAsset);
}

void PostLoadModelAsset(CAssetContainer* const pak, CAsset* const asset)
{
    UNUSED(pak);

    CPakAsset* const pakAsset = static_cast<CPakAsset*>(asset);

    ModelAsset* const modelAsset = reinterpret_cast<ModelAsset*>(pakAsset->extraData());

    if (!modelAsset)
    {
        return;
    }

    ModelParsedData_t* const parsedData = modelAsset->GetParsedData();

    // get vertex data
    const std::unique_ptr<char[]> pStreamed = modelAsset->vertexStreamingData.size > 0 ? pakAsset->getStarPakData(modelAsset->vertexStreamingData.offset, modelAsset->vertexStreamingData.size, false) : nullptr; // probably smarter to check the size inside getStarPakData but whatever!
    char* const vertexBuffer = pStreamed.get() ? pStreamed.get() : modelAsset->staticStreamingData;

    if (vertexBuffer == nullptr)
    {
        Log("%s loaded with no vertex data\n", modelAsset->name);
    }

    // parse sequences for children
    ParseExternalSequences(parsedData, modelAsset->numAnimSeqs, modelAsset->animSeqs);

    // external include models
    if (modelAsset->numAnimRigs)
    {
        parsedData->numExternalIncludeModels = modelAsset->numAnimRigs;
        parsedData->externalIncludeModels = modelAsset->animRigs;
    }

    // [rika]: in post load now because it depends on asqd
    switch (modelAsset->version)
    {
    case eMDLVersion::VERSION_8:
    {
        StudioLooseData_t looseData(reinterpret_cast<const char* const>(modelAsset->data), modelAsset->vertexComponentData, reinterpret_cast<const char* const>(modelAsset->physics));

        parsedData->ParseModelVertexData_VTX<r5::mstudiomodel_v8_t, r5::mstudiomesh_v8_t>(&looseData);
        parsedData->ParseModelSequenceData_NoStall();
        break;
    }
    case eMDLVersion::VERSION_9:
    case eMDLVersion::VERSION_10:
    case eMDLVersion::VERSION_11:
    case eMDLVersion::VERSION_12:
    {
        parsedData->ParseModelVertexData_v9(vertexBuffer);
        parsedData->ParseModelSequenceData_NoStall();
        break;
    }
    case eMDLVersion::VERSION_12_1: // has to have its own vertex func
    case eMDLVersion::VERSION_12_2:
    case eMDLVersion::VERSION_12_3:
    case eMDLVersion::VERSION_12_4:
    case eMDLVersion::VERSION_12_5:
    case eMDLVersion::VERSION_13:
    case eMDLVersion::VERSION_13_1:
    {
        parsedData->ParseModelVertexData_v12_1(vertexBuffer);
        parsedData->ParseModelSequenceData_Stall_V8();
        break;
    }
    case eMDLVersion::VERSION_14:
    case eMDLVersion::VERSION_14_1:
    case eMDLVersion::VERSION_15:
    {
        parsedData->ParseModelVertexData_v14(vertexBuffer);
        parsedData->ParseModelSequenceData_Stall_V8();
        break;
    }
    case eMDLVersion::VERSION_16:
    case eMDLVersion::VERSION_17:
    {
        parsedData->ParseModelVertexData_v16(vertexBuffer);
        parsedData->ParseModelSequenceData_Stall_V16();
        break;
    }
    case eMDLVersion::VERSION_18:
    case eMDLVersion::VERSION_19:
    {
        parsedData->ParseModelVertexData_v16(vertexBuffer);
        parsedData->ParseModelSequenceData_Stall_V18();
        break;
    }
    case eMDLVersion::VERSION_19_1:
    {
        parsedData->ParseModelVertexData_v16(vertexBuffer);
        parsedData->ParseModelSequenceData_Stall_V19_1(ANIM_BONEFLAG_BITS_4);
        break;
    }
    case eMDLVersion::VERSION_19_2:
    {
        parsedData->ParseModelVertexData_v16(vertexBuffer, VERT_PARSE_BONES_1024);
        parsedData->ParseModelSequenceData_Stall_V19_1(ANIM_BONEFLAG_BITS_4);
        break;
    }
    case eMDLVersion::VERSION_19_3:
    case eMDLVersion::VERSION_20:
    {
        parsedData->ParseModelVertexData_v16(vertexBuffer, VERT_PARSE_BONES_1024);
        parsedData->ParseModelSequenceData_Stall_V19_1(ANIM_BONEFLAG_BITS_6);
        break;
    }
    default:
    {
        assertm(false, "unaccounted asset version, will cause major issues!");
        return;
    }
    }
}

static void ModelPreview_AddExternalSeq(const uint64_t guid, const PreviewSeqType_e seqType, AnimRigAsset* const rig, ModelAsset* const modelAsset, ModelPreviewInfo_t& previewInfo, std::unordered_set<uint64_t>& knownGuids)
{
    // sequences should only be added once
    if (knownGuids.contains(guid))
        return;

    // add guids early so that if the anim is invalid in some way (like not loaded or no extra data) then we don't try and find it twice
    // bc it's not gonna suddenly be valid
    knownGuids.insert(guid);

    CPakAsset* const seqAsset = g_assetData.FindAssetByGUID<CPakAsset>(guid);

    if (!seqAsset || !seqAsset->hasExtraData())
        return;

    AnimSeqAsset* const animSeq = reinterpret_cast<AnimSeqAsset*>(seqAsset->extraData());

    if (!animSeq)
        return;

    // if this seq didn't get parsed for whatever reason (model was in an odl pak?) then record where it came from and parse it here
    if (animSeq->animationParsed == false)
    {
        if (animSeq->rig == nullptr)
        {
            // rig will be nullptr if the sequence asset was not found thru a rig and instead from the model itself
            if (rig)
                animSeq->rig = rig->GetParsedData();
            else
                animSeq->rig = modelAsset->GetParsedData();
        }

        AnimSeq_ParseExtraData(seqAsset);
    }

    const ModelParsedData_t* srcBones = animSeq->rig;

    previewInfo.sequences.emplace_back(
        animSeq->name,
        guid,
        &animSeq->seqdesc,
        srcBones,
        seqType,
        animSeq->animationParsed && nullptr != srcBones
    );
};

static void ModelPreview_DiscoverSequences(ModelAsset* const modelAsset, ModelPreviewInfo_t& previewInfo)
{
    previewInfo.sequences.clear();
    previewInfo.animState = AnimState_t{
        .selectedSeqIndex = -1,
        .selectedAnimIndex = -1,
        .activeSeqIdx = -1,
        .activeAnimIdx = -1,
        .frame = 0.f,
        .playing = false,
        .looping = true,
    };

    ModelParsedData_t* const parsedData = modelAsset->GetParsedData();

    for (int i = 0; i < parsedData->LocalSeqCount(); i++)
    {
        const ModelSeq_t* const seqdesc = parsedData->pLocalSeq(i);

        previewInfo.sequences.emplace_back(
            seqdesc->szlabel,
            0ull, // guid
            seqdesc,
            modelAsset->GetRig(), // local sequences are always parsed against the model's own skeleton
            PreviewSeqType_e::SEQ_LOCAL,
            true // local sequences are always already parsed
        );
    }

    std::unordered_set<uint64_t> knownGuids;

    // Add all sequences that are directly attached to the model asset (instead of being referenced by a rig that the model uses)
    for (uint32_t i = 0; i < modelAsset->numAnimSeqs; i++)
        ModelPreview_AddExternalSeq(modelAsset->animSeqs[i].guid, PreviewSeqType_e::SEQ_ASEQ, nullptr, modelAsset, previewInfo, knownGuids);

    // Go thru each of the model's rigs and find all seq assets that are referenced that way
    for (uint32_t i = 0; i < modelAsset->numAnimRigs; i++)
    {
        CPakAsset* const rigAsset = g_assetData.FindAssetByGUID<CPakAsset>(modelAsset->animRigs[i].guid);

        if (!rigAsset)
            continue;

        AnimRigAsset* const animRig = reinterpret_cast<AnimRigAsset*>(rigAsset->extraData());

        if (!animRig)
            continue;

        for (int j = 0; j < animRig->numAnimSeqs; j++)
            ModelPreview_AddExternalSeq(animRig->animSeqs[j].guid, PreviewSeqType_e::SEQ_ARIG, animRig, modelAsset, previewInfo, knownGuids);
    }
}

void* PreviewModelAsset(CAsset* const asset, const bool firstFrameForAsset)
{
    CPakAsset* const pakAsset = static_cast<CPakAsset*>(asset);

    assertm(pakAsset, "Asset should be valid.");

    ModelAsset* const modelAsset = reinterpret_cast<ModelAsset*>(pakAsset->extraData());

    ModelParsedData_t* const parsedData = modelAsset->GetParsedData();

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
        
        ModelPreview_DiscoverSequences(modelAsset, previewInfo);
    }

    ImGui::Text("Rigs: %i", modelAsset->numAnimRigs);
    ImGui::Text("Sequences: %i", modelAsset->numAnimSeqs);

    void* const drawData = PreviewParsedData(&previewInfo, parsedData, modelAsset->name, asset->GetAssetGUID(), firstFrameForAsset);

    if (drawData && Preview_SequencesSection(&previewInfo, parsedData, reinterpret_cast<CDXDrawData*>(drawData)))
        ModelPreview_DiscoverSequences(modelAsset, previewInfo);

    return drawData;
}

static bool ExportModelStreamedData(const ModelAsset* const modelAsset, std::filesystem::path& exportPath, const char* const streamedData, const char* const extension)
{
    const ModelParsedData_t* const parsedData = modelAsset->GetParsedData();

    switch (modelAsset->version)
    {
    case eMDLVersion::VERSION_8:
    case eMDLVersion::VERSION_9:
    case eMDLVersion::VERSION_10:
    case eMDLVersion::VERSION_11:
    case eMDLVersion::VERSION_12:
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
        // [rika]: .hwData would be better but this is set in stone at this point essentially
        exportPath.replace_extension(extension);

        StreamIO hwOut(exportPath.string(), eStreamIOMode::Write);
        hwOut.write(streamedData, parsedData->hwDataSize);

        return true;
    }
    case eMDLVersion::VERSION_16:
    case eMDLVersion::VERSION_17:
    case eMDLVersion::VERSION_18:
    case eMDLVersion::VERSION_19:
    case eMDLVersion::VERSION_19_1:
    case eMDLVersion::VERSION_19_2:
    case eMDLVersion::VERSION_19_3:
    case eMDLVersion::VERSION_20:
    {
        // special case because of compression
        exportPath.replace_extension(extension);

        CManagedBuffer* const outBuf = g_BufferManager.ClaimBuffer();
        CManagedBuffer* const grpBuf = g_BufferManager.ClaimBuffer();

        char* pPos = outBuf->Buffer(); // position in the decompressed buffer for writing

        for (int i = 0; i < parsedData->GroupCount(); i++)
        {
            const ModelHWGroup_t* const pGroup = parsedData->pLODGroup(i);

            switch (pGroup->dataCompression)
            {
            case eCompressionType::NONE:
            {
                memcpy_s(pPos, pGroup->dataSizeDecompressed, streamedData + pGroup->dataOffset, pGroup->dataSizeDecompressed);
                break;
            }
            case eCompressionType::PAKFILE:
            case eCompressionType::SNOWFLAKE:
            case eCompressionType::OODLE:
            {
                char* const dcmpBuf = grpBuf->Buffer();

                memcpy_s(dcmpBuf, pGroup->dataSizeCompressed, streamedData + pGroup->dataOffset, pGroup->dataSizeCompressed);

                size_t dataSizeDecompressed = pGroup->dataSizeDecompressed;
                RTech::DecompressStreamedBuffer(dcmpBuf, pPos, dataSizeDecompressed, pGroup->dataCompression);

                break;
            }
            default:
                break;
            }

            pPos += pGroup->dataSizeDecompressed; // advance position
        }

        StreamIO hwOut(exportPath.string(), eStreamIOMode::Write);
        hwOut.write(outBuf->Buffer(), parsedData->hwDataSize);

        g_BufferManager.RelieveBuffer(outBuf);
        g_BufferManager.RelieveBuffer(grpBuf);

        return true;
    }
    default:
        assertm(false, "Asset version not handled.");
        return false;
    }
}

static bool ExportRawModelAsset(const ModelAsset* const modelAsset, std::filesystem::path& exportPath, const char* const streamedData)
{
    // Is asset permanent or streamed?
    //const char* const pDataBuffer = streamedData ? streamedData : modelAsset->staticStreamingData;

    const ModelParsedData_t* const parsedData = modelAsset->GetParsedData();

    StreamIO studioOut(exportPath.string(), eStreamIOMode::Write);
    studioOut.write(reinterpret_cast<char*>(modelAsset->data), parsedData->length);

    if (parsedData->phySize > 0)
    {
        exportPath.replace_extension(".phy");

        // if we error here something is broken with setting up the model asset
        StreamIO physOut(exportPath.string(), eStreamIOMode::Write);
        physOut.write(reinterpret_cast<char*>(modelAsset->physics), parsedData->phySize);
    }

    // make a manifest of this assets dependencies
    exportPath.replace_extension(".rson");

    StreamIO depOut(exportPath.string(), eStreamIOMode::Write);
    WriteRSONDependencyArray(*depOut.W(), "rigs", modelAsset->animRigs, modelAsset->numAnimRigs);
    WriteRSONDependencyArray(*depOut.W(), "seqs", modelAsset->animSeqs, modelAsset->numAnimSeqs);
    depOut.close();

    // static (prop) streamed data
    if (modelAsset->staticStreamingData && modelAsset->streamingDataSize)
    {
        if (!ExportModelStreamedData(modelAsset, exportPath, modelAsset->staticStreamingData, ".vg_static"))
            return false;
    }

    // starpak streamed data
    if (streamedData && modelAsset->streamingDataSize)
    {
        if (!ExportModelStreamedData(modelAsset, exportPath, streamedData, ".vg"))
            return false;
    }

    // export the vertex components
    if (modelAsset->componentDataSize && modelAsset->vertexComponentData)
    {
        // vvd
        if (parsedData->vvdSize)
        {
            exportPath.replace_extension(".vvd");

            StreamIO vertOut(exportPath.string(), eStreamIOMode::Write);
            vertOut.write(modelAsset->vertexComponentData + parsedData->vvdOffset, parsedData->vvdSize);
        }

        // vvc
        if (parsedData->vvcSize > 0)
        {
            exportPath.replace_extension(".vvc");

            StreamIO vertColorOut(exportPath.string(), eStreamIOMode::Write);
            vertColorOut.write(modelAsset->vertexComponentData + parsedData->vvcOffset, parsedData->vvcSize);
        }

        // vvw
        if (parsedData->vvwSize > 0)
        {
            exportPath.replace_extension(".vvw");

            StreamIO vertWeightOut(exportPath.string(), eStreamIOMode::Write);
            vertWeightOut.write(modelAsset->vertexComponentData + parsedData->vvwOffset, parsedData->vvwSize);
        }

        // vtx
        if (parsedData->vtxSize > 0)
        {
            exportPath.replace_extension(".dx11.vtx"); // cope

            // 'opt' being optimized
            StreamIO vertOptOut(exportPath.string(), eStreamIOMode::Write);
            vertOptOut.write(modelAsset->vertexComponentData + parsedData->vtxOffset, parsedData->vtxSize); // [rika]: 
        }
    }

    return true;
}

template <typename phyheader_t>
static bool ExportPhysicsModelPhy(const ModelAsset* const modelAsset, std::filesystem::path& exportPath)
{
    const ModelParsedData_t* const parsedData = modelAsset->GetParsedData();

    if (parsedData->phySize == 0)
        return false;

    const int mask = (parsedData->contents & g_rsxSettings.exportPhysicsContentsFilter);
    const bool inFilter = g_rsxSettings.exportPhysicsFilterAND ? mask == static_cast<int>(g_rsxSettings.exportPhysicsContentsFilter) : mask != 0;

    const bool skip = g_rsxSettings.exportPhysicsFilterExclusive ? inFilter : !inFilter;

    if (skip)
        return false; // Filtered out.

    const phyheader_t* const phyHdr = reinterpret_cast<const phyheader_t*>(modelAsset->physics);
    const irps::phyptrheader_t* const ptrHdr = reinterpret_cast<const irps::phyptrheader_t*>(reinterpret_cast<const char*>(phyHdr) + sizeof(phyheader_t));

    CollisionModel_t colModel;

    for (int i = 0; i < phyHdr->solidCount; i++)
    {
        const irps::solidgroup_t* const solidGroup = reinterpret_cast<const irps::solidgroup_t*>((reinterpret_cast<const char*>(ptrHdr) + ptrHdr->solidOffset) + i * sizeof(irps::solidgroup_t));

        for (int j = 0; j < solidGroup->solidCount; j++)
        {
            const irps::solid_t* const solid = reinterpret_cast<const irps::solid_t*>((reinterpret_cast<const char*>(ptrHdr) + solidGroup->solidOffset) + j * sizeof(irps::solid_t));

            const Vector* const verts = reinterpret_cast<const Vector*>(reinterpret_cast<const char*>(ptrHdr) + solid->vertOffset);
            const irps::side_t* const sides = reinterpret_cast<const irps::side_t*>(reinterpret_cast<const char*>(ptrHdr) + solid->sideOffset);

            for (int k = 0; k < solid->sideCount; k++)
            {
                const irps::side_t& side = sides[k];
                const Vector& base = verts[side.vertIndices[0]];

                for (int vi = 1; vi < solid->vertCount - 1; ++vi)
                {
                    const int idx1 = side.vertIndices[vi];
                    const int idx2 = side.vertIndices[vi + 1];

                    if (idx1 == -1 || idx2 == -1)
                        break;

                    Triangle& tri = colModel.tris.emplace_back();

                    tri.a = base;
                    tri.b = verts[idx1];
                    tri.c = verts[idx2];
                    tri.flags = 0;
                }
            }
        }
    }

    if (!colModel.tris.size())
        return false;

    return colModel.exportSTL(exportPath.replace_extension(".stl"));
}

template <typename mstudiocollmodel_t, typename mstudiocollheader_t>
static bool ExportPhysicsModelBVH(const ModelAsset* const modelAsset, std::filesystem::path& exportPath)
{
    const ModelParsedData_t* const parsedData = modelAsset->GetParsedData();

    if (parsedData->bvhData == nullptr)
        return false;

    CollisionModel_t outModel;

    const mstudiocollmodel_t* collModel = reinterpret_cast<const mstudiocollmodel_t*>(parsedData->bvhData);
    const mstudiocollheader_t* collHeaders = reinterpret_cast<const mstudiocollheader_t*>(collModel + 1);

    const int headerCount = collModel->headerCount;

    // [amos]: so far only 1 model had this value: mdl/Humans/class/medium/pilot_medium_nova_01.rmdl.
    // unclear what it is yet, but the offset in the hdr looked correct and the mdl had no BVH.
    if (headerCount == 0x3F8000)
        return false;

    const uint32_t* maskData = reinterpret_cast<const uint32_t*>((reinterpret_cast<const char*>(collModel) + collModel->contentMasksIndex));

    for (int i = 0; i < headerCount; i++)
    {
        const mstudiocollheader_t& collHeader = collHeaders[i];

        const void* bvhNodes = reinterpret_cast<const char*>(collModel) + collHeader.bvhNodeIndex;
        const void* vertData = reinterpret_cast<const char*>(collModel) + collHeader.vertIndex;
        const void* leafData = reinterpret_cast<const char*>(collModel) + collHeader.bvhLeafIndex;

        BVHModel_t data;

        data.nodes = reinterpret_cast<const dbvhnode_t*>(bvhNodes);
        data.verts = reinterpret_cast<const Vector*>(vertData);
        data.packedVerts = reinterpret_cast<const PackedVector*>(vertData);
        data.leafs = reinterpret_cast<const char*>(leafData);
        data.masks = reinterpret_cast<const uint32_t*>(maskData);
        data.origin = reinterpret_cast<const Vector*>(&collHeader.origin);
        data.scale = collHeader.scale;
        data.maskFilter = g_rsxSettings.exportPhysicsContentsFilter;
        data.filterExclusive = g_rsxSettings.exportPhysicsFilterExclusive;
        data.filterAND = g_rsxSettings.exportPhysicsFilterAND;

        const dbvhnode_t* startNode = reinterpret_cast<const dbvhnode_t*>(bvhNodes);
        const uint32_t contents = maskData[startNode->cmIndex];

        Coll_HandleNodeChildType(outModel, contents, 0, startNode->child0Type, startNode->index0, &data);
        Coll_HandleNodeChildType(outModel, contents, 0, startNode->child1Type, startNode->index1, &data);
        Coll_HandleNodeChildType(outModel, contents, 0, startNode->child2Type, startNode->index2, &data);
        Coll_HandleNodeChildType(outModel, contents, 0, startNode->child3Type, startNode->index3, &data);
    }

    if (!outModel.tris.size() && !outModel.quads.size())
        return false;

    outModel.exportSTL(exportPath.replace_extension(".stl"));
    return true;
}

static bool ExportModelHitboxes(const ModelAsset* modelAsset, std::filesystem::path& exportPath)
{
    const ModelParsedData_t* parsedData = modelAsset->GetParsedData();

    std::string objData;

    for (int setIdx = 0; setIdx < parsedData->HitboxSetCount(); setIdx++)
    {
        const ModelHitboxSet_t* const pHitboxSet = parsedData->pHitboxSet(setIdx);

        for (int i = 0; i < pHitboxSet->numHitboxes; ++i)
        {
            const ModelHitbox_t* const pHitbox = pHitboxSet->pHitbox(i);

            objData += std::format("o {}_{}_{}\n", pHitboxSet->name, i, pHitbox->name);

            const Vector* bbmin = pHitbox->bbmin;
            const Vector* bbmax = pHitbox->bbmax;

            // -8: x y z
            // -7: X y z
            // -6: x Y z
            // -5: x y Z
            // -4: X Y z
            // -3: X y Z
            // -2: x Y Z
            // -1: X Y Z

            objData += std::format(
                "v {} {} {}\nv {} {} {}\nv {} {} {}\nv {} {} {}\nv {} {} {}\nv {} {} {}\nv {} {} {}\nv {} {} {}\n",
                bbmin->x, bbmin->y, bbmin->z,
                bbmax->x, bbmin->y, bbmin->z,
                bbmin->x, bbmax->y, bbmin->z,
                bbmin->x, bbmin->y, bbmax->z,
                bbmax->x, bbmax->y, bbmin->z,
                bbmax->x, bbmin->y, bbmax->z,
                bbmin->x, bbmax->y, bbmax->z,
                bbmax->x, bbmax->y, bbmax->z
            );

            objData += "f -8 -7 -4 -6\n"
                "f -6 -4 -1 -2\n"
                "f -7 -3 -1 -4\n"
                "f -5 -8 -6 -2\n"
                "f -7 -8 -5 -3\n"
                "f -3 -5 -2 -1\n\n";
        }
    }

    StreamIO hitboxesOut(exportPath.replace_extension(".hitboxes.obj"), eStreamIOMode::Write);

    hitboxesOut.write(objData.c_str(), objData.length());
    hitboxesOut.close();

    return true;
}

static const char* const s_PathPrefixMDL = s_AssetTypePaths.find(AssetType_t::MDL_)->second;
bool ExportModelAsset(CAsset* const asset, const int setting)
{
    CPakAsset* const pakAsset = static_cast<CPakAsset*>(asset);
    assertm(pakAsset, "Asset should be valid.");

    const ModelAsset* const modelAsset = reinterpret_cast<ModelAsset*>(pakAsset->extraData());

    if (!modelAsset)
        return false;

    std::unique_ptr<char[]> streamedData = pakAsset->getStarPakData(modelAsset->vertexStreamingData.offset, modelAsset->vertexStreamingData.size, false);

    assertm(modelAsset->name, "No name for model.");

    // Create exported path + asset path.
    std::filesystem::path exportPath = g_rsxSettings.GetExportDirectory();
    const std::filesystem::path modelPath(modelAsset->name);
    const std::string modelStem(modelPath.stem().string());

    // truncate paths?
    if (g_rsxSettings.exportPathsFull)
        exportPath.append(modelPath.parent_path().string());
    else
        exportPath.append(std::format("{}/{}", s_PathPrefixMDL, modelStem));

    if (!CreateDirectories(exportPath))
    {
        assertm(false, "Failed to create asset directory.");
        return false;
    }

    const ModelParsedData_t* const parsedData = &modelAsset->parsedData;

    if (g_rsxSettings.exportRigSequences && modelAsset->numAnimSeqs > 0)
    {
        if (!ExportAnimSeqFromAsset(exportPath, modelStem, modelAsset->name, modelAsset->numAnimSeqs, modelAsset->animSeqs, modelAsset->GetRig()))
            return false;
    }

    if (g_rsxSettings.exportRigSequences && parsedData->LocalSeqCount() > 0)
    {
        std::filesystem::path outputPath(exportPath);
        outputPath.append(std::format("anims_{}/temp", modelStem));

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

            ExportSeqDesc(aseqAssetBinding->second.e.exportSetting, seqdesc, outputPath, modelAsset->name, modelAsset->GetRig(), RTech::StringToGuid(seqdesc->szlabel));
        }
    }

    exportPath.append(std::format("{}.rmdl", modelStem));

    switch (setting)
    {
    case eModelExportSetting::MODEL_CAST:
    case eModelExportSetting::MODEL_RMAX:
    case eModelExportSetting::MODEL_SMD:
    {
        return ExportModelMeshes(parsedData, exportPath, setting, 54);
    }
    case eModelExportSetting::MODEL_RMDL:
    {
        return ExportRawModelAsset(modelAsset, exportPath, streamedData.get());
    }
    case eModelExportSetting::MODEL_STL_VALVE_PHYSICS:
    {
        if (modelAsset->version >= eMDLVersion::VERSION_16)
            return ExportPhysicsModelPhy<irps::phyheader_v16_t>(modelAsset, exportPath);
        else
            return ExportPhysicsModelPhy<irps::phyheader_t>(modelAsset, exportPath);
    }
    case eModelExportSetting::MODEL_STL_RESPAWN_PHYSICS:
    {
        // [amos]: the high detail bvh4 mesh seems encased in a mesh that is
        // more or less identical to the vphysics one. The polygon winding
        // order of the vphysics replica is however always inverted.
        if (modelAsset->version >= eMDLVersion::VERSION_12_1)
            return ExportPhysicsModelBVH<r5::mstudiocollmodel_v8_t, r5::mstudiocollheader_v12_t>(modelAsset, exportPath);
        else
            return ExportPhysicsModelBVH<r5::mstudiocollmodel_v8_t, r5::mstudiocollheader_v8_t>(modelAsset, exportPath);
    }
    case eModelExportSetting::MODEL_HITBOXES:
    {
        return ExportModelHitboxes(modelAsset, exportPath);
    }
    default:
    {
        assertm(false, "Export setting is not handled.");
        return false;
    }
    }

    unreachable();
}

void InitModelAssetType()
{
    AssetTypeBinding_t type =
    {
        .name = "Model",
        .type = '_ldm',
        .headerAlignment = 8,
        .loadFunc = LoadModelAsset,
        .postLoadFunc = PostLoadModelAsset,
        .previewFunc = PreviewModelAsset,
        .e = { ExportModelAsset, 0, s_ModelExportSettingNames, ARRSIZE(s_ModelExportSettingNames) },
    };

    REGISTER_TYPE(type);

    //g_rsxSettings.assetSettings[type.type][RSXSettings_RMDL_e::SET_EXPORT_SEQUENCES] = UISetting_t("ExportSequences=%i", "Export associated sequences", true);
}

// extra stuff
const eMDLVersion GetModelVersionFromAsset(CPakFile* const pak, const void* const studioBuffer, const int version, const int headerStructSize)
{
    eMDLVersion out = eMDLVersion::VERSION_UNK;

    if (s_mdlVersionFromPak.count(version) == 1)
        out = s_mdlVersionFromPak.at(version);

    // pointer to the studiohdr is always the first entry in ModelAssetHeader regardless of versions (if this changes it won't affect this anyway)
    // so get that pointer for our studiohdr pointer, probably a better way to snag this but if it works it works
    const int* const pMDL = reinterpret_cast<const int* const>(studioBuffer);

    switch (out)
    {
    case eMDLVersion::VERSION_12:
    {
        // [rika]: love to see it
        // each of these index to the position of sourceFilenameOffset, we check what value it has (should point to end of header) to see which iteration it is, and then verify the asset header's size is correct
        if ((pMDL[97] == sizeof(r5::studiohdr_v8_t) || pMDL[41] == sizeof(r5::studiohdr_v8_t)) && headerStructSize == sizeof(ModelAssetHeader_v9_t))
            return eMDLVersion::VERSION_12;

        if ((pMDL[101] == sizeof(r5::studiohdr_v12_1_t) || pMDL[41] == sizeof(r5::studiohdr_v8_t)) && headerStructSize == sizeof(ModelAssetHeader_v12_1_t))
            return eMDLVersion::VERSION_12_1;

        if ((pMDL[102] == sizeof(r5::studiohdr_v12_2_t) || pMDL[41] == sizeof(r5::studiohdr_v12_2_t)) && headerStructSize == sizeof(ModelAssetHeader_v12_1_t))
            return eMDLVersion::VERSION_12_2;

        if ((pMDL[102] == sizeof(r5::studiohdr_v12_4_t) || pMDL[41] == sizeof(r5::studiohdr_v12_4_t)) && headerStructSize == sizeof(ModelAssetHeader_v12_1_t))
            return eMDLVersion::VERSION_12_4;

        if ((pMDL[102] == sizeof(r5::studiohdr_v12_5_t) || pMDL[41] == sizeof(r5::studiohdr_v12_5_t)) && headerStructSize == sizeof(ModelAssetHeader_v12_1_t))
            return eMDLVersion::VERSION_12_5;

        return eMDLVersion::VERSION_UNK;
    }
    case eMDLVersion::VERSION_13:
    {
        const r5::studiohdr_v12_5_t* const pHdr = reinterpret_cast<const r5::studiohdr_v12_5_t* const>(pMDL);

        if (pHdr->numbodyparts == 0)
            return out;

        const mstudiobodyparts_t* const pBodypart0 = pHdr->pBodypart(0);
        const r5::mstudiomodel_v12_1_t* const pModel = pBodypart0->pModel<r5::mstudiomodel_v12_1_t>(0);

        if (pModel->meshindex == 0)
        {
            assertm(false, "could not properly check version");
            return out;
        }

        // get the start and end point for mstudiomodel_t structs
        const int modelStart = static_cast<int>(reinterpret_cast<const char*>(pModel) - reinterpret_cast<const char*>(pMDL));
        const int modelEnd = modelStart + pModel->meshindex;
        const int modelSize = modelEnd - modelStart;
        int modelCount = 0;

        for (int i = 0; i < pHdr->numbodyparts; i++)
            modelCount += pHdr->pBodypart(i)->nummodels;

        const int modelSizeSingle = modelSize / modelCount;

        if (modelSizeSingle == static_cast<int>(sizeof(r5::mstudiomodel_v13_1_t)))
            return eMDLVersion::VERSION_13_1;

        return out;
    }
    case eMDLVersion::VERSION_14:
    {
        const r5::studiohdr_v14_t* const pHdr = reinterpret_cast<const r5::studiohdr_v14_t* const>(pMDL);

        if (pHdr->numlocalnodes == 0)
        {
            return out;
        }

        const int index = *reinterpret_cast<const int* const>((char*)pHdr + pHdr->localnodenameindex);

        if (index + pHdr->localnodenameindex < pHdr->length)
        {
            return eMDLVersion::VERSION_14_1;
        }

        return out;
    }
    case eMDLVersion::VERSION_19:
    {
        if (pak->header()->createdTime >= s_MdlTimeStamp_V19_3)
            return eMDLVersion::VERSION_19_3;

        const r5::studiohdr_v19_2_t* const pHdr = reinterpret_cast<const r5::studiohdr_v19_2_t* const>(pMDL);
        if (pHdr->sourceFilenameOffset == sizeof(r5::studiohdr_v19_2_t))
            return eMDLVersion::VERSION_19_2;

        if (pak->header()->createdTime >= s_MdlTimeStamp_V19_1)
            return eMDLVersion::VERSION_19_1;

        return out;
    }
    default:
    {
        return out;
    }
    }
}

void ParseExternalSequences(ModelParsedData_t* const parsedData, const uint32_t numAnimSeqs, AssetGuid_t* const animSeqs)
{
    if (numAnimSeqs == 0)
    {
        return;
    }

    parsedData->numExternalSequences = numAnimSeqs;
    parsedData->externalSequences = animSeqs;

    const uint64_t* guids = reinterpret_cast<const uint64_t*>(animSeqs);

    for (uint16_t seqIdx = 0; seqIdx < numAnimSeqs; seqIdx++)
    {
        const uint64_t guid = guids[seqIdx];

        CPakAsset* const animSeqAsset = g_assetData.FindAssetByGUID<CPakAsset>(guid);

        if (nullptr == animSeqAsset)
            continue;

        if (!animSeqAsset->hasExtraData())
            continue;

        AnimSeqAsset* const animSeq = reinterpret_cast<AnimSeqAsset* const>(animSeqAsset->extraData());

        if (nullptr == animSeq)
            continue;

        if (animSeq->rig)
            continue;

        animSeq->rig = parsedData;
    }
}