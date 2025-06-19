#include <pch.h>
#include <game/rtech/assets/settings_layout.h>
#include <game/rtech/cpakfile.h>
#include <game/rtech/utils/utils.h>
#include <imgui.h>

extern ExportSettings_t g_ExportSettings;
static const char* const s_PathPrefixSTLT = s_AssetTypePaths.find(AssetType_t::STLT)->second;

uint32_t SettingsLayout_GetFieldAlignmentForType(const eSettingsFieldType type)
{
	switch (type)
	{
	case eSettingsFieldType::ST_BOOL:
		return sizeof(bool);
	case eSettingsFieldType::ST_INTEGER:
	case eSettingsFieldType::ST_ARRAY_2:
		return sizeof(int);
	case eSettingsFieldType::ST_FLOAT:
	case eSettingsFieldType::ST_FLOAT2:
	case eSettingsFieldType::ST_FLOAT3:
		return sizeof(float);
	case eSettingsFieldType::ST_STRING:
	case eSettingsFieldType::ST_ASSET:
	case eSettingsFieldType::ST_ASSET_2:
		return sizeof(void*);

	default: assert(0); return 0;
	}
}

bool SettingsFieldFinder_FindFieldByAbsoluteOffset(const SettingsLayoutAsset* const layout, const uint32_t targetOffset, SettingsLayoutFindByOffsetResult_s& result)
{
	for (const SettingsField& field : layout->layoutFields)
	{
		const uint32_t totalValueBufSizeAligned = IALIGN(layout->totalLayoutSize, layout->alignment);
		const uint32_t originalBase = result.currentBase;

		if (targetOffset > result.currentBase + (layout->arrayValueCount * totalValueBufSizeAligned))
			return false; // Beyond this layout.

		if (targetOffset < field.valueOffset)
			return false; // Invalid offset (i.e. we have 2 ints at 4 and 8, but target was 5).

		// [amos]: handle everything in our current layout, including nested arrays.
		// I haven't seen mods mapped to nested arrays yet, and I also haven't seen
		// dynamic arrays nested into static arrays yet in original paks, but RePak
		// supports nesting static arrays and dynamic arrays into static arrays.
		for (int currArrayIdx = 0; currArrayIdx < layout->arrayValueCount; currArrayIdx++)
		{
			const uint32_t elementBase = result.currentBase + (currArrayIdx * totalValueBufSizeAligned);
			const uint32_t absoluteFieldOffset = elementBase + field.valueOffset;

			const bool isStaticArray = field.dataType == eSettingsFieldType::ST_ARRAY;

			// [amos]: the first member of an element in a static array will
			// share the same offset as its static array. Delay it off to
			// the next recursion so we return the name of the member of the
			// element in the array instead since this function does a lookup
			// by absolute offsets, and static arrays technically don't exist
			// in that context. This is also required for constructing the 
			// field access path correctly for given offset.
			if (!isStaticArray && targetOffset == absoluteFieldOffset)
			{
				result.field = &field;
				result.fieldAccessPath.insert(0, field.fieldName);
				result.lastArrayIdx = currArrayIdx;

				return true;
			}

			// [amos]: getting offsets to dynamic arrays items outside the
			// game's runtime is not supported! Only static arrays are.
			if (isStaticArray)
			{
				const SettingsLayoutAsset* const subLayout = &layout->subHeaders[field.valueSubLayoutIdx];
				result.currentBase = IALIGN(absoluteFieldOffset, subLayout->alignment);

				if (SettingsFieldFinder_FindFieldByAbsoluteOffset(subLayout, targetOffset, result))
				{
					result.fieldAccessPath.insert(0, std::format("{:s}[{:d}].", field.fieldName, result.lastArrayIdx));
					result.lastArrayIdx = currArrayIdx;

					return true;
				}

				result.currentBase = originalBase;
			}
		}
	}

	// Not found.
	return false;
}

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

	pakAsset->SetAssetName(settingsLayoutAsset->name, true);
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

struct SettingsLayoutPreviewState_s
{
	void ResetIndices(CAsset* const asset, const SettingsLayoutAsset* layout)
	{
		lastAssetCached = asset;

		previewList.clear();
		previewList.emplace_back(layout);
	}

	const SettingsLayoutAsset* GetCurrent()
	{
		return previewList.back();
	}

	const void SetCurrent(const SettingsLayoutAsset* layout)
	{
		previewList.emplace_back(layout);
	}

	bool HasParent() const
	{
		return previewList.size() > 1;
	}

	const void SetToParent()
	{
		previewList.pop_back();
	}

	const CAsset* lastAssetCached;
	std::vector<const SettingsLayoutAsset*> previewList;
};

static SettingsLayoutPreviewState_s s_settingsLayoutPreviewState;

void* PreviewSettingsLayoutAsset(CAsset* const asset, const bool firstFrameForAsset)
{
	UNUSED(firstFrameForAsset);
	CPakAsset* pakAsset = static_cast<CPakAsset*>(asset);

	const SettingsLayoutAsset* const layoutAsset = reinterpret_cast<SettingsLayoutAsset*>(pakAsset->extraData());

	if (asset != s_settingsLayoutPreviewState.lastAssetCached)
		s_settingsLayoutPreviewState.ResetIndices(asset, layoutAsset);

	ImGui::BeginDisabled(!s_settingsLayoutPreviewState.HasParent());

	if (ImGui::Button("Return to Parent##SettingsLayoutPreview"))
		s_settingsLayoutPreviewState.SetToParent();

	ImGui::EndDisabled();
	ImGui::SameLine();

	const SettingsLayoutAsset* const currentLayoutAsset = s_settingsLayoutPreviewState.GetCurrent();
	const size_t numSubHeaders = currentLayoutAsset->subHeaders.size();

	ImGui::BeginDisabled(numSubHeaders == 0);

	if (ImGui::BeginCombo("Sub-layout", nullptr))
	{
		for (size_t i = 0; i < numSubHeaders; i++)
		{
			const SettingsLayoutAsset* const subLayout = &currentLayoutAsset->subHeaders[i];

			if (ImGui::Selectable(std::format("{:d}##SettingsLayoutPreview", i).c_str(), subLayout == currentLayoutAsset))
			{
				s_settingsLayoutPreviewState.SetCurrent(subLayout);
				break;
			}
		}

		ImGui::EndCombo();
	}

	ImGui::EndDisabled();

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

		for (size_t i = 0; i < currentLayoutAsset->layoutFields.size(); ++i)
		{
			ImGui::PushID(static_cast<int>(i));

			ImGui::TableNextRow(ImGuiTableRowFlags_None, 0.f);

			const SettingsField& layoutField = currentLayoutAsset->layoutFields[i];

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