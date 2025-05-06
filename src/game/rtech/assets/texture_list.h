#pragma once

constexpr const char* const s_TextureListCamoSkinsPath = "texture_list/camo_skins.rpak";

struct TextureListHeader_v1_s
{
	const uint64_t* textureGuids;
	const char** textureNames;
	uint64_t numTextures;
};

class TextureListAsset
{
public:
	TextureListAsset(const TextureListHeader_v1_s* const hdr) : textureGuids(hdr->textureGuids), textureNames(hdr->textureNames), numTextures(hdr->numTextures) {}

	const uint64_t* textureGuids;
	const char** textureNames;

	uint64_t numTextures;
};