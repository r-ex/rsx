#include <pch.h>
#include <game/rtech/assets/shader.h>
#include <game/rtech/cpakfile.h>
#include <game/rtech/utils/utils.h>

#include <imgui.h>

extern CDXParentHandler* g_dxHandler;

extern ExportSettings_t g_ExportSettings;

void LoadShaderAsset(CAssetContainer* pak, CAsset* asset)
{
	UNUSED(pak);
	CPakAsset* pakAsset = static_cast<CPakAsset*>(asset);

	ShaderAsset* shaderAsset = nullptr;
	switch (pakAsset->version())
	{
	case 8:
	{
		ShaderAssetHeader_v8_t* hdr = reinterpret_cast<ShaderAssetHeader_v8_t*>(pakAsset->header());
		shaderAsset = new ShaderAsset(hdr, reinterpret_cast<ShaderAssetCPU_t*>(pakAsset->cpu()));
		break;
	}
	case 12:
	{
		ShaderAssetHeader_v12_t* hdr = reinterpret_cast<ShaderAssetHeader_v12_t*>(pakAsset->header());
		shaderAsset = new ShaderAsset(hdr, reinterpret_cast<ShaderAssetCPU_t*>(pakAsset->cpu()));
		break;
	}
	case 13:
	{
		ShaderAssetHeader_v13_t* hdr = reinterpret_cast<ShaderAssetHeader_v13_t*>(pakAsset->header());
		shaderAsset = new ShaderAsset(hdr, reinterpret_cast<ShaderAssetCPU_t*>(pakAsset->cpu()));
		break;
	}
	case 14:
	{
		ShaderAssetHeader_v14_t* hdr = reinterpret_cast<ShaderAssetHeader_v14_t*>(pakAsset->header());
		shaderAsset = new ShaderAsset(hdr, reinterpret_cast<ShaderAssetCPU_t*>(pakAsset->cpu()));
		break;
	}
	case 15:
	{
		// [rika]: there's some where shaders in newer versions that don't have cpu data, and point to places that don't really have a shader header
		ShaderAssetHeader_v15_t* hdr = reinterpret_cast<ShaderAssetHeader_v15_t*>(pakAsset->header());

		if (pakAsset->data()->HasValidDataPagePointer())
		{
			shaderAsset = new ShaderAsset(hdr, reinterpret_cast<ShaderAssetCPU_t*>(pakAsset->cpu()));
		}
		// there are shaders that have all of the following:
		// - have no data
		// - have an invalid shaderType
		// - have a guid in the place of a pointer in unk_18
		// the guid is for a shader asset, perhaps these are child assets of a shader?
		// todo: get data from these parent shaders and parse
		else // if no cpu data page, manually construct the values that can be taken from the header only
		{
			shaderAsset = new ShaderAsset();

			shaderAsset->name = hdr->name;
			shaderAsset->type = hdr->type;
			shaderAsset->numShaders = 0;
			shaderAsset->data = nullptr;
			shaderAsset->dataSize = 0;
		}

		break;
	}
	default:
	{
		assertm(false, "unaccounted asset version, will cause major issues!");
		return;
	}
	}

	if (shaderAsset->name)
	{
		std::string name = shaderAsset->name;
		if (!name.starts_with("shader/"))
			name = "shader/" + name;

		if (!name.ends_with(".rpak"))
			name += ".rpak";

		pakAsset->SetAssetName(name);
	}

	pakAsset->setExtraData(shaderAsset);
}

// vertex layout flags
#define VLF_POSITION_UNPACKED    (1ull <<  0)
#define VLF_POSITION_PACKED      (1ull <<  1)
#define VLF_COLOR                (1ull <<  4)
#define VLF_NORMAL_UNPACKED      (1ull <<  8)
#define VLF_NORMAL_PACKED        (1ull <<  9)
#define VLF_TANGENT_FLOAT3       (1ull << 10)
#define VLF_TANGENT_FLOAT4       (1ull << 11)
#define VLF_BLENDINDICES         (1ull << 12)
#define VLF_BLENDWEIGHT_UNPACKED (1ull << 13)
#define VLF_BLENDWEIGHT_PACKED	 (1ull << 14)

#define VLF_TEXCOORD_FORMAT_BITS 4
#define VLF_TEXCOORD_FORMAT_MASK ((1 << VLF_TEXCOORD_FORMAT_BITS) - 1)

#define VLF_TEXCOORDn(n)         (VLF_TEXCOORD_FORMAT_MASK << (24ull + (n * VLF_TEXCOORD_FORMAT_BITS)))

#define VLF_TEXCOORD0            VLF_TEXCOORDn(0)


// TEXCOORD FORMATS
// | 0b0000 | UNKNOWN            | 0x00
// | 0b0001 | R32_FLOAT          | 0x29
// | 0b0010 | R32G32_FLOAT       | 0x10
// | 0b0011 | R32G32B32_FLOAT    | 0x06
// | 0b0100 | R32G32B32A32_FLOAT | 0x02
// | 0b0101 | R16G16_SINT        | 0x26
// | 0b0110 | R16G16B16A16_SINT  | 0x0E
// | 0b0111 | R16G16_UINT        | 0x24
// | 0b1000 | R16G16B16A16_UINT  | 0x0C
// | 0b1001 | R32G32_UINT        | 0x11

#define VLF_POSITION_MASK    (VLF_POSITION_UNPACKED    | VLF_POSITION_PACKED   )
#define VLF_NORMAL_MASK      (VLF_NORMAL_UNPACKED      | VLF_NORMAL_PACKED     )
#define VLF_BLENDWEIGHT_MASK (VLF_BLENDWEIGHT_UNPACKED | VLF_BLENDWEIGHT_PACKED)

ID3D11InputLayout* Shader_CreateInputLayoutFromFlags(const uint64_t inputFlags, void* shaderBytecode, size_t shaderSize)
{
	D3D11_INPUT_ELEMENT_DESC inputElements[32] = {};

	int elementIndex = 0;
	int elementOffset = 0;

	if (inputFlags & VLF_POSITION_MASK)
	{
		D3D11_INPUT_ELEMENT_DESC& desc = inputElements[elementIndex++];

		desc.SemanticIndex = 0;
		desc.SemanticName = "POSITION";
		desc.InputSlot = 0;
		desc.InputSlotClass = D3D11_INPUT_PER_VERTEX_DATA;
		desc.AlignedByteOffset = elementOffset;

		desc.Format = (inputFlags & VLF_POSITION_PACKED) ? DXGI_FORMAT_R32G32_UINT : DXGI_FORMAT_R32G32B32_FLOAT;

		elementOffset = (-4 * inputFlags) & 0xC;
	}

	// if there are blendindices and blendweights
	if (inputFlags & VLF_BLENDINDICES)
	{
		if (inputFlags & VLF_BLENDWEIGHT_MASK)
		{
			D3D11_INPUT_ELEMENT_DESC& desc = inputElements[elementIndex++];

			desc.SemanticIndex = 0;
			desc.SemanticName = "BLENDWEIGHT";
			desc.InputSlot = 0;
			desc.InputSlotClass = D3D11_INPUT_PER_VERTEX_DATA;
			desc.AlignedByteOffset = elementOffset;

			desc.Format = (inputFlags & VLF_BLENDWEIGHT_PACKED) ? DXGI_FORMAT_R16G16_SINT : DXGI_FORMAT_R32G32_FLOAT;

			elementOffset += (0x2132100 >> (((inputFlags >> 10) & 0x18) + 2)) & 0xC;
		}

		D3D11_INPUT_ELEMENT_DESC& desc = inputElements[elementIndex++];

		desc.SemanticIndex = 0;
		desc.SemanticName = "BLENDINDICES";
		desc.InputSlot = 0;
		desc.InputSlotClass = D3D11_INPUT_PER_VERTEX_DATA;
		desc.AlignedByteOffset = elementOffset;

		desc.Format = DXGI_FORMAT_R8G8B8A8_UINT;

		elementOffset += (0x2132100 >> (((inputFlags >> 10) & 4) + 2)) & 0xC;
	}

	if (inputFlags & VLF_NORMAL_MASK)
	{
		D3D11_INPUT_ELEMENT_DESC& desc = inputElements[elementIndex++];

		desc.SemanticIndex = 0;
		desc.SemanticName = "NORMAL";
		desc.InputSlot = 0;
		desc.InputSlotClass = D3D11_INPUT_PER_VERTEX_DATA;
		desc.AlignedByteOffset = elementOffset;

		desc.Format = (inputFlags & VLF_NORMAL_PACKED) ? DXGI_FORMAT_R32_UINT : DXGI_FORMAT_R32G32B32_FLOAT;
		elementOffset += (inputFlags & VLF_NORMAL_PACKED) ? 4 : 12;
	}

	if (inputFlags & VLF_COLOR)
	{
		D3D11_INPUT_ELEMENT_DESC& desc = inputElements[elementIndex++];

		desc.SemanticIndex = 0;
		desc.SemanticName = "COLOR";
		desc.InputSlot = 0;
		desc.InputSlotClass = D3D11_INPUT_PER_VERTEX_DATA;
		desc.AlignedByteOffset = elementOffset;

		desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
		elementOffset += 4;
	}

	// dword_14130AF30 dd 0, 29h, 10h, 6, 2, 26h, 0Eh, 24h, 0Ch, 11h

	static DXGI_FORMAT s_texcoordFlags[] = {
		DXGI_FORMAT_UNKNOWN,
		DXGI_FORMAT_R32_FLOAT,          // float
		DXGI_FORMAT_R32G32_FLOAT,       // float2
		DXGI_FORMAT_R32G32B32_FLOAT,    // float3
		DXGI_FORMAT_R32G32B32A32_FLOAT, // float4
		DXGI_FORMAT_R16G16_SINT,
		DXGI_FORMAT_R16G16B16A16_SINT,
		DXGI_FORMAT_R16G16_UINT,
		DXGI_FORMAT_R16G16B16A16_UINT,
		DXGI_FORMAT_R32G32_UINT,
	};

	if (inputFlags & VLF_TEXCOORD0)
	{
		int texCoordIdx = 0;
		int texCoordShift = 24;

		uint64_t inputFlagsShifted = inputFlags >> texCoordShift;
		do
		{
			inputFlagsShifted = inputFlags >> texCoordShift;

			int8_t texCoordFormat = inputFlagsShifted & VLF_TEXCOORD_FORMAT_MASK;

			if (texCoordFormat != 0)
			{
				D3D11_INPUT_ELEMENT_DESC& desc = inputElements[elementIndex++];

				desc.SemanticIndex = texCoordIdx;
				desc.SemanticName = "TEXCOORD";
				desc.InputSlot = 0;
				desc.InputSlotClass = D3D11_INPUT_PER_VERTEX_DATA;
				desc.AlignedByteOffset = elementOffset;

				// for some reason, the game only defines 10 possible formats for the texcoord format array
				// despite the flags supporting 15. this can cause problems as if we don't check the bounds on this, it will run into
				// other static memory
#if defined(ASSERTS)
				assert(texCoordFormat < 10);
#else
				if (texCoordFormat >= 10)
					texCoordFormat = 0;
#endif

				desc.Format = s_texcoordFlags[texCoordFormat];

				elementOffset += (0x48A31A20 >> (3 * texCoordFormat)) & 0x1C;
			}

			texCoordShift += VLF_TEXCOORD_FORMAT_BITS;
			texCoordIdx++;
		} while (inputFlagsShifted >= (1 << VLF_TEXCOORD_FORMAT_BITS)); // while the flag value is large enough that there is more than just one 
	}

	ID3D11InputLayout* inputLayout = nullptr;
	HRESULT hr = g_dxHandler->GetDevice()->CreateInputLayout(inputElements, elementIndex, shaderBytecode, shaderSize, &inputLayout);

	if (FAILED(hr))
	{
		Log("Failed to create input layout for flags %016llX\n", inputFlags);
		return nullptr;
	}
	return inputLayout;
}

void PostLoadShaderAsset(CAssetContainer* const pak, CAsset* const asset)
{
	UNUSED(pak);

	CPakAsset* pakAsset = static_cast<CPakAsset*>(asset);

	// get the parent shader's data once it has been loaded.
	// [rika]: hack need to find actual code for this.
	//if (asset->cpu() == nullptr && asset->version() == 15)
	//{
	//	ShaderAssetHeader_v15_t* const hdr = reinterpret_cast<ShaderAssetHeader_v15_t* const>(asset->header());
	//	const uint64_t parentGuid = reinterpret_cast<uint64_t>(hdr->unk_18);

	//	if (CPakAsset* const parentAsset = g_assetData.FindAssetByGUID(parentGuid))
	//	{
	//		asset->data()->dataPagePtr.ptr = parentAsset->cpu();
	//		ShaderAsset* shaderAssetNew = new ShaderAsset(hdr, reinterpret_cast<ShaderAssetCPU_t*>(parentAsset->cpu()));
	//		const ShaderAsset* const parentAssetHdr = reinterpret_cast<const ShaderAsset* const>(parentAsset->extraData());

	//		shaderAssetNew->numShaders = parentAssetHdr->numShaders;

	//		asset->setExtraData(shaderAssetNew); // memory leak?
	//	}
	//}

	ShaderAsset* const shaderAsset = reinterpret_cast<ShaderAsset*>(pakAsset->extraData());

	if (shaderAsset->numShaders != -1)
	{
		const char* const cpuData = pakAsset->cpu();

		if (pakAsset->version() <= 8)
		{
			// Vertex shaders have an entry size of 24 bytes instead of the regular 16 bytes?
			// Pretty sure this is to fit another pointer (not sure what it does tho)
			const int8_t entrySize = shaderAsset->type == eShaderType::Vertex ? 24 : 16;

			for (int i = 0; i < shaderAsset->numShaders; ++i)
			{
				const char* const entryStart = cpuData + (entrySize * i);
				int bufferSize = *reinterpret_cast<const int*>(entryStart + 8);
				const char* const bufferPointer = *reinterpret_cast<const char* const*>(entryStart);

				if (bufferSize < 0)
					bufferSize = 0;

				const ShaderBufEntry_t bufEntry = { bufferPointer, bufferSize, i, bufferSize == 0, bufferSize == 0, false };

				shaderAsset->shaderBuffers.push_back(bufEntry);
			}
		}
		else
		{
			uint32_t v12 = 0;
			for (int i = 0; i < shaderAsset->numShaders; ++i)
			{
				int bufferLen = *(int*)(cpuData + 8 + (v12 * 8));

				ShaderBufEntry_t bufEntry = { nullptr, bufferLen, i, false, false, false };

				if (bufferLen > 0) // if greater than 0, we have a unique shader buffer
				{
					// create shader
					const char* buffer = *(char**)(cpuData + (v12 * 8));

					if (buffer)
					{
						// At this point, the game creates the shader instance using bytecode ptr.
						bufEntry.buffer = buffer;
					}
					else
					{
						bufEntry.isNullBuffer = true;
					}
				}
				else if (bufferLen < 0) // if this entry is a reference to another entry
				{
					bufEntry.isRef = true;

					// If bytecodeLen is less than 0, this shader entry is a reference to another shader.
					// shdr->shaderInstances[v12] = shdr->shaderInstances[2 * ~bytecodeLen];
					// add ref with "shdr->shaderInstances[v12]->AddRef();" if operator= doesn't catch it.

					//Log("%s shader %i is a ref. %i %i\n", asset->name().c_str(), i, bufferLen, ~bufferLen);
				}
				else
				{
					bufEntry.isZeroLength = true;
				}

				shaderAsset->shaderBuffers.push_back(bufEntry);

				// Vertex shaders use a different number of bytes per buffer compared to other types of shaders
				// so adjust the offset accordingly (3 qwords vs 2 qwords)
				if (shaderAsset->type == eShaderType::Vertex)
					v12 += 3;
				else
					v12 += 2;
			}
		}


		for (auto& it : shaderAsset->shaderBuffers)
		{
			const char* buf = it.buffer;

			if (buf)
			{
				const DXBCHeader* const hdr = reinterpret_cast<const DXBCHeader*>(buf);

				if (!hdr->isValid())
					return;

				for (uint32_t blobIdx = 0; blobIdx < hdr->BlobCount; blobIdx++)
				{
					const DXBCBlobHeader* const blob = hdr->pBlob(blobIdx);

					if (!blob->isRDEF())
						continue;

					RDEFBlobHeader* rdefBlob = blob->pRDEFBlob();

					shaderAsset->compilerStrings.emplace_back(rdefBlob->CompilerSignature());

					break;
				}

			}

		}

	}

#if defined(ADVANCED_MODEL_PREVIEW) // saves some memory and loading time if we don't create these when AMP is not enabled
	HRESULT hr = E_INVALIDARG;

	switch (shaderAsset->type)
	{
	case eShaderType::Pixel:
	{
		hr = g_dxHandler->GetDevice()->CreatePixelShader(shaderAsset->data, shaderAsset->dataSize, NULL, &shaderAsset->pixelShader);
		break;
	}
	case eShaderType::Vertex:
	{
		hr = g_dxHandler->GetDevice()->CreateVertexShader(shaderAsset->data, shaderAsset->dataSize, NULL, &shaderAsset->vertexShader);
		break;
	}
	case eShaderType::Geometry:
	{
		hr = g_dxHandler->GetDevice()->CreateGeometryShader(shaderAsset->data, shaderAsset->dataSize, NULL, &shaderAsset->geometryShader);
		break;
	}
	case eShaderType::Hull:
	{
		hr = g_dxHandler->GetDevice()->CreateHullShader(shaderAsset->data, shaderAsset->dataSize, NULL, &shaderAsset->hullShader);
		break;
	}
	case eShaderType::Domain:
	{
		hr = g_dxHandler->GetDevice()->CreateDomainShader(shaderAsset->data, shaderAsset->dataSize, NULL, &shaderAsset->domainShader);
		break;
	}
	case eShaderType::Compute:
	{
		hr = g_dxHandler->GetDevice()->CreateComputeShader(shaderAsset->data, shaderAsset->dataSize, NULL, &shaderAsset->computeShader);
		break;
	}
	}

	if(shaderAsset->type == eShaderType::Vertex)
		shaderAsset->vertexInputLayout = Shader_CreateInputLayoutFromFlags(shaderAsset->inputFlags[0], shaderAsset->data, shaderAsset->dataSize);

	if (FAILED(hr))
	{
		Log("failed to create %s shader for asset %s (0x%08X)\n", GetShaderTypeName(shaderAsset->type), asset->name().c_str(), hr);
	}
#endif
}

void* PreviewShaderAsset(CAsset* const asset, const bool firstFrameForAsset)
{
	UNUSED(firstFrameForAsset);

	CPakAsset* pakAsset = static_cast<CPakAsset*>(asset);

	const ShaderAsset* const shaderAsset = reinterpret_cast<ShaderAsset*>(pakAsset->extraData());
	assertm(shaderAsset, "Extra data should be valid at this point.");

	ImGui::Text("Features: %016X", *(uint64_t*)shaderAsset->shaderFeatures);

	//for (auto& it : shaderAsset->compilerStrings)
	//{
	//	ImGui::TextUnformatted(it.c_str());
	//}

	if (nullptr != shaderAsset->inputFlags)
	{
		std::string inputFlagsStr = "[\n";

		// 2 per shader for VS

		const int numFlagsPerShader = shaderAsset->type == eShaderType::Vertex ? 2 : 1;

		for (int i = 0; i < numFlagsPerShader * shaderAsset->numShaders; ++i)
		{
			const int index = (3 - numFlagsPerShader) * i;
			const int64_t inputFlags = shaderAsset->inputFlags[index];

			inputFlagsStr += std::format("\t\"0x{:X}\"", inputFlags);

			if (i + 1 != numFlagsPerShader * shaderAsset->numShaders)
				inputFlagsStr += ",";

			inputFlagsStr += "\n";
		}
		inputFlagsStr += "]";

		// yes i know the const_cast is bad, but the input is ReadOnly so it shouldn't be an issue
		ImGui::InputTextMultiline("Shader Input Flags", const_cast<char*>(inputFlagsStr.c_str()), inputFlagsStr.length(), ImVec2(0, 800), ImGuiInputTextFlags_ReadOnly);
	}


	return nullptr;
}

enum eShaderAssetExportSetting
{
	Raw,
	MSW, // MultiShaderWrapper
};

static void ExportShaderMetaData(const ShaderAsset* const shaderAsset, std::filesystem::path& exportPath)
{
	exportPath.replace_extension(".json");
	std::ofstream ofs(exportPath, std::ios::out);

	ofs << "{\n";

	ofs << "\t\"type\": \"" << s_dxShaderTypeNames[static_cast<int>(shaderAsset->type)] << "\",\n";

	if (shaderAsset->name)
		ofs << "\t\"name\": \"" << shaderAsset->name << "\",\n";

	// Mask out the last byte as the shader only has 7 bytes dedicated to it.
	ofs << "\t\"features\": \"" << std::uppercase << std::hex << ((*(uint64_t*)shaderAsset->shaderFeatures) & 0x00FFFFFFFFFFFFFF) << "\",\n";

	ofs << "\t\"inputFlags\": [\n";
	const size_t shaderBufferCount = shaderAsset->shaderBuffers.size();

	// Copy shader features from the shader asset to the MSW instance
	for (size_t i = 0, flagIdx = 0; i < shaderBufferCount; i++, flagIdx+=2)
	{

		const uint64_t inputFlags1 = shaderAsset->inputFlags[flagIdx];
		const uint64_t inputFlags2 = shaderAsset->inputFlags[flagIdx + 1];

		const char* const commaChar = i != (shaderBufferCount - 1) ? "," : "";

		ofs << "\t\t\"0x" << std::uppercase << std::hex << inputFlags1 << "\",\n";
		ofs << "\t\t\"0x" << std::uppercase << std::hex << inputFlags2 << "\"" << commaChar << "\n";
	}

	ofs << "\t],\n";
	ofs << "\t\"refIndices\": [\n";

	size_t i = 0;

	for (auto& buf : shaderAsset->shaderBuffers)
	{
		const uint16_t index = buf.isRef ? ~static_cast<uint16_t>(buf.bufferSize) : UINT16_MAX;

		const char* const commaChar = i != (shaderBufferCount - 1) ? "," : "";
		ofs << "\t\t" << std::dec << index << commaChar << "\n";

		i++;
	}

	ofs << "\t]\n";
	ofs << "}\n";
}

bool ExportRawShaderAsset(const ShaderAsset* const shaderAsset, std::filesystem::path& exportPath)
{
	ExportShaderMetaData(shaderAsset, exportPath);

	const std::string fileStem = exportPath.stem().string();
	const char* const fileStemString = fileStem.c_str();

	for (auto& buf : shaderAsset->shaderBuffers)
	{
		if (buf.buffer && buf.bufferSize > 0)
		{
			exportPath.replace_filename(std::format("{}_{}.fxc", fileStemString, buf.shaderIdx));

			StreamIO out(exportPath, eStreamIOMode::Write);
			out.write(buf.buffer, buf.bufferSize);
		}
	}

	return true;
}

#include <core/shaderexp/multishader.h>

void ConstructMSWShader(CMultiShaderWrapperIO::Shader_t& shader, const ShaderAsset* const shaderAsset)
{
	// Copy shader features from the shader asset to the MSW instance
	size_t i = 0;
	for (auto& buf : shaderAsset->shaderBuffers)
	{
		//Log("%i = %p %i\n", i, buf.buffer, buf.bufferSize);

		const uint64_t inputFlags1 = shaderAsset->inputFlags[i];
		const uint64_t inputFlags2 = shaderAsset->inputFlags[i + 1];

		CMultiShaderWrapperIO::ShaderEntry_t& entry = shader.entries.emplace_back();

		entry.size = static_cast<uint32_t>(buf.bufferSize);
		entry.deleteBuffer = false;
		entry.flags[0] = inputFlags1;
		entry.flags[1] = inputFlags2;

		if (buf.isRef)
		{
			entry.buffer = NULL;
			entry.refIndex = ~static_cast<uint16_t>(buf.bufferSize);
		}
		else
		{
			entry.buffer = buf.buffer;
			entry.refIndex = UINT16_MAX;
		}

		i+=2;
	}

	if (shaderAsset->name)
		shader.name = shaderAsset->name;

	shader.shaderType = static_cast<MultiShaderWrapperShaderType_e>(shaderAsset->type);
	memcpy_s(shader.features, sizeof(shader.features), shaderAsset->shaderFeatures, sizeof(shader.features));
}

bool ExportMSWShaderAsset(const ShaderAsset* const shaderAsset, std::filesystem::path& exportPath)
{
	exportPath.replace_extension(".msw");

	CMultiShaderWrapperIO writer = {};
	writer.SetFileType(MultiShaderWrapperFileType_e::SHADER);

	CMultiShaderWrapperIO::Shader_t shader;
	ConstructMSWShader(shader, shaderAsset);

	writer.SetShader(&shader);
	return writer.WriteFile(exportPath.string().c_str());
}

static const char* const s_PathPrefixSHDR = s_AssetTypePaths.find(AssetType_t::SHDR)->second;
bool ExportShaderAsset(CAsset* const asset, const int setting)
{
	CPakAsset* pakAsset = static_cast<CPakAsset*>(asset);

	const ShaderAsset* const shaderAsset = reinterpret_cast<ShaderAsset*>(pakAsset->extraData());
	assertm(shaderAsset, "Extra asset data should be valid at this point.");

	// shaders with no data/invalid type need to be skipped until we properly handle them
	if (shaderAsset->type >= eShaderType::Invalid)
	{
		Log("Tried to export %s with invalid shader type, skipping...\n");
		return false;
	}

	// Create exported path + asset path.
	std::filesystem::path exportPath = std::filesystem::current_path().append(EXPORT_DIRECTORY_NAME);
	const std::filesystem::path shaderPath(asset->GetAssetName());

	if (g_ExportSettings.exportPathsFull)
		exportPath.append(shaderPath.parent_path().string());
	else
		exportPath.append(s_PathPrefixSHDR);

	if (!CreateDirectories(exportPath))
	{
		assertm(false, "Failed to create asset type directory.");
		return false;
	}

	exportPath.append(shaderPath.filename().string());

	switch (setting)
	{
	case eShaderAssetExportSetting::Raw:
	{
		// NOTE: this func changes the value of exportPath!!
		return ExportRawShaderAsset(shaderAsset, exportPath);
	}
	case eShaderAssetExportSetting::MSW:
	{
		// NOTE: this func changes the value of exportPath!!
		return ExportMSWShaderAsset(shaderAsset, exportPath);
	}
	default:
	{
		assertm(false, "Export setting is not handled.");
		return false;
	}
	}

	unreachable();
}

void InitShaderAssetType()
{
	static const char* settings[] = { "Raw", "MSW" };
	AssetTypeBinding_t type =
	{
		.type = 'rdhs',
		.headerAlignment = 8,
		.loadFunc = LoadShaderAsset,
		.postLoadFunc = PostLoadShaderAsset,
		.previewFunc = PreviewShaderAsset,
		.e = { ExportShaderAsset, 0, settings, ARRSIZE(settings) },
	};

	REGISTER_TYPE(type);
}

std::map<uint32_t, ShaderResource> ResourceBindingFromDXBlob(CPakAsset* const asset, D3D_SHADER_INPUT_TYPE inputType)
{
	const ShaderAsset* const shaderAsset = reinterpret_cast<ShaderAsset*>(asset->extraData());
	assertm(shaderAsset, "Extra asset data should be valid at this point.");

	std::map<uint32_t, ShaderResource> bindings;

	if (!shaderAsset->data)
		return bindings;

	const DXBCHeader* const hdr = reinterpret_cast<DXBCHeader*>(shaderAsset->data);

	if (!hdr->isValid())
		return bindings;

	for (uint32_t blobIdx = 0; blobIdx < hdr->BlobCount; blobIdx++)
	{
		const DXBCBlobHeader* const blob = hdr->pBlob(blobIdx);

		if (!blob->isRDEF())
			continue;

		RDEFBlobHeader* rdefBlob = blob->pRDEFBlob();
		for (uint32_t resIdx = 0; resIdx < rdefBlob->BoundResourceCount; resIdx++)
		{
			RDEFResourceBinding* resource = rdefBlob->pBoundResource(resIdx);
			if (resource->Type != inputType)
				continue;

			const ShaderResource tmp(resource->Name(rdefBlob), *resource);
			bindings.emplace(resource->BindPoint, tmp);
		}

		break;
	}

	return bindings;
}

std::vector<TmpConstBufVar> ConstBufVarFromDXBlob(CPakAsset* const asset, const char* constBufName)
{
	const ShaderAsset* const shaderAsset = reinterpret_cast<ShaderAsset*>(asset->extraData());
	assertm(shaderAsset, "Extra asset data should be valid at this point.");

	std::vector<TmpConstBufVar> vars;

	if (!shaderAsset->data)
		return vars;

	const DXBCHeader* const hdr = reinterpret_cast<DXBCHeader*>(shaderAsset->data);

	if (!hdr->isValid())
		return vars;

	for (uint32_t blobIdx = 0; blobIdx < hdr->BlobCount; blobIdx++)
	{
		const DXBCBlobHeader* const blob = hdr->pBlob(blobIdx);

		if (!blob->isRDEF())
			continue;

		RDEFBlobHeader* rdefBlob = blob->pRDEFBlob();
		for (uint32_t constBufIdx = 0; constBufIdx < rdefBlob->ConstBufferCount; constBufIdx++)
		{
			const RDefConstBuffer* const constBuf = rdefBlob->pConstBuffer(constBufIdx);
			if (strncmp(constBufName, constBuf->Name(rdefBlob), 64))
				continue;

			for (uint32_t constIdx = 0; constIdx < constBuf->ConstCount; constIdx++)
			{
				const RDEFConst* const constVar = constBuf->pConst(rdefBlob, constIdx);
				const RDEFType* const constType = constVar->pType(rdefBlob);

				const TmpConstBufVar tmp(constVar->Name(rdefBlob), static_cast<D3D_SHADER_VARIABLE_TYPE>(constType->Type), constVar->Size);
				vars.push_back(tmp);
			}

			break;
		}

		break;
	}

	return vars;
}