#include "pch.h"
#include "miles.h"

#include <game/audio/wavefile.h>
#include <game/rtech/utils/utils.h>
#include <imgui.h>
#include <miniaudio/miniaudio.h>

#include <core/audio/audioplayer.h>
#include <core/fonts/codicons.h>
#include <misc/imgui_utility.h>

// RAD LZB - compressed event data
#include <rad_lzb_simple/rad_lzb_simple.h>
#include "source.h"
#include "event.h"

CMilesAudioAsset::~CMilesAudioAsset()
{
	delete (MilesSource_t*)m_assetData;
};

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

template<typename T>
static void MilesBank_ParseSources(CMilesAudioBank* bank)
{
	const T* const sourceArray = reinterpret_cast<const T*>(bank->GetSourceData());

	// This is only used for v48, but it saves on code complexity to just define this on all versions of this func
	const size_t sourceNameOffsetDifference = sourceArray[0].nameOffset;

	Log("MBNK: Parsing sources...\n");
	for (uint32_t i = 0; i < bank->GetSourceCount(); ++i)
	{
		const T* const srcData = &sourceArray[i];

		MilesSource_t* const sourceAssetData = new MilesSource_t(srcData);

		if (!bank->IsValidSource(sourceAssetData))
		{
			delete sourceAssetData;
			continue;
		}

		const uint32_t markerOffset = srcData->markerOffset;
		const uint8_t markerCount = srcData->markerCount;

		if (bank->GetMarkers() != nullptr) // only try and get markers if there is actually a marker pointer
		{
			for (uint32_t j = 0; j < markerCount; ++j)
			{
				const MilesAudioMarker_t* marker = bank->GetMarkers() + ((markerOffset / sizeof(MilesAudioMarker_t)) + j);

				sourceAssetData->audioMarkers.emplace_back(bank->GetString(marker->nameOffset), marker->framePosition);
			}
		}

		const char* sourceName = nullptr;
		
		// Source names are quite weird on v48, so we have to do this ugly special case for it
		if (typeid(T) == typeid(MilesSource_v48_t))
		{
			const char* sourceNameStringTable = reinterpret_cast<const char*>(sourceArray) + (sizeof(T) * bank->GetSourceCount());

			sourceName = sourceNameStringTable + (srcData->nameOffset - sourceNameOffsetDifference);
		}
		else
			// Thank you Mr Miles for restoring sane offsets in v49!
			sourceName = bank->GetString(srcData->nameOffset);

		CMilesAudioAsset* sourceAsset = new CMilesAudioAsset(sourceName, sourceAssetData, bank);
		sourceAsset->SetAssetType((uint32_t)AssetType_t::ASRC); // asrc - audio source
		sourceAsset->SetAssetGUID(RTech::StringToGuid(sourceName));
		sourceAsset->SetAssetVersion({ bank->GetVersion() });

		sourceAsset->SetContainerName(bank->GetStreamingFileNameForSource(sourceAssetData));

		if (sourceAsset != nullptr)
			g_assetData.v_assets.push_back({ sourceAsset->GetAssetGUID(), sourceAsset });
	}
}


static void MilesBank_ParseEvents(CMilesAudioBank* bank)
{
	const EventName_s* const evNameArray = bank->GetEventNamesData();
	const void* const evData = bank->GetEventActionsData();

	if (!evNameArray || !evData)
	{
		Log("MBNK: Failed to parse events. Missing eventName or eventAction pointer\n");
		return;
	}

	Log("MBNK: Parsing events...\n");
	for (uint32_t i = 0; i < bank->GetEventCount(); ++i)
	{
		const EventName_s* const evName = &evNameArray[i];
		const char* const eventData = reinterpret_cast<const char*>(evData) + evName->dataOffset;

		const uint16_t decompSize = reinterpret_cast<const uint16_t*>(eventData)[0];
		const uint16_t compSize = reinterpret_cast<const uint16_t*>(eventData)[1];

		MilesEvent_s* event = new MilesEvent_s(eventData + 4, nullptr, {}, decompSize, compSize, false);

		const char* eventName = bank->GetString(evName->nameOffset);

		CMilesAudioAsset* eventAsset = new CMilesAudioAsset(eventName, event, bank);
		eventAsset->SetAssetType((uint32_t)AssetType_t::AEVT); // asrc - audio source
		eventAsset->SetAssetGUID(RTech::StringToGuid(eventName));
		eventAsset->SetAssetVersion({ bank->GetVersion() });

		eventAsset->SetContainerName(std::string(bank->GetBankStem()) + ".mbnk");

		if (eventAsset != nullptr)
			g_assetData.v_assets.push_back({ eventAsset->GetAssetGUID(), eventAsset });
	}
}

const bool CMilesAudioBank::ParseFromHeader()
{
	switch (this->m_version)
	{
	case 13:
	{
		this->languageNames = {
			"english", "french", "german", "spanish", "italian",
			"japanese", "polish", "portuguese", "russian", "tchinese",
			"mspanish"
		};

		const MilesBankHeader_v13_t* const header = reinterpret_cast<MilesBankHeader_v13_t*>(m_fileBuf.get());

		this->Construct(header);
		this->DiscoverStreamingFiles();

		MilesBank_ParseSources<MilesSource_v13_t>(this);

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
		this->languageNames = {
			"english", "french", "german", "spanish", "italian",
			"japanese", "polish", "russian", "mandarin"
		};

		const MilesBankHeader_v28_t* const header = reinterpret_cast<MilesBankHeader_v28_t*>(m_fileBuf.get());

		this->Construct(header);

		this->DiscoverStreamingFiles();

		MilesBank_ParseSources<MilesSource_v28_t>(this);

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
		this->languageNames = {
			"english", "french", "german", "spanish", "italian",
			"japanese", "polish", "russian", "mandarin", "korean"
		};

		const MilesBankHeader_v45_t* const header = reinterpret_cast<MilesBankHeader_v45_t*>(m_fileBuf.get());

		this->Construct(header);

		this->DiscoverStreamingFiles();

		MilesBank_ParseSources<MilesSource_v39_t>(this);

		break;
	}
	case 48: // Apex Season 27.1.x 2025_12_05_16_29, released 06/01/2026
	{
		this->languageNames = {
			"english", "french", "german", "spanish", "italian",
			"japanese", "polish", "russian", "mandarin", "korean"
		};

		const MilesBankHeader_v45_t* const header = reinterpret_cast<MilesBankHeader_v45_t*>(m_fileBuf.get());

		this->Construct(header);

		this->DiscoverStreamingFiles();

		MilesBank_ParseSources<MilesSource_v48_t>(this);

		break;
	}
	case 49: // Apex Season 30.0   2026_07_29_15_44, released 
	{
		this->languageNames = {
			"english", "french", "german", "spanish", "italian",
			"japanese", "polish", "russian", "mandarin", "korean"
		};

		const MilesBankHeader_v49_t* const header = reinterpret_cast<MilesBankHeader_v49_t*>(m_fileBuf.get());

		this->Construct(header);

		this->DiscoverStreamingFiles();

		MilesBank_ParseSources<MilesSource_v49_t>(this);
		MilesBank_ParseEvents(this);

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
