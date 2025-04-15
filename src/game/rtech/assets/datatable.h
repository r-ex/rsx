#pragma once
#include <game/rtech/cpakfile.h>
#include <game/rtech/utils/utils.h>

enum class DatatableColumType_t : int
{
	Bool,
	Int,
	Float,
	Vector,
	String,
	Asset,
	AssetNoPrecache,

	_COUNT,
};

static size_t s_DatatableColumnTypeSize[static_cast<int>(DatatableColumType_t::_COUNT)] = {
	sizeof(bool), // padded bool
	sizeof(int),
	sizeof(float),
	sizeof(Vector),
	sizeof(char*),
	sizeof(char*),
	sizeof(char*),
};

static const char* s_DatatableColumnTypeName[static_cast<int>(DatatableColumType_t::_COUNT)] = {
	"\"bool\"",
	"\"int\"",
	"\"float\"",
	"\"vector\"",
	"\"string\"",
	"\"asset\"",
	"\"asset_noprecache\"",
};

inline bool DataTable_IsStringType(DatatableColumType_t type)
{
	switch (type)
	{
	case DatatableColumType_t::String:
	case DatatableColumType_t::Asset:
	case DatatableColumType_t::AssetNoPrecache:
		return true;
	default:
		return false;
	}
}

struct DatatableAssetColumn_v0_t
{
	char* name; // column name/heading
	DatatableColumType_t type; // column value data type
	int rowOffset; // offset in row data to this column's value
};
static_assert(sizeof(DatatableAssetColumn_v0_t) == 0x10);

struct DatatableAssetColumn_v1_1_t
{
	char* name; // column name/heading
	char unk_8[8];
	DatatableColumType_t type; // column value data type
	int rowOffset; // offset in row data to this column's value
};
static_assert(sizeof(DatatableAssetColumn_v1_1_t) == 0x18);

struct DatatableAssetHeader_v0_t
{
	int numColumns;
	int numRows;

	void* columns;
	void* rows;

	int rowStride;	// Number of bytes per row

	int unk_1C; // alignment
};
static_assert(sizeof(DatatableAssetHeader_v0_t) == 0x20);

struct DatatableAssetHeader_v1_t
{
	int numColumns;
	int numRows;

	void* columns;
	void* rows;

	char unk_18[8];

	int rowStride;	// Number of bytes per row

	int unk_24;
};
static_assert(sizeof(DatatableAssetHeader_v1_t) == 0x28);

constexpr uint64_t dtblChangeDate = 0x1d692d897275335; // 25/09/2020 01:10:00  // 1.0 -> 1.1
constexpr uint64_t dtblChangeDate2 = 0x1da975b0a106ef2; // 25/04/2024 21:53:42 // 1.1 -> 1.0

enum class eDTBLVersion : int
{
	VERSION_UNK = -1,
	VERSION_0,
	VERSION_1,
	VERSION_1_1,
};

inline const eDTBLVersion GetDTBLVersion(const CAssetContainer* const pak, CAsset* const asset)
{
	CPakAsset* pakAsset = static_cast<CPakAsset*>(asset);
	switch (pakAsset->data()->version)
	{
	case 0:
	{
		return eDTBLVersion::VERSION_0;
	}
	case 1:
	{
		if (static_cast<const CPakFile*>(pak)->header()->createdTime > dtblChangeDate && static_cast<const CPakFile*>(pak)->header()->createdTime < dtblChangeDate2) UNLIKELY
			return eDTBLVersion::VERSION_1_1;

		return eDTBLVersion::VERSION_1;
	}
	default:
		return eDTBLVersion::VERSION_UNK;
	}
}

class DatatableAssetColumn
{
public:
	DatatableAssetColumn(const DatatableAssetColumn_v0_t* const column) : name(column->name), type(column->type), rowOffset(column->rowOffset) {};
	DatatableAssetColumn(const DatatableAssetColumn_v1_1_t* const column) : name(column->name), type(column->type), rowOffset(column->rowOffset) {};

	char* name; // column name/heading
	DatatableColumType_t type; // column value data type
	int rowOffset; // offset in row data to this column's value
};

class DatatableAsset
{
public:
	DatatableAsset(const DatatableAssetHeader_v0_t* const hdr, const eDTBLVersion ver) : name(nullptr), numColumns(hdr->numColumns), numRows(hdr->numRows), columns(hdr->columns), rows(hdr->rows), rowStride(hdr->rowStride), version(ver)
	{
		columnData.reserve(numColumns);

		for (int i = 0; i < numColumns; i++)
			columnData.emplace_back(reinterpret_cast<DatatableAssetColumn_v0_t*>(columns) + i);
	};
	DatatableAsset(const DatatableAssetHeader_v1_t* const hdr, const eDTBLVersion ver) : name(nullptr), numColumns(hdr->numColumns), numRows(hdr->numRows), columns(hdr->columns), rows(hdr->rows), rowStride(hdr->rowStride), version(ver)
	{
		columnData.reserve(numColumns);

		switch (version)
		{
		case eDTBLVersion::VERSION_1:
		{
			for (int i = 0; i < numColumns; i++)
				columnData.emplace_back(reinterpret_cast<DatatableAssetColumn_v0_t*>(columns) + i);

			break;
		}
		case eDTBLVersion::VERSION_1_1:
		{
			for (int i = 0; i < numColumns; i++)
				columnData.emplace_back(reinterpret_cast<DatatableAssetColumn_v1_1_t*>(columns) + i);

			break;
		}
		case eDTBLVersion::VERSION_UNK:
		case eDTBLVersion::VERSION_0:
		default:
		{
			assertm(false, "invalid version");
			break;
		}
		}
	};

	char* name; // cached names

	int numColumns;
	int numRows;

	void* columns;
	void* rows;

	int rowStride;

	eDTBLVersion version;

	std::vector<DatatableAssetColumn> columnData;

	inline const DatatableAssetColumn* const GetColumn(const int colIdx) const { return &columnData.at(colIdx); };
	inline const char* const GetRowPtr(const int rowIdx) const { return reinterpret_cast<char*>(rows) + (rowStride * rowIdx); };
};
