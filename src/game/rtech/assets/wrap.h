#pragma once
#include <game/vpk/vpk.h>

#define WRAP_FLAG_FILE_IS_COMPRESSED	0x1   // First bit seems to indicate the asset is compressed.
#define WRAP_FLAG_FILE_IS_PERMANENT		0x4   // Fourth bit seems to indicate that the asset is streamed.
#define WRAP_FLAG_FILE_IS_STREAMED		0x10

struct WrapAssetHeader_v1_t
{
	uint32_t size;
	uint32_t unk_4; // never used and we don't wanna accidently make a large file if it is
	void* data;
};

struct WrapAssetHeader_v7_t
{
	char* path;
	void* data;

	uint32_t hash;
	uint32_t cmpSize;
	uint32_t dcmpSize;

	uint16_t pathSize;
	uint16_t skipFirstFolderPos;
	uint16_t fileNamePos;

	uint16_t flags;
	uint16_t unk4;
	uint8_t unk5[2];
};



class WrapAsset
{
public:
	WrapAsset() = default;
	WrapAsset(WrapAssetHeader_v1_t* const hdr) : path(nullptr), data(hdr->data), cmpSize(hdr->size), dcmpSize(hdr->size), pathSize(0u), skipFirstFolderPos(0u),
		fileNamePos(0u), flags(0u), skipSize(0u), isCompressed(false), isStreamed(false), parsedData(nullptr), type(VPKFileType_e::UNKNOWN)
	{

	}
	WrapAsset(WrapAssetHeader_v7_t* const hdr) : path(hdr->path), data(hdr->data), cmpSize(hdr->cmpSize), dcmpSize(hdr->dcmpSize), pathSize(hdr->pathSize), skipFirstFolderPos(hdr->skipFirstFolderPos),
		fileNamePos(hdr->fileNamePos), flags(hdr->flags), parsedData(nullptr), type(VPKFileType_e::UNKNOWN)
	{
		// This assert has been placed here in case we find an asset who's compressed
		// size == decompressed size, since I don't know if we need to decompress in
		// that case or not. If we do we need to update the isCompressed assignment
		// below, if not we need to remove this wall of text + assert below.
		assert(cmpSize != dcmpSize);

		const bool shouldDecompress = (cmpSize < dcmpSize);

		// Skip size is typically 2, we need to shift else we export
		// garbage and truncate the data with the difference below.
		skipSize = shouldDecompress ? 0 : (cmpSize - dcmpSize);

		// Some assets have the compressed flag while compressed size is larger than
		// decompressed, in this case the asset is NOT compressed.
		isCompressed = (flags & WRAP_FLAG_FILE_IS_COMPRESSED) && shouldDecompress;
		isStreamed = (flags & WRAP_FLAG_FILE_IS_STREAMED);
	};

	~WrapAsset()
	{
		if (type == VPKFileType_e::BSP && parsedData)
		{
			delete parsedData;
			parsedData = nullptr;
		}
	}

	char* path;
	void* data;

	uint32_t skipSize;

	uint32_t cmpSize;
	uint32_t dcmpSize;

	uint16_t pathSize;
	uint16_t skipFirstFolderPos;
	uint16_t fileNamePos;

	uint16_t flags;
	//
	bool isCompressed;
	bool isStreamed;

	VPKFileType_e type;

	void* parsedData; // data class for something like 


	std::unique_ptr<char[]> rawData;
};