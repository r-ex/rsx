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

static void ParseModelVertexData_v8(CPakAsset* const asset, ModelAsset* const modelAsset)
{
    UNUSED(asset);

    if (!modelAsset->vertexComponentData)
    {
        Log("%s loaded with no vertex data\n", modelAsset->name);
        return;
    }

    r5::studiohdr_v8_t* const pStudioHdr = reinterpret_cast<r5::studiohdr_v8_t*>(modelAsset->data);
    const OptimizedModel::FileHeader_t* const pVTX = modelAsset->GetVTX();
    const vvd::vertexFileHeader_t* const pVVD = modelAsset->GetVVD();
    const vvc::vertexColorFileHeader_t* const pVVC = modelAsset->GetVVC();
    const vvw::vertexBoneWeightsExtraFileHeader_t* const pVVW = modelAsset->GetVVW();

    // no valid vertex data
    if (!pVTX || !pVVD)
        return;

    assertm(pVTX->version == OPTIMIZED_MODEL_FILE_VERSION, "invalid vtx version");
    assertm(pVVD->id == MODEL_VERTEX_FILE_ID, "invalid vvd file");

    if (pVTX->version != OPTIMIZED_MODEL_FILE_VERSION)
        return;

    if (pVVD->id != MODEL_VERTEX_FILE_ID)
        return;

    if (pVVC && (pVVC->id != MODEL_VERTEX_COLOR_FILE_ID))
        return;

    ModelParsedData_t* const parsedData = modelAsset->GetParsedData();

    parsedData->lods.resize(pVTX->numLODs);
    parsedData->bodyParts.resize(pStudioHdr->numbodyparts);

    constexpr size_t maxVertexDataSize = sizeof(vvd::mstudiovertex_t) + sizeof(Vector4D) + sizeof(Vector2D) + sizeof(Color32);
    constexpr size_t maxVertexBufferSize = maxVertexDataSize * s_MaxStudioVerts;

    // needed due to how vtx is parsed!
    CManagedBuffer* const   parseBuf = g_BufferManager.ClaimBuffer();

    Vertex_t* const         parseVertices   = reinterpret_cast<Vertex_t*>       (parseBuf->Buffer() + maxVertexBufferSize);
    Vector2D* const         parseTexcoords  = reinterpret_cast<Vector2D*>       (&parseVertices[s_MaxStudioVerts]);
    uint16_t* const         parseIndices    = reinterpret_cast<uint16_t*>       (&parseTexcoords[s_MaxStudioVerts * 2]);
    VertexWeight_t* const   parseWeights    = reinterpret_cast<VertexWeight_t*> (&parseIndices[s_MaxStudioTriIndices]); // ~8mb for weights

    for (int lodIdx = 0; lodIdx < pVTX->numLODs; lodIdx++)
    {
        int lodMeshCount = 0;

        ModelLODData_t& lodData = parsedData->lods.at(lodIdx);

        for (int bdyIdx = 0; bdyIdx < pStudioHdr->numbodyparts; bdyIdx++)
        {
            const mstudiobodyparts_t* const pStudioBodyPart = pStudioHdr->pBodypart(bdyIdx);
            const OptimizedModel::BodyPartHeader_t* const pVertBodyPart = pVTX->pBodyPart(bdyIdx);

            parsedData->SetupBodyPart(bdyIdx, pStudioBodyPart->pszName(), static_cast<int>(lodData.models.size()), pStudioBodyPart->nummodels);

            for (int modelIdx = 0; modelIdx < pStudioBodyPart->nummodels; modelIdx++)
            {
                const r5::mstudiomodel_v8_t* const pStudioModel = pStudioBodyPart->pModel<r5::mstudiomodel_v8_t>(modelIdx);
                const OptimizedModel::ModelHeader_t* const pVertModel = pVertBodyPart->pModel(modelIdx);

                const OptimizedModel::ModelLODHeader_t* const pVertLOD = pVertModel->pLOD(lodIdx);
                lodData.switchPoint = pVertLOD->switchPoint;
                lodData.meshes.resize(lodMeshCount + pVertLOD->numMeshes);

                ModelModelData_t modelData = {};

                modelData.name = std::format("{}_{}", pStudioBodyPart->pszName(), std::to_string(modelIdx));
                modelData.meshIndex = lodMeshCount;

                for (int meshIdx = 0; meshIdx < pStudioModel->nummeshes; ++meshIdx)
                {
                    const r5::mstudiomesh_v8_t* const pStudioMesh = pStudioModel->pMesh(meshIdx);
                    const OptimizedModel::MeshHeader_t* const pVertMesh = pVertLOD->pMesh(meshIdx);

                    const int baseVertexOffset = (pStudioModel->vertexindex / sizeof(vvd::mstudiovertex_t)) + pStudioMesh->vertexoffset;
                    const int studioVertCount = pStudioMesh->vertexloddata.numLODVertexes[lodIdx];

                    if (pVertMesh->numStripGroups == 0)
                        continue;

                    vvd::mstudiovertex_t* verts = reinterpret_cast<vvd::mstudiovertex_t*>(parseBuf->Buffer());
                    Vector4D* tangs = reinterpret_cast<Vector4D*>(&verts[studioVertCount]);
                    Color32* colors = reinterpret_cast<Color32*>(&tangs[studioVertCount]);
                    Vector2D* uv2s = reinterpret_cast<Vector2D*>(&colors[studioVertCount]);

                    pVVD->PerLODVertexBuffer(lodIdx, verts, tangs, baseVertexOffset, baseVertexOffset + studioVertCount);

                    if (pVVC)
                        pVVC->PerLODVertexBuffer(lodIdx, pVVD->numFixups, pVVD->GetFixupData(0), colors, uv2s, baseVertexOffset, baseVertexOffset + studioVertCount);

                    // reserve a buffer
                    CManagedBuffer* const buffer = g_BufferManager.ClaimBuffer();

                    CMeshData* meshVertexData = reinterpret_cast<CMeshData*>(buffer->Buffer());
                    meshVertexData->InitWriter();

                    ModelMeshData_t& meshData = lodData.meshes.at(lodMeshCount);

                    meshData.bodyPartIndex = bdyIdx;

                    // is this correct?
                    meshData.rawVertexLayoutFlags |= (VERT_LEGACY | (pStudioHdr->flags & STUDIOHDR_FLAGS_USES_VERTEX_COLOR ? VERT_COLOR : 0x0));
                    meshData.vertCacheSize = static_cast<uint16_t>(pVTX->vertCacheSize);

                    // do we have a section texcoord
                    meshData.rawVertexLayoutFlags |= pStudioHdr->flags & STUDIOHDR_FLAGS_USES_UV2 ? VERT_TEXCOORDn_FMT(2, 0x2) : 0x0;

                    meshData.ParseTexcoords();

                    // parsing more than one is unfun and not a single model from respawn has two
                    int weightIdx = 0;
                    assertm(pVertMesh->numStripGroups == 1, "model had more than one strip group");
                    for (int stripGrpIdx = 0; stripGrpIdx < 1; stripGrpIdx++)
                    {
                        OptimizedModel::StripGroupHeader_t* pStripGrp = pVertMesh->pStripGroup(stripGrpIdx);

                        meshData.vertCount += pStripGrp->numVerts;
                        lodData.vertexCount += pStripGrp->numVerts;

                        meshData.indexCount += pStripGrp->numIndices;
                        lodData.indexCount += pStripGrp->numIndices;

                        assertm(s_MaxStudioTriIndices >= meshData.indexCount, "too many triangles");

                        for (int stripIdx = 0; stripIdx < pStripGrp->numStrips; stripIdx++)
                        {
                            OptimizedModel::StripHeader_t* pStrip = pStripGrp->pStrip(stripIdx);

                            for (int vertIdx = 0; vertIdx < pStrip->numVerts; vertIdx++)
                            {
                                OptimizedModel::Vertex_t* pVert = pStripGrp->pVertex(pStrip->vertOffset + vertIdx);

                                Vector2D* const texcoords = meshData.texcoordCount > 1 ? &parseTexcoords[(pStrip->vertOffset + vertIdx) * (meshData.texcoordCount - 1)] : nullptr;
                                Vertex_t::ParseVertexFromVTX(&parseVertices[pStrip->vertOffset + vertIdx], &parseWeights[weightIdx], texcoords, &meshData, pVert, verts, tangs, colors, uv2s, pVVW, weightIdx);
                            }

                            memcpy(&parseIndices[pStrip->indexOffset], pStripGrp->pIndex(pStrip->indexOffset), pStrip->numIndices * sizeof(uint16_t));
                        }

                    }
                    meshData.weightsCount = weightIdx;

                    // add mesh data
                    meshVertexData->AddIndices(parseIndices, meshData.indexCount);
                    meshVertexData->AddVertices(parseVertices, meshData.vertCount);

                    if (meshData.texcoordCount > 1)
                        meshVertexData->AddTexcoords(parseTexcoords, meshData.vertCount * (meshData.texcoordCount - 1));

                    meshVertexData->AddWeights(parseWeights, meshData.weightsCount);

                    meshData.ParseMaterial(parsedData, pStudioMesh->material);

                    lodMeshCount++;
                    modelData.meshCount++;
                    modelData.vertCount += meshData.vertCount;

                    // for export
                    lodData.weightsPerVert = meshData.weightsPerVert > lodData.weightsPerVert ? meshData.weightsPerVert : lodData.weightsPerVert;
                    lodData.texcoordsPerVert = meshData.texcoordCount > lodData.texcoordsPerVert ? meshData.texcoordCount : lodData.texcoordsPerVert;

                    // remove it from usage
                    meshVertexData->DestroyWriter();

                    meshData.meshVertexDataIndex = parsedData->meshVertexData.size();
                    parsedData->meshVertexData.addBack(reinterpret_cast<char*>(meshVertexData), meshVertexData->GetSize());

                    // relieve buffer
                    g_BufferManager.RelieveBuffer(buffer);
                }

                lodData.models.push_back(modelData);
            }
        }

        // [rika]: to remove excess meshes (empty meshes we skipped, since we set size at the beginning). this should only deallocate memory
        lodData.meshes.resize(lodMeshCount);

        // fixup our model pointers
        int curIdx = 0;
        for (auto& model : lodData.models)
        {
            model.meshes = model.meshCount> 0 ? &lodData.meshes.at(curIdx) : nullptr;

            curIdx += model.meshCount;
        }
    }

    g_BufferManager.RelieveBuffer(parseBuf);
}

const uint8_t s_VertexDataBaseBoneMap[8] = { 0, 1, 2, 3, 4, 5, 6, 7 };
const uint16_t s_VertexDataBaseBoneMapButWide[8] = { 0, 1, 2, 3, 4, 5, 6, 7 };

static void ParseModelVertexData_v9(CPakAsset* const asset, ModelAsset* const modelAsset)
{
    const std::unique_ptr<char[]> pStreamed = modelAsset->vertexStreamingData.size > 0 ? asset->getStarPakData(modelAsset->vertexStreamingData.offset, modelAsset->vertexStreamingData.size, false) : nullptr; // probably smarter to check the size inside getStarPakData but whatever!
    char* const pDataBuffer = pStreamed.get() ? pStreamed.get() : modelAsset->staticStreamingData;

    if (!pDataBuffer)
    {
        Log("%s loaded with no vertex data\n", modelAsset->name);
        return;
    }

    const vg::rev1::VertexGroupHeader_t* const vgHdr = reinterpret_cast<const vg::rev1::VertexGroupHeader_t*>(pDataBuffer);

    assertm(vgHdr->id == 'GVt0', "hwData id was invalid");

    if (vgHdr->lodCount == 0)
        return;

    ModelParsedData_t* const parsedData = modelAsset->GetParsedData();

    parsedData->studiohdr.hwDataSize = vgHdr->dataSize; // [rika]: set here, makes things easier. if we use the value from ModelAssetHeader it will be aligned 4096, making it slightly oversized.
    parsedData->lods.resize(vgHdr->lodCount);

    // group setup
    {
        studio_hw_groupdata_t& group = parsedData->studiohdr.groups[0];

        group.dataOffset = 0;
        group.dataSizeCompressed = -1;
        group.dataSizeDecompressed = vgHdr->dataSize;
        group.dataCompression = eCompressionType::NONE;

        group.lodIndex = 0;
        group.lodCount = static_cast<uint8_t>(vgHdr->lodCount);
        group.lodMap = 0xff >> (8 - vgHdr->lodCount);

    }

    const r5::studiohdr_v8_t* const pStudioHdr = reinterpret_cast<r5::studiohdr_v8_t*>(modelAsset->data);

    parsedData->bodyParts.resize(pStudioHdr->numbodyparts);
    parsedData->meshVertexData.resize(vgHdr->meshCount);

    const uint8_t* boneMap = vgHdr->boneStateChangeCount ? vgHdr->pBoneMap() : s_VertexDataBaseBoneMap; // does this model have remapped bones? use default map if not

    const uint8_t vertexWeightParseFlags = (pStudioHdr->flags & STUDIOHDR_FLAGS_USES_EXTRA_BONE_WEIGHTS) ? VERT_PARSE_EXTRAWEIGHT : 0x0;

    for (int lodLevel = 0; lodLevel < vgHdr->lodCount; lodLevel++)
    {
        int lodMeshCount = 0;

        ModelLODData_t& lodData = parsedData->lods.at(lodLevel);
        const vg::rev1::ModelLODHeader_t* const lod = vgHdr->pLOD(lodLevel);

        lodData.switchPoint = lod->switchPoint;
        lodData.meshes.resize(lod->meshCount);

        for (int bdyIdx = 0; bdyIdx < pStudioHdr->numbodyparts; bdyIdx++)
        {
            const mstudiobodyparts_t* const pBodypart = pStudioHdr->pBodypart(bdyIdx);

            parsedData->SetupBodyPart(bdyIdx, pBodypart->pszName(), static_cast<int>(lodData.models.size()), pBodypart->nummodels);

            for (int modelIdx = 0; modelIdx < pBodypart->nummodels; modelIdx++)
            {
                const r5::mstudiomodel_v8_t* const pModel = pBodypart->pModel<r5::mstudiomodel_v8_t>(modelIdx);
                ModelModelData_t modelData = {};

                modelData.name = std::format("{}_{}", pBodypart->pszName(), std::to_string(modelIdx));
                modelData.meshIndex = lodMeshCount;

                // because we resize, having a pointer to the element in the container is fine.
                modelData.meshes = pModel->nummeshes > 0 ? &lodData.meshes.at(lodMeshCount) : nullptr;
                
                for (int meshIdx = 0; meshIdx < pModel->nummeshes; ++meshIdx)
                {
                    const r5::mstudiomesh_v8_t* const pMesh = pModel->pMesh(meshIdx);
                    const vg::rev1::MeshHeader_t* const mesh = lod->pMesh(vgHdr, pMesh->meshid);

                    if (mesh->flags == 0 || mesh->stripCount == 0)
                        continue;

                    // reserve a buffer
                    CManagedBuffer* const buffer = g_BufferManager.ClaimBuffer();

                    CMeshData* meshVertexData = reinterpret_cast<CMeshData*>(buffer->Buffer());
                    meshVertexData->InitWriter();

                    ModelMeshData_t& meshData = lodData.meshes.at(lodMeshCount);

                    meshData.rawVertexLayoutFlags |= mesh->flags;

                    meshData.vertCacheSize = static_cast<uint16_t>(mesh->vertCacheSize);
                    meshData.vertCount = mesh->vertCount;
                    meshData.indexCount = mesh->indexCount;

                    meshData.bodyPartIndex = bdyIdx;

                    if (mesh->extraBoneWeightSize)
                    {
                        char* ebw = new char[mesh->extraBoneWeightSize];
                        memcpy_s(ebw, mesh->extraBoneWeightSize, mesh->pBoneWeight(vgHdr), mesh->extraBoneWeightSize);

                        meshData.extraBoneWeights = ebw;
                        meshData.extraBoneWeightsSize = mesh->extraBoneWeightSize;
                    }
                    else
                    {
                        meshData.extraBoneWeights = nullptr;
                        meshData.extraBoneWeightsSize = 0;
                    }

                    lodData.vertexCount += mesh->vertCount;
                    lodData.indexCount += mesh->indexCount;

                    meshData.ParseTexcoords();

                    const char* const rawVertexData = mesh->pVertices(vgHdr);// pointer to all of the vertex data for this mesh
                    const vvw::mstudioboneweightextra_t* const weights = mesh->pBoneWeight(vgHdr);
                    const uint16_t* const meshIndexData = mesh->pIndices(vgHdr); // pointer to all of the index data for this mesh

#if 0//defined(ADVANCED_MODEL_PREVIEW)
                    meshData.rawVertexData = new char[mesh->vertCacheSize * mesh->vertCount]; // get a pointer to the raw vertex data for use with the game's shaders

                    memcpy(meshData.rawVertexData, rawVertexData, static_cast<uint64_t>(mesh->vertCacheSize) * mesh->vertCount);
#endif
                  
                    meshVertexData->AddIndices(meshIndexData, meshData.indexCount);
                    meshVertexData->AddVertices(nullptr, meshData.vertCount);
                    
                    if (meshData.texcoordCount > 1)
                        meshVertexData->AddTexcoords(nullptr, meshData.vertCount * (meshData.texcoordCount - 1));

                    meshVertexData->AddWeights(nullptr, 0);

                    int weightIdx = 0;
                    for (unsigned int vertIdx = 0; vertIdx < mesh->vertCount; ++vertIdx)
                    {
                        const char* const vertexData = rawVertexData + (vertIdx * mesh->vertCacheSize);
                        Vector2D* const texcoords = meshData.texcoordCount > 1 ? &meshVertexData->GetTexcoords()[vertIdx * (meshData.texcoordCount - 1)] : nullptr;
                        Vertex_t::ParseVertexFromVG(&meshVertexData->GetVertices()[vertIdx], &meshVertexData->GetWeights()[weightIdx], texcoords, &meshData, vertexData, boneMap, weights, vertexWeightParseFlags, weightIdx);
                    }
                    meshData.weightsCount = weightIdx;
                    meshVertexData->AddWeights(nullptr, meshData.weightsCount);

                    meshData.ParseMaterial(parsedData, pMesh->material);

                    lodMeshCount++;
                    modelData.meshCount++;
                    modelData.vertCount += meshData.vertCount;

                    // for export
                    lodData.weightsPerVert = meshData.weightsPerVert > lodData.weightsPerVert ? meshData.weightsPerVert : lodData.weightsPerVert;
                    lodData.texcoordsPerVert = meshData.texcoordCount > lodData.texcoordsPerVert ? meshData.texcoordCount : lodData.texcoordsPerVert;

                    // remove it from usage
                    meshVertexData->DestroyWriter();

                    meshData.meshVertexDataIndex = parsedData->meshVertexData.size();
                    parsedData->meshVertexData.addBack(reinterpret_cast<char*>(meshVertexData), meshVertexData->GetSize());

                    // relieve buffer
                    g_BufferManager.RelieveBuffer(buffer);
                }

                lodData.models.push_back(modelData);
            }
        }

        // [rika]: to remove excess meshes (empty meshes we skipped, since we set size at the beginning). this should only deallocate memory
        lodData.meshes.resize(lodMeshCount);
    }

    parsedData->meshVertexData.shrink();
}

static void ParseModelVertexData_v12_1(CPakAsset* const asset, ModelAsset* const modelAsset)
{
    const std::unique_ptr<char[]> pStreamed = modelAsset->vertexStreamingData.size > 0 ? asset->getStarPakData(modelAsset->vertexStreamingData.offset, modelAsset->vertexStreamingData.size, false) : nullptr; // probably smarter to check the size inside getStarPakData but whatever!
    char* const pDataBuffer = pStreamed.get() ? pStreamed.get() : modelAsset->staticStreamingData;

    if (!pDataBuffer)
    {
        Log("%s loaded with no vertex data\n", modelAsset->name);
        return;
    }

    ModelParsedData_t* const parsedData = modelAsset->GetParsedData();

    const r5::studiohdr_v12_1_t* const pStudioHdr = reinterpret_cast<r5::studiohdr_v12_1_t*>(modelAsset->data);

    const uint8_t* boneMap = pStudioHdr->boneStateCount ? pStudioHdr->pBoneStates() : s_VertexDataBaseBoneMap; // does this model have remapped bones? use default map if not

    const uint8_t vertexWeightParseFlags = (pStudioHdr->flags & STUDIOHDR_FLAGS_USES_EXTRA_BONE_WEIGHTS) ? VERT_PARSE_EXTRAWEIGHT : 0x0;

    parsedData->lods.resize(pStudioHdr->lodCount);
    parsedData->bodyParts.resize(pStudioHdr->numbodyparts);

    uint16_t lodMeshCount[8]{ 0 };

    for (uint16_t groupIdx = 0; groupIdx < pStudioHdr->groupHeaderCount; groupIdx++)
    {
        const r5::studio_hw_groupdata_v12_1_t* group = pStudioHdr->pLODGroup(groupIdx);

        const vg::rev2::VertexGroupHeader_t* grouphdr = reinterpret_cast<const vg::rev2::VertexGroupHeader_t*>(pDataBuffer + group->dataOffset);

        uint8_t lodIdx = 0;
        for (uint16_t lodLevel = 0; lodLevel < pStudioHdr->lodCount; lodLevel++)
        {
            if (lodIdx == grouphdr->lodCount)
                break;

            // does this group contian this lod
            if (!(grouphdr->lodMap & (1 << lodLevel)))
                continue;

            assert(static_cast<uint8_t>(lodIdx) < grouphdr->lodCount);

            const vg::rev2::ModelLODHeader_t* lod = grouphdr->pLod(lodIdx);
            ModelLODData_t& lodData = parsedData->lods.at(lodLevel);
            lodData.switchPoint = lod->switchPoint;

            parsedData->meshVertexData.resize(parsedData->meshVertexData.size() + lod->meshCount);

            // [rika]: this should only get hit once per LOD
            const size_t curMeshCount = lodData.meshes.size();
            lodData.meshes.resize(curMeshCount + lod->meshCount);

            for (uint16_t bdyIdx = 0; bdyIdx < pStudioHdr->numbodyparts; bdyIdx++)
            {
                const mstudiobodyparts_t* const pBodypart = pStudioHdr->pBodypart(bdyIdx);

                parsedData->SetupBodyPart(bdyIdx, pBodypart->pszName(), static_cast<int>(lodData.models.size()), pBodypart->nummodels);

                for (int modelIdx = 0; modelIdx < pBodypart->nummodels; modelIdx++)
                {
                    // studio model changed in v12.3
                    const r5::mstudiomodel_v12_1_t* pModel = modelAsset->version < eMDLVersion::VERSION_13_1 ? pBodypart->pModel<r5::mstudiomodel_v12_1_t>(modelIdx) : pBodypart->pModel<r5::mstudiomodel_v13_1_t>(modelIdx)->AsV12_1();

                    ModelModelData_t modelData = {};

                    modelData.name = std::format("{}_{}", pBodypart->pszName(), std::to_string(modelIdx));
                    modelData.meshIndex = static_cast<size_t>(lodMeshCount[lodLevel]);

                    // because we resize, having a pointer to the element in the container is fine.
                    modelData.meshes = pModel->nummeshes > 0 ? &lodData.meshes.at(lodMeshCount[lodLevel]) : nullptr;

                    for (int meshIdx = 0; meshIdx < pModel->nummeshes; ++meshIdx)
                    {
                        const r5::mstudiomesh_v12_1_t* const pMesh = pModel->pMesh(meshIdx);

                        const vg::rev2::MeshHeader_t* const mesh = lod->pMesh(pMesh->meshid);

                        if (mesh->flags == 0)
                            continue;

                        // reserve a buffer
                        CManagedBuffer* const buffer = g_BufferManager.ClaimBuffer();

                        CMeshData* meshVertexData = reinterpret_cast<CMeshData*>(buffer->Buffer());
                        meshVertexData->InitWriter();

                        ModelMeshData_t& meshData = lodData.meshes.at(lodMeshCount[lodLevel]);

                        meshData.rawVertexLayoutFlags |= mesh->flags;

                        meshData.vertCacheSize = static_cast<uint16_t>(mesh->vertCacheSize);
                        meshData.vertCount = mesh->vertCount;
                        meshData.indexCount = static_cast<uint32_t>(mesh->indexCount);

                        meshData.bodyPartIndex = bdyIdx;

                        if (mesh->extraBoneWeightSize)
                        {
                            char* ebw = new char[mesh->extraBoneWeightSize];
                            memcpy_s(ebw, mesh->extraBoneWeightSize, mesh->pBoneWeights(), mesh->extraBoneWeightSize);

                            meshData.extraBoneWeights = ebw;
                            meshData.extraBoneWeightsSize = mesh->extraBoneWeightSize;
                        }
                        else
                        {
                            meshData.extraBoneWeights = nullptr;
                            meshData.extraBoneWeightsSize = 0;
                        }


                        lodData.vertexCount += mesh->vertCount;
                        lodData.indexCount += mesh->indexCount;

                        meshData.ParseTexcoords();

                        char* const rawVertexData = mesh->pVertices(); // pointer to all of the vertex data for this mesh
                        const vvw::mstudioboneweightextra_t* const weights = mesh->pBoneWeights();
                        const uint16_t* const meshIndexData = mesh->pIndices(); // pointer to all of the index data for this mesh

#if (ADVANCED_MODEL_PREVIEW)
                        meshData.rawVertexData = new char[mesh->vertCacheSize * mesh->vertCount]; // get a pointer to the raw vertex data for use with the game's shaders

                        memcpy(meshData.rawVertexData, rawVertexData, static_cast<uint64_t>(mesh->vertCacheSize) * mesh->vertCount);
#endif

                        meshVertexData->AddIndices(meshIndexData, meshData.indexCount);
                        meshVertexData->AddVertices(nullptr, meshData.vertCount);

                        if (meshData.texcoordCount > 1)
                            meshVertexData->AddTexcoords(nullptr, meshData.vertCount * (meshData.texcoordCount - 1));

                        meshVertexData->AddWeights(nullptr, 0);

                        int weightIdx = 0;
                        for (int vertIdx = 0; vertIdx < mesh->vertCount; ++vertIdx)
                        {
                            char* const vertexData = rawVertexData + (vertIdx * mesh->vertCacheSize);
                            Vector2D* const texcoords = meshData.texcoordCount > 1 ? &meshVertexData->GetTexcoords()[vertIdx * (meshData.texcoordCount - 1)] : nullptr;
                            Vertex_t::ParseVertexFromVG(&meshVertexData->GetVertices()[vertIdx], &meshVertexData->GetWeights()[weightIdx], texcoords, &meshData, vertexData, boneMap, weights, vertexWeightParseFlags, weightIdx);
                        }
                        meshData.weightsCount = weightIdx;
                        meshVertexData->AddWeights(nullptr, meshData.weightsCount);

                        meshData.ParseMaterial(parsedData, pMesh->material);

                        lodMeshCount[lodLevel]++;
                        modelData.meshCount++;
                        modelData.vertCount += meshData.vertCount;

                        // for export
                        lodData.weightsPerVert = meshData.weightsPerVert > lodData.weightsPerVert ? meshData.weightsPerVert : lodData.weightsPerVert;
                        lodData.texcoordsPerVert = meshData.texcoordCount > lodData.texcoordsPerVert ? meshData.texcoordCount : lodData.texcoordsPerVert;

                        // remove it from usage
                        meshVertexData->DestroyWriter();

                        meshData.meshVertexDataIndex = parsedData->meshVertexData.size();
                        parsedData->meshVertexData.addBack(reinterpret_cast<char*>(meshVertexData), meshVertexData->GetSize());

                        // relieve buffer
                        g_BufferManager.RelieveBuffer(buffer);
                    }

                    lodData.models.push_back(modelData);
                }
            }

            lodIdx++;

            // [rika]: to remove excess meshes (empty meshes we skipped, since we set size at the beginning). this should only deallocate memory
            // [rika]: this should only be hit once per LOD level, since it's either all levels in one group, or a level per group.
            lodData.meshes.resize(lodMeshCount[lodLevel]);
        }
    }

    parsedData->meshVertexData.shrink();
}

static void ParseModelVertexData_v14(CPakAsset* const asset, ModelAsset* const modelAsset)
{
    const std::unique_ptr<char[]> pStreamed = modelAsset->vertexStreamingData.size > 0 ? asset->getStarPakData(modelAsset->vertexStreamingData.offset, modelAsset->vertexStreamingData.size, false) : nullptr; // probably smarter to check the size inside getStarPakData but whatever!
    char* const pDataBuffer = pStreamed.get() ? pStreamed.get() : modelAsset->staticStreamingData;

    if (!pDataBuffer)
    {
        Log("%s loaded with no vertex data\n", modelAsset->name);
        return;
    }

    ModelParsedData_t* const parsedData = modelAsset->GetParsedData();

    const r5::studiohdr_v14_t* const pStudioHdr = reinterpret_cast<r5::studiohdr_v14_t*>(modelAsset->data);

    const uint8_t* boneMap = pStudioHdr->boneStateCount ? pStudioHdr->pBoneStates() : s_VertexDataBaseBoneMap; // does this model have remapped bones? use default map if not

    const uint8_t vertexWeightParseFlags = (pStudioHdr->flags & STUDIOHDR_FLAGS_USES_EXTRA_BONE_WEIGHTS) ? VERT_PARSE_EXTRAWEIGHT : 0x0;

    parsedData->lods.resize(pStudioHdr->lodCount);
    parsedData->bodyParts.resize(pStudioHdr->numbodyparts);

    uint16_t lodMeshCount[8]{ 0 };

    for (uint16_t groupIdx = 0; groupIdx < pStudioHdr->groupHeaderCount; groupIdx++)
    {
        const r5::studio_hw_groupdata_v12_1_t* group = pStudioHdr->pLODGroup(groupIdx);

        const vg::rev3::VertexGroupHeader_t* grouphdr = reinterpret_cast<const vg::rev3::VertexGroupHeader_t*>(pDataBuffer + group->dataOffset);

        uint8_t lodIdx = 0;
        for (uint16_t lodLevel = 0; lodLevel < pStudioHdr->lodCount; lodLevel++)
        {
            if (lodIdx == grouphdr->lodCount)
                break;

            // does this group contian this lod
            if (!(grouphdr->lodMap & (1 << lodLevel)))
                continue;

            assert(static_cast<uint8_t>(lodIdx) < grouphdr->lodCount);

            const vg::rev3::ModelLODHeader_t* lod = grouphdr->pLod(lodIdx);
            ModelLODData_t& lodData = parsedData->lods.at(lodLevel);
            lodData.switchPoint = lod->switchPoint;

            parsedData->meshVertexData.resize(parsedData->meshVertexData.size() + lod->meshCount);

            // [rika]: this should only get hit once per LOD
            const size_t curMeshCount = lodData.meshes.size();
            lodData.meshes.resize(curMeshCount + lod->meshCount);

            for (uint16_t bdyIdx = 0; bdyIdx < pStudioHdr->numbodyparts; bdyIdx++)
            {
                const mstudiobodyparts_t* const pBodypart = modelAsset->version == eMDLVersion::VERSION_15 ? pStudioHdr->pBodypart_V15(bdyIdx)->AsV8() : pStudioHdr->pBodypart(bdyIdx);

                parsedData->SetupBodyPart(bdyIdx, pBodypart->pszName(), static_cast<int>(lodData.models.size()), pBodypart->nummodels);

                for (int modelIdx = 0; modelIdx < pBodypart->nummodels; modelIdx++)
                {
                    const r5::mstudiomodel_v14_t* const pModel = pBodypart->pModel<r5::mstudiomodel_v14_t>(modelIdx);
                    ModelModelData_t modelData = {};

                    modelData.name = std::format("{}_{}", pBodypart->pszName(), std::to_string(modelIdx));
                    modelData.meshIndex = static_cast<size_t>(lodMeshCount[lodLevel]);

                    // because we resize, having a pointer to the element in the container is fine.
                    modelData.meshes = pModel->meshCountTotal > 0 ? &lodData.meshes.at(lodMeshCount[lodLevel]) : nullptr;

                    for (uint16_t meshIdx = 0; meshIdx < pModel->meshCountTotal; ++meshIdx)
                    {
                        // we do not handle blendstates currently
                        if (meshIdx == pModel->meshCountBase)
                            break;

                        const r5::mstudiomesh_v14_t* const pMesh = pModel->pMesh(meshIdx);
                        const vg::rev3::MeshHeader_t* const mesh = lod->pMesh(static_cast<uint8_t>(pMesh->meshid));

                        if (mesh->flags == 0)
                            continue;

                        // reserve a buffer
                        CManagedBuffer* const buffer = g_BufferManager.ClaimBuffer();

                        CMeshData* meshVertexData = reinterpret_cast<CMeshData*>(buffer->Buffer());
                        meshVertexData->InitWriter();

                        ModelMeshData_t& meshData = lodData.meshes.at(lodMeshCount[lodLevel]);

                        meshData.rawVertexLayoutFlags |= mesh->flags;

                        meshData.vertCacheSize = static_cast<uint16_t>(mesh->vertCacheSize);
                        meshData.vertCount = mesh->vertCount;
                        meshData.indexCount = static_cast<uint32_t>(mesh->indexCount);

                        meshData.bodyPartIndex = bdyIdx;

                        if (mesh->extraBoneWeightSize)
                        {
                            char* ebw = new char[mesh->extraBoneWeightSize];
                            memcpy_s(ebw, mesh->extraBoneWeightSize, mesh->pBoneWeights(), mesh->extraBoneWeightSize);

                            meshData.extraBoneWeights = ebw;
                            meshData.extraBoneWeightsSize = mesh->extraBoneWeightSize;
                        }
                        else
                        {
                            meshData.extraBoneWeights = nullptr;
                            meshData.extraBoneWeightsSize = 0;
                        }


                        lodData.vertexCount += mesh->vertCount;
                        lodData.indexCount += mesh->indexCount;

                        meshData.ParseTexcoords();

                        char* const rawVertexData = mesh->pVertices(); // pointer to all of the vertex data for this mesh
                        const vvw::mstudioboneweightextra_t* const weights = mesh->pBoneWeights();
                        const uint16_t* const meshIndexData = mesh->pIndices(); // pointer to all of the index data for this mesh

#if (ADVANCED_MODEL_PREVIEW)
                        meshData.rawVertexData = new char[mesh->vertCacheSize * mesh->vertCount]; // get a pointer to the raw vertex data for use with the game's shaders

                        memcpy(meshData.rawVertexData, rawVertexData, static_cast<uint64_t>(mesh->vertCacheSize) * mesh->vertCount);
#endif

                        meshVertexData->AddIndices(meshIndexData, meshData.indexCount);
                        meshVertexData->AddVertices(nullptr, meshData.vertCount);

                        if (meshData.texcoordCount > 1)
                            meshVertexData->AddTexcoords(nullptr, meshData.vertCount * (meshData.texcoordCount - 1));

                        meshVertexData->AddWeights(nullptr, 0);

                        int weightIdx = 0;
                        for (unsigned int vertIdx = 0; vertIdx < mesh->vertCount; ++vertIdx)
                        {
                            char* const vertexData = rawVertexData + (vertIdx * mesh->vertCacheSize);
                            Vector2D* const texcoords = meshData.texcoordCount > 1 ? &meshVertexData->GetTexcoords()[vertIdx * (meshData.texcoordCount - 1)] : nullptr;
                            Vertex_t::ParseVertexFromVG(&meshVertexData->GetVertices()[vertIdx], &meshVertexData->GetWeights()[weightIdx], texcoords, &meshData, vertexData, boneMap, weights, vertexWeightParseFlags, weightIdx);
                        }
                        meshData.weightsCount = weightIdx;
                        meshVertexData->AddWeights(nullptr, meshData.weightsCount);

                        meshData.ParseMaterial(parsedData, pMesh->material);

                        lodMeshCount[lodLevel]++;
                        modelData.meshCount++;
                        modelData.vertCount += meshData.vertCount;

                        // for export
                        lodData.weightsPerVert = meshData.weightsPerVert > lodData.weightsPerVert ? meshData.weightsPerVert : lodData.weightsPerVert;
                        lodData.texcoordsPerVert = meshData.texcoordCount > lodData.texcoordsPerVert ? meshData.texcoordCount : lodData.texcoordsPerVert;

                        // remove it from usage
                        meshVertexData->DestroyWriter();

                        meshData.meshVertexDataIndex = parsedData->meshVertexData.size();
                        parsedData->meshVertexData.addBack(reinterpret_cast<char*>(meshVertexData), meshVertexData->GetSize());

                        // relieve buffer
                        g_BufferManager.RelieveBuffer(buffer);
                    }

                    lodData.models.push_back(modelData);
                }
            }

            lodIdx++;

            // [rika]: to remove excess meshes (empty meshes we skipped, since we set size at the beginning). this should only deallocate memory
            // [rika]: this should only be hit once per LOD level, since it's either all levels in one group, or a level per group.
            lodData.meshes.resize(lodMeshCount[lodLevel]);
        }
    }

    parsedData->meshVertexData.shrink();
}

static void ParseModelVertexData_v16(CPakAsset* const asset, ModelAsset* const modelAsset)
{
    const std::unique_ptr<char[]> pStreamed = modelAsset->vertexStreamingData.size > 0 ? asset->getStarPakData(modelAsset->vertexStreamingData.offset, modelAsset->vertexStreamingData.size, false) : nullptr; // probably smarter to check the size inside getStarPakData but whatever!
    char* const pDataBuffer = pStreamed.get() ? pStreamed.get() : modelAsset->staticStreamingData;

    if (!pDataBuffer)
    {
        Log("%s loaded with no vertex data\n", modelAsset->name);
        return;
    }

    ModelParsedData_t* const parsedData = modelAsset->GetParsedData();

    const r5::studiohdr_v16_t* const pStudioHdr = reinterpret_cast<r5::studiohdr_v16_t*>(modelAsset->data);

    const uint8_t* boneMap = pStudioHdr->boneStateCount ? pStudioHdr->pBoneStates() : s_VertexDataBaseBoneMap; // does this model have remapped bones? use default map if not

    const uint8_t vertexWeightParseFlags = (pStudioHdr->flags & STUDIOHDR_FLAGS_USES_EXTRA_BONE_WEIGHTS) ? VERT_PARSE_EXTRAWEIGHT : 0x0;

    parsedData->lods.resize(pStudioHdr->lodCount);
    parsedData->bodyParts.resize(pStudioHdr->numbodyparts);

    uint16_t lodMeshCount[8]{ 0 };

    for (uint16_t groupIdx = 0; groupIdx < pStudioHdr->groupHeaderCount; groupIdx++)
    {
        const r5::studio_hw_groupdata_v16_t* group = pStudioHdr->pLODGroup(groupIdx);

        std::unique_ptr<char[]> dcmpBuf = nullptr;

        // decompress buffer
        switch (group->dataCompression)
        {
        case eCompressionType::NONE:
        {
            dcmpBuf = std::make_unique<char[]>(group->dataSizeDecompressed);
            std::memcpy(dcmpBuf.get(), pDataBuffer + group->dataOffset, group->dataSizeDecompressed);
            break;
        }
        case eCompressionType::PAKFILE:
        case eCompressionType::SNOWFLAKE:
        case eCompressionType::OODLE:
        {
            std::unique_ptr<char[]> cmpBuf = std::make_unique<char[]>(group->dataSizeCompressed);
            std::memcpy(cmpBuf.get(), pDataBuffer + group->dataOffset, group->dataSizeCompressed);

            uint64_t dataSizeDecompressed = group->dataSizeDecompressed; // this is cringe, can't  be const either, so awesome
            dcmpBuf = RTech::DecompressStreamedBuffer(std::move(cmpBuf), dataSizeDecompressed, group->dataCompression);

            break;
        }
        default:
            break;
        }

        const vg::rev4::VertexGroupHeader_t* grouphdr = reinterpret_cast<vg::rev4::VertexGroupHeader_t*>(dcmpBuf.get());

        uint8_t lodIdx = 0;
        for (uint16_t lodLevel = 0; lodLevel < pStudioHdr->lodCount; lodLevel++)
        {
            if (lodIdx == grouphdr->lodCount)
                break;

            // does this group contian this lod
            if (!(grouphdr->lodMap & (1 << lodLevel)))
                continue;

            assert(static_cast<uint8_t>(lodIdx) < grouphdr->lodCount);

            const vg::rev4::ModelLODHeader_t* lod = grouphdr->pLod(lodIdx);
            ModelLODData_t& lodData = parsedData->lods.at(lodLevel);
            lodData.switchPoint = pStudioHdr->LODThreshold(lodLevel);

            parsedData->meshVertexData.resize(parsedData->meshVertexData.size() + lod->meshCount);

            // [rika]: this should only get hit once per LOD
            const size_t curMeshCount = lodData.meshes.size();
            lodData.meshes.resize(curMeshCount + lod->meshCount);

            for (uint16_t bdyIdx = 0; bdyIdx < pStudioHdr->numbodyparts; bdyIdx++)
            {
                const r5::mstudiobodyparts_v16_t* const pBodypart = pStudioHdr->pBodypart(bdyIdx);

                parsedData->SetupBodyPart(bdyIdx, pBodypart->pszName(), static_cast<int>(lodData.models.size()), pBodypart->nummodels);

                for (uint16_t modelIdx = 0; modelIdx < pBodypart->nummodels; modelIdx++)
                {
                    const r5::mstudiomodel_v16_t* const pModel = pBodypart->pModel(modelIdx);
                    ModelModelData_t modelData = {};

                    modelData.name = std::format("{}_{}", pBodypart->pszName(), std::to_string(modelIdx));
                    modelData.meshIndex = static_cast<size_t>(lodMeshCount[lodLevel]);

                    // because we resize, having a pointer to the element in the container is fine.
                    modelData.meshes = pModel->meshCountTotal > 0 ? &lodData.meshes.at(lodMeshCount[lodLevel]) : nullptr;

                    for (uint16_t meshIdx = 0; meshIdx < pModel->meshCountTotal; ++meshIdx)
                    {
                        // we do not handle blendstates currently
                        if (meshIdx == pModel->meshCountBase)
                            break;

                        const r5::mstudiomesh_v16_t* const pMesh = pModel->pMesh(meshIdx);
                        const vg::rev4::MeshHeader_t* const mesh = lod->pMesh(static_cast<uint8_t>(pMesh->meshid));

                        if (mesh->flags == 0)
                            continue;

                        // reserve a buffer
                        CManagedBuffer* const buffer = g_BufferManager.ClaimBuffer();

                        CMeshData* meshVertexData = reinterpret_cast<CMeshData*>(buffer->Buffer());
                        meshVertexData->InitWriter();

                        ModelMeshData_t& meshData = lodData.meshes.at(lodMeshCount[lodLevel]);

                        meshData.rawVertexLayoutFlags |= mesh->flags;

                        meshData.vertCacheSize = mesh->vertCacheSize;
                        meshData.vertCount = mesh->vertCount;
                        meshData.indexCount = mesh->indexCount;

                        meshData.bodyPartIndex = bdyIdx;

                        if (mesh->extraBoneWeightSize)
                        {
                            char* ebw = new char[mesh->extraBoneWeightSize];
                            memcpy_s(ebw, mesh->extraBoneWeightSize, mesh->pBoneWeights(), mesh->extraBoneWeightSize);

                            meshData.extraBoneWeights = ebw;
                            meshData.extraBoneWeightsSize = mesh->extraBoneWeightSize;
                        }
                        else
                        {
                            meshData.extraBoneWeights = nullptr;
                            meshData.extraBoneWeightsSize = 0;
                        }


                        lodData.vertexCount += mesh->vertCount;
                        lodData.indexCount += mesh->indexCount;

                        meshData.ParseTexcoords();

                        char* const rawVertexData = mesh->pVertices(); // pointer to all of the vertex data for this mesh
                        const vvw::mstudioboneweightextra_t* const weights = mesh->pBoneWeights();
                        const uint16_t* const meshIndexData = mesh->pIndices(); // pointer to all of the index data for this mesh

#if (ADVANCED_MODEL_PREVIEW)
                        meshData.rawVertexData = new char[mesh->vertCacheSize * mesh->vertCount]; // get a pointer to the raw vertex data for use with the game's shaders

                        memcpy(meshData.rawVertexData, rawVertexData, static_cast<uint64_t>(mesh->vertCacheSize)* mesh->vertCount);
#endif
                        
                        meshVertexData->AddIndices(meshIndexData, meshData.indexCount);
                        meshVertexData->AddVertices(nullptr, meshData.vertCount);

                        if (meshData.texcoordCount > 1)
                            meshVertexData->AddTexcoords(nullptr, meshData.vertCount * (meshData.texcoordCount - 1));

                        meshVertexData->AddWeights(nullptr, 0);

                        int weightIdx = 0;
                        for (unsigned int vertIdx = 0; vertIdx < mesh->vertCount; ++vertIdx)
                        {
                            char* const vertexData = rawVertexData + (vertIdx * mesh->vertCacheSize);
                            Vector2D* const texcoords = meshData.texcoordCount > 1 ? &meshVertexData->GetTexcoords()[vertIdx * (meshData.texcoordCount - 1)] : nullptr;
                            Vertex_t::ParseVertexFromVG(&meshVertexData->GetVertices()[vertIdx], &meshVertexData->GetWeights()[weightIdx], texcoords, &meshData, vertexData, boneMap, weights, vertexWeightParseFlags, weightIdx);
                        }
                        meshData.weightsCount = weightIdx;
                        meshVertexData->AddWeights(nullptr, meshData.weightsCount);

                        meshData.ParseMaterial(parsedData, pMesh->material);

                        lodMeshCount[lodLevel]++;
                        modelData.meshCount++;
                        modelData.vertCount += meshData.vertCount;

                        // for export
                        lodData.weightsPerVert = meshData.weightsPerVert > lodData.weightsPerVert ? meshData.weightsPerVert : lodData.weightsPerVert;
                        lodData.texcoordsPerVert = meshData.texcoordCount > lodData.texcoordsPerVert ? meshData.texcoordCount : lodData.texcoordsPerVert;

                        // remove it from usage
                        meshVertexData->DestroyWriter();

                        meshData.meshVertexDataIndex = parsedData->meshVertexData.size();
                        parsedData->meshVertexData.addBack(reinterpret_cast<char*>(meshVertexData), meshVertexData->GetSize());

                        // relieve buffer
                        g_BufferManager.RelieveBuffer(buffer);
                    }

                    lodData.models.push_back(modelData);
                }
            }

            lodIdx++;

            // [rika]: to remove excess meshes (empty meshes we skipped, since we set size at the beginning). this should only deallocate memory
            // [rika]: this should only be hit once per LOD level, since it's either all levels in one group, or a level per group.
            lodData.meshes.resize(lodMeshCount[lodLevel]);
        }
    }

    parsedData->meshVertexData.shrink();
}


static void ParseModelVertexData_v19_2(CPakAsset* const asset, ModelAsset* const modelAsset)
{
    const std::unique_ptr<char[]> pStreamed = modelAsset->vertexStreamingData.size > 0 ? asset->getStarPakData(modelAsset->vertexStreamingData.offset, modelAsset->vertexStreamingData.size, false) : nullptr; // probably smarter to check the size inside getStarPakData but whatever!
    char* const pDataBuffer = pStreamed.get() ? pStreamed.get() : modelAsset->staticStreamingData;

    if (!pDataBuffer)
    {
        Log("%s loaded with no vertex data\n", modelAsset->name);
        return;
    }

    ModelParsedData_t* const parsedData = modelAsset->GetParsedData();

    const r5::studiohdr_v19_2_t* const pStudioHdr = reinterpret_cast<r5::studiohdr_v19_2_t*>(modelAsset->data);

    const uint16_t* boneMap = pStudioHdr->boneStateCount ? pStudioHdr->pBoneStates() : s_VertexDataBaseBoneMapButWide; // does this model have remapped bones? use default map if not

    const uint8_t vertexWeightParseFlags = ((pStudioHdr->flags & STUDIOHDR_FLAGS_USES_EXTRA_BONE_WEIGHTS) ? VERT_PARSE_EXTRAWEIGHT : 0x0) | VERT_PARSE_BONES_1024;

    parsedData->lods.resize(pStudioHdr->lodCount);
    parsedData->bodyParts.resize(pStudioHdr->numbodyparts);

    uint16_t lodMeshCount[8]{ 0 };

    for (uint16_t groupIdx = 0; groupIdx < pStudioHdr->groupHeaderCount; groupIdx++)
    {
        const r5::studio_hw_groupdata_v16_t* group = pStudioHdr->pLODGroup(groupIdx);

        std::unique_ptr<char[]> dcmpBuf = nullptr;

        // decompress buffer
        switch (group->dataCompression)
        {
        case eCompressionType::NONE:
        {
            dcmpBuf = std::make_unique<char[]>(group->dataSizeDecompressed);
            std::memcpy(dcmpBuf.get(), pDataBuffer + group->dataOffset, group->dataSizeDecompressed);
            break;
        }
        case eCompressionType::PAKFILE:
        case eCompressionType::SNOWFLAKE:
        case eCompressionType::OODLE:
        {
            std::unique_ptr<char[]> cmpBuf = std::make_unique<char[]>(group->dataSizeCompressed);
            std::memcpy(cmpBuf.get(), pDataBuffer + group->dataOffset, group->dataSizeCompressed);

            uint64_t dataSizeDecompressed = group->dataSizeDecompressed; // this is cringe, can't  be const either, so awesome
            dcmpBuf = RTech::DecompressStreamedBuffer(std::move(cmpBuf), dataSizeDecompressed, group->dataCompression);

            break;
        }
        default:
            break;
        }

        const vg::rev4::VertexGroupHeader_t* grouphdr = reinterpret_cast<vg::rev4::VertexGroupHeader_t*>(dcmpBuf.get());

        uint8_t lodIdx = 0;
        for (uint16_t lodLevel = 0; lodLevel < pStudioHdr->lodCount; lodLevel++)
        {
            if (lodIdx == grouphdr->lodCount)
                break;

            // does this group contian this lod
            if (!(grouphdr->lodMap & (1 << lodLevel)))
                continue;

            assert(static_cast<uint8_t>(lodIdx) < grouphdr->lodCount);

            const vg::rev4::ModelLODHeader_t* lod = grouphdr->pLod(lodIdx);
            ModelLODData_t& lodData = parsedData->lods.at(lodLevel);
            lodData.switchPoint = pStudioHdr->LODThreshold(lodLevel);

            parsedData->meshVertexData.resize(parsedData->meshVertexData.size() + lod->meshCount);

            // [rika]: this should only get hit once per LOD
            const size_t curMeshCount = lodData.meshes.size();
            lodData.meshes.resize(curMeshCount + lod->meshCount);

            for (uint16_t bdyIdx = 0; bdyIdx < pStudioHdr->numbodyparts; bdyIdx++)
            {
                const r5::mstudiobodyparts_v16_t* const pBodypart = pStudioHdr->pBodypart(bdyIdx);

                parsedData->SetupBodyPart(bdyIdx, pBodypart->pszName(), static_cast<int>(lodData.models.size()), pBodypart->nummodels);

                for (uint16_t modelIdx = 0; modelIdx < pBodypart->nummodels; modelIdx++)
                {
                    const r5::mstudiomodel_v16_t* const pModel = pBodypart->pModel(modelIdx);
                    ModelModelData_t modelData = {};

                    modelData.name = std::format("{}_{}", pBodypart->pszName(), std::to_string(modelIdx));
                    modelData.meshIndex = static_cast<size_t>(lodMeshCount[lodLevel]);

                    // because we resize, having a pointer to the element in the container is fine.
                    modelData.meshes = pModel->meshCountTotal > 0 ? &lodData.meshes.at(lodMeshCount[lodLevel]) : nullptr;

                    for (uint16_t meshIdx = 0; meshIdx < pModel->meshCountTotal; ++meshIdx)
                    {
                        // we do not handle blendstates currently
                        if (meshIdx == pModel->meshCountBase)
                            break;

                        const r5::mstudiomesh_v16_t* const pMesh = pModel->pMesh(meshIdx);
                        const vg::rev4::MeshHeader_t* const mesh = lod->pMesh(static_cast<uint8_t>(pMesh->meshid));

                        if (mesh->flags == 0)
                            continue;

                        // reserve a buffer
                        CManagedBuffer* const buffer = g_BufferManager.ClaimBuffer();

                        CMeshData* meshVertexData = reinterpret_cast<CMeshData*>(buffer->Buffer());
                        meshVertexData->InitWriter();

                        ModelMeshData_t& meshData = lodData.meshes.at(lodMeshCount[lodLevel]);

                        meshData.rawVertexLayoutFlags |= mesh->flags;

                        meshData.vertCacheSize = mesh->vertCacheSize;
                        meshData.vertCount = mesh->vertCount;
                        meshData.indexCount = mesh->indexCount;

                        meshData.bodyPartIndex = bdyIdx;

                        if (mesh->extraBoneWeightSize)
                        {
                            char* ebw = new char[mesh->extraBoneWeightSize];
                            memcpy_s(ebw, mesh->extraBoneWeightSize, mesh->pBoneWeights(), mesh->extraBoneWeightSize);

                            meshData.extraBoneWeights = ebw;
                            meshData.extraBoneWeightsSize = mesh->extraBoneWeightSize;
                        }
                        else
                        {
                            meshData.extraBoneWeights = nullptr;
                            meshData.extraBoneWeightsSize = 0;
                        }


                        lodData.vertexCount += mesh->vertCount;
                        lodData.indexCount += mesh->indexCount;

                        meshData.ParseTexcoords();

                        char* const rawVertexData = mesh->pVertices(); // pointer to all of the vertex data for this mesh
                        const vvw::mstudioboneweightextra_t* const weights = mesh->pBoneWeights();
                        const uint16_t* const meshIndexData = mesh->pIndices(); // pointer to all of the index data for this mesh

#if (ADVANCED_MODEL_PREVIEW)
                        meshData.rawVertexData = new char[mesh->vertCacheSize * mesh->vertCount]; // get a pointer to the raw vertex data for use with the game's shaders

                        memcpy(meshData.rawVertexData, rawVertexData, static_cast<uint64_t>(mesh->vertCacheSize) * mesh->vertCount);
#endif

                        meshVertexData->AddIndices(meshIndexData, meshData.indexCount);
                        meshVertexData->AddVertices(nullptr, meshData.vertCount);

                        if (meshData.texcoordCount > 1)
                            meshVertexData->AddTexcoords(nullptr, meshData.vertCount * (meshData.texcoordCount - 1));

                        meshVertexData->AddWeights(nullptr, 0);

                        int weightIdx = 0;
                        for (unsigned int vertIdx = 0; vertIdx < mesh->vertCount; ++vertIdx)
                        {
                            char* const vertexData = rawVertexData + (vertIdx * mesh->vertCacheSize);
                            Vector2D* const texcoords = meshData.texcoordCount > 1 ? &meshVertexData->GetTexcoords()[vertIdx * (meshData.texcoordCount - 1)] : nullptr;
                            Vertex_t::ParseVertexFromVG(&meshVertexData->GetVertices()[vertIdx], &meshVertexData->GetWeights()[weightIdx], texcoords, &meshData, vertexData, boneMap, weights, vertexWeightParseFlags, weightIdx);
                        }
                        meshData.weightsCount = weightIdx;
                        meshVertexData->AddWeights(nullptr, meshData.weightsCount);

                        meshData.ParseMaterial(parsedData, pMesh->material);

                        lodMeshCount[lodLevel]++;
                        modelData.meshCount++;
                        modelData.vertCount += meshData.vertCount;

                        // for export
                        lodData.weightsPerVert = meshData.weightsPerVert > lodData.weightsPerVert ? meshData.weightsPerVert : lodData.weightsPerVert;
                        lodData.texcoordsPerVert = meshData.texcoordCount > lodData.texcoordsPerVert ? meshData.texcoordCount : lodData.texcoordsPerVert;

                        // remove it from usage
                        meshVertexData->DestroyWriter();

                        meshData.meshVertexDataIndex = parsedData->meshVertexData.size();
                        parsedData->meshVertexData.addBack(reinterpret_cast<char*>(meshVertexData), meshVertexData->GetSize());

                        // relieve buffer
                        g_BufferManager.RelieveBuffer(buffer);
                    }

                    lodData.models.push_back(modelData);
                }
            }

            lodIdx++;

            // [rika]: to remove excess meshes (empty meshes we skipped, since we set size at the beginning). this should only deallocate memory
            // [rika]: this should only be hit once per LOD level, since it's either all levels in one group, or a level per group.
            lodData.meshes.resize(lodMeshCount[lodLevel]);
        }
    }

    parsedData->meshVertexData.shrink();
}

static void ParseModelTextureData_v8(ModelParsedData_t* const parsedData)
{
    const studiohdr_generic_t* const pStudioHdr = parsedData->pStudioHdr();

    const r5::mstudiotexture_v8_t* const pTextures = reinterpret_cast<const r5::mstudiotexture_v8_t* const>(pStudioHdr->pTextures());
    parsedData->materials.resize(pStudioHdr->textureCount);

    for (int i = 0; i < pStudioHdr->textureCount; ++i)
    {
        ModelMaterialData_t& matlData = parsedData->materials.at(i);
        const r5::mstudiotexture_v8_t* const texture = &pTextures[i];

        // if guid is 0, the material is a VMT
        if (texture->texture != 0)
            matlData.asset = g_assetData.FindAssetByGUID<CPakAsset>(texture->texture);

        matlData.guid = texture->texture;
        matlData.SetName(texture->pszName());
    }

    parsedData->skins.reserve(pStudioHdr->numSkinFamilies);
    for (int i = 0; i < pStudioHdr->numSkinFamilies; i++)
        parsedData->skins.emplace_back(pStudioHdr->pSkinName(i), pStudioHdr->pSkinFamily(i));
}

static void ParseModelTextureData_v16(ModelParsedData_t* const parsedData)
{
    const studiohdr_generic_t* const pStudioHdr = parsedData->pStudioHdr();

    const uint64_t* const pTextures = reinterpret_cast<const uint64_t* const>(pStudioHdr->pTextures());
    parsedData->materials.resize(pStudioHdr->textureCount);

    char namebuf[16]{};

    for (int i = 0; i < pStudioHdr->textureCount; ++i)
    {
        ModelMaterialData_t& matlData = parsedData->materials.at(i);
        uint64_t texture = pTextures[i];

        matlData.guid = texture;

        // not possible to have vmt materials
        matlData.asset = g_assetData.FindAssetByGUID<CPakAsset>(texture);

        snprintf(namebuf, 16, "0x%llX", texture);
        matlData.StoreName(namebuf);
    }

    parsedData->skins.reserve(pStudioHdr->numSkinFamilies);
    for (int i = 0; i < pStudioHdr->numSkinFamilies; i++)
        parsedData->skins.emplace_back(pStudioHdr->pSkinName(i), pStudioHdr->pSkinFamily(i));
}

void LoadModelAsset(CAssetContainer* const pak, CAsset* const asset)
{
    UNUSED(pak);

    CPakAsset* const pakAsset = static_cast<CPakAsset*>(asset);

    ModelAsset* mdlAsset = nullptr;
    const AssetPtr_t streamEntry = pakAsset->getStarPakStreamEntry(false); // vertex data is never opt streamed (I hope)

    const eMDLVersion ver = GetModelVersionFromAsset(pakAsset, static_cast<CPakFile* const>(pak));
    switch (ver)
    {
    case eMDLVersion::VERSION_8:
    {
        ModelAssetHeader_v8_t* hdr = reinterpret_cast<ModelAssetHeader_v8_t*>(pakAsset->header());
        mdlAsset = new ModelAsset(hdr, streamEntry, ver);

        ParseModelBoneData_v8(mdlAsset->GetParsedData());
        ParseModelAttachmentData_v8(mdlAsset->GetParsedData());
        ParseModelHitboxData_v8(mdlAsset->GetParsedData());
        ParseModelTextureData_v8(mdlAsset->GetParsedData());
        ParseModelVertexData_v8(pakAsset, mdlAsset);
        ParseModelAnimTypes_V8(mdlAsset->GetParsedData());
        break;
    }
    case eMDLVersion::VERSION_9:
    case eMDLVersion::VERSION_10:
    case eMDLVersion::VERSION_11:
    case eMDLVersion::VERSION_12:
    {
        ModelAssetHeader_v9_t* hdr = reinterpret_cast<ModelAssetHeader_v9_t*>(pakAsset->header());
        mdlAsset = new ModelAsset(hdr, streamEntry, ver);

        ParseModelBoneData_v8(mdlAsset->GetParsedData());
        ParseModelAttachmentData_v8(mdlAsset->GetParsedData());
        ParseModelHitboxData_v8(mdlAsset->GetParsedData());
        ParseModelTextureData_v8(mdlAsset->GetParsedData());
        ParseModelVertexData_v9(pakAsset, mdlAsset);
        ParseModelAnimTypes_V8(mdlAsset->GetParsedData());
        break;
    }
    case eMDLVersion::VERSION_12_1: // has to have its own vertex func
    case eMDLVersion::VERSION_12_2:
    case eMDLVersion::VERSION_12_3:
    case eMDLVersion::VERSION_12_4:
    case eMDLVersion::VERSION_12_5:
    {
        ModelAssetHeader_v12_1_t* hdr = reinterpret_cast<ModelAssetHeader_v12_1_t*>(pakAsset->header());
        mdlAsset = new ModelAsset(hdr, streamEntry, ver);

        ParseModelBoneData_v12_1(mdlAsset->GetParsedData());
        ParseModelAttachmentData_v8(mdlAsset->GetParsedData());
        ParseModelHitboxData_v8(mdlAsset->GetParsedData());
        ParseModelTextureData_v8(mdlAsset->GetParsedData());
        ParseModelVertexData_v12_1(pakAsset, mdlAsset);
        ParseModelAnimTypes_V8(mdlAsset->GetParsedData());
        break;
    }
    case eMDLVersion::VERSION_13:
    case eMDLVersion::VERSION_13_1:
    {
        ModelAssetHeader_v13_t* hdr = reinterpret_cast<ModelAssetHeader_v13_t*>(pakAsset->header());
        mdlAsset = new ModelAsset(hdr, streamEntry, ver);

        ParseModelBoneData_v12_1(mdlAsset->GetParsedData());
        ParseModelAttachmentData_v8(mdlAsset->GetParsedData());
        ParseModelHitboxData_v8(mdlAsset->GetParsedData());
        ParseModelTextureData_v8(mdlAsset->GetParsedData());
        ParseModelVertexData_v12_1(pakAsset, mdlAsset);
        ParseModelAnimTypes_V8(mdlAsset->GetParsedData());
        break;
    }
    case eMDLVersion::VERSION_14:
    case eMDLVersion::VERSION_14_1:
    case eMDLVersion::VERSION_15:
    {
        ModelAssetHeader_v13_t* hdr = reinterpret_cast<ModelAssetHeader_v13_t*>(pakAsset->header());
        mdlAsset = new ModelAsset(hdr, streamEntry, ver);

        ParseModelBoneData_v12_1(mdlAsset->GetParsedData());
        ParseModelAttachmentData_v8(mdlAsset->GetParsedData());
        ParseModelHitboxData_v8(mdlAsset->GetParsedData());
        ParseModelTextureData_v8(mdlAsset->GetParsedData());
        ParseModelVertexData_v14(pakAsset, mdlAsset);
        ParseModelAnimTypes_V8(mdlAsset->GetParsedData());
        break;
    }
    case eMDLVersion::VERSION_16:
    case eMDLVersion::VERSION_17:
    {
        ModelAssetHeader_v16_t* hdr = reinterpret_cast<ModelAssetHeader_v16_t*>(pakAsset->header());
        ModelAssetCPU_v16_t* cpu = reinterpret_cast<ModelAssetCPU_v16_t*>(pakAsset->cpu());
        mdlAsset = new ModelAsset(hdr, cpu, streamEntry, ver);

        ParseModelBoneData_v16(mdlAsset->GetParsedData());
        ParseModelAttachmentData_v16(mdlAsset->GetParsedData());
        ParseModelHitboxData_v16(mdlAsset->GetParsedData());
        ParseModelTextureData_v16(mdlAsset->GetParsedData());
        ParseModelVertexData_v16(pakAsset, mdlAsset);
        ParseModelAnimTypes_V16(mdlAsset->GetParsedData());
        break;
    }
    case eMDLVersion::VERSION_18:
    {
        ModelAssetHeader_v16_t* hdr = reinterpret_cast<ModelAssetHeader_v16_t*>(pakAsset->header());
        ModelAssetCPU_v16_t* cpu = reinterpret_cast<ModelAssetCPU_v16_t*>(pakAsset->cpu());
        mdlAsset = new ModelAsset(hdr, cpu, streamEntry, ver);

        ParseModelBoneData_v16(mdlAsset->GetParsedData());
        ParseModelAttachmentData_v16(mdlAsset->GetParsedData());
        ParseModelHitboxData_v16(mdlAsset->GetParsedData());
        ParseModelTextureData_v16(mdlAsset->GetParsedData());
        ParseModelVertexData_v16(pakAsset, mdlAsset);
        ParseModelAnimTypes_V16(mdlAsset->GetParsedData());
        break;
    }
    case eMDLVersion::VERSION_19:
    case eMDLVersion::VERSION_19_1:
    {
        ModelAssetHeader_v16_t* hdr = reinterpret_cast<ModelAssetHeader_v16_t*>(pakAsset->header());
        ModelAssetCPU_v16_t* cpu = reinterpret_cast<ModelAssetCPU_v16_t*>(pakAsset->cpu());
        mdlAsset = new ModelAsset(hdr, cpu, streamEntry, ver);

        ParseModelBoneData_v19(mdlAsset->GetParsedData());
        ParseModelAttachmentData_v16(mdlAsset->GetParsedData());
        ParseModelHitboxData_v16(mdlAsset->GetParsedData());
        ParseModelTextureData_v16(mdlAsset->GetParsedData());
        ParseModelVertexData_v16(pakAsset, mdlAsset);
        ParseModelAnimTypes_V16(mdlAsset->GetParsedData());
        break;
    }
    case eMDLVersion::VERSION_19_2:
    case eMDLVersion::VERSION_19_3:
    {
        ModelAssetHeader_v16_t* hdr = reinterpret_cast<ModelAssetHeader_v16_t*>(pakAsset->header());
        ModelAssetCPU_v16_t* cpu = reinterpret_cast<ModelAssetCPU_v16_t*>(pakAsset->cpu());
        mdlAsset = new ModelAsset(hdr, cpu, streamEntry, ver);

        ParseModelBoneData_v19(mdlAsset->GetParsedData());
        ParseModelAttachmentData_v16(mdlAsset->GetParsedData());
        ParseModelHitboxData_v16(mdlAsset->GetParsedData());
        ParseModelTextureData_v16(mdlAsset->GetParsedData());
        ParseModelVertexData_v19_2(pakAsset, mdlAsset);
        ParseModelAnimTypes_V16(mdlAsset->GetParsedData());
        break;
    }
    default:
    {
        assertm(false, "unaccounted asset version, will cause major issues!");
        return;
    }
    }

    // [rika]: go back and set our subversion
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

    // parse sequences for children
    if (modelAsset->numAnimSeqs)
    {
        ModelParsedData_t* const parsedData = modelAsset->GetParsedData();

        parsedData->numExternalSequences = modelAsset->numAnimSeqs;
        parsedData->externalSequences = modelAsset->animSeqs;

        const uint64_t* guids = reinterpret_cast<const uint64_t*>(modelAsset->animSeqs);

        for (uint16_t seqIdx = 0; seqIdx < modelAsset->numAnimSeqs; seqIdx++)
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

            animSeq->parentModel = !animSeq->parentModel ? modelAsset : animSeq->parentModel;
        }
    }

    // external include models
    if (modelAsset->numAnimRigs)
    {
        ModelParsedData_t* const parsedData = modelAsset->GetParsedData();

        parsedData->numExternalIncludeModels = modelAsset->numAnimRigs;
        parsedData->externalIncludeModels = modelAsset->animRigs;
    }    

    // [rika]: in post load now because it depends on asqd
    switch (modelAsset->version)
    {
    case eMDLVersion::VERSION_8:
    case eMDLVersion::VERSION_9:
    case eMDLVersion::VERSION_10:
    case eMDLVersion::VERSION_11:
    case eMDLVersion::VERSION_12:
    {
        ParseModelSequenceData_NoStall(modelAsset->GetParsedData(), reinterpret_cast<char* const>(modelAsset->data));
        break;
    }
    case eMDLVersion::VERSION_12_1: // has to have its own vertex func
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
        ParseModelSequenceData_Stall_V8(modelAsset->GetParsedData(), reinterpret_cast<char* const>(modelAsset->data));
        break;
    }
    case eMDLVersion::VERSION_16:
    case eMDLVersion::VERSION_17:
    {
        ParseModelSequenceData_Stall_V16(modelAsset->GetParsedData(), reinterpret_cast<char* const>(modelAsset->data));
        break;
    }
    case eMDLVersion::VERSION_18:
    case eMDLVersion::VERSION_19:
    {
        ParseModelSequenceData_Stall_V18(modelAsset->GetParsedData(), reinterpret_cast<char* const>(modelAsset->data));
        break;
    }
    case eMDLVersion::VERSION_19_1:
    case eMDLVersion::VERSION_19_2:
    {
        ParseModelSequenceData_Stall_V19_1(modelAsset->GetParsedData(), reinterpret_cast<char* const>(modelAsset->data), ANIM_BONEFLAG_BITS_4);
        break;
    }
    case eMDLVersion::VERSION_19_3:
    {
        ParseModelSequenceData_Stall_V19_1(modelAsset->GetParsedData(), reinterpret_cast<char* const>(modelAsset->data), ANIM_BONEFLAG_BITS_6);
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
    if (!animSeq->animationParsed)
    {
        if (!(animSeq->parentModel || animSeq->parentRig))
        {
            // rig will be nullptr if the sequence asset was not found thru a rig and instead from the model itself
            if (rig)
                animSeq->parentRig = rig;
            else
                animSeq->parentModel = modelAsset;
        }

        AnimSeq_ParseExtraData(seqAsset);
    }

    const std::vector<ModelBone_t>* srcBones = nullptr;

    if (animSeq->parentModel)
        srcBones = animSeq->parentModel->GetRig();
    else if (animSeq->parentRig)
        srcBones = animSeq->parentRig->GetRig();

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

    for (int i = 0; i < parsedData->NumLocalSeq(); i++)
    {
        const ModelSeq_t* const seqdesc = parsedData->LocalSeq(i);

        previewInfo.sequences.emplace_back(
            seqdesc->szlabel,
            0ull, // guid
            seqdesc,
            parsedData->GetRig(), // local sequences are always parsed against the model's own skeleton
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

        previewInfo.bodygroupModelSelected.resize(parsedData->bodyParts.size(), 0ull);

        previewInfo.selectedBodypartIndex = previewInfo.selectedBodypartIndex > parsedData->bodyParts.size() ? 0 : previewInfo.selectedBodypartIndex;
        previewInfo.selectedSkinIndex = previewInfo.selectedSkinIndex > parsedData->skins.size() ? 0 : previewInfo.selectedSkinIndex;

        // [rika]: lod handling
        assertm(parsedData->lods.size(), "no lods in preview?");
        previewInfo.maxLODIndex = static_cast<uint8_t>(parsedData->lods.size()) - 1;
        previewInfo.selectedLODLevel = previewInfo.selectedLODLevel > previewInfo.maxLODIndex ? previewInfo.maxLODIndex : previewInfo.selectedLODLevel; // clamp it
        
        ModelPreview_DiscoverSequences(modelAsset, previewInfo);
    }

    ImGui::Text("Rigs: %i", modelAsset->numAnimRigs);
    ImGui::Text("Sequences: %i", modelAsset->numAnimSeqs);

    void* const drawData = PreviewParsedData(&previewInfo, parsedData, modelAsset->name, asset->GetAssetGUID(), firstFrameForAsset);

#if defined(HAS_BONED_MODELS)
    if (drawData && Preview_SequencesSection(&previewInfo, parsedData, reinterpret_cast<CDXDrawData*>(drawData)))
        ModelPreview_DiscoverSequences(modelAsset, previewInfo);
#endif

    return drawData;
}

static bool ExportModelStreamedData(const ModelAsset* const modelAsset, std::filesystem::path& exportPath, const char* const streamedData, const char* const extension)
{
    const studiohdr_generic_t* const pStudioHdr = modelAsset->pStudioHdr();

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
        hwOut.write(streamedData, pStudioHdr->hwDataSize);

        return true;
    }
    case eMDLVersion::VERSION_16:
    case eMDLVersion::VERSION_17:
    case eMDLVersion::VERSION_18:
    case eMDLVersion::VERSION_19:
    case eMDLVersion::VERSION_19_1:
    case eMDLVersion::VERSION_19_2:
    case eMDLVersion::VERSION_19_3:
    {
        // special case because of compression
        exportPath.replace_extension(extension);

        std::unique_ptr<char[]> hwBufTmp = std::make_unique<char[]>(pStudioHdr->hwDataSize);
        char* pPos = hwBufTmp.get(); // position in the decompressed buffer for writing

        for (int i = 0; i < pStudioHdr->groupCount; i++)
        {
            const studio_hw_groupdata_t& group = pStudioHdr->groups[i];

            switch (group.dataCompression)
            {
            case eCompressionType::NONE:
            {
                memcpy_s(pPos, group.dataSizeDecompressed, streamedData + group.dataOffset, group.dataSizeDecompressed);
                break;
            }
            case eCompressionType::PAKFILE:
            case eCompressionType::SNOWFLAKE:
            case eCompressionType::OODLE:
            {
                std::unique_ptr<char[]> dcmpBuf = std::make_unique<char[]>(group.dataSizeCompressed);

                memcpy_s(dcmpBuf.get(), group.dataSizeCompressed, streamedData + group.dataOffset, group.dataSizeCompressed);

                size_t dataSizeDecompressed = group.dataSizeDecompressed;
                dcmpBuf = RTech::DecompressStreamedBuffer(std::move(dcmpBuf), dataSizeDecompressed, group.dataCompression);

                memcpy_s(pPos, group.dataSizeDecompressed, dcmpBuf.get(), group.dataSizeDecompressed);

                break;
            }
            default:
                break;
            }

            pPos += group.dataSizeDecompressed; // advance position
        }

        StreamIO hwOut(exportPath.string(), eStreamIOMode::Write);
        hwOut.write(hwBufTmp.get(), pStudioHdr->hwDataSize);

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

    const studiohdr_generic_t* const pStudioHdr = modelAsset->pStudioHdr();

    StreamIO studioOut(exportPath.string(), eStreamIOMode::Write);
    studioOut.write(reinterpret_cast<char*>(modelAsset->data), pStudioHdr->length);

    if (pStudioHdr->phySize > 0)
    {
        exportPath.replace_extension(".phy");

        // if we error here something is broken with setting up the model asset
        StreamIO physOut(exportPath.string(), eStreamIOMode::Write);
        physOut.write(reinterpret_cast<char*>(modelAsset->physics), pStudioHdr->phySize);
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
        if (pStudioHdr->vvdSize)
        {
            exportPath.replace_extension(".vvd");

            StreamIO vertOut(exportPath.string(), eStreamIOMode::Write);
            vertOut.write(modelAsset->vertexComponentData + pStudioHdr->vvdOffset, pStudioHdr->vvdSize);
        }

        // vvc
        if (pStudioHdr->vvcSize > 0)
        {
            exportPath.replace_extension(".vvc");

            StreamIO vertColorOut(exportPath.string(), eStreamIOMode::Write);
            vertColorOut.write(modelAsset->vertexComponentData + pStudioHdr->vvcOffset, pStudioHdr->vvcSize);
        }

        // vvw
        if (pStudioHdr->vvwSize > 0)
        {
            exportPath.replace_extension(".vvw");

            StreamIO vertWeightOut(exportPath.string(), eStreamIOMode::Write);
            vertWeightOut.write(modelAsset->vertexComponentData + pStudioHdr->vvwOffset, pStudioHdr->vvwSize);
        }

        // vtx
        if (pStudioHdr->vtxSize > 0)
        {
            exportPath.replace_extension(".dx11.vtx"); // cope

            // 'opt' being optimized
            StreamIO vertOptOut(exportPath.string(), eStreamIOMode::Write);
            vertOptOut.write(modelAsset->vertexComponentData + pStudioHdr->vtxOffset, pStudioHdr->vtxSize); // [rika]: 
        }
    }

    return true;
}

template <typename phyheader_t>
static bool ExportPhysicsModelPhy(const ModelAsset* const modelAsset, std::filesystem::path& exportPath)
{
    const studiohdr_generic_t& hdr = modelAsset->StudioHdr();

    if (!hdr.phySize)
        return false;

    const int mask = (hdr.contents & g_rsxSettings.exportPhysicsContentsFilter);
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
    const studiohdr_generic_t& hdr = modelAsset->StudioHdr();

    if (!hdr.bvhOffset)
        return false;

    CollisionModel_t outModel;
    const void* bvhData = (const char*)modelAsset->data + hdr.bvhOffset;

    const mstudiocollmodel_t* collModel = reinterpret_cast<const mstudiocollmodel_t*>(bvhData);
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

    for (auto& hitboxSet : parsedData->hitboxsets)
    {
        for (int i = 0; i < hitboxSet.numHitboxes; ++i)
        {
            const ModelHitbox_t& h = hitboxSet.hitboxes[i];

            objData += std::format("o {}_{}_{}\n", hitboxSet.name, i, h.name);

            const Vector* bbmin = h.bbmin;
            const Vector* bbmax = h.bbmax;

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

    if (g_rsxSettings.exportRigSequences && parsedData->NumLocalSeq() > 0)
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

        for (int i = 0; i < parsedData->NumLocalSeq(); i++)
        {
            const ModelSeq_t* const seqdesc = parsedData->LocalSeq(i);

            outputPath.replace_filename(seqdesc->szlabel);

            ExportSeqDesc(aseqAssetBinding->second.e.exportSetting, seqdesc, outputPath, modelAsset->name, modelAsset->GetRig(), RTech::StringToGuid(seqdesc->szlabel));
        }
    }

    exportPath.append(std::format("{}.rmdl", modelStem));

    switch (setting)
    {
        case eModelExportSetting::MODEL_CAST:
        {
            return ExportModelCast(parsedData, exportPath, asset->GetAssetGUID());
        }
        case eModelExportSetting::MODEL_RMAX:
        {
            return ExportModelRMAX(parsedData, exportPath);
        }
        case eModelExportSetting::MODEL_RMDL:
        {
            return ExportRawModelAsset(modelAsset, exportPath, streamedData.get());
        }
        case eModelExportSetting::MODEL_SMD:
        {
            return ExportModelSMD(parsedData, exportPath) && ExportModelQC(parsedData, exportPath, setting, 54);
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