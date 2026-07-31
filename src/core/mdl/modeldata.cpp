#include <pch.h>
#include <core/mdl/modeldata.h>

#include <core/mdl/rmax.h>
#include <core/mdl/cast.h>
#include <core/mdl/smd.h>

//#include <core/render/dx.h>
//#include <thirdparty/imgui/imgui.h>
#include <thirdparty/imgui/misc/imgui_utility.h>
#include <core/render/preview/preview.h>

extern CDXParentHandler* g_dxHandler;
extern CBufferManager g_BufferManager;
extern RSXSettings_t g_rsxSettings;
extern CPreviewDrawData g_currentPreviewDrawData;

//
// PARSEDDATA
//
#define VERT_DATA(t, d, o) reinterpret_cast<const t* const>(d + o)
#define MAP_BONE(b) bigBones ? reinterpret_cast<const uint16_t* const>(boneMap)[b] : (uint16_t)reinterpret_cast<const uint8_t* const>(boneMap)[b];
bool Vertex_t::ParseVertexFromVG(Vertex_t* const vert, VertexWeight_t* const weights, Vector2D* const texcoords, ModelMeshData_t* const mesh, const char* const rawVertexData, const void* const boneMap, const vvw::mstudioboneweightextra_t* const weightExtra, bool bigBones, int& weightIdx)
{
	int offset = 0;

	// [rika]: older hwdata models used bit flags, but starting in season 11.1 (rmdl 13.1) it's treated more like an enum
	/*if (mesh->rawVertexLayoutFlags & VERT_POSITION_UNPACKED)
	{
		vert->position = *VERT_DATA(Vector, rawVertexData, offset);
		offset += sizeof(Vector);
	}

	if (mesh->rawVertexLayoutFlags & VERT_POSITION_PACKED)
	{
		vert->position = VERT_DATA(Vector64, rawVertexData, offset)->Unpack();
		offset += sizeof(Vector64);
	}*/

	const vg::eVertPositionType posType = static_cast<vg::eVertPositionType>(mesh->rawVertexLayoutFlags & 3);
	switch (posType)
	{
	case vg::eVertPositionType::VG_POS_NONE:
	{
		break;
	}
	case vg::eVertPositionType::VG_POS_UNPACKED:
	{
		vert->position = *VERT_DATA(Vector, rawVertexData, offset);
		offset += sizeof(Vector);

		break;
	}
	case vg::eVertPositionType::VG_POS_PACKED64:
	{
		vert->position = VERT_DATA(Vector64, rawVertexData, offset)->Unpack();
		offset += sizeof(Vector64);

		break;
	}
	case vg::eVertPositionType::VG_POS_PACKED48:
	{
		// [rika]: not sure the format on this one, currently only used on switch and I can't be asked to find tools to decompile a shader at this time
		vert->position = Vector(0.0f);
		offset += 0x6;

		break;
	}
	}

	assertm(nullptr != weights, "weight pointer should be valid");
	vert->weightIndex = weightIdx;

	// we have weight data
	// note: if for some reason 'VERT_BLENDWEIGHTS_UNPACKED' is encountered, weights would not be processed.
	assertm(!(mesh->rawVertexLayoutFlags & VERT_BLENDWEIGHTS_UNPACKED), "mesh had unpacked weights!");
	if (mesh->rawVertexLayoutFlags & (VERT_BLENDINDICES | VERT_BLENDWEIGHTS_PACKED))
	{
		const vg::BlendWeightsPacked_s* const blendWeights = VERT_DATA(vg::BlendWeightsPacked_s, rawVertexData, offset);
		const vg::BlendWeightIndices_s* const blendIndices = VERT_DATA(vg::BlendWeightIndices_s, rawVertexData, offset + 4);

		offset += 8;

		// Copy blend data into the vert struct
		memcpy_s(&vert->blendData, sizeof(vert->blendData), blendWeights, sizeof(vert->blendData));

		uint8_t curIdx = 0; // current weight
		uint16_t remaining = 32767; // 'weight' remaining to assign to the last bone

		// model has more than 3 weights per vertex
		if (nullptr != weightExtra)
		{
			assertm(blendIndices->boneCount < 16, "model had more than 16 bones on complex weights");

			// first weight, we will always have this
			const int16_t firstBoneIndex = bigBones ? static_cast<uint16_t>(blendIndices->Packed()->firstBone) : blendIndices->bone[0];
			weights[curIdx].bone = MAP_BONE(firstBoneIndex);
			weights[curIdx].weight = blendWeights->Weight(0);
			remaining -= blendWeights->weight[0];

			curIdx++;

			// only hit if we have over 2 bones/weights
			for (uint8_t i = curIdx; i < blendIndices->boneCount; i++)
			{
				auto extraWeight = weightExtra[blendWeights->ExtraWeightsStartIndex() + (curIdx - 1)];

				weights[curIdx].bone = MAP_BONE(extraWeight.bone);
				weights[curIdx].weight = extraWeight.Weight();

				remaining -= extraWeight.weight;

				curIdx++;
			}

			// only hit if we have over 1 bone/weight
			if (blendIndices->boneCount > 0)
			{
				// im just using bigBones as a flag for >=v19.2 lmao
				const int16_t finalBoneIndex = bigBones ? static_cast<uint16_t>(blendIndices->Packed()->lastBone) : blendIndices->bone[1];
				weights[curIdx].bone = MAP_BONE(finalBoneIndex);
				weights[curIdx].weight = UNPACKWEIGHT(remaining);

				curIdx++;
			}
		}
		else
		{
			assertm(blendIndices->boneCount < 3, "model had more than 3 bones on simple weights");

			// There seems to be a condition here on v19.2, where if the model uses simple weights and this vert has less than two "extra" bones (i.e. boneCount < 2),
			// the blend indices use the same packed system as the complex weights. If the model uses simple weights and has TWO extra bones, they revert to the old system?
			//const bool singleExtraBigBone = bigBones && blendIndices->boneCount == 1;
			for (uint8_t i = 0; i < blendIndices->boneCount; i++)
			{
				weights[curIdx].bone = MAP_BONE(blendIndices->bone[curIdx]);
				weights[curIdx].weight = blendWeights->Weight(curIdx);

				remaining -= blendWeights->weight[curIdx];

				curIdx++;
			}

			weights[curIdx].bone = MAP_BONE(blendIndices->bone[curIdx]);
			weights[curIdx].weight = UNPACKWEIGHT(remaining);

			curIdx++;
		}

		vert->weightCount = curIdx;
		assert(static_cast<uint8_t>(vert->weightCount) == (blendIndices->boneCount + 1)); // numbones is really 'extra' bones on top of the base weight, verify the count is correct

		weightIdx += curIdx;
	}

	// our mesh does not have weight data, use a set of default weights. 
	// [rika]: this can only happen when a model has one bone
	else
	{
		vert->weightCount = 1;
		weights[0].bone = 0;
		weights[0].weight = 1.0f;

		weightIdx++;
	}

	mesh->weightsPerVert = static_cast<uint16_t>(vert->weightCount) > mesh->weightsPerVert ? static_cast<uint16_t>(vert->weightCount) : mesh->weightsPerVert;

	vert->normalPacked = *VERT_DATA(Normal32, rawVertexData, offset);
	offset += sizeof(Normal32);

	if (mesh->rawVertexLayoutFlags & VERT_COLOR)
	{
		// Vertex Colour
		vert->color = *VERT_DATA(Color32, rawVertexData, offset);
		offset += sizeof(Color32);
	}
	else // no vert colour, write default
	{
		vert->color = Color32(255, 255);
	}

	if (mesh->rawVertexLayoutFlags & VERT_TEXCOORD0)
	{
		vert->texcoord = *VERT_DATA(Vector2D, rawVertexData, offset);
		offset += sizeof(Vector2D);
	}

	for (int localIdx = 1, countIdx = 1; countIdx < mesh->texcoordCount; localIdx++)
	{
		assertm(nullptr != texcoords, "texcoord pointer should be valid");

		if (!VERT_TEXCOORDn(localIdx))
			continue;

		texcoords[countIdx - 1] = *VERT_DATA(Vector2D, rawVertexData, offset);
		offset += sizeof(Vector2D);

		countIdx++;
	}

	assertm(offset == mesh->vertCacheSize, "parsed data size differed from vertexCacheSize");

	return true;
}
#undef VERT_DATA

// Generic (basic data shared between them)
void Vertex_t::ParseVertexFromVTX(Vertex_t* const vert, Vector2D* const texcoords, ModelMeshData_t* const mesh, const vvd::mstudiovertex_t* const pVerts, const Vector4D* const pTangs, const Color32* const pColors, const Vector2D* const pUVs, const int origId)
{
	vert->position = pVerts[origId].m_vecPosition;

	// not normal
	vert->normalPacked.PackNormal(pVerts[origId].m_vecNormal, pTangs[origId]);

	if (pColors)
		vert->color = pColors[origId];
	else // no vert colour, write default
		vert->color = Color32(255, 255);

	vert->texcoord = pVerts[origId].m_vecTexCoord;

	for (int localIdx = 1, countIdx = 1; countIdx < mesh->texcoordCount; localIdx++)
	{
		assertm(nullptr != texcoords, "texcoord pointer should be valid");

		if (!VERT_TEXCOORDn(localIdx))
			continue;

		texcoords[countIdx - 1] = pUVs[origId]; // [rika]: add proper support for uv3 (though I doubt we'll ever get files for it, I think it would be quite quirky.)

		countIdx++;
	}
}

// Basic Source
void Vertex_t::ParseVertexFromVTX(Vertex_t* const vert, VertexWeight_t* const weights, Vector2D* const texcoords, ModelMeshData_t* mesh, const OptimizedModel::Vertex_t* const pVertex, const vvd::mstudiovertex_t* const pVerts, const Vector4D* const pTangs, const Color32* const pColors, const Vector2D* const pUVs,
	int& weightIdx, const bool isHwSkinned, const OptimizedModel::BoneStateChangeHeader_t* const pBoneStates)
{
	const int origId = pVertex->origMeshVertID;

	ParseVertexFromVTX(vert, texcoords, mesh, pVerts, pTangs, pColors, pUVs, origId);

	const vvd::mstudiovertex_t& oldVert = pVerts[origId];

	assertm(nullptr != weights, "weight pointer should be valid");
	vert->weightIndex = weightIdx;
	vert->weightCount = oldVert.m_BoneWeights.numbones;

	for (int i = 0; i < oldVert.m_BoneWeights.numbones; i++)
	{
		weights[i].weight = oldVert.m_BoneWeights.weight[pVertex->boneWeightIndex[i]];

		// static props can be hardware skinned, but have no bonestates (one bone). this make it skip this, however it's a non issue as the following statement will work fine for this (there is only one bone at idx 0)
		if (isHwSkinned && pBoneStates)
		{
			weights[i].bone = static_cast<int16_t>(pBoneStates[pVertex->boneID[i]].newBoneID);
			continue;
		}

		weights[i].bone = pVertex->boneID[i];
	}

	weightIdx += oldVert.m_BoneWeights.numbones;

	mesh->weightsPerVert = static_cast<uint16_t>(vert->weightCount) > mesh->weightsPerVert ? static_cast<uint16_t>(vert->weightCount) : mesh->weightsPerVert;
}

// Apex Legends
void Vertex_t::ParseVertexFromVTX(Vertex_t* const vert, VertexWeight_t* const weights, Vector2D* const texcoords, ModelMeshData_t* mesh, const OptimizedModel::Vertex_t* const pVertex, const vvd::mstudiovertex_t* const pVerts, const Vector4D* const pTangs, const Color32* const pColors, const Vector2D* const pUVs,
	const vvw::vertexBoneWeightsExtraFileHeader_t* const pVVW, int& weightIdx)
{
	const int origId = pVertex->origMeshVertID;

	ParseVertexFromVTX(vert, texcoords, mesh, pVerts, pTangs, pColors, pUVs, origId);

	const vvd::mstudiovertex_t& oldVert = pVerts[origId];

	assertm(nullptr != weights, "weight pointer should be valid");
	vert->weightIndex = weightIdx;
	vert->weightCount = oldVert.m_BoneWeights.numbones;

	if (nullptr != pVVW)
	{
		const vvw::mstudioboneweightextra_t* const pExtraWeights = pVVW->GetWeightData(oldVert.m_BoneWeights.weightextra.extraweightindex);

		for (int i = 0; i < oldVert.m_BoneWeights.numbones; i++)
		{
			if (i >= 3)
			{
				weights[i].bone = pExtraWeights[i - 3].bone;
				weights[i].weight = pExtraWeights[i - 3].Weight();

				continue;
			}

			weights[i].bone = oldVert.m_BoneWeights.bone[i];
			weights[i].weight = oldVert.m_BoneWeights.weightextra.Weight(i);
		}
	}
	else
	{
		for (int i = 0; i < oldVert.m_BoneWeights.numbones; i++)
		{
			weights[i].bone = oldVert.m_BoneWeights.bone[i];
			weights[i].weight = oldVert.m_BoneWeights.weight[i];
		}
	}

	weightIdx += oldVert.m_BoneWeights.numbones;

	mesh->weightsPerVert = static_cast<uint16_t>(vert->weightCount) > mesh->weightsPerVert ? static_cast<uint16_t>(vert->weightCount) : mesh->weightsPerVert;
}

void ModelMeshData_t::ParseTexcoords()
{
	if (rawVertexLayoutFlags & VERT_TEXCOORD0)
	{
		int texCoordIdx = 0;
		int texCoordShift = 24;

		uint64_t inputFlagsShifted = rawVertexLayoutFlags >> texCoordShift;
		do
		{
			inputFlagsShifted = rawVertexLayoutFlags >> texCoordShift;

			int8_t texCoordFormat = inputFlagsShifted & VERT_TEXCOORD_MASK;

			assertm(texCoordFormat == 2 || texCoordFormat == 0, "invalid texcoord format");

			if (texCoordFormat != 0)
			{
				texcoordCount++;
				texcoodIndices |= (1 << texCoordIdx);
			}

			texCoordShift += VERT_TEXCOORD_BITS;
			texCoordIdx++;
		} while (inputFlagsShifted >= (1 << VERT_TEXCOORD_BITS)); // while the flag value is large enough that there is more than just one 
	}
}

void ModelMeshData_t::ParseMaterial(ModelParsedData_t* const parsed, const int material)
{
	// [rika]: handling mesh's material here since we've already looped through everything
	assertm(material < parsed->materials.size() && material >= 0, "invalid mesh material index");
	materialId = material;
	materialAsset = parsed->materials.at(material).asset;
}

// bones
void ParseModelBoneData_v8(ModelParsedData_t* const parsedData)
{
	const studiohdr_generic_t* const pStudioHdr = parsedData->pStudioHdr();

	const r5::mstudiobone_v8_t* const bones = reinterpret_cast<const r5::mstudiobone_v8_t* const>(pStudioHdr->pBones());
	parsedData->bones.resize(pStudioHdr->boneCount);

	for (int i = 0; i < pStudioHdr->boneCount; i++)
	{
		parsedData->bones.at(i) = ModelBone_t(&bones[i]);
	}
}

void ParseModelBoneData_v12_1(ModelParsedData_t* const parsedData)
{
	const studiohdr_generic_t* const pStudioHdr = parsedData->pStudioHdr();

	const r5::mstudiobone_v12_1_t* const bones = reinterpret_cast<const r5::mstudiobone_v12_1_t* const>(pStudioHdr->pBones());
	parsedData->bones.resize(pStudioHdr->boneCount);

	for (int i = 0; i < pStudioHdr->boneCount; i++)
	{
		parsedData->bones.at(i) = ModelBone_t(&bones[i]);
	}
}

void ParseModelBoneData_v16(ModelParsedData_t* const parsedData)
{
	const studiohdr_generic_t* const pStudioHdr = parsedData->pStudioHdr();

	const r5::mstudiobonehdr_v16_t* const bonehdrs = reinterpret_cast<const r5::mstudiobonehdr_v16_t* const>(pStudioHdr->pBones());
	const r5::mstudiobonedata_v16_t* const bonedata = reinterpret_cast<const r5::mstudiobonedata_v16_t* const>(pStudioHdr->pBoneData());

	parsedData->bones.resize(pStudioHdr->boneCount);

	for (int i = 0; i < pStudioHdr->boneCount; i++)
		parsedData->bones.at(i) = ModelBone_t(&bonehdrs[i], &bonedata[i]);
}

void ParseModelBoneData_v19(ModelParsedData_t* const parsedData)
{
	const studiohdr_generic_t* const pStudioHdr = parsedData->pStudioHdr();

	const r5::mstudiobonehdr_v16_t* const bonehdrs = reinterpret_cast<const r5::mstudiobonehdr_v16_t* const>(pStudioHdr->pBones());
	const r5::mstudiobonedata_v19_t* const bonedata = reinterpret_cast<const r5::mstudiobonedata_v19_t* const>(pStudioHdr->pBoneData());
	const r5::mstudiolinearbone_v19_t* const linearbone = reinterpret_cast<const r5::mstudiolinearbone_v19_t* const>(pStudioHdr->pLinearBone());

	parsedData->bones.resize(pStudioHdr->boneCount);

	for (int i = 0; i < pStudioHdr->boneCount; i++)
		parsedData->bones.at(i) = ModelBone_t(&bonehdrs[i], &bonedata[i], linearbone, i);
}

void ParseModelAttachmentData_v8(ModelParsedData_t* const parsedData)
{
	const studiohdr_generic_t* const pStudioHdr = parsedData->pStudioHdr();
	const r5::mstudioattachment_v8_t* const pAttachments = reinterpret_cast<const r5::mstudioattachment_v8_t* const>(pStudioHdr->baseptr + pStudioHdr->localAttachmentOffset);

	parsedData->attachments.reserve(pStudioHdr->localAttachmentCount);

	for (int i = 0; i < pStudioHdr->localAttachmentCount; i++)
	{
		parsedData->attachments.emplace_back(pAttachments + i);
	}
}

void ParseModelAttachmentData_v16(ModelParsedData_t* const parsedData)
{
	const studiohdr_generic_t* const pStudioHdr = parsedData->pStudioHdr();
	const r5::mstudioattachment_v16_t* const pAttachments = reinterpret_cast<const r5::mstudioattachment_v16_t* const>(pStudioHdr->baseptr + pStudioHdr->localAttachmentOffset);

	parsedData->attachments.reserve(pStudioHdr->localAttachmentCount);

	for (int i = 0; i < pStudioHdr->localAttachmentCount; i++)
	{
		parsedData->attachments.emplace_back(pAttachments + i);
	}
}

void ParseModelHitboxData_v8(ModelParsedData_t* const parsedData)
{
	const studiohdr_generic_t* const pStudioHdr = parsedData->pStudioHdr();
	const mstudiohitboxset_t* const pHitboxSets = reinterpret_cast<const mstudiohitboxset_t* const>(pStudioHdr->baseptr + pStudioHdr->hitboxSetOffset);

	parsedData->hitboxsets.reserve(pStudioHdr->hitboxSetCount);

	for (int i = 0; i < pStudioHdr->hitboxSetCount; i++)
	{
		parsedData->hitboxsets.emplace_back(pHitboxSets + i, pHitboxSets->pHitbox<r5::mstudiobbox_v8_t>(0));
	}
}

void ParseModelHitboxData_v16(ModelParsedData_t* const parsedData)
{
	const studiohdr_generic_t* const pStudioHdr = parsedData->pStudioHdr();
	const r5::mstudiohitboxset_v16_t* const pHitboxSets = reinterpret_cast<const r5::mstudiohitboxset_v16_t* const>(pStudioHdr->baseptr + pStudioHdr->hitboxSetOffset);

	parsedData->hitboxsets.reserve(pStudioHdr->hitboxSetCount);

	for (int i = 0; i < pStudioHdr->hitboxSetCount; i++)
	{
		parsedData->hitboxsets.emplace_back(pHitboxSets + i);
	}
}

void CreateBuffersForModelHitboxes(ModelParsedData_t* const parsedData, CDXDrawData* const drawData)
{
	CShader* vertexShader = g_dxHandler->GetShaderManager()->LoadShaderFromString("shaders/model_vs", s_PreviewVertexShader, eShaderType::Vertex);;
	CShader* pixelShader = g_dxHandler->GetShaderManager()->LoadShaderFromString("shaders/model_ps", s_PreviewPixelShader, eShaderType::Pixel);

	for (auto& hitboxSet : parsedData->hitboxsets)
	{
		for (int i = 0; i < hitboxSet.numHitboxes; ++i)
		{
			const ModelHitbox_t& h = hitboxSet.hitboxes[i];

			Vector bonePos = parsedData->bones[h.bone].pos;
			Vector bbmin = (*h.bbmin) + bonePos;
			Vector bbmax = (*h.bbmax) + bonePos;

			// -8: x y z
			// -7: X y z
			// -6: x Y z
			// -5: x y Z
			// -4: X Y z
			// -3: X y Z
			// -2: x Y Z
			// -1: X Y Z

			const std::vector<Vertex_t> vertices = {
				{bbmin.x, bbmin.y, bbmin.z},
				{bbmax.x, bbmin.y, bbmin.z},
				{bbmin.x, bbmax.y, bbmin.z},
				{bbmin.x, bbmin.y, bbmax.z},
				{bbmax.x, bbmax.y, bbmin.z},
				{bbmax.x, bbmin.y, bbmax.z},
				{bbmin.x, bbmax.y, bbmax.z},
				{bbmax.x, bbmax.y, bbmax.z},
			};

			const std::vector<uint16_t> indices = {
				2, 4, 0,
				0, 4, 1,
				5, 3, 1,
				1, 3, 0,
				4, 7, 1,
				1, 7, 5,
				6, 7, 2,
				2, 7, 4,
				7, 6, 5,
				5, 6, 3,
				6, 2, 3,
				3, 2, 0
			};

			DXMeshDrawData_t& meshDrawData = drawData->meshBuffers.emplace_back();

			meshDrawData.visible = false;
			meshDrawData.doFrustumCulling = false;
			meshDrawData.wireframe = true;
			meshDrawData.indexFormat = DXGI_FORMAT_R16_UINT;
			meshDrawData.vertexShader = vertexShader->Get<ID3D11VertexShader>();
			meshDrawData.pixelShader = pixelShader->Get<ID3D11PixelShader>();
			meshDrawData.inputLayout = vertexShader->GetInputLayout();
			meshDrawData.hasGameShaders = false;

			if (!meshDrawData.vertexBuffer)
			{
				constexpr UINT vertStride = sizeof(Vertex_t);

				D3D11_BUFFER_DESC desc = {};

				desc.Usage = D3D11_USAGE_DYNAMIC;
				desc.ByteWidth = static_cast<UINT>(vertStride * vertices.size());
				desc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
				desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
				desc.MiscFlags = 0;

				D3D11_SUBRESOURCE_DATA srd{ vertices.data() };

				if (FAILED(g_dxHandler->GetDevice()->CreateBuffer(&desc, &srd, &meshDrawData.vertexBuffer)))
					return;

				meshDrawData.vertexStride = vertStride;
			}

			if (!meshDrawData.indexBuffer)
			{
				D3D11_BUFFER_DESC desc = {};

				desc.Usage = D3D11_USAGE_DYNAMIC;
				desc.ByteWidth = static_cast<UINT>(indices.size() * sizeof(uint16_t));
				desc.BindFlags = D3D11_BIND_INDEX_BUFFER;
				desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
				desc.MiscFlags = 0;

				D3D11_SUBRESOURCE_DATA srd = { indices.data()};
				if (FAILED(g_dxHandler->GetDevice()->CreateBuffer(&desc, &srd, &meshDrawData.indexBuffer)))
					return;

				meshDrawData.numIndices = indices.size();
			}
		}
	}
}

void CreateBuffersForModelDrawData(ModelParsedData_t* const parsedData, CDXDrawData* const drawData, const uint64_t lod)
{
	// [rika]: eventually parse through models
	for (size_t i = 0; i < parsedData->lods.at(lod).meshes.size(); ++i)
	{
		const ModelMeshData_t& mesh = parsedData->lods.at(lod).meshes.at(i);
		DXMeshDrawData_t* const meshDrawData = &drawData->meshBuffers[i];

		meshDrawData->visible = true;
		meshDrawData->doFrustumCulling = false;
		meshDrawData->wireframe = false;

		if (mesh.materialAsset)
		{
			MaterialAsset* matl = mesh.GetMaterialAsset();
			meshDrawData->uberStaticBuf = matl->uberStaticBuffer;
			meshDrawData->uberDynamicBuf = matl->uberDynamicBuffer;
		}

		assertm(mesh.meshVertexDataIndex != invalidNoodleIdx, "mesh data hasn't been parsed ??");

		std::unique_ptr<char[]> parsedVertexDataBuf = parsedData->meshVertexData.getIdx(mesh.meshVertexDataIndex);
		const CMeshData* const parsedVertexData = reinterpret_cast<CMeshData*>(parsedVertexDataBuf.get());

		if (!meshDrawData->vertexBuffer)
		{
			constexpr UINT vertStride = sizeof(Vertex_t);

			D3D11_BUFFER_DESC desc = {};

			desc.Usage = D3D11_USAGE_DYNAMIC;
			desc.ByteWidth = vertStride * mesh.vertCount;
			desc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
			desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
			desc.MiscFlags = 0;

			const void* vertexData = parsedVertexData->GetVertices();

			D3D11_SUBRESOURCE_DATA srd{ vertexData };

			if (FAILED(g_dxHandler->GetDevice()->CreateBuffer(&desc, &srd, &meshDrawData->vertexBuffer)))
				return;

			meshDrawData->vertexStride = vertStride;
		}

		if (!meshDrawData->indexBuffer)
		{
			D3D11_BUFFER_DESC desc = {};

			desc.Usage = D3D11_USAGE_DYNAMIC;
			desc.ByteWidth = static_cast<UINT>(mesh.indexCount * sizeof(uint16_t));
			desc.BindFlags = D3D11_BIND_INDEX_BUFFER;
			desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
			desc.MiscFlags = 0;

			D3D11_SUBRESOURCE_DATA srd = { parsedVertexData->GetIndices() };
			if (FAILED(g_dxHandler->GetDevice()->CreateBuffer(&desc, &srd, &meshDrawData->indexBuffer)))
				return;

			meshDrawData->numIndices = mesh.indexCount;
		}

		if (!meshDrawData->weightsBuffer)
		{
			VertexWeight_t* const weights = parsedVertexData->GetWeights();
			const int64_t numWeights = parsedVertexData->GetWeightCount();

			VertexWeight_ForShader_t* wfs = new VertexWeight_ForShader_t[numWeights];
			for (int64_t j = 0; j < numWeights; ++j)
				wfs[j] = weights[j];

			if(CreateD3DBuffer(g_dxHandler->GetDevice(),
				&meshDrawData->weightsBuffer, static_cast<UINT>(numWeights) * sizeof(VertexWeight_ForShader_t),
				D3D11_USAGE_DYNAMIC, D3D11_BIND_SHADER_RESOURCE,
				D3D11_CPU_ACCESS_WRITE, D3D11_RESOURCE_MISC_BUFFER_STRUCTURED, sizeof(VertexWeight_ForShader_t), wfs
			))
			{
				D3D11_SHADER_RESOURCE_VIEW_DESC desc{};
				desc.Format = DXGI_FORMAT_UNKNOWN;
				desc.ViewDimension = D3D11_SRV_DIMENSION_BUFFER;
				desc.Buffer.FirstElement = 0;
				desc.Buffer.NumElements = static_cast<UINT>(numWeights);

				HRESULT hr = g_dxHandler->GetDevice()->CreateShaderResourceView(meshDrawData->weightsBuffer, &desc, &meshDrawData->weightsSRV);

				UNUSED(hr);
				assert(SUCCEEDED(hr));
			}

			delete[] wfs;
		}
	}

	return;
}


//
// COMPDATA
//

// CMeshData
void CMeshData::AddIndices(const uint16_t* const indices, const size_t indiceCount)
{
	assertm(writer, "attempting to write data, but writer is not initialized.");

	indiceOffset = writer - reinterpret_cast<char*>(this);

	const size_t bufferSize = indiceCount * sizeof(uint16_t);
	assertm((writer + bufferSize) - reinterpret_cast<char*>(this) < managedBufferSize, "data exceeded buffer size!!!");
	if (indices)
		memcpy(writer, indices, bufferSize);

	writer += IALIGN16(bufferSize);
}

void CMeshData::AddVertices(const Vertex_t* const vertices, const size_t vertexCount)
{
	assertm(writer, "attempting to write data, but writer is not initialized.");

	vertexOffset = writer - reinterpret_cast<char*>(this);

	const size_t bufferSize = vertexCount * sizeof(Vertex_t);
	assertm((writer + bufferSize) - reinterpret_cast<char*>(this) < managedBufferSize, "data exceeded buffer size!!!");
	if (vertices)
		memcpy(writer, vertices, bufferSize);

	writer += IALIGN16(bufferSize);
}

void CMeshData::AddWeights(const VertexWeight_t* const weights, const size_t _weightCount)
{
	assertm(writer, "attempting to write data, but writer is not initialized.");

	weightOffset = writer - reinterpret_cast<char*>(this);

	const size_t bufferSize = _weightCount * sizeof(VertexWeight_t);
	assertm((writer + bufferSize) - reinterpret_cast<char*>(this) < managedBufferSize, "data exceeded buffer size!!!");
	if (weights)
		memcpy(writer, weights, bufferSize);

	this->weightCount = _weightCount;

	writer += IALIGN16(bufferSize);
}

void CMeshData::AddTexcoords(const Vector2D* const texcoords, const size_t texcoordCount)
{
	assertm(writer, "attempting to write data, but writer is not initialized.");

	texcoordOffset = writer - reinterpret_cast<char*>(this);

	const size_t bufferSize = texcoordCount * sizeof(Vector2D);
	assertm((writer + bufferSize) - reinterpret_cast<char*>(this) < managedBufferSize, "data exceeded buffer size!!!");
	if (texcoords)
		memcpy(writer, texcoords, bufferSize);

	writer += IALIGN16(bufferSize);
}

// CAnimData
CAnimData::CAnimData(char* const buf) : pBuffer(buf), memory(true)
{
	assertm(nullptr != pBuffer, "invalid pointer provided");

	const char* curpos = pBuffer;

	memcpy(&numBones, curpos, sizeof(int) * 2);
	curpos += sizeof(int) * 2;

	pOffsets = reinterpret_cast<const size_t* const>(curpos);
	curpos += IALIGN16(sizeof(size_t) * numBones);

	pFlags = reinterpret_cast<const uint8_t*>(curpos);
	curpos += IALIGN16(sizeof(uint8_t) * numBones);
};

// access memory data
const Vector* const CAnimData::GetBonePosForFrame(const int bone, const int frame) const
{
	const uint8_t boneFlags = GetFlag(bone);
	assertm(boneFlags & CAnimDataBone::ANIMDATA_POS, "bone did not have position");

	const Vector* const tmp = reinterpret_cast<const Vector* const>(pBuffer + pOffsets[bone]);

	return tmp + frame;
}

const Quaternion* const CAnimData::GetBoneQuatForFrame(const int bone, const int frame) const
{
	const uint8_t boneFlags = GetFlag(bone);
	assertm(boneFlags & CAnimDataBone::ANIMDATA_ROT, "bone did not have rotation");

	const Quaternion* const tmp = reinterpret_cast<const Quaternion* const>(pBuffer + pOffsets[bone] + (s_AnimDataBoneSizeLUT[boneFlags & CAnimDataBone::ANIMDATA_POS] * numFrames));

	return tmp + frame;
}

const Vector* const CAnimData::GetBoneScaleForFrame(const int bone, const int frame) const
{
	const uint8_t boneFlags = GetFlag(bone);
	assertm(boneFlags & CAnimDataBone::ANIMDATA_SCL, "bone did not have scale");

	const Vector* const tmp = reinterpret_cast<const Vector* const>(pBuffer + pOffsets[bone] + (s_AnimDataBoneSizeLUT[boneFlags & (CAnimDataBone::ANIMDATA_POS | CAnimDataBone::ANIMDATA_ROT)] * numFrames));

	return tmp + frame;
}

const size_t CAnimData::ToMemory(char* const buf)
{
	char* curpos = buf;

	// dumb
	memcpy(curpos, &numBones, sizeof(int) * 2);
	curpos += sizeof(int) * 2;

	size_t* offsets = reinterpret_cast<size_t*>(curpos);
	curpos += IALIGN16(sizeof(size_t) * numBones);

	uint8_t* flags = reinterpret_cast<uint8_t*>(curpos);
	curpos += IALIGN16(sizeof(uint8_t) * numBones);

	for (size_t i = 0; i < numBones; i++)
	{
		const CAnimDataBone& bone = bones.at(i);

		offsets[i] = static_cast<size_t>(curpos - buf);

		flags[i] = bone.GetFlags();

		if (flags[i] & CAnimDataBone::ANIMDATA_POS)
		{
			memcpy(curpos, bone.GetPosPtr(), sizeof(Vector) * numFrames);
			curpos += sizeof(Vector) * numFrames;
		}

		if (flags[i] & CAnimDataBone::ANIMDATA_ROT)
		{
			memcpy(curpos, bone.GetRotPtr(), sizeof(Quaternion) * numFrames);
			curpos += sizeof(Quaternion) * numFrames;
		}

		if (flags[i] & CAnimDataBone::ANIMDATA_SCL)
		{
			memcpy(curpos, bone.GetSclPtr(), sizeof(Vector) * numFrames);
			curpos += sizeof(Vector) * numFrames;
		}
	}

	const size_t size = static_cast<size_t>(curpos - buf);
	assertm(size < CBufferManager::MaxBufferSize(), "animation data too large");

	return size;
};


//
// EXPORT SETTINGS
//

struct ModelMaterialExport_t
{
	ModelMaterialExport_t(MaterialAsset* const material, const int materialId) : asset(material), id(materialId) {}

	std::unordered_map<uint32_t, MaterialTextureExportInfo_s> textures;
	MaterialAsset* asset;
	int id;
};

// export materials from parsed data
void HandleModelMaterials(const ModelParsedData_t* const parsedData, std::unordered_map<int, ModelMaterialExport_t>& materials, const std::filesystem::path& exportPath)
{
	// [rika]: this will decide if we want textures/materials local to the model, or to use full paths in the future.
	const bool useFullPaths = false; // temp

	// [rika]: make sure materials are empty
	materials.clear();
	materials.reserve(parsedData->materials.size());

	// [rika]: pick a skin !
	if (g_rsxSettings.exportModelSkin && g_rsxSettings.previewedSkinIndex >= static_cast<int>(parsedData->skins.size()))
	{
		assertm(false, "skin index out of range");
		g_rsxSettings.previewedSkinIndex = 0;
	}

	const int skin = g_rsxSettings.exportModelSkin ? g_rsxSettings.previewedSkinIndex : 0;

	const ModelSkinData_t* const skinData = &parsedData->skins.at(skin);

	// [rika]: we don't need to cycle through all the LODs here, any used mesh should be in the top LOD and cannot change per LOD
	// this gets the data we need to export materials, and set them properly in meshes later
	// keep track of the materials that are actually used by meshes, so we don't export unneeded ones (speeds up export)
	for (const ModelMeshData_t& mesh : parsedData->lods.front().meshes)
	{
		// [rika]: no vertices, this mesh will not be exported.
		if (!mesh.vertCount)
			continue;

		const int baseId = mesh.materialId;
		const int skinId = static_cast<int>(skinData->indices[baseId]);

		const ModelMaterialData_t* const materialData = parsedData->pMaterial(skinId);
		MaterialAsset* asset = materialData->GetMaterialAsset();

		ModelMaterialExport_t material(asset, skinId);

		if (!asset)
		{
			materials.emplace(baseId, material);

			continue;
		}

		ParseMaterialTextureExportInfo(material.textures, material.asset, exportPath, eTextureExportName::TXTR_NAME_TEXT, useFullPaths);

		materials.emplace(baseId, material);
	}

	// [rika]: don't export material textures if it's not enabled
	if (!g_rsxSettings.exportMaterialTextures)
		return;

	// [rika]: export material textures
	std::atomic<uint32_t> remainingMaterials = 0; // we don't actually need thread safe here
	const ProgressBarEvent_t* const materialExportProgress = g_pImGuiHandler->AddProgressBarEvent("Exporting Materials..", static_cast<uint32_t>(materials.size()), &remainingMaterials, true);

	// [rika]: so we don't export textures per lod, we should exclude skins
	// todo: move this into the base function, don't export if raw
	for (const auto& it : materials)
	{
		++remainingMaterials;

		const ModelMaterialExport_t& material = it.second;

		if (!material.asset)
			continue;

		const MaterialAsset* const matlAsset = material.asset;

		// skip this material if it has no textures
		if (matlAsset->txtrAssets.size() == 0ull)
			continue;

		// enable exporting other image formats! (would if blender didn't smell)
		// [rika]: the extension also needs to be altered in model export formats if we do this!
		ExportMaterialTextures(eTextureExportSetting::PNG_HM, matlAsset, material.textures); // NOTE: LOOK INTO MAKING A FOLDER PER MATERIAL ?

	}
	g_pImGuiHandler->FinishProgressBarEvent(materialExportProgress);
}

// [rika]: todo also fix this up
// export parsed data to rmax
bool ExportModelRMAX(const ModelParsedData_t* const parsedData, std::filesystem::path& exportPath)
{
	std::string fileNameBase = exportPath.stem().string();
	const std::filesystem::path filePath(exportPath.parent_path());

	rmax::RMAXExporter rmaxFile(filePath, fileNameBase.c_str(), fileNameBase.c_str());

	// do bones
	rmaxFile.ReserveBones(parsedData->bones.size());
	for (auto& bone : parsedData->bones)
		rmaxFile.AddBone(bone.name, bone.parent, bone.pos, bone.quat, bone.scale);

	// [rika]: model is skin and bones, no meat
	if (parsedData->lods.size() == 0)
	{
		const std::string tmpName(std::format("{}.rmax", fileNameBase));
		rmaxFile.SetName(tmpName.c_str());

		rmaxFile.ToFile();

		return true;
	}

	const std::filesystem::path texturePath(std::format("{}/{}", filePath.string(), fileNameBase)); // todo, remove duplicate code
	std::unordered_map<int, ModelMaterialExport_t> materials;
	HandleModelMaterials(parsedData, materials, texturePath);

	// [rika]: now we parse lods
	for (size_t lodIdx = 0; lodIdx < parsedData->lods.size(); lodIdx++)
	{
		const ModelLODData_t& lodData = parsedData->lods.at(lodIdx);

		const std::string tmpName = std::format("{}_LOD{}.rmax", fileNameBase, lodIdx);
		rmaxFile.SetName(tmpName.c_str());		

		// do materials
		rmaxFile.ReserveMaterials(parsedData->materials.size());
		for (size_t matIdx = 0; matIdx < parsedData->materials.size(); matIdx++)
		{
			const ModelMaterialData_t& materialData = parsedData->materials.at(matIdx);
			const int materialId = static_cast<int>(matIdx);

			// [rika]: if it's unloaded, or unused we will just write a stub material
			if (!materialData.asset)
			{
				rmaxFile.AddMaterial(materialData.name);
				continue;
			}

			assert(materialData.GetMaterialAsset());
			const MaterialAsset* const matlAsset = materialData.GetMaterialAsset();
			rmaxFile.AddMaterial(matlAsset->name);

			// [rika]: write stub material (unused material)
			if (!materials.contains(materialId) || !matlAsset->resourceBindings.size())
				continue;

			const ModelMaterialExport_t& materialExport = materials.find(materialId)->second;

			rmax::RMAXMaterial* const matl = rmaxFile.GetMaterialLast();

			for (const TextureAssetEntry_t& entry : matlAsset->txtrAssets)
			{
				// [rika]: we don't have a resource binding or we don't have a name for the texture
				if (!matlAsset->resourceBindings.count(entry.index) || !materialExport.textures.contains(entry.index))
					continue;

				const std::string resource = matlAsset->resourceBindings.find(entry.index)->second.name;

				// [rika]: do we need this resource?
				if (!rmax::s_TextureTypeMap.count(resource))
					continue;

				const MaterialTextureExportInfo_s& info = materialExport.textures.find(entry.index)->second;
				const std::string path = std::format("{}/{}", info.exportPath.string(), info.exportName);

				matl->AddTexture(path.c_str(), rmax::s_TextureTypeMap.find(resource)->second);
			}
		}

		// do models
		rmaxFile.ReserveCollections(lodData.models.size());
		rmaxFile.ReserveMeshes(lodData.meshes.size());
		rmaxFile.ReserveVertices(lodData.vertexCount, lodData.texcoordsPerVert, lodData.weightsPerVert);
		rmaxFile.ReserveIndices(lodData.indexCount);
		for (auto& model : lodData.models)
		{
			if (!model.meshCount)
				continue;

			rmaxFile.AddCollection(model.name.c_str(), model.meshCount);

			for (uint32_t meshIdx = 0; meshIdx < model.meshCount; meshIdx++)
			{
				const ModelMeshData_t& meshData = model.meshes[meshIdx];

				assertm(materials.contains(meshData.materialId), "material should be parsed as it is used");
				const ModelMaterialExport_t& material = materials.find(meshData.materialId)->second;

				assertm(meshData.meshVertexDataIndex != invalidNoodleIdx, "mesh data hasn't been parsed ??");

				std::unique_ptr<char[]> parsedVertexDataBuf = parsedData->meshVertexData.getIdx(meshData.meshVertexDataIndex);
				const CMeshData* const parsedVertexData = reinterpret_cast<CMeshData*>(parsedVertexDataBuf.get());

				rmaxFile.AddMesh(static_cast<int16_t>(rmaxFile.CollectionCount() - 1), static_cast<int16_t>(material.id), meshData.texcoordCount, meshData.texcoodIndices, (meshData.rawVertexLayoutFlags & VERT_COLOR));

				rmax::RMAXMesh* const mesh = rmaxFile.GetMeshLast();

				// data parsing
				for (uint32_t i = 0; i < meshData.vertCount; i++)
				{
					const Vertex_t* const vertData = &parsedVertexData->GetVertices()[i];

					Vector normal;
					Vector tangent;
					vertData->normalPacked.UnpackNormal(normal, tangent);

					mesh->AddVertex(vertData->position, normal);

					if (meshData.rawVertexLayoutFlags & VERT_COLOR)
						mesh->AddColor(vertData->color);

					for (uint16_t texcoordIdx = 0; texcoordIdx < meshData.texcoordCount; texcoordIdx++)
						mesh->AddTexcoord(*vertData->GetTexcoordForVertex(texcoordIdx, meshData.texcoordCount, parsedVertexData->GetTexcoords(), i));

					for (uint32_t weightIdx = 0; weightIdx < vertData->weightCount; weightIdx++)
					{
						const VertexWeight_t* const weight = &parsedVertexData->GetWeights()[vertData->weightIndex + weightIdx];
						mesh->AddWeight(i, weight->bone, weight->weight);
					}
				}

				for (uint32_t i = 0; i < meshData.indexCount; i += 3)
					mesh->AddIndice(parsedVertexData->GetIndices()[i], parsedVertexData->GetIndices()[i + 1], parsedVertexData->GetIndices()[i + 2]);
			}
		}

		rmaxFile.ToFile();
		rmaxFile.ResetMeshData();
	}

	return true;
}

// export parsed data to cast
// [rika]: todo rewrite this soon tm (it is so bad)
bool ExportModelCast(const ModelParsedData_t* const parsedData, std::filesystem::path& exportPath, const uint64_t guid)
{
	std::string fileNameBase = exportPath.stem().string();

	// [rika]: build the skeleton once, and reuse it
	cast::CastNode skelNode(cast::CastId::Skeleton, RTech::StringToGuid(fileNameBase.c_str()));
	{
		const size_t boneCount = parsedData->bones.size();
		skelNode.ReserveChildren(boneCount);

		// uses hashes for lookup, still gets bone parents by index :clown:
		for (size_t i = 0; i < boneCount; i++)
		{
			const ModelBone_t& boneData = parsedData->bones.at(i);

			cast::CastNodeBone boneNode(&skelNode);
			boneNode.MakeBone(boneData.name, boneData.parent, &boneData.pos, &boneData.quat, false);
		}
	}
	const cast::CastNode& skelNodeConst = skelNode;

	// [rika]: model is skin and bones, no meat
	if (parsedData->lods.size() == 0)
	{
		const std::string tmpName(std::format("{}.cast", fileNameBase));
		exportPath.replace_filename(tmpName);

		cast::CastExporter cast(exportPath.string());

		// cast
		cast::CastNode* const rootNode = cast.GetChild(0); // we only have one root node, no hash
		cast::CastNode* const modelNode = rootNode->AddChild(cast::CastId::Model, guid);

		// [rika]: we can predict how big this vector needs to be, however resizing it will make adding new members a pain.
		const size_t modelChildrenCount = 1; // skeleton (one)
		modelNode->ReserveChildren(modelChildrenCount);

		// do skeleton
		modelNode->AddChild(skelNode); // one time use

		cast.ToFile();

		return true;
	}

	const std::filesystem::path texturePath(std::format("{}/{}", exportPath.parent_path().string(), fileNameBase)); // todo, remove duplicate code
	std::unordered_map<int, ModelMaterialExport_t> materials;
	HandleModelMaterials(parsedData, materials, texturePath);

	for (size_t lodIdx = 0; lodIdx < parsedData->lods.size(); lodIdx++)
	{
		const ModelLODData_t& lodData = parsedData->lods.at(lodIdx);

		std::string tmpName(std::format("{}_LOD{}.cast", fileNameBase, std::to_string(lodIdx)));
		exportPath.replace_filename(tmpName);

		cast::CastExporter cast(exportPath.string());

		// cast
		cast::CastNode* rootNode = cast.GetChild(0); // we only have one root node, no hash
		cast::CastNode* modelNode = rootNode->AddChild(cast::CastId::Model, guid);

		// [rika]: we can predict how big this vector needs to be, however resizing it will make adding new members a pain.
		const size_t modelChildrenCount = 1 + parsedData->materials.size() + lodData.meshes.size(); // skeleton (one), materials (varies), meshes (varies)
		modelNode->ReserveChildren(modelChildrenCount);

		// do skeleton
		modelNode->AddChild(skelNodeConst);

		// do materials
		for (const auto& it : materials)
		{
			const ModelMaterialExport_t& material = it.second;
			const ModelMaterialData_t* const materialData = &parsedData->materials.at(static_cast<size_t>(material.id));

			// [rika]: a cast material has at least two properties, name and material type (pbr in our case)
			cast::CastNode matlNode(cast::CastId::Material, 2, materialData->guid);
			matlNode.SetProperty(1, cast::CastPropertyId::String, static_cast<int>(cast::CastPropsMaterial::Type), "pbr", 1u);

			if (!material.asset)
			{
				matlNode.SetProperty(0, cast::CastPropertyId::String, static_cast<int>(cast::CastPropsMaterial::Name), GetStringAfterLastSlash(materialData->name), 1u); // unsure why it does this but we're rolling with it!
				modelNode->AddChild(matlNode);
				continue;
			}

			const MaterialAsset* const materialAsset = material.asset;

			matlNode.SetProperty(0, cast::CastPropertyId::String, static_cast<int>(cast::CastPropsMaterial::Name), GetStringAfterLastSlash(materialAsset->name), 1u);

			// [rika]: parse out our textures if we have bindings for them, don't if not
			// [rika]: exit early if no textures
			if (materialAsset->resourceBindings.empty())
			{
				modelNode->AddChild(matlNode);
				continue;
			}

			for (const TextureAssetEntry_t& entry : materialAsset->txtrAssets)
			{
				// [rika]: texture cannot be accurately identified, skip it
				if (!materialAsset->resourceBindings.count(entry.index))
					continue;

				const std::string resource(materialAsset->resourceBindings.find(entry.index)->second.name);

				// [rika]: texture type isn't supported, skip it
				if (!cast::s_TextureTypeMap.count(resource))
					continue;

				// [rika]: if MaterialTextureExportInfo_s doesn't exist for this texture it's not loaded, and by extension is not exported
				if (!material.textures.contains(entry.index))
				{
					// todo: store a name in parsed data
					//Log("Material %s for model %s did not have a valid texture pointer for res idx %i\n", materialAsset->name, name, entry.index);

					continue;
				}

				const MaterialTextureExportInfo_s& info = material.textures.find(entry.index)->second;

				const uint64_t textureGuid = entry.asset->data()->guid; // texture guid

				const cast::CastPropsMaterial matlTxtrProp = cast::s_TextureTypeMap.find(resource)->second;

				matlNode.AddProperty(cast::CastPropertyId::Integer64, static_cast<int>(matlTxtrProp), textureGuid);

				cast::CastNode fileNode(cast::CastId::File, 1, textureGuid);

				// [rika]: need to figure out how this works more
				const std::string filePath(std::format("{}/{}.png", fileNameBase, info.exportName));
				fileNode.SetString(filePath); // materials exported from models always use png, as blender support for dds is bad, todo: make it so we can use ALL formats!
				fileNode.SetProperty(0, cast::CastPropertyId::String, static_cast<int>(cast::CastPropsFile::Path), fileNode.GetString(), 1u);

				matlNode.AddChild(fileNode);
			}

			modelNode->AddChild(matlNode);
		}

		// !!! TODO !!!
		// this needs to get cleaned up, but if I don't push I am gonna be stuck forever

		// [rika]: a system to have these allocated to each node would be cleaner, but more expensive.
		struct DataPtrs_t {
			Vector* positions;
			Vector* normals;
			Color32* colors;
			Vector2D* texcoords; // all uvs stored here
			uint8_t* blendIndices;
			float* blendWeights;

			uint16_t* indices;
		};

		DataPtrs_t vertexData{ nullptr };

		//                                    Postion & Normal       Color
		constexpr size_t castMinSizePerVert = (sizeof(Vector) * 2) + sizeof(Color32);

		char* vertDataBlockBuf = new char[(castMinSizePerVert + (sizeof(Vector2D) * lodData.texcoordsPerVert)) * lodData.vertexCount] {};

		vertexData.positions = reinterpret_cast<Vector*>(vertDataBlockBuf);
		vertexData.normals = &vertexData.positions[lodData.vertexCount];
		vertexData.colors = reinterpret_cast<Color32*>(&vertexData.normals[lodData.vertexCount]); // discarded if unneeded
		vertexData.texcoords = reinterpret_cast<Vector2D*>(&vertexData.colors[lodData.vertexCount]);
		vertexData.blendIndices = new uint8_t[lodData.vertexCount * lodData.weightsPerVert]{};
		vertexData.blendWeights = new float[lodData.vertexCount * lodData.weightsPerVert] {};

		vertexData.indices = new uint16_t[lodData.indexCount];

		size_t curIndex = 0; // current index into vertex data
		size_t idxIndex = 0; // shit format

		// do meshes
		for (auto& modelData : lodData.models)
		{
			for (size_t i = 0; i < modelData.meshCount; i++)
			{
				const ModelMeshData_t& meshData = modelData.meshes[i];

				assertm(materials.contains(meshData.materialId), "material should be parsed as it is used");
				const uint64_t materialGuid = parsedData->materials.at(materials.find(meshData.materialId)->second.id).guid;

				assertm(meshData.meshVertexDataIndex != invalidNoodleIdx, "mesh data hasn't been parsed ??");

				std::unique_ptr<char[]> parsedVertexDataBuf = parsedData->meshVertexData.getIdx(meshData.meshVertexDataIndex);
				const CMeshData* const parsedVertexData = reinterpret_cast<CMeshData*>(parsedVertexDataBuf.get());

				std::string matl = nullptr != meshData.materialAsset ? GetStringAfterLastSlash(meshData.GetMaterialAsset()->name) : std::to_string(materialGuid);
				std::string meshName = std::format("{}_{}", modelData.name, matl);
				cast::CastNode meshNode(cast::CastId::Mesh, 1, RTech::StringToGuid(meshName.c_str())); // name

				// name, pos, normal, blendweight, blendindices, indices, uv count, max blends, material, texcoords, color
				const size_t meshPropertiesCount = 9 + meshData.texcoordCount + (meshData.rawVertexLayoutFlags & VERT_COLOR ? 1 : 0);
				meshNode.ReserveProperties(meshPropertiesCount);

				// works on files but not here, why?
				// update: now it works after changing how the string is formed, lovely.
				meshNode.SetString(meshName);
				meshNode.SetProperty(0, cast::CastPropertyId::String, static_cast<int>(cast::CastPropsMesh::Name), meshNode.GetString(), 1u);

				meshNode.AddProperty(cast::CastPropertyId::Vector3, static_cast<int>(cast::CastPropsMesh::Vertex_Postion_Buffer), &vertexData.positions[curIndex], meshData.vertCount);
				meshNode.AddProperty(cast::CastPropertyId::Vector3, static_cast<int>(cast::CastPropsMesh::Vertex_Normal_Buffer), &vertexData.normals[curIndex], meshData.vertCount);

				if (meshData.rawVertexLayoutFlags & VERT_COLOR)
					meshNode.AddProperty(cast::CastPropertyId::Integer32, static_cast<int>(cast::CastPropsMesh::Vertex_Color_Buffer), &vertexData.colors[curIndex], meshData.vertCount);

				// cast cries if we use the proper index
				for (int16_t texcoordIdx = 0; texcoordIdx < meshData.texcoordCount; texcoordIdx++)
					meshNode.AddProperty(cast::CastPropertyId::Vector2, static_cast<int>(cast::CastPropsMesh::Vertex_UV_Buffer), &vertexData.texcoords[(lodData.vertexCount * texcoordIdx) + curIndex], meshData.vertCount, true, texcoordIdx);

				meshNode.AddProperty(cast::CastPropertyId::Byte, static_cast<int>(cast::CastPropsMesh::Vertex_Weight_Bone_Buffer), &vertexData.blendIndices[lodData.weightsPerVert * curIndex], (meshData.vertCount * meshData.weightsPerVert));
				meshNode.AddProperty(cast::CastPropertyId::Float, static_cast<int>(cast::CastPropsMesh::Vertex_Weight_Value_Buffer), &vertexData.blendWeights[lodData.weightsPerVert * curIndex], (meshData.vertCount * meshData.weightsPerVert));

				// parse our vertices into the buffer, so cringe!
				for (uint32_t vertIdx = 0; vertIdx < meshData.vertCount; vertIdx++)
				{
					const Vertex_t& vert = parsedVertexData->GetVertices()[vertIdx];

					Vector tangent;

					vertexData.positions[curIndex + vertIdx] = vert.position;
					vert.normalPacked.UnpackNormal(vertexData.normals[curIndex + vertIdx], tangent);
					vertexData.colors[curIndex + vertIdx] = vert.color;

					for (uint16_t texcoordIdx = 0; texcoordIdx < meshData.texcoordCount; texcoordIdx++)
					{
						const Vector2D* const texcoord = vert.GetTexcoordForVertex(texcoordIdx, meshData.texcoordCount, parsedVertexData->GetTexcoords(), vertIdx);
						vertexData.texcoords[(lodData.vertexCount * texcoordIdx) + curIndex + vertIdx] = *texcoord;
					}

					for (uint32_t weightIdx = 0; weightIdx < vert.weightCount; weightIdx++)
					{
						vertexData.blendIndices[(lodData.weightsPerVert * curIndex) + (meshData.weightsPerVert * vertIdx) + weightIdx] = static_cast<uint8_t>(parsedVertexData->GetWeights()[vert.weightIndex + weightIdx].bone);
						vertexData.blendWeights[(lodData.weightsPerVert * curIndex) + (meshData.weightsPerVert * vertIdx) + weightIdx] = parsedVertexData->GetWeights()[vert.weightIndex + weightIdx].weight;
					}
				}

				// parse our indices into the buffer, and shuffle them! extra cringe!
				const uint32_t indexCount = meshData.indexCount;
				meshNode.AddProperty(cast::CastPropertyId::Short, static_cast<int>(cast::CastPropsMesh::Face_Buffer), &vertexData.indices[idxIndex], indexCount);

				for (uint32_t idxIdx = 0; idxIdx < indexCount; idxIdx += 3)
				{
					vertexData.indices[idxIndex + idxIdx] = parsedVertexData->GetIndices()[idxIdx + 2];
					vertexData.indices[idxIndex + idxIdx + 1] = parsedVertexData->GetIndices()[idxIdx + 1];
					vertexData.indices[idxIndex + idxIdx + 2] = parsedVertexData->GetIndices()[idxIdx];
				}

				meshNode.AddProperty(cast::CastPropertyId::Short, static_cast<int>(cast::CastPropsMesh::UV_Layer_Count), meshData.texcoordCount);
				meshNode.AddProperty(cast::CastPropertyId::Short, static_cast<int>(cast::CastPropsMesh::Max_Weight_Influence), meshData.weightsPerVert);

				meshNode.AddProperty(cast::CastPropertyId::Integer64, static_cast<int>(cast::CastPropsMesh::Material), materialGuid);

				modelNode->AddChild(meshNode);

				curIndex += meshData.vertCount;
				idxIndex += indexCount;
			}
		}

		cast.ToFile();

		// cleanup our allocated buffers
		delete[] vertDataBlockBuf;
		delete[] vertexData.blendIndices;
		delete[] vertexData.blendWeights;

		delete[] vertexData.indices;
	}

	return true;
}

// parse a Vertex_t into a smd vertex
inline void ParseVertexIntoSMD(const Vertex_t* const srcVert, const VertexWeight_t* const srcWeights, smd::Vertex* const vert, const bool isStaticProp, const uint32_t texcoordWidth = 1u, const Vector2D* const extraTexcoords = nullptr, const uint32_t vertexIndex = 0u)
{
	Vector tangent;
	vert->position = srcVert->position;
	srcVert->normalPacked.UnpackNormal(vert->normal, tangent);

	if (isStaticProp)
	{
		StaticPropFlipFlop(vert->position);
		StaticPropFlipFlop(vert->normal);
	}

	vert->numTexcoords = texcoordWidth > smd::maxTexcoords ? smd::maxTexcoords : texcoordWidth;
	for (uint32_t texcoordIdx = 0u; texcoordIdx < vert->numTexcoords; texcoordIdx++)
	{
		vert->texcoords[texcoordIdx] = *srcVert->GetTexcoordForVertex(texcoordIdx, texcoordWidth, extraTexcoords, vertexIndex);
		Vertex_t::InvertTexcoord(vert->texcoords[texcoordIdx]); // [rika]: the texcoord has to be inverted for proper recompile
	}	

	vert->numBones = srcVert->weightCount > smd::maxBoneWeights ? smd::maxBoneWeights : srcVert->weightCount;
	for (uint32_t weightIdx = 0u; weightIdx < vert->numBones; weightIdx++)
	{
		const VertexWeight_t* const weight = &srcWeights[srcVert->weightIndex + weightIdx];

		vert->bone[weightIdx] = weight->bone;
		vert->weight[weightIdx] = weight->weight;
	}
}

// export parsed data into smd files
bool ExportModelSMD(const ModelParsedData_t* const parsedData, std::filesystem::path& exportPath)
{
	std::string fileNameBase = exportPath.stem().string();
	const std::filesystem::path filePath(exportPath.parent_path());

	smd::CStudioModelData* const smd = new smd::CStudioModelData(filePath, parsedData->bones.size(), 1ull);

	// [rika]: initialize the nodes, and in this case the frames since we should only have one
	for (size_t i = 0; i < parsedData->bones.size(); i++)
	{
		const ModelBone_t& bone = parsedData->bones.at(i);
		const int ibone = static_cast<int>(i);

		smd->InitNode(bone.name, ibone, bone.parent);
		smd->InitFrameBone(0, ibone, bone.pos, bone.rot);
	}

	// [rika]: model is skin and bones, no meat
	if (parsedData->lods.size() == 0)
	{
		smd->SetName(fileNameBase);
		smd->Write();

		return true;
	}

	const std::filesystem::path texturePath(std::format("{}/{}", filePath.string(), fileNameBase)); // todo, remove duplicate code
	std::unordered_map<int, ModelMaterialExport_t> materials;
	HandleModelMaterials(parsedData, materials, texturePath);

	const bool isStaticProp = parsedData->studiohdr.flags & STUDIOHDR_FLAGS_STATIC_PROP ? true : false;

	CManagedBuffer* const buf = g_BufferManager.ClaimBuffer();

	for (size_t lodIdx = 0; lodIdx < parsedData->lods.size(); lodIdx++)
	{
		const ModelLODData_t& lod = parsedData->lods.at(lodIdx);

		for (const ModelModelData_t& model : lod.models)
		{
			std::string name(model.name);
			FixupExportLodNames(name, static_cast<int>(lodIdx));

			// unique prefix so we don't overwrite files
			name = std::format("{}_{}", fileNameBase, name);
			smd->SetName(name);

			for (uint32_t meshIdx = 0; meshIdx < model.meshCount; meshIdx++)
			{
				const ModelMeshData_t& meshData = lod.meshes.at(model.meshIndex + meshIdx);

				assertm(meshData.meshVertexDataIndex != invalidNoodleIdx, "mesh data hasn't been parsed ??");

				std::unique_ptr<char[]> parsedVertexDataBuf = parsedData->meshVertexData.getIdx(meshData.meshVertexDataIndex);
				const CMeshData* const parsedVertexData = reinterpret_cast<CMeshData*>(parsedVertexDataBuf.get());

				const uint16_t* const indices = parsedVertexData->GetIndices();
				const Vertex_t* const vertices = parsedVertexData->GetVertices();
				const VertexWeight_t* const weights = parsedVertexData->GetWeights();
				const Vector2D* const texcoords = parsedVertexData->GetTexcoords();

				// [rika]: add more triangles
				smd->AddMeshCapacity(meshData.vertCount, meshData.indexCount / 3u);

				const ModelMaterialData_t* const materialData = parsedData->pMaterial(meshData.materialId);

				// [rika]: making the choice to use the stored rmdl name here when possible, as that is what it was likely compiled with
				const char* material = materialData->GetName(true);
				assertm(material, "material name should always be valid");

				material = g_rsxSettings.exportModelMatsTruncated ? material : GetStringAfterLastSlash(material);

				for (uint32_t vertexIdx = 0; vertexIdx < meshData.vertCount; vertexIdx++)
				{
					smd::Vertex vertex;
					ParseVertexIntoSMD(&vertices[vertexIdx], weights, &vertex, isStaticProp, meshData.texcoordCount, texcoords, vertexIdx);

					smd->InitVertex(&vertex);
				}

				for (uint32_t indiceIdx = 0; indiceIdx < meshData.indexCount; indiceIdx += 3)
				{
					const uint16_t indice0 = indices[indiceIdx];
					const uint16_t indice1 = indices[indiceIdx + 1];
					const uint16_t indice2 = indices[indiceIdx + 2];

					// order of indices is odd
					smd->InitLocalTriangle(material, indice0, indice2, indice1);
				}
			}

			smd->Write(buf->Buffer(), managedBufferSize);
			smd->ResetMeshData();
		}
	}

	FreeAllocVar(smd);
	g_BufferManager.RelieveBuffer(buf);

	return true;
}

// export a seqdesc to rmax
bool ExportSeqDescRMAX(const ModelSeq_t* const seqdesc, std::filesystem::path& exportPath, const char* const skelName, const std::vector<ModelBone_t>* const bones)
{
	const std::string fileNameBase = exportPath.stem().string();
	const std::string skelNameBase = std::filesystem::path(skelName).stem().string();

	const size_t boneCount = bones->size();

	for (int animIdx = 0; animIdx < seqdesc->AnimCount(); animIdx++)
	{
		const std::string animName = std::format("{}{}", fileNameBase.c_str(), animIdx);

		const std::string tmpName = std::format("{}.rmax", animName);
		exportPath.replace_filename(tmpName);

		rmax::RMAXExporter rmaxFile(exportPath, fileNameBase.c_str(), fileNameBase.c_str());

		// do bones
		rmaxFile.ReserveBones(boneCount);
		for (auto& bone : *bones)
			rmaxFile.AddBone(bone.name, bone.parent, bone.pos, bone.quat, bone.scale);

		const ModelAnim_t* const animdesc = seqdesc->anims + animIdx; // check flag 0x20000

		uint16_t animFlags = 0;

		if (animdesc->flags & eStudioAnimFlags::ANIM_DELTA) // delta flag
			animFlags |= rmax::AnimFlags_t::ANIM_DELTA;

		// [rika]: not touching this for now since we really don't care about empty bones on types not for re import
		if (!(animdesc->flags & eStudioAnimFlags::ANIM_VALID) || animdesc->parsedBufferIndex == invalidNoodleIdx)
			animFlags |= rmax::AnimFlags_t::ANIM_EMPTY;

		rmaxFile.AddAnim(animName.c_str(), static_cast<uint16_t>(animdesc->numframes), animdesc->fps, animFlags, boneCount);
		rmax::RMAXAnim* const anim = rmaxFile.GetAnimLast();

		if (anim->GetFlags() & rmax::AnimFlags_t::ANIM_EMPTY)
		{
			rmaxFile.ToFile();

			continue;
		}

		const std::unique_ptr<char[]> noodle = seqdesc->parsedData.getIdx(animdesc->parsedBufferIndex);
		CAnimData animData(noodle.get());

		for (int i = 0; i < boneCount; i++)
		{
			const uint8_t flags = animData.GetFlag(i);

			anim->SetTrack(flags, static_cast<uint16_t>(i));
			rmax::RMAXAnimTrack* const track = anim->GetTrack(i);

			const Vector* pos = nullptr;
			const Quaternion* q = nullptr;
			const Vector* scale = nullptr;

			if (flags & CAnimDataBone::ANIMDATA_POS)
				pos = animData.GetBonePosForFrame(i, 0);

			if (flags & CAnimDataBone::ANIMDATA_ROT)
				q = animData.GetBoneQuatForFrame(i, 0);

			if (flags & CAnimDataBone::ANIMDATA_SCL)
				scale = animData.GetBoneScaleForFrame(i, 0);

			for (int frameIdx = 0; frameIdx < animdesc->numframes; frameIdx++)
			{
				track->AddFrame(frameIdx, &pos[frameIdx], &q[frameIdx], &scale[frameIdx]);
			}
		}

		rmaxFile.ToFile();
	}

	return true;
}

// export a seq desc to cast
bool ExportSeqDescCast(const ModelSeq_t* const seqdesc, std::filesystem::path& exportPath, const char* const skelName, const std::vector<ModelBone_t>* const bones, const uint64_t guid)
{
	const std::string fileNameBase = exportPath.stem().string();
	const std::string skelNameBase = std::filesystem::path(skelName).stem().string();

	const size_t boneCount = bones->size();

	for (int animIdx = 0; animIdx < seqdesc->AnimCount(); animIdx++)
	{
		const ModelAnim_t* const animdesc = seqdesc->anims + animIdx;

		const std::string tmpName(std::format("{}_{}.cast", fileNameBase, std::to_string(animIdx)));
		exportPath.replace_filename(tmpName);

		cast::CastExporter cast(exportPath.string());

		// cast
		cast::CastNode* const rootNode = cast.GetChild(0); // we only have one root node, no hash
		cast::CastNode* const animNode = rootNode->AddChild(cast::CastId::Animation, guid);

		// [rika]: we can predict how big this vector needs to be, however resizing it will make adding new members a pain.
		const size_t animChildrenCount = 1 + (boneCount * 7); // skeleton (one), curve per bone per data type
		animNode->ReserveChildren(animChildrenCount);
		animNode->ReserveProperties(2); // framerate, looping

		animNode->AddProperty(cast::CastPropertyId::Float, static_cast<int>(cast::CastPropsAnimation::Framerate), FLOAT_AS_UINT(animdesc->fps));
		animNode->AddProperty(cast::CastPropertyId::Byte, static_cast<int>(cast::CastPropsAnimation::Looping), animdesc->flags & eStudioAnimFlags::ANIM_LOOPING ? true : false);

		// do skeleton
		{
			// it would be more ideal to just feed it bones, but I don't want to deal with that mess of functions currently
			cast::CastNode* const skelNode = animNode->AddChild(cast::CastId::Skeleton, RTech::StringToGuid(fileNameBase.c_str()));
			skelNode->ReserveChildren(boneCount);

			// uses hashes for lookup, still gets bone parents by index :clown:
			for (size_t i = 0; i < boneCount; i++)
			{
				const ModelBone_t* const boneData = &bones->at(i);

				cast::CastNodeBone boneNode(skelNode);
				boneNode.MakeBone(boneData->name, boneData->parent, &boneData->pos, &boneData->quat, false);
			}
		}

		// [rika]: not touching this for now since we really don't care about empty bones on types not for re import
		if (!(animdesc->flags & eStudioAnimFlags::ANIM_VALID) || animdesc->parsedBufferIndex == invalidNoodleIdx)
		{
			cast.ToFile();

			continue;
		}

		const std::unique_ptr<char[]> noodle = seqdesc->parsedData.getIdx(animdesc->parsedBufferIndex);
		CAnimData animData(noodle.get());

		const cast::CastPropsCurveMode curveMode = animdesc->flags & eStudioAnimFlags::ANIM_DELTA ? cast::CastPropsCurveMode::MODE_ADDITIVE : cast::CastPropsCurveMode::MODE_ABSOLUTE;

		// setup the stupid key frame buffer thing that cast curves use
		cast::CastPropertyId frameBufferId;
		void* const frameBuffer = cast::CastNodeCurve::MakeCurveKeyFrameBuffer(static_cast<size_t>(animdesc->numframes), frameBufferId);

		Vector deltaPos(0.0f, 0.0f, 0.0f);
		Quaternion deltaQuat(0.0f, 0.0f, 0.0f, 1.0f);
		Vector deltaScale(1.0f, 1.0f, 1.0f);

		for (int i = 0; i < boneCount; i++)
		{
			const ModelBone_t* const boneData = &bones->at(i);

			// parsed data
			const uint8_t flags = animData.GetFlag(i);

			// weight for delta anims
			const float animWeight = seqdesc->weight(i);

			if (flags & CAnimDataBone::ANIMDATA_POS)
			{
				cast::CastNodeCurve curveNode(animNode);
				curveNode.MakeCurveVector(boneData->name, animData.GetBonePosForFrame(i, 0), static_cast<size_t>(animdesc->numframes), frameBuffer, static_cast<size_t>(animdesc->numframes), cast::CastPropsCurveValue::POS_X, curveMode, animWeight);
			}
			else
			{
				const Vector* const track = animdesc->flags & eStudioAnimFlags::ANIM_DELTA ? &deltaPos : &boneData->pos;

				cast::CastNodeCurve curveNode(animNode);
				curveNode.MakeCurveVector(boneData->name, track, 1ull, frameBuffer, static_cast<size_t>(animdesc->numframes), cast::CastPropsCurveValue::POS_X, curveMode, animWeight);
			}

			if (flags & CAnimDataBone::ANIMDATA_ROT)
			{
				cast::CastNodeCurve curveNode(animNode);
				curveNode.MakeCurveQuaternion(boneData->name, animData.GetBoneQuatForFrame(i, 0), static_cast<size_t>(animdesc->numframes), frameBuffer, static_cast<size_t>(animdesc->numframes), curveMode, animWeight);
			}
			else
			{
				const Quaternion* const track = animdesc->flags & eStudioAnimFlags::ANIM_DELTA ? &deltaQuat : &boneData->quat;

				cast::CastNodeCurve curveNode(animNode);
				curveNode.MakeCurveQuaternion(boneData->name, track, 1ull, frameBuffer, static_cast<size_t>(animdesc->numframes), curveMode, animWeight);
			}

			// check if the sequence has scale data.
			if (seqdesc->flags & 0x20000)
			{
				if (flags & CAnimDataBone::ANIMDATA_SCL)
				{
					cast::CastNodeCurve curveNode(animNode);
					curveNode.MakeCurveVector(boneData->name, animData.GetBoneScaleForFrame(i, 0), static_cast<size_t>(animdesc->numframes), frameBuffer, static_cast<size_t>(animdesc->numframes), cast::CastPropsCurveValue::SCL_X, curveMode, animWeight);
				}
				else
				{
					const Vector* track = animdesc->flags & eStudioAnimFlags::ANIM_DELTA ? &deltaScale : &boneData->scale;

					cast::CastNodeCurve curveNode(animNode);
					curveNode.MakeCurveVector(boneData->name, track, 1ull, frameBuffer, static_cast<size_t>(animdesc->numframes), cast::CastPropsCurveValue::SCL_X, curveMode, animWeight);
				}
			}
		}

		cast.ToFile();

		delete[] frameBuffer;
	}

	return true;
}

bool ExportSeqDescSMD(const ModelSeq_t* const seqdesc, std::filesystem::path& exportPath, const char* const skelName, const std::vector<ModelBone_t>* const bones)
{
	const std::string fileNameBase = exportPath.stem().string();
	const std::string skelNameBase = std::filesystem::path(skelName).stem().string();

	const size_t boneCount = bones->size();

	smd::CStudioModelData* const smd = new smd::CStudioModelData(exportPath.parent_path(), bones->size(), 1ull);

	// [rika]: initialize the nodes
	for (size_t i = 0; i < boneCount; i++)
	{
		const ModelBone_t& bone = bones->at(i);

		smd->InitNode(bone.name, static_cast<int>(i), bone.parent);
	}

	const Vector deltaPos(0.0f, 0.0f, 0.0f);
	const Quaternion deltaQuat(0.0f, 0.0f, 0.0f, 1.0f);

	CManagedBuffer* const buf = g_BufferManager.ClaimBuffer();

	for (int animIdx = 0; animIdx < seqdesc->AnimCount(); animIdx++)
	{
		const ModelAnim_t* const animdesc = seqdesc->anims + animIdx;

		assertm(animdesc->name, "name was nullptr");
		std::string animname(animdesc->pszName());
		animname.append(".smd");

		smd->ResetFrameData(static_cast<size_t>(animdesc->numframes));
		smd->SetName(animname);

		if (animdesc->parsedBufferIndex == invalidNoodleIdx)
		{
			smd->Write();

			continue;
		}

		const std::unique_ptr<char[]> noodle = seqdesc->parsedData.getIdx(animdesc->parsedBufferIndex);
		CAnimData animData(noodle.get());

		for (int frame = 0; frame < animdesc->numframes; frame++)
		{
			for (int bone = 0; bone < boneCount; bone++)
			{
				const ModelBone_t* const boneData = &bones->at(bone);

				// parsed data
				const uint8_t flags = animData.GetFlag(bone);

				const Vector* pos = nullptr;
				const Quaternion* q = nullptr;

				if (flags & CAnimDataBone::ANIMDATA_POS)
					pos = animData.GetBonePosForFrame(bone, frame);
				else
					pos = animdesc->flags & eStudioAnimFlags::ANIM_DELTA ? &deltaPos : &boneData->pos;

				if (flags & CAnimDataBone::ANIMDATA_ROT)
					q = animData.GetBoneQuatForFrame(bone, frame);
				else
					q = animdesc->flags & eStudioAnimFlags::ANIM_DELTA ? &deltaQuat : &boneData->quat;

				assertm(pos, "should not be nullptr");
				assertm(q, "should not be nullptr");

				const RadianEuler rot(*q);

				smd->InitFrameBone(frame, bone, *pos, rot);
			}
		}

		smd->Write(buf->Buffer(), managedBufferSize);
	}

	FreeAllocVar(smd);
	g_BufferManager.RelieveBuffer(buf);

	return true;
}

bool ExportSeqDesc(const int setting, const ModelSeq_t* const seqdesc, std::filesystem::path& exportPath, const char* const skelName, const std::vector<ModelBone_t>* const bones, const uint64_t guid)
{
	switch (setting)
	{

	case eAnimSeqExportSetting::ANIMSEQ_CAST:
	{
		return ExportSeqDescCast(seqdesc, exportPath, skelName, bones, guid);
	}
	case eAnimSeqExportSetting::ANIMSEQ_RMAX:
	{
		return ExportSeqDescRMAX(seqdesc, exportPath, skelName, bones);
	}
	case eAnimSeqExportSetting::ANIMSEQ_SMD:
	{
		return ExportSeqDescSMD(seqdesc, exportPath, skelName, bones);
	}
	case eAnimSeqExportSetting::ANIMSEQ_RSEQ:
	{
		return false;
	}
	default:
	{
		assertm(false, "Export setting is not handled.");
		return false;
	}
	}
}

#if defined(HAS_BONED_MODELS)

void CalcMatrixForBone_Unparented(const DXBone_t& bone, XMMATRIX& matOut)
{
	XMVECTOR quat = { bone.quat.x, bone.quat.y, bone.quat.z, bone.quat.w };

	XMMATRIX rotationMatrix = XMMatrixRotationQuaternion(quat);

	XMMATRIX translationMatrix = XMMatrixTranslation(bone.pos.x, bone.pos.y, bone.pos.z);

	// srt ong fr
	matOut = XMMatrixMultiply(XMMatrixMultiply(XMMatrixScaling(bone.scale.x, bone.scale.y, bone.scale.z), rotationMatrix), translationMatrix);
}

//
// PREVIEWDATA
//
void UpdateModelBoneMatrix(CDXDrawData* const drawData)
{
	ID3D11DeviceContext* const ctx = g_dxHandler->GetDeviceContext();

	D3D11_MAPPED_SUBRESOURCE resource;
	HRESULT hr = ctx->Map(
		drawData->boneMatrixBuffer, 0,
		D3D11_MAP_WRITE_DISCARD, 0,
		&resource
	);

	assert(SUCCEEDED(hr));

	if (FAILED(hr))
		return;

	XMMATRIX* boneArray = reinterpret_cast<XMMATRIX*>(resource.pData);

	std::vector<XMMATRIX> tempBoneMatrices(drawData->bones.size());

	int i = 0;
	for (const DXBone_t& bone : drawData->bones)
	{
		CalcMatrixForBone_Unparented(bone, tempBoneMatrices[i]);
		
		// now handle parenting
		if (bone.parent != -1)
			tempBoneMatrices[i] = XMMatrixMultiply(tempBoneMatrices[i], tempBoneMatrices[bone.parent]);

		const XMMATRIX inverseBindMat = drawData->boneInverseBindMatrices.at(i);
		const XMMATRIX multiplied = XMMatrixMultiply(inverseBindMat, tempBoneMatrices[i]);

		boneArray[i] = XMMatrixTranspose(multiplied);

		//if (bone.parent != -1)
		//{
		//	XMVECTOR scale;
		//	XMVECTOR pos;
		//	XMVECTOR rot;
		//	XMMatrixDecompose(&scale, &rot, &pos, tempBoneMatrices[i]);

		//	XMVECTOR parentPos;
		//	XMMatrixDecompose(&scale, &rot, &parentPos, tempBoneMatrices[bone.parent]);

		//	constexpr uint32_t boneColour = 0xFF0000FF;

		//	Vector pos1 = pos;
		//	Vector pos2 = parentPos;
		//	drawData->DrawLine(Vector(pos1.x, pos1.z, pos1.y), Vector(pos2.x, pos2.z, pos2.y), boneColour, true, 1.f, 0.f);
		//}

		i++;
	}

	ctx->Unmap(drawData->boneMatrixBuffer, 0);
}

// Calculates a matrix that translates from model-space to joint-space.
// This is only calculated once when the model is first selected for preview, as it's completely useless for export
void CalculateBonesInverseBindMatrix(CDXDrawData* const drawData)
{
	drawData->boneInverseBindMatrices.resize(drawData->bones.size());

	std::vector<XMMATRIX> tempBoneMatrices(drawData->bones.size());

	int i = 0;
	for (const DXBone_t& bone : drawData->bones)
	{
		CalcMatrixForBone_Unparented(bone, tempBoneMatrices[i]);

		// now handle parenting
		if (bone.parent != -1)
			tempBoneMatrices[i] = XMMatrixMultiply(tempBoneMatrices[i], tempBoneMatrices[bone.parent]);

		XMVECTOR determinant;
		drawData->boneInverseBindMatrices[i] = XMMatrixInverse(&determinant, tempBoneMatrices[i]);

		assert(determinant.m128_f32[0] != 0 && determinant.m128_f32[1] != 0 && determinant.m128_f32[2] != 0);

		i++;
	}
}

void InitModelBoneMatrix(CDXDrawData* const drawData, ModelParsedData_t* const parsedData)
{
	ID3D11Device* const device = g_dxHandler->GetDevice();

	D3D11_BUFFER_DESC desc{};

	desc.ByteWidth = static_cast<UINT>(parsedData->bones.size()) * sizeof(XMMATRIX);
	desc.StructureByteStride = sizeof(XMMATRIX);

	// make sure this buffer can be updated every frame
	desc.Usage = D3D11_USAGE_DYNAMIC;
	desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;

	// pixel is a const buffer
	desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
	desc.MiscFlags = D3D11_RESOURCE_MISC_BUFFER_STRUCTURED;

	HRESULT hr = device->CreateBuffer(&desc, NULL, &drawData->boneMatrixBuffer);

#if defined(ASSERTS)
	assert(!FAILED(hr));
#else
	if (FAILED(hr))
		return;
#endif

	// SRV
	D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc{};
	srvDesc.Format = DXGI_FORMAT_UNKNOWN;
	srvDesc.ViewDimension = D3D11_SRV_DIMENSION_BUFFER;
	srvDesc.Buffer.FirstElement = 0;
	srvDesc.Buffer.NumElements = static_cast<UINT>(parsedData->bones.size());

	hr = device->CreateShaderResourceView(drawData->boneMatrixBuffer, &srvDesc, &drawData->boneMatrixSRV);

#if defined(ASSERTS)
	assert(!FAILED(hr));
#else
	if (FAILED(hr))
		return;
#endif

	drawData->bones.resize(parsedData->bones.size());

	size_t i = 0;
	for (auto& bone : parsedData->bones)
	{
		drawData->bones.at(i) = DXBone_t(bone.name, bone.pos, bone.quat, bone.scale, bone.parent);

		i++;
	}

	CalculateBonesInverseBindMatrix(drawData);

	// Initial update for the bone matrices
	UpdateModelBoneMatrix(drawData);
}

static void ModelPreview_ResetPose(const ModelParsedData_t* const parsedData, CDXDrawData* const drawData)
{
	// get our bones back!
	for (size_t i = 0; i < drawData->bones.size(); i++)
	{
		const ModelBone_t& originalBone = parsedData->bones.at(i);
		DXBone_t& bone = drawData->bones.at(i);

		bone.pos = originalBone.pos;
		bone.quat = originalBone.quat;
		bone.scale = originalBone.scale;
	}
}

static void ModelPreview_AnimFrame(AnimState_t* const state, const SeqPreviewEntry_t* const entry, const ModelParsedData_t* const parsedData, CDXDrawData* const drawData, const float dt)
{
	const ModelAnim_t* const animdesc = entry->seqdesc->Anim(state->selectedAnimIndex);

	// update active data when our selection changes
	if (state->activeSeqIdx != state->selectedSeqIndex || state->activeAnimIdx != state->selectedAnimIndex || state->boneRemap.size() != drawData->bones.size())
	{
		state->frame = 0.0f;

		state->activeSeqIdx = state->selectedSeqIndex;
		state->activeAnimIdx = state->selectedAnimIndex;
		state->dcmpNoodle.reset();

		if ((animdesc->flags & eStudioAnimFlags::ANIM_VALID) && animdesc->parsedBufferIndex != invalidNoodleIdx)
			state->dcmpNoodle = entry->seqdesc->parsedData.getIdx(animdesc->parsedBufferIndex);

		// placeholder value of -1 means that the bone has no mapping to the animation's skeleton
		state->boneRemap.assign(drawData->bones.size(), -1);

		const std::vector<ModelBone_t>& srcBones = *entry->srcBones;

		// find each of our model's bones in the animation's skeleton
		// i thought that they would match up but apparently not so that's super fun!
		// i'm sure this isn't the right way of doing it but it's good enough for now
		for (size_t i = 0; i < drawData->bones.size(); ++i)
		{
			for (size_t j = 0; j < srcBones.size(); ++j)
			{
				if (!_stricmp(drawData->bones.at(i).name, srcBones.at(j).name))
				{
					state->boneRemap.at(i) = static_cast<int>(j);
					break;
				}
			}
		}
	}

	// start from the non-animated position of the model
	ModelPreview_ResetPose(parsedData, drawData);

	// this check must be after resetting the pose so that we end up in a neutral position if there is no real anim data 
	if (!state->dcmpNoodle)
		return;

	const int finalFrameIdx = animdesc->numframes - 1;

	// if the player is paused or there's only one anim frame then dont advance any further
	if (state->playing && finalFrameIdx > 0)
	{
		state->frame += dt * animdesc->fps;

		// if we've hit the end of the anim!
		if (state->frame > finalFrameIdx)
		{
			state->frame = state->looping ? fmodf(state->frame, static_cast<float>(finalFrameIdx)) : finalFrameIdx;
			state->playing = state->looping; // if looping we r still playing
		}
	}
	
	// the cast rounds down to the current anim frame that we're using.
	// state->frame will contain a fractional part since we will almost definitely be running at >30fps (or animdesc->fps)
	const int currFrameIdx = std::clamp(static_cast<int>(state->frame), 0, finalFrameIdx);
	const int nextFrameIdx = std::min(currFrameIdx + 1, finalFrameIdx);

	// get the distance that we are between the two frames so we can (s)lerp the pos and scale
	const float frameFrac = std::clamp(state->frame - currFrameIdx, 0.0f, 1.0f);

	CAnimData animData(state->dcmpNoodle.get());

	for (size_t i = 0; i < drawData->bones.size(); i++)
	{
		// map the gpu's bone on to that of the skeleton that the anim uses
		const int boneIndex = state->boneRemap.at(i);

		// if boneindex is -1 then this bone is not in the anim's skeleton
		if (boneIndex < 0)
			continue;

		DXBone_t& bone = drawData->bones.at(i);
		const uint8_t flags = animData.GetFlag(boneIndex);

		if (flags & CAnimDataBone::ANIMDATA_POS)
		{
			const Vector& pos0 = *animData.GetBonePosForFrame(boneIndex, currFrameIdx);
			const Vector& pos1 = *animData.GetBonePosForFrame(boneIndex, nextFrameIdx);

			bone.pos = Vector::Lerp(pos0, pos1, frameFrac);
		}

		if (flags & CAnimDataBone::ANIMDATA_ROT)
		{
			Quaternion quat;
			QuaternionSlerp(*animData.GetBoneQuatForFrame(boneIndex, currFrameIdx), *animData.GetBoneQuatForFrame(boneIndex, nextFrameIdx), frameFrac, quat);

			bone.quat = quat;
		}

		if (flags & CAnimDataBone::ANIMDATA_SCL)
		{
			const Vector& scale0 = *animData.GetBoneScaleForFrame(boneIndex, currFrameIdx);
			const Vector& scale1 = *animData.GetBoneScaleForFrame(boneIndex, nextFrameIdx);

			bone.scale = Vector::Lerp(scale0, scale1, frameFrac);
		}
	}
}

bool Preview_SequencesSection(ModelPreviewInfo_t* const info, const ModelParsedData_t* const parsedData, CDXDrawData* const drawData)
{
	AnimState_t& state = info->animState;

	bool refreshRequested = false;

	const SeqPreviewEntry_t* selectedSequenceEntry = nullptr;
	if (state.selectedSeqIndex >= 0 && state.selectedSeqIndex < static_cast<int>(info->sequences.size()))
	{
		selectedSequenceEntry = &info->sequences.at(state.selectedSeqIndex);

		state.selectedAnimIndex = std::clamp(state.selectedAnimIndex, 0, std::max(selectedSequenceEntry->seqdesc->AnimCount() - 1, 0));
	}
	else
		state.selectedSeqIndex = -1;

	if (ImGui::CollapsingHeader("Sequences (WIP)"))
	{
		ImGui::PushFont(NULL, 16.f);
		ImGui::TextDisabled("Animation preview is an unfinished feature that may have major visual issues");
		ImGui::PopFont();

		if (info->sequences.empty())
			ImGui::TextUnformatted("No sequences associated with this model.");
		else
		{
			// just in case the sequence data hasn't been parsed properly for whatever reason
			// return to the caller and let it know that we want new data!
			if (ImGui::Button("Refresh##AnimPreview"))
				refreshRequested = true;

			const char* const comboLabel = selectedSequenceEntry ? selectedSequenceEntry->name.c_str() : "(base pose)";

			if (ImGui::BeginCombo("Sequence##AnimPreview", comboLabel))
			{
				// if there's no selected seq then we have selected the base pose (non animated)
				if (ImGui::Selectable("(base pose)", selectedSequenceEntry == nullptr))
				{
					state.selectedSeqIndex = -1;
					state.Stop();

					selectedSequenceEntry = nullptr;
				}

				for (int i = 0; i < static_cast<int>(info->sequences.size()); i++)
				{
					const SeqPreviewEntry_t& entry = info->sequences.at(i);

					const bool isSelected = state.selectedSeqIndex == i;

					if (ImGui::Selectable(entry.name.c_str(), isSelected, !entry.parsed ? ImGuiSelectableFlags_Disabled : 0))
					{
						state.selectedSeqIndex = i;
						state.selectedAnimIndex = 0;

						state.Restart();

						selectedSequenceEntry = &info->sequences.at(state.selectedSeqIndex);
					}

					if (isSelected)
						ImGui::SetItemDefaultFocus();
				}

				ImGui::EndCombo();
			}

			if (selectedSequenceEntry)
			{
				if (selectedSequenceEntry->seqdesc->AnimCount() > 0)
				{
					const ModelSeq_t* const seqdesc = selectedSequenceEntry->seqdesc;
					const ModelAnim_t* const animdesc = seqdesc->Anim(state.selectedAnimIndex);

					ImGui::Text("Name: %s\nMetadata: %.1f fps, %i frames, %.3f seconds", animdesc->name, animdesc->fps, animdesc->numframes, animdesc->Duration());

					if (animdesc->flags & eStudioAnimFlags::ANIM_DELTA)
						ImGui::TextUnformatted("Unsupported delta anim; Preview unavailable");
					else if (!(animdesc->flags & eStudioAnimFlags::ANIM_VALID) || animdesc->parsedBufferIndex == invalidNoodleIdx)
						ImGui::TextUnformatted("Invalid anim data; Preview unavailable");

					// i don't know what this does ibsr
					if (seqdesc->AnimCount() > 1)
						ImGui::SliderInt("AnimIdx##AnimPreview", &state.selectedAnimIndex, 0, seqdesc->AnimCount() - 1);

					if (ImGui::Button(state.playing ? "Pause##AnimPreview" : "Play##AnimPreview"))
						state.TogglePlay();

					ImGui::SameLine();

					// if the player isnt playing then the user cannot click stop (obvs)
					ImGui::BeginDisabled(!state.playing && state.frame == 0.f);
					{
						if (ImGui::Button("Stop##AnimPreview")) state.Stop();
					}
					ImGui::EndDisabled();

					ImGui::SameLine();
					ImGui::Checkbox("Loop##AnimPreview", &state.looping);

					const int finalFrameIdx = animdesc->numframes > 0 ? animdesc->numframes - 1 : 0;

					ImGui::BeginDisabled(finalFrameIdx == 0);
					{
						ImGui::SliderFloat("Frame##AnimPreview", &state.frame, 0.0f, static_cast<float>(finalFrameIdx), "%.1f");
					}
					ImGui::EndDisabled();

					// when scrubbing, don't keep playing
					if (ImGui::IsItemActive())
						state.playing = false;

				} else ImGui::TextUnformatted("Sequence has no anims!");
			}
		}
	}

	// can run a frame if:
	// - not refreshing
	// - we have a sequence entry that:
	//   - is parsed
	//   - has animdescs
	//   - does not have a delta anim selected
	const bool canRunFrame = !refreshRequested &&
		(selectedSequenceEntry != nullptr
			&& selectedSequenceEntry->parsed
			&& selectedSequenceEntry->seqdesc->AnimCount() > 0
			&& !(selectedSequenceEntry->seqdesc->Anim(state.selectedAnimIndex)->flags & eStudioAnimFlags::ANIM_DELTA));

	if (!canRunFrame)
	{
		// if we aren't able to show a frame of the sequence while we have one selected then show the base pose instead
		// this happens when you click refresh while a sequence is active or swap to a sequence with no animdescs or a delta anim
		// (sorry rika i dunno how delta anims work)
		if (state.selectedSeqIndex != -1)
		{
			ModelPreview_ResetPose(parsedData, drawData);

			state.activeSeqIdx = -1;
			state.activeAnimIdx = -1;
			state.dcmpNoodle.reset(); // clear the noodle
		}
	} else ModelPreview_AnimFrame(&state, selectedSequenceEntry, parsedData, drawData, ImGui::GetIO().DeltaTime);

	// if we clicked refresh then the caller needs to reparse the data so let them know!
	return refreshRequested;
}
#endif

void* PreviewParsedData(ModelPreviewInfo_t* const info, ModelParsedData_t* const parsedData, char* const assetName, const uint64_t assetGUID, const bool firstFrameForAsset)
{
	// [rika]: set up CDXDrawData
	g_currentPreviewDrawData.CheckForMonitorChange();

	if (assetGUID != g_currentPreviewDrawData.guid || g_currentPreviewDrawData.GetDrawData() == nullptr || info->selectedLODLevel != g_currentPreviewDrawData.activeLODLevel)
	{
		g_currentPreviewDrawData.FreeDrawData();

		CDXDrawData* const drawData = new CDXDrawData();

		// this leaks mem?
		drawData->meshBuffers.resize(parsedData->lods.at(info->selectedLODLevel).meshes.size());
		drawData->modelName = assetName;
		drawData->dataType = CDXDrawData::DrawDataType_e::MODEL;

		CreateBuffersForModelDrawData(parsedData, drawData, info->selectedLODLevel);
		CreateBuffersForModelHitboxes(parsedData, drawData);

		g_currentPreviewDrawData.UpdateAssetInfo(drawData, assetGUID, info->selectedLODLevel);
	}

	CDXDrawData* const drawData = g_currentPreviewDrawData.GetDrawData();
	if (!drawData)
		return nullptr;

	drawData->vertexShader = g_dxHandler->GetShaderManager()->LoadShaderFromString("shaders/model_vs", s_PreviewVertexShader, eShaderType::Vertex);
	drawData->pixelShader = g_dxHandler->GetShaderManager()->LoadShaderFromString("shaders/model_ps", s_PreviewPixelShader, eShaderType::Pixel);

	// [rika]: do the preview stuff here!
	assertm(parsedData->lods.size() > 0, "no lods in preview?");
	const ModelLODData_t& lodData = parsedData->lods.at(info->selectedLODLevel);

	ImGui::Text("Bones: %llu", parsedData->bones.size());
	ImGui::Text("LODs: %llu", parsedData->lods.size());
	ImGui::Text("Local Sequences: %i", parsedData->NumLocalSeq());

	if (info->minLODIndex != info->maxLODIndex)
		ImGui::SliderScalar("LOD Level", ImGuiDataType_U8, &info->selectedLODLevel, &info->minLODIndex, &info->maxLODIndex);

	if (parsedData->skins.size())
	{
		ImGui::TextUnformatted("Skins:");
		ImGui::SameLine();

		// [rika]: cheat a little here since the first skin name should always be 'STUDIO_DEFAULT_SKIN_NAME'
		static const char* label = nullptr;
		if (firstFrameForAsset)
			label = STUDIO_DEFAULT_SKIN_NAME;

		if (ImGui::BeginCombo("##SKins", label, ImGuiComboFlags_NoArrowButton))
		{
			for (size_t i = 0; i < parsedData->skins.size(); i++)
			{
				const ModelSkinData_t& skin = parsedData->skins.at(i);

				const bool isSelected = info->selectedSkinIndex == i || (firstFrameForAsset && info->selectedSkinIndex == info->lastSelectedSkinIndex);

				if (ImGui::Selectable(skin.name, isSelected))
				{
					info->selectedSkinIndex = i;
					label = skin.name;
				}

				if (isSelected) ImGui::SetItemDefaultFocus();
			}

			ImGui::EndCombo();
		}
	}


	if (parsedData->bodyParts.size())
	{
		ImGui::TextUnformatted("Bodypart:");
		ImGui::SameLine();

		// [rika]: previous implemention was loading an invalid string upon loading different files without closing the tool
		static const char* bodypartLabel = nullptr;
		if (firstFrameForAsset)
			bodypartLabel = parsedData->bodyParts.at(0).GetNameCStr();

		if (ImGui::BeginCombo("##Bodypart", bodypartLabel, ImGuiComboFlags_NoArrowButton))
		{
			for (size_t i = 0; i < parsedData->bodyParts.size(); i++)
			{
				const ModelBodyPart_t& bodypart = parsedData->bodyParts.at(i);

				const bool isSelected = info->selectedBodypartIndex == i || (firstFrameForAsset && info->selectedBodypartIndex == info->lastSelectedBodypartIndex);

				if (ImGui::Selectable(bodypart.GetNameCStr(), isSelected))
				{
					info->selectedBodypartIndex = i;
					bodypartLabel = bodypart.GetNameCStr();
				}

				if (isSelected) ImGui::SetItemDefaultFocus();
			}

			ImGui::EndCombo();
		}

		// If the selected bodypart index is out of range, reset it to 0 to prevent an exception
		if (info->selectedBodypartIndex >= parsedData->bodyParts.size())
			info->selectedBodypartIndex = 0;

		if (info->selectedSkinIndex >= parsedData->skins.size())
			info->selectedSkinIndex = 0;

		if (info->selectedLODLevel >= parsedData->lods.size())
			info->selectedLODLevel = 0;

		g_rsxSettings.previewedSkinIndex = static_cast<int>(info->selectedSkinIndex);

		if (parsedData->bodyParts.at(info->selectedBodypartIndex).numModels > 1)
		{
			const ModelBodyPart_t& bodypart = parsedData->bodyParts.at(info->selectedBodypartIndex);
			size_t& selectedModelIndex = info->bodygroupModelSelected.at(info->selectedBodypartIndex);

			ImGui::TextUnformatted("Model:");
			ImGui::SameLine();

			static const char* label = nullptr;

			// update label if our bodypart changes
			if (info->selectedBodypartIndex != info->lastSelectedBodypartIndex || firstFrameForAsset)
				label = lodData.models.at(bodypart.modelIndex + selectedModelIndex).name.c_str();

			if (ImGui::BeginCombo("##Model", label, ImGuiComboFlags_NoArrowButton))
			{
				for (int i = 0; i < bodypart.numModels; i++)
				{
					const bool isSelected = selectedModelIndex == i;

					const char* tmp = lodData.models.at(bodypart.modelIndex + i).name.c_str();
					if (ImGui::Selectable(tmp, isSelected))
					{
						selectedModelIndex = static_cast<size_t>(i);
						label = tmp;
					}

					if (isSelected) ImGui::SetItemDefaultFocus();
				}

				ImGui::EndCombo();
			}
		}
	}

	// Load these first so we don't have to look them up for every mesh.
#if (ADVANCED_MODEL_PREVIEW)
	const CShader* const vertexShader = g_dxHandler->GetShaderManager()->LoadShader("C:/p4/rtech_utils_imgui/src/shaders/amp_vs", eShaderType::Vertex);
#else
	const CShader* const vertexShader = g_dxHandler->GetShaderManager()->LoadShaderFromString("shaders/model_vs", s_PreviewVertexShader, eShaderType::Vertex);
#endif
	const CShader* const pixelShader = g_dxHandler->GetShaderManager()->LoadShaderFromString("shaders/model_ps", s_PreviewPixelShader, eShaderType::Pixel);

	const ModelSkinData_t& skinData = parsedData->skins.at(info->selectedSkinIndex);
	for (size_t i = 0; i < lodData.meshes.size(); ++i)
	{
		const ModelMeshData_t& mesh = lodData.meshes.at(i);
		DXMeshDrawData_t* const meshDrawData = &drawData->meshBuffers[i];

		meshDrawData->indexFormat = DXGI_FORMAT_R16_UINT; // uint16_t
		meshDrawData->wireframe = false;

		// If this mesh belongs to a bodypart that is not selected, we should set it to be invisible
		drawData->meshBuffers[i].visible = parsedData->bodyParts[mesh.bodyPartIndex].IsPreviewEnabled();

		const ModelBodyPart_t& bodypart = parsedData->bodyParts[mesh.bodyPartIndex];
		const ModelModelData_t& model = lodData.models.at(bodypart.modelIndex + info->bodygroupModelSelected.at(mesh.bodyPartIndex));
		
		if (i >= model.meshIndex && i < model.meshIndex + model.meshCount)
			drawData->meshBuffers[i].visible = true;
		else
			drawData->meshBuffers[i].visible = false;

		// Placeholder shaders
		meshDrawData->pixelShader = pixelShader->Get<ID3D11PixelShader>();
		meshDrawData->vertexShader = vertexShader->Get<ID3D11VertexShader>();
		meshDrawData->inputLayout = vertexShader->GetInputLayout();
		meshDrawData->hasGameShaders = false;

		// the rest of this loop requires the material to be valid
		// so if it isn't just continue to the next iteration
		CPakAsset* const matlAsset = parsedData->materials.at(skinData.indices[mesh.materialId]).asset;
		if (!matlAsset)
			continue;

		const MaterialAsset* const matl = reinterpret_cast<MaterialAsset*>(matlAsset->extraData());

#if (ADVANCED_MODEL_PREVIEW)
		// If the material has a valid shaderset loaded, try and grab its shaders to use for this mesh's advanced model preview
		if (matl->shaderSetAsset)
		{
			ShaderSetAsset* const shaderSet = reinterpret_cast<ShaderSetAsset*>(matl->shaderSetAsset->extraData());

			if (shaderSet->vertexShaderAsset && shaderSet->pixelShaderAsset)
			{
				//ShaderAsset* const shadersetVS = reinterpret_cast<ShaderAsset*>(shaderSet->vertexShaderAsset->extraData());
				ShaderAsset* const shadersetPS = reinterpret_cast<ShaderAsset*>(shaderSet->pixelShaderAsset->extraData());

				//meshDrawData->vertexShader = vertexShader->vertexShader;
				meshDrawData->pixelShader = shadersetPS->pixelShader;

				//meshDrawData->inputLayout = vertexShader->vertexInputLayout;

				meshDrawData->hasGameShaders = true;
			}
		}
#endif

		if ((meshDrawData->textures.size() == 0 || info->lastSelectedSkinIndex != info->selectedSkinIndex) && matl)
		{
			meshDrawData->textures.clear();
			for (auto& texEntry : matl->txtrAssets)
			{
				if (texEntry.asset)
				{
					TextureAsset* txtr = reinterpret_cast<TextureAsset*>(texEntry.asset->extraData());

					for (auto& mip : txtr->mipArray | std::views::reverse)
					{
						// Find the highest mip in the texture's mip array that is loaded
						if (mip.isLoaded)
						{
							const std::shared_ptr<CTexture> highestTextureMip = CreateTextureFromMip(texEntry.asset, &mip, s_PakToDxgiFormat[txtr->imgFormat]);
							meshDrawData->textures.push_back({ texEntry.index, highestTextureMip });

							break;
						}
					}
				}
			}
		}
	}

#if defined(HAS_BONED_MODELS)
	// Map some (potentially incorrect) bone data
	if (!drawData->boneMatrixBuffer)
		InitModelBoneMatrix(drawData, parsedData);
#endif

	Preview_MapTransformsBuffer(drawData);
	Preview_MapModelInstanceBuffer(drawData);

	if (info->lastSelectedBodypartIndex != info->selectedBodypartIndex) UNLIKELY
		info->lastSelectedBodypartIndex = info->selectedBodypartIndex;

	if (info->lastSelectedSkinIndex != info->selectedSkinIndex) UNLIKELY
		info->lastSelectedSkinIndex = info->selectedSkinIndex;

	return drawData;
}

void PreviewAnimDesc(const ModelAnim_t* const animdesc, const int index)
{
	if (ImGui::TreeNodeEx(std::to_string(index).c_str(), ImGuiTreeNodeFlags_SpanAvailWidth))
	{
		ImGui::Text("Name: %s", animdesc->name);
		ImGui::Text("Flags: 0x%x", animdesc->flags);

		ImGui::Text("Frame Rate: %f", animdesc->fps);
		ImGui::Text("Frame Count: %i", animdesc->numframes);
		ImGui::Text("Duration: %.3f seconds", animdesc->Duration());

		ImGui::TreePop();
	}
}

void PreviewSeqDesc(const ModelSeq_t* const seqdesc)
{
	ImGui::Text("Label: %s", seqdesc->szlabel);
	ImGui::Text("Activity: %s", seqdesc->szactivityname);

	ImGui::Text("Flags: 0x%x", seqdesc->flags);

	if (!seqdesc->AnimCount())
		return;

	ImGui::TextUnformatted("Animations:");

	for (int i = 0; i < seqdesc->AnimCount(); i++)
	{
		PreviewAnimDesc(seqdesc->anims + i, i);
	}
}