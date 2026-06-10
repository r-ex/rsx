#include <pch.h>
#include <game/rtech/assets/shaderset.h>
#include <thirdparty/imgui/imgui.h>

extern ExportSettings_t g_ExportSettings;

void LoadShaderSetAsset(CAssetContainer* const pak, CAsset* const asset)
{
	UNUSED(pak);

	CPakAsset* pakAsset = static_cast<CPakAsset*>(asset);

	ShaderSetAsset* shdsAsset = nullptr;

	switch (pakAsset->version())
	{
	case 8:
	{
		ShaderSetAssetHeader_v8_t* hdr = reinterpret_cast<ShaderSetAssetHeader_v8_t*>(pakAsset->header());
		shdsAsset = new ShaderSetAsset(hdr);
		break;
	}
	case 11:
	{
		ShaderSetAssetHeader_v11_t* hdr = reinterpret_cast<ShaderSetAssetHeader_v11_t*>(pakAsset->header());
		shdsAsset = new ShaderSetAsset(hdr);
		break;
	}
	case 12:
	{
		ShaderSetAssetHeader_v12_t* hdr = reinterpret_cast<ShaderSetAssetHeader_v12_t*>(pakAsset->header());
		shdsAsset = new ShaderSetAsset(hdr);
		break;
	}
	case 13:
	{
		if (pakAsset->data()->headerStructSize == sizeof(ShaderSetAssetHeader_v12_t))
		{
			pakAsset->SetAssetVersion({ 13, 1 }); // [rika]: set minor version

			ShaderSetAssetHeader_v12_t* hdr = reinterpret_cast<ShaderSetAssetHeader_v12_t*>(pakAsset->header());
			shdsAsset = new ShaderSetAsset(hdr);
			break;
		}

		ShaderSetAssetHeader_v13_t* hdr = reinterpret_cast<ShaderSetAssetHeader_v13_t*>(pakAsset->header());
		shdsAsset = new ShaderSetAsset(hdr);
		break;
	}
	case 14: 
	{
		assertm(pakAsset->data()->headerStructSize == sizeof(ShaderSetAssetHeader_v14_t), "incorrect header");

		ShaderSetAssetHeader_v14_t* hdr = reinterpret_cast<ShaderSetAssetHeader_v14_t*>(pakAsset->header());
		shdsAsset = new ShaderSetAsset(hdr);
		break;
	}
	case 15:
	{
		assertm(pakAsset->data()->headerStructSize == sizeof(ShaderSetAssetHeader_v15_t), "incorrect header");

		ShaderSetAssetHeader_v15_t* hdr = reinterpret_cast<ShaderSetAssetHeader_v15_t*>(pakAsset->header());
		shdsAsset = new ShaderSetAsset(hdr);
		break;
	}
	default:
	{
		assertm(false, "Unknown ShaderSet asset version");
		return;
	}
	}

	if (shdsAsset->name)
	{
		std::string name = shdsAsset->name;
		if (!name.starts_with("shaderset/"))
			name = "shaderset/" + name;

		if (!name.ends_with(".rpak"))
			name += ".rpak";

		pakAsset->SetAssetName(name, true);
	}
	else
		pakAsset->SetAssetNameFromCache();

	pakAsset->setExtraData(shdsAsset);
}

void PostLoadShaderSetAsset(CAssetContainer* const pak, CAsset* const asset)
{
	UNUSED(pak);

	CPakAsset* pakAsset = static_cast<CPakAsset*>(asset);

	switch (pakAsset->version())
	{
	case 8:
	case 11:
	case 12:
	case 13:
	case 14:
	case 15:
		break;
	default:
		return;
	}

	ShaderSetAsset* const shdsAsset = pakAsset->extraData<ShaderSetAsset* const>();

	shdsAsset->vertexShaderAsset = g_assetData.FindAssetByGUID<CPakAsset>(shdsAsset->vertexShader);
	shdsAsset->pixelShaderAsset = g_assetData.FindAssetByGUID<CPakAsset>(shdsAsset->pixelShader);
}

void* PreviewShaderSetAsset(CAsset* const asset, const bool firstFrameForAsset)
{
	UNUSED(firstFrameForAsset);

	CPakAsset* pakAsset = static_cast<CPakAsset*>(asset);

	const ShaderSetAsset* const shdsAsset = pakAsset->extraData<const ShaderSetAsset* const>();

	ImGui::Text("Number of vertex shader textures: %i", shdsAsset->numVertexShaderTextures);
	ImGui::Text("Number of pixel shader textures:  %i", shdsAsset->numPixelShaderTextures);
	ImGui::Text("Number of samplers:  %i", shdsAsset->numSamplers);

	ImGui::Text("First resource bind point: %i", shdsAsset->firstResourceBindPoint);
	ImGui::Text("Number of Resources: %i", shdsAsset->numResources);

	const char* vertexShaderDebugName = "(no debug name)";
	const char* pixelShaderDebugName = "(no debug name)";

	// Vertex Shader
	if (shdsAsset->vertexShaderAsset)
	{
		const ShaderAsset* const shaderAsset = shdsAsset->vertexShaderAsset->extraData<const ShaderAsset* const>();

		if (shaderAsset->name)
			vertexShaderDebugName = shaderAsset->name;
	}

	// Pixel Shader
	if (shdsAsset->pixelShaderAsset)
	{
		const ShaderAsset* const shaderAsset = shdsAsset->pixelShaderAsset->extraData<const ShaderAsset* const>();

		if (shaderAsset->name)
			pixelShaderDebugName = shaderAsset->name;
	}

	ImGui::Text("Vertex Shader: %llX (%s)", shdsAsset->vertexShader, vertexShaderDebugName);
	ImGui::Text("Pixel Shader: %llX (%s)", shdsAsset->pixelShader, pixelShaderDebugName);

	return nullptr;
}

enum eShaderSetExportSetting
{
	ShaderSet,
	ShaderSetPacked
};

#include <core/shaderexp/multishader.h>
extern void ConstructMSWShader(CMultiShaderWrapperIO::Shader_t& shader, const ShaderAsset* const shaderAsset);

static bool ExportMSWShaderSetAsset(const ShaderSetAsset* const shaderSetAsset, std::filesystem::path& exportPath, const bool packShaders)
{
	exportPath.replace_extension(".msw");

	CMultiShaderWrapperIO writer;
	writer.SetFileType(MultiShaderWrapperFileType_e::SHADERSET);

	CMultiShaderWrapperIO::Shader_t pixelShader;

	if (packShaders && shaderSetAsset->pixelShaderAsset)
	{
		const ShaderAsset* const pixelShaderAsset = shaderSetAsset->pixelShaderAsset->extraData<const ShaderAsset* const>();
		ConstructMSWShader(pixelShader, pixelShaderAsset);

		writer.SetShader(&pixelShader);
	}

	CMultiShaderWrapperIO::Shader_t vertexShader;

	if (packShaders && shaderSetAsset->vertexShaderAsset)
	{
		const ShaderAsset* const vertexShaderAsset = shaderSetAsset->vertexShaderAsset->extraData<const ShaderAsset* const>();
		ConstructMSWShader(vertexShader, vertexShaderAsset);

		writer.SetShader(&vertexShader);
	}

	writer.SetShaderSetHeader(shaderSetAsset->pixelShader, shaderSetAsset->vertexShader,
		shaderSetAsset->numPixelShaderTextures, shaderSetAsset->numVertexShaderTextures, shaderSetAsset->numSamplers,
		static_cast<uint8_t>(shaderSetAsset->firstResourceBindPoint), static_cast<uint8_t>(shaderSetAsset->numResources));

	return writer.WriteFile(exportPath.string().c_str());
}

static const char* const s_PathPrefixSHDR = s_AssetTypePaths.find(AssetType_t::SHDS)->second;
bool ExportShaderSetAsset(CAsset* const asset, const int setting)
{
	CPakAsset* pakAsset = static_cast<CPakAsset*>(asset);

	const ShaderSetAsset* const shdsAsset = pakAsset->extraData<const ShaderSetAsset* const>();

	// not currently used
	// Create exported path + asset path.
	std::filesystem::path exportPath = g_ExportSettings.GetExportDirectory();
	std::filesystem::path dirPath = exportPath;

	dirPath.append(s_PathPrefixSHDR);

	if (!CreateDirectories(dirPath))
	{
		assertm(false, "Failed to create asset type directory.");
		return false;
	}

	exportPath /= asset->GetAssetName();

	switch (setting)
	{
	case eShaderSetExportSetting::ShaderSet:
	{
		return ExportMSWShaderSetAsset(shdsAsset, exportPath, false);
	}
	case eShaderSetExportSetting::ShaderSetPacked:
	{
		return ExportMSWShaderSetAsset(shdsAsset, exportPath, true);
	}
	default:
	{
		assertm(false, "Export setting is not handled.");
		return false;
	}
	}

	unreachable();
}

void InitShaderSetAssetType()
{
	static const char* settings[] = { "MSW", "MSW (Packed)" };
	AssetTypeBinding_t type =
	{
		.name = "Shader Set",
		.type = 'sdhs',
		.headerAlignment = 8,
		.loadFunc = LoadShaderSetAsset,
		.postLoadFunc = PostLoadShaderSetAsset,
		.previewFunc = PreviewShaderSetAsset,
		.e = { ExportShaderSetAsset, 0, settings, ARRSIZE(settings) },
	};

	REGISTER_TYPE(type);
}
