#include <pch.h>
#include <game/rtech/assets/datatable.h>
#include <thirdparty/imgui/imgui.h>

void LoadDatatableAsset(CAssetContainer* const pak, CAsset* const asset)
{
    DatatableAsset* dtblAsset = nullptr;

    CPakAsset* pakAsset = static_cast<CPakAsset*>(asset);


    const eDTBLVersion version = GetDTBLVersion(pak, asset);

    switch (version)
    {
    case eDTBLVersion::VERSION_0:
    {
        const DatatableAssetHeader_v0_t* const hdr = reinterpret_cast<DatatableAssetHeader_v0_t*>(pakAsset->header());
        dtblAsset = new DatatableAsset(hdr, version);
        break;
    }
    case eDTBLVersion::VERSION_1:
    case eDTBLVersion::VERSION_1_1:
    {
        const DatatableAssetHeader_v1_t* const hdr = reinterpret_cast<DatatableAssetHeader_v1_t*>(pakAsset->header());
        dtblAsset = new DatatableAsset(hdr, version);
        break;
    }
    case eDTBLVersion::VERSION_UNK:
    default:
        return;
    }

    // cached names when
    pakAsset->setExtraData(dtblAsset);
}

void* PreviewDatatableAsset(CAsset* const asset, const bool firstFrameForAsset)
{
    UNUSED(firstFrameForAsset);

    CPakAsset* pakAsset = static_cast<CPakAsset*>(asset);

    const DatatableAsset* const dtblAsset = reinterpret_cast<DatatableAsset*>(pakAsset->extraData());
    assertm(dtblAsset, "Extra data should be valid at this point.");

    ImGui::TextUnformatted(std::format("Datatable: {} (0x{:X})", nullptr != dtblAsset->name ? dtblAsset->name : "null name", asset->GetAssetGUID()).c_str());
    ImGui::Text("Columns: %i Rows: %i", dtblAsset->numColumns, dtblAsset->numRows);

    constexpr ImGuiTableFlags tableFlags =
        ImGuiTableFlags_Resizable  | ImGuiTableFlags_Hideable | ImGuiTableFlags_RowBg | ImGuiTableFlags_Borders
        | ImGuiTableFlags_ScrollY | ImGuiTableFlags_ScrollX | ImGuiTableFlags_SizingFixedFit;

    const ImVec2 outerSize = ImVec2(0.f, 0.f);

    if (ImGui::BeginTable("Datatable", dtblAsset->numColumns, tableFlags, outerSize))
    {
        for (int i = 0; i < dtblAsset->numColumns; i++)
        {
            ImGui::TableSetupColumn(dtblAsset->GetColumn(i)->name, ImGuiTableColumnFlags_NoResize | ImGuiTableColumnFlags_WidthFixed, 0.0f, i);
        }
        ImGui::TableSetupScrollFreeze(0, 1);

        ImGui::TableHeadersRow();

        for (int i = 0; i < dtblAsset->numRows; i++)
        {
            ImGui::PushID(i); // id of this line ?

            ImGui::TableNextRow(ImGuiTableRowFlags_None, 0.0f);

            const char* const row = dtblAsset->GetRowPtr(i);

            for (int j = 0; j < dtblAsset->numColumns; j++)
            {
                if (ImGui::TableSetColumnIndex(j))
                {
                    const DatatableAssetColumn* const column = dtblAsset->GetColumn(j);

                    switch (column->type)
                    {
                    case DatatableColumType_t::Bool:
                    {
                        const bool& data = *reinterpret_cast<const bool* const>(row + column->rowOffset);
                        ImGui::TextUnformatted(data ? "true" : "false");

                        break;
                    }
                    case DatatableColumType_t::Int:
                    {
                        const int& data = *reinterpret_cast<const int* const>(row + column->rowOffset);
                        ImGui::Text("%i", data);

                        break;
                    }
                    case DatatableColumType_t::Float:
                    {
                        const float& data = *reinterpret_cast<const float* const>(row + column->rowOffset);
                        ImGui::Text("%f", data);

                        break;
                    }
                    case DatatableColumType_t::Vector:
                    {
                        const Vector* const data = reinterpret_cast<const Vector* const>(row + column->rowOffset);
                        ImGui::Text("%f, %f, %f", data->x, data->y, data->z);

                        break;
                    }
                    case DatatableColumType_t::String:
                    case DatatableColumType_t::Asset:
                    case DatatableColumType_t::AssetNoPrecache:
                    {
                        const char* const data = *reinterpret_cast<const char* const* const>(row + column->rowOffset);

                        // catch excluded data
                        if (data[0] == 0xf)
                        {
                            ImGui::TextUnformatted("!!DATA EXCLUDED!!");

                            break;
                        }

                        ImGui::TextUnformatted(data);    

                        break;
                    }
                    default:
                    {
                        assertm(false, "invalid datatable type");
                        break;
                    }
                    }
                }
            }

            ImGui::PopID(); // no longer working on this id
        }

        ImGui::EndTable();
    }

    return nullptr;
}

enum eDatatableExportSetting
{
    CSV,
};

#define HANDLE_LAST_COLUMN(idx, num) (idx == (num - 1) ? "\n" : ",")
bool ExportCSVDatatableAsset(CPakAsset* const asset, const DatatableAsset* const dtblAsset, std::filesystem::path& exportPath)
{
    UNUSED(asset);

    exportPath.replace_extension(".csv");

    StreamIO outFile(exportPath.string(), eStreamIOMode::Write);
    std::ofstream& out = *outFile.W();

    // set up the header row
    for (int i = 0; i < dtblAsset->numColumns; i++)
    {
        out << "\"" << dtblAsset->GetColumn(i)->name << "\"";
        out << HANDLE_LAST_COLUMN(i, dtblAsset->numColumns);
    }
    
    // write rows
    for (int i = 0; i < dtblAsset->numRows; i++)
    {
        const char* const row = dtblAsset->GetRowPtr(i);

        for (int j = 0; j < dtblAsset->numColumns; j++)
        {
            const DatatableAssetColumn* const column = dtblAsset->GetColumn(j);

            switch (column->type)
            {
            case DatatableColumType_t::Bool:
            {
                const bool& data = *reinterpret_cast<const bool* const>(row + column->rowOffset);
                out << (data ? "true" : "false");

                break;
            }
            case DatatableColumType_t::Int:
            {
                const int& data = *reinterpret_cast<const int* const>(row + column->rowOffset);
                out << data;

                break;
            }
            case DatatableColumType_t::Float:
            {
                const float& data = *reinterpret_cast<const float* const>(row + column->rowOffset);
                out << data;

                break;
            }
            case DatatableColumType_t::Vector:
            {
                const Vector* const data = reinterpret_cast<const Vector* const>(row + column->rowOffset);
                out << "\"<" << data->x << "," << data->y << "," << data->z << ">\"";

                break;
            }
            case DatatableColumType_t::String:
            case DatatableColumType_t::Asset:
            case DatatableColumType_t::AssetNoPrecache:
            {
                const char* const data = *reinterpret_cast<const char* const* const>(row + column->rowOffset);

                // catch excluded data
                if (data[0] == 0xf)
                {
                    out << "\"" << "!!DATA EXCLUDED!!" << "\"";
                    break;
                }

                out << "\"" << data << "\"";

                break;
            }
            default:
            {
                assertm(false, "invalid datatable type");
                break;
            }
            }

            out << HANDLE_LAST_COLUMN(j, dtblAsset->numColumns);
        }
    }

    // final row to save the type of each column
    for (int i = 0; i < dtblAsset->numColumns; i++)
    {
        out << s_DatatableColumnTypeName[static_cast<int>(dtblAsset->GetColumn(i)->type)];
        // rapidcsv handles an empty line as a new entry, so unlike other columns,
        // we shouldn't newline here when we reached the last column as otherwise
        // we will treat the empty line as the asset type row in repak.
        out << (i == (dtblAsset->numColumns - 1) ? "" : ",");
    }

    outFile.close();

    return true;
}
#undef HANDLE_LAST_COLUMN

bool ExportDatatableAsset(CAsset* const asset, const int setting)
{
    CPakAsset* pakAsset = static_cast<CPakAsset*>(asset);

    const DatatableAsset* const dtblAsset = reinterpret_cast<DatatableAsset*>(pakAsset->extraData());
    assertm(dtblAsset, "Extra data should be valid at this point.");

    std::filesystem::path exportPath = std::filesystem::current_path().append(EXPORT_DIRECTORY_NAME);
    std::filesystem::path stgsPath = asset->GetAssetName();

    exportPath.append(stgsPath.parent_path().string());

    if (!CreateDirectories(exportPath))
    {
        assertm(false, "Failed to create asset type directory.");
        return false;
    }

    exportPath.append(stgsPath.filename().string());

    switch (setting)
    {
    case eDatatableExportSetting::CSV:
    {
        return ExportCSVDatatableAsset(pakAsset, dtblAsset, exportPath);
    }
    default:
    {
        assertm(false, "Export setting is not handled.");
        return false;
    }
    }

    unreachable();
}

void InitDatatableAssetType()
{
    AssetTypeBinding_t type =
    {
        .type = 'lbtd',
        .headerAlignment = 8,
        .loadFunc = LoadDatatableAsset,
        .postLoadFunc = nullptr,
        .previewFunc = PreviewDatatableAsset,
        .e = { ExportDatatableAsset, 0, nullptr, 0ull },
    };

    REGISTER_TYPE(type);
}