#include "pch.h"
#include <game/audio/miles.h>

#include <game/rtech/utils/utils.h>

struct BinkFileHeader_t
{
	uint32_t magic;
	uint8_t version;
	uint8_t channelCount;
	uint16_t sampleRate;
	uint32_t sampleCount;
	uint32_t maxCompSpaceNeeded;
	uint32_t fileSize;
	uint32_t seekTableEntryCount;
};

bool ASI_stream_parse_metadata_bink(const char* buffer, size_t dataSize, uint16_t* outChannels, uint32_t* outSampleRate, uint32_t* outFrameCount, int* outSizeInfo, uint32_t* outMemNeededToOpen)
{
	if (dataSize < sizeof(BinkFileHeader_t))
		return false;

	const BinkFileHeader_t* header = reinterpret_cast<const BinkFileHeader_t*>(buffer);
	
	if (header->magic != 'BCF1' || header->version > 2)
		return false;

	int v9;

	if (header->sampleRate < 22050u)
		v9 = header->channelCount << 10;
	else if (header->sampleRate < 44100u)
		v9 = header->channelCount << 11;
	else
		v9 = header->channelCount << 12;

	const uint32_t v13 = v9 + (header->channelCount << 10) + 160;
	const uint32_t v14 = (header->channelCount + 1) >> 1;
	uint32_t v11 = 0;

	uint32_t channels = header->channelCount;
	uint32_t samplesPerFrame = 0;
	for (uint32_t i = 0; i < v14; ++i)
	{
		const uint32_t v16 = (channels >= 2) ? 2 : 1;

		if (header->sampleRate < 22050u)
			samplesPerFrame = 1 << 9;
		else if (header->sampleRate < 44100u)
			samplesPerFrame = 1 << 10;
		else
			samplesPerFrame = 1 << 11;


		v11 += (((2 * samplesPerFrame * v16) >> 4) + 175) & ~0xF;
		channels -= 2;
	}

	*outChannels = header->channelCount;
	*outSampleRate = header->sampleRate;
	*outFrameCount = header->sampleCount;

	*outSizeInfo = (((((((4 * header->seekTableEntryCount + 259) & 0xFFFFFFC0) + 2 * header->maxCompSpaceNeeded + 71) & 0xFFFFFFC0)
		+ v11
		+ 63) & 0xFFFFFFC0)
		+ v13
		+ 63) & 0xFFFFFFC0;

	UNUSED(outMemNeededToOpen);

	return true;
}

void ASI_reset_blend_bink(void* container)
{
	UNUSED(container);
	//BinkAudioDecompressResetStartFrame(container);
}


MilesASIDecoder_t* GetBinkAudioDecoder()
{
	MilesASIDecoder_t* binkAudio = new MilesASIDecoder_t;

	binkAudio->unk0 = 1;
	binkAudio->decoderType = 2;
	// todo

	return binkAudio;
}
