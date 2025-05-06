#pragma once

// todo: check if v4 header is smaller ?
struct MapAssetHeader_v5_t
{
	char unk_0[88];

	void* unk_58;
	void* unk_60;
};
static_assert(sizeof(MapAssetHeader_v5_t) == 104);