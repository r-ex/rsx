#include <pch.h>
#include <game/rtech/assets/shader.h>
#include <game/rtech/cpakfile.h>
#include <game/rtech/utils/utils.h>

#include <imgui.h>
#include <dxcapi.h>
#include <d3d12shader.h>

extern CDXParentHandler* g_dxHandler;

extern ExportSettings_t g_ExportSettings;

#define DXBC_FOURCC_DXIL    (('L'<<24)+('I'<<16)+('X'<<8)+'D')

static HMODULE LoadDxCompilerDll()
{
	static HMODULE dxCompiler = nullptr;
	static bool didLoad = false;

	if (didLoad)
		return dxCompiler;

	didLoad = true;
	dxCompiler = LoadLibraryA("dxcompiler.dll");

	return dxCompiler;
}

static bool IsValidDXBCContainer(const char* const shaderData, const int shaderDataSize)
{
	if (!shaderData || shaderDataSize < static_cast<int>(sizeof(DXBCHeader)))
		return false;

	const DXBCHeader* const hdr = reinterpret_cast<const DXBCHeader*>(shaderData);
	if (!hdr->isValid() || hdr->ContainerSizeInBytes > static_cast<uint32_t>(shaderDataSize))
		return false;

	const uint64_t blobOffsetTableSize = sizeof(DXBCHeader) + (static_cast<uint64_t>(hdr->BlobCount) * sizeof(uint32_t));
	if (blobOffsetTableSize > hdr->ContainerSizeInBytes)
		return false;

	for (uint32_t blobIdx = 0; blobIdx < hdr->BlobCount; blobIdx++)
	{
		const uint32_t blobOffset = hdr->BlobOffset(blobIdx);
		if (blobOffset > hdr->ContainerSizeInBytes || hdr->ContainerSizeInBytes - blobOffset < sizeof(DXBCBlobHeader))
			return false;

		const DXBCBlobHeader* const blob = hdr->pBlob(blobIdx);
		if (blob->BlobSize > hdr->ContainerSizeInBytes - blobOffset - sizeof(DXBCBlobHeader))
			return false;
	}

	return true;
}

static ID3D12ShaderReflection* CreateDXILShaderReflection(const char* const shaderData, const int shaderDataSize)
{
	if (!IsValidDXBCContainer(shaderData, shaderDataSize))
		return nullptr;

	const DXBCHeader* const hdr = reinterpret_cast<const DXBCHeader*>(shaderData);

	bool hasDXIL = false;
	for (uint32_t blobIdx = 0; blobIdx < hdr->BlobCount; blobIdx++)
	{
		const DXBCBlobHeader* const blob = hdr->pBlob(blobIdx);
		if (blob->BlobFourCC == DXBC_FOURCC_DXIL)
		{
			hasDXIL = true;
			break;
		}
	}

	if (!hasDXIL)
		return nullptr;

	const HMODULE dxCompiler = LoadDxCompilerDll();
	if (!dxCompiler)
		return nullptr;

	const DxcCreateInstanceProc dxcCreateInstance = reinterpret_cast<DxcCreateInstanceProc>(GetProcAddress(dxCompiler, "DxcCreateInstance"));
	if (!dxcCreateInstance)
		return nullptr;

	IDxcUtils* utils = nullptr;
	if (FAILED(dxcCreateInstance(CLSID_DxcUtils, IID_PPV_ARGS(&utils))) || !utils)
		return nullptr;

	IDxcBlobEncoding* blob = nullptr;
	HRESULT hr = utils->CreateBlobFromPinned(shaderData, hdr->ContainerSizeInBytes, CP_ACP, &blob);
	utils->Release();

	if (FAILED(hr) || !blob)
		return nullptr;

	IDxcContainerReflection* containerReflection = nullptr;
	hr = dxcCreateInstance(CLSID_DxcContainerReflection, IID_PPV_ARGS(&containerReflection));
	if (FAILED(hr) || !containerReflection)
	{
		blob->Release();
		return nullptr;
	}

	hr = containerReflection->Load(blob);
	blob->Release();

	if (FAILED(hr))
	{
		containerReflection->Release();
		return nullptr;
	}

	UINT32 dxilPart = 0;
	hr = containerReflection->FindFirstPartKind(DXBC_FOURCC_DXIL, &dxilPart);
	if (FAILED(hr))
	{
		containerReflection->Release();
		return nullptr;
	}

	ID3D12ShaderReflection* shaderReflection = nullptr;
	hr = containerReflection->GetPartReflection(dxilPart, IID_PPV_ARGS(&shaderReflection));
	containerReflection->Release();

	if (FAILED(hr))
		return nullptr;

	return shaderReflection;
}

static uint32_t DXILShaderTypePackedSize(const D3D12_SHADER_TYPE_DESC& typeDesc)
{
	uint32_t elementSize = 0;
	switch (typeDesc.Type)
	{
	case D3D_SVT_BOOL:
	case D3D_SVT_INT:
	case D3D_SVT_UINT:
	case D3D_SVT_FLOAT:
		elementSize = 4;
		break;
	case D3D_SVT_DOUBLE:
	case D3D_SVT_INT64:
	case D3D_SVT_UINT64:
		elementSize = 8;
		break;
	case D3D_SVT_INT16:
	case D3D_SVT_UINT16:
	case D3D_SVT_FLOAT16:
		elementSize = 2;
		break;
	case D3D_SVT_UINT8:
		elementSize = 1;
		break;
	default:
		return 0;
	}

	uint32_t count = 1;
	switch (typeDesc.Class)
	{
	case D3D_SVC_SCALAR:
		break;
	case D3D_SVC_VECTOR:
		count = std::max<UINT>(typeDesc.Columns, 1);
		break;
	case D3D_SVC_MATRIX_ROWS:
	case D3D_SVC_MATRIX_COLUMNS:
		count = std::max<UINT>(typeDesc.Rows, 1) * std::max<UINT>(typeDesc.Columns, 1);
		break;
	default:
		return 0;
	}

	if (typeDesc.Elements)
		count *= typeDesc.Elements;

	return elementSize * count;
}

static std::string DXILStructTypeName(const char* const name)
{
	if (!name)
		return "UnnamedStruct";

	std::string typeName(name);
	if (typeName.starts_with("struct."))
		typeName.erase(0, 7);

	for (char& c : typeName)
	{
		if (!isalnum(static_cast<unsigned char>(c)) && c != '_')
			c = '_';
	}

	return typeName.empty() ? "UnnamedStruct" : typeName;
}

static TmpConstBufVar DXILConstBufVarFromType(ID3D12ShaderReflectionType* const type, const char* const name, const uint32_t offset, const uint32_t fallbackSize)
{
	if (!type)
		return TmpConstBufVar(name, D3D_SVT_VOID, 0, offset);

	D3D12_SHADER_TYPE_DESC typeDesc{};
	if (FAILED(type->GetDesc(&typeDesc)))
		return TmpConstBufVar(name, D3D_SVT_VOID, 0, offset);

	if (typeDesc.Class == D3D_SVC_STRUCT && typeDesc.Members)
	{
		TmpConstBufVar structVar(name, D3D_SVT_VOID, fallbackSize, offset);
		structVar.structTypeName = DXILStructTypeName(typeDesc.Name);

		for (UINT memberIdx = 0; memberIdx < typeDesc.Members; memberIdx++)
		{
			ID3D12ShaderReflectionType* const memberType = type->GetMemberTypeByIndex(memberIdx);
			if (!memberType)
				continue;

			D3D12_SHADER_TYPE_DESC memberTypeDesc{};
			if (FAILED(memberType->GetDesc(&memberTypeDesc)))
				continue;

			uint32_t memberFallbackSize = 0;
			if (fallbackSize && memberTypeDesc.Offset < fallbackSize)
			{
				uint32_t memberEndOffset = fallbackSize;
				if (memberIdx + 1 < typeDesc.Members)
				{
					ID3D12ShaderReflectionType* const nextMemberType = type->GetMemberTypeByIndex(memberIdx + 1);
					D3D12_SHADER_TYPE_DESC nextMemberTypeDesc{};
					if (nextMemberType && SUCCEEDED(nextMemberType->GetDesc(&nextMemberTypeDesc)) && nextMemberTypeDesc.Offset > memberTypeDesc.Offset)
						memberEndOffset = nextMemberTypeDesc.Offset;
				}

				memberFallbackSize = memberEndOffset - memberTypeDesc.Offset;
			}

			const char* const memberName = type->GetMemberTypeName(memberIdx);
			structVar.members.emplace_back(DXILConstBufVarFromType(memberType, memberName, offset + memberTypeDesc.Offset, memberFallbackSize));
		}

		return structVar;
	}

	const uint32_t reflectedPackedSize = DXILShaderTypePackedSize(typeDesc);
	const uint32_t layoutSize = fallbackSize ? fallbackSize : reflectedPackedSize;
	if (!layoutSize)
		return TmpConstBufVar(name, typeDesc.Type, 0, offset);

	TmpConstBufVar var(std::string(name ? name : ""), typeDesc.Type, layoutSize, offset);
	var.packedSize = reflectedPackedSize ? reflectedPackedSize : layoutSize;
	return var;
}

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
	case 16:
	case 17:
	{
		// [rika]: switch headerStructSize == 48, switch shaders in general are very odd
		assertm(pakAsset->data()->headerStructSize == sizeof(ShaderAssetHeader_v15_t), "incorrect header");

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
	case 19: // man idfk
	{
		// [rika]: there's some where shaders in newer versions that don't have cpu data, and point to places that don't really have a shader header
		ShaderAssetHeader_v14_t* hdr = reinterpret_cast<ShaderAssetHeader_v14_t*>(pakAsset->header());

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

		pakAsset->SetAssetName(name, true);
	} else
		pakAsset->SetAssetNameFromCache();

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

	//if (asset->GetAssetVersion().majorVer == 19)
	//	return;

	CPakAsset* pakAsset = static_cast<CPakAsset*>(asset);

	// get the parent shader's data once it has been loaded. (if done in LoadShaderAsset and parent is in another pak, we will encounter issues.)
	// [rika]: hack need to find actual code for this.
#ifdef SHADER_CHILD_HACK
	if (pakAsset->cpu() == nullptr && pakAsset->version() >= 15)
	{
		ShaderAssetHeader_v15_t* const hdr = reinterpret_cast<ShaderAssetHeader_v15_t* const>(pakAsset->header());

		assertm(hdr->type >= eShaderType::Invalid, "bad type, should have data ??");

		const uint64_t guid = reinterpret_cast<uint64_t>(hdr->unk_18); // [rika]: this is not cool

		CPakAsset* const parentAsset = g_assetData.FindAssetByGUID<CPakAsset>(guid);
		if (parentAsset)
		{
			ShaderAssetHeader_v15_t tmp;
			memcpy(&tmp, parentAsset->header(), sizeof(ShaderAssetHeader_v15_t));
			tmp.name = hdr->name;

			ShaderAsset* assetNew = new ShaderAsset(&tmp, reinterpret_cast<ShaderAssetCPU_t*>(parentAsset->cpu())); // does this leak memory?
			pakAsset->setExtraData(assetNew);

			pakAsset->data()->dataPagePtr.ptr = parentAsset->cpu(); // new cpu new you (gets used later)
		}
	}
#endif

	ShaderAsset* const shaderAsset = pakAsset->extraData<ShaderAsset* const>();

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

	if(shaderAsset->type == eShaderType::Vertex && shaderAsset->inputFlags)
		shaderAsset->vertexInputLayout = Shader_CreateInputLayoutFromFlags(shaderAsset->inputFlags[0], shaderAsset->data, shaderAsset->dataSize);

	if (FAILED(hr))
	{
		Log("failed to create %s shader for asset %s (0x%08X)\n", GetShaderTypeName(shaderAsset->type), asset->GetAssetName().c_str(), hr);
	}
#endif
}

void* PreviewShaderAsset(CAsset* const asset, const bool firstFrameForAsset)
{
	UNUSED(firstFrameForAsset);

	CPakAsset* pakAsset = static_cast<CPakAsset*>(asset);

	const ShaderAsset* const shaderAsset = pakAsset->extraData<const ShaderAsset* const>();

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
		ImGui::InputTextMultiline("Shader Input Flags", const_cast<char*>(inputFlagsStr.c_str()), inputFlagsStr.length()+1, ImVec2(0, 800), ImGuiInputTextFlags_ReadOnly);
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
		const char* const commaChar = i != (shaderBufferCount - 1) ? "," : "";

		// [rika]: no input flags, don't write them
		if (!shaderAsset->inputFlags)
		{
			ofs << "\t\t\"0x0" << "\",\n";
			ofs << "\t\t\"0x0" << "\"" << commaChar << "\n";

			continue;
		}

		const uint64_t inputFlags1 = shaderAsset->inputFlags[flagIdx];
		const uint64_t inputFlags2 = shaderAsset->inputFlags[flagIdx + 1];

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
	// [rika]: I'm not doing this but MSW needs to handle shaders without shader input flags
	assertm(shaderAsset->inputFlags, "shader did not have valid input flags");
	if (!shaderAsset->inputFlags)
		return;

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

	const ShaderAsset* const shaderAsset = pakAsset->extraData<const ShaderAsset* const>();

	if (!shaderAsset) return false;

	// shaders with no data/invalid type need to be skipped until we properly handle them
	if (shaderAsset->type >= eShaderType::Invalid)
	{
		Log("SHDR: Tried to export %s with invalid shader type, skipping...\n", asset->GetAssetName().c_str());
		return false;
	}

	// Create exported path + asset path.
	std::filesystem::path exportPath = g_ExportSettings.GetExportDirectory();
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

std::map<uint32_t, ShaderResource> ResourceBindingFromDXBlob(CPakAsset* const asset, D3D_SHADER_INPUT_TYPE inputType)
{
	const ShaderAsset* const shaderAsset = asset->extraData<const ShaderAsset* const>();

	std::map<uint32_t, ShaderResource> bindings;

	if (!shaderAsset || !shaderAsset->data || !IsValidDXBCContainer(shaderAsset->data, shaderAsset->dataSize))
		return bindings;

	const DXBCHeader* const hdr = reinterpret_cast<DXBCHeader*>(shaderAsset->data);

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

	if (bindings.empty())
	{
		ID3D12ShaderReflection* const reflection = CreateDXILShaderReflection(shaderAsset->data, shaderAsset->dataSize);
		if (!reflection)
			return bindings;

		D3D12_SHADER_DESC shaderDesc{};
		if (SUCCEEDED(reflection->GetDesc(&shaderDesc)))
		{
			for (UINT resIdx = 0; resIdx < shaderDesc.BoundResources; resIdx++)
			{
				D3D12_SHADER_INPUT_BIND_DESC resource{};
				if (FAILED(reflection->GetResourceBindingDesc(resIdx, &resource)) || resource.Type != inputType)
					continue;

				RDEFResourceBinding binding{};
				binding.Type = resource.Type;
				binding.ReturnType = resource.ReturnType;
				binding.Dimension = static_cast<D3D10_SRV_DIMENSION>(resource.Dimension);
				binding.NumSamples = resource.NumSamples;
				binding.BindPoint = resource.BindPoint;
				binding.BindCount = resource.BindCount;
				binding.Flags = static_cast<D3D_SHADER_INPUT_FLAGS>(resource.uFlags);

				const ShaderResource tmp(std::string(resource.Name ? resource.Name : ""), binding);
				bindings.emplace(resource.BindPoint, tmp);
			}
		}

		reflection->Release();
	}

	return bindings;
}

std::vector<TmpConstBufVar> ConstBufVarFromDXBlob(CPakAsset* const asset, const char* constBufName, uint32_t expectedConstBufSize)
{
	const ShaderAsset* const shaderAsset = asset->extraData<const ShaderAsset* const>();
	assertm(shaderAsset, "Extra asset data should be valid at this point.");

	std::vector<TmpConstBufVar> vars;

	if (!shaderAsset->data || !IsValidDXBCContainer(shaderAsset->data, shaderAsset->dataSize))
		return vars;

	const DXBCHeader* const hdr = reinterpret_cast<DXBCHeader*>(shaderAsset->data);

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

			if (expectedConstBufSize && constBuf->ConstBufSize > expectedConstBufSize)
			{
				Log("SHDR: Skipping RDEF constant buffer '%s' for %s, reflected size %u exceeds expected %u\n",
					constBuf->Name(rdefBlob), asset->GetAssetName().c_str(), constBuf->ConstBufSize, expectedConstBufSize);
				continue;
			}

			for (uint32_t constIdx = 0; constIdx < constBuf->ConstCount; constIdx++)
			{
				const RDEFConst* const constVar = constBuf->pConst(rdefBlob, constIdx);
				const RDEFType* const constType = constVar->pType(rdefBlob);

				const TmpConstBufVar tmp(constVar->Name(rdefBlob), static_cast<D3D_SHADER_VARIABLE_TYPE>(constType->Type), constVar->Size, constVar->StartOffset);
				vars.push_back(tmp);
			}

			break;
		}

		break;
	}

	if (vars.empty())
	{
		ID3D12ShaderReflection* const reflection = CreateDXILShaderReflection(shaderAsset->data, shaderAsset->dataSize);
		if (!reflection)
			return vars;

		D3D12_SHADER_DESC shaderDesc{};
		const HRESULT descHr = reflection->GetDesc(&shaderDesc);

		ID3D12ShaderReflectionConstantBuffer* constBuf = nullptr;
		D3D12_SHADER_BUFFER_DESC constBufDesc{};

		if (SUCCEEDED(descHr))
		{
			for (UINT constBufIdx = 0; constBufIdx < shaderDesc.ConstantBuffers; constBufIdx++)
			{
				ID3D12ShaderReflectionConstantBuffer* const reflectedConstBuf = reflection->GetConstantBufferByIndex(constBufIdx);
				if (!reflectedConstBuf)
					continue;

				D3D12_SHADER_BUFFER_DESC reflectedConstBufDesc{};
				if (FAILED(reflectedConstBuf->GetDesc(&reflectedConstBufDesc)) || !reflectedConstBufDesc.Name || strncmp(constBufName, reflectedConstBufDesc.Name, 64))
					continue;

				if (expectedConstBufSize && reflectedConstBufDesc.Size > expectedConstBufSize)
				{
					Log("SHDR: Skipping DXIL constant buffer '%s' for %s, reflected size %u exceeds expected %u\n",
						reflectedConstBufDesc.Name ? reflectedConstBufDesc.Name : "<null>",
						asset->GetAssetName().c_str(), reflectedConstBufDesc.Size, expectedConstBufSize);
					continue;
				}

				constBuf = reflectedConstBuf;
				constBufDesc = reflectedConstBufDesc;
				break;
			}
		}

		if (constBuf)
		{
			TmpConstBufVar root(constBufDesc.Name ? constBufDesc.Name : constBufName, D3D_SVT_VOID, constBufDesc.Size, 0);
			root.structTypeName = DXILStructTypeName(constBufDesc.Name);

			for (UINT constIdx = 0; constIdx < constBufDesc.Variables; constIdx++)
			{
				ID3D12ShaderReflectionVariable* const constVar = constBuf->GetVariableByIndex(constIdx);
				if (!constVar)
					continue;

				D3D12_SHADER_VARIABLE_DESC constVarDesc{};
				if (FAILED(constVar->GetDesc(&constVarDesc)))
					continue;

				ID3D12ShaderReflectionType* const constType = constVar->GetType();
				if (!constType)
					continue;

				root.members.emplace_back(DXILConstBufVarFromType(constType, constVarDesc.Name, constVarDesc.StartOffset, constVarDesc.Size));
			}

			if (!root.members.empty())
				vars.push_back(std::move(root));
		}

		reflection->Release();
	}

	return vars;
}

void InitShaderAssetType()
{
	static const char* settings[] = { "Raw", "MSW" };
	AssetTypeBinding_t type =
	{
		.name = "Shader",
		.type = 'rdhs',
		.headerAlignment = 8,
		.loadFunc = LoadShaderAsset,
		.postLoadFunc = PostLoadShaderAsset,
		.previewFunc = PreviewShaderAsset,
		.e = { ExportShaderAsset, 0, settings, ARRSIZE(settings) },
	};

	REGISTER_TYPE(type);
}
