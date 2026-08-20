#include "pch.h"
#include "miles.h"

#include <game/audio/wavefile.h>
#include <game/rtech/utils/utils.h>
#include <imgui.h>
#include <miniaudio/miniaudio.h>

#include <core/audio/audioplayer.h>
#include <core/fonts/codicons.h>
#include <misc/imgui_utility.h>

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
	assert(GetFilePath().has_parent_path());
	std::filesystem::path dirPath = GetFilePath().parent_path();

	this->m_streamStates = 0;

	for (auto& it : std::filesystem::directory_iterator(dirPath))
	{
		if (!it.is_regular_file() || it.path().extension() != ".mstr")
			continue;

		StreamIO stream(it.path(), eStreamIOMode::Read);

		MilesStreamHeader_t header = stream.read<MilesStreamHeader_t>();
		stream.close();

		if (header.magic != 'CSTR' || header.version != 2u)
			continue;

		if (header.buildTag != this->buildTag)
			continue;

		// max shift is 1 << 31 since the state is stored in a 32-bit type
		// i don't think there should ever be 32 patches though so i think we're good.
		assert(header.patchIdx <= 31);
		// assert and then skip the file to make sure that release builds don't die
		if (header.patchIdx > 31)
			continue;

		if (header.languageIdx == UINT16_MAX)
			this->m_streamStates |= (1 << header.patchIdx); // Indicate that a streaming file exists for this patch
		else
		{
			if (!this->m_localisedStreamStates.contains(header.languageIdx))
				this->m_localisedStreamStates[header.languageIdx] = 1 << header.patchIdx;
			else
				this->m_localisedStreamStates[header.languageIdx] |= 1 << header.patchIdx;
		}
	}

	Log("MBNK: Finished discovering streams.\n");
}

const bool CMilesAudioBank::ParseFromHeader()
{
	switch (this->m_version)
	{
	case 13:
	{
		this->languageCount = 11;
		this->languageNames = {
			"english", "french", "german", "spanish", "italian",
			"japanese", "polish", "portuguese", "russian", "tchinese",
			"mspanish"
		};

		const MilesBankHeader_v13_t* const header = reinterpret_cast<MilesBankHeader_v13_t*>(m_fileBuf.get());

		this->Construct(header);
		this->DiscoverStreamingFiles();

		Log("MBNK: Parsing sources...\n");
		for (uint32_t i = 0; i < this->sourceCount; ++i)
		{
			const MilesSource_v13_t* const source = reinterpret_cast<MilesSource_v13_t*>(reinterpret_cast<char*>(this->audioSources) + (i * sizeof(MilesSource_v13_t)));
			MilesSource_t* const sourceAssetData = new MilesSource_t(source);

			if (!IsValidSource(sourceAssetData))
			{
				delete sourceAssetData;
				continue;
			}

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
		this->languageNames = {
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
			{
				delete sourceAssetData;
				continue;
			}

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
		this->languageNames = {
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
			{
				delete sourceAssetData;
				continue;
			}

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
	case 48: // Apex Season 27.1.x 2025_12_05_16_29, released 06/01/2026
	{
		this->languageCount = 10;
		this->languageNames = {
			"english", "french", "german", "spanish", "italian",
			"japanese", "polish", "russian", "mandarin", "korean"
		};

		const MilesBankHeader_v45_t* const header = reinterpret_cast<MilesBankHeader_v45_t*>(m_fileBuf.get());

		this->Construct(header);

		this->DiscoverStreamingFiles();

		const MilesSource_v48_t* const sourceArray = reinterpret_cast<MilesSource_v48_t*>(this->audioSources);

		const size_t sourceNameOffsetDifference = sourceArray[0].nameOffset; // this assumes that the first source's name is always at offset 0. This is Not Good.

		Log("MBNK: Parsing sources...\n");
		for (uint32_t i = 0; i < this->sourceCount; ++i)
		{
			const MilesSource_v48_t* const source = &sourceArray[i];
			MilesSource_t* const sourceAssetData = new MilesSource_t(source);

			if (!IsValidSource(sourceAssetData))
			{
				delete sourceAssetData;
				continue;
			}

			const char* sourceNameStringTable = reinterpret_cast<char*>(this->audioSources) + (sizeof(MilesSource_v48_t) * this->sourceCount);

			const char* const sourceName = sourceNameStringTable + (source->nameOffset - sourceNameOffsetDifference);

			CMilesAudioAsset* sourceAsset = new CMilesAudioAsset(sourceName, sourceAssetData, this);
			sourceAsset->SetAssetType((uint32_t)AssetType_t::ASRC); // asrc - audio source
			sourceAsset->SetAssetGUID(RTech::StringToGuid(sourceName));
			sourceAsset->SetAssetVersion({ m_version });

			sourceAsset->SetContainerName(GetStreamingFileNameForSource(sourceAssetData));

			g_assetData.v_assets.push_back({ sourceAsset->GetAssetGUID(), sourceAsset });
		}

		break;
	}
	case 49: // Apex Season 30.0   2026_07_29_15_44, released 
	{
		this->languageCount = 10;
		this->languageNames = {
			"english", "french", "german", "spanish", "italian",
			"japanese", "polish", "russian", "mandarin", "korean"
		};

		const MilesBankHeader_v49_t* const header = reinterpret_cast<MilesBankHeader_v49_t*>(m_fileBuf.get());

		this->Construct(header);

		this->DiscoverStreamingFiles();

		const MilesSource_v48_t* const sourceArray = reinterpret_cast<MilesSource_v48_t*>(this->audioSources);

		Log("MBNK: Parsing sources...\n");
		for (uint32_t i = 0; i < this->sourceCount; ++i)
		{
			const MilesSource_v48_t* const source = &sourceArray[i];
			MilesSource_t* const sourceAssetData = new MilesSource_t(source);

			if (!IsValidSource(sourceAssetData))
			{
				delete sourceAssetData;
				continue;
			}

			// Thank you Mr Miles for restoring sane offsets!
			const char* const sourceName = this->GetString(source->nameOffset);

			CMilesAudioAsset* sourceAsset = new CMilesAudioAsset(sourceName, sourceAssetData, this);
			sourceAsset->SetAssetType((uint32_t)AssetType_t::ASRC); // asrc - audio source
			sourceAsset->SetAssetGUID(RTech::StringToGuid(sourceName));
			sourceAsset->SetAssetVersion({ m_version });

			sourceAsset->SetContainerName(GetStreamingFileNameForSource(sourceAssetData));

			g_assetData.v_assets.push_back({ sourceAsset->GetAssetGUID(), sourceAsset });
		}

		break;

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

	SetFilePath(path);

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
	else
	{
		Log("MBNK: Loaded bank \"%s\" with %u sources and %u events.\n", this->stringTable, this->sourceCount, this->eventCount);
		return true;
	}
}

extern RSXSettings_t g_rsxSettings;

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

struct DecodedAudioMetadata_t
{
	uint32_t decodeFormat;
	uint32_t sampleCount;
	uint32_t sampleRate;
	uint16_t channelCount;
};

std::optional<std::vector<char>> DecodeAudioDataForSource(const std::filesystem::path& streamPath, MilesSource_t* source, DecodedAudioMetadata_t* metadataOut)
{
	// Open the MSTR file that contains this audio source
	StreamIO streamFile(streamPath, eStreamIOMode::Read);

	MilesStreamHeader_t streamFileHeader = streamFile.read<MilesStreamHeader_t>();

	streamFile.seek(source->streamHeaderOffset);

	char* sourceStreamHeaderData = new char[source->streamHeaderSize];
	streamFile.read(sourceStreamHeaderData, source->streamHeaderSize);

	MilesASIDecoder_t* decoder = nullptr;

	// So far these are thoe only types that have been used in Titanfall 2 and Apex Legends.
	// Rad Audio is only used from Apex Legends Season 24 onwards, before then all audio sources were Bink Audio
	switch (*reinterpret_cast<uint32_t*>(sourceStreamHeaderData))
	{
	case 'RADA': // Rad Audio
		decoder = GetRadAudioDecoder();
		break;
	case 'BCF1': // Bink Audio
		decoder = GetBinkAudioDecoder();
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
		return std::nullopt;
	}

	const bool isBinkA = decoder->decoderType == MILES_DECODER_BINKA;

	struct MilesDecoderInfo {
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
	const auto ASI_dealloc = static_cast<ASI_dealloc_f>(decoder->ASI_dealloc);

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

	userData.audioStreamSize = *(uint32_t*)(container.data() + 0x18) - source->streamHeaderSize;

	const uint8_t sampleValueSize = (parsedMetadata.decodeFormat & 2) == 0 ? sizeof(uint16_t) : sizeof(float);

	const size_t interleavedBufferDataSize = static_cast<size_t>(sampleValueSize) * channels * samplesCount;
	std::vector<char> interleavedBuffer(interleavedBufferDataSize);

	// Buffer for holding the decoded data for each decode_block call.
	std::vector<char> radDecodedData(sampleValueSize * channels * parsedMetadata.maxSamplesPerDecode);

	size_t totalFramesDecoded = 0;
	uint32_t minInputBufferSize = 0; // start off with 0 bytes for input buffer so we can ask the decoder what it wants

	std::vector<char> stream_data;
	while (totalFramesDecoded < samplesCount)
	{
		// Clear the decode buffer just in case something goes wrong
		memset(radDecodedData.data(), 0, radDecodedData.size());

		uint32_t bytesConsumed = 0;
		uint32_t blockSize = 0;

		// If we have not yet established the smallest that our input buffer can be, call getblocksize once to find out
		if (minInputBufferSize == 0)
		{
			if (isBinkA)
			{
				stream_data.resize(8);
				ReadAudioStream(stream_data.data(), 8, &userData);
			}

			ASI_get_block_size(container.data(), stream_data.data(), stream_data.size(), &bytesConsumed, &blockSize, &minInputBufferSize);

			if (!isBinkA)
			{
				// Fetch the smallest possible amount of data to populate the input buffer.
				// Future decode iterations will include this minimum buffer size in their read operation
				stream_data.resize(minInputBufferSize);
				ReadAudioStream(stream_data.data(), stream_data.size(), &userData);
			}
		}

		if (!isBinkA)
		{
			// Make a call to the decoder to find out how much data it wants for the next decode
			ASI_get_block_size(container.data(), stream_data.data(), stream_data.size(), &bytesConsumed, &blockSize, &minInputBufferSize);

			if (blockSize == 0xFFFF)
				break;
		}

		const size_t oldSize = stream_data.size();
		const size_t newSize = isBinkA ? blockSize : blockSize + minInputBufferSize;
		stream_data.resize(newSize);
		ReadAudioStream(stream_data.data() + oldSize, stream_data.size() - oldSize, &userData);

		ASI_get_block_size(container.data(), stream_data.data(), stream_data.size(), &bytesConsumed, &blockSize, &minInputBufferSize);

		// if we have now got a valid decode input buffer
		if (blockSize != 0xFFFF)
		{
			uint32_t decodeBytesConsumed = 0;
			uint32_t samplesDecoded = 0;

			ASI_decode_block(container.data(), stream_data.data(), stream_data.size(), radDecodedData.data(), radDecodedData.size(), &decodeBytesConsumed, &samplesDecoded);

			// The decoder provides us with a non-interleaved buffer which means that
			// each channel's data is separate out into separate locations within the decode buffer
			// before writing to file, the data must be brought back together
			// e.g.: (L - left channel, R - right channel)
			// non-interleaved: LLLLLLRRRRRR
			// interleaved:     LRLRLRLRLRLR
			// https://stackoverflow.com/a/17883834
			// 
			// This may not be valid for other decoders, as miles uses parsedMetadata.decodeFormat to identify the decoded data format
			// and decide how to process the audio immediately after decoding
			for (int channelIdx = 0; channelIdx < channels; ++channelIdx)
			{
				const char* const channelSampleBuffer = radDecodedData.data() + (sampleValueSize * channelIdx * parsedMetadata.maxSamplesPerDecode);

				for (uint32_t sampleIdx = 0; sampleIdx < samplesDecoded; ++sampleIdx)
				{
					// Index in the output buffer from which the channels of this sample begin
					const size_t outputIdx = static_cast<size_t>(channels) * (sampleIdx + totalFramesDecoded);

					if (parsedMetadata.decodeFormat & 2)
						reinterpret_cast<float*>(interleavedBuffer.data())[outputIdx + channelIdx] = reinterpret_cast<const float*>(channelSampleBuffer)[sampleIdx];
					else
						reinterpret_cast<uint16_t*>(interleavedBuffer.data())[outputIdx + channelIdx] = reinterpret_cast<const uint16_t*>(channelSampleBuffer)[sampleIdx];
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

			// i hate this function so so very much
			if (isBinkA)
				minInputBufferSize = 0;
		}
		else
			break;

	}

	if (ASI_dealloc)
		ASI_dealloc(container.data());

	if (metadataOut)
	{
		metadataOut->channelCount = channels;
		metadataOut->decodeFormat = parsedMetadata.decodeFormat;
		metadataOut->sampleCount = samplesCount;
		metadataOut->sampleRate = sampleRate;
	}

	delete[] sourceStreamHeaderData;

	return std::move(interleavedBuffer);
}


bool ExportAudioSourceAsset(CAsset* const asset, const int setting)
{
	UNUSED(setting);

	CMilesAudioAsset* audioAsset = static_cast<CMilesAudioAsset*>(asset);
	CMilesAudioBank* audioBank = asset->GetContainerFile<CMilesAudioBank>();

	// Create exported path + asset path.
	std::filesystem::path exportPath = g_rsxSettings.GetExportDirectory();
	const std::filesystem::path asrcPath(audioAsset->GetAssetName());

	// truncate paths?
	if (g_rsxSettings.exportPathsFull)
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

	DecodedAudioMetadata_t metadata;
	auto decodedDataOpt = DecodeAudioDataForSource(streamPath, source, &metadata);

	if (!decodedDataOpt.has_value())
	{
		Log("Failed to decode audio source.\n");
		return false;
	}

	const std::vector<char>& decodedData = decodedDataOpt.value();
	const uint64_t DataSize = decodedData.size();

	StreamIO outFile(exportPath, eStreamIOMode::Write);

	WAVEHEADER hdr;

	outFile.write(hdr);
	outFile.write(decodedData.data(), decodedData.size());

	hdr.size = static_cast<long>(DataSize + 36);

	hdr.fmt.formatTag = metadata.decodeFormat & DECODE_FORMAT_F32 ? 3 : WAVE_FORMAT_PCM;
	hdr.fmt.channels = metadata.channelCount;
	hdr.fmt.sampleRate = metadata.sampleRate;
	hdr.fmt.blockAlign = static_cast<uint16_t>(DataSize / metadata.sampleCount);
	hdr.fmt.bitsPerSample = static_cast<uint16_t>(((DataSize * 8) / metadata.sampleCount) / metadata.channelCount);

	hdr.data.chunkSize = static_cast<long>(DataSize);

	hdr.fmt.avgBytesPerSecond = hdr.fmt.blockAlign * metadata.sampleRate;

	outFile.seek(0);
	outFile.write(hdr);
	outFile.close();


	return true;
}

void* AudioSource_Preview(CAsset* const asset, const bool firstFrameForAsset)
{
	if (firstFrameForAsset && g_audioPlayer.IsInitialised())
		g_audioPlayer.Shutdown();

	CMilesAudioAsset* audioAsset = static_cast<CMilesAudioAsset*>(asset);
	MilesSource_t* source = reinterpret_cast<MilesSource_t*>(audioAsset->GetAssetData());


	const float progressTime = g_audioPlayer.GetCursorAsTime();
	const float soundLengthTime = source->duration();

	const std::string progress = std::format("{:.3f}", progressTime);

	if (g_audioPlayer.IsInitialised())
	{
		if (!g_audioPlayer.IsAudioFinished())
		{
			if (ImGui::Button(g_audioPlayer.IsPlaying() ? ICON_CI_DEBUG_PAUSE : ICON_CI_DEBUG_START))
				g_audioPlayer.TogglePause();
		}
		else
		{
			if (ImGui::Button(ICON_CI_DEBUG_RESTART))
				g_audioPlayer.Restart();
		}
	}
	else
	{
		if (ImGui::Button(ICON_CI_DEBUG_START))
		{
			CMilesAudioBank* audioBank = asset->GetContainerFile<CMilesAudioBank>();

			const std::filesystem::path asrcPath(audioAsset->GetAssetName());


			// get the bank's path and replace the filename
			// with the stream file name that we've just put together
			std::filesystem::path streamPath(audioBank->GetFilePath());
			streamPath.replace_filename(audioAsset->GetContainerFileName());

			DecodedAudioMetadata_t metadata;
			auto decodedDataOpt = DecodeAudioDataForSource(streamPath, source, &metadata);

			if (!decodedDataOpt.has_value())
			{
				Log("Failed to decode audio source.\n");
				return nullptr;
			}

			g_audioPlayer.Setup(decodedDataOpt.value(),
				metadata.sampleCount, metadata.sampleRate, static_cast<uint8_t>(metadata.channelCount),
				metadata.decodeFormat & DECODE_FORMAT_F32 ? sizeof(float) : sizeof(uint16_t));
		}
	}

	ImGui::SameLine();

	ImGuiExt::ProgressBarCentered(progressTime / soundLengthTime, ImVec2(200.f, 25.f), progress.c_str(), nullptr);
	ImGui::SameLine();
	ImGui::Text("%.3f", soundLengthTime);

	if (source->bpm != 0)
		ImGui::Text("BPM: %u", source->bpm);

	ImGui::Text("Sample Rate: %u", source->sampleRate);

	return nullptr;
}

void InitAudioSourceAssetType()
{
	AssetTypeBinding_t type =
	{
		.name = "Audio Source",
		.type = 'crsa',
		.headerAlignment = 1,
		.loadFunc = nullptr,
		.postLoadFunc = nullptr,
		.previewFunc = AudioSource_Preview,
		.e = { ExportAudioSourceAsset, 0, nullptr, 0ull },
	};

	REGISTER_TYPE(type);
}