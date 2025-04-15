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
		ShaderSetAssetHeader_v13_t* hdr = reinterpret_cast<ShaderSetAssetHeader_v13_t*>(pakAsset->header());
		shdsAsset = new ShaderSetAsset(hdr);
		break;
	}
	default:
	{
		assertm(false, "unaccounted asset version, will cause major issues!");
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

		pakAsset->SetAssetName(name);
	}

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
		break;
	default:
		return;
	}

	ShaderSetAsset* const shdsAsset = reinterpret_cast<ShaderSetAsset*>(pakAsset->extraData());
	assertm(shdsAsset, "Extra data should be valid at this point.");

	shdsAsset->vertexShaderAsset = g_assetData.FindAssetByGUID<CPakAsset>(shdsAsset->vertexShader);
	shdsAsset->pixelShaderAsset = g_assetData.FindAssetByGUID<CPakAsset>(shdsAsset->pixelShader);

	//if (shdsAsset->vertexShader && !shdsAsset->vertexShaderAsset)
	//	Log("Shaderset has vertex shader but it is not loaded.\n");

	//if (shdsAsset->pixelShader && !shdsAsset->pixelShader)
	//	Log("Shaderset has pixel shader but it is not loaded.\n");
}

void* PreviewShaderSetAsset(CAsset* const asset, const bool firstFrameForAsset)
{
	UNUSED(firstFrameForAsset);

	CPakAsset* pakAsset = static_cast<CPakAsset*>(asset);

	const ShaderSetAsset* const shdsAsset = reinterpret_cast<ShaderSetAsset*>(pakAsset->extraData());
	assertm(shdsAsset, "Extra data should be valid at this point.");

	ImGui::Text("Number of vertex shader textures: %i", shdsAsset->numVertexShaderTextures);
	ImGui::Text("Number of pixel shader textures:  %i", shdsAsset->numPixelShaderTextures);
	ImGui::Text("Number of samplers:  %i", shdsAsset->numSamplers);

	ImGui::Text("First resource bind point: %i", shdsAsset->firstResourceBindPoint);
	ImGui::Text("Number of Resources: %i", shdsAsset->numResources);

	// Vertex Shader
	bool foundVertexShaderName = false;
	if (shdsAsset->vertexShaderAsset)
	{
		const ShaderAsset* const shaderAsset = reinterpret_cast<ShaderAsset*>(shdsAsset->vertexShaderAsset->extraData());
		assertm(shaderAsset, "Extra data should be valid at this point.");

		if (shaderAsset->name)
		{
			foundVertexShaderName = true;
			ImGui::Text("Vertex Shader: %s", shaderAsset->name);
		}
	}

	// If there is no vertex shader or the pixel shader does not have a name pointer
	if(!foundVertexShaderName)
		ImGui::Text("Vertex Shader: %llx (not loaded or no debug name)", shdsAsset->vertexShader);

	// Pixel Shader
	bool foundPixelShaderName = false;
	if (shdsAsset->pixelShaderAsset)
	{
		const ShaderAsset* const shaderAsset = reinterpret_cast<ShaderAsset*>(shdsAsset->pixelShaderAsset->extraData());
		assertm(shaderAsset, "Extra data should be valid at this point.");

		if (shaderAsset->name)
		{
			foundPixelShaderName = true;
			ImGui::Text("Pixel Shader: %s", shaderAsset->name);
		}
	}

	// If there is no pixel shader or the pixel shader does not have a name pointer
	if(!foundPixelShaderName)
		ImGui::Text("Pixel Shader: %llx (not loaded or no debug name)", shdsAsset->pixelShader);

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
		const ShaderAsset* const pixelShaderAsset = reinterpret_cast<ShaderAsset*>(shaderSetAsset->pixelShaderAsset->extraData());
		ConstructMSWShader(pixelShader, pixelShaderAsset);

		writer.SetShader(&pixelShader);
	}

	CMultiShaderWrapperIO::Shader_t vertexShader;

	if (packShaders && shaderSetAsset->vertexShaderAsset)
	{
		const ShaderAsset* const vertexShaderAsset = reinterpret_cast<ShaderAsset*>(shaderSetAsset->vertexShaderAsset->extraData());
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

	const ShaderSetAsset* const shdsAsset = reinterpret_cast<ShaderSetAsset*>(pakAsset->extraData());
	assertm(shdsAsset, "Extra data should be valid at this point.");

	// not currently used
	// Create exported path + asset path.
	std::filesystem::path exportPath = std::filesystem::current_path().append(EXPORT_DIRECTORY_NAME);
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
		.type = 'sdhs',
		.headerAlignment = 8,
		.loadFunc = LoadShaderSetAsset,
		.postLoadFunc = PostLoadShaderSetAsset,
		.previewFunc = PreviewShaderSetAsset,
		.e = { ExportShaderSetAsset, 0, settings, ARRSIZE(settings) },
	};

	REGISTER_TYPE(type);
}