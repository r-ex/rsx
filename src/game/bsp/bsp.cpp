#include <pch.h>
#include "bsp.h"
#include <game/rtech/utils/bsp/lumps.h>
#include <game/rtech/cpakfile.h>
#include <core/render/dx.h>
#include <game/rtech/assets/material.h>
#include <game/rtech/assets/texture.h>

extern CDXParentHandler* g_dxHandler;
extern std::unique_ptr<char[]> GetWrapAssetData(CAsset* const asset, uint64_t* outSize);

void GetShadersForVertexLump(int vertexType, CShader** vertexShaderOut, CShader** pixelShaderOut)
{
	CShader* vertexShader = nullptr;
	CShader* pixelShader = nullptr;
	D3D11_INPUT_ELEMENT_DESC* inputElements = nullptr;
	UINT numElements = 0;

	switch (vertexType)
	{
	case MESH_VERTEX_LIT_FLAT:
	{
		// float3 pos
		// float3 normal
		// float2 uv
		// int unk

		numElements = 4;

		inputElements = new D3D11_INPUT_ELEMENT_DESC[numElements];

		inputElements[0] = { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0,                            0, D3D11_INPUT_PER_VERTEX_DATA, 0 };
		inputElements[1] = { "NORMAL",   0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 };
		inputElements[2] = { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT,    0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 };
		inputElements[3] = { "UNK",      0, DXGI_FORMAT_R32_UINT,        0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 };

		vertexShader = g_dxHandler->GetShaderManager()->LoadShader("shaders/vertexLitFlat_vs", eShaderType::Vertex, false);
		pixelShader = g_dxHandler->GetShaderManager()->LoadShader("shaders/bsp_ps", eShaderType::Pixel, false);

		break;
	}
	case MESH_VERTEX_LIT_BUMP:
	{
		// float3 pos
		// float3 normal
		// float2 uv
		// int unk_negativeOne
		// float2 lmapUV
		// uint colour

		// pos
		// nml
		// uv
		// lmapUV
		// colour
		// unk

		numElements = 6;

		inputElements = new D3D11_INPUT_ELEMENT_DESC[numElements];

		inputElements[0] = { "POSITION",  0, DXGI_FORMAT_R32G32B32_FLOAT, 0,                            0, D3D11_INPUT_PER_VERTEX_DATA, 0 };
		inputElements[1] = { "NORMAL",    0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 };
		inputElements[2] = { "TEXCOORD",  0, DXGI_FORMAT_R32G32_FLOAT,    0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 };
		inputElements[3] = { "COLOR",     0, DXGI_FORMAT_R32_UINT,        0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 };
		inputElements[4] = { "TEXCOORD",  1, DXGI_FORMAT_R32G32_FLOAT,    0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 };
		inputElements[5] = { "UNK",       0, DXGI_FORMAT_R32_UINT,        0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 };

		vertexShader = g_dxHandler->GetShaderManager()->LoadShader("shaders/vertexLitBump_vs", eShaderType::Vertex, false);
		pixelShader = g_dxHandler->GetShaderManager()->LoadShader("shaders/bsp_ps", eShaderType::Pixel, false);

		break;
	}
	case MESH_VERTEX_UNLIT:
	{
		// float3 pos
		// float3 normal
		// float2 uv
		// int unk

		numElements = 4;

		inputElements = new D3D11_INPUT_ELEMENT_DESC[numElements];

		inputElements[0] = { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0,                            0, D3D11_INPUT_PER_VERTEX_DATA, 0 };
		inputElements[1] = { "NORMAL",   0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 };
		inputElements[2] = { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT,    0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 };
		inputElements[3] = { "UNK",      0, DXGI_FORMAT_R32_UINT,        0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 };

		vertexShader = g_dxHandler->GetShaderManager()->LoadShader("shaders/vertexLitFlat_vs", eShaderType::Vertex, false);
		pixelShader = g_dxHandler->GetShaderManager()->LoadShader("shaders/bsp_ps", eShaderType::Pixel, false);

		break;
	}
	case MESH_VERTEX_UNLIT_TS:
	{
		// float3 pos
		// float3 normal
		// float2 uv
		// int2 unk

		numElements = 4;

		inputElements = new D3D11_INPUT_ELEMENT_DESC[numElements];

		inputElements[0] = { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT,    0,                            0, D3D11_INPUT_PER_VERTEX_DATA, 0 };
		inputElements[1] = { "NORMAL",   0, DXGI_FORMAT_R32G32B32_FLOAT,    0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 };
		inputElements[2] = { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT,       0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 };
		inputElements[3] = { "UNK",      0, DXGI_FORMAT_R32G32_UINT,        0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 };

		vertexShader = g_dxHandler->GetShaderManager()->LoadShader("shaders/vertexUnlitTS_vs", eShaderType::Vertex, false);
		pixelShader = g_dxHandler->GetShaderManager()->LoadShader("shaders/bsp_ps", eShaderType::Pixel, false);

		break;
	}
	}

	if (!vertexShader->GetInputLayout())
	{
		ID3D11InputLayout* inputLayout = nullptr;

		const HRESULT hr = g_dxHandler->GetDevice()->CreateInputLayout(
			inputElements, numElements,
			vertexShader->GetBytecodeBlob()->GetBufferPointer(),
			vertexShader->GetBytecodeBlob()->GetBufferSize(),
			&inputLayout);
		UNUSED(hr);
		assert(SUCCEEDED(hr));

		vertexShader->SetInputLayout(inputLayout);
	}

	*vertexShaderOut = vertexShader;
	*pixelShaderOut = pixelShader;

	delete[] inputElements;
}

UINT GetVertexStrideByLumpId(int lumpId)
{
	switch (lumpId)
	{
	case LUMP_VERTS_LIT_FLAT:
	case LUMP_VERTS_UNLIT:
	{
		return 20;
	}
	case LUMP_VERTS_UNLIT_TS:
	{
		return 24;
	}
	case LUMP_VERTS_LIT_BUMP:
	{
		return 32;
	}
	default:
		assert(0);
		return 0;
	}
}

UINT GetVertexStrideByMeshFlag(int vertexType)
{
	switch (vertexType)
	{
	case MESH_VERTEX_LIT_FLAT:
	case MESH_VERTEX_UNLIT:
	{
		return 20;
	}
	case MESH_VERTEX_UNLIT_TS:
	{
		return 24;
	}
	case MESH_VERTEX_LIT_BUMP:
	{
		return 32;
	}
	default:
		assert(0);
		return 0;
	}
}

uint8_t GetVertexLumpIdByMeshFlag(int vertexType)
{
	switch (vertexType)
	{
	case MESH_VERTEX_LIT_FLAT:
	{
		return LUMP_VERTS_LIT_FLAT;
	}
	case MESH_VERTEX_UNLIT:
	{
		return LUMP_VERTS_UNLIT;
	}
	case MESH_VERTEX_UNLIT_TS:
	{
		return LUMP_VERTS_UNLIT_TS;
	}
	case MESH_VERTEX_LIT_BUMP:
	{
		return LUMP_VERTS_LIT_BUMP;
	}
	default:
		assert(0);
		return 0;
	}
}

#define CONVERT_VERT_STRIDE(originalStride) (originalStride - (2*sizeof(uint32_t))) + (2 * sizeof(float3))

void CBSPData::CreateOrUpdatePreviewStructuredBuffers()
{
	const uint32_t vertPositionsLumpSize = GetLumpSize(LUMP_VERTEXES);
	const uint32_t vertNormalsLumpSize = GetLumpSize(LUMP_VERTNORMALS);

	if (!m_vertPositionsBuffer && vertPositionsLumpSize > 0)
	{
		if (CreateD3DBuffer(g_dxHandler->GetDevice(),
			&m_vertPositionsBuffer, vertPositionsLumpSize,
			D3D11_USAGE_DYNAMIC, D3D11_BIND_SHADER_RESOURCE,
			D3D11_CPU_ACCESS_WRITE, D3D11_RESOURCE_MISC_BUFFER_STRUCTURED,
			sizeof(float3)))
		{
			D3D11_SHADER_RESOURCE_VIEW_DESC desc{};
			desc.Format = DXGI_FORMAT_UNKNOWN;
			desc.ViewDimension = D3D11_SRV_DIMENSION_BUFFER;
			desc.Buffer.FirstElement = 0;
			desc.Buffer.NumElements = static_cast<UINT>(vertPositionsLumpSize / sizeof(float3));

			HRESULT hr = g_dxHandler->GetDevice()->CreateShaderResourceView(m_vertPositionsBuffer, &desc, &this->m_vertPositionsSRV);

			assert(SUCCEEDED(hr));

			if (SUCCEEDED(hr))
				m_drawData->vertexShaderResources.push_back({ 0, m_vertPositionsSRV });
		}

		if (CreateD3DBuffer(g_dxHandler->GetDevice(),
			&m_vertNormalsBuffer, vertNormalsLumpSize,
			D3D11_USAGE_DYNAMIC, D3D11_BIND_SHADER_RESOURCE,
			D3D11_CPU_ACCESS_WRITE, D3D11_RESOURCE_MISC_BUFFER_STRUCTURED,
			sizeof(float3)))
		{
			D3D11_SHADER_RESOURCE_VIEW_DESC desc{};
			desc.Format = DXGI_FORMAT_UNKNOWN;
			desc.ViewDimension = D3D11_SRV_DIMENSION_BUFFER;
			desc.Buffer.FirstElement = 0;
			desc.Buffer.NumElements = static_cast<UINT>(vertNormalsLumpSize / sizeof(float3));

			HRESULT hr = g_dxHandler->GetDevice()->CreateShaderResourceView(m_vertNormalsBuffer, &desc, &this->m_vertNormalsSRV);

			assert(SUCCEEDED(hr));

			if (SUCCEEDED(hr))
				m_drawData->vertexShaderResources.push_back({ 1, m_vertNormalsSRV });
		}
	}

	ID3D11DeviceContext* ctx = g_dxHandler->GetDeviceContext();
	if (m_vertPositionsBuffer)
	{
		D3D11_MAPPED_SUBRESOURCE resource;
		assert(SUCCEEDED(ctx->Map(this->m_vertPositionsBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &resource)));

		memcpy(resource.pData, GetLumpData(LUMP_VERTEXES).get(), vertPositionsLumpSize);

		ctx->Unmap(this->m_vertPositionsBuffer, 0);
	}

	if (m_vertNormalsBuffer)
	{
		D3D11_MAPPED_SUBRESOURCE resource;
		assert(SUCCEEDED(ctx->Map(this->m_vertNormalsBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &resource)));

		memcpy(resource.pData, GetLumpData(LUMP_VERTNORMALS).get(), vertNormalsLumpSize);

		ctx->Unmap(this->m_vertNormalsBuffer, 0);
	}
}

void CBSPData::PopulateFromPakAsset(CPakAsset* pakAsset, void* bspData)
{
	CPakFile* pak = static_cast<CPakFile*>(pakAsset->GetContainerFile());
	UNUSED(pak);
	BSPHeader_t* header = reinterpret_cast<BSPHeader_t*>(bspData);

	SetVersion(header->version);

	// If bsp lumps are stored outside of the main .bsp
	// In the case of this function, this will mean that
	// the lumps are in separate wrap assets
	if (header->flags & 1)
	{
		for (uint8_t i = 0; i < header->lastLump; ++i)
		{
			if (header->lumps[i].filelen != 0)
			{
				m_lumpSizes[i] = header->lumps[i].filelen;

				const std::string lumpAssetName = std::format("maps/{}.bsp.{:04X}.bsp_lump.client", m_mapName, i);
				const uint64_t lumpAssetGuid = RTech::StringToGuid(lumpAssetName.c_str());
				CAsset* lumpAsset = g_assetData.FindAssetByGUID(lumpAssetGuid);

				if (lumpAsset)
					SetLumpData(i, GetWrapAssetData(lumpAsset, nullptr));
				else
					printf("no asset %s?\n", lumpAssetName.c_str());
			}
		}
	}

	l.numModels = header->lumps[LUMP_MODELS].filelen / sizeof(dmodel_t);
	l.numMeshes = header->lumps[LUMP_MESHES].filelen / sizeof(dmesh_t);

	l.numVertPositions = header->lumps[LUMP_VERTEXES].filelen / sizeof(Vector);
	l.numVertNormals = header->lumps[LUMP_VERTNORMALS].filelen / sizeof(Vector);
}

void CreateDXDrawDataTransformsBuffer(CDXDrawData* drawData)
{
	if (!drawData->transformsBuffer)
	{
		D3D11_BUFFER_DESC desc{};

		constexpr UINT transformsBufferSizeAligned = IALIGN(sizeof(VS_TransformConstants), 16);

		desc.ByteWidth = transformsBufferSizeAligned;

		// make sure this buffer can be updated every frame
		desc.Usage = D3D11_USAGE_DYNAMIC;
		desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;

		// const buffer
		desc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;

		g_dxHandler->GetDevice()->CreateBuffer(&desc, NULL, &drawData->transformsBuffer);
	}

	D3D11_MAPPED_SUBRESOURCE resource;
	g_dxHandler->GetDeviceContext()->Map(
		drawData->transformsBuffer, 0,
		D3D11_MAP_WRITE_DISCARD, 0,
		&resource
	);

	CDXCamera* const camera = g_dxHandler->GetCamera();
	const XMMATRIX view = camera->GetViewMatrix();
	const XMMATRIX model = XMMatrixTranslationFromVector(drawData->position.AsXMVector());
	const XMMATRIX projection = g_dxHandler->GetProjMatrix();

	VS_TransformConstants* const transforms = reinterpret_cast<VS_TransformConstants*>(resource.pData);
	transforms->modelMatrix = XMMatrixTranspose(model);
	transforms->viewMatrix = XMMatrixTranspose(view);
	transforms->projectionMatrix = XMMatrixTranspose(projection);

	g_dxHandler->GetDeviceContext()->Unmap(drawData->transformsBuffer, 0);
}

CDXDrawData* CBSPData::ConstructPreviewData()
{
	if (!m_drawData)
	{
		m_drawData = new CDXDrawData();

		//CreateOrUpdatePreviewStructuredBuffers();

		m_drawData->pixelShader = g_dxHandler->GetShaderManager()->LoadShader("shaders/bsp_ps", eShaderType::Pixel);
		m_drawData->vertexShader = g_dxHandler->GetShaderManager()->LoadShader("shaders/bsp_vs", eShaderType::Vertex);

		std::map<int, ID3D11Buffer*> lumpVertexBuffers;

		const float3* vertexPositionsLumpData = reinterpret_cast<const float3*>(GetLumpData(LUMP_VERTEXES).get());
		const float3* vertexNormalsLumpData = reinterpret_cast<const float3*>(GetLumpData(LUMP_VERTNORMALS).get());

		for (int i = LUMP_VERTS_UNLIT; i <= LUMP_VERTS_UNLIT_TS; ++i)
		{
			lumpVertexBuffers[i] = NULL;

			if (GetLumpSize(i) <= 0)
				continue;

			const UINT originalStride = GetVertexStrideByLumpId(i);
			const UINT newStride = CONVERT_VERT_STRIDE(originalStride);
			const size_t numVertices = GetLumpSize(i) / GetVertexStrideByLumpId(i);

			char* newVertexBuffer = new char[numVertices * newStride];
			char* newVertCursor = newVertexBuffer;

			const char* vertLumpData = GetLumpData(i).get();

			for (int j = 0; j < numVertices; ++j)
			{
				const uint32_t posIdx = *reinterpret_cast<const uint32_t*>(vertLumpData);
				const uint32_t nmlIdx = *reinterpret_cast<const uint32_t*>(vertLumpData + sizeof(uint32_t));

				*reinterpret_cast<float3*>(newVertCursor) = vertexPositionsLumpData[posIdx];
				*reinterpret_cast<float3*>(newVertCursor + sizeof(float3)) = vertexNormalsLumpData[nmlIdx];

				// copy over the rest of the data as is
				memcpy_s(
					newVertCursor  + (2 * sizeof(float3)),
					newStride      - (2 * sizeof(float3)),

					vertLumpData   + (2 * sizeof(uint32_t)),
					originalStride - (2 * sizeof(uint32_t)));

				// advance the lump pointer by the original vertex stride
				// (basically just go to the next original vert)
				vertLumpData += originalStride;

				// advance the pointer for writing the new verts into the vertex buffer
				newVertCursor += newStride;
			}

			CreateD3DBuffer(
				g_dxHandler->GetDevice(),
				&lumpVertexBuffers[i],
				static_cast<UINT>(numVertices * newStride),
				D3D11_USAGE_DYNAMIC,
				D3D11_BIND_VERTEX_BUFFER,
				D3D11_CPU_ACCESS_WRITE,
				0, 0,
				newVertexBuffer
			);

			delete[] newVertexBuffer;
		}

		const dmodel_t* modelLumpData = reinterpret_cast<dmodel_t*>(GetLumpData(LUMP_MODELS).get());
		const dmesh_t* meshLumpData = reinterpret_cast<dmesh_t*>(GetLumpData(LUMP_MESHES).get());
		const dmaterialsort_t* materialLumpData = reinterpret_cast<dmaterialsort_t*>(GetLumpData(LUMP_MATERIAL_SORT).get());
		const uint16_t* indexLumpData = reinterpret_cast<uint16_t*>(GetLumpData(LUMP_MESH_INDICES).get());
		const dtexdata_t* texLumpData = reinterpret_cast<dtexdata_t*>(GetLumpData(LUMP_TEXDATA).get());
		const char* texStringLumpData = reinterpret_cast<char*>(GetLumpData(LUMP_TEXDATA_STRING_DATA).get());

		for (int i = 0; i < l.numModels; ++i)
		{
			const dmodel_t* model = &modelLumpData[i];

			for (int j = model->firstMesh; j < (model->firstMesh + model->meshCount); ++j)
			{
				const dmesh_t* mesh = &meshLumpData[j];

				if (mesh->triCount <= 0)
					continue;

				DXMeshDrawData_t meshDrawData;
				meshDrawData.indexFormat = DXGI_FORMAT_R32_UINT;

				meshDrawData.doFrustumCulling = true;
				meshDrawData.modelMins = model->mins;
				meshDrawData.modelMaxs = model->maxs;

				const dmaterialsort_t* mtlSort = &materialLumpData[mesh->mtlSortIdx];

				CShader* meshPixelShader = nullptr;
				CShader* meshVertexShader = nullptr;

				const int meshVertType = mesh->flags & 0x600;
				const int meshVertLumpId = GetVertexLumpIdByMeshFlag(meshVertType);

				GetShadersForVertexLump(meshVertType, &meshVertexShader, &meshPixelShader);

				meshDrawData.vertexShader = meshVertexShader->Get<ID3D11VertexShader>();
				meshDrawData.pixelShader = meshPixelShader->Get<ID3D11PixelShader>();

				meshDrawData.inputLayout = meshVertexShader->GetInputLayout();

				// vertex stride using the original data (with pos/nml idx instead of vectors)
				const UINT originalStride = GetVertexStrideByLumpId(meshVertLumpId);

				// updated stride with the converted position and normal data
				meshDrawData.vertexStride = CONVERT_VERT_STRIDE(originalStride);
				meshDrawData.numIndices = mesh->triCount * 3;

				uint32_t* meshIndexData = new uint32_t[meshDrawData.numIndices];

				int indexIndex = 0; // ok
				for (int k = mesh->firstIdx; k < mesh->firstIdx	+ (mesh->triCount*3); ++k)
				{
					meshIndexData[indexIndex] = indexLumpData[k] + mtlSort->firstVertex;

					indexIndex++;
				}

				meshDrawData.vertexBuffer = lumpVertexBuffers[meshVertLumpId];

				CreateD3DBuffer(
					g_dxHandler->GetDevice(),
					&meshDrawData.indexBuffer,
					static_cast<UINT>(meshDrawData.numIndices*sizeof(uint32_t)),
					D3D11_USAGE_DYNAMIC,
					D3D11_BIND_INDEX_BUFFER,
					D3D11_CPU_ACCESS_WRITE,
					0, 0,
					meshIndexData
				);


				delete[] meshIndexData;


				const dtexdata_t& tex = texLumpData[mtlSort->texdata];
				std::string materialName = &texStringLumpData[tex.nameStringTableID];
				materialName = "material/" + materialName + "_wldc.rpak";

				CAsset* materialAsset = g_assetData.FindAssetByGUID(RTech::StringToGuid(materialName.c_str()));
				if (materialAsset)
				{
					CPakAsset* matlPakAsset = reinterpret_cast<CPakAsset*>(materialAsset);
					const MaterialAsset* const matl = reinterpret_cast<MaterialAsset*>(matlPakAsset->extraData());

					meshDrawData.textures.clear();
					const TextureAssetEntry_t& texEntry = matl->txtrAssets[0];
					//for (auto& texEntry : matl->txtrAssets)
					{
						if (texEntry.asset)
						{
							TextureAsset* txtr = reinterpret_cast<TextureAsset*>(texEntry.asset->extraData());
							const std::shared_ptr<CTexture> highestTextureMip = CreateTextureFromMip(texEntry.asset, &txtr->mipArray[std::min(2ull, txtr->mipArray.size() - 1)], s_PakToDxgiFormat[txtr->imgFormat]);
							meshDrawData.textures.push_back({ texEntry.index, highestTextureMip });
						}
					}
				}

				m_drawData->meshBuffers.push_back(meshDrawData);
			}
		}
	}

	CreateDXDrawDataTransformsBuffer(m_drawData);

	return m_drawData;
}

// very temp
void CBSPData::Export(std::ofstream* out)
{
	UNUSED(out);
	const float3* positionsLump = reinterpret_cast<float3*>(GetLumpData(LUMP_VERTEXES).get());
	const float3* normalsLump = reinterpret_cast<float3*>(GetLumpData(LUMP_VERTNORMALS).get());
	const uint16_t* indicesLump = reinterpret_cast<uint16_t*>(GetLumpData(LUMP_MESH_INDICES).get());

	for (int i = 0; i < l.numVertPositions; ++i)
	{
		*out << (std::format("v {} {} {}\n", positionsLump[i].x, positionsLump[i].y, positionsLump[i].z));
	}

	for (int i = 0; i < l.numVertNormals; ++i)
	{
		*out << (std::format("vn {} {} {}\n", normalsLump[i].x, normalsLump[i].y, normalsLump[i].z));
	}

	//for (int i = LUMP_VERTS_UNLIT; i <= LUMP_VERTS_UNLIT_TS; ++i)
	//{
	//	if (GetLumpSize(i) <= 0)
	//		continue;

	//	const UINT originalStride = GetVertexStrideByLumpId(i);
	//	const UINT newStride = CONVERT_VERT_STRIDE(originalStride);
	//	const size_t numVertices = GetLumpSize(i) / GetVertexStrideByLumpId(i);

	//}

	for (int i = 0; i < l.numModels; ++i)
	{
		dmodel_t* model = &reinterpret_cast<dmodel_t*>(GetLumpData(LUMP_MODELS).get())[i];

		for (int j = model->firstMesh; j < (model->firstMesh + model->meshCount); ++j)
		{
			dmesh_t* mesh = &reinterpret_cast<dmesh_t*>(GetLumpData(LUMP_MESHES).get())[j];

			if (mesh->triCount <= 0)
				continue;

			const dmaterialsort_t* mtlSort = &reinterpret_cast<dmaterialsort_t*>(GetLumpData(LUMP_MATERIAL_SORT).get())[mesh->mtlSortIdx];
			
			const int meshVertType = mesh->flags & 0x600;
			const int meshVertLumpId = GetVertexLumpIdByMeshFlag(meshVertType);

			const UINT vertexStride = GetVertexStrideByLumpId(meshVertLumpId);

			std::string faceString = "";
			int vertWriteIndex = 0;
			for (int k = mesh->firstIdx; k < mesh->firstIdx + (mesh->triCount * 3); ++k)
			{
				const int index = indicesLump[k] + mtlSort->firstVertex;

				const uint32_t* vertPointer = reinterpret_cast<uint32_t*>(GetLumpData(meshVertLumpId).get() + (vertexStride * index));

				const uint32_t posIdx = vertPointer[0];
				const uint32_t nmlIdx = vertPointer[1];

				if ((vertWriteIndex % 3) == 0)
				{
					*out << (faceString) << "\n";
					faceString = "f";
				}

				faceString += std::format(" {}//{}", posIdx + 1, nmlIdx + 1);

				vertWriteIndex++;
			}
			*out << (faceString);
		}
	}
}