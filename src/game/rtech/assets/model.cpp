#include <pch.h>

#include <game/rtech/assets/model.h>
#include <game/rtech/assets/texture.h>
#include <game/rtech/assets/material.h>
#include <game/rtech/assets/rson.h>
#include <game/rtech/assets/animseq.h>
#include <game/rtech/utils/bvh/bvh.h>
#include <game/rtech/utils/bsp/bspflags.h>

#include <game/model/sourcemodel.h>

#include <core/mdl/rmax.h>
#include <core/mdl/stringtable.h>
#include <core/mdl/cast.h>

#include <core/render/dx.h>
#include <thirdparty/imgui/imgui.h>
#include <thirdparty/imgui/misc/imgui_utility.h>

#include <immintrin.h>

extern CDXParentHandler* g_dxHandler;
extern CBufferManager* g_BufferManager;
extern ExportSettings_t g_ExportSettings;

#define VERT_DATA(t, d, o) reinterpret_cast<const t* const>(d + o)
inline void ParseVertexFromVG(Vertex_t* const vert, VertexWeight_t* const weights, Vector2D* const texcoords, ModelMeshData_t* const mesh, const char* const rawVertexData, const uint8_t* const boneMap, const vvw::mstudioboneweightextra_t* const weightExtra, int& weightIdx)
{
    int offset = 0;

    if (mesh->rawVertexLayoutFlags & VERT_POSITION_UNPACKED)
    {
        vert->position = *VERT_DATA(Vector, rawVertexData, offset);
        offset += sizeof(Vector);
    }

    if (mesh->rawVertexLayoutFlags & VERT_POSITION_PACKED)
    {
        vert->position = VERT_DATA(Vector64, rawVertexData, offset)->Unpack();
        offset += sizeof(Vector64);
    }

    assertm(nullptr != weights, "weight pointer should be valid");
    vert->weightIndex = weightIdx;

    // we have weight data
    // note: if for some reason 'VERT_BLENDWEIGHTS_UNPACKED' is encountered, weights would not be processed.
    assertm(!(mesh->rawVertexLayoutFlags & VERT_BLENDWEIGHTS_UNPACKED), "mesh had unpacked weights!");
    if (mesh->rawVertexLayoutFlags & (VERT_BLENDINDICES | VERT_BLENDWEIGHTS_PACKED))
    {
        const vg::BlendWeightsPacked_s* const blendWeights = VERT_DATA(vg::BlendWeightsPacked_s, rawVertexData, offset);
        const vg::BlendWeightIndices_s* const blendIndices = VERT_DATA(vg::BlendWeightIndices_s, rawVertexData, offset + 4);

        offset += 8;

        uint8_t curIdx = 0; // current weight
        uint16_t remaining = 32767; // 'weight' remaining to assign to the last bone

        // model has more than 3 weights per vertex
        if (nullptr != weightExtra)
        {
            assertm(blendIndices->boneCount < 16, "model had more than 16 bones on complex weights");

            // first weight, we will always have this
            weights[curIdx].bone = boneMap[blendIndices->bone[0]];
            weights[curIdx].weight = blendWeights->Weight(0);
            remaining -= blendWeights->weight[0];

            curIdx++;

            // only hit if we have over 2 bones/weights
            for (uint8_t i = curIdx; i < blendIndices->boneCount; i++)
            {
                weights[curIdx].bone = boneMap[weightExtra[blendWeights->Index() + (curIdx - 1)].bone];
                weights[curIdx].weight = weightExtra[blendWeights->Index() + (curIdx - 1)].Weight();

                remaining -= weightExtra[blendWeights->Index() + (curIdx - 1)].weight;

                curIdx++;
            }

            // only hit if we have over 1 bone/weight
            if (blendIndices->boneCount > 0)
            {
                weights[curIdx].bone = boneMap[blendIndices->bone[1]];
                weights[curIdx].weight = UNPACKWEIGHT(remaining);

                curIdx++;
            }
        }
        else
        {
            assertm(blendIndices->boneCount < 3, "model had more than 3 bones on simple weights");

            for (uint8_t i = 0; i < blendIndices->boneCount; i++)
            {
                weights[curIdx].bone = boneMap[blendIndices->bone[curIdx]];
                weights[curIdx].weight = blendWeights->Weight(curIdx);

                remaining -= blendWeights->weight[curIdx];

                curIdx++;
            }

            weights[curIdx].bone = boneMap[blendIndices->bone[curIdx]];
            weights[curIdx].weight = UNPACKWEIGHT(remaining);

            curIdx++;
        }

        vert->weightCount = curIdx;
        assert(static_cast<uint8_t>(vert->weightCount) == (blendIndices->boneCount + 1)); // numbones is really 'extra' bones on top of the base weight, verify the count is correct

        weightIdx += curIdx;
    }
    // our mesh does not have weight data, use a set of default weights. 
    // [rika]: this can only happen when a model has one bone
    else
    {
        vert->weightCount = 1;
        weights[0].bone = 0;
        weights[0].weight = 1.0f;

        weightIdx++;
    }

    mesh->weightsPerVert = static_cast<uint16_t>(vert->weightCount) > mesh->weightsPerVert ? static_cast<uint16_t>(vert->weightCount) : mesh->weightsPerVert;

    vert->normalPacked = *VERT_DATA(Normal32, rawVertexData, offset);
    offset += sizeof(Normal32);

    if (mesh->rawVertexLayoutFlags & VERT_COLOR)
    {
        // Vertex Colour
        vert->color = *VERT_DATA(Color32, rawVertexData, offset);
        offset += sizeof(Color32);
    }
    else // no vert colour, write default
    {
        vert->color = Color32(255, 255);
    }

    if (mesh->rawVertexLayoutFlags & VERT_TEXCOORD0)
    {
        vert->texcoord = *VERT_DATA(Vector2D, rawVertexData, offset);
        offset += sizeof(Vector2D);
    }

    for (int localIdx = 1, countIdx = 1; countIdx < mesh->texcoordCount; localIdx++)
    {
        assertm(nullptr != texcoords, "texcoord pointer should be valid");

        if (!VERT_TEXCOORDn(localIdx))
            continue;

        texcoords[countIdx - 1] = *VERT_DATA(Vector2D, rawVertexData, offset);
        offset += sizeof(Vector2D);

        countIdx++;
    }
}
#undef VERT_DATA

// Generic (basic data shared between them)
inline void ParseVertexFromVTX(Vertex_t* const vert, Vector2D* const texcoords, ModelMeshData_t* const mesh, const vvd::mstudiovertex_t* const pVerts, const Vector4D* const pTangs, const Color32* const pColors, const Vector2D* const pUVs, const int origId)
{
    vert->position = pVerts[origId].m_vecPosition;

    // not normal
    vert->normalPacked.PackNormal(pVerts[origId].m_vecNormal, pTangs[origId]);

    if (pColors)
        vert->color = pColors[origId];
    else // no vert colour, write default
        vert->color = Color32(255, 255);

    vert->texcoord = pVerts[origId].m_vecTexCoord;

    for (int localIdx = 1, countIdx = 1; countIdx < mesh->texcoordCount; localIdx++)
    {
        assertm(nullptr != texcoords, "texcoord pointer should be valid");

        if (!VERT_TEXCOORDn(localIdx))
            continue;

        texcoords[countIdx - 1] = pUVs[origId]; // [rika]: add proper support for uv3 (though I doubt we'll ever get files for it, I think it would be quite quirky.)

        countIdx++;
    }
}

// Basic Source
inline void ParseVertexFromVTX(Vertex_t* const vert, VertexWeight_t* const weights, Vector2D* const texcoords, ModelMeshData_t* mesh, const OptimizedModel::Vertex_t* const pVertex, const vvd::mstudiovertex_t* const pVerts, const Vector4D* const pTangs, const Color32* const pColors, const Vector2D* const pUVs,
    int& weightIdx, const bool isHwSkinned, const OptimizedModel::BoneStateChangeHeader_t* const pBoneStates)
{
    const int origId = pVertex->origMeshVertID;

    ParseVertexFromVTX(vert, texcoords, mesh, pVerts, pTangs, pColors, pUVs, origId);

    const vvd::mstudiovertex_t& oldVert = pVerts[origId];

    assertm(nullptr != weights, "weight pointer should be valid");
    vert->weightIndex = weightIdx;
    vert->weightCount = oldVert.m_BoneWeights.numbones;

    for (int i = 0; i < oldVert.m_BoneWeights.numbones; i++)
    {
        weights[i].weight = oldVert.m_BoneWeights.weight[pVertex->boneWeightIndex[i]];

        // static props can be hardware skinned, but have no bonestates (one bone). this make it skip this, however it's a non issue as the following statement will work fine for this (there is only one bone at idx 0)
        if (isHwSkinned && pBoneStates)
        {
            weights[i].bone = static_cast<int16_t>(pBoneStates[pVertex->boneID[i]].newBoneID);
            continue;
        }

        weights[i].bone = pVertex->boneID[i];
    }

    weightIdx += oldVert.m_BoneWeights.numbones;

    mesh->weightsPerVert = static_cast<uint16_t>(vert->weightCount) > mesh->weightsPerVert ? static_cast<uint16_t>(vert->weightCount) : mesh->weightsPerVert;
}

// Apex Legends
inline void ParseVertexFromVTX(Vertex_t* const vert, VertexWeight_t* const weights, Vector2D* const texcoords, ModelMeshData_t* mesh, const OptimizedModel::Vertex_t* const pVertex, const vvd::mstudiovertex_t* const pVerts, const Vector4D* const pTangs, const Color32* const pColors, const Vector2D* const pUVs,
    const vvw::vertexBoneWeightsExtraFileHeader_t* const pVVW, int& weightIdx)
{
    const int origId = pVertex->origMeshVertID;

    ParseVertexFromVTX(vert, texcoords, mesh, pVerts, pTangs, pColors, pUVs, origId);

    const vvd::mstudiovertex_t& oldVert = pVerts[origId];

    assertm(nullptr != weights, "weight pointer should be valid");
    vert->weightIndex = weightIdx;
    vert->weightCount = oldVert.m_BoneWeights.numbones;

    if (nullptr != pVVW)
    {
        const vvw::mstudioboneweightextra_t* const pExtraWeights = pVVW->GetWeightData(oldVert.m_BoneWeights.weightextra.extraweightindex);

        for (int i = 0; i < oldVert.m_BoneWeights.numbones; i++)
        {
            if (i >= 3)
            {
                weights[i].bone = pExtraWeights[i - 3].bone;
                weights[i].weight = pExtraWeights[i - 3].Weight();

                continue;
            }

            weights[i].bone = oldVert.m_BoneWeights.bone[i];
            weights[i].weight = oldVert.m_BoneWeights.weightextra.Weight(i);
        }
    }
    else
    {
        for (int i = 0; i < oldVert.m_BoneWeights.numbones; i++)
        {
            weights[i].bone = oldVert.m_BoneWeights.bone[i];
            weights[i].weight = oldVert.m_BoneWeights.weight[i];
        }
    }

    weightIdx += oldVert.m_BoneWeights.numbones;

    mesh->weightsPerVert = static_cast<uint16_t>(vert->weightCount) > mesh->weightsPerVert ? static_cast<uint16_t>(vert->weightCount) : mesh->weightsPerVert;
}

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

    modelAsset->lods.resize(pVTX->numLODs);
    modelAsset->bodyParts.resize(pStudioHdr->numbodyparts);

    constexpr size_t maxVertexDataSize = sizeof(vvd::mstudiovertex_t) + sizeof(Vector4D) + sizeof(Vector2D) + sizeof(Color32);
    constexpr size_t maxVertexBufferSize = maxVertexDataSize * s_MaxStudioVerts;

    // needed due to how vtx is parsed!
    CManagedBuffer* const   parseBuf = g_BufferManager->ClaimBuffer();

    Vertex_t* const         parseVertices   = reinterpret_cast<Vertex_t*>       (parseBuf->Buffer() + maxVertexBufferSize);
    Vector2D* const         parseTexcoords  = reinterpret_cast<Vector2D*>       (&parseVertices[s_MaxStudioVerts]);
    uint16_t* const         parseIndices    = reinterpret_cast<uint16_t*>       (&parseTexcoords[s_MaxStudioVerts * 2]);
    VertexWeight_t* const   parseWeights    = reinterpret_cast<VertexWeight_t*> (&parseIndices[s_MaxStudioTriangles]); // ~8mb for weights

    for (int lodIdx = 0; lodIdx < pVTX->numLODs; lodIdx++)
    {
        int lodMeshCount = 0;

        ModelLODData_t& lodData = modelAsset->lods.at(lodIdx);

        for (int bdyIdx = 0; bdyIdx < pStudioHdr->numbodyparts; bdyIdx++)
        {
            const mstudiobodyparts_t* const pStudioBodyPart = pStudioHdr->pBodypart(bdyIdx);
            const OptimizedModel::BodyPartHeader_t* const pVertBodyPart = pVTX->pBodyPart(bdyIdx);

            modelAsset->SetupBodyPart(bdyIdx, pStudioBodyPart->pszName(), static_cast<int>(lodData.models.size()), pStudioBodyPart->nummodels);

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
                    CManagedBuffer* const buffer = g_BufferManager->ClaimBuffer();

                    CMeshData* meshVertexData = reinterpret_cast<CMeshData*>(buffer->Buffer());
                    meshVertexData->InitWriter();

                    ModelMeshData_t& meshData = lodData.meshes.at(lodMeshCount);

                    meshData.bodyPartIndex = bdyIdx;

                    // is this correct?
                    meshData.rawVertexLayoutFlags |= (VERT_LEGACY | (pStudioHdr->flags & STUDIOHDR_FLAGS_USES_VERTEX_COLOR ? VERT_COLOR : 0x0));
                    meshData.vertCacheSize = static_cast<uint16_t>(pVTX->vertCacheSize);

                    // do we have a section texcoord
                    meshData.rawVertexLayoutFlags |= modelAsset->studiohdr.flags & STUDIOHDR_FLAGS_USES_UV2 ? VERT_TEXCOORDn_FMT(2, 0x2) : 0x0;

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
                                ParseVertexFromVTX(&parseVertices[pStrip->vertOffset + vertIdx], &parseWeights[weightIdx], texcoords, &meshData, pVert, verts, tangs, colors, uv2s, pVVW, weightIdx);
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

                    meshData.ParseMaterial(modelAsset, pStudioMesh->material, pStudioMesh->meshid);

                    lodMeshCount++;
                    modelData.meshCount++;

                    // for export
                    lodData.weightsPerVert = meshData.weightsPerVert > lodData.weightsPerVert ? meshData.weightsPerVert : lodData.weightsPerVert;
                    lodData.texcoordsPerVert = meshData.texcoordCount > lodData.texcoordsPerVert ? meshData.texcoordCount : lodData.texcoordsPerVert;

                    // remove it from usage
                    meshVertexData->DestroyWriter();

                    meshData.meshVertexDataIndex = modelAsset->meshVertexData.size();
                    modelAsset->meshVertexData.addBack(reinterpret_cast<char*>(meshVertexData), meshVertexData->GetSize());

                    // relieve buffer
                    g_BufferManager->RelieveBuffer(buffer);
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

    g_BufferManager->RelieveBuffer(parseBuf);
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

    modelAsset->studiohdr.hwDataSize = vgHdr->dataSize; // [rika]: set here, makes things easier. if we use the value from ModelAssetHeader it will be aligned 4096, making it slightly oversized.
    modelAsset->lods.resize(vgHdr->lodCount);

    // group setup
    {
        studio_hw_groupdata_t& group = modelAsset->studiohdr.groups[0];

        group.dataOffset = 0;
        group.dataSizeCompressed = -1;
        group.dataSizeDecompressed = vgHdr->dataSize;
        group.dataCompression = eCompressionType::NONE;

        group.lodIndex = 0;
        group.lodCount = static_cast<uint8_t>(vgHdr->lodCount);
        group.lodMap = 0xff >> (8 - vgHdr->lodCount);

    }

    const r5::studiohdr_v8_t* const pStudioHdr = reinterpret_cast<r5::studiohdr_v8_t*>(modelAsset->data);

    modelAsset->bodyParts.resize(pStudioHdr->numbodyparts);
    modelAsset->meshVertexData.resize(vgHdr->meshCount);

    const uint8_t* boneMap = vgHdr->boneStateChangeCount ? vgHdr->pBoneMap() : s_VertexDataBaseBoneMap; // does this model have remapped bones? use default map if not

    for (int lodLevel = 0; lodLevel < vgHdr->lodCount; lodLevel++)
    {
        int lodMeshCount = 0;

        ModelLODData_t& lodData = modelAsset->lods.at(lodLevel);
        const vg::rev1::ModelLODHeader_t* const lod = vgHdr->pLOD(lodLevel);

        lodData.switchPoint = lod->switchPoint;
        lodData.meshes.resize(lod->meshCount);

        for (int bdyIdx = 0; bdyIdx < pStudioHdr->numbodyparts; bdyIdx++)
        {
            const mstudiobodyparts_t* const pBodypart = pStudioHdr->pBodypart(bdyIdx);

            modelAsset->SetupBodyPart(bdyIdx, pBodypart->pszName(), static_cast<int>(lodData.models.size()), pBodypart->nummodels);

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
                    CManagedBuffer* const buffer = g_BufferManager->ClaimBuffer();

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
                        ParseVertexFromVG(&meshVertexData->GetVertices()[vertIdx], &meshVertexData->GetWeights()[weightIdx], texcoords, &meshData, vertexData, boneMap, weights, weightIdx);
                    }
                    meshData.weightsCount = weightIdx;
                    meshVertexData->AddWeights(nullptr, meshData.weightsCount);

                    meshData.ParseMaterial(modelAsset, pMesh->material, pMesh->meshid);

                    lodMeshCount++;
                    modelData.meshCount++;

                    // for export
                    lodData.weightsPerVert = meshData.weightsPerVert > lodData.weightsPerVert ? meshData.weightsPerVert : lodData.weightsPerVert;
                    lodData.texcoordsPerVert = meshData.texcoordCount > lodData.texcoordsPerVert ? meshData.texcoordCount : lodData.texcoordsPerVert;

                    // remove it from usage
                    meshVertexData->DestroyWriter();

                    meshData.meshVertexDataIndex = modelAsset->meshVertexData.size();
                    modelAsset->meshVertexData.addBack(reinterpret_cast<char*>(meshVertexData), meshVertexData->GetSize());

                    // relieve buffer
                    g_BufferManager->RelieveBuffer(buffer);
                }

                lodData.models.push_back(modelData);
            }
        }

        // [rika]: to remove excess meshes (empty meshes we skipped, since we set size at the beginning). this should only deallocate memory
        lodData.meshes.resize(lodMeshCount);
    }

    modelAsset->meshVertexData.shrink();
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

    const r5::studiohdr_v12_1_t* const pStudioHdr = reinterpret_cast<r5::studiohdr_v12_1_t*>(modelAsset->data);

    const uint8_t* boneMap = pStudioHdr->boneStateCount ? pStudioHdr->pBoneStates() : s_VertexDataBaseBoneMap; // does this model have remapped bones? use default map if not

    modelAsset->lods.resize(pStudioHdr->lodCount);
    modelAsset->bodyParts.resize(pStudioHdr->numbodyparts);

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
            ModelLODData_t& lodData = modelAsset->lods.at(lodLevel);
            lodData.switchPoint = lod->switchPoint;

            modelAsset->meshVertexData.resize(modelAsset->meshVertexData.size() + lod->meshCount);

            // [rika]: this should only get hit once per LOD
            const size_t curMeshCount = lodData.meshes.size();
            lodData.meshes.resize(curMeshCount + lod->meshCount);

            for (uint16_t bdyIdx = 0; bdyIdx < pStudioHdr->numbodyparts; bdyIdx++)
            {
                const mstudiobodyparts_t* const pBodypart = pStudioHdr->pBodypart(bdyIdx);

                modelAsset->SetupBodyPart(bdyIdx, pBodypart->pszName(), static_cast<int>(lodData.models.size()), pBodypart->nummodels);

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
                        CManagedBuffer* const buffer = g_BufferManager->ClaimBuffer();

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
                            ParseVertexFromVG(&meshVertexData->GetVertices()[vertIdx], &meshVertexData->GetWeights()[weightIdx], texcoords, &meshData, vertexData, boneMap, weights, weightIdx);
                        }
                        meshData.weightsCount = weightIdx;
                        meshVertexData->AddWeights(nullptr, meshData.weightsCount);

                        meshData.ParseMaterial(modelAsset, pMesh->material, pMesh->meshid);

                        lodMeshCount[lodLevel]++;
                        modelData.meshCount++;

                        // for export
                        lodData.weightsPerVert = meshData.weightsPerVert > lodData.weightsPerVert ? meshData.weightsPerVert : lodData.weightsPerVert;
                        lodData.texcoordsPerVert = meshData.texcoordCount > lodData.texcoordsPerVert ? meshData.texcoordCount : lodData.texcoordsPerVert;

                        // remove it from usage
                        meshVertexData->DestroyWriter();

                        meshData.meshVertexDataIndex = modelAsset->meshVertexData.size();
                        modelAsset->meshVertexData.addBack(reinterpret_cast<char*>(meshVertexData), meshVertexData->GetSize());

                        // relieve buffer
                        g_BufferManager->RelieveBuffer(buffer);
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

    modelAsset->meshVertexData.shrink();
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

    const r5::studiohdr_v14_t* const pStudioHdr = reinterpret_cast<r5::studiohdr_v14_t*>(modelAsset->data);

    const uint8_t* boneMap = pStudioHdr->boneStateCount ? pStudioHdr->pBoneStates() : s_VertexDataBaseBoneMap; // does this model have remapped bones? use default map if not

    modelAsset->lods.resize(pStudioHdr->lodCount);
    modelAsset->bodyParts.resize(pStudioHdr->numbodyparts);

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
            ModelLODData_t& lodData = modelAsset->lods.at(lodLevel);
            lodData.switchPoint = lod->switchPoint;

            modelAsset->meshVertexData.resize(modelAsset->meshVertexData.size() + lod->meshCount);

            // [rika]: this should only get hit once per LOD
            const size_t curMeshCount = lodData.meshes.size();
            lodData.meshes.resize(curMeshCount + lod->meshCount);

            for (uint16_t bdyIdx = 0; bdyIdx < pStudioHdr->numbodyparts; bdyIdx++)
            {
                const mstudiobodyparts_t* const pBodypart = modelAsset->version == eMDLVersion::VERSION_15 ? pStudioHdr->pBodypart_V15(bdyIdx)->AsV8() : pStudioHdr->pBodypart(bdyIdx);

                modelAsset->SetupBodyPart(bdyIdx, pBodypart->pszName(), static_cast<int>(lodData.models.size()), pBodypart->nummodels);

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
                        CManagedBuffer* const buffer = g_BufferManager->ClaimBuffer();

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
                            ParseVertexFromVG(&meshVertexData->GetVertices()[vertIdx], &meshVertexData->GetWeights()[weightIdx], texcoords, &meshData, vertexData, boneMap, weights, weightIdx);
                        }
                        meshData.weightsCount = weightIdx;
                        meshVertexData->AddWeights(nullptr, meshData.weightsCount);

                        meshData.ParseMaterial(modelAsset, pMesh->material, pMesh->meshid);

                        lodMeshCount[lodLevel]++;
                        modelData.meshCount++;

                        // for export
                        lodData.weightsPerVert = meshData.weightsPerVert > lodData.weightsPerVert ? meshData.weightsPerVert : lodData.weightsPerVert;
                        lodData.texcoordsPerVert = meshData.texcoordCount > lodData.texcoordsPerVert ? meshData.texcoordCount : lodData.texcoordsPerVert;

                        // remove it from usage
                        meshVertexData->DestroyWriter();

                        meshData.meshVertexDataIndex = modelAsset->meshVertexData.size();
                        modelAsset->meshVertexData.addBack(reinterpret_cast<char*>(meshVertexData), meshVertexData->GetSize());

                        // relieve buffer
                        g_BufferManager->RelieveBuffer(buffer);
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

    modelAsset->meshVertexData.shrink();
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

    const r5::studiohdr_v16_t* const pStudioHdr = reinterpret_cast<r5::studiohdr_v16_t*>(modelAsset->data);

    const uint8_t* boneMap = pStudioHdr->boneStateCount ? pStudioHdr->pBoneStates() : s_VertexDataBaseBoneMap; // does this model have remapped bones? use default map if not

    modelAsset->lods.resize(pStudioHdr->lodCount);
    modelAsset->bodyParts.resize(pStudioHdr->numbodyparts);

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
            ModelLODData_t& lodData = modelAsset->lods.at(lodLevel);
            lodData.switchPoint = pStudioHdr->LODThreshold(lodLevel);

            modelAsset->meshVertexData.resize(modelAsset->meshVertexData.size() + lod->meshCount);

            // [rika]: this should only get hit once per LOD
            const size_t curMeshCount = lodData.meshes.size();
            lodData.meshes.resize(curMeshCount + lod->meshCount);

            for (uint16_t bdyIdx = 0; bdyIdx < pStudioHdr->numbodyparts; bdyIdx++)
            {
                const r5::mstudiobodyparts_v16_t* const pBodypart = pStudioHdr->pBodypart(bdyIdx);

                modelAsset->SetupBodyPart(bdyIdx, pBodypart->pszName(), static_cast<int>(lodData.models.size()), pBodypart->nummodels);

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
                        CManagedBuffer* const buffer = g_BufferManager->ClaimBuffer();

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
                            ParseVertexFromVG(&meshVertexData->GetVertices()[vertIdx], &meshVertexData->GetWeights()[weightIdx], texcoords, &meshData, vertexData, boneMap, weights, weightIdx);
                        }
                        meshData.weightsCount = weightIdx;
                        meshVertexData->AddWeights(nullptr, meshData.weightsCount);

                        meshData.ParseMaterial(modelAsset, pMesh->material, pMesh->meshid);

                        lodMeshCount[lodLevel]++;
                        modelData.meshCount++;

                        // for export
                        lodData.weightsPerVert = meshData.weightsPerVert > lodData.weightsPerVert ? meshData.weightsPerVert : lodData.weightsPerVert;
                        lodData.texcoordsPerVert = meshData.texcoordCount > lodData.texcoordsPerVert ? meshData.texcoordCount : lodData.texcoordsPerVert;

                        // remove it from usage
                        meshVertexData->DestroyWriter();

                        meshData.meshVertexDataIndex = modelAsset->meshVertexData.size();
                        modelAsset->meshVertexData.addBack(reinterpret_cast<char*>(meshVertexData), meshVertexData->GetSize());

                        // relieve buffer
                        g_BufferManager->RelieveBuffer(buffer);
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

    modelAsset->meshVertexData.shrink();
}

static void ParseModelTextureData_v8(CPakAsset* const asset, ModelAsset* const modelAsset)
{
    UNUSED(asset);

    const studiohdr_generic_t* const pStudioHdr = &modelAsset->studiohdr;

    const r5::mstudiotexture_v8_t* const pTextures = reinterpret_cast<r5::mstudiotexture_v8_t*>((char*)modelAsset->data + pStudioHdr->textureOffset);
    modelAsset->materials.resize(pStudioHdr->textureCount);

    for (int i = 0; i < pStudioHdr->textureCount; ++i)
    {
        ModelMaterialData_t& matlData = modelAsset->materials.at(i);
        const r5::mstudiotexture_v8_t* const texture = &pTextures[i];

        // if guid is 0, the material is a VMT
        if (texture->guid != 0)
            matlData.materialAsset = g_assetData.FindAssetByGUID<CPakAsset>(texture->guid);

        matlData.materialGuid = texture->guid;
        matlData.materialName = texture->pszName();
    }

    modelAsset->skins.reserve(pStudioHdr->numSkinFamilies);
    for (int i = 0; i < pStudioHdr->numSkinFamilies; i++)
        modelAsset->skins.emplace_back(pStudioHdr->pSkinName(i), pStudioHdr->pSkinFamily(i));
}

static void ParseModelTextureData_v16(CPakAsset* const asset, ModelAsset* const modelAsset)
{
    UNUSED(asset);

    const studiohdr_generic_t* const pStudioHdr = &modelAsset->studiohdr;

    const uint64_t* const pTextures = reinterpret_cast<uint64_t*>((char*)modelAsset->data + pStudioHdr->textureOffset);
    modelAsset->materials.resize(pStudioHdr->textureCount);

    for (int i = 0; i < pStudioHdr->textureCount; ++i)
    {
        ModelMaterialData_t& matlData = modelAsset->materials.at(i);
        uint64_t texture = pTextures[i];

        matlData.materialGuid = texture;

        // not possible to have vmt materials
        matlData.materialAsset = g_assetData.FindAssetByGUID<CPakAsset>(texture);

        matlData.materialName = matlData.materialAsset ? matlData.materialAsset->GetAssetName() : std::format("0x{:x}", texture); // no name
    }

    modelAsset->skins.reserve(pStudioHdr->numSkinFamilies);
    for (int i = 0; i < pStudioHdr->numSkinFamilies; i++)
        modelAsset->skins.emplace_back(pStudioHdr->pSkinName_V16(i), pStudioHdr->pSkinFamily(i));
}

static void ParseModelBoneData_v8(CPakAsset* const asset, ModelAsset* const modelAsset)
{
    UNUSED(asset);

    const r5::mstudiobone_v8_t* const bones = reinterpret_cast<r5::mstudiobone_v8_t*>((char*)modelAsset->data + modelAsset->studiohdr.boneOffset);
    modelAsset->bones.resize(modelAsset->studiohdr.boneCount);

    for (int i = 0; i < modelAsset->studiohdr.boneCount; i++)
    {
        modelAsset->bones.at(i) = ModelBone_t(&bones[i]);
    }
}

static void ParseModelBoneData_v12_1(CPakAsset* const asset, ModelAsset* const modelAsset)
{
    UNUSED(asset);

    const r5::mstudiobone_v12_1_t* const bones = reinterpret_cast<r5::mstudiobone_v12_1_t*>((char*)modelAsset->data + modelAsset->studiohdr.boneOffset);
    modelAsset->bones.resize(modelAsset->studiohdr.boneCount);

    for (int i = 0; i < modelAsset->studiohdr.boneCount; i++)
    {
        modelAsset->bones.at(i) = ModelBone_t(&bones[i]);
    }
}

static void ParseModelBoneData_v16(CPakAsset* const asset, ModelAsset* const modelAsset)
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

        ParseModelBoneData_v8(pakAsset, mdlAsset);
        ParseModelTextureData_v8(pakAsset, mdlAsset);
        ParseModelVertexData_v8(pakAsset, mdlAsset);
        break;
    }
    case eMDLVersion::VERSION_9:
    case eMDLVersion::VERSION_10:
    case eMDLVersion::VERSION_11:
    case eMDLVersion::VERSION_12:
    {
        ModelAssetHeader_v9_t* hdr = reinterpret_cast<ModelAssetHeader_v9_t*>(pakAsset->header());
        mdlAsset = new ModelAsset(hdr, streamEntry, ver);

        ParseModelBoneData_v8(pakAsset, mdlAsset);
        ParseModelTextureData_v8(pakAsset, mdlAsset);
        ParseModelVertexData_v9(pakAsset, mdlAsset);
        break;
    }
    case eMDLVersion::VERSION_12_1: // has to have its own vertex func
    case eMDLVersion::VERSION_12_2:
    case eMDLVersion::VERSION_12_3:
    {
        ModelAssetHeader_v12_1_t* hdr = reinterpret_cast<ModelAssetHeader_v12_1_t*>(pakAsset->header());
        mdlAsset = new ModelAsset(hdr, streamEntry, ver);

        ParseModelBoneData_v12_1(pakAsset, mdlAsset);
        ParseModelTextureData_v8(pakAsset, mdlAsset);
        ParseModelVertexData_v12_1(pakAsset, mdlAsset);
        break;
    }
    case eMDLVersion::VERSION_13:
    case eMDLVersion::VERSION_13_1:
    {
        ModelAssetHeader_v13_t* hdr = reinterpret_cast<ModelAssetHeader_v13_t*>(pakAsset->header());
        mdlAsset = new ModelAsset(hdr, streamEntry, ver);

        ParseModelBoneData_v12_1(pakAsset, mdlAsset);
        ParseModelTextureData_v8(pakAsset, mdlAsset);
        ParseModelVertexData_v12_1(pakAsset, mdlAsset);
        break;
    }
    case eMDLVersion::VERSION_14:
    case eMDLVersion::VERSION_14_1:
    case eMDLVersion::VERSION_15:
    {
        ModelAssetHeader_v13_t* hdr = reinterpret_cast<ModelAssetHeader_v13_t*>(pakAsset->header());
        mdlAsset = new ModelAsset(hdr, streamEntry, ver);

        ParseModelBoneData_v12_1(pakAsset, mdlAsset);
        ParseModelTextureData_v8(pakAsset, mdlAsset);
        ParseModelVertexData_v14(pakAsset, mdlAsset);
        break;
    }
    case eMDLVersion::VERSION_16:
    case eMDLVersion::VERSION_17:
    case eMDLVersion::VERSION_18:
    {
        ModelAssetHeader_v16_t* hdr = reinterpret_cast<ModelAssetHeader_v16_t*>(pakAsset->header());
        ModelAssetCPU_v16_t* cpu = reinterpret_cast<ModelAssetCPU_v16_t*>(pakAsset->cpu());
        mdlAsset = new ModelAsset(hdr, cpu, streamEntry, ver);

        ParseModelBoneData_v16(pakAsset, mdlAsset);
        ParseModelTextureData_v16(pakAsset, mdlAsset);
        ParseModelVertexData_v16(pakAsset, mdlAsset);
        break;
    }
    default:
    {
        assertm(false, "unaccounted asset version, will cause major issues!");
        return;
    }
    }

    assertm(mdlAsset->name, "Model had no name.");
    pakAsset->SetAssetName(mdlAsset->name);
    pakAsset->setExtraData(mdlAsset);
}

void PostLoadModelAsset(CAssetContainer* const pak, CAsset* const asset)
{
    UNUSED(pak);

    CPakAsset* const pakAsset = static_cast<CPakAsset*>(asset);

    ModelAsset* const mdlAsset = reinterpret_cast<ModelAsset*>(pakAsset->extraData());

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

    if (mdlAsset->lods.empty())
        return;
    
    const size_t lodLevel = 0;

    if (!mdlAsset->drawData)
    {
        mdlAsset->drawData = new CDXDrawData();
        mdlAsset->drawData->meshBuffers.resize(mdlAsset->lods.at(lodLevel).meshes.size());
        mdlAsset->drawData->modelName = mdlAsset->name;
    }

    // [rika]: eventually parse through models
    CDXDrawData* const drawData = mdlAsset->drawData;
    for (size_t i = 0; i < mdlAsset->lods.at(lodLevel).meshes.size(); ++i)
    {
        const ModelMeshData_t& mesh = mdlAsset->lods.at(lodLevel).meshes.at(i);
        DXMeshDrawData_t* const meshDrawData = &drawData->meshBuffers[i];

        meshDrawData->visible = true;

        if(mesh.material)
            meshDrawData->uberStaticBuf = mesh.material->uberStaticBuffer;

        assertm(mesh.meshVertexDataIndex != invalidNoodleIdx, "mesh data hasn't been parsed ??");

        std::unique_ptr<char[]> parsedVertexDataBuf = mdlAsset->meshVertexData.getIdx(mesh.meshVertexDataIndex);
        const CMeshData* const parsedVertexData = reinterpret_cast<CMeshData*>(parsedVertexDataBuf.get());

        if (!meshDrawData->vertexBuffer)
        {
#if defined(ADVANCED_MODEL_PREVIEW)
            const UINT vertStride = mesh.vertCacheSize;
#else
            const UINT vertStride = sizeof(Vertex_t);
#endif

            D3D11_BUFFER_DESC desc = {};

            desc.Usage = D3D11_USAGE_DYNAMIC;
            desc.ByteWidth = vertStride * mesh.vertCount;
            desc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
            desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
            desc.MiscFlags = 0;

#if defined(ADVANCED_MODEL_PREVIEW)
            const void* vertexData = mesh.rawVertexData;
#else
            const void* vertexData = parsedVertexData->GetVertices();
#endif

            D3D11_SUBRESOURCE_DATA srd{ vertexData };

            if (FAILED(g_dxHandler->GetDevice()->CreateBuffer(&desc, &srd, &meshDrawData->vertexBuffer)))
                return;

            meshDrawData->vertexStride = vertStride;

#if defined(ADVANCED_MODEL_PREVIEW)
            delete[] mesh.rawVertexData;
#endif
        }

        if (!meshDrawData->indexBuffer)
        {
            D3D11_BUFFER_DESC desc = {};

            desc.Usage = D3D11_USAGE_DYNAMIC;
            desc.ByteWidth = static_cast<UINT>(mesh.indexCount * sizeof(uint16_t));
            desc.BindFlags = D3D11_BIND_INDEX_BUFFER;
            desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
            desc.MiscFlags = 0;

            D3D11_SUBRESOURCE_DATA srd = { parsedVertexData->GetIndices() };
            if (FAILED(g_dxHandler->GetDevice()->CreateBuffer(&desc, &srd, &meshDrawData->indexBuffer)))
                return;

            meshDrawData->numIndices = mesh.indexCount;
        }
    }
}

enum eModelExportSetting
{
    MDL_CAST,
    MDL_RMAX,
    MDL_RMDL,
    MDL_STL_VALVE_PHYSICS,
    MDL_STL_RESPAWN_PHYSICS,
};

// bad function names
static bool ExportModelAssetMaterials(const ModelAsset* const modelAsset, std::filesystem::path& exportPath, std::unordered_set<int>&usedMaterialIndices)
{
    // create folder for the textures
    if (!CreateDirectories(exportPath))
    {
        assertm(false, "Failed to create asset's directory.");
        return false;
    }

    // get texture binding if it exists
    auto txtrAssetBinding = g_assetData.m_assetTypeBindings.find('rtxt');

    // only parse textures if one has actually been loaded
    if (txtrAssetBinding == g_assetData.m_assetTypeBindings.end())
        return false;

    std::atomic<uint32_t> remainingMaterials = 0; // we don't actually need thread safe here
    const ProgressBarEvent_t* const materialExportProgress = g_pImGuiHandler->AddProgressBarEvent("Exporting Materials..", static_cast<uint32_t>(usedMaterialIndices.size()), &remainingMaterials, true);

    // [rika]: so we don't export textures per lod, we should exclude skins
    // todo: move this into the base function, don't export if raw
    for (auto& materialId : usedMaterialIndices)
    {
        ++remainingMaterials;
        const ModelMaterialData_t& material = modelAsset->materials.at(materialId);

        // not in this pak, skip
        if (nullptr == material.materialAsset)
            continue;

        assert(nullptr != material.materialAsset->extraData());
        const MaterialAsset* const matlAsset = reinterpret_cast<MaterialAsset*>(material.materialAsset->extraData());

        // skip this material if it has no textures
        if (matlAsset->txtrAssets.size() == 0ull)
            continue;

        // might need to recalculate normals
        // enable exporting other image formats!
        ExportMaterialTextures(eTextureExportSetting::PNG_HM, matlAsset, exportPath, true);

    }
    g_pImGuiHandler->FinishProgressBarEvent(materialExportProgress);

    return true;
}

static bool ExportRawModelAsset(const ModelAsset* const modelAsset, std::filesystem::path& exportPath, const char* const streamedData)
{
    // Is asset permanent or streamed?
    const char* const pDataBuffer = streamedData ? streamedData : modelAsset->vertDataPermanent;

    StreamIO studioOut(exportPath.string(), eStreamIOMode::Write);
    studioOut.write(reinterpret_cast<char*>(modelAsset->data), modelAsset->studiohdr.length);

    if (modelAsset->studiohdr.phySize > 0)
    {
        exportPath.replace_extension(".phy");

        // if we error here something is broken with setting up the model asset
        StreamIO physOut(exportPath.string(), eStreamIOMode::Write);
        physOut.write(reinterpret_cast<char*>(modelAsset->physics), modelAsset->studiohdr.phySize);
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
    case eMDLVersion::VERSION_53: // TEEEEMPPP!
    {
        // vvd
        exportPath.replace_extension(".vvd");

        StreamIO vertOut(exportPath.string(), eStreamIOMode::Write);
        vertOut.write(pDataBuffer + modelAsset->studiohdr.vvdOffset, modelAsset->studiohdr.vvdSize);

        // vvc
        if (modelAsset->studiohdr.vvcSize)
        {
            exportPath.replace_extension(".vvc");

            StreamIO vertColorOut(exportPath.string(), eStreamIOMode::Write);
            vertColorOut.write(pDataBuffer + modelAsset->studiohdr.vvcOffset, modelAsset->studiohdr.vvcSize);
        }

        // vvw
        if (modelAsset->studiohdr.vvwSize)
        {
            exportPath.replace_extension(".vvw");

            StreamIO vertWeightOut(exportPath.string(), eStreamIOMode::Write);
            vertWeightOut.write(pDataBuffer + modelAsset->studiohdr.vvwOffset, modelAsset->studiohdr.vvwSize);
        }

        // vtx
        exportPath.replace_extension(".dx11.vtx"); // cope

        // 'opt' being optimized
        StreamIO vertOptOut(exportPath.string(), eStreamIOMode::Write);
        vertOptOut.write(pDataBuffer, modelAsset->studiohdr.vtxSize); // [rika]: 

        return true;
    }
    case eMDLVersion::VERSION_9:
    case eMDLVersion::VERSION_10:
    case eMDLVersion::VERSION_11:
    case eMDLVersion::VERSION_12:
    case eMDLVersion::VERSION_12_1:
    case eMDLVersion::VERSION_12_2:
    case eMDLVersion::VERSION_12_3:
    case eMDLVersion::VERSION_13:
    case eMDLVersion::VERSION_13_1:
    case eMDLVersion::VERSION_14:
    case eMDLVersion::VERSION_14_1:
    case eMDLVersion::VERSION_15:
    {
        // [rika]: .hwData would be better but this is set in stone at this point essentially
        exportPath.replace_extension(".vg");

        StreamIO hwOut(exportPath.string(), eStreamIOMode::Write);
        hwOut.write(pDataBuffer, modelAsset->studiohdr.hwDataSize);

        return true;
    }
    case eMDLVersion::VERSION_16:
    case eMDLVersion::VERSION_17:
    case eMDLVersion::VERSION_18:
    {
        // special case because of compression
        exportPath.replace_extension(".vg");

        std::unique_ptr<char[]> hwBufTmp = std::make_unique<char[]>(modelAsset->studiohdr.hwDataSize);
        char* pPos = hwBufTmp.get(); // position in the decompressed buffer for writing

        for (int i = 0; i < modelAsset->studiohdr.groupCount; i++)
        {
            const studio_hw_groupdata_t& group = modelAsset->studiohdr.groups[i];

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
        hwOut.write(hwBufTmp.get(), modelAsset->studiohdr.hwDataSize);

        return true;
    }
    default:
        assertm(false, "Asset version not handled.");
        return false;
    }

    unreachable();
}

static bool ExportRMAXModelAsset(const ModelAsset* const modelAsset, std::filesystem::path& exportPath)
{
    std::string fileNameBase = exportPath.stem().string();

    // [rika]: keep track of the materials that are actually used by meshes, so we don't export unneeded ones (speeds up export)
    std::unordered_set<int> usedMaterialIndices;

    for (size_t lodIdx = 0; lodIdx < modelAsset->lods.size(); lodIdx++)
    {
        const ModelLODData_t& lodData = modelAsset->lods.at(lodIdx);

        const std::string tmpName = std::format("{}_LOD{}.rmax", fileNameBase, lodIdx);
        exportPath.replace_filename(tmpName);

        rmax::RMAXExporter rmaxFile(exportPath, fileNameBase.c_str(), fileNameBase.c_str());

        // do bones
        rmaxFile.ReserveBones(modelAsset->bones.size());
        for (auto& bone : modelAsset->bones)
            rmaxFile.AddBone(bone.name, bone.parentIndex, bone.pos, bone.quat, bone.scale);

        // do materials
        rmaxFile.ReserveMaterials(modelAsset->materials.size());
        for (auto& material : modelAsset->materials)
        {
            // empty material basically, uses rmdl's material name (this will not work on retail ;3)
            if (nullptr == material.materialAsset)
            {
                rmaxFile.AddMaterial(material.materialName.c_str());
                continue;
            }

            assert(nullptr != material.materialAsset->extraData());
            const MaterialAsset* const matlAsset = reinterpret_cast<MaterialAsset*>(material.materialAsset->extraData());

            rmaxFile.AddMaterial(matlAsset->name);

            if (!matlAsset->resourceBindings.size())
                continue;

            rmax::RMAXMaterial* const matl = rmaxFile.GetMaterialLast();

            for (auto& txtr : matlAsset->txtrAssets)
            {
                if (!matlAsset->resourceBindings.count(txtr.index))
                    continue;

                std::string resource = matlAsset->resourceBindings.find(txtr.index)->second.name;

                if (!rmax::s_TextureTypeMap.count(resource))
                    continue;

                matl->AddTexture(rmax::s_TextureTypeMap.find(resource)->second);
            }
        }

        // do models
        rmaxFile.ReserveCollections(lodData.models.size());
        rmaxFile.ReserveMeshes(lodData.meshes.size());
        rmaxFile.ReserveVertices(lodData.vertexCount, lodData.texcoordsPerVert, lodData.weightsPerVert);
        rmaxFile.ReserveIndices(lodData.indexCount);
        for (auto& model : lodData.models)
        {
            if (!model.meshCount)
                continue;

            rmaxFile.AddCollection(model.name.c_str(), model.meshCount);

            for (uint32_t meshIdx = 0; meshIdx < model.meshCount; meshIdx++)
            {
                ModelMeshData_t& meshData = model.meshes[meshIdx];

                assertm(meshData.meshVertexDataIndex != invalidNoodleIdx, "mesh data hasn't been parsed ??");

                std::unique_ptr<char[]> parsedVertexDataBuf = modelAsset->meshVertexData.getIdx(meshData.meshVertexDataIndex);
                const CMeshData* const parsedVertexData = reinterpret_cast<CMeshData*>(parsedVertexDataBuf.get());

                rmaxFile.AddMesh(static_cast<int16_t>(rmaxFile.CollectionCount() - 1), static_cast<int16_t>(meshData.materialId), meshData.texcoordCount, meshData.texcoodIndices, (meshData.rawVertexLayoutFlags & VERT_COLOR));

                usedMaterialIndices.insert(meshData.materialId);

                rmax::RMAXMesh* const mesh = rmaxFile.GetMeshLast();

                // data parsing
                for (uint32_t i = 0; i < meshData.vertCount; i++)
                {
                    const Vertex_t* const vertData = &parsedVertexData->GetVertices()[i];

                    Vector normal;
                    vertData->normalPacked.UnpackNormal(normal);

                    mesh->AddVertex(vertData->position, normal);

                    if (meshData.rawVertexLayoutFlags & VERT_COLOR)
                        mesh->AddColor(vertData->color);

                    for (int16_t texcoordIdx = 0; texcoordIdx < meshData.texcoordCount; texcoordIdx++)
                        mesh->AddTexcoord(texcoordIdx > 0 ? parsedVertexData->GetTexcoords()[(i * (meshData.texcoordCount - 1)) + (texcoordIdx - 1)] : vertData->texcoord);

                    for (uint32_t weightIdx = 0; weightIdx < vertData->weightCount; weightIdx++)
                    {
                        const VertexWeight_t* const weight = &parsedVertexData->GetWeights()[vertData->weightIndex + weightIdx];
                        mesh->AddWeight(i, weight->bone, weight->weight);
                    }
                }

                for (uint32_t i = 0; i < meshData.indexCount; i += 3)
                    mesh->AddIndice(parsedVertexData->GetIndices()[i], parsedVertexData->GetIndices()[i + 1], parsedVertexData->GetIndices()[i + 2]);
            }
        }

        rmaxFile.ToFile();
    }

    // export textures for model
    std::filesystem::path textureDir(std::format("{}/{}", exportPath.parent_path().string(), fileNameBase)); // todo, remove duplicate code
    ExportModelAssetMaterials(modelAsset, textureDir, usedMaterialIndices);

    return true;
}

static bool ExportCastModelAsset(const ModelAsset* const modelAsset, std::filesystem::path& exportPath, const uint64_t guid)
{
    // [rika]: keep track of the materials that are actually used by meshes, so we don't export unneeded ones (speeds up export)
    std::unordered_set<int> usedMaterialIndices;

    std::string fileNameBase = exportPath.stem().string();
    for (size_t lodIdx = 0; lodIdx < modelAsset->lods.size(); lodIdx++)
    {
        const ModelLODData_t& lodData = modelAsset->lods.at(lodIdx);

        std::string tmpName(std::format("{}_LOD{}.cast", fileNameBase, std::to_string(lodIdx)));
        exportPath.replace_filename(tmpName);

        cast::CastExporter cast(exportPath.string());

        // cast
        cast::CastNode* rootNode = cast.GetChild(0); // we only have one root node, no hash
        cast::CastNode* modelNode = rootNode->AddChild(cast::CastId::Model, guid);

        // [rika]: we can predict how big this vector needs to be, however resizing it will make adding new members a pain.
        const size_t modelChildrenCount = 1 + modelAsset->materials.size() + lodData.meshes.size(); // skeleton (one), materials (varies), meshes (varies)
        modelNode->ReserveChildren(modelChildrenCount);

        // do skeleton
        {
            const size_t boneCount = modelAsset->bones.size();

            cast::CastNode* skelNode = modelNode->AddChild(cast::CastId::Skeleton, RTech::StringToGuid(fileNameBase.c_str()));
            skelNode->ReserveChildren(boneCount);

            // uses hashes for lookup, still gets bone parents by index :clown:
            for (size_t i = 0; i < boneCount; i++)
            {
                const ModelBone_t& boneData = modelAsset->bones.at(i);

                cast::CastNodeBone boneNode(skelNode);
                boneNode.MakeBone(boneData.name, boneData.parentIndex, &boneData.pos, &boneData.quat, false);
            }
        }

        // do materials
        for (auto& matlData : modelAsset->materials)
        {
            cast::CastNode matlNode(cast::CastId::Material, 2, matlData.materialGuid); // has at least two properties, name and material type, in this case pbr

            matlNode.SetProperty(1, cast::CastPropertyId::String, static_cast<int>(cast::CastPropsMaterial::Type), "pbr", 1u);

            // not in any loaded pak, skip
            if (nullptr == matlData.materialAsset)
            {
                matlNode.SetProperty(0, cast::CastPropertyId::String, static_cast<int>(cast::CastPropsMaterial::Name), "UNK_0xFFFFFFFF", 1u); // unsure why it does this but we're rolling with it!
                modelNode->AddChild(matlNode);
                continue;
            }

            assert(nullptr != matlData.materialAsset->extraData());
            const MaterialAsset* const matlAsset = reinterpret_cast<MaterialAsset*>(matlData.materialAsset->extraData());

            const char* matlStem = keepAfterLastSlashOrBackslash(matlAsset->name);

            matlNode.SetProperty(0, cast::CastPropertyId::String, static_cast<int>(cast::CastPropsMaterial::Name), matlStem, 1u);

            // 
            if (matlAsset->resourceBindings.size() > 0)
            {
                for (auto& txtr : matlAsset->txtrAssets)
                {
                    if (!matlAsset->resourceBindings.count(txtr.index))
                        continue;

                    const std::string resource = matlAsset->resourceBindings.find(txtr.index)->second.name;

                    if (!cast::s_TextureTypeMap.count(resource))
                        continue;

                    const uint64_t textureGuid = txtr.asset ? txtr.asset->data()->guid : 0ull;

                    // If the texture GUID is 0, the user has most likely not loaded an RPak that is required for this material asset.
                    // Usually some combination of common.rpak, common_mp.rpak, and root_lgnd_skins.rpak
                    if (textureGuid == 0)
                        Log("Material %s for model %s did not have a valid texture pointer for res idx %i\n", matlAsset->name, modelAsset->name, txtr.index);

                    const cast::CastPropsMaterial matlTxtrProp = cast::s_TextureTypeMap.find(resource)->second;

                    matlNode.AddProperty(cast::CastPropertyId::Integer64, static_cast<int>(matlTxtrProp), textureGuid);

                    cast::CastNode fileNode(cast::CastId::File, 1, textureGuid);

                    const std::string filePath(std::format("{}/{}_{}.png", fileNameBase, matlStem, resource));
                    fileNode.SetString(filePath); // materials exported from models always use png, as blender support for dds is bad, todo: make it so we can use ALL formats!
                    fileNode.SetProperty(0, cast::CastPropertyId::String, static_cast<int>(cast::CastPropsFile::Path), fileNode.GetString(), 1u);

                    matlNode.AddChild(fileNode);
                }
            }

            modelNode->AddChild(matlNode);
        }

        // !!! TODO !!!
        // this needs to get cleaned up, but if I don't push I am gonna be stuck forever

        // [rika]: a system to have these allocated to each node would be cleaner, but more expensive.
        struct DataPtrs_t{
            Vector* positions;
            Vector* normals;
            Color32* colors;
            Vector2D* texcoords; // all uvs stored here
            uint8_t* blendIndices;
            float* blendWeights;

            uint16_t* indices;
        };

        DataPtrs_t vertexData { nullptr };

        //                                    Postion & Normal       Color
        constexpr size_t castMinSizePerVert = (sizeof(Vector) * 2) + sizeof(Color32);

        char* vertDataBlockBuf = new char[(castMinSizePerVert + (sizeof(Vector2D) * lodData.texcoordsPerVert)) * lodData.vertexCount] {};

        vertexData.positions = reinterpret_cast<Vector*>(vertDataBlockBuf);
        vertexData.normals = &vertexData.positions[lodData.vertexCount];
        vertexData.colors = reinterpret_cast<Color32*>(&vertexData.normals[lodData.vertexCount]); // discarded if unneeded
        vertexData.texcoords = reinterpret_cast<Vector2D*>(&vertexData.colors[lodData.vertexCount]);
        vertexData.blendIndices = new uint8_t[lodData.vertexCount * lodData.weightsPerVert] {};
        vertexData.blendWeights = new float[lodData.vertexCount * lodData.weightsPerVert] {};

        vertexData.indices = new uint16_t[lodData.indexCount];

        size_t curIndex = 0; // current index into vertex data
        size_t idxIndex = 0; // shit format

        // do meshes
        for (auto& modelData : lodData.models)
        {
            for (size_t i = 0; i < modelData.meshCount; i++)
            {
                const ModelMeshData_t& meshData = modelData.meshes[i];
                const ModelMaterialData_t& matlData = modelAsset->materials.at(meshData.materialId);

                assertm(meshData.meshVertexDataIndex != invalidNoodleIdx, "mesh data hasn't been parsed ??");

                std::unique_ptr<char[]> parsedVertexDataBuf = modelAsset->meshVertexData.getIdx(meshData.meshVertexDataIndex);
                const CMeshData* const parsedVertexData = reinterpret_cast<CMeshData*>(parsedVertexDataBuf.get());

                // we are using this material!
                usedMaterialIndices.insert(meshData.materialId);

                std::string matl = nullptr != meshData.material ? keepAfterLastSlashOrBackslash(meshData.material->name) : std::to_string(matlData.materialGuid);
                std::string meshName = std::format("{}_{}", modelData.name, matl);
                cast::CastNode meshNode(cast::CastId::Mesh, 1, RTech::StringToGuid(meshName.c_str())); // name

                // name, pos, normal, blendweight, blendindices, indices, uv count, max blends, material, texcoords, color
                const size_t meshPropertiesCount = 9 + meshData.texcoordCount + (meshData.rawVertexLayoutFlags & VERT_COLOR ? 1 : 0);
                meshNode.ReserveProperties(meshPropertiesCount);

                // works on files but not here, why?
                // update: now it works after changing how the string is formed, lovely.
                meshNode.SetString(meshName);
                meshNode.SetProperty(0, cast::CastPropertyId::String, static_cast<int>(cast::CastPropsMesh::Name), meshNode.GetString(), 1u);

                meshNode.AddProperty(cast::CastPropertyId::Vector3, static_cast<int>(cast::CastPropsMesh::Vertex_Postion_Buffer), &vertexData.positions[curIndex], meshData.vertCount);
                meshNode.AddProperty(cast::CastPropertyId::Vector3, static_cast<int>(cast::CastPropsMesh::Vertex_Normal_Buffer), &vertexData.normals[curIndex], meshData.vertCount);

                if (meshData.rawVertexLayoutFlags & VERT_COLOR)
                    meshNode.AddProperty(cast::CastPropertyId::Integer32, static_cast<int>(cast::CastPropsMesh::Vertex_Color_Buffer), &vertexData.colors[curIndex], meshData.vertCount);

                // cast cries if we use the proper index
                for (int16_t texcoordIdx = 0; texcoordIdx < meshData.texcoordCount; texcoordIdx++)
                    meshNode.AddProperty(cast::CastPropertyId::Vector2, static_cast<int>(cast::CastPropsMesh::Vertex_UV_Buffer), &vertexData.texcoords[(lodData.vertexCount * texcoordIdx) + curIndex], meshData.vertCount, true, texcoordIdx);

                meshNode.AddProperty(cast::CastPropertyId::Byte, static_cast<int>(cast::CastPropsMesh::Vertex_Weight_Bone_Buffer), &vertexData.blendIndices[lodData.weightsPerVert * curIndex], (meshData.vertCount * meshData.weightsPerVert));
                meshNode.AddProperty(cast::CastPropertyId::Float, static_cast<int>(cast::CastPropsMesh::Vertex_Weight_Value_Buffer), &vertexData.blendWeights[lodData.weightsPerVert * curIndex], (meshData.vertCount * meshData.weightsPerVert));

                // parse our vertices into the buffer, so cringe!
                for (uint32_t vertIdx = 0; vertIdx < meshData.vertCount; vertIdx++)
                {
                    const Vertex_t& vert = parsedVertexData->GetVertices()[vertIdx];

                    vertexData.positions[curIndex + vertIdx] = vert.position;
                    vert.normalPacked.UnpackNormal(vertexData.normals[curIndex + vertIdx]);
                    vertexData.colors[curIndex + vertIdx] = vert.color;

                    for (int16_t texcoordIdx = 0; texcoordIdx < meshData.texcoordCount; texcoordIdx++)
                        vertexData.texcoords[(lodData.vertexCount * texcoordIdx) + curIndex + vertIdx] = texcoordIdx > 0 ? parsedVertexData->GetTexcoords()[(vertIdx * (meshData.texcoordCount - 1)) + (texcoordIdx - 1)] : vert.texcoord;

                    for (uint32_t weightIdx = 0; weightIdx < vert.weightCount; weightIdx++)
                    {
                        vertexData.blendIndices[(lodData.weightsPerVert * curIndex) + (meshData.weightsPerVert * vertIdx) + weightIdx] = static_cast<uint8_t>(parsedVertexData->GetWeights()[vert.weightIndex + weightIdx].bone);
                        vertexData.blendWeights[(lodData.weightsPerVert * curIndex) + (meshData.weightsPerVert * vertIdx) + weightIdx] = parsedVertexData->GetWeights()[vert.weightIndex + weightIdx].weight;
                    }
                }

                // parse our indices into the buffer, and shuffle them! extra cringe!
                const uint32_t indexCount = meshData.indexCount;
                meshNode.AddProperty(cast::CastPropertyId::Short, static_cast<int>(cast::CastPropsMesh::Face_Buffer), &vertexData.indices[idxIndex], indexCount);

                for (uint32_t idxIdx = 0; idxIdx < indexCount; idxIdx += 3)
                {
                    vertexData.indices[idxIndex + idxIdx]     = parsedVertexData->GetIndices()[idxIdx + 2];
                    vertexData.indices[idxIndex + idxIdx + 1] = parsedVertexData->GetIndices()[idxIdx + 1];
                    vertexData.indices[idxIndex + idxIdx + 2] = parsedVertexData->GetIndices()[idxIdx];
                }

                meshNode.AddProperty(cast::CastPropertyId::Short, static_cast<int>(cast::CastPropsMesh::UV_Layer_Count), meshData.texcoordCount);
                meshNode.AddProperty(cast::CastPropertyId::Short, static_cast<int>(cast::CastPropsMesh::Max_Weight_Influence), meshData.weightsPerVert);

                meshNode.AddProperty(cast::CastPropertyId::Integer64, static_cast<int>(cast::CastPropsMesh::Material), modelAsset->materials.at(meshData.materialId).materialGuid);

                modelNode->AddChild(meshNode);

                curIndex += meshData.vertCount;
                idxIndex += indexCount;
            }
        }

        cast.ToFile();

        // cleanup our allocated buffers
        delete[] vertDataBlockBuf;
        delete[] vertexData.blendIndices;
        delete[] vertexData.blendWeights;

        delete[] vertexData.indices;
    }

    // export textures for model
    std::filesystem::path textureDir(std::format("{}/{}", exportPath.parent_path().string(), fileNameBase)); // todo, remove duplicate code
    ExportModelAssetMaterials(modelAsset, textureDir, usedMaterialIndices);

    return true;
}

template <typename phyheader_t>
static bool ExportPhysicsModelPhy(const ModelAsset* const modelAsset, std::filesystem::path& exportPath)
{
    const studiohdr_generic_t& hdr = modelAsset->studiohdr;

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
    const studiohdr_generic_t& hdr = modelAsset->studiohdr;

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
    ModelAsset* modelAsset = nullptr;
    std::unique_ptr<char[]> streamedData = nullptr;

    int settingFixup = setting; // we want to use the 'mdl_' asset's export setting for 'mdl' (and not have a drop down for 'mdl'

    // temporary
    switch (asset->GetAssetContainerType())
    {
    case CAsset::PAK:
    {
        CPakAsset* const pakAsset = static_cast<CPakAsset*>(asset);
        assertm(pakAsset, "Asset should be valid.");

        modelAsset = reinterpret_cast<ModelAsset*>(pakAsset->extraData());
        assertm(modelAsset, "Extra data should be valid at this point.");

        if (modelAsset->vertDataStreamed.size > 0)
        {
            streamedData = pakAsset->getStarPakData(modelAsset->vertDataStreamed.offset, modelAsset->vertDataStreamed.size, false);
        }

        break;
    }
    case CAsset::MDL:
    {
        CSourceModelAsset* const sourceModelAsset = static_cast<CSourceModelAsset*>(asset);
        assertm(sourceModelAsset, "Asset should be valid.");

        modelAsset = sourceModelAsset->GetModelAsset();
        assertm(modelAsset, "Extra data should be valid at this point.");

        // fix our setting
        if (g_assetData.m_assetTypeBindings.count(static_cast<uint32_t>(AssetType_t::MDL_)))
            settingFixup = g_assetData.m_assetTypeBindings.find(static_cast<uint32_t>(AssetType_t::MDL_))->second.e.exportSetting;
        else
            assertm(false, "bad export setting");

        break;
    }
    default:
    {
        return false;
    }
    }

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

    // [rika]: needs setting to toggle this
    if (g_ExportSettings.exportSeqsWithRig && modelAsset->numAnimSeqs > 0)
    {
        auto aseqAssetBinding = g_assetData.m_assetTypeBindings.find('qesa');

        assertm(aseqAssetBinding != g_assetData.m_assetTypeBindings.end(), "Unable to find asset type binding for \"aseq\" assets");

        if (aseqAssetBinding != g_assetData.m_assetTypeBindings.end())
        {
            std::filesystem::path outputPath(exportPath);
            outputPath.append(std::format("anims_{}/temp", modelStem));

            if (!CreateDirectories(outputPath.parent_path()))
            {
                assertm(false, "Failed to create directory.");
                return false;
            }

            std::atomic<uint32_t> remainingSeqs = 0; // we don't actually need thread safe here
            const ProgressBarEvent_t* const seqExportProgress = g_pImGuiHandler->AddProgressBarEvent("Exporting Sequences..", static_cast<uint32_t>(modelAsset->numAnimSeqs), &remainingSeqs, true);
            for (int i = 0; i < modelAsset->numAnimSeqs; i++)
            {
                const uint64_t guid = modelAsset->animSeqs[i].guid;

                CPakAsset* const animSeq = g_assetData.FindAssetByGUID<CPakAsset>(guid);

                if (nullptr == animSeq)
                {
                    Log("RMDL EXPORT: animseq asset 0x%llX was not loaded, skipping...\n", guid);

                    continue;
                }

                const AnimSeqAsset* const animSeqAsset = reinterpret_cast<AnimSeqAsset*>(animSeq->extraData());

                // skip this animation if for some reason it has not been parsed. if a loaded mdl/animrig has sequence children, it should always be parsed. possibly move this to an assert.
                if (!animSeqAsset->animationParsed)
                    continue;

                outputPath.replace_filename(std::filesystem::path(animSeqAsset->name).filename());

                ExportAnimSeqAsset(animSeq, aseqAssetBinding->second.e.exportSetting, animSeqAsset, outputPath, modelAsset->name, &modelAsset->bones);

                ++remainingSeqs;
            }
            g_pImGuiHandler->FinishProgressBarEvent(seqExportProgress);
        }
    }

    exportPath.append(std::format("{}{}", modelStem, modelPath.extension().string())); // grab our extension here instead of hardcoded 'rmdl' for non pak models.

    switch (settingFixup)
    {
        case eModelExportSetting::MDL_RMDL:
        {
            return ExportRawModelAsset(modelAsset, exportPath, streamedData.get());
        }
        case eModelExportSetting::MDL_RMAX:
        {
            return ExportRMAXModelAsset(modelAsset, exportPath);
        }
        case eModelExportSetting::MDL_CAST:
        {
            return ExportCastModelAsset(modelAsset, exportPath, asset->GetAssetGUID());
        }
        case eModelExportSetting::MDL_STL_VALVE_PHYSICS:
        {
            if (asset->GetAssetContainerType() == CAsset::ContainerType::MDL)
                return false;

            if (modelAsset->version >= eMDLVersion::VERSION_16)
                return ExportPhysicsModelPhy<irps::phyheader_v16_t>(modelAsset, exportPath);
            else
                return ExportPhysicsModelPhy<irps::phyheader_v8_t>(modelAsset, exportPath);
        }
        case eModelExportSetting::MDL_STL_RESPAWN_PHYSICS:
        {
            if (asset->GetAssetContainerType() == CAsset::ContainerType::MDL)
                return false;

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

void ModelAsset::InitBoneMatrix()
{
    ID3D11Device* const device = g_dxHandler->GetDevice();

    D3D11_BUFFER_DESC desc{};

    desc.ByteWidth = static_cast<UINT>(this->bones.size()) * sizeof(matrix3x4_t);
    desc.StructureByteStride = sizeof(matrix3x4_t);

    // make sure this buffer can be updated every frame
    desc.Usage = D3D11_USAGE_DYNAMIC;
    desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;

    // pixel is a const buffer
    desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
    desc.MiscFlags = D3D11_RESOURCE_MISC_BUFFER_STRUCTURED;

    HRESULT hr = device->CreateBuffer(&desc, NULL, &drawData->boneMatrixBuffer);

#if defined(ASSERTS)
    assert(!FAILED(hr));
#else
    if (FAILED(hr))
        return;
#endif

    // SRV
    D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc{};
    srvDesc.Format = DXGI_FORMAT_UNKNOWN;
    srvDesc.ViewDimension = D3D11_SRV_DIMENSION_BUFFER;
    srvDesc.Buffer.FirstElement = 0;
    srvDesc.Buffer.NumElements = static_cast<UINT>(this->bones.size());

    hr = device->CreateShaderResourceView(drawData->boneMatrixBuffer, &srvDesc, &drawData->boneMatrixSRV);

#if defined(ASSERTS)
    assert(!FAILED(hr));
#else
    if (FAILED(hr))
        return;
#endif

    this->UpdateBoneMatrix();
}

void ModelAsset::UpdateBoneMatrix()
{
    ID3D11DeviceContext* const ctx = g_dxHandler->GetDeviceContext();

    D3D11_MAPPED_SUBRESOURCE resource;
    HRESULT hr = ctx->Map(
        drawData->boneMatrixBuffer, 0,
        D3D11_MAP_WRITE_DISCARD, 0,
        &resource
    );

    assert(SUCCEEDED(hr));

    if (FAILED(hr))
        return;

    matrix3x4_t* boneArray = reinterpret_cast<matrix3x4_t*>(resource.pData);

    int i = 0;
    for (const ModelBone_t& bone : this->bones)
    {
        const XMMATRIX bonePosMatrix = XMLoadFloat3x4(reinterpret_cast<const XMFLOAT3X4*>(&bone.poseToBone));
        XMStoreFloat3x4(reinterpret_cast<XMFLOAT3X4*>(boneArray + i), bonePosMatrix);

        i++;
    }

    ctx->Unmap(drawData->boneMatrixBuffer, 0);
}

void* PreviewModelAsset(CAsset* const asset, const bool firstFrameForAsset)
{
    ModelAsset* modelAsset = nullptr;

    switch (asset->GetAssetContainerType())
    {
    case CAsset::PAK:
    {
        CPakAsset* const pakAsset = static_cast<CPakAsset*>(asset);
        assertm(pakAsset, "Asset should be valid.");

        modelAsset = reinterpret_cast<ModelAsset*>(pakAsset->extraData());
        assertm(modelAsset, "Extra data should be valid at this point.");

        break;
    }
    case CAsset::MDL:
    {
        CSourceModelAsset* const sourceModelAsset = static_cast<CSourceModelAsset*>(asset);
        assertm(sourceModelAsset, "Asset should be valid.");

        modelAsset = sourceModelAsset->GetModelAsset();
        assertm(modelAsset, "Extra data should be valid at this point.");

        break;
    }
    default:
    {
        return nullptr;
    }
    }

    CDXDrawData* const drawData = modelAsset->drawData;
    if (!drawData)
        return nullptr;

    static std::vector<size_t> bodygroupModelSelected;
    static size_t lastSelectedBodypartIndex = 0;
    static size_t selectedBodypartIndex = 0;

    static size_t selectedSkinIndex = 0;
    static size_t lastSelectedSkinIndex = 0;

    static size_t lodLevel = 0;

    if (firstFrameForAsset)
    {
        bodygroupModelSelected.clear();

        bodygroupModelSelected.resize(modelAsset->bodyParts.size(), 0ull);

        selectedBodypartIndex = selectedBodypartIndex > modelAsset->bodyParts.size() ? 0 : selectedBodypartIndex;
        selectedSkinIndex = selectedSkinIndex > modelAsset->skins.size() ? 0 : selectedSkinIndex;
    }

    assertm(modelAsset->lods.size() > 0, "no lods in preview?");
    const ModelLODData_t& lodData = modelAsset->lods.at(lodLevel);

    ImGui::Text("Bones: %llu", modelAsset->bones.size());
    ImGui::Text("LODs: %llu", modelAsset->lods.size());
    ImGui::Text("Rigs: %i", modelAsset->numAnimRigs);
    ImGui::Text("Sequences: %i", modelAsset->numAnimSeqs);

    if (modelAsset->skins.size())
    {
        ImGui::TextUnformatted("Skins:");
        ImGui::SameLine();

        static const char* label = modelAsset->skins.at(0).name;
        if (ImGui::BeginCombo("##SKins", label, ImGuiComboFlags_NoArrowButton))
        {
            for (size_t i = 0; i < modelAsset->skins.size(); i++)
            {
                const ModelSkinData_t& skin = modelAsset->skins.at(i);

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

    if (modelAsset->bodyParts.size())
    {
        ImGui::TextUnformatted("Bodypart:");
        ImGui::SameLine();

        static const char* bodypartLabel = modelAsset->bodyParts.at(0).GetNameCStr();
        if (ImGui::BeginCombo("##Bodypart", bodypartLabel, ImGuiComboFlags_NoArrowButton))
        {
            for (size_t i = 0; i < modelAsset->bodyParts.size(); i++)
            {
                const ModelBodyPart_t& bodypart = modelAsset->bodyParts.at(i);

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

        if (modelAsset->bodyParts.at(selectedBodypartIndex).numModels > 1)
        {
            const ModelBodyPart_t& bodypart = modelAsset->bodyParts.at(selectedBodypartIndex);
            size_t& selectedModelIndex = bodygroupModelSelected.at(selectedBodypartIndex);

            ImGui::TextUnformatted("Model:");
            ImGui::SameLine();

            static const char* label = lodData.models.at(bodypart.modelIndex + selectedModelIndex).name.c_str();

            // update label if our bodypart changes
            if (selectedBodypartIndex != lastSelectedBodypartIndex)
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

    for (size_t i = 0; i < lodData.meshes.size(); ++i)
    {
        const ModelMeshData_t& mesh = lodData.meshes.at(i);
        DXMeshDrawData_t* const meshDrawData = &drawData->meshBuffers[i];

        // the rest of this loop requires the material to be valid
        // so if it isn't just continue to the next iteration
        CPakAsset* const matlAsset = modelAsset->materials.at(modelAsset->skins.at(selectedSkinIndex).indices[mesh.materialId]).materialAsset;
        if (!matlAsset)
            continue;

        const MaterialAsset* const matl = reinterpret_cast<MaterialAsset*>(matlAsset->extraData());

        // If this body part is disabled, don't draw the mesh.
        drawData->meshBuffers[i].visible = modelAsset->bodyParts[mesh.bodyPartIndex].IsPreviewEnabled();

        const ModelBodyPart_t& bodypart = modelAsset->bodyParts[mesh.bodyPartIndex];
        const ModelModelData_t& model = lodData.models.at(bodypart.modelIndex + bodygroupModelSelected.at(mesh.bodyPartIndex));
        if (i >= model.meshIndex && i < model.meshIndex + model.meshCount)
            drawData->meshBuffers[i].visible = true;
        else
            drawData->meshBuffers[i].visible = false;

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
        modelAsset->InitBoneMatrix();


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
    static const char* settings[] = { "CAST", "RMAX", "RMDL", "STL (Valve Physics)", "STL (Respawn Physics)" };
    AssetTypeBinding_t type =
    {
        .type = '_ldm',
        .headerAlignment = 8,
        .loadFunc = LoadModelAsset,
        .postLoadFunc = PostLoadModelAsset,
        .previewFunc = PreviewModelAsset,
        .e = { ExportModelAsset, 0, settings, ARRSIZE(settings) },
    };

    REGISTER_TYPE(type);
}