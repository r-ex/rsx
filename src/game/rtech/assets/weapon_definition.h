#pragma once
#include <game/rtech/cpakfile.h>

enum WepnValueType_t : uint32_t
{
	INTEGER = 0,
	FLT = 1,
	STRING = 2,
};

union WepnValue_u
{
	const char* strVal;
	uint64_t intVal;
	float fltVal;

	uint64_t rawVal;
};

struct WepnKeyValue_v1_t
{
	const char* key;
	char unk_8[4];
	uint32_t valueType;
	WepnValue_u value;
};
static_assert(sizeof(WepnKeyValue_v1_t) == 24);

struct WepnData_v1_t
{
	const char* name;
	char unk_8[4];
	uint16_t numValues; // number of child values?
	uint16_t numChildren; // number of child objects?
	WepnKeyValue_v1_t* childKVPairs; // 24 bytes per numValues
	WepnData_v1_t* childObjects; // 48 bytes per numChildren

	// used but idk what for
	uint16_t* unk_20; // related to numValues?
	uint16_t* unk_28; // related to numChildren?
};
static_assert(sizeof(WepnData_v1_t) == 48);

struct WepnAssetHeader_v1_t
{
	const char* weaponName;
	WepnData_v1_t* rootKey;

	// seems to mostly be null ?
	void* unk_10;
	void* unk_18;
};
static_assert(sizeof(WepnAssetHeader_v1_t) == 32);

class WeaponAsset
{
public:
	WeaponAsset() = default;
};