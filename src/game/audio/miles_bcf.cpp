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
static_assert(sizeof(BinkFileHeader_t) == 24);

// From UnrealEngine: BinkAudioDecoder/SDK/BinkAudio/Include/binka_ue_decode.h
// 
// For the given rate and channels, this is the memory required for the decoder
// structure. Max 2 channels. Rates above 48000 are pretty useless as high freqs
// are crushed.
typedef uint32_t    DecodeMemoryFnType(uint32_t rate, uint32_t chans);

// Initialize the decoder, returns 0 on invalid parameters. Unreal should always be encoding bink_audio_2
typedef uint32_t    DecodeOpenFnType(void* BinkAudioDecoderMemory, uint32_t rate, uint32_t chans, bool interleave_output, bool is_bink_audio_2);

// Decode a single block. InputBuffer is updated with the amount of compressed data consumed for the block.
// Output is 16 bit pcm, interleaved if OpenFn() specified interleave_output.
typedef uint32_t    DecodeFnType(void* BinkAudioDecoderMemory, uint8_t* OutputBuffer, uint32_t OutputBufferLen, uint8_t const** InputBuffer, uint8_t const* InputBufferEnd);

// Call this when seeking/looping, this clears the overlap buffers.
typedef void        DecodeResetStartFrameFnType(void* BinkAudioDecoderMemory);

typedef struct UEBinkAudioDecodeInterface
{
	DecodeMemoryFnType* MemoryFn;
	DecodeOpenFnType* OpenFn;
	DecodeFnType* DecodeFn;
	DecodeResetStartFrameFnType* ResetStartFn;
} UEBinkAudioDecodeInterface;

// From linked BinkA UE decoder lib
extern UEBinkAudioDecodeInterface* __fastcall UnrealBinkAudioDecodeInterface();

bool ASI_stream_parse_metadata_bink(const char* dataBuffer, size_t dataBufferLen, uint16_t* outChannels, uint32_t* outSampleRate, uint32_t* outSampleCount, int* outDecoderInfo, uint32_t* outMemNeededToOpen)
{
	if (dataBufferLen < sizeof(BinkFileHeader_t))
		return false;

	const BinkFileHeader_t* header = reinterpret_cast<const BinkFileHeader_t*>(dataBuffer);
	
	if (header->magic != 'BCF1' || header->version > 2)
		return false;

	const uint32_t seekTableEntryCount = header->version == 2 ? header->seekTableEntryCount & 0xFFFF : header->seekTableEntryCount;

	uint16_t maxSamplesPerFrame = 0;
	if (header->sampleRate >= 44100u)
		maxSamplesPerFrame = 2048u;
	else if (header->sampleRate >= 22050u)
		maxSamplesPerFrame = 1024u;
	else
		maxSamplesPerFrame = 512u;

	uint32_t totalDecoderSize = 0;
	uint32_t numChannels = header->channelCount;
	const uint32_t streamCount = (header->channelCount + 1) >> 1;
	for (uint32_t i = 0; i < streamCount; ++i)
	{
		const uint32_t streamChannels = (numChannels >= 2) ? 2 : 1;

		totalDecoderSize += UnrealBinkAudioDecodeInterface()->MemoryFn(header->sampleRate, streamChannels);

		numChannels -= streamChannels;
	}

	*outChannels = header->channelCount;
	*outSampleRate = header->sampleRate;
	*outSampleCount = header->sampleCount;

	outDecoderInfo[0] = (((totalDecoderSize + 191) & ~63) + 4 * seekTableEntryCount + 67) & ~63;
	outDecoderInfo[1] = header->maxCompSpaceNeeded + 16; // decoder buffer size
	outDecoderInfo[2] = maxSamplesPerFrame;
	outDecoderInfo[3] = 0; // decode output format. 0 is 16-bit pcm, 2 is float. im pretty sure this is a flag variable

	UNUSED(outMemNeededToOpen);

	return true;
}

struct BinkDecoder_t
{
	uint32_t magic;
	uint8_t version;
	uint8_t channelCount;
	uint16_t sampleRate;
	uint32_t sampleCount;
	uint32_t maxCompSpaceNeeded;
	uint32_t fileSize;
	uint32_t seekTableEntryCount;
	uint32_t blocksPerSeekTableEntry_v2;
	uint32_t seekTableSize;
	uint8_t streamCount;
	void* streamDecoders[4];
	int* decodedSeekTable;
	char gap50[48];
	void* decoders;
};

static_assert(offsetof(BinkDecoder_t, blocksPerSeekTableEntry_v2) == 24);
static_assert(offsetof(BinkDecoder_t, seekTableSize) == 0x1c);
static_assert(offsetof(BinkDecoder_t, streamDecoders) == 0x28);
static_assert(offsetof(BinkDecoder_t, decodedSeekTable) == 72);

uint8_t ASI_open_stream(void* decoderMem, size_t* decoderMemSize, ASI_read_stream_f readFunc, void* userData)
{
	UNUSED(decoderMemSize);

	BinkFileHeader_t fileHeader;
	if (readFunc((char*)&fileHeader, sizeof(fileHeader), userData) != sizeof(fileHeader) || fileHeader.magic != 'BCF1' || fileHeader.version > 2u)
		return 0;
	
	// For v2 files, seekTableEntryCount is split into two variables, so we need to
	// extract the new variable to make sure this code works properly
	uint16_t unk = 0;
	if (fileHeader.version == 2)
	{
		unk = HIWORD(fileHeader.seekTableEntryCount);

		// Mask off the top word
		fileHeader.seekTableEntryCount &= 0xFFFF;
	}

	BinkDecoder_t* decoder = reinterpret_cast<BinkDecoder_t*>(decoderMem);

	decoder->magic = 'BCF1';
	decoder->version = fileHeader.version;
	decoder->channelCount = fileHeader.channelCount;
	decoder->sampleRate = fileHeader.sampleRate;
	decoder->sampleCount = fileHeader.sampleCount;

	decoder->maxCompSpaceNeeded = fileHeader.maxCompSpaceNeeded;
	decoder->fileSize = fileHeader.fileSize;
	decoder->seekTableEntryCount = fileHeader.seekTableEntryCount;
	decoder->blocksPerSeekTableEntry_v2 = fileHeader.fileSize; // This is unused by the decoder, so we can repurpose it to store the fileSize value that RSX's "miles" impl expects
	decoder->seekTableSize = 2 * decoder->seekTableEntryCount + 24;
	memset(decoder->streamDecoders, 0, sizeof(decoder->streamDecoders));
	
	constexpr uint8_t MAX_STREAMS = 8;
	// Max 8 streams; 16ch, 2ch per stream
	uint32_t decoderSizePerStream[MAX_STREAMS];
	uint8_t channelsPerStream[MAX_STREAMS]; // this seems to only be 4 bytes in r5r's dll, but that makes no sense

	uint32_t totalDecoderSize = 0;

	decoder->streamCount = (fileHeader.channelCount + 1) >> 1;
	
	uint8_t numChannelsRemaining = fileHeader.channelCount;
	for (uint32_t i = 0; i < decoder->streamCount; ++i)
	{
		// Max 2 channels per stream
		const uint8_t streamChannels = numChannelsRemaining >= 2 ? 2 : 1;

		channelsPerStream[i] = streamChannels;

		decoderSizePerStream[i] = UnrealBinkAudioDecodeInterface()->MemoryFn(fileHeader.sampleRate, streamChannels);
		totalDecoderSize += decoderSizePerStream[i];
		
		numChannelsRemaining -= streamChannels;
	}

	const uint32_t maybeDecoderSizeAligned = (totalDecoderSize + 191) & ~63u;

	decoder->decodedSeekTable = reinterpret_cast<int*>((char*)decoderMem + maybeDecoderSizeAligned);


	char seekTableBuffer[256];
	uint32_t lastSeek = 0;
	uint32_t v22 = 0;
	uint32_t numDecodedSeeks = 0;
	for (uint32_t i = 0; i < decoder->seekTableEntryCount;)
	{
		uint32_t v23 = decoder->seekTableEntryCount - i;
		uint32_t v24 = decoder->seekTableEntryCount - v22;

		// Limit to 128 entries at a time
		if (v23 > 128)
		{
			v23 = 128;
			v24 = 128;
		}

		// Each entry is a u16
		if (readFunc(seekTableBuffer, sizeof(uint16_t) * v23, userData) != sizeof(uint16_t) * v23)
			return 0;

		// Seek table is relative on disk
		// Decode into absolute values
		for (uint32_t seekIdx = 0; seekIdx < v23; ++seekIdx)
		{
			decoder->decodedSeekTable[numDecodedSeeks + seekIdx] = lastSeek;

			lastSeek += seekTableBuffer[seekIdx];
		}

		i += v23;
		v22 += v24 + numDecodedSeeks;
		numDecodedSeeks += v24;
	}

	// Write the "lastSeek" value into the end of the decoded seek table to represent seeking to the end of the audio
	decoder->decodedSeekTable[decoder->seekTableEntryCount] = lastSeek;

	for (uint32_t i = 0; i < decoder->streamCount; ++i)
	{
		// cleaned up in ASI_dealloc_bink
		char* decMem = new char[decoderSizePerStream[i]];
		const uint8_t streamChannels = channelsPerStream[i];

		// if this flag is set, the file is version 2?
		const bool unkFlag = HIWORD(fileHeader.maxCompSpaceNeeded) != 0;

		UnrealBinkAudioDecodeInterface()->OpenFn(decMem, fileHeader.sampleRate, streamChannels, false, unkFlag);
	
		decoder->streamDecoders[i] = decMem;
	}

	return (decoder->seekTableEntryCount != 0) + 1;
}

void ASI_reset_start(void* container)
{
	BinkDecoder_t* decoder = reinterpret_cast<BinkDecoder_t*>(container);

	for (uint32_t i = 0; i < decoder->streamCount; ++i)
	{
		UnrealBinkAudioDecodeInterface()->ResetStartFn(decoder->streamDecoders[i]);
	}
}

void __fastcall BinkInterface_ResetStart(__int64 a1)
{
	if (a1)
		*(uint32_t*)(a1 + 32) = 1;
}

void get_block_size_internal(BinkDecoder_t* decoder, const char* input_reservoir, size_t input_reservoir_len,
	uint32_t* out_consumed_bytes, uint32_t* out_block_size, uint32_t* out_req_size)
{
	unsigned int v13; // ecx
	unsigned int v14; // ecx
	int v15; // edx

	ASI_reset_start(decoder);
	const char* initialReservoirPtr = input_reservoir;
	if (input_reservoir_len < 4)
	{
	LABEL_12:
		*out_consumed_bytes = 0xFFFF;
	}
	else
	{
		while (1)
		{
			v13 = *(uint32_t*)input_reservoir;
			if (*(uint32_t*)input_reservoir == 'BCF1')
				break;
			if ((uint16_t)v13 == 0x9999)
			{
				v14 = HIWORD(v13);
				v15 = 4;
				if (v14 == 0xFFFF)
				{
					if (input_reservoir_len < 8)
						goto LABEL_12;
					v14 = *(unsigned __int16*)(input_reservoir + 4);
					v15 = 8;
				}
				if (v14 <= LOWORD(decoder->maxCompSpaceNeeded))
				{
					*out_consumed_bytes = static_cast<uint32_t>(input_reservoir - initialReservoirPtr);
					*out_block_size = v15 + v14;
					*out_req_size = 8;
					return;
				}
			}
			--input_reservoir_len;
			++input_reservoir;
			if (input_reservoir_len < 4)
				goto LABEL_12;
		}
		*out_consumed_bytes = decoder->seekTableSize;
	}
	*out_block_size = 0xFFFF;
	*out_req_size = 0;
}


typedef struct BINKAC_OUT_RINGBUF
{
	void* outptr;         // pointer within outstart and outend to write to (after return, contains the new end output ptr)
	void* outstart;
	void* outend;

	uint32_t   outlen;          // unused available (starting at outptr, possibly wrapping) - (after return, how many bytes copied)
	// outlen should always be a multiple of 16

	uint32_t   eatfirst;        // remove this many bytes from the start of the decompressed data (after return, how many bytes left)
	// eatfirst should always be a multple of 16                       

	uint32_t   decoded_bytes;   // from the input stream, how many bytes decoded (usually outlen, unless outlen was smaller than a frame - that is, when not enough room in the ringbuffer)
} BINKAC_OUT_RINGBUF;

typedef struct BINKAC_IN
{
	void const* inptr;      // pointer to input data (after return, contains new end input ptr)
	void const* inend;      // end of input buffer (there should be at least BINKACD_EXTRA_INPUT_SPACE bytes after this ptr)
} BINKAC_IN;

char __fastcall sub_180001700(
	BinkDecoder_t* decoder,
	const char* inputBuffer,
	char* outDecodeBuffer,
	uint32_t outputBufferLen,
	uint32_t* outInputBytesConsumed,
	uint32_t* outSamplesDecoded)
{
	const char* initialInput = inputBuffer;

	int v6 = -1;

	uint32_t frameSize = HIWORD(*(uint32_t*)inputBuffer);
	const char* inptr = inputBuffer + 4;

	if (frameSize == 0xFFFF)
	{
		const uint32_t trimmedFrameHeader = *reinterpret_cast<const uint32_t*>(inptr);

		inptr = inputBuffer + 8;

		frameSize = LOWORD(trimmedFrameHeader);
		v6 = HIWORD(trimmedFrameHeader);
	}

	uint32_t totalDecodedBytes = 0;

	const uint8_t* inputEndPtr = reinterpret_cast<const uint8_t*>(&inptr[frameSize]);

	// Output ptr for the currently decoding stream
	char* streamOutput = outDecodeBuffer;

	for (uint32_t i = 0; i < decoder->streamCount; ++i)
	{
		const uint32_t decodedBytes = UnrealBinkAudioDecodeInterface()->DecodeFn(
			decoder->streamDecoders[i],
			reinterpret_cast<uint8_t*>(streamOutput),
			outputBufferLen,
			reinterpret_cast<const uint8_t**>(&inptr),
			inputEndPtr
			);
	
		streamOutput += decodedBytes;
		totalDecodedBytes += decodedBytes;
	}


	if (v6 != -1)
	{
		char* v20 = &outDecodeBuffer[2 * v6];
		size_t v21 = 2ull * (totalDecodedBytes / (2ull * decoder->channelCount));

		char* i = &outDecodeBuffer[v21];
		for (uint32_t v19 = 1; v19 < decoder->channelCount; ++v19)
		{

			memmove(v20, i, 2 * v6);
			i += v21;
			v20 += 2 * v6;
		}

		totalDecodedBytes = 2 * v6 * decoder->channelCount;
	}

	*outInputBytesConsumed = static_cast<uint32_t>(inptr - initialInput);
	*outSamplesDecoded = totalDecodedBytes / (2ull * decoder->channelCount);
	return 1;
}

void ASI_decode_block(void* container, const char* inputBuffer, size_t inputBufferLen, char* outputBuffer, size_t outputBufferLen, uint32_t* outInputBytesUsed, uint32_t* outFramesDecoded)
{
	BinkDecoder_t* decoder = reinterpret_cast<BinkDecoder_t*>(container);

	void* endPtr = (void*)&inputBuffer[inputBufferLen];
	
	if (inputBufferLen < 4)
	{
		*outInputBytesUsed = 4;
		*outFramesDecoded = 0;
		return;
	}

	const uint32_t firstDWORD = *reinterpret_cast<const uint32_t*>(inputBuffer);

	if (LOWORD(firstDWORD) != 0x9999)
	{
		*outInputBytesUsed = 4;
		*outFramesDecoded = 0;
		return;
	}

	const char* v11 = inputBuffer + 4;

	uint32_t v12 = HIWORD(firstDWORD);

	if (HIWORD(firstDWORD) == 0xFFFF)
	{
		if (inputBufferLen < 8)
		{
			*outInputBytesUsed = 4;
			*outFramesDecoded = 0;
			return;
		}

		v12 = LOWORD(*(uint32_t*)v11);
		v11 += 4;
	}

	if (v12 > decoder->maxCompSpaceNeeded)
	{
		*outInputBytesUsed = 4;
		*outFramesDecoded = 0;
		return;
	}

	const char* v14 = &v11[v12];

	if (v14 > endPtr || v14 != endPtr && v14 + 4 <= endPtr && *(uint16_t*)v14 != 0x9999)
	{
		*outInputBytesUsed = 4;
		*outFramesDecoded = 0;
		return;
	}

	if (!sub_180001700(decoder, inputBuffer, outputBuffer, static_cast<uint32_t>(outputBufferLen), outInputBytesUsed, outFramesDecoded))
	{
		*outInputBytesUsed = 4;
		*outFramesDecoded = 0;
		return;
	}

}

void ASI_get_block_size(void* container, const char* input_reservoir, size_t input_reservoir_len,
	uint32_t* out_consumed_bytes, uint32_t* out_block_size, uint32_t* out_req_size)
{
	BinkDecoder_t* decoder = reinterpret_cast<BinkDecoder_t*>(container);
	if (input_reservoir_len < 4)
	{
		*out_consumed_bytes = 0;
		*out_block_size = 0xFFFF;
		*out_req_size = 0;

		return;
	}

	uint32_t headerSize = 4;
	uint32_t someSizeVar = 0;

	if (*reinterpret_cast<const uint16_t*>(input_reservoir) != 0x9999)
	{
		get_block_size_internal(decoder, input_reservoir, input_reservoir_len, out_consumed_bytes, out_block_size, out_req_size);

		return;
	}

	if (*reinterpret_cast<const uint16_t*>(input_reservoir + 2) == 0xFFFF)
	{
		if (input_reservoir_len < 8)
		{
			*out_consumed_bytes = 0;
			*out_block_size = 0xFFFF;
			*out_req_size = 0;
			return;
		}

		someSizeVar = *reinterpret_cast<const uint16_t*>(input_reservoir + 4);
		headerSize = 8;

	}

	if (someSizeVar <= decoder->maxCompSpaceNeeded) // maxCompSpaceNeeded
	{
		*out_block_size = headerSize + someSizeVar;
		*out_consumed_bytes = 0;
		*out_req_size = 8;
	}
	else
	{
		get_block_size_internal(decoder, input_reservoir, input_reservoir_len, out_consumed_bytes, out_block_size, out_req_size);
	}
}

void ASI_dealloc_bink(void* container)
{
	BinkDecoder_t* decoder = reinterpret_cast<BinkDecoder_t*>(container);

	for (int i = 0; i < 4; ++i)
	{
		if (decoder->streamDecoders[i])
			delete[] decoder->streamDecoders[i];
	}
}


MilesASIDecoder_t* GetBinkAudioDecoder()
{
	MilesASIDecoder_t* binkAudio = new MilesASIDecoder_t;

	binkAudio->unk0 = 1;
	binkAudio->decoderType = MILES_DECODER_BINKA;
	binkAudio->ASI_stream_parse_metadata = ASI_stream_parse_metadata_bink;
	binkAudio->ASI_open_stream = ASI_open_stream;
	binkAudio->ASI_notify_seek = ASI_reset_start;
	binkAudio->ASI_decode_block = ASI_decode_block;
	binkAudio->ASI_get_block_size = ASI_get_block_size;
	binkAudio->ASI_dealloc = ASI_dealloc_bink;

	return binkAudio;
}
