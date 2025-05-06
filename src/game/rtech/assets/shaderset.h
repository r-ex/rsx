#pragma once
#include <game/rtech/cpakfile.h>
#include <game/rtech/utils/utils.h>
#include <game/rtech/assets/shader.h>

#define MAX_SHADERS_PER_ASSET_R2 32
// not a lot known about these structs
struct ShaderSetAssetHeader_v8_t
{
	uint64_t reserved_vftable;

	char* name;

	uint64_t reserved_inputFlags;

	uint16_t textureInputCounts[2];

	uint16_t numSamplers;

	uint16_t firstResourceBindPoint;
	uint16_t numResources;

	// reserved array for storing the index of input layout for each of the VS asset's shaders
	// set to 0xFF if there is no shader present for the respective index in this array
	uint8_t reserved_vsInputLayoutIds[MAX_SHADERS_PER_ASSET_R2];

	uint64_t vertexShader;
	uint64_t pixelShader;
};
static_assert(sizeof(ShaderSetAssetHeader_v8_t) == 88);
static_assert(offsetof(ShaderSetAssetHeader_v8_t, textureInputCounts) == 0x18);
static_assert(offsetof(ShaderSetAssetHeader_v8_t, reserved_vsInputLayoutIds) == 0x22);

struct ShaderSetAssetHeader_v11_t
{
	uint64_t reserved_vftable;

	char* name;

	uint64_t reserved_inputFlags;

	uint16_t textureInputCounts[2];

	uint16_t numSamplers;

	uint8_t firstResourceBindPoint;
	uint8_t numResources;

	// reserved array for storing the index of input layout for each of the VS asset's shaders
	uint8_t reserved_vsInputLayoutIds[16];

	uint64_t vertexShader;
	uint64_t pixelShader;
};
static_assert(sizeof(ShaderSetAssetHeader_v11_t) == 64);
static_assert(offsetof(ShaderSetAssetHeader_v11_t, reserved_vsInputLayoutIds) == 0x20);

// [rika]: newer versions of v13 use this header, seemingly omitting unk_40
struct ShaderSetAssetHeader_v12_t
{
	uint64_t reserved_vftable;

	char* name;

	uint64_t reserved_inputFlags;

	uint16_t textureInputCounts[2];

	uint16_t numSamplers;

	uint8_t firstResourceBindPoint;
	uint8_t numResources;

	// reserved array for storing the index of input layout for each of the VS asset's shaders
	uint8_t reserved_vsInputLayoutIds[32];

	uint64_t vertexShader;
	uint64_t pixelShader;
};
static_assert(sizeof(ShaderSetAssetHeader_v12_t) == 80);

struct ShaderSetAssetHeader_v13_t
{
	uint64_t reserved_vftable;

	char* name;

	uint64_t reserved_inputFlags;

	uint16_t textureInputCounts[2];

	uint16_t numSamplers;

	uint8_t firstResourceBindPoint;
	uint8_t numResources;

	// reserved array for storing the index of input layout for each of the VS asset's shaders
	// [rika] dunno what these are supposed to look like, please check this later rexx. they might've just extended this array
	uint8_t reserved_vsInputLayoutIds[32];

	uint8_t unk_40[32];

	uint64_t vertexShader;
	uint64_t pixelShader;
};
static_assert(sizeof(ShaderSetAssetHeader_v13_t) == 112);


class ShaderSetAsset
{
public:
	ShaderSetAsset() = default;
	ShaderSetAsset(ShaderSetAssetHeader_v8_t* const hdr)  : name(hdr->name), numVertexShaderTextures(hdr->textureInputCounts[0]), numPixelShaderTextures(hdr->textureInputCounts[1]), numSamplers(hdr->numSamplers), firstResourceBindPoint(hdr->firstResourceBindPoint), numResources(hdr->numResources), vertexShader(hdr->vertexShader), pixelShader(hdr->pixelShader), vertexShaderAsset(nullptr), pixelShaderAsset(nullptr)
	{ };
	ShaderSetAsset(ShaderSetAssetHeader_v11_t* const hdr) : name(hdr->name), numVertexShaderTextures(hdr->textureInputCounts[1] - hdr->textureInputCounts[0]), numPixelShaderTextures(hdr->textureInputCounts[0]), numSamplers(hdr->numSamplers), firstResourceBindPoint(hdr->firstResourceBindPoint), numResources(hdr->numResources), vertexShader(hdr->vertexShader), pixelShader(hdr->pixelShader), vertexShaderAsset(nullptr), pixelShaderAsset(nullptr)
	{ };
	ShaderSetAsset(ShaderSetAssetHeader_v12_t* const hdr) : name(hdr->name), numVertexShaderTextures(hdr->textureInputCounts[1] - hdr->textureInputCounts[0]), numPixelShaderTextures(hdr->textureInputCounts[0]), numSamplers(hdr->numSamplers), firstResourceBindPoint(hdr->firstResourceBindPoint), numResources(hdr->numResources), vertexShader(hdr->vertexShader), pixelShader(hdr->pixelShader), vertexShaderAsset(nullptr), pixelShaderAsset(nullptr)
	{ };
	ShaderSetAsset(ShaderSetAssetHeader_v13_t* const hdr) : name(hdr->name), numVertexShaderTextures(hdr->textureInputCounts[1] - hdr->textureInputCounts[0]), numPixelShaderTextures(hdr->textureInputCounts[0]), numSamplers(hdr->numSamplers), firstResourceBindPoint(hdr->firstResourceBindPoint), numResources(hdr->numResources), vertexShader(hdr->vertexShader), pixelShader(hdr->pixelShader), vertexShaderAsset(nullptr), pixelShaderAsset(nullptr)
	{ };

	char* name;

	// If numVertexShaderTextures == numPixelShaderTextures, the textures are only used by the pixel shader
	// Textures are used by pixel shader by default, then numVertexShaderTextures - numPixelShaderTextures is the actual
	// number of textures that the vertex shader uses.
	// 
	// This usually isn't many (and is actually often zero), since the pixel shader typically handles all use of textures.
	// The exception is probably VXD (vertex displacement) textures, but I haven't checked this.
	uint16_t numVertexShaderTextures;
	uint16_t numPixelShaderTextures;

	uint16_t numSamplers; // number of samplers used (split between PS/VS depending on textures)

	uint16_t firstResourceBindPoint;
	uint16_t numResources;

	// pointers?
	// how to handle external assets
	uint64_t vertexShader;
	uint64_t pixelShader;

	CPakAsset* vertexShaderAsset;
	CPakAsset* pixelShaderAsset;
};