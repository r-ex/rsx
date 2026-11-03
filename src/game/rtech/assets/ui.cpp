#include <pch.h>
#include <game/rtech/assets/ui.h>
#include <game/rtech/cpakfile.h>
#include <game/rtech/utils/utils.h>

#include <core/render/dx.h>
#include <thirdparty/imgui/imgui.h>
#include <thirdparty/imgui/misc/imgui_utility.h>
#include <thirdparty/imgui/misc/imgui_memory_editor.h>

extern RSXSettings_t g_rsxSettings;

void LoadUIAsset(CAssetContainer* const container, CAsset* const asset)
{
    UNUSED(container);

    CPakAsset* pakAsset = static_cast<CPakAsset*>(asset);

    UIAsset* uiAsset = nullptr;

    // [rika]: version 43 added in season 31, not sure the changes but does not affect us currently
    switch (pakAsset->version())
    {
    case 29: // r2
    case 30: // |
    case 39: // r5
    case 40: // |
    case 42: // |
    case 43: // |
    {
        UIAssetHeader_t* const hdr = reinterpret_cast<UIAssetHeader_t* const>(pakAsset->header());
        uiAsset = new UIAsset(hdr);
        break;
    }
    default:
    {
        assertm(false, "unaccounted asset version, will cause major issues!");
        return;
    }
    }

    if (uiAsset->name)
    {
        const std::string uiName = "ui/" + std::string(uiAsset->name) + ".rpak";
        pakAsset->SetAssetName(uiName, true);
    }
    else
        pakAsset->SetAssetNameFromCache();

    pakAsset->setExtraData(uiAsset);
}

struct UIPreviewData_t
{
    enum eColumnID
    {
        TPC_Offset,
        TPC_Index,
        TPC_Type,
        TPC_Name,
        TPC_DefaultVal,

        _TPC_COUNT,
    };

    const char* name;
    UIAssetArgValue_t value;

    const char* typeStr;

    int index;

    uint16_t hash;

    UIAssetArgType_t type;
    uint16_t offset;

    const bool operator==(const UIPreviewData_t& in)
    {
        return name == in.name && value.rawptr == in.value.rawptr && type == in.type;
    }

    const bool operator==(const UIPreviewData_t* in)
    {
        return name == in->name && value.rawptr == in->value.rawptr && type == in->type;
    }
};

struct UICompare_t
{
    const ImGuiTableSortSpecs* sortSpecs;

    bool operator() (const UIPreviewData_t& a, const UIPreviewData_t& b) const
    {

        for (int n = 0; n < sortSpecs->SpecsCount; n++)
        {
            // Here we identify columns using the ColumnUserID value that we ourselves passed to TableSetupColumn()
            // We could also choose to identify columns based on their index (sort_spec->ColumnIndex), which is simpler!
            const ImGuiTableColumnSortSpecs* sort_spec = &sortSpecs->Specs[n];
            __int64 delta = 0;
            switch (sort_spec->ColumnUserID)
            {
            case UIPreviewData_t::eColumnID::TPC_Index:     delta = (a.index - b.index);                                                break;
            case UIPreviewData_t::eColumnID::TPC_Type:      delta = (static_cast<uint8_t>(a.type) - static_cast<uint8_t>(b.type));      break;
            case UIPreviewData_t::eColumnID::TPC_Offset:    delta = (a.offset - b.offset);                                              break;
            }
            if (delta > 0)
                return (sort_spec->SortDirection == ImGuiSortDirection_Ascending) ? false : true;
            if (delta < 0)
                return (sort_spec->SortDirection == ImGuiSortDirection_Ascending) ? true : false;
        }

        return (a.index - b.index) > 0;
    }
};

// ImU32: 0xAABBGGRR
// Determine the background colour of the contents of the arg data hex editor
ImU32 UIArgData_BGColorCallback(const ImU8* /*mem*/, size_t off, void* user_data)
{
    std::vector<UIPreviewData_t>* const previewData = reinterpret_cast<std::vector<UIPreviewData_t>*>(user_data);

    for (auto& arg : *previewData)
    {
        if (arg.type == UI_ARG_TYPE_NONE)
            continue;

        if (arg.offset <= off && off < (arg.offset + s_typeSizes[arg.type]))
            return s_typeColours[arg.type];
    }

    return 0x00000000;
}

void* PreviewUIAsset(CAsset* const asset, const bool firstFrameForAsset)
{
    assertm(asset, "Asset should be valid.");

    CPakAsset* pakAsset = static_cast<CPakAsset*>(asset);

    //if (pakAsset->version() > 30)
    //{
    //    ImGui::Text("This asset version is not currently supported for preview.");
    //    return nullptr;
    //}

    UIAsset* const uiAsset = reinterpret_cast<UIAsset*>(pakAsset->extraData());

    static std::vector<UIPreviewData_t> previewData;

    if (firstFrameForAsset)
    {
        previewData.clear();
        previewData.resize(uiAsset->argCount);

        uiAsset->minArgDataOffset = 0xFFFF;

        for (int i = 0; i < uiAsset->argCount; i++)
        {
            UIPreviewData_t& argPreviewData = previewData.at(i);
            const UIAssetArg_t* const argData = &uiAsset->args[i];

            assertm(argData->dataOffset < uiAsset->argDefaultValueSize, "potentially invalid data");

            argPreviewData.index = i;
            argPreviewData.name = uiAsset->argNames ? uiAsset->argNames + argData->nameOffset : nullptr;
            argPreviewData.value.rawptr = (reinterpret_cast<char*>(uiAsset->argDefaultValues) + argData->dataOffset);
            argPreviewData.type = argData->type;
            argPreviewData.typeStr = s_UIArgTypeNames[argData->type];
            argPreviewData.offset = argData->dataOffset; // [rika]: dataOffset on 'none' type args is just 0
            argPreviewData.hash = argData->shortHash; // use if we have no name

            if (argData->type != UI_ARG_TYPE_NONE && argData->dataOffset < uiAsset->minArgDataOffset)
                uiAsset->minArgDataOffset = argData->dataOffset;
        }
    }

    ImGui::Text("Num Arg Clusters: %u", uiAsset->argClusterCount);
    ImGui::Text("Struct Size: %u", uiAsset->ruiDataStructSize);
    ImGui::Text("Constant Values Size: %u", uiAsset->argDefaultValueSize);

    if (uiAsset->argClusterCount == 1)
    {
        const UIAssetArgCluster_t* const ac = &uiAsset->argClusters[0];
        ImGui::Text("Hash Constants: MUL (0x%X), ADD (0x%X)", ac->hashMul, ac->hashAdd);
    }

    constexpr ImGuiTableFlags tableFlags =
        ImGuiTableFlags_Resizable | ImGuiTableFlags_Reorderable | ImGuiTableFlags_Hideable
        | ImGuiTableFlags_Sortable | ImGuiTableFlags_SortMulti
        | ImGuiTableFlags_RowBg | ImGuiTableFlags_Borders | ImGuiTableFlags_NoBordersInBody
        | ImGuiTableFlags_ScrollY | ImGuiTableFlags_SizingFixedFit;

    const ImVec2 outerSize = ImVec2(0.f, ImGui::GetTextLineHeightWithSpacing() * 12.f);

    if (ImGui::BeginTable("Arg Table", UIPreviewData_t::eColumnID::_TPC_COUNT, tableFlags, outerSize))
    {
        ImGui::TableSetupColumn("Offset", ImGuiTableColumnFlags_DefaultSort | ImGuiTableColumnFlags_NoResize | ImGuiTableColumnFlags_WidthFixed, 0.0f, UIPreviewData_t::eColumnID::TPC_Offset);
        ImGui::TableSetupColumn("Index", ImGuiTableColumnFlags_NoResize | ImGuiTableColumnFlags_WidthFixed | ImGuiTableColumnFlags_DefaultHide, 0.0f, UIPreviewData_t::eColumnID::TPC_Index);
        ImGui::TableSetupColumn("Type", ImGuiTableColumnFlags_NoResize | ImGuiTableColumnFlags_WidthFixed, 0.0f, UIPreviewData_t::eColumnID::TPC_Type);
        ImGui::TableSetupColumn("Name", ImGuiTableColumnFlags_NoResize | ImGuiTableColumnFlags_WidthFixed, 0.0f, UIPreviewData_t::eColumnID::TPC_Name);
        ImGui::TableSetupColumn("Value", ImGuiTableColumnFlags_NoResize | ImGuiTableColumnFlags_WidthFixed, 0.0f, UIPreviewData_t::eColumnID::TPC_DefaultVal);
        ImGui::TableSetupScrollFreeze(1, 1);

        ImGuiTableSortSpecs* sortSpecs = ImGui::TableGetSortSpecs(); // get the sorting settings from this table

        if (sortSpecs && (firstFrameForAsset || sortSpecs->SpecsDirty) && previewData.size() > 1)
        {
            std::sort(previewData.begin(), previewData.end(), UICompare_t(sortSpecs));
            sortSpecs->SpecsDirty = false;
        }

        ImGui::TableHeadersRow();

        for (int i = 0; i < uiAsset->argCount; i++)
        {
            const UIPreviewData_t* item = &previewData.at(i);

            if (item->type == UIAssetArgType_t::UI_ARG_TYPE_NONE)
                continue;

            ImGui::PushID(item->index); // id of this line ?

            ImGui::TableNextRow(ImGuiTableRowFlags_None, 0.0f);

            if (ImGui::TableSetColumnIndex(UIPreviewData_t::eColumnID::TPC_Offset))
                ImGui::Text("0x%X", item->offset);

            if (ImGui::TableSetColumnIndex(UIPreviewData_t::eColumnID::TPC_Index))
                ImGui::Text("%i", item->index);

            if (ImGui::TableSetColumnIndex(UIPreviewData_t::eColumnID::TPC_Type))
                ImGui::Text("%s", item->typeStr);

            if (ImGui::TableSetColumnIndex(UIPreviewData_t::eColumnID::TPC_Name))
            {
                if (item->name)
                    ImGui::Text("%s", item->name);
                else
                    ImGui::Text("%x", item->hash);
            }

            if (ImGui::TableSetColumnIndex(UIPreviewData_t::eColumnID::TPC_DefaultVal))
            {
                switch (item->type)
                {
                case UIAssetArgType_t::UI_ARG_TYPE_NONE:
                {

                    break;
                }
                case UIAssetArgType_t::UI_ARG_TYPE_STRING:
                {
                    ImGui::Text("\"%s\"", *item->value.string);

                    break;
                }
                case UIAssetArgType_t::UI_ARG_TYPE_ASSET:
                case UIAssetArgType_t::UI_ARG_TYPE_IMAGE:
                {
                    ImGui::Text("$\"%s\"", *item->value.string);

                    break;
                }
                case UIAssetArgType_t::UI_ARG_TYPE_UIHANDLE:
                {
                    if (pakAsset->version() > 30)
                        ImGui::Text("$\"ui:%X\"", *item->value.integer);
                    else
                        ImGui::Text("$\"%s\"", *item->value.string);
                    break;
                }
                case UIAssetArgType_t::UI_ARG_TYPE_BOOL:
                {
                    ImGui::Text("%s", *item->value.boolean ? "True" : "False");

                    break;
                }
                case UIAssetArgType_t::UI_ARG_TYPE_INT:
                {
                    ImGui::Text("%i", *item->value.integer);

                    break;
                }
                case UIAssetArgType_t::UI_ARG_TYPE_FLOAT:
                case UIAssetArgType_t::UI_ARG_TYPE_GAMETIME:
                {
                    // this is a very common placeholder value that otherwise gets displayed as -1000000015047466219876677755040.000000
                    if (*item->value.integer == 0xf149f2ca)
                        ImGui::Text("-1e30");
                    else
                        ImGui::Text("%f", *item->value.fpn);

                    break;
                }
                case UIAssetArgType_t::UI_ARG_TYPE_FLOAT2:
                {
                    const float* const floatArray = item->value.fpn;

                    ImGui::Text("%f, %f", floatArray[0], floatArray[1]);

                    break;
                }
                case UIAssetArgType_t::UI_ARG_TYPE_FLOAT3:
                {
                    const float* const floatArray = item->value.fpn;

                    ImGui::Text("%f, %f, %f", floatArray[0], floatArray[1], floatArray[2]);

                    break;
                }
                case UIAssetArgType_t::UI_ARG_TYPE_COLOR_ALPHA:
                {
                    const float* const floatArray = item->value.fpn;

                    ImGui::Text("%f, %f, %f, %f", floatArray[0], floatArray[1], floatArray[2], floatArray[3]);

                    break;
                }
                case UIAssetArgType_t::UI_ARG_TYPE_WALLTIME:
                {
                    ImGui::Text("%lli", *item->value.integer64);

                    break;
                }
                case UIAssetArgType_t::UI_ARG_TYPE_FONT_HASH:
                {
                    ImGui::Text("%X", *item->value.integer);

                    break;
                }
                // font face
                // array
                default:
                {
                    ImGui::Text("UNSUPPORTED");
                    assertm(false, "unsupported arg type!");
                    break;
                }
                }
            }

            ImGui::PopID(); // no longer working on this id

        }

        ImGui::EndTable();
    }

    if (ImGui::BeginChild("Raw Data", ImVec2(0, 0), ImGuiChildFlags_Borders))
    {
        static MemoryEditor rawDataEdit;

        rawDataEdit.ReadOnly = true;
        rawDataEdit.OptShowDataPreview = true;
        rawDataEdit.UserData = &previewData;
        rawDataEdit.BgColorFn = UIArgData_BGColorCallback;

        rawDataEdit.DrawContents(uiAsset->argDefaultValues, uiAsset->argDefaultValueSize);
    }
    ImGui::EndChild();

    return nullptr;
}

enum class eUIExportSetting : int
{
    TXT, // just the arguments in text form
    CPP, // erm
};

bool ExportUIAsset_CPP(CAsset* const asset, UIAsset* const uiAsset, std::filesystem::path& exportPath)
{
    UNUSED(asset);

    std::vector<UIPreviewData_t> exportData;
    exportData.clear();
    exportData.resize(uiAsset->argCount);

    uiAsset->minArgDataOffset = 0xFFFF;
    for (int i = 0; i < uiAsset->argCount; i++)
    {
        UIPreviewData_t& argPreviewData = exportData.at(i);
        const UIAssetArg_t* const argData = &uiAsset->args[i];

        assertm(argData->dataOffset < uiAsset->argDefaultValueSize, "potentially invalid data");

        argPreviewData.index = i;
        argPreviewData.name = uiAsset->argNames ? uiAsset->argNames + argData->nameOffset : nullptr;
        argPreviewData.value.rawptr = (reinterpret_cast<char*>(uiAsset->argDefaultValues) + argData->dataOffset);
        argPreviewData.type = argData->type;
        argPreviewData.typeStr = s_UIArgTypeNames[argData->type];
        argPreviewData.offset = argData->dataOffset; // [rika]: dataOffset on 'none' type args is just 0
        argPreviewData.hash = argData->shortHash; // use if we have no name

        if (argData->type != UI_ARG_TYPE_NONE && argData->dataOffset < uiAsset->minArgDataOffset)
            uiAsset->minArgDataOffset = argData->dataOffset;
    }

    std::sort(exportData.begin(), exportData.end(), [](UIPreviewData_t& a, UIPreviewData_t& b) {
        return a.offset < b.offset;
    });

    const std::string hashConstantString = std::format("{}/{}",
        uiAsset->argClusterCount == 1 ? uiAsset->argClusters->hashMul : -1,
        uiAsset->argClusterCount == 1 ? uiAsset->argClusters->hashAdd : -1);

    const std::string typeName = std::format("{}_s", uiAsset->name);

    std::stringstream rawtext;

    rawtext <<
        "#include <cassert>\n\n" <<
        (!uiAsset->argNames ? "// !!WARNING!! This UI asset does not contain variable names.\n// The members of the data structure are instead named using a unique hash value.\n\n" : "") <<

        "#ifndef SHADER_DATA\n"
        "#define SHADER_DATA(sz) char __shader_data[sz]; void* ShaderData(int ofs) { assert(ofs < sz); return reinterpret_cast<char*>(this) + ofs; };\n"
        "#endif\n\n"
        "#ifndef VAR_PAD\n"
        "#define CONCAT(a, b) XCONCAT(a, b)\n"
        "#define XCONCAT(a, b) a ## b\n"
        "#define VAR_PAD(n) char CONCAT(pad_, __COUNTER__)[n];\n"
        "#endif\n\n" <<

        (!uiAsset->argNames ? std::format("// Hash Constants: {}\n", hashConstantString) : "") <<
        "// Hash Table Size: " << uiAsset->argClusters->argCount << "\n"

        "struct " << typeName << "\n"
        "{\n";

    // If the minArgDataOffset is 65535, there are no args. If there are no args, the entire argDefaultValueSize amount is part of SHADER_DATA
    const uint32_t shaderDataSize = uiAsset->minArgDataOffset != 0xFFFF ? uiAsset->minArgDataOffset : uiAsset->argDefaultValueSize;

    if(uiAsset->minArgDataOffset > 0)
        rawtext << "\tSHADER_DATA(" << shaderDataSize << "); // The first " << shaderDataSize << " bytes of this struct are unannotated* constant values that are accessed by internal rendering code. (*no names, no type info)\n\n";

    size_t i = 0;
    for (auto& it : exportData)
    {
        if (it.type == UI_ARG_TYPE_NONE)
            continue;

        rawtext << "\t" << s_typeNamesCPP[it.type] << " " << (it.name ? it.name : std::format("var_{:x}_{}", it.hash, it.index)) << "; " << "// @ " << it.offset << "\n";

        //if (i != 0)
        //{
        //    auto& lastVar = exportData[i - 1];
        //    const int8_t uiHandleSize = asset->GetContainerFile<CPakFile>()->header()->version == 7 ? sizeof(const char*) : sizeof(uint32_t);
        //    const int8_t typeSize = lastVar.type == UI_ARG_TYPE_UIHANDLE ? uiHandleSize : s_typeSizes[lastVar.type];
        //    const int lastVarOffsetEnd = lastVar.offset + typeSize;
        //    
        //    if (lastVarOffsetEnd != it.offset)
        //        rawtext << "\tVAR_PAD(" << it.offset - lastVarOffsetEnd << ");\n";
        //}

        i++;
    }

    const int extraDataSize = uiAsset->ruiDataStructSize - uiAsset->argDefaultValueSize;
    rawtext << "\n"
        "\tchar unk_data[" << extraDataSize << "];\n\n"
        "\ttemplate<typename T>\n"
        "\tT* data(int offset)\n"
        "\t{\n"
        "\t\tassert(offset < sizeof(*this));\n"
        "\t\treturn reinterpret_cast<T*>((char*)this + offset);\n"
        "\t};\n"
        
        "};\n\n"
        "void " << uiAsset->name << "(UIScriptApi_s* api, UIConstantVars_s* consts, UIInstance_s* inst, " << typeName << "* data)\n"
        "{\n"
        "\t// Not implemented\n"
        "};\n\n";

    for (auto& it : exportData)
    {
        if (it.type == UI_ARG_TYPE_NONE)
            continue;

        rawtext << "static_assert(offsetof(" << typeName << ", " << (it.name ? it.name : std::format("var_{:x}_{}", it.hash, it.index)) << ") == " << it.offset << ");\n";
    }

    exportPath.replace_extension(".cpp");

    StreamIO out;
    if (!out.open(exportPath.string(), eStreamIOMode::Write))
    {
        assertm(false, "Failed to open file for write.");
        return false;
    }

    out.write(rawtext.str().c_str(), rawtext.str().length());
    out.close();

    return true;
}

static const char* const s_PathPrefixUI = s_AssetTypePaths.find(AssetType_t::UI)->second;
bool ExportUIAsset(CAsset* const asset, const int setting)
{
    CPakAsset* pakAsset = static_cast<CPakAsset*>(asset);
    UIAsset* const uiAsset = reinterpret_cast<UIAsset*>(pakAsset->extraData());

    // Create exported path + asset path.
    std::filesystem::path exportPath = g_rsxSettings.GetExportDirectory();
    const std::filesystem::path uiPath(asset->GetAssetName());

    if (g_rsxSettings.exportPathsFull)
        exportPath.append(uiPath.parent_path().string());
    else
        exportPath.append(s_PathPrefixUI);

    if (!CreateDirectories(exportPath))
    {
        assertm(false, "Failed to create asset type directory.");
        return false;
    }

    exportPath.append(uiPath.stem().string());

    switch ((eUIExportSetting)setting)
    {
    case eUIExportSetting::TXT:
    {
        std::stringstream rawtext;

        for (int i = 0; i < uiAsset->argClusterCount; ++i)
        {
            UIAssetArgCluster_t* ac = &uiAsset->argClusters[i];
            rawtext << std::format("// ArgCluster {} Hash Consts: {}/{}\n", i, ac->hashMul, ac->hashAdd);
        }

        uiAsset->minArgDataOffset = 0xFFFF;


        for (int i = 0; i < uiAsset->argCount; i++)
        {
            const UIAssetArg_t* const arg = &uiAsset->args[i];

            if (arg->type == UIAssetArgType_t::UI_ARG_TYPE_NONE)
                continue;

            if (arg->dataOffset < uiAsset->minArgDataOffset)
                uiAsset->minArgDataOffset = arg->dataOffset;

            rawtext << std::format("{} ", s_typeNamesRUI[arg->type]);

            if (uiAsset->argNames)
                rawtext << (uiAsset->argNames + arg->nameOffset);
            else
                rawtext << std::format("{:x}", arg->shortHash);

            rawtext << std::format("@{}\t", arg->dataOffset);

            UIAssetArgValue_t argValue = { .rawptr = (reinterpret_cast<char*>(uiAsset->argDefaultValues) + arg->dataOffset) };

            switch (arg->type)
            {
            case UIAssetArgType_t::UI_ARG_TYPE_NONE:
            {
                rawtext << "N/A";

                break;
            }
            case UIAssetArgType_t::UI_ARG_TYPE_STRING:
            {
                rawtext << std::format("\"{}\"", EscapeString(*argValue.string));

                break;
            }
            case UIAssetArgType_t::UI_ARG_TYPE_ASSET:
            case UIAssetArgType_t::UI_ARG_TYPE_IMAGE:
            {
                rawtext << std::format("$\"{}\"", *argValue.string);

                break;
            }
            case UIAssetArgType_t::UI_ARG_TYPE_UIHANDLE:
            {
                if (pakAsset->version() > 30)
                    rawtext << std::format("$\"ui:{:X}\"", *argValue.integer);
                else
                    rawtext << std::format("$\"{}\"", *argValue.string);

                break;
            }
            case UIAssetArgType_t::UI_ARG_TYPE_BOOL:
            {
                rawtext << (*argValue.boolean ? "true" : "false");

                break;
            }
            case UIAssetArgType_t::UI_ARG_TYPE_INT:
            {
                rawtext << std::format("{}", *argValue.integer);

                break;
            }
            case UIAssetArgType_t::UI_ARG_TYPE_FLOAT:
            case UIAssetArgType_t::UI_ARG_TYPE_GAMETIME: // no worky
            {
                rawtext << std::format("{}", *argValue.fpn);

                break;
            }
            case UIAssetArgType_t::UI_ARG_TYPE_FLOAT2:
            {
                rawtext << std::format("{}, {}", argValue.fpn[0], argValue.fpn[1]);

                break;
            }
            case UIAssetArgType_t::UI_ARG_TYPE_FLOAT3:
            {
                rawtext << std::format("{}, {}, {}", argValue.fpn[0], argValue.fpn[1], argValue.fpn[2]);

                break;
            }
            case UIAssetArgType_t::UI_ARG_TYPE_COLOR_ALPHA:
            {
                rawtext << std::format("{}, {}, {}, {}", argValue.fpn[0], argValue.fpn[1], argValue.fpn[2], argValue.fpn[3]);

                break;
            }
            case UIAssetArgType_t::UI_ARG_TYPE_WALLTIME:
            {
                rawtext << std::format("{}", *argValue.integer64);

                break;
            }
            // font face
            // font hash
            // array
            default:
            {
                rawtext << "UNSUPPORTED";

                break;
            }
            }

            rawtext << "\n";
        }

        rawtext << "// Shader Data Size: " << uiAsset->minArgDataOffset << "\n";

        exportPath.replace_extension(".txt");

        StreamIO out;
        if (!out.open(exportPath.string(), eStreamIOMode::Write))
        {
            assertm(false, "Failed to open file for write.");
            return false;
        }

        out.write(rawtext.str().c_str(), rawtext.str().length());
        out.close();

        return true;
    }
    case eUIExportSetting::CPP:
    {
        return ExportUIAsset_CPP(asset, uiAsset, exportPath);
    }
    default:
        break;
    }

    unreachable();
}

void InitUIAssetType()
{
    static const char* settings[] = { "TXT", "C++" };
    AssetTypeBinding_t type =
    {
        .name = "RUI",
        .type = '\0iu',
        .headerAlignment = 8,
        .loadFunc = LoadUIAsset,
        .postLoadFunc = nullptr,
        .previewFunc = PreviewUIAsset,
        .e = { ExportUIAsset, 0, settings, ARRAYSIZE(settings) },
    };

    REGISTER_TYPE(type);
}