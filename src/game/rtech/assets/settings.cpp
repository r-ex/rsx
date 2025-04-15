#include <pch.h>
#include <game/rtech/assets/settings.h>
#include <game/rtech/cpakfile.h>
#include <game/rtech/utils/utils.h>
#include <thirdparty/imgui/imgui.h>

void LoadSettingsAsset(CAssetContainer* pak, CAsset* asset)
{
	UNUSED(pak);

	CPakAsset* pakAsset = static_cast<CPakAsset*>(asset);

	SettingsAsset* settingsAsset = nullptr;

	switch (pakAsset->data()->headerStructSize)
	{
	case 72:
	{
		SettingsAssetHeader_v1_t* hdr = reinterpret_cast<SettingsAssetHeader_v1_t*>(pakAsset->header());

		settingsAsset = new SettingsAsset(hdr);

		break;
	}
	case 80:
	{
		SettingsAssetHeader_v2_t* hdr = reinterpret_cast<SettingsAssetHeader_v2_t*>(pakAsset->header());

		settingsAsset = new SettingsAsset(hdr);

		break;
	}
	}

	if (settingsAsset)
	{
		pakAsset->SetAssetName(settingsAsset->name);
		pakAsset->setExtraData(settingsAsset);
	}
	// todo: error handling?
}

void PostLoadSettingsAsset(CAssetContainer* pak, CAsset* asset)
{
	UNUSED(pak);
	CPakAsset* pakAsset = static_cast<CPakAsset*>(asset);

	SettingsAsset* settingsAsset = reinterpret_cast<SettingsAsset*>(pakAsset->extraData());

	settingsAsset->layoutAsset = g_assetData.FindAssetByGUID<CPakAsset>(settingsAsset->layoutGuid);

	if (!settingsAsset->layoutAsset)
	{
		// uh oh!
	}
}

struct DynamicArrayData_t
{
	int arraySize;
	int arrayOffset;
};

void SettingsAsset::R_WriteSetFileArray(std::string& out, const size_t indentLevel, const char* valuePtr,
	const size_t arrayElemCount, const SettingsLayoutAsset& subLayout)
{
	out += "[\n";

	const size_t layoutSize = subLayout.totalLayoutSize;
	const size_t fieldCount = subLayout.layoutFields.size();

	const std::string indentation = GetIndentation(indentLevel);

	for (uint32_t i = 0; i < arrayElemCount; ++i)
	{
		const char* elemValues = reinterpret_cast<const char*>(valuePtr) + (i * layoutSize);
		out += indentation + "\t{\n";

		for (size_t j = 0; j < fieldCount; ++j)
		{
			const SettingsField& subField = subLayout.layoutFields[j];
			out += indentation + "\t\t" + "\"" + subField.fieldName + "\": ";

			R_WriteSetFile(out, indentLevel+2, elemValues, &subLayout , &subField);

			const char* const commaChar = j != (fieldCount - 1) ? ",\n" : "\n";
			out += commaChar;
		}

		out += indentation + "\t}";

		if (fieldCount)
		{
			const char* const commaChar = i != (arrayElemCount - 1) ? ",\n" : "\n";
			out += commaChar;
		}
	}

	out += indentation + "]";
}

void SettingsAsset::R_WriteSetFile(std::string& out, const size_t indentLevel, const char* valData,
	const SettingsLayoutAsset* layout, const SettingsField* const field)
{
	switch (field->dataType)
	{
	case eSettingsFieldType::ST_BOOL:
	{
		out.append((valData[field->valueOffset]) ? "true" : "false");
		break;
	}
	case eSettingsFieldType::ST_INTEGER:
	{
		out.append(std::format("{:d}", *reinterpret_cast<const int*>(&valData[field->valueOffset])));
		break;
	}
	case eSettingsFieldType::ST_FLOAT:
	{
		out.append(std::format("{:f}", *reinterpret_cast<const float*>(&valData[field->valueOffset])));
		break;
	}
	case eSettingsFieldType::ST_FLOAT2:
	{
		const float* floatValues = reinterpret_cast<const float*>(&valData[field->valueOffset]);
		out.append(std::format("\"<{:f},{:f}>\"", floatValues[0], floatValues[1]));

		break;
	}
	case eSettingsFieldType::ST_FLOAT3:
	{
		const float* floatValues = reinterpret_cast<const float*>(&valData[field->valueOffset]);
		out.append(std::format("\"<{:f},{:f},{:f}>\"", floatValues[0], floatValues[1], floatValues[2]));

		break;
	}
	case eSettingsFieldType::ST_STRING:
	case eSettingsFieldType::ST_ASSET:
	case eSettingsFieldType::ST_ASSET_2:
	{
		const char* const charBuf = *(const char**)&valData[field->valueOffset];
		out.append(std::format("\"{:s}\"", charBuf));
		break;
	}
	case eSettingsFieldType::ST_ARRAY:
	{
		const SettingsLayoutAsset& subLayout = layout->subHeaders[field->valueSubLayoutIdx];
		R_WriteSetFileArray(out, indentLevel, &valData[field->valueOffset], subLayout.arrayValueCount, subLayout);

		break;
	}
	case eSettingsFieldType::ST_ARRAY_2:
	{
		const SettingsLayoutAsset& subLayout = layout->subHeaders[field->valueSubLayoutIdx];
		const DynamicArrayData_t* dynamicArrayData = reinterpret_cast<const DynamicArrayData_t*>(&valData[field->valueOffset]);

		const char* dynamicArrayElems = &valData[dynamicArrayData->arrayOffset];
		R_WriteSetFileArray(out, indentLevel, dynamicArrayElems, dynamicArrayData->arraySize, subLayout);

		break;
	}
	}
}

void SettingsAsset::R_WriteSetFile(std::string& out, const size_t indentLevel, const char* valData, const SettingsLayoutAsset* layout)
{
	const std::string indentation = GetIndentation(indentLevel);
	const size_t numLayoutFields = layout->layoutFields.size();

	for (size_t i = 0; i < numLayoutFields; ++i)
	{
		const SettingsField* const field = &layout->layoutFields.at(i);
		out += indentation + "\"" + field->fieldName + "\": ";

		R_WriteSetFile(out, indentLevel, valData, layout, field);

		const char* const commaChar = i != (numLayoutFields-1) ? ",\n" : "\n";
		out += commaChar;
	}
}

void SettingsAsset::R_WriteModNames(std::string& out) const
{
	out += "\t\"modNames\": [\n";

	for (int i = 0; i < modNameCount; i++)
	{
		const char* const commaChar = i != (modNameCount - 1) ? "," : "";
		out += std::string("\t\t\"") + modNames[i] + "\"" + commaChar + "\n";
	}

	out += "\t]";
}

static bool RenderSettingsAsset(CPakAsset* const asset, std::string& stringStream)
{
	SettingsAsset* settingsAsset = reinterpret_cast<SettingsAsset*>(asset->extraData());
	if (!settingsAsset->layoutAsset)
		return false;

	const SettingsLayoutAsset* const layout = reinterpret_cast<SettingsLayoutAsset*>(settingsAsset->layoutAsset->extraData());

	stringStream += std::string("{\n") + "\t\"layoutAsset\": \"" + layout->name + "\",\n";

	if (settingsAsset->uniqueId)
		stringStream += std::format("\t\"uniqueId\": {:d},\n", settingsAsset->uniqueId);;

	stringStream += "\t\"settings\": {\n";

	// Recursively write the .set file contents into the string stream
	settingsAsset->R_WriteSetFile(stringStream, 2, (const char*)settingsAsset->valueData, layout);

	stringStream += "\t}";

	if (settingsAsset->modNameCount)
	{
		stringStream += ",\n";
		settingsAsset->R_WriteModNames(stringStream);
	}

	stringStream += "\n}";
	FixSlashes(stringStream);

	return true;
}

bool ExportSettingsAsset(CAsset* const asset, const int setting)
{
	UNUSED(asset);
	UNUSED(setting);

	CPakAsset* pakAsset = static_cast<CPakAsset*>(asset);

	printf("Exporting settings asset \"%s\"\n", asset->GetAssetName().c_str());

	std::string stringStream;

	if (!RenderSettingsAsset(pakAsset, stringStream))
		return false;

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

	out.write(stringStream.c_str(), stringStream.length());
	return true;
}

static void* PreviewRSONAsset(CAsset* const asset, const bool firstFrameForAsset)
{
	CPakAsset* pakAsset = static_cast<CPakAsset*>(asset);

	std::string stringStream;

	if (!RenderSettingsAsset(pakAsset, stringStream))
	{
		ImGui::Text("Settings asset unavailable");
		return nullptr;
	}

	UNUSED(firstFrameForAsset);
	ImGui::InputTextMultiline("##settings_preview", const_cast<char*>(stringStream.c_str()), stringStream.length(), ImVec2(-1, -1), ImGuiInputTextFlags_ReadOnly);

	return nullptr;
}

void InitSettingsAssetType()
{
	AssetTypeBinding_t type =
	{
		.type = 'sgts',
		.headerAlignment = 8,
		.loadFunc = LoadSettingsAsset,
		.postLoadFunc = PostLoadSettingsAsset,
		.previewFunc = PreviewRSONAsset,
		.e = { ExportSettingsAsset, 0, nullptr, 0ull },
	};

	REGISTER_TYPE(type);
}