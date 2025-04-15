#pragma once
#include <d3d11.h>
#include <game/rtech/cpakfile.h>

struct UIImageAtlasAssetHeader_v10_t
{
	float widthRatio;
	float heightRatio;

	uint16_t width;
	uint16_t height;

	uint16_t textureCount;
	uint16_t unkCount;

	PagePtr_t textureOffsets;
	PagePtr_t textureDimensions;

	uint32_t unk1;
	uint32_t unk2;

	PagePtr_t textureHashes;
	PagePtr_t textureNames;
	uint64_t atlasGUID;
};

struct UIAtlasImageUV
{
	float uv0x;
	float uv0y;

	float uv1x;
	float uv1y;
};

struct UIAtlasImageOffset
{
	float f0;
	float f1;

	float endX;
	float endY;

	float startX;
	float startY;

	float unkX;
	float unkY;
};

struct UIAtlasImage
{
	std::string path;
	uint32_t pathHash;
	uint32_t pathTableOffset;
	
	UIAtlasImageOffset offsets;
	UIAtlasImageUV uv;

	uint16_t width;
	uint16_t height;

	uint16_t posX;
	uint16_t posY;
};

class CTexture;
class UIImageAtlasAsset
{
public:
	UIImageAtlasAsset() = default;
	UIImageAtlasAsset(UIImageAtlasAssetHeader_v10_t* hdr) : widthRatio(hdr->widthRatio), heightRatio(hdr->heightRatio), width(hdr->width), height(hdr->height), textureCount(hdr->textureCount), textureOffsets(hdr->textureOffsets),
		textureDimensions(hdr->textureDimensions), textureHashes(hdr->textureHashes), textureNames(hdr->textureNames), atlasGUID(hdr->atlasGUID), rawTxtr(nullptr), convertedTxtr(nullptr), format(DXGI_FORMAT_UNKNOWN) {};

	float widthRatio;
	float heightRatio;

	uint16_t width;
	uint16_t height;

	uint16_t textureCount;

	PagePtr_t textureOffsets;
	PagePtr_t textureDimensions;

	PagePtr_t textureHashes;
	PagePtr_t textureNames;
	uint64_t atlasGUID;

	//
	std::shared_ptr<CTexture> rawTxtr;
	std::shared_ptr<CTexture> convertedTxtr;
	DXGI_FORMAT format;
	std::vector<UIAtlasImage> imageArray;
};