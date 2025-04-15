#include <pch.h>
#include <game/rtech/assets/material.h>
#include <game/rtech/assets/texture.h>

#include <thirdparty/imgui/imgui.h>
#include <thirdparty/imgui/misc/imgui_utility.h>
#include <core/render/dx.h>
#include <core/render/dxutils.h>

extern CDXParentHandler* g_dxHandler;
extern ExportSettings_t g_ExportSettings;

const MaterialShaderType_t GetTypeFromLegacyMaterial(std::filesystem::path& materialPath, const uint64_t guid)
{
    const std::string stem(materialPath.stem().string());

    for (uint8_t i = 0; i < MaterialShaderType_t::_TYPE_LEGACY_COUNT; i++)
    {
        const MaterialShaderType_t type = static_cast<MaterialShaderType_t>(MaterialShaderType_t::SKN + i);
        materialPath.replace_filename(std::format("{}{}.rpak", stem, GetMaterialShaderSuffixByType(type)));

        const uint64_t hash = RTech::StringToGuid(materialPath.string().c_str());

        if (hash != guid)
            continue;

        return type;
    }

    Log("** failed to find type for material %s\n", materialPath.string().c_str());

    return MaterialShaderType_t::_TYPE_LEGACY;
}

#undef max
void LoadMaterialAsset(CAssetContainer* const pak, CAsset* const asset)
{
    UNUSED(pak);
    CPakAsset* pakAsset = static_cast<CPakAsset*>(asset);


    MaterialAsset* materialAsset = nullptr;

    switch (pakAsset->version())
    {
    case 12:
    {
        MaterialAssetHeader_v12_t* v12 = reinterpret_cast<MaterialAssetHeader_v12_t*>(pakAsset->header());
        materialAsset = new MaterialAsset(v12, reinterpret_cast<MaterialAssetCPU_t*>(pakAsset->cpu()));
        break;
    }
    case 15:
    {
        MaterialAssetHeader_v15_t* v15 = reinterpret_cast<MaterialAssetHeader_v15_t*>(pakAsset->header());
        materialAsset = new MaterialAsset(v15, reinterpret_cast<MaterialAssetCPU_t*>(pakAsset->cpu()));
        break;
    }
    case 16:
    case 17:
    case 18:
    case 19:
    case 20:
    case 21:
    {
        MaterialAssetHeader_v16_t* v16 = reinterpret_cast<MaterialAssetHeader_v16_t*>(pakAsset->header());
        materialAsset = new MaterialAsset(v16, reinterpret_cast<MaterialAssetCPU_t*>(pakAsset->cpu()));
        break;
    }
    case 22:
    case 23:
    {
        MaterialAssetHeader_v22_t* v22 = reinterpret_cast<MaterialAssetHeader_v22_t*>(pakAsset->header());
        materialAsset = new MaterialAsset(v22, reinterpret_cast<MaterialAssetCPU_t*>(pakAsset->cpu()));
        break;
    }
    default:
        return;
    }

    std::string materialName = materialAsset->name;

    if (!materialName.ends_with(".rpak"))
    {
        std::filesystem::path materialPath = "material/" + materialName;

        if (materialAsset->materialType != _TYPE_LEGACY)
        {
            materialPath.replace_filename(std::format("{}{}.rpak", materialPath.stem().string(), GetMaterialShaderSuffixByType(materialAsset->materialType)));
        }
        else
        {
            materialAsset->materialType = GetTypeFromLegacyMaterial(materialPath, materialAsset->guid);

            if (materialAsset->materialType == MaterialShaderType_t::_TYPE_LEGACY)
                materialPath = std::format("material/{}", materialAsset->name);
        }

        materialName = materialPath.string();
    }
 
    pakAsset->SetAssetName(materialName);
    pakAsset->setExtraData(materialAsset);
}

void PostLoadMaterialAsset(CAssetContainer* const pak, CAsset* const asset)
{
    UNUSED(pak);

    CPakAsset* pakAsset = static_cast<CPakAsset*>(asset);

    switch (pakAsset->version())
    {
    case 12:
    case 15:
    case 16:
    case 17:
    case 18:
    case 19:
    case 20:
    case 21:
    case 22:
    case 23:
        break;
    default:
        return;
    }

    MaterialAsset* const materialAsset = reinterpret_cast<MaterialAsset*>(pakAsset->extraData());
    assertm(materialAsset, "Extra data should be valid at this point.");

    materialAsset->shaderSetAsset = g_assetData.FindAssetByGUID<CPakAsset>(materialAsset->shaderSet);

    if (materialAsset->shaderSetAsset)
    {
        ShaderSetAsset* const shdsAsset = reinterpret_cast<ShaderSetAsset*>(materialAsset->shaderSetAsset->extraData());

        if (shdsAsset->pixelShaderAsset)
        {
            materialAsset->resourceBindings = ResourceBindingFromDXBlob(shdsAsset->pixelShaderAsset, D3D10_SIT_TEXTURE);
            materialAsset->cpuDataBuf = ConstBufVarFromDXBlob(shdsAsset->pixelShaderAsset, "CBufUberStatic");
        }
    }

    // streamingTextureHandles always follows immediately after the textureHandles data, so we can calculate
    // the size of textureHandles by getting the difference between the two pointers and dividing it by the size of
    // a single texture handle
    const uint32_t txtrCount = static_cast<uint32_t>((static_cast<char*>(materialAsset->streamingTextureHandles) - static_cast<char*>(materialAsset->textureHandles)) / sizeof(uint64_t));
    const uint64_t* txtrHandles = static_cast<uint64_t*>(materialAsset->textureHandles);

    for (uint32_t i = 0; i < txtrCount; ++i, ++txtrHandles)
    {
        const uint64_t txtrGuid = *txtrHandles;

        // if there is no texture in this handle slot, the guid will be 0
        if (txtrGuid == 0)
            continue;

        CPakAsset* const textureAsset = g_assetData.FindAssetByGUID<CPakAsset>(txtrGuid);
        materialAsset->txtrAssets.push_back(TextureAssetEntry_t(textureAsset, static_cast<uint32_t>(i)));
    }

    if (materialAsset->cpuData)
    {
        CreateD3DBuffer(g_dxHandler->GetDevice(),
            &materialAsset->uberStaticBuffer, materialAsset->cpuDataSize,
            D3D11_USAGE_IMMUTABLE, D3D11_BIND_CONSTANT_BUFFER,
            0, 0, 0, materialAsset->cpuData
        );
    }

}

struct MaterialTexturePreviewData_t
{
    enum eTextureStateFlags
    {
        TEXSF_UNLOADED = 0x1, // When this texture asset is not currently loaded in the selected paks
        TEXSF_RES_UNKNOWN = 0x2, // When the shaderset for the material is unknown or it does not specify a resource type for this texture
    };

    enum eColumnID
    {
        MTPC_ResBindPoint,
        MTPC_TextureGUID,
        MTPC_TextureName,
        MTPC_ResBindingName,
        MTPC_Dimensions,
        MTPC_Status,


        _MTPC_COUNT,
    };

    std::string textureName;
    std::string resourceBindingName;
    uint64_t textureAssetGuid;
    int materialTextureIndex; // index in material texture assets
    int resourceBindPoint;
    int width;
    int height;
    int stateFlags;


    FORCEINLINE const bool IsTextureLoaded() const
    {
        return (stateFlags & TEXSF_UNLOADED) == 0;
    }

    FORCEINLINE const bool HasResourceType() const
    {
        return (stateFlags & TEXSF_RES_UNKNOWN) == 0;
    }

    bool operator==(const MaterialTexturePreviewData_t& rhs)
    {
        return textureAssetGuid == rhs.textureAssetGuid
            && resourceBindPoint == rhs.resourceBindPoint
            && width == rhs.width
            && height == rhs.height;
    }

    bool operator==(const MaterialTexturePreviewData_t* rhs)
    {
        return textureAssetGuid == rhs->textureAssetGuid
            && resourceBindPoint == rhs->resourceBindPoint
            && width == rhs->width
            && height == rhs->height;
    }
};

struct MatTexCompare_t
{
    const ImGuiTableSortSpecs* sortSpecs;

    bool operator() (const MaterialTexturePreviewData_t& a, const MaterialTexturePreviewData_t& b) const
    {

        for (int n = 0; n < sortSpecs->SpecsCount; n++)
        {
            // Here we identify columns using the ColumnUserID value that we ourselves passed to TableSetupColumn()
            // We could also choose to identify columns based on their index (sort_spec->ColumnIndex), which is simpler!
            const ImGuiTableColumnSortSpecs* sort_spec = &sortSpecs->Specs[n];
            __int64 delta = 0;
            switch (sort_spec->ColumnUserID)
            {
            case MaterialTexturePreviewData_t::eColumnID::MTPC_ResBindPoint:      delta = (static_cast<__int64>(a.resourceBindPoint) - b.resourceBindPoint);          break;
            case MaterialTexturePreviewData_t::eColumnID::MTPC_TextureGUID:       delta = (a.textureAssetGuid - b.textureAssetGuid);                                  break;
            case MaterialTexturePreviewData_t::eColumnID::MTPC_TextureName:       delta = (strcmp(a.textureName.c_str(), b.textureName.c_str()));                     break;
            case MaterialTexturePreviewData_t::eColumnID::MTPC_ResBindingName:    delta = (strcmp(a.resourceBindingName.c_str(), b.resourceBindingName.c_str()));     break;
            default: IM_ASSERT(0); break;
            }
            if (delta > 0)
                return (sort_spec->SortDirection == ImGuiSortDirection_Ascending) ? false : true;
            if (delta < 0)
                return (sort_spec->SortDirection == ImGuiSortDirection_Ascending) ? true : false;
        }

        return (a.resourceBindPoint - b.resourceBindPoint) > 0;
    }
};

#define ITEMID(str, num) (str "##" + std::to_string(num)).c_str()
void MatPreview_DXState(const MaterialDXState_t& dxState, const uint8_t dxStateId, const uint8_t numRenderTargets)
{

    ImGui::SeparatorText(std::to_string(dxStateId).c_str());
    UNUSED(dxState);
    UNUSED(numRenderTargets);
    if (ImGui::TreeNodeEx(ITEMID("Blend States", dxStateId), ImGuiTreeNodeFlags_SpanAvailWidth))
    {
        for (int i = 0; i < numRenderTargets; ++i)
        {
            if(ImGui::TreeNodeEx(std::format("{}##dxs_{}", i, dxStateId).c_str(), ImGuiTreeNodeFlags_SpanAvailWidth))
            {
                ImGuiConstIntInputLeft("Raw Value", *reinterpret_cast<const int*>(&dxState.blendStates[i]), 170, ImGuiInputTextFlags_CharsHexadecimal);
                ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 10.f);

                ImGuiConstTextInputLeft("blendEnable", dxState.blendStates[i].blendEnable == 1 ? "true" : "false");
                ImGuiConstTextInputLeft("srcBlend", D3D11_BLEND_NAMES[dxState.blendStates[i].srcBlend + 1]);
                ImGuiConstTextInputLeft("destBlend", D3D11_BLEND_NAMES[dxState.blendStates[i].destBlend + 1]);

                ImGuiConstTextInputLeft("blendOp", D3D11_BLEND_OP_NAMES[dxState.blendStates[i].blendOp + 1]);

                ImGuiConstTextInputLeft("srcBlendAlpha", D3D11_BLEND_NAMES[dxState.blendStates[i].srcBlendAlpha + 1]);
                ImGuiConstTextInputLeft("destBlendAlpha", D3D11_BLEND_NAMES[dxState.blendStates[i].destBlendAlpha + 1]);

                ImGuiConstTextInputLeft("blendOpAlpha", D3D11_BLEND_OP_NAMES[dxState.blendStates[i].blendOpAlpha + 1]);

                ImGuiConstIntInputLeft("renderTargetWriteMask", dxState.blendStates[i].renderTargetWriteMask & 0xF, 170, ImGuiInputTextFlags_CharsHexadecimal);

                ImGui::TreePop();
            }
        }
        ImGui::TreePop();
    }
    ImGui::Text("Depth Stencil Flags: %u", dxState.depthStencilFlags);
    ImGui::Text("Rasterizer Flags:    %u", dxState.rasterizerFlags);
}

void* PreviewMaterialAsset(CAsset* const asset, const bool firstFrameForAsset)
{
    CPakAsset* pakAsset = static_cast<CPakAsset*>(asset);

    const MaterialAsset* const materialAsset = reinterpret_cast<MaterialAsset*>(pakAsset->extraData());
    assertm(materialAsset, "Extra data should be valid at this point.");

    static float textureZoom = 1.f;
    static MaterialTexturePreviewData_t selectedResource = { .resourceBindPoint = -1 };
    static CPakAsset* selectedTexture = nullptr;
    static bool firstFrameForTxtrAsset = false;
    static std::vector<MaterialTexturePreviewData_t> previewTextures;

    static int lastSelectedResourceBindPoint = -1;

    if (firstFrameForTxtrAsset)
        firstFrameForTxtrAsset = false;

    if (firstFrameForAsset)
    {
        selectedResource = { .resourceBindPoint = -1 }; // [rika]: just set to the base one, don't want the last mip like textures.
        //selectedTexture.reset();
        selectedTexture = NULL;
        previewTextures.clear();

        for (int idx = 0; idx < materialAsset->txtrAssets.size(); idx++)
        {
            const TextureAssetEntry_t& entry = materialAsset->txtrAssets.at(idx);

            MaterialTexturePreviewData_t previewTextureData = {};
            previewTextureData.materialTextureIndex = idx;
            previewTextureData.resourceBindPoint = entry.index;
            previewTextureData.textureAssetGuid = reinterpret_cast<const uint64_t*>(materialAsset->textureHandles)[entry.index];
            previewTextureData.width = -1;
            previewTextureData.height = -1;

            int flags = 0;
            if (!entry.asset) // asset not loaded
                flags |= MaterialTexturePreviewData_t::eTextureStateFlags::TEXSF_UNLOADED;

            if (entry.asset)
            {
                const TextureAsset* const textureAssetData = reinterpret_cast<const TextureAsset*>(entry.asset->extraData());

                previewTextureData.textureName = std::filesystem::path(entry.asset->GetAssetName()).filename().string();
                previewTextureData.width = textureAssetData->width;
                previewTextureData.height = textureAssetData->height;
            }
            else
                previewTextureData.textureName = std::format("0x{:X}", previewTextureData.textureAssetGuid);

            if (materialAsset->resourceBindings.count(entry.index))
                previewTextureData.resourceBindingName = materialAsset->resourceBindings.at(entry.index).name;
            else
            {
                flags |= MaterialTexturePreviewData_t::eTextureStateFlags::TEXSF_RES_UNKNOWN;

                previewTextureData.resourceBindingName = "[unknown]";
            }

            previewTextureData.stateFlags = flags;

            previewTextures.push_back(previewTextureData);
        }
    }
    
    constexpr int NUM_MATERIAL_TEXTURE_TABLE_COLUMNS = MaterialTexturePreviewData_t::eColumnID::_MTPC_COUNT;

    constexpr ImGuiTableFlags tableFlags =
        ImGuiTableFlags_Resizable | ImGuiTableFlags_Reorderable | ImGuiTableFlags_Hideable
        | ImGuiTableFlags_Sortable | ImGuiTableFlags_SortMulti
        | ImGuiTableFlags_RowBg | ImGuiTableFlags_Borders | ImGuiTableFlags_NoBordersInBody
        | ImGuiTableFlags_ScrollY | ImGuiTableFlags_SizingFixedFit;

    const ImVec2 outerSize = ImVec2(0.f, ImGui::GetTextLineHeightWithSpacing() * 12.f);

    //ImGui::TextUnformatted(std::format("Material: {} (0x{:X})", materialAsset->name, materialAsset->guid).c_str());
    ImGui::Text("Type: %s", s_MaterialShaderTypeNames[materialAsset->materialType]);

    if (materialAsset->materialType != MaterialShaderType_t::_TYPE_LEGACY)
    {
        ImGui::SameLine();
        g_pImGuiHandler->HelpMarker(s_MaterialShaderTypeHelpText);
    }

    ImGui::Text("Shaderset: %s (0x%llx)", materialAsset->shaderSetAsset ? materialAsset->shaderSetAsset->GetAssetName().c_str() : "unloaded", materialAsset->shaderSet);

    // [rika]: depth materials here

    if (ImGui::BeginTabBar("##MaterialTabs"))
    {
        if (ImGui::BeginTabItem("Textures"))
        {
            if (ImGui::BeginChild("##PropertiesTab", ImVec2(0, 0), false, ImGuiWindowFlags_NoBackground))
            {
                ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 5.f);
                if (ImGui::BeginTable("Material Table", NUM_MATERIAL_TEXTURE_TABLE_COLUMNS, tableFlags, outerSize))
                {
                    ImGui::TableSetupColumn("IDX", ImGuiTableColumnFlags_DefaultSort | ImGuiTableColumnFlags_NoResize | ImGuiTableColumnFlags_WidthFixed | ImGuiTableColumnFlags_NoHide, 0.f, MaterialTexturePreviewData_t::eColumnID::MTPC_ResBindPoint);
                    ImGui::TableSetupColumn("GUID", ImGuiTableColumnFlags_NoResize | ImGuiTableColumnFlags_WidthFixed | ImGuiTableColumnFlags_DefaultHide, 0.f, MaterialTexturePreviewData_t::eColumnID::MTPC_TextureGUID);
                    ImGui::TableSetupColumn("Name", ImGuiTableColumnFlags_NoResize | ImGuiTableColumnFlags_WidthFixed, 0.f, MaterialTexturePreviewData_t::eColumnID::MTPC_TextureName);
                    ImGui::TableSetupColumn("Type", ImGuiTableColumnFlags_NoResize | ImGuiTableColumnFlags_WidthFixed, 0.f, MaterialTexturePreviewData_t::eColumnID::MTPC_ResBindingName);
                    ImGui::TableSetupColumn("Dimensions", ImGuiTableColumnFlags_NoResize | ImGuiTableColumnFlags_WidthFixed | ImGuiTableColumnFlags_NoSort, 0.f, MaterialTexturePreviewData_t::eColumnID::MTPC_Dimensions);
                    ImGui::TableSetupColumn("Status", ImGuiTableColumnFlags_NoResize | ImGuiTableColumnFlags_WidthFixed | ImGuiTableColumnFlags_NoSort, 0.f, MaterialTexturePreviewData_t::eColumnID::MTPC_Status);
                    ImGui::TableSetupScrollFreeze(1, 1);

                    ImGuiTableSortSpecs* sortSpecs = ImGui::TableGetSortSpecs();

                    if (sortSpecs && (firstFrameForAsset || sortSpecs->SpecsDirty) && previewTextures.size() > 1)
                    {
                        std::sort(previewTextures.begin(), previewTextures.end(), MatTexCompare_t(sortSpecs));
                        sortSpecs->SpecsDirty = false;
                    }

                    ImGui::TableHeadersRow();

                    for (size_t i = 0; i < previewTextures.size(); ++i)
                    {
                        const MaterialTexturePreviewData_t* const item = &previewTextures[i];

                        // if row is last selected, or if we have just swapped asset and this texture has the same bind point as the last selected asset
                        const bool isRowSelected = selectedResource == item || (firstFrameForAsset && item->resourceBindPoint == lastSelectedResourceBindPoint);

                        ImGui::PushID(item->resourceBindPoint);

                        ImGui::TableNextRow(ImGuiTableRowFlags_None, 0.f);

                        ImGui::TableSetColumnIndex(MaterialTexturePreviewData_t::eColumnID::MTPC_ResBindPoint);
                        {
                            if (ImGui::Selectable(std::to_string(item->resourceBindPoint).c_str(), isRowSelected, ImGuiSelectableFlags_SpanAllColumns | ImGuiSelectableFlags_AllowItemOverlap, ImVec2(0.f, 0.f))
                                || (firstFrameForAsset && isRowSelected))
                            {
                                selectedResource = *item;
                                lastSelectedResourceBindPoint = item->resourceBindPoint;

                                if (item->IsTextureLoaded())
                                {
                                    const TextureAssetEntry_t* const textureEntry = &materialAsset->txtrAssets.at(item->materialTextureIndex);
                                    CPakAsset* const texturePakAsset = textureEntry->asset;

                                    const TextureAsset* const textureAssetData = reinterpret_cast<const TextureAsset*>(texturePakAsset->extraData());

                                    if (textureAssetData)
                                    {
                                        selectedTexture = texturePakAsset;// CreateTextureFromMip(texturePakAsset, &textureAssetData->mipArray.back(), s_PakToDxgiFormat[textureAssetData->imgFormat]);
                                        firstFrameForTxtrAsset = true;
                                    }
                                    else
                                        selectedTexture = NULL;
                                    //  selectedTexture.reset();
                                }
                                else
                                {
                                    //selectedTexture.reset();
                                    selectedTexture = NULL;
                                }
                            }
                        }

                        //ImGui::PushFont(g_pImGuiHandler->GetMonospaceFont());
                        if (ImGui::TableSetColumnIndex(MaterialTexturePreviewData_t::eColumnID::MTPC_TextureGUID))
                            ImGui::TextUnformatted(std::format("0x{:X}", item->textureAssetGuid).c_str());

                        if (ImGui::TableSetColumnIndex(MaterialTexturePreviewData_t::eColumnID::MTPC_TextureName))
                            ImGui::TextUnformatted(item->textureName.c_str());
                        //ImGui::PopFont();

                        if (ImGui::TableSetColumnIndex(MaterialTexturePreviewData_t::eColumnID::MTPC_ResBindingName))
                        {
                            ImGui::TextUnformatted(item->resourceBindingName.c_str());

                            if (!item->HasResourceType())
                            {
                                ImGui::SameLine();
                                g_pImGuiHandler->HelpMarker("The material asset does not provide any resource type information for this texture entry");
                            }
                        }

                        if (ImGui::TableSetColumnIndex(MaterialTexturePreviewData_t::eColumnID::MTPC_Dimensions))
                        {
                            if (item->width > 0 && item->height > 0)
                                ImGui::Text("%i x %i", item->width, item->height);
                            else
                                ImGui::TextUnformatted("N/A");
                        }

                        if (ImGui::TableSetColumnIndex(MaterialTexturePreviewData_t::eColumnID::MTPC_Status))
                        {
                            if (item->IsTextureLoaded())
                            {
                                ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.f, 1.f, 0.f, 1.f));
                                ImGui::TextUnformatted("Loaded");
                                ImGui::PopStyleColor();
                            }
                            else
                            {
                                ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.f, 0.f, 0.f, 1.f));
                                ImGui::TextUnformatted("Not Loaded");
                                ImGui::PopStyleColor();
                            }
                        }

                        ImGui::PopID();
                    }

                    ImGui::EndTable();
                }

                if (selectedTexture)
                {
                    ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 5.f);
                    ImGui::Separator();
                    ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 5.f);

                    auto txtrBinding = g_assetData.m_assetTypeBindings.find('rtxt');
                    if (txtrBinding != g_assetData.m_assetTypeBindings.end()) LIKELY
                    {
                        txtrBinding->second.previewFunc(selectedTexture, firstFrameForTxtrAsset);
                    }
                }
            }
            ImGui::EndChild();

            ImGui::EndTabItem();
        }

        if (ImGui::BeginTabItem("Properties"))
        {
            if (ImGui::BeginChild("##PropertiesTab", ImVec2(0,0), false, ImGuiWindowFlags_NoBackground))
            {
                if (ImGui::TreeNode("DX States"))
                {
                    MatPreview_DXState(materialAsset->dxStates[0], 0, materialAsset->numRenderTargets);
                    MatPreview_DXState(materialAsset->dxStates[1], 1, materialAsset->numRenderTargets);

                    ImGui::TreePop();
                }
            }
            ImGui::EndChild();

            ImGui::EndTabItem();
        }

        ImGui::EndTabBar();
    }

    return nullptr;
}

enum eMaterialExportSetting
{
    MATL_NONE,  // Just textures
    MATL_UBER_R, // Uber (Raw)
    MATL_UBER_S, // Uber (Struct)
};

bool ExportRawMaterialAsset(const MaterialAsset* const materialAsset, std::filesystem::path& exportPath)
{
    exportPath.replace_extension(".uber");

    StreamIO out(exportPath.string(), eStreamIOMode::Write);
    out.write(reinterpret_cast<char*>(materialAsset->cpuData), materialAsset->cpuDataSize);

    return true;
}

// [rika]: this sucks but I don't wanna redo rn (so it is yoinked from begion)
bool ExportStructMaterialAsset(const MaterialAsset* const materialAsset, std::filesystem::path& exportPath)
{
    exportPath.replace_extension(".uber_struct");

    std::ostringstream ss;
    ss << "struct CBufUberStatic\n{\n";

    char* ptr = reinterpret_cast<char*>(materialAsset->cpuData);
    for (auto& it : materialAsset->cpuDataBuf)
    {
        ss << "\t";

        switch (it.type)
        {
        case D3D_SVT_INT:
        {
            std::string str = std::format("int {} = {};", it.name, *reinterpret_cast<uint32_t*>(ptr));
            ss << str.c_str();
            break;
        }
        case D3D_SVT_UINT:
        {
            std::string str = std::format("uint32_t {} = {};", it.name, *reinterpret_cast<uint32_t*>(ptr));
            ss << str.c_str();
            break;
        }
        case D3D_SVT_FLOAT:
        {
            int elementCount = it.size / sizeof(float);

            std::string str = "";
            switch (elementCount)
            {
            case 1:
            {
                str = std::format("float {} = {};", it.name, *reinterpret_cast<float*>(ptr));
                break;
            }
            default:
            {
                std::string valStr = "";
                for (int i = 0; i < elementCount; ++i)
                {
                    valStr += std::format("{}", *reinterpret_cast<float*>(ptr + (i * sizeof(float))));

                    if (i != elementCount - 1)
                        valStr += ", ";
                }

                str = std::format("float {}[{}] = {{ {} }};", it.name, elementCount, valStr.c_str());
                break;
            }
            }
            ss << str.c_str();
            break;
        }
        default:
            std::string str = std::format("char UNIMPLEMENTED_{}[{}];", it.name, it.size);
            ss << str.c_str();
            break;
        }

        ptr += it.size;

        ss << "\n";
    };
    ss << "};";

    StreamIO out(exportPath.string(), eStreamIOMode::Write);
    out.write(ss.str().c_str(), ss.str().length());

    return true;
}

static bool removeSuffix(std::string& str, const char* const suffix)
{
    const size_t stringLen = str.length();
    const size_t suffixLen = strlen(suffix);

    if (stringLen > suffixLen &&
        str.compare(stringLen - suffixLen, suffixLen, suffix) == 0)
    {
        str.erase(stringLen - suffixLen);
        return true;
    }

    return false;
}

// [amos]: this might cause false positives if assets are actually
// prefixed with _shadow for example, without being a shadow pass material.
// should be very rare however.
static void RemoveRenderPassSuffix(std::string& textureName)
{
    if (removeSuffix(textureName, "_shadow"))
        return;
    if (removeSuffix(textureName, "_prepass"))
        return;
    if (removeSuffix(textureName, "_vsm"))
        return;
    if (removeSuffix(textureName, "_shadow_tight"))
        return;
    if (removeSuffix(textureName, "_colpass"))
        return;
}

std::unordered_map<uint8_t, MaterialTextureExportInfo_s> ExportMaterialTextures(const int setting, const MaterialAsset* materialAsset, const std::filesystem::path& exportPath, const bool semanticNames)
{
    std::filesystem::path exportPathCop = exportPath;
    std::filesystem::path matlPath(materialAsset->name);

    std::unordered_map<uint8_t, MaterialTextureExportInfo_s> textureNames;

    uint8_t nextSlotIdx = 0;
    for (auto& entry : materialAsset->txtrAssets)
    {
        nextSlotIdx++;

        CPakAsset* const txtrAsset = entry.asset;
        MaterialTextureExportInfo_s textureExportInfo;

        if (!txtrAsset)
        {
            textureNames.emplace(static_cast<uint8_t>(nextSlotIdx - 1), textureExportInfo);
            continue;
        }

        textureExportInfo.pTexture = txtrAsset;
        TextureAsset* thisTexture = reinterpret_cast<TextureAsset*>(txtrAsset->extraData());

        if (semanticNames)
        {
            if (!thisTexture->name)
            {
                std::string materialNameStem = matlPath.stem().string();
                RemoveRenderPassSuffix(materialNameStem);

                if (materialAsset->resourceBindings.count(entry.index))
                    textureExportInfo.exportName = std::format("{}_{}", materialNameStem, materialAsset->resourceBindings.at(entry.index).name);
                else if (thisTexture->type != eTextureType::_UNUSED)
                    textureExportInfo.exportName = std::format("{}{}", materialNameStem, GetTextureSuffixByType(thisTexture->type));
                else
                    textureExportInfo.exportName = std::format("{}_{}", materialNameStem, entry.index);
            }
            else
            {
                textureExportInfo.exportName = keepAfterLastSlashOrBackslash(thisTexture->name);
            }

            textureNames.emplace(static_cast<uint8_t>(nextSlotIdx - 1), textureExportInfo);
        }
        else
        {
            textureExportInfo.exportName = std::format("0x{:X}", thisTexture->guid);


            textureNames.emplace(static_cast<uint8_t>(nextSlotIdx - 1), textureExportInfo);
        }

        exportPathCop.append(textureExportInfo.exportName);

        // check if this texture is a normal
        const DXGI_FORMAT fmt = s_PakToDxgiFormat[thisTexture->imgFormat];
        bool txtrIsNormal = false;

        if (fmt == DXGI_FORMAT::DXGI_FORMAT_BC5_TYPELESS || fmt == DXGI_FORMAT::DXGI_FORMAT_BC5_UNORM || fmt == DXGI_FORMAT::DXGI_FORMAT_BC5_SNORM)
        {
            if (textureExportInfo.exportName.find("normalTexture") != std::string::npos)
                txtrIsNormal = true;
        }

        switch (setting)
        {
        case eTextureExportSetting::PNG_HM:
        case eTextureExportSetting::PNG_AM:
        {
            ExportPngTextureAsset(txtrAsset, thisTexture, exportPathCop, setting, txtrIsNormal);
            break;
        }
        case eTextureExportSetting::DDS_HM:
        case eTextureExportSetting::DDS_AM:
        case eTextureExportSetting::DDS_MM:
        {
            ExportDdsTextureAsset(txtrAsset, thisTexture, exportPathCop, setting, txtrIsNormal);
            break;
        }
        default:
            assertm(false, "Export setting is not handled.");
            break;
        }

        exportPathCop = exportPath;
    }

    return textureNames;
}

// [amos]: when the rson parser is finished, we should make both rsx and repak use rson.
bool ExportMaterialStruct(const MaterialAsset* const materialAsset, 
    std::filesystem::path& exportPath, const std::filesystem::path& texturePath, 
    const std::unordered_map<uint8_t, MaterialTextureExportInfo_s>& textureInfo, const bool useSemanticNames)
{
    if (materialAsset->materialType == _TYPE_LEGACY)
        return false;

    exportPath.replace_extension(".json");
    std::ofstream ofs(exportPath, std::ios::out);

    ofs << "{\n";

    ofs << "\t\"name\": \"" << materialAsset->name << "\",\n";

    ofs << "\t\"width\": " << std::dec << materialAsset->width << ",\n";
    ofs << "\t\"height\": " << std::dec << materialAsset->height << ",\n";
    ofs << "\t\"depth\": " << std::dec << materialAsset->depth << ",\n";

    ofs << "\t\"glueFlags\": \"0x" << std::hex << materialAsset->glueFlags << "\",\n";
    ofs << "\t\"glueFlags2\": \"0x" << std::hex << materialAsset->glueFlags2 << "\",\n";

    ofs << "\t\"blendStates\": [\n";

    for (size_t i = 0, size = ARRAYSIZE(materialAsset->dxStates[0].blendStates); i < size; i++)
    {
        const char* const commaChar = i != (size-1) ? "," : "";
        ofs << "\t\t\"0x" << std::uppercase << std::hex << materialAsset->dxStates[0].blendStates[i].Get() << "\"" << commaChar << "\n";
    }

    ofs << "\t],\n";

    ofs << "\t\"blendStateMask\": \"0x"  << std::uppercase << std::hex << materialAsset->dxStates[0].blendStateMask << "\",\n";
    ofs << "\t\"depthStencilFlags\": \"0x" << std::uppercase << std::hex << materialAsset->dxStates[0].depthStencilFlags << "\",\n";
    ofs << "\t\"rasterizerFlags\": \"0x" << std::uppercase << std::hex << materialAsset->dxStates[0].rasterizerFlags << "\",\n";
    ofs << "\t\"uberBufferFlags\": \"0x" << std::uppercase << std::hex << (uint32_t)materialAsset->uberBufferFlags << "\",\n";
    ofs << "\t\"features\": \"0x" << std::uppercase << std::hex << materialAsset->unk << "\",\n";
    ofs << "\t\"samplers\": \"0x" << std::uppercase << std::hex << *(uint32_t*)materialAsset->samplers << "\",\n";

    const char* const surfaceProp = materialAsset->surfaceProp ? materialAsset->surfaceProp : "";
    const char* const surfaceProp2 = materialAsset->surfaceProp2 ? materialAsset->surfaceProp2 : "";

    ofs << "\t\"surfaceProp\": \"" << surfaceProp << "\",\n";
    ofs << "\t\"surfaceProp2\": \"" << surfaceProp2 << "\",\n";

    ofs << "\t\"shaderType\": \"" << &s_MaterialShaderTypeSuffixes[materialAsset->materialType][1] << "\",\n"; // skip the underscore
    ofs << "\t\"shaderSet\": \"0x" << std::uppercase << std::hex << materialAsset->shaderSet << "\",\n";

    ofs << "\t\"$textures\": {\n";

    if (useSemanticNames && !textureInfo.empty())
    {
        const size_t size = textureInfo.size();
        size_t i = 0;

        for (auto& it : textureInfo)
        {
            std::string fullTexturePath = texturePath.string() + "/" + it.second.exportName + ".rpak";
            FixSlashes(fullTexturePath);

            const char* const commaChar = i != (size-1) ? "," : "";
            const TextureAssetEntry_t& tex = materialAsset->txtrAssets[i];

            ofs << "\t\t\"" << std::dec << static_cast<int>(tex.index) << "\": \"" << fullTexturePath << "\"" << commaChar << "\n";

            i++;
        }
    }
    else
    {
        for (size_t i = 0, size = materialAsset->txtrAssets.size(); i < size; i++)
        {
            const TextureAssetEntry_t& tex = materialAsset->txtrAssets[i];
            const char* const commaChar = i != (size - 1) ? "," : "";

            ofs << "\t\t\"" << std::dec << tex.index << "\": \"0x" << std::hex << reinterpret_cast<const uint64_t*>(materialAsset->textureHandles)[tex.index] << "\"" << commaChar << "\n";
        }
    }

    ofs << "\t},\n";
    ofs << "\t\"$textureTypes\": {\n";

    // Write out the texture types, so external tools can remap textures to other shaders.
    for (size_t i = 0, size = materialAsset->txtrAssets.size(); i < size; i++)
    {
        const TextureAssetEntry_t& tex = materialAsset->txtrAssets[i];
        const char* const commaChar = i != (size - 1) ? "," : "";

        ofs << "\t\t\"" << std::dec << tex.index << "\": \"";

        const char* toPrint = nullptr;

        if (materialAsset->resourceBindings.count(tex.index))
            toPrint = materialAsset->resourceBindings.at(tex.index).name;
        else if (!textureInfo.empty())
        {
            const MaterialTextureExportInfo_s& info = textureInfo.at((uint8_t)i);

            if (info.pTexture)
            {
                const TextureAsset* const textureAsset = reinterpret_cast<const TextureAsset*>(info.pTexture->extraData());

                if (textureAsset->type != eTextureType::_UNUSED)
                    toPrint = s_TextureTypeMap.at(textureAsset->type);
            }
        }

        ofs << (toPrint ? toPrint : "unavailable");
        ofs << "\"" << commaChar << "\n";
    }

    ofs << "\t},\n";

    ofs << "\t\"$depthShadowMaterial\": \"0x" << std::hex << materialAsset->depthShadowMaterial << "\",\n";
    ofs << "\t\"$depthPrepassMaterial\": \"0x" << std::hex << materialAsset->depthPrepassMaterial << "\",\n";
    ofs << "\t\"$depthVSMMaterial\": \"0x" << std::hex << materialAsset->depthVSMMaterial << "\",\n";
    ofs << "\t\"$depthShadowTightMaterial\": \"0x" << std::hex << materialAsset->depthShadowTightMaterial << "\",\n";
    ofs << "\t\"$colpassMaterial\": \"0x" << std::hex << materialAsset->colpassMaterial << "\",\n";
    ofs << "\t\"$textureAnimation\": \"0x" << std::hex << materialAsset->textureAnimation << "\"\n";

    ofs << "}\n";

    return true;
}

static std::filesystem::path ChangeFirstDirectory(const std::filesystem::path& originalPath, const std::string& newFirstDir)
{
    if (originalPath.empty())
        return originalPath;

    std::filesystem::path newPath;
    newPath /= newFirstDir;

    std::filesystem::path::iterator iter = originalPath.begin();

    for (++iter; iter != originalPath.end(); ++iter)
        newPath /= *iter;

    return newPath;
}

static const char* const s_PathPrefixMATL = s_AssetTypePaths.find(AssetType_t::MATL)->second;
bool ExportMaterialAsset(CAsset* const asset, const int setting)
{
    CPakAsset* pakAsset = static_cast<CPakAsset*>(asset);

    const MaterialAsset* const materialAsset = reinterpret_cast<MaterialAsset*>(pakAsset->extraData());
    assertm(materialAsset, "Extra data should be valid at this point.");

    const std::filesystem::path materialPath(asset->GetAssetName());
    const std::filesystem::path texturePath = ChangeFirstDirectory(materialPath.parent_path(), "texture");

    // Create exported path + asset path.
    std::filesystem::path exportPath = /*std::filesystem::current_path().append(*/EXPORT_DIRECTORY_NAME/*)*/; // 

    // truncate paths?
    if (g_ExportSettings.exportPathsFull)
        exportPath.append(materialPath.parent_path().string());
    else
    {
        exportPath.append(s_PathPrefixMATL);
        exportPath.append(materialPath.stem().string());
    }

    if (!CreateDirectories(exportPath))
    {
        assertm(false, "Failed to create asset's directory.");
        return false;
    }

    // get texture binding if it exists
    auto txtrAssetBinding = g_assetData.m_assetTypeBindings.find('rtxt');

    std::unordered_map<uint8_t, MaterialTextureExportInfo_s> textureNames;

    // only parse textures if one has actually been loaded
    if (g_ExportSettings.exportTxtrWithMat && txtrAssetBinding != g_assetData.m_assetTypeBindings.end() && materialAsset->txtrAssets.size())
    {
        std::filesystem::path textureExportPath;
        // textures should be exported to 'texture/' instead
        if (g_ExportSettings.exportPathsFull)
        {
            textureExportPath = /*std::filesystem::current_path().append(*/EXPORT_DIRECTORY_NAME/*)*/;
            textureExportPath /= g_ExportSettings.useSemanticTextureNames ? texturePath : "texture";

            if (!CreateDirectories(textureExportPath))
            {
                assertm(false, "Failed to create texture directory.");
                return false;
            }
        }
        else
            textureExportPath = exportPath;

        textureNames = ExportMaterialTextures(txtrAssetBinding->second.e.exportSetting, materialAsset, textureExportPath, g_ExportSettings.useSemanticTextureNames);
    }

    std::filesystem::path materialExportPath = exportPath;
    materialExportPath /= materialPath.stem();

    switch (setting)
    {
    case eMaterialExportSetting::MATL_NONE:
    {
        return true;
    }
    case eMaterialExportSetting::MATL_UBER_R:
    {
        // [amos]: when exporting cpu we typically also want the material itself so
        // it can be used directly in repak
        return ExportRawMaterialAsset(materialAsset, materialExportPath) && ExportMaterialStruct(materialAsset, materialExportPath, texturePath, textureNames, g_ExportSettings.useSemanticTextureNames);
    }
    case eMaterialExportSetting::MATL_UBER_S:
    {
        return ExportStructMaterialAsset(materialAsset, materialExportPath) && ExportMaterialStruct(materialAsset, materialExportPath, texturePath, textureNames, g_ExportSettings.useSemanticTextureNames);
    }
    default:
    {
        assertm(false, "Export setting is not handled.");
        return false;
    }
    }

    unreachable();
}

void InitMaterialAssetType()
{
    static const char* settings[] = { "Base (Textures)", "Uber (Raw)", "Uber (Struct)" };
    AssetTypeBinding_t type =
    {
        .type = 'ltam',
        .headerAlignment = 16,
        .loadFunc = LoadMaterialAsset,
        .postLoadFunc = PostLoadMaterialAsset,
        .previewFunc = PreviewMaterialAsset,
        .e = { ExportMaterialAsset, 0, settings, ARRSIZE(settings) },
    };

    REGISTER_TYPE(type);
}