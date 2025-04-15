#include <pch.h>
#include <game/rtech/assets/subtitles.h>

#include <thirdparty/imgui/imgui.h>

extern ExportSettings_t g_ExportSettings;

void LoadSubtitlesAsset(CAssetContainer* const pak, CAsset* const asset)
{
	CPakAsset* pakAsset = static_cast<CPakAsset*>(asset);

	SubtitlesAsset* subtitlesAsset = nullptr;

	switch (pakAsset->version())
	{
	case 0:
	case 1:
	{
		SubtitlesAssetHeader_v0_t* hdr = reinterpret_cast<SubtitlesAssetHeader_v0_t*>(pakAsset->header());
		subtitlesAsset = new SubtitlesAsset(hdr);
		break;
	}
	default:
	{
		assertm(false, "unaccounted asset version, will cause major issues!");
		return;
	}
	}

	std::string language = static_cast<CPakFile*>(pak)->getPakStem();
	language = language.substr(10, language.length() - 10);
	const std::string name = "subtitles/subtitles_" + language + ".rpak";

	pakAsset->setExtraData(subtitlesAsset);
	pakAsset->SetAssetName(name);
}

void PostLoadSubtitlesAsset(CAssetContainer* const pak, CAsset* const asset)
{
	UNUSED(pak);

	CPakAsset* pakAsset = static_cast<CPakAsset*>(asset);

	switch (pakAsset->version())
	{
	case 0:
	case 1:
		break;
	default:
		return;
	}

	assertm(pakAsset->extraData(), "extra data should be valid");
	SubtitlesAsset* const subtitlesAsset = reinterpret_cast<SubtitlesAsset* const>(pakAsset->extraData());
	subtitlesAsset->parsed.reserve(subtitlesAsset->stringCount);

	for (int i = 0; i < subtitlesAsset->entryCount; i++)
	{
		const SubtitlesAssetEntry_t* const entry = &subtitlesAsset->entries[i];

		if (entry->hash == 0)
			continue;

		char* entryStr = subtitlesAsset->strings + entry->stringOffset;

		if (!strncmp(entryStr, "<clr:", 5))
		{
			int values[3];
			sscanf_s(entryStr, "<clr:%i,%i,%i>", &values[0], &values[1], &values[2]);

			const char* string = strchr(entryStr, '>') + 1;

			subtitlesAsset->parsed.emplace_back(Vector(static_cast<float>(values[0]), static_cast<float>(values[1]), static_cast<float>(values[2])), entry->hash, string);
		}
		else
		{
			subtitlesAsset->parsed.emplace_back(Vector(255.0f), entry->hash, entryStr);
			assertm(false, "checking checking checking"); // minor spelling mistakes mspainish
		}

		assertm(subtitlesAsset->parsed.size() <= static_cast<size_t>(subtitlesAsset->stringCount), "more strings than stringCount");
	}
}

void* PreviewSubtitlesAsset(CAsset* const asset, const bool firstFrameForAsset)
{
	UNUSED(firstFrameForAsset);

	CPakAsset* pakAsset = static_cast<CPakAsset*>(asset);

    assertm(asset, "Asset should be valid.");

    if (pakAsset->version() > 1)
    {
        ImGui::Text("This asset version is not currently supported for preview.");
        return nullptr;
    }

    SubtitlesAsset* const subtitlesAsset = reinterpret_cast<SubtitlesAsset*>(pakAsset->extraData());
    assertm(subtitlesAsset, "Extra data should be valid at this point.");

    constexpr ImGuiTableFlags tableFlags =
        ImGuiTableFlags_Resizable | ImGuiTableFlags_Reorderable | ImGuiTableFlags_Hideable
        | ImGuiTableFlags_Sortable | ImGuiTableFlags_SortMulti
        | ImGuiTableFlags_RowBg | ImGuiTableFlags_Borders
        | ImGuiTableFlags_ScrollY | ImGuiTableFlags_SizingFixedFit;

    const ImVec2 outerSize = ImVec2(0.f, ImGui::GetTextLineHeightWithSpacing() * 12.f);

    if (ImGui::BeginTable("Arg Table", 2, tableFlags, outerSize))
    {
        ImGui::TableSetupColumn("Hash", ImGuiTableColumnFlags_NoResize | ImGuiTableColumnFlags_WidthFixed | ImGuiTableColumnFlags_NoHide, 0.0f, 0);
        ImGui::TableSetupColumn("Subtitle", ImGuiTableColumnFlags_NoResize | ImGuiTableColumnFlags_WidthFixed | ImGuiTableColumnFlags_NoHide, 0.0f, 1);
        ImGui::TableSetupScrollFreeze(1, 1);

        //ImGuiTableSortSpecs* sortSpecs = ImGui::TableGetSortSpecs(); // get the sorting settings from this table

        ImGui::TableHeadersRow();

        ImGuiListClipper clipper;
        clipper.Begin(static_cast<int>(subtitlesAsset->parsed.size()));
        while (clipper.Step())
        {
            for (int i = clipper.DisplayStart; i < clipper.DisplayEnd; i++)
            {
                ImGui::PushID(i); 

				const SubtitlesEntry& entry = subtitlesAsset->parsed.at(i);

                ImGui::TableNextRow(ImGuiTableRowFlags_None, 0.0f);

				ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(entry.clr.x, entry.clr.y, entry.clr.z, 1.f));
                if (ImGui::TableSetColumnIndex(0))
					ImGui::Text("0x%x", entry.hash);

				if (ImGui::TableSetColumnIndex(1))
					ImGui::TextUnformatted(entry.subtitle);

				ImGui::PopStyleColor();

                ImGui::PopID(); // no longer working on this id

            }
        }

        ImGui::EndTable();
    }

    return nullptr;
}

static bool ExportTXTSubtitlesAsset(const SubtitlesAsset* const subtitlesAsset, std::filesystem::path& exportPath)
{
	exportPath.replace_extension(".txt");

	StreamIO out;
	if (!out.open(exportPath.string(), eStreamIOMode::Write))
	{
		assertm(false, "Failed to open file for write.");
		return false;
	}

	std::ofstream& stream = *out.W();

	for (auto& entry : subtitlesAsset->parsed)
	{
		stream << entry.subtitle << "\n";
	}

	out.close();

	return true;
}

static bool ExportCSVSubtitlesAsset(const SubtitlesAsset* const subtitlesAsset, std::filesystem::path& exportPath)
{
	exportPath.replace_extension(".csv");

	StreamIO out;
	if (!out.open(exportPath.string(), eStreamIOMode::Write))
	{
		assertm(false, "Failed to open file for write.");
		return false;
	}

	std::ofstream& stream = *out.W();

	// add header row
	stream << "\"hash\",\"color\",\"subtitle\"\n";

	char fmt[128]{};

	for (auto& entry : subtitlesAsset->parsed)
	{
        sprintf_s(fmt, 128, "\"%x\",\"%i,%i,%i\",\"", entry.hash, static_cast<int>(entry.clr.x), static_cast<int>(entry.clr.y), static_cast<int>(entry.clr.z));
		stream << fmt << entry.subtitle << "\"\n";
	}

	out.close();

	return true;
}

enum eSubtitlesExportSetting
{
    SUBT_TXT,	// export the subtitle strings
	SUBT_CSV,	// export all the data for subtitles in a reusable form
};

static const char* const s_PathPrefixUI = s_AssetTypePaths.find(AssetType_t::SUBT)->second;
bool ExportSubtitlesAsset(CAsset* const asset, const int setting)
{
	CPakAsset* pakAsset = static_cast<CPakAsset*>(asset);

    SubtitlesAsset* const subtitlesAsset = reinterpret_cast<SubtitlesAsset*>(pakAsset->extraData());
    assertm(subtitlesAsset, "Extra data should be valid at this point.");

    // Create exported path + asset path.
    std::filesystem::path exportPath = std::filesystem::current_path().append(EXPORT_DIRECTORY_NAME); // 
    const std::filesystem::path subtitlesPath(pakAsset->GetAssetName());

    // truncate paths?
    if (g_ExportSettings.exportPathsFull)
        exportPath.append(subtitlesPath.parent_path().string());
    else
        exportPath.append(s_PathPrefixUI);

    if (!CreateDirectories(exportPath))
    {
        assertm(false, "Failed to create asset type directory.");
        return false;
    }

    exportPath.append(subtitlesPath.stem().string());

    switch (setting)
    {
    case eSubtitlesExportSetting::SUBT_TXT:
    {
		return ExportTXTSubtitlesAsset(subtitlesAsset, exportPath);
    }
	case eSubtitlesExportSetting::SUBT_CSV:
	{
		return ExportCSVSubtitlesAsset(subtitlesAsset, exportPath);
	}
    default:
		assertm(false, "Export setting is not handled.");
		return false;
    }

    unreachable();
}

void InitSubtitlesAsset()
{
	static const char* settings[] = { "TXT", "CSV" };
	AssetTypeBinding_t type =
	{
		.type = 'tbus',
		.headerAlignment = 8,
		.loadFunc = LoadSubtitlesAsset,
		.postLoadFunc = PostLoadSubtitlesAsset,
		.previewFunc = PreviewSubtitlesAsset,
		.e = { ExportSubtitlesAsset, 0, settings, ARRSIZE(settings) },
	};

	REGISTER_TYPE(type);
}