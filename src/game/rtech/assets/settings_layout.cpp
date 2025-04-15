#include <pch.h>
#include <game/rtech/assets/settings_layout.h>
#include <game/rtech/cpakfile.h>
#include <game/rtech/utils/utils.h>
#include <imgui.h>

extern ExportSettings_t g_ExportSettings;
static const char* const s_PathPrefixSTLT = s_AssetTypePaths.find(AssetType_t::STLT)->second;

void SettingsLayoutAsset::ParseAndSortFields()
{
	for (uint32_t i = 0; i < this->fieldCount; ++i)
	{
		const SettingsFieldMap_t& map = fieldMap[i];
		const SettingsField_t& entry = fieldData[map.fieldBucketIndex];

		assert(!entry.IsEmpty());

		SettingsField parsedEntry = {
			.fieldName = this->GetStringFromOffset(entry.nameOffset),
			.helpText = this->GetStringFromOffset(map.helpTextIndex),
			.valueOffset = entry.valueOffset,
			.valueSubLayoutIdx = entry.valueSubLayoutIdx,
			.dataType = entry.dataType
		};
		this->layoutFields.push_back(parsedEntry);
	}

	std::sort(this->layoutFields.begin(), this->layoutFields.end());

	// Also sort all the sub layouts.
	for (SettingsLayoutAsset& subLayout : subHeaders)
		subLayout.ParseAndSortFields();
}


void LoadSettingsLayoutAsset(CAssetContainer* pak, CAsset* asset)
{
	UNUSED(pak);

	CPakAsset* pakAsset = static_cast<CPakAsset*>(asset);

	SettingsLayoutHeader_v0_t* hdr = reinterpret_cast<SettingsLayoutHeader_v0_t*>(pakAsset->header());
	
	SettingsLayoutAsset* settingsLayoutAsset = new SettingsLayoutAsset(hdr);

	pakAsset->SetAssetName(settingsLayoutAsset->name);
	pakAsset->setExtraData(settingsLayoutAsset);
}

void PostLoadSettingsLayoutAsset(CAssetContainer* pak, CAsset* asset)
{
	UNUSED(pak);

	CPakAsset* pakAsset = static_cast<CPakAsset*>(asset);

	SettingsLayoutAsset* layoutAsset = reinterpret_cast<SettingsLayoutAsset*>(pakAsset->extraData());

	layoutAsset->ParseAndSortFields();
}

enum eSettingsLayoutColumnID
{
	SLC_NAME,
	SLC_TYPE,
	SLC_VALUE_OFFSET,
	SLC_SUB_LAYOUT_INDEX,
	SLC_HELP_TEXT,

	_SLC_COUNT
};

void* PreviewSettingsLayoutAsset(CAsset* const asset, const bool firstFrameForAsset)
{
	UNUSED(firstFrameForAsset);

	CPakAsset* pakAsset = static_cast<CPakAsset*>(asset);

	SettingsLayoutAsset* layoutAsset = reinterpret_cast<SettingsLayoutAsset*>(pakAsset->extraData());

	constexpr ImGuiTableFlags tableFlags =
		ImGuiTableFlags_Resizable | ImGuiTableFlags_Reorderable | ImGuiTableFlags_Hideable
		| /*ImGuiTableFlags_Sortable |*/ ImGuiTableFlags_SortMulti
		| ImGuiTableFlags_RowBg | ImGuiTableFlags_Borders | ImGuiTableFlags_NoBordersInBody
		| ImGuiTableFlags_ScrollY /*| ImGuiTableFlags_SizingFixedFit*/;

	const ImVec2 outerSize = ImVec2(0.f, 0.f);

	if (ImGui::BeginTable("Settings Layout Fields", eSettingsLayoutColumnID::_SLC_COUNT, tableFlags, outerSize))
	{
		ImGui::TableSetupColumn("Field Name", ImGuiTableColumnFlags_NoResize | ImGuiTableColumnFlags_WidthFixed | ImGuiTableColumnFlags_NoHide, 0.0f, eSettingsLayoutColumnID::SLC_NAME);
		ImGui::TableSetupColumn("Data Type", ImGuiTableColumnFlags_NoResize | ImGuiTableColumnFlags_WidthFixed, 0.0f, eSettingsLayoutColumnID::SLC_TYPE);
		ImGui::TableSetupColumn("Value Offset", ImGuiTableColumnFlags_NoResize | ImGuiTableColumnFlags_WidthFixed, 0.0f, eSettingsLayoutColumnID::SLC_VALUE_OFFSET);
		ImGui::TableSetupColumn("Layout Index", ImGuiTableColumnFlags_NoResize | ImGuiTableColumnFlags_WidthFixed, 0.0f, eSettingsLayoutColumnID::SLC_SUB_LAYOUT_INDEX);
		ImGui::TableSetupColumn("Help Text", ImGuiTableColumnFlags_NoResize | ImGuiTableColumnFlags_WidthFixed, 0.0f, eSettingsLayoutColumnID::SLC_HELP_TEXT);

		ImGui::TableSetupScrollFreeze(1, 1);

		ImGui::TableHeadersRow();

		for (size_t i = 0; i < layoutAsset->layoutFields.size(); ++i)
		{
			ImGui::PushID(static_cast<int>(i));

			ImGui::TableNextRow(ImGuiTableRowFlags_None, 0.f);

			const SettingsField& layoutField = layoutAsset->layoutFields[i];

			if (ImGui::TableSetColumnIndex(eSettingsLayoutColumnID::SLC_NAME))
				ImGui::TextUnformatted(layoutField.fieldName);

			if (ImGui::TableSetColumnIndex(eSettingsLayoutColumnID::SLC_TYPE))
				ImGui::TextUnformatted(Settings_GetTypeNameString(layoutField.dataType));

			if (ImGui::TableSetColumnIndex(eSettingsLayoutColumnID::SLC_VALUE_OFFSET))
				ImGui::Text("%u", layoutField.valueOffset);

			if (ImGui::TableSetColumnIndex(eSettingsLayoutColumnID::SLC_SUB_LAYOUT_INDEX))
				ImGui::Text("%u", layoutField.valueSubLayoutIdx);

			if (ImGui::TableSetColumnIndex(eSettingsLayoutColumnID::SLC_HELP_TEXT))
				ImGui::TextUnformatted(layoutField.helpText);
			
			ImGui::PopID();

		}

		ImGui::EndTable();
	}

	return nullptr;
}

#define SETTINGS_LAYOUT_TABLE_HEADER "\"fieldName\",\"dataType\",\"layoutIndex\",\"helpText\"\n"
static bool ExportSettingsLayoutInternal(const SettingsLayoutAsset* const hdr, const char* const path,
	const size_t relativePathIndex, std::vector<std::string>* const pSubLayouts)
{
	std::vector<std::string> subLayouts;

	std::stringstream out;
	out << SETTINGS_LAYOUT_TABLE_HEADER;

	for (size_t i = 0; i < hdr->fieldCount; i++)
	{
		const SettingsField& field = hdr->layoutFields[i];
		const char* const end = i == (hdr->fieldCount - 1) ? "\"" : "\"\n";

		out << "\"" << field.fieldName << "\",\"" << s_settingsFieldTypeNames[(int)field.dataType] << "\",\"" << field.valueSubLayoutIdx <<  "\",\"" << field.helpText << end;

		if (field.dataType == eSettingsFieldType::ST_ARRAY || field.dataType == eSettingsFieldType::ST_ARRAY_2)
		{
			std::string subPath = path;

			if (!CreateDirectories(subPath))
				return false;

			subPath += '/';
			subPath += field.fieldName;

			const SettingsLayoutAsset* const sub = &hdr->subHeaders[field.valueSubLayoutIdx];

			if (!ExportSettingsLayoutInternal(sub, subPath.c_str(), relativePathIndex, &subLayouts))
				return false;
		}
	}

	std::string exportPath = path;
	const size_t extensionIndex = exportPath.length();

	exportPath.append(".csv");

	if (pSubLayouts)
	{
		const char* const relativePath = &exportPath.c_str()[relativePathIndex];
		pSubLayouts->push_back(relativePath);
	}

	StreamIO tableIO;
	if (!tableIO.open(exportPath.c_str(), eStreamIOMode::Write))
	{
		assertm(false, "Failed to open table file for write.");
		return false;
	}

	std::string outBuf = out.str();
	tableIO.write(outBuf.c_str(), outBuf.length());

	exportPath.resize(exportPath.length() + 1);
	exportPath.replace(extensionIndex, std::string::npos, ".json");

	StreamIO mapIO;
	if (!mapIO.open(exportPath.c_str(), eStreamIOMode::Write))
	{
		assertm(false, "Failed to open table file for write.");
		return false;
	}

	out.clear();
	out.str(std::string());

	out << "{\n";

	if (hdr->arrayValueCount > 1)
		out << "\t\"elementCount\": " << hdr->arrayValueCount << ",\n";

	const size_t numSublayoutPaths = subLayouts.size();

	const char* const end = numSublayoutPaths == 0 ? "\n" : ",\n";
	out << "\t\"extraDataSizeIndex\": " << hdr->extraDataSizeIndex << end;

	if (numSublayoutPaths > 0)
	{
		out << "\t\"subLayouts\": [\n";

		for (size_t i = 0; i < numSublayoutPaths; i++)
		{
			const char* const commaChar = i == (numSublayoutPaths - 1) ? "\"\n" : "\",\n";
			out << "\t\t\"" << subLayouts[i] << commaChar;
		}
		out << "\t]\n";
	}

	out << "}\n";

	outBuf = out.str();
	mapIO.write(outBuf.c_str(), outBuf.length());

	return true;
}

bool ExportSettingsLayout(CAsset* const asset, const int setting)
{
	UNUSED(setting);
	CPakAsset* pakAsset = static_cast<CPakAsset*>(asset);

	SettingsLayoutAsset* const hdr = reinterpret_cast<SettingsLayoutAsset*>(pakAsset->extraData());

	std::filesystem::path exportPath = std::filesystem::current_path().append(EXPORT_DIRECTORY_NAME);
	const std::filesystem::path stltPath(asset->GetAssetName());

	// +1 to skip the '/' at the end.
	const size_t relativePathIndex = exportPath.string().length()+1;

	if (g_ExportSettings.exportPathsFull)
		exportPath.append(stltPath.parent_path().string());
	else
		exportPath.append(s_PathPrefixSTLT);

	if (!CreateDirectories(exportPath))
	{
		assertm(false, "Failed to create asset type directory.");
		return false;
	}

	std::string rootPath = exportPath.string();
	rootPath += '/';
	rootPath += stltPath.stem().string();

	if (!ExportSettingsLayoutInternal(hdr, rootPath.c_str(), relativePathIndex, nullptr))
		return false;

	return true;
}

void InitSettingsLayoutAssetType()
{
	AssetTypeBinding_t type =
	{
		.type = 'tlts',
		.headerAlignment = 8,
		.loadFunc = LoadSettingsLayoutAsset,
		.postLoadFunc = PostLoadSettingsLayoutAsset,
		.previewFunc = PreviewSettingsLayoutAsset,
		.e = { ExportSettingsLayout, 0, nullptr, 0ull },
	};

	REGISTER_TYPE(type);
}