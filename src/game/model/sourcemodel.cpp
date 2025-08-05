#include <pch.h>
#include <core/mdl/modeldata.h>
#include <game/model/sourcemodel.h>

#include <core/render/dx.h>
#include <thirdparty/imgui/imgui.h>
#include <thirdparty/imgui/misc/imgui_utility.h>

extern CDXParentHandler* g_dxHandler;
extern CBufferManager g_BufferManager;
extern ExportSettings_t g_ExportSettings;

static const char* const s_PathPrefixMDL = s_AssetTypePaths.find(AssetType_t::MDL)->second;
static const char* const s_PathPrefixSEQ = s_AssetTypePaths.find(AssetType_t::SEQ)->second;

void CSourceModelAsset::FixupSkinData()
{
    const int skinCount = static_cast<int>(m_modelParsed->skins.size());

    // [rika]: for models with only animations basically.
    if (skinCount == 0)
        return;

    ModelSkinData_t* const skinData = &m_modelParsed->skins.front();
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

// [rika]: check how practical it is to use this for v8 rmdl
template<typename studiohdr_t, typename mstudiomesh_t>
void ParseSourceModelVertexData(ModelParsedData_t* const parsedData, StudioLooseData_t* const looseData)
{
    const studiohdr_t* const pStudioHdr = reinterpret_cast<const studiohdr_t* const>(parsedData->pStudioHdr()->baseptr);
    const OptimizedModel::FileHeader_t* const pVTX = looseData->GetVTX();
    const vvd::vertexFileHeader_t* const pVVD = looseData->GetVVD();
    const vvc::vertexColorFileHeader_t* const pVVC = looseData->GetVVC();

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

    parsedData->lods.resize(pVTX->numLODs);
    parsedData->bodyParts.resize(pStudioHdr->numbodyparts);

    constexpr size_t maxVertexDataSize = sizeof(vvd::mstudiovertex_t) + sizeof(Vector4D) + sizeof(Vector2D) + sizeof(Color32);
    constexpr size_t maxVertexBufferSize = maxVertexDataSize * s_MaxStudioVerts;

    // needed due to how vtx is parsed!
    CManagedBuffer* const   parseBuf = g_BufferManager.ClaimBuffer();

    Vertex_t* const         parseVertices = reinterpret_cast<Vertex_t*>         (parseBuf->Buffer() + maxVertexBufferSize);
    Vector2D* const         parseTexcoords = reinterpret_cast<Vector2D*>        (&parseVertices[s_MaxStudioVerts]);
    uint16_t* const         parseIndices = reinterpret_cast<uint16_t*>          (&parseTexcoords[s_MaxStudioVerts * 2]);
    VertexWeight_t* const   parseWeights = reinterpret_cast<VertexWeight_t*>    (&parseIndices[s_MaxStudioTriangles]); // ~8mb for weights

    for (int lodIdx = 0; lodIdx < pVTX->numLODs; lodIdx++)
    {
        int lodMeshCount = 0;

        ModelLODData_t& lodData = parsedData->lods.at(lodIdx);
        lodData.vertexCount = 0;
        lodData.indexCount = 0;

        for (int bdyIdx = 0; bdyIdx < pStudioHdr->numbodyparts; bdyIdx++)
        {
            const mstudiobodyparts_t* const pStudioBodyPart = pStudioHdr->pBodypart(bdyIdx);
            const OptimizedModel::BodyPartHeader_t* const pVertBodyPart = pVTX->pBodyPart(bdyIdx);

            parsedData->SetupBodyPart(bdyIdx, pStudioBodyPart->pszName(), static_cast<int>(lodData.models.size()), pStudioBodyPart->nummodels);

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
                                Vertex_t::ParseVertexFromVTX(&parseVertices[pStrip->vertOffset + vertIdx], &parseWeights[weightIdx], texcoords, &meshData, pVert, verts, tangs, colors, uv2s, weightIdx, isHwSkinned, pBoneStates);
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
            model.meshes = model.meshCount > 0 ? &lodData.meshes.at(curIdx) : nullptr;

            curIdx += model.meshCount;
        }
    }

    g_BufferManager.RelieveBuffer(parseBuf);
}

template<typename mstudiotexture_t>
void ParseSourceModelTextureData(ModelParsedData_t* const parsedData)
{
    const studiohdr_generic_t* const pStudioHdr = parsedData->pStudioHdr();

    const mstudiotexture_t* const pTextures = reinterpret_cast<const mstudiotexture_t* const>(pStudioHdr->pTextures());
    parsedData->materials.resize(pStudioHdr->textureCount);

    for (int i = 0; i < pStudioHdr->textureCount; ++i)
    {
        ModelMaterialData_t& matlData = parsedData->materials.at(i);
        const mstudiotexture_t* const texture = &pTextures[i];

        const std::string skn = std::format("material/{}_skn.rpak", texture->pszName());
        const std::string fix = std::format("material/{}_fix.rpak", texture->pszName());

        const uint64_t sknGUID = RTech::StringToGuid(skn.c_str());
        const uint64_t fixGUID = RTech::StringToGuid(fix.c_str());

        CPakAsset* const sknAsset = g_assetData.FindAssetByGUID<CPakAsset>(sknGUID);
        CPakAsset* const fixAsset = g_assetData.FindAssetByGUID<CPakAsset>(fixGUID);

        if (sknAsset)
        {
            matlData.asset = sknAsset;
            matlData.guid = sknGUID;
        }
        else if (fixAsset)
        {
            matlData.asset = sknAsset;
            matlData.guid = fixGUID;
        }
        else
        {
            matlData.asset = nullptr;
            matlData.guid = 0;
        }

        matlData.SetName(texture->pszName());
    }

    // [rika]: skin names will be fixed up in the load function
    parsedData->skins.reserve(pStudioHdr->numSkinFamilies);
    for (int i = 0; i < pStudioHdr->numSkinFamilies; i++)
        parsedData->skins.emplace_back(STUDIO_NULL_SKIN_NAME, pStudioHdr->pSkinFamily(i));
}

template<typename mstudiobone_t>
void ParseSouceModelBoneData(ModelParsedData_t* const parsedData)
{
    const studiohdr_generic_t* const pStudioHdr = parsedData->pStudioHdr();

    const mstudiobone_t* const bones = reinterpret_cast<const mstudiobone_t* const>(pStudioHdr->pBones());
    parsedData->bones.resize(pStudioHdr->boneCount);

    for (int i = 0; i < pStudioHdr->boneCount; i++)
        parsedData->bones.at(i) = ModelBone_t(&bones[i]);
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

        looseData = new StudioLooseData_t(srcMdlSource->GetFilePath(), pStudioHdr->pszName(), buffer->Buffer());

        g_BufferManager.RelieveBuffer(buffer);

        // these are now managed by the asset
        srcMdlAsset->SetExtraData(looseData->VertBuf(), CSourceModelAsset::SRCMDL_VERT);
        srcMdlAsset->SetExtraData(looseData->PhysBuf(), CSourceModelAsset::SRCMDL_PHYS);

        // parsed data
        parsedData = new ModelParsedData_t(pStudioHdr, looseData);

        ParseSouceModelBoneData<r1::mstudiobone_t>(parsedData);
        ParseSourceModelTextureData<r1::mstudiotexture_t>(parsedData);
        ParseSourceModelVertexData<r1::studiohdr_t, r1::mstudiomesh_t>(parsedData, looseData);

        srcMdlAsset->SetName(pStudioHdr->pszName());

        break;
    }
    case 53:
    {
        r2::studiohdr_t* const pStudioHdr = reinterpret_cast<r2::studiohdr_t* const>(srcMdlAsset->GetAssetData());
        looseData = new StudioLooseData_t(reinterpret_cast<char*>(pStudioHdr));
        parsedData = new ModelParsedData_t(pStudioHdr);

        ParseSouceModelBoneData<r2::mstudiobone_t>(parsedData);
        ParseSourceModelTextureData<r2::mstudiotexture_t>(parsedData);
        ParseSourceModelVertexData<r2::studiohdr_t, r2::mstudiomesh_t>(parsedData, looseData);

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

    srcMdlSource->SetFileName(keepAfterLastSlashOrBackslash(srcMdlAsset->GetName()));

    srcMdlAsset->FixupSkinData();
}

void PostLoadSourceModelAsset(CAssetContainer* container, CAsset* asset)
{
    UNUSED(container);

    CSourceModelAsset* const srcMdlAsset = static_cast<CSourceModelAsset* const>(asset);
    ModelParsedData_t* const parsedData = srcMdlAsset->GetParsedData();
    
    if (parsedData->lods.empty())
        return;

    const size_t lod = 0;

    if (!srcMdlAsset->GetDrawData())
        srcMdlAsset->AllocateDrawData(lod);

    ParseModelDrawData(parsedData, srcMdlAsset->GetDrawData(), lod);
}

// [rika]: todo remove duplicate code and make one function (PreviewModelAsset)
void* PreviewSourceModelAsset(CAsset* const asset, const bool firstFrameForAsset)
{
    CSourceModelAsset* const srcMdlAsset = static_cast<CSourceModelAsset*>(asset);
    assertm(srcMdlAsset, "Asset should be valid.");

    CDXDrawData* const drawData = srcMdlAsset->GetDrawData();
    if (!drawData)
        return nullptr;

    drawData->vertexShader = g_dxHandler->GetShaderManager()->LoadShaderFromString("shaders/model_vs", s_PreviewVertexShader, eShaderType::Vertex);;
    drawData->pixelShader = g_dxHandler->GetShaderManager()->LoadShaderFromString("shaders/model_ps", s_PreviewPixelShader, eShaderType::Pixel);

    ModelParsedData_t* const parsedData = srcMdlAsset->GetParsedData();

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
    //ImGui::Text("Rigs: %i", modelAsset->numAnimRigs);
    //ImGui::Text("Sequences: %i", modelAsset->numAnimSeqs);
    ImGui::Text("Local Sequences: %i", parsedData->NumLocalSeq());

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

    // [rika]: our currently selected skin
    const ModelSkinData_t& skinData = parsedData->skins.at(selectedSkinIndex);
    for (size_t i = 0; i < lodData.meshes.size(); ++i)
    {
        const ModelMeshData_t& mesh = lodData.meshes.at(i);
        DXMeshDrawData_t* const meshDrawData = &drawData->meshBuffers[i];

        meshDrawData->indexFormat = DXGI_FORMAT_R16_UINT;

        // If this body part is disabled, don't draw the mesh.
        drawData->meshBuffers[i].visible = parsedData->bodyParts[mesh.bodyPartIndex].IsPreviewEnabled();

        const ModelBodyPart_t& bodypart = parsedData->bodyParts[mesh.bodyPartIndex];
        const ModelModelData_t& model = lodData.models.at(bodypart.modelIndex + bodygroupModelSelected.at(mesh.bodyPartIndex));
        if (i >= model.meshIndex && i < model.meshIndex + model.meshCount)
            drawData->meshBuffers[i].visible = true;
        else
            drawData->meshBuffers[i].visible = false;

        // the rest of this loop requires the material to be valid
        // so if it isn't just continue to the next iteration
        CPakAsset* const matlAsset = parsedData->materials.at(skinData.indices[mesh.materialId]).asset;
        if (!matlAsset)
            continue;

        const MaterialAsset* const matl = reinterpret_cast<MaterialAsset*>(matlAsset->extraData());

#if defined(ADVANCED_MODEL_PREVIEW)
        if (matl->shaderSetAsset)
        {
            ShaderSetAsset* const shaderSet = reinterpret_cast<ShaderSetAsset*>(matl->shaderSetAsset->extraData());

            if (shaderSet->vertexShaderAsset && shaderSet->pixelShaderAsset)
            {
                ShaderAsset* const vertexShader = reinterpret_cast<ShaderAsset*>(shaderSet->vertexShaderAsset->extraData());
                ShaderAsset* const pixelShader = reinterpret_cast<ShaderAsset*>(shaderSet->pixelShaderAsset->extraData());

                meshDrawData->vertexShader = vertexShader->vertexShader;
                meshDrawData->pixelShader = pixelShader->pixelShader;

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

    return srcMdlAsset->GetDrawData();
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
    std::filesystem::path exportPath = std::filesystem::current_path().append(EXPORT_DIRECTORY_NAME);
    const std::filesystem::path modelPath(srcMdlAsset->GetAssetName());
    const std::string modelStem(modelPath.stem().string());

    // truncate paths?
    if (g_ExportSettings.exportPathsFull)
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
    if (g_ExportSettings.exportRigSequences && srcMdlAsset->GetSequenceCount() > 0)
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
    {
        return ExportModelCast(parsedData, exportPath, asset->GetAssetGUID());
    }
    case eModelExportSetting::MODEL_RMAX:
    {
        return ExportModelRMAX(parsedData, exportPath);
    }
    case eModelExportSetting::MODEL_SMD:
    {
        return ExportModelSMD(parsedData, exportPath);
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

    srcSeqAsset->SetRig(srcMdlAsset->GetRig()); // bone
    const std::vector<ModelBone_t>* bones = srcSeqAsset->GetRig();
    assertm(!bones->empty(), "we should have bones at this point.");

    seqdesc_t* const seqdesc = srcSeqAsset->GetSequence();
    switch (srcSeqAsset->GetAssetVersion().majorVer)
    {
    case 52:
    {
        break;
    }
    case 53:
    {
        ParseSeqDesc_R2(seqdesc, bones, reinterpret_cast<const r2::studiohdr_t* const>(srcMdlAsset->GetAssetData()));

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
    std::filesystem::path exportPath = std::filesystem::current_path().append(EXPORT_DIRECTORY_NAME);
    const std::filesystem::path seqPath(srcSeqAsset->GetAssetName());
    const std::string seqStem(seqPath.stem().string());
    const std::string srcStem(std::filesystem::path(srcSeqAsset->GetContainerFileName()).stem().string());

    // truncate paths?
    if (g_ExportSettings.exportPathsFull)
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

    const std::vector<ModelBone_t>* bones = srcSeqAsset->GetRig();
    assertm(!bones->empty(), "we should have bones at this point.");

    std::filesystem::path exportPathCop = exportPath;

    return ExportSeqDesc(settingFixup, srcSeqAsset->GetSequence(), exportPath, srcMdlAsset->GetAssetName().c_str(), bones, asset->GetAssetGUID());
}

void InitSourceSequenceAssetType()
{
    AssetTypeBinding_t type =
    {
        .type = 'qes',
        .headerAlignment = 8,
        .loadFunc = LoadSourceSequenceAsset,
        .postLoadFunc = PostLoadSourceSequenceAsset,
        .previewFunc = PreviewSourceSequenceAsset,
        .e = { ExportSourceSequenceAsset, 0, nullptr, 0ull },
    };

    REGISTER_TYPE(type);
}