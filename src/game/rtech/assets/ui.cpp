#include <pch.h>
#include <game/rtech/assets/ui.h>
#include <game/rtech/cpakfile.h>
#include <game/rtech/utils/utils.h>

#include <core/render/dx.h>
#include <thirdparty/imgui/imgui.h>
#include <thirdparty/imgui/misc/imgui_utility.h>

extern ExportSettings_t g_ExportSettings;

void LoadUIAsset(CAssetContainer* const pak, CAsset* const asset)
{
    UNUSED(pak);

    CPakAsset* pakAsset = static_cast<CPakAsset*>(asset);

    UIAsset* uiAsset = nullptr;

    switch (pakAsset->version())
    {
    /*case 29:
    {
        assertm(false, "R2TT unsupported at this time");
        return;
    }*/
    case 29:
    case 30:
    {
        UIAssetHeader_t* const hdr = reinterpret_cast<UIAssetHeader_t* const>(pakAsset->header());
        uiAsset = new UIAsset(hdr);
        break;
    }
    default:
        return;
    }

    if (uiAsset->name)
    {
        const std::string uiName = "ui/" + std::string(uiAsset->name) + ".rpak";
        pakAsset->SetAssetName(uiName);
    }

    pakAsset->setExtraData(uiAsset);
}

struct UIPreviewData_t
{
    enum eColumnID
    {
        TPC_Index, // base should be -1
        TPC_Type,
        TPC_Name,
        TPC_DefaultVal,
        TPC_Offset,

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

void* PreviewUIAsset(CAsset* const asset, const bool firstFrameForAsset)
{
    assertm(asset, "Asset should be valid.");

    CPakAsset* pakAsset = static_cast<CPakAsset*>(asset);

    if (pakAsset->version() > 30)
    {
        ImGui::Text("This asset version is not currently supported for preview.");
        return nullptr;
    }

    UIAsset* const uiAsset = reinterpret_cast<UIAsset*>(pakAsset->extraData());
    assertm(uiAsset, "Extra data should be valid at this point.");


    static std::vector<UIPreviewData_t> previewData;

    if (firstFrameForAsset)
    {
        previewData.clear();
        previewData.resize(uiAsset->argCount);

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
            argPreviewData.offset = argData->type == UIAssetArgType_t::UI_ARG_TYPE_NONE? 0xFFFF :argData->dataOffset;
            argPreviewData.hash = argData->shortHash; // use if we have no name
        }
    }

    constexpr ImGuiTableFlags tableFlags =
        ImGuiTableFlags_Resizable | ImGuiTableFlags_Reorderable | ImGuiTableFlags_Hideable
        | ImGuiTableFlags_Sortable | ImGuiTableFlags_SortMulti
        | ImGuiTableFlags_RowBg | ImGuiTableFlags_Borders | ImGuiTableFlags_NoBordersInBody
        | ImGuiTableFlags_ScrollY | ImGuiTableFlags_SizingFixedFit;

    const ImVec2 outerSize = ImVec2(0.f, ImGui::GetTextLineHeightWithSpacing() * 12.f);

    if (ImGui::BeginTable("Arg Table", UIPreviewData_t::eColumnID::_TPC_COUNT, tableFlags, outerSize))
    {
        ImGui::TableSetupColumn("Index", ImGuiTableColumnFlags_DefaultSort | ImGuiTableColumnFlags_NoResize | ImGuiTableColumnFlags_WidthFixed | ImGuiTableColumnFlags_NoHide, 0.0f, UIPreviewData_t::eColumnID::TPC_Index);
        ImGui::TableSetupColumn("Type", ImGuiTableColumnFlags_NoResize | ImGuiTableColumnFlags_WidthFixed, 0.0f, UIPreviewData_t::eColumnID::TPC_Type);
        ImGui::TableSetupColumn("Name", ImGuiTableColumnFlags_NoResize | ImGuiTableColumnFlags_WidthFixed, 0.0f, UIPreviewData_t::eColumnID::TPC_Name);
        ImGui::TableSetupColumn("Value", ImGuiTableColumnFlags_NoResize | ImGuiTableColumnFlags_WidthFixed, 0.0f, UIPreviewData_t::eColumnID::TPC_DefaultVal);
        ImGui::TableSetupColumn("Offset", ImGuiTableColumnFlags_NoResize | ImGuiTableColumnFlags_WidthFixed, 0.0f, UIPreviewData_t::eColumnID::TPC_Offset);
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

            //const bool isRowSelected = selectedFontCharacter == item || (firstFrameForAsset && item->index == lastSelectedTexture);

            ImGui::PushID(item->index); // id of this line ?

            ImGui::TableNextRow(ImGuiTableRowFlags_None, 0.0f);

            if (ImGui::TableSetColumnIndex(UIPreviewData_t::eColumnID::TPC_Index))
                ImGui::Text("%i", item->index);

            if (ImGui::TableSetColumnIndex(UIPreviewData_t::eColumnID::TPC_Type))
                ImGui::Text("%s", item->typeStr);
            if(ImGui::TableSetColumnIndex(UIPreviewData_t::eColumnID::TPC_Offset))
                if(item->type == UIAssetArgType_t::UI_ARG_TYPE_NONE)
                    ImGui::Text("");
                else
                    ImGui::Text("0x%X",item->offset);
            if (ImGui::TableSetColumnIndex(UIPreviewData_t::eColumnID::TPC_Name))
            {
                if(item->type == UIAssetArgType_t::UI_ARG_TYPE_NONE)
                    ImGui::Text("");
                else if (item->name)
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
                case UIAssetArgType_t::UI_ARG_TYPE_UIHANDLE:
                {
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
                case UIAssetArgType_t::UI_ARG_TYPE_GAMETIME: // no worky
                {
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
                // font face
                // font hash
                // array
                default:
                {
                    ImGui::Text("UNSUPPORTED");

                    break;
                }
                }
            }

            ImGui::PopID(); // no longer working on this id

        }

        ImGui::EndTable();
    }

    return nullptr;
}

enum eUIExportSetting
{
    TXT, // just the arguments in text form
};

static const char* const s_PathPrefixUI = s_AssetTypePaths.find(AssetType_t::UI)->second;
bool ExportUIAsset(CAsset* const asset, const int setting)
{
    CPakAsset* pakAsset = static_cast<CPakAsset*>(asset);
    UIAsset* const uiAsset = reinterpret_cast<UIAsset*>(pakAsset->extraData());
    assertm(uiAsset, "Extra data should be valid at this point.");

    // Create exported path + asset path.
    std::filesystem::path exportPath = std::filesystem::current_path().append(EXPORT_DIRECTORY_NAME); // 
    const std::filesystem::path uiPath(asset->GetAssetName());

    // truncate paths?
    if (g_ExportSettings.exportPathsFull)
        exportPath.append(uiPath.parent_path().string());
    else
        exportPath.append(s_PathPrefixUI);

    if (!CreateDirectories(exportPath))
    {
        assertm(false, "Failed to create asset type directory.");
        return false;
    }

    exportPath.append(uiPath.stem().string());

    switch (setting)
    {
    case eUIExportSetting::TXT:
    {
        std::stringstream rawtext;

        for (int i = 0; i < uiAsset->argCount; i++)
        {
            const UIAssetArg_t* const arg = &uiAsset->args[i];

            if (uiAsset->argNames)
                rawtext << (uiAsset->argNames + arg->nameOffset);
            else
                rawtext << std::format("{:x}", arg->shortHash);

            rawtext << ":\t";

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
                rawtext << std::format("\"{}\"", *argValue.string);

                break;
            }
            case UIAssetArgType_t::UI_ARG_TYPE_ASSET:
            case UIAssetArgType_t::UI_ARG_TYPE_IMAGE:
            case UIAssetArgType_t::UI_ARG_TYPE_UIHANDLE:
            {
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
    default:
        break;
    }

    unreachable();
}

void InitUIAssetType()
{
    static const char* settings[] = { "TXT" };
    AssetTypeBinding_t type =
    {
        .type = '\0iu',
        .headerAlignment = 8,
        .loadFunc = LoadUIAsset,
        .postLoadFunc = nullptr,
        .previewFunc = PreviewUIAsset,
        .e = { ExportUIAsset, 0, settings, ARRAYSIZE(settings) },
    };

    REGISTER_TYPE(type);
}