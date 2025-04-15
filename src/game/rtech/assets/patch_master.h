#pragma once
#include <game/rtech/cpakfile.h>
#include <game/rtech/utils/utils.h>

struct PatchAssetHeader_t
{
	int unk;
	int patchCount;
	char** pakNames;
	uint8_t* pakPatchVersions;
};
