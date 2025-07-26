#include <pch.h>

#include <game/rtech/assets/model.h>
#include <game/rtech/assets/texture.h>
#include <game/rtech/assets/material.h>
#include <game/rtech/assets/rson.h>
#include <game/rtech/assets/animseq.h>
#include <game/rtech/utils/bvh/bvh.h>
#include <game/rtech/utils/bsp/bspflags.h>

#include <core/mdl/stringtable.h>
#include <core/mdl/rmax.h>
#include <core/mdl/cast.h>

#include <core/render/dx.h>
#include <thirdparty/imgui/imgui.h>
#include <thirdparty/imgui/misc/imgui_utility.h>

#include <immintrin.h>

extern CDXParentHandler* g_dxHandler;
extern CBufferManager g_BufferManager;
extern ExportSettings_t g_ExportSettings;

static void ParseModelVertexData_v8(CPakAsset* const asset, ModelAsset* const modelAsset)
{
    UNUSED(asset);
    char* const pDataBuffer = modelAsset->vertDataPermanent;

    if (!pDataBuffer)
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
    VertexWeight_t* const   parseWeights    = reinterpret_cast<VertexWeight_t*> (&parseIndices[s_MaxStudioTriangles]); // ~8mb for weights

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

                        assertm(s_MaxStudioTriangles >= meshData.indexCount, "too many triangles");

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

static void ParseModelVertexData_v9(CPakAsset* const asset, ModelAsset* const modelAsset)
{
    const std::unique_ptr<char[]> pStreamed = modelAsset->vertDataStreamed.size > 0 ? asset->getStarPakData(modelAsset->vertDataStreamed.offset, modelAsset->vertDataStreamed.size, false) : nullptr; // probably smarter to check the size inside getStarPakData but whatever!
    char* const pDataBuffer = pStreamed.get() ? pStreamed.get() : modelAsset->vertDataPermanent;

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

                    lodData.vertexCount += mesh->vertCount;
                    lodData.indexCount += mesh->indexCount;

                    meshData.ParseTexcoords();                    

                    const char* const rawVertexData = mesh->pVertices(vgHdr);// pointer to all of the vertex data for this mesh
                    const vvw::mstudioboneweightextra_t* const weights = mesh->pBoneWeight(vgHdr);
                    const uint16_t* const meshIndexData = mesh->pIndices(vgHdr); // pointer to all of the index data for this mesh

#if defined(ADVANCED_MODEL_PREVIEW)
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
                        Vertex_t::ParseVertexFromVG(&meshVertexData->GetVertices()[vertIdx], &meshVertexData->GetWeights()[weightIdx], texcoords, &meshData, vertexData, boneMap, weights, weightIdx);
                    }
                    meshData.weightsCount = weightIdx;
                    meshVertexData->AddWeights(nullptr, meshData.weightsCount);

                    meshData.ParseMaterial(parsedData, pMesh->material);

                    lodMeshCount++;
                    modelData.meshCount++;

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
    const std::unique_ptr<char[]> pStreamed = asset->getStarPakData(modelAsset->vertDataStreamed.offset, modelAsset->vertDataStreamed.size, false);
    char* const pDataBuffer = pStreamed.get();

    if (!pDataBuffer)
    {
        Log("%s loaded with no vertex data\n", modelAsset->name);
        return;
    }

    ModelParsedData_t* const parsedData = modelAsset->GetParsedData();

    const r5::studiohdr_v12_1_t* const pStudioHdr = reinterpret_cast<r5::studiohdr_v12_1_t*>(modelAsset->data);

    const uint8_t* boneMap = pStudioHdr->boneStateCount ? pStudioHdr->pBoneStates() : s_VertexDataBaseBoneMap; // does this model have remapped bones? use default map if not

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

                        lodData.vertexCount += mesh->vertCount;
                        lodData.indexCount += mesh->indexCount;

                        meshData.ParseTexcoords();

                        char* const rawVertexData = mesh->pVertices(); // pointer to all of the vertex data for this mesh
                        const vvw::mstudioboneweightextra_t* const weights = mesh->pBoneWeights();
                        const uint16_t* const meshIndexData = mesh->pIndices(); // pointer to all of the index data for this mesh

#if defined(ADVANCED_MODEL_PREVIEW)
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
                            Vertex_t::ParseVertexFromVG(&meshVertexData->GetVertices()[vertIdx], &meshVertexData->GetWeights()[weightIdx], texcoords, &meshData, vertexData, boneMap, weights, weightIdx);
                        }
                        meshData.weightsCount = weightIdx;
                        meshVertexData->AddWeights(nullptr, meshData.weightsCount);

                        meshData.ParseMaterial(parsedData, pMesh->material);

                        lodMeshCount[lodLevel]++;
                        modelData.meshCount++;

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
    const std::unique_ptr<char[]> pStreamed = asset->getStarPakData(modelAsset->vertDataStreamed.offset, modelAsset->vertDataStreamed.size, false);
    char* const pDataBuffer = pStreamed.get();

    if (!pDataBuffer)
    {
        Log("%s loaded with no vertex data\n", modelAsset->name);
        return;
    }

    ModelParsedData_t* const parsedData = modelAsset->GetParsedData();

    const r5::studiohdr_v14_t* const pStudioHdr = reinterpret_cast<r5::studiohdr_v14_t*>(modelAsset->data);

    const uint8_t* boneMap = pStudioHdr->boneStateCount ? pStudioHdr->pBoneStates() : s_VertexDataBaseBoneMap; // does this model have remapped bones? use default map if not

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

                        const r5::mstudiomesh_v12_1_t* const pMesh = pModel->pMesh(meshIdx);
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

                        lodData.vertexCount += mesh->vertCount;
                        lodData.indexCount += mesh->indexCount;

                        meshData.ParseTexcoords();

                        char* const rawVertexData = mesh->pVertices(); // pointer to all of the vertex data for this mesh
                        const vvw::mstudioboneweightextra_t* const weights = mesh->pBoneWeights();
                        const uint16_t* const meshIndexData = mesh->pIndices(); // pointer to all of the index data for this mesh

#if defined(ADVANCED_MODEL_PREVIEW)
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
                            Vertex_t::ParseVertexFromVG(&meshVertexData->GetVertices()[vertIdx], &meshVertexData->GetWeights()[weightIdx], texcoords, &meshData, vertexData, boneMap, weights, weightIdx);
                        }
                        meshData.weightsCount = weightIdx;
                        meshVertexData->AddWeights(nullptr, meshData.weightsCount);

                        meshData.ParseMaterial(parsedData, pMesh->material);

                        lodMeshCount[lodLevel]++;
                        modelData.meshCount++;

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
    const std::unique_ptr<char[]> pStreamed = asset->getStarPakData(modelAsset->vertDataStreamed.offset, modelAsset->vertDataStreamed.size, false);
    char* const pDataBuffer = pStreamed.get();

    if (!pDataBuffer)
    {
        Log("%s loaded with no vertex data\n", modelAsset->name);
        return;
    }

    ModelParsedData_t* const parsedData = modelAsset->GetParsedData();

    const r5::studiohdr_v16_t* const pStudioHdr = reinterpret_cast<r5::studiohdr_v16_t*>(modelAsset->data);

    const uint8_t* boneMap = pStudioHdr->boneStateCount ? pStudioHdr->pBoneStates() : s_VertexDataBaseBoneMap; // does this model have remapped bones? use default map if not

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
            std::memcpy(dcmpBuf.get(), pStreamed.get() + group->dataOffset, group->dataSizeDecompressed);
            break;
        }
        case eCompressionType::PAKFILE:
        case eCompressionType::SNOWFLAKE:
        case eCompressionType::OODLE:
        {
            std::unique_ptr<char[]> cmpBuf = std::make_unique<char[]>(group->dataSizeCompressed);
            std::memcpy(cmpBuf.get(), pStreamed.get() + group->dataOffset, group->dataSizeCompressed);

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

                        lodData.vertexCount += mesh->vertCount;
                        lodData.indexCount += mesh->indexCount;

                        meshData.ParseTexcoords();

                        char* const rawVertexData = mesh->pVertices(); // pointer to all of the vertex data for this mesh
                        const vvw::mstudioboneweightextra_t* const weights = mesh->pBoneWeights();
                        const uint16_t* const meshIndexData = mesh->pIndices(); // pointer to all of the index data for this mesh

#if defined(ADVANCED_MODEL_PREVIEW)
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
                            Vertex_t::ParseVertexFromVG(&meshVertexData->GetVertices()[vertIdx], &meshVertexData->GetWeights()[weightIdx], texcoords, &meshData, vertexData, boneMap, weights, weightIdx);
                        }
                        meshData.weightsCount = weightIdx;
                        meshVertexData->AddWeights(nullptr, meshData.weightsCount);

                        meshData.ParseMaterial(parsedData, pMesh->material);

                        lodMeshCount[lodLevel]++;
                        modelData.meshCount++;

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
        if (texture->guid != 0)
            matlData.asset = g_assetData.FindAssetByGUID<CPakAsset>(texture->guid);

        matlData.guid = texture->guid;
        matlData.name = texture->pszName();
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

    for (int i = 0; i < pStudioHdr->textureCount; ++i)
    {
        ModelMaterialData_t& matlData = parsedData->materials.at(i);
        uint64_t texture = pTextures[i];

        matlData.guid = texture;

        // not possible to have vmt materials
        matlData.asset = g_assetData.FindAssetByGUID<CPakAsset>(texture);

        matlData.name = matlData.asset ? matlData.asset->GetAssetName() : std::format("0x{:x}", texture); // no name
    }

    parsedData->skins.reserve(pStudioHdr->numSkinFamilies);
    for (int i = 0; i < pStudioHdr->numSkinFamilies; i++)
        parsedData->skins.emplace_back(pStudioHdr->pSkinName_V16(i), pStudioHdr->pSkinFamily(i));
}

void LoadModelAsset(CAssetContainer* const pak, CAsset* const asset)
{
    UNUSED(pak);

    CPakAsset* const pakAsset = static_cast<CPakAsset*>(asset);

    ModelAsset* mdlAsset = nullptr;
    const AssetPtr_t streamEntry = pakAsset->getStarPakStreamEntry(false); // vertex data is never opt streamed (I hope)

    const eMDLVersion ver = GetModelVersionFromAsset(pakAsset);
    switch (ver)
    {
    case eMDLVersion::VERSION_8:
    {
        ModelAssetHeader_v8_t* hdr = reinterpret_cast<ModelAssetHeader_v8_t*>(pakAsset->header());
        mdlAsset = new ModelAsset(hdr, streamEntry, ver);

        ParseModelBoneData_v8(mdlAsset->GetParsedData());
        ParseModelTextureData_v8(mdlAsset->GetParsedData());
        ParseModelVertexData_v8(pakAsset, mdlAsset);
        ParseModelSequenceData_NoStall(mdlAsset->GetParsedData(), reinterpret_cast<char* const>(mdlAsset->data));
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
        ParseModelTextureData_v8(mdlAsset->GetParsedData());
        ParseModelVertexData_v9(pakAsset, mdlAsset);
        ParseModelSequenceData_NoStall(mdlAsset->GetParsedData(), reinterpret_cast<char* const>(mdlAsset->data));
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
        ParseModelTextureData_v8(mdlAsset->GetParsedData());
        ParseModelVertexData_v12_1(pakAsset, mdlAsset);
        ParseModelSequenceData_Stall<r5::mstudioseqdesc_v8_t>(mdlAsset->GetParsedData(), reinterpret_cast<char* const>(mdlAsset->data));
        break;
    }
    case eMDLVersion::VERSION_13:
    case eMDLVersion::VERSION_13_1:
    {
        ModelAssetHeader_v13_t* hdr = reinterpret_cast<ModelAssetHeader_v13_t*>(pakAsset->header());
        mdlAsset = new ModelAsset(hdr, streamEntry, ver);

        ParseModelBoneData_v12_1(mdlAsset->GetParsedData());
        ParseModelTextureData_v8(mdlAsset->GetParsedData());
        ParseModelVertexData_v12_1(pakAsset, mdlAsset);
        ParseModelSequenceData_Stall<r5::mstudioseqdesc_v8_t>(mdlAsset->GetParsedData(), reinterpret_cast<char* const>(mdlAsset->data));
        break;
    }
    case eMDLVersion::VERSION_14:
    case eMDLVersion::VERSION_14_1:
    case eMDLVersion::VERSION_15:
    {
        ModelAssetHeader_v13_t* hdr = reinterpret_cast<ModelAssetHeader_v13_t*>(pakAsset->header());
        mdlAsset = new ModelAsset(hdr, streamEntry, ver);

        ParseModelBoneData_v12_1(mdlAsset->GetParsedData());
        ParseModelTextureData_v8(mdlAsset->GetParsedData());
        ParseModelVertexData_v14(pakAsset, mdlAsset);
        ParseModelSequenceData_Stall<r5::mstudioseqdesc_v8_t>(mdlAsset->GetParsedData(), reinterpret_cast<char* const>(mdlAsset->data));
        break;
    }
    case eMDLVersion::VERSION_16:
    case eMDLVersion::VERSION_17:
    {
        ModelAssetHeader_v16_t* hdr = reinterpret_cast<ModelAssetHeader_v16_t*>(pakAsset->header());
        ModelAssetCPU_v16_t* cpu = reinterpret_cast<ModelAssetCPU_v16_t*>(pakAsset->cpu());
        mdlAsset = new ModelAsset(hdr, cpu, streamEntry, ver);

        ParseModelBoneData_v16(mdlAsset->GetParsedData());
        ParseModelTextureData_v16(mdlAsset->GetParsedData());
        ParseModelVertexData_v16(pakAsset, mdlAsset);
        ParseModelSequenceData_Stall<r5::mstudioseqdesc_v16_t>(mdlAsset->GetParsedData(), reinterpret_cast<char* const>(mdlAsset->data));
        break;
    }
    case eMDLVersion::VERSION_18:
    {
        ModelAssetHeader_v16_t* hdr = reinterpret_cast<ModelAssetHeader_v16_t*>(pakAsset->header());
        ModelAssetCPU_v16_t* cpu = reinterpret_cast<ModelAssetCPU_v16_t*>(pakAsset->cpu());
        mdlAsset = new ModelAsset(hdr, cpu, streamEntry, ver);

        ParseModelBoneData_v16(mdlAsset->GetParsedData());
        ParseModelTextureData_v16(mdlAsset->GetParsedData());
        ParseModelVertexData_v16(pakAsset, mdlAsset);
        ParseModelSequenceData_Stall<r5::mstudioseqdesc_v18_t>(mdlAsset->GetParsedData(), reinterpret_cast<char* const>(mdlAsset->data));
        break;
    }
    case eMDLVersion::VERSION_19:
    {
        ModelAssetHeader_v16_t* hdr = reinterpret_cast<ModelAssetHeader_v16_t*>(pakAsset->header());
        ModelAssetCPU_v16_t* cpu = reinterpret_cast<ModelAssetCPU_v16_t*>(pakAsset->cpu());
        mdlAsset = new ModelAsset(hdr, cpu, streamEntry, ver);

        ParseModelBoneData_v19(mdlAsset->GetParsedData());
        ParseModelTextureData_v16(mdlAsset->GetParsedData());
        ParseModelVertexData_v16(pakAsset, mdlAsset);
        ParseModelSequenceData_Stall<r5::mstudioseqdesc_v18_t>(mdlAsset->GetParsedData(), reinterpret_cast<char* const>(mdlAsset->data));
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

    ModelAsset* const mdlAsset = reinterpret_cast<ModelAsset*>(pakAsset->extraData());
    if (!mdlAsset)
        return;

    // parse sequences for children
    const uint64_t* guids = reinterpret_cast<const uint64_t*>(mdlAsset->animSeqs);

    for (uint16_t seqIdx = 0; seqIdx < mdlAsset->numAnimSeqs; seqIdx++)
    {
        const uint64_t guid = guids[seqIdx];

        CPakAsset* const animSeqAsset = g_assetData.FindAssetByGUID<CPakAsset>(guid);

        if (nullptr == animSeqAsset)
            continue;

        AnimSeqAsset* const animSeq = reinterpret_cast<AnimSeqAsset* const>(animSeqAsset->extraData());

        if (nullptr == animSeq)
            continue;

        animSeq->parentModel = !animSeq->parentModel ? mdlAsset : animSeq->parentModel;
    }

    // draw data
    ModelParsedData_t* const parsedData = mdlAsset->GetParsedData();

    if (parsedData->lods.empty())
        return;

    const size_t lodLevel = 0;

    if (!mdlAsset->drawData)
    {
        mdlAsset->drawData = new CDXDrawData();
        mdlAsset->drawData->meshBuffers.resize(parsedData->lods.at(lodLevel).meshes.size());
        mdlAsset->drawData->modelName = mdlAsset->name;
    }

    ParseModelDrawData(mdlAsset->GetParsedData(), mdlAsset->drawData, lodLevel);
}

static bool ExportRawModelAsset(const ModelAsset* const modelAsset, std::filesystem::path& exportPath, const char* const streamedData)
{
    // Is asset permanent or streamed?
    const char* const pDataBuffer = streamedData ? streamedData : modelAsset->vertDataPermanent;

    const studiohdr_generic_t& studiohdr = modelAsset->StudioHdr();

    StreamIO studioOut(exportPath.string(), eStreamIOMode::Write);
    studioOut.write(reinterpret_cast<char*>(modelAsset->data), studiohdr.length);

    if (studiohdr.phySize > 0)
    {
        exportPath.replace_extension(".phy");

        // if we error here something is broken with setting up the model asset
        StreamIO physOut(exportPath.string(), eStreamIOMode::Write);
        physOut.write(reinterpret_cast<char*>(modelAsset->physics), studiohdr.phySize);
    }

    // make a manifest of this assets dependencies
    exportPath.replace_extension(".rson");

    StreamIO depOut(exportPath.string(), eStreamIOMode::Write);
    WriteRSONDependencyArray(*depOut.W(), "rigs", modelAsset->animRigs, modelAsset->numAnimRigs);
    WriteRSONDependencyArray(*depOut.W(), "seqs", modelAsset->animSeqs, modelAsset->numAnimSeqs);
    depOut.close();

    // [rika]: should we export both starpak and rpak data?
    switch (modelAsset->version)
    {
    case eMDLVersion::VERSION_8:
    {
        // vvd
        exportPath.replace_extension(".vvd");

        StreamIO vertOut(exportPath.string(), eStreamIOMode::Write);
        vertOut.write(pDataBuffer + studiohdr.vvdOffset, studiohdr.vvdSize);

        // vvc
        if (studiohdr.vvcSize)
        {
            exportPath.replace_extension(".vvc");

            StreamIO vertColorOut(exportPath.string(), eStreamIOMode::Write);
            vertColorOut.write(pDataBuffer + studiohdr.vvcOffset, studiohdr.vvcSize);
        }

        // vvw
        if (studiohdr.vvwSize)
        {
            exportPath.replace_extension(".vvw");

            StreamIO vertWeightOut(exportPath.string(), eStreamIOMode::Write);
            vertWeightOut.write(pDataBuffer + studiohdr.vvwOffset, studiohdr.vvwSize);
        }

        // vtx
        exportPath.replace_extension(".dx11.vtx"); // cope

        // 'opt' being optimized
        StreamIO vertOptOut(exportPath.string(), eStreamIOMode::Write);
        vertOptOut.write(pDataBuffer, studiohdr.vtxSize); // [rika]: 

        return true;
    }
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
        exportPath.replace_extension(".vg");

        StreamIO hwOut(exportPath.string(), eStreamIOMode::Write);
        hwOut.write(pDataBuffer, studiohdr.hwDataSize);

        return true;
    }
    case eMDLVersion::VERSION_16:
    case eMDLVersion::VERSION_17:
    case eMDLVersion::VERSION_18:
    case eMDLVersion::VERSION_19:
    {
        // special case because of compression
        exportPath.replace_extension(".vg");

        std::unique_ptr<char[]> hwBufTmp = std::make_unique<char[]>(studiohdr.hwDataSize);
        char* pPos = hwBufTmp.get(); // position in the decompressed buffer for writing

        for (int i = 0; i < studiohdr.groupCount; i++)
        {
            const studio_hw_groupdata_t& group = studiohdr.groups[i];

            switch (group.dataCompression)
            {
            case eCompressionType::NONE:
            {
                memcpy_s(pPos, group.dataSizeDecompressed, pDataBuffer + group.dataOffset, group.dataSizeDecompressed);
                break;
            }
            case eCompressionType::PAKFILE:
            case eCompressionType::SNOWFLAKE:
            case eCompressionType::OODLE:
            {
                std::unique_ptr<char[]> dcmpBuf = std::make_unique<char[]>(group.dataSizeCompressed);

                memcpy_s(dcmpBuf.get(), group.dataSizeCompressed, pDataBuffer + group.dataOffset, group.dataSizeCompressed);

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
        hwOut.write(hwBufTmp.get(), studiohdr.hwDataSize);

        return true;
    }
    default:
        assertm(false, "Asset version not handled.");
        return false;
    }

    unreachable();
}

template <typename phyheader_t>
static bool ExportPhysicsModelPhy(const ModelAsset* const modelAsset, std::filesystem::path& exportPath)
{
    const studiohdr_generic_t& hdr = modelAsset->StudioHdr();

    if (!hdr.phySize)
        return false;

    const int mask = (hdr.contents & g_ExportSettings.exportPhysicsContentsFilter);
    const bool inFilter = g_ExportSettings.exportPhysicsFilterAND ? mask == static_cast<int>(g_ExportSettings.exportPhysicsContentsFilter) : mask != 0;

    const bool skip = g_ExportSettings.exportPhysicsFilterExclusive ? inFilter : !inFilter;

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
        data.maskFilter = g_ExportSettings.exportPhysicsContentsFilter;
        data.filterExclusive = g_ExportSettings.exportPhysicsFilterExclusive;
        data.filterAND = g_ExportSettings.exportPhysicsFilterAND;

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

static const char* const s_PathPrefixMDL = s_AssetTypePaths.find(AssetType_t::MDL_)->second;
bool ExportModelAsset(CAsset* const asset, const int setting)
{
    CPakAsset* const pakAsset = static_cast<CPakAsset*>(asset);
    assertm(pakAsset, "Asset should be valid.");

    const ModelAsset* const modelAsset = reinterpret_cast<ModelAsset*>(pakAsset->extraData());
    assertm(modelAsset, "Extra data should be valid at this point.");
    if (!modelAsset)
        return false;

    std::unique_ptr<char[]> streamedData = pakAsset->getStarPakData(modelAsset->vertDataStreamed.offset, modelAsset->vertDataStreamed.size, false);

    assertm(modelAsset->name, "No name for model.");

    // Create exported path + asset path.
    std::filesystem::path exportPath = std::filesystem::current_path().append(EXPORT_DIRECTORY_NAME);
    const std::filesystem::path modelPath(modelAsset->name);
    const std::string modelStem(modelPath.stem().string());

    // truncate paths?
    if (g_ExportSettings.exportPathsFull)
        exportPath.append(modelPath.parent_path().string());
    else
        exportPath.append(std::format("{}/{}", s_PathPrefixMDL, modelStem));

    if (!CreateDirectories(exportPath))
    {
        assertm(false, "Failed to create asset directory.");
        return false;
    }

    const ModelParsedData_t* const parsedData = &modelAsset->parsedData;

    if (g_ExportSettings.exportRigSequences && modelAsset->numAnimSeqs > 0)
    {
        if (!ExportAnimSeqFromAsset(exportPath, modelStem, modelAsset->name, modelAsset->numAnimSeqs, modelAsset->animSeqs, modelAsset->GetRig()))
            return false;
    }

    if (g_ExportSettings.exportRigSequences && parsedData->NumLocalSeq() > 0)
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
            const seqdesc_t* const seqdesc = parsedData->LocalSeq(i);

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
            return ExportModelSMD(parsedData, exportPath);
        }
        case eModelExportSetting::MODEL_STL_VALVE_PHYSICS:
        {
            if (modelAsset->version >= eMDLVersion::VERSION_16)
                return ExportPhysicsModelPhy<irps::phyheader_v16_t>(modelAsset, exportPath);
            else
                return ExportPhysicsModelPhy<irps::phyheader_v8_t>(modelAsset, exportPath);
        }
        case eModelExportSetting::MODEL_STL_RESPAWN_PHYSICS:
        {
            // [amos]: the high detail bvh4 mesh seems encased in a mesh that is
            // more or less identical to the vphysics one. The polygon winding
            // order of the vphysics replica is however always inverted.
            if (modelAsset->version >= eMDLVersion::VERSION_12_1)
                return ExportPhysicsModelBVH<r5::mstudiocollmodel_v8_t, r5::mstudiocollheader_v12_1_t>(modelAsset, exportPath);
            else
                return ExportPhysicsModelBVH<r5::mstudiocollmodel_v8_t, r5::mstudiocollheader_v8_t>(modelAsset, exportPath);
        }
        default:
        {
            assertm(false, "Export setting is not handled.");
            return false;
        }
    }

    unreachable();
}

void* PreviewModelAsset(CAsset* const asset, const bool firstFrameForAsset)
{
    CPakAsset* const pakAsset = static_cast<CPakAsset*>(asset);
    assertm(pakAsset, "Asset should be valid.");

    ModelAsset* const modelAsset = reinterpret_cast<ModelAsset*>(pakAsset->extraData());
    assertm(modelAsset, "Extra data should be valid at this point.");
    if (!modelAsset)
        return nullptr;

    CDXDrawData* const drawData = modelAsset->drawData;
    if (!drawData)
        return nullptr;

    drawData->vertexShader = g_dxHandler->GetShaderManager()->LoadShaderFromString("shaders/model_vs", s_PreviewVertexShader, eShaderType::Vertex);;
    drawData->pixelShader = g_dxHandler->GetShaderManager()->LoadShaderFromString("shaders/model_ps", s_PreviewPixelShader, eShaderType::Pixel);

    ModelParsedData_t* const parsedData = &modelAsset->parsedData;

    static std::vector<size_t> bodygroupModelSelected;
    static size_t lastSelectedBodypartIndex = 0;
    static size_t selectedBodypartIndex = 0;

    static size_t selectedSkinIndex = 0;
    static size_t lastSelectedSkinIndex = 0;

    static size_t lodLevel = 0;

    if (firstFrameForAsset)
    {
        bodygroupModelSelected.clear();

        bodygroupModelSelected.resize(parsedData->bodyParts.size(), 0ull);

        selectedBodypartIndex = selectedBodypartIndex > parsedData->bodyParts.size() ? 0 : selectedBodypartIndex;
        selectedSkinIndex = selectedSkinIndex > parsedData->skins.size() ? 0 : selectedSkinIndex;
    }

    assertm(parsedData->lods.size() > 0, "no lods in preview?");
    const ModelLODData_t& lodData = parsedData->lods.at(lodLevel);

    ImGui::Text("Bones: %llu", parsedData->bones.size());
    ImGui::Text("LODs: %llu", parsedData->lods.size());
    ImGui::Text("Rigs: %i", modelAsset->numAnimRigs);
    ImGui::Text("Sequences: %i", modelAsset->numAnimSeqs);
    ImGui::Text("Local Sequences: %i", modelAsset->parsedData.NumLocalSeq());

    if (parsedData->skins.size())
    {
        ImGui::TextUnformatted("Skins:");
        ImGui::SameLine();

        // [rika]: cheat a little here since the first skin name should always be 'STUDIO_DEFAULT_SKIN_NAME'
        static const char* label = nullptr;
        if (firstFrameForAsset)
            label = STUDIO_DEFAULT_SKIN_NAME;

        if (ImGui::BeginCombo("##SKins", label, ImGuiComboFlags_NoArrowButton))
        {
            for (size_t i = 0; i < parsedData->skins.size(); i++)
            {
                const ModelSkinData_t& skin = parsedData->skins.at(i);

                const bool isSelected = selectedSkinIndex == i || (firstFrameForAsset && selectedSkinIndex == lastSelectedSkinIndex);

                if (ImGui::Selectable(skin.name, isSelected))
                {
                    selectedSkinIndex = i;
                    label = skin.name;
                }

                if (isSelected) ImGui::SetItemDefaultFocus();
            }

            ImGui::EndCombo();
        }
    }

    g_ExportSettings.previewedSkinIndex = static_cast<int>(selectedSkinIndex);

    if (parsedData->bodyParts.size())
    {
        ImGui::TextUnformatted("Bodypart:");
        ImGui::SameLine();

        // [rika]: previous implemention was loading an invalid string upon loading different files without closing the tool
        static const char* bodypartLabel = nullptr;
        if (firstFrameForAsset)
            bodypartLabel = parsedData->bodyParts.at(0).GetNameCStr();

        if (ImGui::BeginCombo("##Bodypart", bodypartLabel, ImGuiComboFlags_NoArrowButton))
        {
            for (size_t i = 0; i < parsedData->bodyParts.size(); i++)
            {
                const ModelBodyPart_t& bodypart = parsedData->bodyParts.at(i);

                const bool isSelected = selectedBodypartIndex == i || (firstFrameForAsset && selectedBodypartIndex == lastSelectedBodypartIndex);

                if (ImGui::Selectable(bodypart.GetNameCStr(), isSelected))
                {
                    selectedBodypartIndex = i;
                    bodypartLabel = bodypart.GetNameCStr();
                }

                if (isSelected) ImGui::SetItemDefaultFocus();
            }

            ImGui::EndCombo();
        }

        if (parsedData->bodyParts.at(selectedBodypartIndex).numModels > 1)
        {
            const ModelBodyPart_t& bodypart = parsedData->bodyParts.at(selectedBodypartIndex);
            size_t& selectedModelIndex = bodygroupModelSelected.at(selectedBodypartIndex);

            ImGui::TextUnformatted("Model:");
            ImGui::SameLine();

            static const char* label = nullptr;

            // update label if our bodypart changes
            if (selectedBodypartIndex != lastSelectedBodypartIndex || firstFrameForAsset)
                label = lodData.models.at(bodypart.modelIndex + selectedModelIndex).name.c_str();

            if (ImGui::BeginCombo("##Model", label, ImGuiComboFlags_NoArrowButton))
            {
                for (int i = 0; i < bodypart.numModels; i++)
                {
                    const bool isSelected = selectedModelIndex == i;

                    const char* tmp = lodData.models.at(bodypart.modelIndex + i).name.c_str();
                    if (ImGui::Selectable(tmp, isSelected))
                    {
                        selectedModelIndex = static_cast<size_t>(i);
                        label = tmp;
                    }

                    if (isSelected) ImGui::SetItemDefaultFocus();
                }

                ImGui::EndCombo();
            }
        }
    }    

    /*if (modelAsset->materials.size() && modelAsset->skins.size())
    {
        ImGui::SeparatorText("Skin Materials");

        const int numSkinRefs = modelAsset->studiohdr.numSkinRef;

        int i = 0;
        for (auto& skin : modelAsset->skins)
        {
            if (ImGui::CollapsingHeader(skin.name))
            {
                constexpr ImGuiTableFlags tableFlags =
                    ImGuiTableFlags_RowBg | ImGuiTableFlags_Borders | ImGuiTableFlags_NoBordersInBody
                    | ImGuiTableFlags_ScrollY | ImGuiTableFlags_SizingFixedFit;
                const ImVec2 outerSize = ImVec2(0.f, ImGui::GetTextLineHeightWithSpacing() * 12.f);


                if (ImGui::BeginTable((std::string("mst_") + skin.name).c_str(), 3, tableFlags, outerSize))
                {
                    ImGui::TableSetupColumn("IDX", ImGuiTableColumnFlags_NoResize | ImGuiTableColumnFlags_WidthFixed | ImGuiTableColumnFlags_NoHide, 0.f);
                    ImGui::TableSetupColumn("Material Name",ImGuiTableColumnFlags_NoResize | ImGuiTableColumnFlags_WidthFixed | ImGuiTableColumnFlags_NoHide, 0.f);
                    ImGui::TableSetupColumn("",ImGuiTableColumnFlags_NoResize | ImGuiTableColumnFlags_WidthFixed | ImGuiTableColumnFlags_NoHide, 0.f);
                    ImGui::TableSetupScrollFreeze(1, 1);


                    for (int skinRefIdx = 0; skinRefIdx < numSkinRefs; ++skinRefIdx)
                    {
                        ImGui::PushID(skinRefIdx);
                        ImGui::TableNextRow(ImGuiTableRowFlags_None, 0.f);


                        const ModelMaterialData_t& materialData = modelAsset->materials.at(skin.indices[skinRefIdx]);

                        if (ImGui::TableSetColumnIndex(0))
                        {
                            ImGui::TextUnformatted(std::to_string(i).c_str());
                        }

                        if (ImGui::TableSetColumnIndex(1))
                        {
                            ImGui::TextUnformatted(materialData.materialName.c_str());
                        }

                        if (ImGui::TableSetColumnIndex(2))
                        {
                            ImGui::TextUnformatted("");
                        }

                        ImGui::PopID();
                    }

                    ImGui::EndTable();


                }
            }

            i++;
        }
    }*/

    // [rika]: our currently selected skin
    const ModelSkinData_t& skinData = parsedData->skins.at(selectedSkinIndex);
    for (size_t i = 0; i < lodData.meshes.size(); ++i)
    {
        const ModelMeshData_t& mesh = lodData.meshes.at(i);
        DXMeshDrawData_t* const meshDrawData = &drawData->meshBuffers[i];

        // the rest of this loop requires the material to be valid
        // so if it isn't just continue to the next iteration
        CPakAsset* const matlAsset = parsedData->materials.at(skinData.indices[mesh.materialId]).asset;
        if (!matlAsset)
            continue;

        meshDrawData->indexFormat = DXGI_FORMAT_R16_UINT;

        const MaterialAsset* const matl = reinterpret_cast<MaterialAsset*>(matlAsset->extraData());

        // If this body part is disabled, don't draw the mesh.
        drawData->meshBuffers[i].visible = parsedData->bodyParts[mesh.bodyPartIndex].IsPreviewEnabled();

        const ModelBodyPart_t& bodypart = parsedData->bodyParts[mesh.bodyPartIndex];
        const ModelModelData_t& model = lodData.models.at(bodypart.modelIndex + bodygroupModelSelected.at(mesh.bodyPartIndex));
        if (i >= model.meshIndex && i < model.meshIndex + model.meshCount)
            drawData->meshBuffers[i].visible = true;
        else
            drawData->meshBuffers[i].visible = false;

#if defined(ADVANCED_MODEL_PREVIEW)
        if (matl->shaderSetAsset)
        {
            ShaderSetAsset* const shaderSet = reinterpret_cast<ShaderSetAsset*>(matl->shaderSetAsset->extraData());

            if (shaderSet->vertexShaderAsset && shaderSet->pixelShaderAsset)
            {
                ShaderAsset* const vertexShader = reinterpret_cast<ShaderAsset*>(shaderSet->vertexShaderAsset->extraData());
                ShaderAsset* const pixelShader = reinterpret_cast<ShaderAsset*>(shaderSet->pixelShaderAsset->extraData());

                meshDrawData->vertexShader = vertexShader->vertexShader;
                meshDrawData->pixelShader  = pixelShader->pixelShader;

                meshDrawData->inputLayout = vertexShader->vertexInputLayout;
            }
        }
#endif

        if ((meshDrawData->textures.size() == 0 || lastSelectedSkinIndex != selectedSkinIndex) && matl)
        {
            meshDrawData->textures.clear();
            for (auto& texEntry : matl->txtrAssets)
            {
                if (texEntry.asset)
                {
                    TextureAsset* txtr = reinterpret_cast<TextureAsset*>(texEntry.asset->extraData());
                    const std::shared_ptr<CTexture> highestTextureMip = CreateTextureFromMip(texEntry.asset, &txtr->mipArray[txtr->mipArray.size() - 1], s_PakToDxgiFormat[txtr->imgFormat]);
                    meshDrawData->textures.push_back({ texEntry.index, highestTextureMip });
                }
            }
        }
    }

    if (!drawData->boneMatrixBuffer)
        InitModelBoneMatrix(drawData, parsedData);


    if (!drawData->transformsBuffer)
    {
        D3D11_BUFFER_DESC desc{};

#if defined(ADVANCED_MODEL_PREVIEW)
        constexpr UINT transformsBufferSizeAligned = IALIGN(sizeof(CBufModelInstance), 16);
#else
        constexpr UINT transformsBufferSizeAligned = IALIGN(sizeof(VS_TransformConstants), 16);
#endif

        desc.ByteWidth = transformsBufferSizeAligned;

        // make sure this buffer can be updated every frame
        desc.Usage = D3D11_USAGE_DYNAMIC;
        desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;

        // const buffer
        desc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;

        g_dxHandler->GetDevice()->CreateBuffer(&desc, NULL, &drawData->transformsBuffer);
    }

    D3D11_MAPPED_SUBRESOURCE resource;
    g_dxHandler->GetDeviceContext()->Map(
        drawData->transformsBuffer, 0,
        D3D11_MAP_WRITE_DISCARD, 0,
        &resource
    );

    CDXCamera* const camera = g_dxHandler->GetCamera();
    const XMMATRIX view = camera->GetViewMatrix();
    const XMMATRIX model = XMMatrixTranslationFromVector(drawData->position.AsXMVector());
    const XMMATRIX projection = g_dxHandler->GetProjMatrix();

#if !defined(ADVANCED_MODEL_PREVIEW)
    VS_TransformConstants* const transforms = reinterpret_cast<VS_TransformConstants*>(resource.pData);
    transforms->modelMatrix = XMMatrixTranspose(model);
    transforms->viewMatrix = XMMatrixTranspose(view);
    transforms->projectionMatrix = XMMatrixTranspose(projection);
#else
    CBufModelInstance* const modelInstance = reinterpret_cast<CBufModelInstance*>(resource.pData);
    CBufModelInstance::ModelInstance& mi = modelInstance->c_modelInst;

    // sub_18001FB40 in r2
    XMMATRIX modelMatrix = {};
    memcpy(&modelMatrix, &modelAsset->bones[0].poseToBone, sizeof(ModelBone_t::poseToBone));

    modelMatrix.r[3].m128_f32[3] = 1.f;

    mi.objectToCameraRelativePrevFrame = mi.objectToCameraRelative;

    modelMatrix.r[0].m128_f32[3] += camera->position.x;
    modelMatrix.r[1].m128_f32[3] += camera->position.y;
    modelMatrix.r[2].m128_f32[3] += camera->position.z;

    modelMatrix -= XMMatrixRotationRollPitchYaw(camera->rotation.x, camera->rotation.y, camera->rotation.z);

    modelMatrix = XMMatrixTranspose(modelMatrix);

    mi.diffuseModulation = { 1.f, 1.f, 1.f, 1.f };

    XMStoreFloat3x4(&mi.objectToCameraRelative, modelMatrix);
    XMStoreFloat3x4(&mi.objectToCameraRelativePrevFrame, modelMatrix);

#endif

    if (lastSelectedBodypartIndex != selectedBodypartIndex)
        lastSelectedBodypartIndex = selectedBodypartIndex;
    
    if (lastSelectedSkinIndex != selectedSkinIndex)
        lastSelectedSkinIndex = selectedSkinIndex;

    g_dxHandler->GetDeviceContext()->Unmap(drawData->transformsBuffer, 0);

    return modelAsset->drawData;
}

void InitModelAssetType()
{
    AssetTypeBinding_t type =
    {
        .type = '_ldm',
        .headerAlignment = 8,
        .loadFunc = LoadModelAsset,
        .postLoadFunc = PostLoadModelAsset,
        .previewFunc = PreviewModelAsset,
        .e = { ExportModelAsset, 0, s_ModelExportSettingNames, ARRSIZE(s_ModelExportSettingNames) },
    };

    REGISTER_TYPE(type);
}