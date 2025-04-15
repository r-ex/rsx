#include "pch.h"
#include "miles.h"

#include <game/audio/wavefile.h>
#include <game/rtech/utils/utils.h>

std::string CMilesAudioBank::GetStreamingFileNameForSource(const MilesSource_t* source) const
{
	std::string sourceStreamFileName = GetBankStem();

	if (source->languageIdx != 0xFFFF)
		sourceStreamFileName += std::format("_{}", GetLanguageNames()[source->languageIdx]);
	else
		sourceStreamFileName += "_stream";

	if (source->patchIdx)
		sourceStreamFileName += std::format("_patch_{}.mstr", source->patchIdx);
	else
		sourceStreamFileName += ".mstr";

	return sourceStreamFileName;
}

bool CMilesAudioBank::IsValidSource(const MilesSource_t* source) const
{
	// if the source isn't localised, we don't need to check if there is a localised stream for it
	if (source->languageIdx != 0xFFFF)
	{
		// If this source uses a language that we don't have a streaming file for, skip it.
		if (!m_localisedStreamStates.contains(source->languageIdx))
			return false;

		// If the bitfield shows that the source uses a language patch that we don't have, skip it.
		if ((m_localisedStreamStates.at(source->languageIdx) & (1 << source->patchIdx)) == 0)
			return false;
	}

	// If the bitfield shows that the source uses a non-localised patch that we don't have, skip it.
	if ((m_streamStates & (1 << source->patchIdx)) == 0)
		return false;

	return true;
}

void CMilesAudioBank::DiscoverStreamingFiles()
{
	const std::filesystem::path filePath(m_filePath);

	assert(filePath.has_parent_path());
	std::filesystem::path dirPath = filePath.parent_path();

	this->m_streamStates = 0;

	for (auto& it : std::filesystem::directory_iterator(dirPath))
	{
		if (it.is_regular_file())
		{
			if (it.path().extension() == ".mstr")
			{
				//Log("MSTR: Checking %s\n", it.path().string().c_str());
				StreamIO stream(it.path(), eStreamIOMode::Read);

				MilesStreamHeader_t header = stream.read<MilesStreamHeader_t>();
				stream.close();

				// Require 'CSTR' magic and "version" 2
				// It's not clear if the 2 is actually a version, but it lines up
				// with the location of the version in other MSS files so it probably is
				if (header.magic != 'CSTR' || header.version != 2u)
					continue;

				// Require a matching build tag between the stream and the bank
				// This ensures that the stream actually belongs to the same build as the bank
				if (header.buildTag != this->buildTag)
					continue;

				// max shift is 1 << 31 since the state is stored in a 32-bit type
				// i don't think there should ever be 32 patches though so i think we're good.
				assert(header.patchIdx <= 31);
				// assert and then skip the file to make sure that release builds don't die
				if (header.patchIdx > 31)
					continue;

				if (header.languageIdx == 0xFFFFu)
					// Set the bit that corresponds to this patch to 1 to indicate that
					// the stream exists and is valid for use by this bank.
					this->m_streamStates |= (1 << header.patchIdx);
				else
				{
					if (!this->m_localisedStreamStates.contains(header.languageIdx))
						this->m_localisedStreamStates[header.languageIdx] = 1 << header.patchIdx;
					else
						this->m_localisedStreamStates[header.languageIdx] |= 1 << header.patchIdx;
				}
			}
		}
	}

	Log("MBNK: Finished discovering streams.\n");
}

const bool CMilesAudioBank::ParseFromHeader()
{
	switch (this->m_version)
	{
	case 28:
	case 32:

		// these versions are valid for sources and the header but
		// any other data has not been checked
	case 33:
	case 34:
	case 36:
	case 38:
	{
		this->languageCount = 9;
		this->m_languageNames = {
			"english", "french", "german", "spanish", "italian",
			"japanese", "polish", "russian", "mandarin"
		};

		const MilesBankHeader_v28_t* const header = reinterpret_cast<MilesBankHeader_v28_t*>(m_fileBuf.get());

		this->Construct(header);

		this->DiscoverStreamingFiles();

		Log("MBNK: Parsing sources...\n");
		for (uint32_t i = 0; i < this->sourceCount; ++i)
		{
			const MilesSource_v28_t* const source = reinterpret_cast<MilesSource_v28_t*>(reinterpret_cast<char*>(this->audioSources) + (i * sizeof(MilesSource_v28_t)));
			MilesSource_t* const sourceAssetData = new MilesSource_t(source);

			if (!IsValidSource(sourceAssetData))
				continue;

			const char* const sourceName = this->GetString(sourceAssetData->nameOffset);

			CMilesAudioAsset* sourceAsset = new CMilesAudioAsset(sourceName, sourceAssetData, this);
			sourceAsset->SetAssetType((uint32_t)AssetType_t::ASRC); // asrc - audio source
			sourceAsset->SetAssetGUID(RTech::StringToGuid(sourceName));
			sourceAsset->SetAssetVersion({ m_version });

			sourceAsset->SetContainerName(GetStreamingFileNameForSource(sourceAssetData));

			g_assetData.v_assets.push_back({ sourceAsset->GetAssetGUID(), sourceAsset });
		}
		break;
	}
	case 39:
	case 40:
	case 42:
	case 43:
	case 44:
	case 45:
	case 46:
	{
		this->languageCount = 10;
		this->m_languageNames = {
			"english", "french", "german", "spanish", "italian",
			"japanese", "polish", "russian", "mandarin", "korean"
		};

		const MilesBankHeader_v45_t* const header = reinterpret_cast<MilesBankHeader_v45_t*>(m_fileBuf.get());

		this->Construct(header);

		this->DiscoverStreamingFiles();

		Log("MBNK: Parsing sources...\n");
		for (uint32_t i = 0; i < this->sourceCount; ++i)
		{
			const MilesSource_v39_t* const source = reinterpret_cast<MilesSource_v39_t*>(reinterpret_cast<char*>(this->audioSources) + (i * sizeof(MilesSource_v39_t)));
			MilesSource_t* const sourceAssetData = new MilesSource_t(source);

			if (!IsValidSource(sourceAssetData))
				continue;

			const char* const sourceName = this->GetString(sourceAssetData->nameOffset);

			CMilesAudioAsset* sourceAsset = new CMilesAudioAsset(sourceName, sourceAssetData, this);
			sourceAsset->SetAssetType((uint32_t)AssetType_t::ASRC); // asrc - audio source
			sourceAsset->SetAssetGUID(RTech::StringToGuid(sourceName));
			sourceAsset->SetAssetVersion({ m_version });

			sourceAsset->SetContainerName(GetStreamingFileNameForSource(sourceAssetData));

			g_assetData.v_assets.push_back({ sourceAsset->GetAssetGUID(), sourceAsset });
		}

		break;
	}
	default:
		return false;
	}

	return true;
}

const bool CMilesAudioBank::ParseFile(const std::string& path)
{
	Log("MBNK: Trying to load file: %s\n", path.c_str());

	m_filePath = path;

	if (!FileSystem::ReadFileData(path, &m_fileBuf))
		return false;

	MilesBankHeaderShort_t* hdrShort = reinterpret_cast<MilesBankHeaderShort_t*>(m_fileBuf.get());

	if (hdrShort->magic != 'CBNK')
		return false;

	this->m_version = hdrShort->version;

	if (m_version < 0)
		return false;


	if (!this->ParseFromHeader())
	{
		Log("MBNK: Tried to parse unimplemented file version %i.\n", hdrShort->version);
		return false;
	}

	Log("MBNK: Loaded bank \"%s\" with %u sources and %u events.\n", this->stringTable, this->sourceCount, this->eventCount);

	return true;
}

extern ExportSettings_t g_ExportSettings;

constexpr const char* PATH_PREFIX_ASRC = "audio";

uint32_t ReadAudioStream(char* buffer, size_t length, MilesASIUserData_t* userData)
{
	size_t totalRead = 0;

	if (userData->dataRead < userData->headerSize)
	{
		auto Diff = userData->headerSize - userData->dataRead;
		auto MinDiff = std::min(length, Diff);
		userData->streamReader->read(buffer, MinDiff);
		userData->dataRead += MinDiff;
		totalRead += MinDiff;

		if (userData->dataRead >= userData->headerSize)
			userData->streamReader->seek(userData->audioStreamOffset);
	}

	uint64_t LengthToRead = length - totalRead;
	LengthToRead = std::min(userData->audioStreamSize, LengthToRead);

	userData->streamReader->read(buffer + totalRead, LengthToRead);
	totalRead += LengthToRead;
	userData->audioStreamSize -= LengthToRead;

	return (uint32_t)totalRead;
}

bool ExportAudioSourceAsset(CAsset* const asset, const int setting)
{
	UNUSED(setting);

	CMilesAudioAsset* audioAsset = static_cast<CMilesAudioAsset*>(asset);
	CMilesAudioBank* audioBank = asset->GetContainerFile<CMilesAudioBank>();

	// Create exported path + asset path.
	std::filesystem::path exportPath = std::filesystem::current_path().append(EXPORT_DIRECTORY_NAME);
	const std::filesystem::path asrcPath(audioAsset->GetAssetName());

	// truncate paths?
	if (g_ExportSettings.exportPathsFull)
		exportPath.append(asrcPath.parent_path().string());
	else
		exportPath.append(PATH_PREFIX_ASRC);

	if (!CreateDirectories(exportPath))
	{
		assertm(false, "Failed to create asset type directory.");
		return false;
	}

	exportPath.append(asrcPath.filename().string());
	exportPath.replace_extension("wav");

	MilesSource_t* source = reinterpret_cast<MilesSource_t*>(audioAsset->GetAssetData());

	// get the bank's path and replace the filename
	// with the stream file name that we've just put together
	std::filesystem::path streamPath(audioBank->GetFilePath());
	streamPath.replace_filename(audioAsset->GetContainerFileName());

	// Data Reading
	StreamIO streamFile(streamPath, eStreamIOMode::Read);

	MilesStreamHeader_t streamFileHeader = streamFile.read<MilesStreamHeader_t>();

	streamFile.seek(source->streamHeaderOffset);

	char* sourceStreamHeaderData = new char[source->streamHeaderSize];

	streamFile.read(sourceStreamHeaderData, source->streamHeaderSize);

	std::vector<char> decodedAudioData;

	MilesASIDecoder_t* decoder = nullptr;

	switch (*(uint32_t*)sourceStreamHeaderData)
	{
	case 'RADA': // Rad Audio
		decoder = GetRadAudioDecoder();
		break;
	case 'BCF1': // Bink Audio
		decoder = nullptr;
		break;
	default:
	{
		decoder = nullptr;
		break;
	}
	}

	if (!decoder)
	{
		Log("MILES: Unsupported audio format.\n");
		return false;
	}

	struct {
		uint32_t minSizeToOpenStream;
		uint32_t maxBlockSize;
		uint32_t maxSamplesPerDecode;
		uint32_t decodeFormat;
	} parsedMetadata;

	const auto ASI_stream_parse_metadata = static_cast<ASI_parse_metadata_f>(decoder->ASI_stream_parse_metadata);
	const auto ASI_open_stream = static_cast<ASI_open_stream_f>(decoder->ASI_open_stream);
	const auto ASI_notify_seek = static_cast<ASI_notify_seek_f>(decoder->ASI_notify_seek);
	const auto ASI_decode_block = static_cast<ASI_decode_block_f>(decoder->ASI_decode_block);
	const auto ASI_get_block_size = static_cast<ASI_get_block_size_f>(decoder->ASI_get_block_size);

	uint16_t channels;
	uint32_t sampleRate;
	uint32_t samplesCount;
	ASI_stream_parse_metadata(sourceStreamHeaderData, source->streamHeaderSize, &channels, &sampleRate, &samplesCount, (int*)&parsedMetadata, nullptr);

	streamFile.seek(source->streamHeaderOffset);

	std::vector<char> container(parsedMetadata.minSizeToOpenStream, 0);

	MilesASIUserData_t userData = {
		&streamFile,
		0,
		source->streamHeaderSize,
		streamFileHeader.streamDataOffset + source->streamDataOffset
	};

	size_t containerSize = container.size();
	ASI_open_stream(container.data(), &containerSize, ReadAudioStream, &userData);

	ASI_notify_seek(container.data());

	userData.audioStreamSize = *(uint64_t*)(container.data() + 0x18) - source->streamHeaderSize;


	std::vector<float> interleavedBuffer = std::vector<float>(channels * samplesCount);
	float* outputBuffer = interleavedBuffer.data();

	std::vector<char> stream_data;

	// Buffer for holding the decoded data for each decode_block call.
	// parsedSizeInfo[2] is the max number of samples per decode
	std::vector<float> radDecodedData(channels* parsedMetadata.maxSamplesPerDecode);

	size_t totalFramesDecoded = 0;
	uint32_t minInputBufferSize = 0; // start off with 0 bytes for input buffer so we can ask the decoder what it wants

	while (totalFramesDecoded < samplesCount)
	{
		// Clear the decode buffer just in case something goes wrong
		memset(radDecodedData.data(), 0, radDecodedData.size() * 4);

		//const auto decode_block = static_cast<decoder_new_f_t>(decoder->ASI_decode_block);

		uint32_t bytesConsumed = 0;
		uint32_t blockSize = 0;

		// If we have not yet established the smallest that our input buffer can be, call getblocksize once to find out
		if (minInputBufferSize == 0)
		{
			ASI_get_block_size(container.data(), stream_data.data(), 0, &bytesConsumed, &blockSize, &minInputBufferSize);

			// Fetch the smallest possible amount of data to populate the input buffer.
			// Future decode iterations will include this minimum buffer size in their read operation
			stream_data.resize(minInputBufferSize);
			ReadAudioStream(stream_data.data(), stream_data.size(), &userData);
		}

		// Make a call to the decoder to find out how much data it wants for the next decode
		ASI_get_block_size(container.data(), stream_data.data(), stream_data.size(), &bytesConsumed, &blockSize, &minInputBufferSize);

		if (blockSize == 0xFFFF)
			break;

		const size_t oldSize = stream_data.size();
		stream_data.resize(blockSize + minInputBufferSize);
		ReadAudioStream(stream_data.data() + oldSize, stream_data.size() - oldSize, &userData);

		ASI_get_block_size(container.data(), stream_data.data(), stream_data.size(), &bytesConsumed, &blockSize, &minInputBufferSize);

		// if we have now got a valid decode input buffer
		if (blockSize != 0xFFFF)
		{
			uint32_t decodeBytesConsumed = 0;
			uint32_t samplesDecoded = 0;

			ASI_decode_block(container.data(), stream_data.data(), stream_data.size(), radDecodedData.data(), radDecodedData.size() * sizeof(float), &decodeBytesConsumed, &samplesDecoded);

			if (decodeBytesConsumed == 0)
			{
				printf("Finished decoding.\n");
				//break;
			}

			// The decoder provides us with a non-interleaved buffer which means that
			// each channel's data is separate out into separate locations within the decode buffer
			// before writing to file, the data must be brought back together
			// e.g.: (L - left channel, R - right channel)
			// non-interleaved: LLLLLLRRRRRR
			// interleaved:     LRLRLRLRLRLR
			// https://stackoverflow.com/a/17883834
			// 
			// This may not be valid for other decoders, as miles uses parsedSizeInfo[3] to identify the decoded data format
			// and decide how to process the audio immediately after decoding
			for (int channelIdx = 0; channelIdx < channels; ++channelIdx)
			{
				const float* const channelSampleBuffer = radDecodedData.data() + (parsedMetadata.maxSamplesPerDecode * channelIdx);

				for (uint32_t sampleIdx = 0; sampleIdx < samplesDecoded; ++sampleIdx)
				{
					// Index in the output buffer from which the channels of this sample begin
					const size_t outputIdx = static_cast<size_t>(channels) * (sampleIdx + totalFramesDecoded);

					outputBuffer[outputIdx + channelIdx] = channelSampleBuffer[sampleIdx];
				}
			}

			// Add number of samples decoded to the total to keep track of when we are done decoding the whole thing
			totalFramesDecoded += samplesDecoded;

			const size_t unconsumedInputBytes = stream_data.size() - decodeBytesConsumed;

			// Allocate temporary buffer to store the unconsumed input bytes until the main input buffer can be resized
			char* tempBuffer = new char[unconsumedInputBytes];

			// Copy unconsumed bytes to the temporary buffer
			memcpy(tempBuffer, stream_data.data() + decodeBytesConsumed, unconsumedInputBytes);

			// Resize the input buffer to the size of the temporary buffer
			stream_data.resize(unconsumedInputBytes);

			// Copy the bytes from the temporary buffer to the actual input buffer
			memcpy(stream_data.data(), tempBuffer, stream_data.size());

			// Release the temporary buffer
			delete[] tempBuffer;
			tempBuffer = nullptr;
		}
		else
		{
			//printf("Received blockSize = 0xFFFF after %lld decoded samples\n", totalFramesDecoded);
			break;
		}

	}

	StreamIO outFile(exportPath, eStreamIOMode::Write);

	WAVEHEADER hdr;


	outFile.write(hdr);
	outFile.write(reinterpret_cast<char*>(interleavedBuffer.data()), interleavedBuffer.size() * sizeof(float));

	const uint64_t DataSize = interleavedBuffer.size() * sizeof(float);
	hdr.size = static_cast<long>(DataSize + 36);

	hdr.fmt.channels = channels;
	hdr.fmt.sampleRate = sampleRate;
	hdr.fmt.blockAlign = static_cast<uint16_t>(DataSize / samplesCount);
	hdr.fmt.bitsPerSample = static_cast<uint16_t>(((DataSize * 8) / samplesCount) / channels);

	hdr.data.chunkSize = static_cast<long>(DataSize);

	hdr.fmt.avgBytesPerSecond = hdr.fmt.blockAlign * sampleRate;

	outFile.seek(0);
	outFile.write(hdr);
	outFile.close();

	return true;
}

void InitAudioSourceAssetType()
{
	AssetTypeBinding_t type =
	{
		.type = 'crsa',
		.headerAlignment = 1,
		.loadFunc = nullptr,
		.postLoadFunc = nullptr,
		.previewFunc = nullptr,
		.e = { ExportAudioSourceAsset, 0, nullptr, 0ull },
	};

	REGISTER_TYPE(type);
}