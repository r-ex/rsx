#pragma once

#include <game/rtech/cpakfile.h>

struct SubtitlesAssetEntry_t
{
	uint32_t hash;
	int stringOffset; // offset to string from strings in 'SubtitlesAssetHeader_v0_t'
};

struct SubtitlesAssetHeader_v0_t
{
	int stringCount;

	int entryCount;
	SubtitlesAssetEntry_t* entries;

	char* strings;
};

struct SubtitlesEntry
{
	SubtitlesEntry(const Vector& clrIn, const uint32_t hashIn, const char* subtitleIn) : clr(clrIn), hash(hashIn), subtitle(subtitleIn) {};

	Vector clr;
	uint32_t hash;
	const char* subtitle;
};

class SubtitlesAsset
{
public:
	SubtitlesAsset(SubtitlesAssetHeader_v0_t* hdr) : entries(hdr->entries), entryCount(hdr->entryCount), stringCount(hdr->stringCount), strings(hdr->strings) {};

	SubtitlesAssetEntry_t* entries;
	int entryCount;

	int stringCount;
	char* strings;

	std::vector<SubtitlesEntry> parsed;

};