#pragma once

#include <game/rtech/utils/utils.h>

enum eRSONFieldType : int
{
	RSON_NULL = 0x1,
	RSON_STRING = 0x2,
	RSON_VALUE = 0x4,
	RSON_OBJECT = 0x8,
	RSON_BOOLEAN = 0x10,
	RSON_INTEGER = 0x20,
	RSON_SIGNED_INTEGER = 0x40,
	RSON_UNSIGNED_INTEGER = 0x80,
	RSON_DOUBLE = 0x100,
	RSON_ARRAY = 0x1000,


	RSON_ARRAY_OF_OBJECTS = (RSON_ARRAY | RSON_OBJECT),
};

union RSONNodeValue_t
{
	RSONNodeValue_t* nodeValPtr;
	void* valPtr;
	char* string;
	uint64_t value;
	double valueFP;
	bool valueBool;
};

// asset header
struct RSONAssetHeader_v1_t
{
	int type;

	int valueCount;
	RSONNodeValue_t values;
};

class RSONAssetNode;

class RSONAsset
{
public:
	RSONAsset(RSONAssetHeader_v1_t* hdr) : type(hdr->type), valueCount(hdr->valueCount), values(hdr->values) {};

	int type;
	int valueCount;
	RSONNodeValue_t values;

	std::string rawText;
};

// nodes
struct RSONAssetNode_t
{
public:
	RSONAssetNode_t(const RSONAsset* const asset) : name(nullptr), type(asset->type), valueCount(asset->valueCount), values(asset->values), nextPeer(nullptr), prevPeer(nullptr) {};

	const char* name;
	int type;

	int valueCount;
	RSONNodeValue_t values; // sometimes a raw value is stored in here (boolean, integer, etc)

	// should be the right type?
	RSONAssetNode_t* nextPeer;
	RSONAssetNode_t* prevPeer; // unsure if ever written to disk

	void R_ParseNodeValues(std::stringstream& out, const size_t indentIdx) const;
	void R_WriteNodeValue(std::stringstream& out, RSONNodeValue_t val, const std::string& indentation, const size_t indentIdx) const;

	FORCEINLINE const bool IsArray() const
	{
		return this->type & eRSONFieldType::RSON_ARRAY;
	}

	FORCEINLINE const bool IsObject() const
	{
		return this->type & eRSONFieldType::RSON_OBJECT;
	}

	FORCEINLINE const bool IsArrayOfObjects() const
	{
		return (this->type & (eRSONFieldType::RSON_ARRAY_OF_OBJECTS)) == eRSONFieldType::RSON_ARRAY_OF_OBJECTS;
	}

	// Only the root node can be a keyless value
	FORCEINLINE const bool IsRoot() const
	{
		return this->name == nullptr;
	}
};

union AssetGuid_t;
void WriteRSONDependencyArray(std::ofstream& out, const char* const name, const AssetGuid_t* const dependencies, const int count);