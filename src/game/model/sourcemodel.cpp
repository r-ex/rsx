#include <pch.h>
#include <game/model/sourcemodel.h>

extern CDXParentHandler* g_dxHandler;
extern CBufferManager* g_BufferManager;
extern ExportSettings_t g_ExportSettings;

template<typename studiohdr_t, typename mstudiomesh_t>
void ParseSourceModelVertexData(ModelAsset* const modelAsset)
{
    const studiohdr_t* const pStudioHdr = reinterpret_cast<const studiohdr_t* const>(modelAsset->data);
    const OptimizedModel::FileHeader_t* const pVTX = modelAsset->GetVTX();
    const vvd::vertexFileHeader_t* const pVVD = modelAsset->GetVVD();
    const vvc::vertexColorFileHeader_t* const pVVC = modelAsset->GetVVC();

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

    Vertex_t* const         parseVertices = reinterpret_cast<Vertex_t*>         (parseBuf->Buffer() + maxVertexBufferSize);
    Vector2D* const         parseTexcoords = reinterpret_cast<Vector2D*>        (&parseVertices[s_MaxStudioVerts]);
    uint16_t* const         parseIndices = reinterpret_cast<uint16_t*>          (&parseTexcoords[s_MaxStudioVerts * 2]);
    VertexWeight_t* const   parseWeights = reinterpret_cast<VertexWeight_t*>    (&parseIndices[s_MaxStudioTriangles]); // ~8mb for weights

    for (int lodIdx = 0; lodIdx < pVTX->numLODs; lodIdx++)
    {
        int lodMeshCount = 0;

        ModelLODData_t& lodData = modelAsset->lods.at(lodIdx);
        lodData.vertexCount = 0;
        lodData.indexCount = 0;

        for (int bdyIdx = 0; bdyIdx < pStudioHdr->numbodyparts; bdyIdx++)
        {
            const mstudiobodyparts_t* const pStudioBodyPart = pStudioHdr->pBodypart(bdyIdx);
            const OptimizedModel::BodyPartHeader_t* const pVertBodyPart = pVTX->pBodyPart(bdyIdx);

            modelAsset->SetupBodyPart(bdyIdx, pStudioBodyPart->pszName(), static_cast<int>(lodData.models.size()), pStudioBodyPart->nummodels);

            for (int modelIdx = 0; modelIdx < pStudioBodyPart->nummodels; modelIdx++)
            {
                const r1::mstudiomodel_t* const pStudioModel = pStudioBodyPart->pModel<r1::mstudiomodel_t>(modelIdx);
                const OptimizedModel::ModelHeader_t* const pVertModel = pVertBodyPart->pModel(modelIdx);

                const OptimizedModel::ModelLODHeader_t* const pVertLOD = pVertModel->pLOD(lodIdx);
                lodData.switchPoint = pVertLOD->switchPoint;
                lodData.meshes.resize(lodMeshCount + pVertLOD->numMeshes);

                ModelModelData_t modelData = {};

                // [rika]: need to edit rmax for this and I cannot get the effort to do it currently
                //modelData.name = std::filesystem::path(pStudioModel->name).stem().string();
                modelData.name = std::format("{}_{}", pStudioBodyPart->pszName(), std::to_string(modelIdx));;
                modelData.meshIndex = lodMeshCount;

                for (int meshIdx = 0; meshIdx < pStudioModel->nummeshes; ++meshIdx)
                {
                    const mstudiomesh_t* const pStudioMesh = pStudioModel->pMesh<mstudiomesh_t>(meshIdx);
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
                        const bool isHwSkinned = pStripGrp->IsHWSkinned();

                        meshData.vertCount += pStripGrp->numVerts;
                        lodData.vertexCount += pStripGrp->numVerts;

                        meshData.indexCount += pStripGrp->numIndices;
                        lodData.indexCount += pStripGrp->numIndices;

                        assertm(s_MaxStudioTriangles >= meshData.indexCount, "too many triangles");

                        for (int stripIdx = 0; stripIdx < pStripGrp->numStrips; stripIdx++)
                        {
                            OptimizedModel::StripHeader_t* pStrip = pStripGrp->pStrip(stripIdx);
                            const OptimizedModel::BoneStateChangeHeader_t* const pBoneStates = pStrip->pBoneStateChange(0);

                            for (int vertIdx = 0; vertIdx < pStrip->numVerts; vertIdx++)
                            {
                                OptimizedModel::Vertex_t* pVert = pStripGrp->pVertex(pStrip->vertOffset + vertIdx);

                                Vector2D* const texcoords = meshData.texcoordCount > 1 ? &parseTexcoords[(pStrip->vertOffset + vertIdx) * (meshData.texcoordCount - 1)] : nullptr;
                                ParseVertexFromVTX(&parseVertices[pStrip->vertOffset + vertIdx], &parseWeights[weightIdx], texcoords, &meshData, pVert, verts, tangs, colors, uv2s, weightIdx, isHwSkinned, pBoneStates);
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
            model.meshes = model.meshCount > 0 ? &lodData.meshes.at(curIdx) : nullptr;

            curIdx += model.meshCount;
        }
    }

    g_BufferManager->RelieveBuffer(parseBuf);
}

template<typename mstudiotexture_t>
void ParseSourceModelTextureData(ModelAsset* const modelAsset)
{
    const studiohdr_generic_t* const pStudioHdr = &modelAsset->studiohdr;

    const mstudiotexture_t* const pTextures = reinterpret_cast<const mstudiotexture_t* const>(pStudioHdr->baseptr + pStudioHdr->textureOffset);
    modelAsset->materials.resize(modelAsset->studiohdr.textureCount);

    for (int i = 0; i < modelAsset->studiohdr.textureCount; ++i)
    {
        ModelMaterialData_t& matlData = modelAsset->materials.at(i);
        const mstudiotexture_t* const texture = &pTextures[i];

        const std::string skn = std::format("material/{}_skn.rpak", texture->pszName());
        const std::string fix = std::format("material/{}_fix.rpak", texture->pszName());

        const uint64_t sknGUID = RTech::StringToGuid(skn.c_str());
        const uint64_t fixGUID = RTech::StringToGuid(fix.c_str());

        CPakAsset* const sknAsset = g_assetData.FindAssetByGUID<CPakAsset>(sknGUID);
        CPakAsset* const fixAsset = g_assetData.FindAssetByGUID<CPakAsset>(fixGUID);

        if (sknAsset)
        {
            matlData.materialAsset = sknAsset;
            matlData.materialGuid = sknGUID;
        }
        else if (fixAsset)
        {
            matlData.materialAsset = sknAsset;
            matlData.materialGuid = fixGUID;
        }
        else
        {
            matlData.materialAsset = nullptr;
            matlData.materialGuid = 0;
        }

        matlData.materialName = texture->pszName();
    }

    // look into this (names)
    modelAsset->skins.reserve(pStudioHdr->numSkinFamilies);
    for (int i = 0; i < pStudioHdr->numSkinFamilies; i++)
        modelAsset->skins.emplace_back(STUDIO_NULL_SKIN_NAME, pStudioHdr->pSkinFamily(i));
}

template<typename mstudiobone_t>
void ParseSouceModelBoneData(ModelAsset* const modelAsset)
{
    const mstudiobone_t* const bones = reinterpret_cast<mstudiobone_t*>((char*)modelAsset->data + modelAsset->studiohdr.boneOffset);
    modelAsset->bones.resize(modelAsset->studiohdr.boneCount);

    for (int i = 0; i < modelAsset->studiohdr.boneCount; i++)
    {
        modelAsset->bones.at(i) = ModelBone_t(&bones[i]);
    }
}

void LoadSourceModelAsset(CAssetContainer* container, CAsset* asset)
{
    CSourceModelAsset* const srcMdlAsset = static_cast<CSourceModelAsset* const>(asset);
    CSourceModelSource* const srcMdlSource = static_cast<CSourceModelSource* const>(container);

    ModelAsset* mdlAsset = nullptr;

    switch (srcMdlAsset->GetAssetVersion().majorVer)
    {
    case 52:
    {
        r1::studiohdr_t* const pStudioHdr = reinterpret_cast<r1::studiohdr_t* const>(srcMdlAsset->GetAssetData());

        // next section is to fill out the StudioLooseData_t struct so we can properly store the loose files
        // parse and load our vertex files
        StudioLooseData_t looseData = {};

        std::filesystem::path filePath = srcMdlSource->GetFilePath();

        CManagedBuffer* buffer = g_BufferManager->ClaimBuffer();
        char* curpos = buffer->Buffer(); // current position in the buffer
        size_t curoff = 0ull; // current offset in the buffer

        const char* const fileStem = keepAfterLastSlashOrBackslash(pStudioHdr->pszName());

        for (int i = 0; i < StudioLooseData_t::SLD_COUNT; i++)
        {
            filePath.replace_filename(fileStem); // because of vtx file extension
            filePath.replace_extension(s_StudioLooseDataExtensions[i]);

            if (!std::filesystem::exists(filePath))
            {
                looseData.vertexDataOffset[i] = static_cast<int>(curoff);
                looseData.vertexDataSize[i] = 0;

                continue;
            }

            const size_t fileSize = std::filesystem::file_size(filePath);
            std::unique_ptr<char[]> fileBuf = std::make_unique<char[]>(fileSize);

            StreamIO fileIn(filePath, eStreamIOMode::Read);
            fileIn.R()->read(curpos, fileSize);

            looseData.vertexDataOffset[i] = static_cast<int>(curoff);
            looseData.vertexDataSize[i] = static_cast<int>(fileSize);

            curoff += IALIGN16(fileSize);
            curpos += IALIGN16(fileSize);

            assertm(curoff < managedBufferSize, "overflowed managed buffer");
        }

        char* const vertexBuf = new char[curoff];
        memcpy_s(vertexBuf, curoff, buffer->Buffer(), curoff);
        srcMdlAsset->SetExtraData(vertexBuf, CSourceModelAsset::SRCMDL_VERT);

        // parse and load phys
        {
            filePath.replace_extension(".phy");

            if (!std::filesystem::exists(filePath))
            {
                looseData.physicsDataOffset = 0;
                looseData.physicsDataSize = 0;
            }
            else
            {
                const size_t fileSize = std::filesystem::file_size(filePath);
                std::unique_ptr<char[]> fileBuf = std::make_unique<char[]>(fileSize);

                StreamIO fileIn(filePath, eStreamIOMode::Read);
                fileIn.R()->read(buffer->Buffer(), fileSize);

                looseData.physicsDataOffset = 0;
                looseData.physicsDataSize = static_cast<int>(fileSize);
            }
        }

        char* const physicsBuf = new char[looseData.physicsDataSize];
        memcpy_s(physicsBuf, looseData.physicsDataSize, buffer->Buffer(), looseData.physicsDataSize);
        srcMdlAsset->SetExtraData(physicsBuf, CSourceModelAsset::SRCMDL_PHYS);

        // here's where ani will go when I do animations (soontm)

        g_BufferManager->RelieveBuffer(buffer);

        looseData.vertexDataBuffer = srcMdlAsset->GetExtraData(CSourceModelAsset::SRCMDL_VERT);
        looseData.physicsDataBuffer = srcMdlAsset->GetExtraData(CSourceModelAsset::SRCMDL_PHYS);

        // model asset
        mdlAsset = new ModelAsset(pStudioHdr, &looseData);

        ParseSouceModelBoneData<r1::mstudiobone_t>(mdlAsset);
        ParseSourceModelTextureData<r1::mstudiotexture_t>(mdlAsset);
        ParseSourceModelVertexData<r1::studiohdr_t, r1::mstudiomesh_t>(mdlAsset);

        break;
    }
    case 53:
    {
        r2::studiohdr_t* const pStudioHdr = reinterpret_cast<r2::studiohdr_t* const>(srcMdlAsset->GetAssetData());
        mdlAsset = new ModelAsset(pStudioHdr);

        ParseSouceModelBoneData<r2::mstudiobone_t>(mdlAsset);
        ParseSourceModelTextureData<r2::mstudiotexture_t>(mdlAsset);
        ParseSourceModelVertexData<r2::studiohdr_t, r2::mstudiomesh_t>(mdlAsset);

        // include models

        break;
    }
    default:
    {
        assertm(false, "unaccounted asset version, will cause major issues!");
        return;
    }
    }

    srcMdlAsset->SetAssetName(mdlAsset->name);
    srcMdlAsset->SetModelAsset(mdlAsset);

    srcMdlSource->SetFileName(keepAfterLastSlashOrBackslash(mdlAsset->name));
}

void PostLoadSourceModelAsset(CAssetContainer* container, CAsset* asset)
{
    UNUSED(container);

    CSourceModelAsset* const srcMdlAsset = static_cast<CSourceModelAsset* const>(asset);
    ModelAsset* const modelAsset = srcMdlAsset->GetModelAsset();

    if (modelAsset->lods.empty())
        return;

    const size_t lodLevel = 0;

    if (!modelAsset->drawData)
    {
        modelAsset->drawData = new CDXDrawData();
        modelAsset->drawData->meshBuffers.resize(modelAsset->lods.at(lodLevel).meshes.size());
        modelAsset->drawData->modelName = modelAsset->name;
    }

    // [rika]: eventually parse through models
    CDXDrawData* const drawData = modelAsset->drawData;
    for (size_t i = 0; i < modelAsset->lods.at(lodLevel).meshes.size(); ++i)
    {
        const ModelMeshData_t& mesh = modelAsset->lods.at(lodLevel).meshes.at(i);
        DXMeshDrawData_t* const meshDrawData = &drawData->meshBuffers[i];

        meshDrawData->visible = true;

        if (mesh.material)
            meshDrawData->uberStaticBuf = mesh.material->uberStaticBuffer;

        assertm(mesh.meshVertexDataIndex != invalidNoodleIdx, "mesh data hasn't been parsed ??");

        std::unique_ptr<char[]> parsedVertexDataBuf = modelAsset->meshVertexData.getIdx(mesh.meshVertexDataIndex);
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

extern void* PreviewModelAsset(CAsset* const asset, const bool firstFrameForAsset);
extern bool ExportModelAsset(CAsset* const asset, const int setting);

void InitSourceModelAssetType()
{
    AssetTypeBinding_t type =
    {
        .type = 'ldm',
        .headerAlignment = 8,
        .loadFunc = LoadSourceModelAsset,
        .postLoadFunc = PostLoadSourceModelAsset,
        .previewFunc = PreviewModelAsset,
        .e = { ExportModelAsset, 0, nullptr, 0ull },
    };

    REGISTER_TYPE(type);
}

void LoadSourceSequenceAsset(CAssetContainer* container, CAsset* asset)
{
    UNUSED(container);

    CSourceSequenceAsset* const srcSeqAsset = static_cast<CSourceSequenceAsset* const>(asset);
    seqdesc_t* sequence = nullptr;

    switch (srcSeqAsset->GetAssetVersion().majorVer)
    {
    case 52:
    {
        break;
    }
    case 53:
    {
        r2::mstudioseqdesc_t* const pSeqdesc = reinterpret_cast<r2::mstudioseqdesc_t* const>(srcSeqAsset->GetAssetData());
        sequence = new seqdesc_t(pSeqdesc);

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

    srcSeqAsset->SetRig(&srcMdlAsset->GetModelAsset()->bones); // bone
    const std::vector<ModelBone_t>* bones = srcSeqAsset->GetRig();
    assertm(!bones->empty(), "we should have bones at this point.");

    const size_t boneCount = bones->size();

    seqdesc_t* const seqdesc = srcSeqAsset->GetSequence();
    switch (srcSeqAsset->GetAssetVersion().majorVer)
    {
    case 52:
    {
        break;
    }
    case 53:
    {
        const r2::studiohdr_t* const pStudioHdr = reinterpret_cast<const r2::studiohdr_t* const>(srcMdlAsset->GetAssetData());

        for (int i = 0; i < seqdesc->AnimCount(); i++)
        {
            animdesc_t* const animdesc = &seqdesc->anims.at(i);

            // no point to allocate memory on empty animations!
            CAnimData animData(boneCount, animdesc->numframes);
            animData.ReserveVector();

            for (int frameIdx = 0; frameIdx < animdesc->numframes; frameIdx++)
            {
                const float cycle = animdesc->numframes > 1 ? static_cast<float>(frameIdx) / static_cast<float>(animdesc->numframes - 1) : 0.0f;
                assertm(isfinite(cycle), "cycle was nan");

                float fFrame = cycle * static_cast<float>(animdesc->numframes - 1);

                const int iFrame = static_cast<int>(fFrame);
                const float s = (fFrame - static_cast<float>(iFrame));

                int iLocalFrame = iFrame;

                const r2::mstudio_rle_anim_t* panim = reinterpret_cast<const r2::mstudio_rle_anim_t*>(animdesc->pAnimdataNoStall(&iLocalFrame));

                for (int boneIdx = 0; boneIdx < boneCount; boneIdx++)
                {
                    const ModelBone_t* const bone = &bones->at(boneIdx);

                    Vector pos;
                    Quaternion q;
                    Vector scale;

                    // r2 animations will always have position
                    uint8_t boneFlags = CAnimDataBone::ANIMDATA_POS 
                                        | (panim->flags & r2::RleFlags_t::STUDIO_ANIM_NOROT ? 0 : CAnimDataBone::ANIMDATA_ROT)
                                        | (seqdesc->flags & 0x20000 ? CAnimDataBone::ANIMDATA_SCL : 0);

                    if (panim && panim->bone == boneIdx)
                    {
                        // need to add a bool here, we do not want the interpolated values (inbetween frames)
                        r2::CalcBonePosition(iLocalFrame, s, pStudioHdr->pBone(panim->bone), pStudioHdr->pLinearBones(), panim, pos);
                        r2::CalcBoneQuaternion(iLocalFrame, s, pStudioHdr->pBone(panim->bone), pStudioHdr->pLinearBones(), panim, q);
                        r2::CalcBoneScale(iLocalFrame, s, pStudioHdr->pBone(panim->bone)->scale, pStudioHdr->pBone(panim->bone)->scalescale, panim, scale);

                        panim = panim->pNext();
                    }
                    else
                    {
                        if (animdesc->flags & eStudioAnimFlags::ANIM_DELTA)
                        {
                            pos.Init(0.0f, 0.0f, 0.0f);
                            q.Init(0.0f, 0.0f, 0.0f, 1.0f);
                            scale.Init(1.0f, 1.0f, 1.0f);
                        }
                        else
                        {
                            pos = bone->pos;
                            q = bone->quat;
                            scale = bone->scale;
                        }
                    }

                    // adjust the origin bone
                    // do after we get anim data, so our rotation does not get overwritten
                    if (boneIdx == 0)
                    {
                        QAngle vecAngleBase(q);

                        if (nullptr != animdesc->movement && animdesc->flags & eStudioAnimFlags::ANIM_FRAMEMOVEMENT)
                        {
                            Vector vecPos;
                            QAngle vecAngle;

                            r1::Studio_AnimPosition(animdesc, cycle, vecPos, vecAngle);

                            pos += vecPos; // add our base movement position to our base position 
                            vecAngleBase.y += (vecAngle.y);
                        }

                        vecAngleBase.y += -90; // rotate -90 degree on the yaw axis

                        // adjust position as we are rotating on the Z axis
                        const float x = pos.x;
                        const float y = pos.y;

                        pos.x = y;
                        pos.y = -x;

                        AngleQuaternion(vecAngleBase, q);

                        // has pos/rot data regardless since we just adjusted pos/rot
                        boneFlags |= (CAnimDataBone::ANIMDATA_POS | CAnimDataBone::ANIMDATA_ROT);
                    }

                    CAnimDataBone& animDataBone = animData.GetBone(boneIdx);
                    animDataBone.SetFlags(boneFlags);
                    animDataBone.SetFrame(frameIdx, pos, q, scale);
                }
            }

            // parse into memory and compress
            CManagedBuffer* buffer = g_BufferManager->ClaimBuffer();

            const size_t sizeInMem = animData.ToMemory(buffer->Buffer());
            animdesc->parsedBufferIndex = seqdesc->parsedData.addBack(buffer->Buffer(), sizeInMem);

            g_BufferManager->RelieveBuffer(buffer);
        }

        break;
    }
    default:
        break;
    }

    // the sequence has been parsed for exporting
    srcSeqAsset->SetParsed();
}

extern bool ExportSeqDescRMAX(const seqdesc_t* const seqdesc, std::filesystem::path& exportPath, const char* const skelName, const std::vector<ModelBone_t>* const bones);
extern bool ExportSeqDescCast(const seqdesc_t* const seqdesc, std::filesystem::path& exportPath, const char* const skelName, const std::vector<ModelBone_t>* const bones, const uint64_t guid);

// MUST MATCH ASEQ!
enum eSrcSeqExportSetting
{
    SEQ_CAST,
    SEQ_RMAX,
    SEQ_NULL, // RSEQ
};

static const char* const s_PathPrefixASEQ = s_AssetTypePaths.find(AssetType_t::ASEQ)->second;
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
    std::filesystem::path exportPath = std::filesystem::current_path().append(EXPORT_DIRECTORY_NAME);
    const std::filesystem::path seqPath(srcSeqAsset->GetAssetName());
    const std::string seqStem(seqPath.stem().string());
    const std::string srcStem(std::filesystem::path(srcSeqAsset->GetContainerFileName()).stem().string());

    // truncate paths?
    if (g_ExportSettings.exportPathsFull)
        exportPath.append(seqPath.parent_path().string());
    else
        exportPath.append(std::format("{}/{}/{}", s_PathPrefixASEQ, srcStem, seqStem));

    if (!CreateDirectories(exportPath))
    {
        assertm(false, "Failed to create asset directory.");
        return false;
    }

    exportPath.append(std::format("{}.seq", seqStem));

    const std::vector<ModelBone_t>* bones = srcSeqAsset->GetRig();
    assertm(!bones->empty(), "we should have bones at this point.");

    std::filesystem::path exportPathCop = exportPath;

    switch (settingFixup)
    {

    case eSrcSeqExportSetting::SEQ_CAST:
    {
        return ExportSeqDescCast(srcSeqAsset->GetSequence(), exportPathCop, srcMdlAsset->GetAssetName().c_str(), bones, asset->GetAssetGUID());
    }
    case eSrcSeqExportSetting::SEQ_RMAX:
    {
        return ExportSeqDescRMAX(srcSeqAsset->GetSequence(), exportPathCop, srcMdlAsset->GetAssetName().c_str(), bones);
    }
    default:
    {
        assertm(false, "Export setting is not handled.");
        return false;
    }
    }
}

void InitSourceSequenceAssetType()
{
    AssetTypeBinding_t type =
    {
        .type = 'qes',
        .headerAlignment = 8,
        .loadFunc = LoadSourceSequenceAsset,
        .postLoadFunc = PostLoadSourceSequenceAsset,
        .previewFunc = nullptr,
        .e = { ExportSourceSequenceAsset, 0, nullptr, 0ull },
    };

    REGISTER_TYPE(type);
}