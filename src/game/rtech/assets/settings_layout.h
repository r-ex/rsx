#pragma once

enum class eSettingsFieldType : uint16_t
{
	ST_BOOL,
	ST_INTEGER,
	ST_FLOAT,
	ST_FLOAT2,
	ST_FLOAT3,
	ST_STRING,
	ST_ASSET,
	ST_ASSET_2,
	ST_ARRAY,
	ST_ARRAY_2, // "dynamic array" - special type of array - need to check why it's a different type
};

static const char* s_settingsFieldTypeNames[] =
{
	"bool",
	"int",
	"float",
	"float2",
	"float3",
	"string",
	"asset",
	"asset_noprecache",
	"array",
	"array_dynamic",
};

inline const char* Settings_GetTypeNameString(eSettingsFieldType type)
{
	return s_settingsFieldTypeNames[static_cast<int>(type)];
}

struct SettingsField_t
{
	eSettingsFieldType dataType;

	// offset from the start of layout's stringData to this field's name
	uint16_t nameOffset;

	// offset from the start of any referencing setting asset's value data to the value of this field
	uint32_t valueOffset : 24;

	// [rexx]: not completely sure about the name for this, but it indexes into the "unk_40" array in the settings layout header
	//         and occasionally uses the value as a settings layout asset header
	uint32_t valueSubLayoutIdx : 8;

	FORCEINLINE bool IsEmpty() const
	{
		return nameOffset == 0;
	}
};

// The field bucket size is always a power of 2, but its not necessarily ordered.
// This struct is used by the game to allow indexing into fieldData using an
// iterator on fieldCount and indexing into fieldMap to get direct access to the
// actual field and its name. See r5r's [r5apex.exe + 0xFB33B0] to see its usage.
struct SettingsFieldMap_t
{
	uint16_t fieldBucketIndex;
	uint16_t helpTextIndex;
};

struct SettingsLayoutHeader_v0_t
{
	char* name;
	SettingsField_t* fieldData;
	SettingsFieldMap_t* fieldMap;
	uint32_t hashTableSize;
	uint32_t fieldCount;
	uint32_t extraDataSizeIndex;
	uint32_t hashStepScale;
	uint32_t hashSeed;
	uint32_t arrayValueCount;
	uint32_t totalBufferSize;
	uint32_t unk_34; // Most likely padding, always 0 so far.
	char* stringData;

	SettingsLayoutHeader_v0_t* subHeaders;

	const char* GetStringFromOffset(uint32_t offset) const
	{
		return stringData + offset;
	}
};
static_assert(sizeof(SettingsLayoutHeader_v0_t) == 72);
static_assert(offsetof(SettingsLayoutHeader_v0_t, stringData) == 0x38);

class SettingsField
{
public:
	const char* fieldName;
	const char* helpText;
	uint32_t valueOffset : 24;
	uint32_t valueSubLayoutIdx : 8;
	eSettingsFieldType dataType;

	bool operator<(const SettingsField& rhs)
	{
		return this->valueOffset < rhs.valueOffset;
	}
};

class SettingsLayoutAsset
{
public:
	SettingsLayoutAsset(SettingsLayoutHeader_v0_t* hdr) :
		name(hdr->name), fieldData(hdr->fieldData), fieldMap(hdr->fieldMap), hashTableSize(hdr->hashTableSize),
		fieldCount(hdr->fieldCount), extraDataSizeIndex(hdr->extraDataSizeIndex), hashStepScale(hdr->hashStepScale), hashSeed(hdr->hashSeed), arrayValueCount(hdr->arrayValueCount), totalLayoutSize(hdr->totalBufferSize), unk_34(hdr->unk_34),
		stringData(hdr->stringData), pSubHeaders(hdr->subHeaders), isSubHeader(false)
	{
		if (pSubHeaders)
		{
			uint32_t highestIndex = 0;

			for (uint32_t i = 0; i < hashTableSize; i++)
			{
				const SettingsField_t& set = hdr->fieldData[i];

				if (set.valueSubLayoutIdx > highestIndex)
					highestIndex = set.valueSubLayoutIdx;
			}

			for (uint32_t i = 0; i <= highestIndex; i++)
			{
				SettingsLayoutAsset& newSubLayout = subHeaders.emplace_back(&pSubHeaders[i]);
				newSubLayout.isSubHeader = true;
			}
		}
	}

	char* name;
	
	// This appears to be the array for a hashmap of fields.
	// An empty field entry is indicated by a nameOffset of 0
	SettingsField_t* fieldData;

	SettingsFieldMap_t* fieldMap;
	uint32_t hashTableSize; // Must always be a power of 2.
	uint32_t fieldCount;
	uint32_t extraDataSizeIndex; // Indexes into s_settingsBlockExtraDataSize (see r5reloaded [r5apex.exe + 0x5C2417]).
	uint32_t hashStepScale;
	uint32_t hashSeed;

	// Always 1 for non-array layouts, and -1 for dynamic arrays. Static arrays can be >=1
	int arrayValueCount;

	uint32_t totalLayoutSize;
	uint32_t unk_34;
	char* stringData; // contains all the field names

	// This is seemingly an array of unnamed "sub layouts"
	// i.e., unk_40[i].name == nullptr
	SettingsLayoutHeader_v0_t* pSubHeaders;

	std::vector<SettingsLayoutAsset> subHeaders;
	std::vector<SettingsField> layoutFields;

	bool isSubHeader;

public:
	void ParseAndSortFields();

	const char* GetStringFromOffset(uint32_t offset) const
	{
		return stringData + offset;
	}
};
