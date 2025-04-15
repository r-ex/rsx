#pragma once
#define TXAN_FILE_MAGIC ('N'<<24 | 'A'<<16 | 'X'<<8 | 'T')
#define TXAN_FILE_VERSION 1 // increment this if the file format changes.

// mostly reversed from r5r's r5apex.exe executable at function 0x140B00450,
// this code is also used in function 0x140B007F0.
struct TextureAnimLayer_t
{
	// the layer animates from start texture to end, start can be higher than
	// the end as the engine simply just wraps the number around; we can
	// animate from texture 10 to texture 2, and from 2 to 10, etc.
	unsigned short startSlot;
	unsigned short endSlot;

	float unk2; // some scale used to switch between textures, cross-fading perhaps?

	unsigned short flags;
	unsigned short unk5; // most likely padding for 4 byte boundary, unused in functions above.
};

struct TextureAnimAssetHeader_v1_t
{
	TextureAnimLayer_t* layers; // the actual txan data, starts with TextureAnimDataHeader_t followed by data, array size is layerCount.
	uint8_t* slots; // points to what appears to be slots indices, its size is the highest referenced slot in TextureAnimLayer_t.
	int layerCount; // num layers, this is confirmed to be a 32bit int by the instruction at r5r's [r5apex.exe + AFFD2A].

	// always 0 and no usages in game after several dynamic breakpoint sessions, this is most likely just padding for the header
	// as it is aligned to 8, and we are 4 bytes short for that boundary without this.
	int padding;
};

// [amos]: this is not an official format, just a format so we can export them
//         and parse it out in repak.
struct TextureAnimFileHeader_t
{
	int magic;
	unsigned short fileVersion;
	unsigned short assetVersion;
	unsigned int layerCount;
	unsigned int slotCount;
};
