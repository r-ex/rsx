#include "pch.h"
#include "texture_list.h"

#include <game/rtech/cpakfile.h>
#include <game/rtech/utils/utils.h>
#include <thirdparty/imgui/imgui.h>

static void TextureList_LoadAsset(CAssetContainer* pak, CAsset* asset)
{
	UNUSED(pak);

	CPakAsset* const pakAsset = static_cast<CPakAsset*>(asset);

	TextureListAsset* textureListAsset = nullptr;

	// [amos]: As of R5pc_r5-241_J17_CL8728955_2025_03_14_15_16, v1 is still the latest.
	switch (pakAsset->version())
	{
	case 1:
	{
		const TextureListHeader_v1_s* const hdr = reinterpret_cast<const TextureListHeader_v1_s* const>(pakAsset->header());
		textureListAsset = new TextureListAsset(hdr);
		break;
	}
	default:
	{
		assertm(false, "unaccounted asset version, will cause major issues!");
		return;
	}
	}

	// [rika]: pretty sure there's only one texture list asset
	const uint64_t guidFromString = RTech::StringToGuid(s_TextureListCamoSkinsPath);

	if (guidFromString == asset->GetAssetGUID())
		pakAsset->SetAssetName(s_TextureListCamoSkinsPath);

	pakAsset->setExtraData(textureListAsset);
}

extern std::string FormatTextureAssetName(const char* const str);
static void TextureList_PostLoadAsset(CAssetContainer* pak, CAsset* asset)
{
	UNUSED(pak);

	CPakAsset* const pakAsset = static_cast<CPakAsset*>(asset);

	assertm(pakAsset->extraData(), "extra data should be valid");
	TextureListAsset* const textureListAsset = reinterpret_cast<TextureListAsset* const>(pakAsset->extraData());

	assertm(textureListAsset->textureGuids, "texture list had invalid guids?");
	assertm(textureListAsset->textureNames, "texture list had invalid names?");

	// [rika]: use the stored texture names to set asset names
	for (uint64_t i = 0; i < textureListAsset->numTextures; i++)
	{
		const uint64_t guid = textureListAsset->textureGuids[i];
		const char* const name = textureListAsset->textureNames[i];

		// [rika]: probably should check if texture asset has a name, but it won't ever get overwritten with an incorrect name
		CPakAsset* const textureAsset = g_assetData.FindAssetByGUID<CPakAsset>(guid);
		if (!textureAsset)
			continue;

		std::string newName = FormatTextureAssetName(name);
		const uint64_t newGuid = RTech::StringToGuid(newName.c_str());

		if (guid != newGuid)
			continue;

		textureAsset->SetAssetName(newName);
	}
}

enum TextureListTablePreviewColumn_e
{
	kIndex = 0,
	kName,
	kGuid,
};

static void* TextureList_PreviewAsset(CAsset* const asset, const bool firstFrameForAsset)
{
	UNUSED(firstFrameForAsset);

	CPakAsset* const pakAsset = static_cast<CPakAsset*>(asset);
	TextureListHeader_v1_s* const txls = reinterpret_cast<TextureListHeader_v1_s* const>(pakAsset->header());

	constexpr ImGuiTableFlags tableFlags =
		ImGuiTableFlags_Resizable | ImGuiTableFlags_Reorderable | ImGuiTableFlags_Hideable
		| /*ImGuiTableFlags_Sortable |*/ ImGuiTableFlags_SortMulti
		| ImGuiTableFlags_RowBg | ImGuiTableFlags_Borders | ImGuiTableFlags_NoBordersInBody
		| ImGuiTableFlags_ScrollY /*| ImGuiTableFlags_SizingFixedFit*/;

	const ImVec2 outerSize = ImVec2(0.f, 0.f);

	if (ImGui::BeginTable("Texture list##TextureList_PreviewAsset", 3, tableFlags, outerSize))
	{
		ImGui::TableSetupColumn("Index##TextureList_PreviewAsset", ImGuiTableColumnFlags_NoResize | ImGuiTableColumnFlags_WidthFixed | ImGuiTableColumnFlags_NoHide, 0.0f, TextureListTablePreviewColumn_e::kIndex);
		ImGui::TableSetupColumn("Name##TextureList_PreviewAsset", ImGuiTableColumnFlags_NoResize | ImGuiTableColumnFlags_WidthFixed, 0.0f, TextureListTablePreviewColumn_e::kName);
		ImGui::TableSetupColumn("Guid##TextureList_PreviewAsset", ImGuiTableColumnFlags_NoResize | ImGuiTableColumnFlags_WidthFixed, 0.0f, TextureListTablePreviewColumn_e::kGuid);

		ImGui::TableSetupScrollFreeze(1, 1);
		ImGui::TableHeadersRow();

		for (size_t i = 0; i < txls->numTextures; ++i)
		{
			ImGui::PushID(static_cast<int>(i));
			ImGui::TableNextRow(ImGuiTableRowFlags_None, 0.f);

			if (ImGui::TableSetColumnIndex(TextureListTablePreviewColumn_e::kIndex))
				ImGui::Text("%zu", i);

			if (ImGui::TableSetColumnIndex(TextureListTablePreviewColumn_e::kName))
				ImGui::TextUnformatted(txls->textureNames[i]);

			if (ImGui::TableSetColumnIndex(TextureListTablePreviewColumn_e::kGuid))
			{
				if (txls->textureGuids) // Null on server RPak's.
					ImGui::Text("%llX", txls->textureGuids[i]);
				else
					ImGui::TextUnformatted("N/A");
			}

			ImGui::PopID();

		}

		ImGui::EndTable();
	}

	return nullptr;
}

static bool TextureList_ExportAsset(CAsset* const asset, const int setting)
{
	UNUSED(asset);
	UNUSED(setting);

	std::filesystem::path exportPath = std::filesystem::current_path().append(EXPORT_DIRECTORY_NAME);
	std::filesystem::path stgsPath = asset->GetAssetName();

	exportPath.append(stgsPath.parent_path().string());

	if (!CreateDirectories(exportPath))
	{
		assertm(false, "Failed to create asset type directory.");
		return false;
	}

	exportPath.append(stgsPath.filename().string());
	exportPath.replace_extension(".json");

	StreamIO out;
	if (!out.open(exportPath.string(), eStreamIOMode::Write))
	{
		assertm(false, "Failed to open file for write.");
		return false;
	}

	std::string outBuf("{\n\t\"textures\": [\n");

	CPakAsset* const pakAsset = static_cast<CPakAsset*>(asset);
	TextureListHeader_v1_s* const txls = reinterpret_cast<TextureListHeader_v1_s* const>(pakAsset->header());

	for (size_t i = 0; i < txls->numTextures; i++)
	{
		std::string nameFormatted = std::format("\t\t\"{:s}\"{:s}\n", txls->textureNames[i], i == (txls->numTextures - 1) ? "" : ",");
		FixSlashes(nameFormatted);

		outBuf.append(nameFormatted);
	}

	outBuf.append("\t]\n}\n");
	out.write(outBuf.c_str(), outBuf.length());

	return true;
}

void InitTextureListAssetType()
{
	AssetTypeBinding_t type =
	{
		.type = 'slxt',
		.headerAlignment = 8,
		.loadFunc = TextureList_LoadAsset,
		.postLoadFunc = TextureList_PostLoadAsset,
		.previewFunc = TextureList_PreviewAsset,
		.e = { TextureList_ExportAsset, 0, nullptr, 0ull },
	};

	REGISTER_TYPE(type);
}
