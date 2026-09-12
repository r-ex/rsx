#pragma once

// from RoyalBlue's Kilometer
struct MilesController_v13_s
{
	uint32_t nameOffset;
	float min;
	float max;
	float defaultValue;
	uint8_t uint8_10;
	uint8_t uint8_11;
	uint8_t gap_12[2];
	int mathFuncOffset;
	int16_t word_18;
	uint8_t byte_1A;
	int8_t type;
};
static_assert(sizeof(MilesController_v13_s) == 0x1C);

struct MilesController_v46_s
{
	uint32_t nameOffset;
	float min;
	float max;
	float defaultValue;

	char gap_10[0x1B];
	int8_t type;
	char gap_2C[4];
};
static_assert(offsetof(MilesController_v46_s, type) == 0x2B);
static_assert(sizeof(MilesController_v46_s) == 0x30);

struct MilesController_s
{
	MilesController_s() = default;
	MilesController_s(const char* stringTable, const MilesController_v13_s& a);
	MilesController_s(const char* stringTable, const MilesController_v46_s& a);

	std::string name;
	float min;
	float max;
	float defaultValue;

	int mathFuncOffset;
	int8_t type;
};

struct MilesBus_v46_s
{
	uint32_t nameOffset;
	uint8_t channelCount;
	char gap_5[3];
	void* reserved_bus;
	char gap_10[12];
	float reserved_pitchScalar;
	float reserved_volumeLevel;
	char gap_24[16];
	int appliedDuckingIdx;

	__int16 weirdBusIdx_38; // this is used to walk thru a load of different buses until it reaches one with gap_4[1] set
	__int16 busIdx_3A;
	__int16 busIdx_3C;
	__int16 busIdx_3E;
	__int16 busIdx_40;
	__int16 busIdx_42;
	__int16 busIdx_44;
	__int16 busIdx_46;

	float outputGainVolumeDb;
	char gap_4C[28];
	float volumeDb;
	float pitchSt;
	char gap_70[16];
};
static_assert(sizeof(MilesBus_v46_s) == 0x80);
static_assert(offsetof(MilesBus_v46_s, busIdx_3A) == 0x3A);
static_assert(offsetof(MilesBus_v46_s, busIdx_46) == 0x46);

struct MilesBus_s
{
	MilesBus_s() = default;
	MilesBus_s(const char* stringTable, const MilesBus_v46_s& a);

	std::string name;
	float volumeDb;
	float outputGainVolumeDb;
	float pitchSt;

	__int16 busIdx_3A;
	__int16 busIdx_3C;
	__int16 busIdx_3E;
	__int16 busIdx_40;
	__int16 busIdx_42;
	__int16 busIdx_44;
	__int16 busIdx_46;

	bool isExported;
	uint8_t channelCount;
};

struct MilesProjectHeader_Short_s
{
	int magic;
	int version;
};

struct MilesProjectHeader_v46_s
{
	int magic;
	int version;
	uint32_t fileSize;
	uint32_t hash;

	OffsetPtr_t controllers;
	OffsetPtr_t strings;
	OffsetPtr_t graphData;
	OffsetPtr_t buses;

	// the rest of these may be inaccurate as they are based on the titanfall 2 version of this struct
    OffsetPtr_t busNames;
    OffsetPtr_t busIndices;
    OffsetPtr_t busExports;
    OffsetPtr_t filters;
    OffsetPtr_t appliedDucking;
    OffsetPtr_t duckNames;
    OffsetPtr_t ducks;
    OffsetPtr_t bankNames;
    OffsetPtr_t unk_68;
    OffsetPtr_t voiceLimitNames;

    OffsetPtr_t unk_80;
    OffsetPtr_t unk_88;

    OffsetPtr_t functions; // seems to be at the correct offset
    OffsetPtr_t languages;
	OffsetPtr_t unk_A0; // unkCount_E4 * 8
	OffsetPtr_t unk_A8; // unkCount_101
	OffsetPtr_t convolutionSettings;
	OffsetPtr_t convolutionSizes; // sizes of the data in convolutionSettings
	OffsetPtr_t unk_C0;
	OffsetPtr_t unk_C8;
	OffsetPtr_t unk_D0;
	OffsetPtr_t unk_D8;

	uint32_t convolutionSettingsCount;
	uint32_t unkCount_E4; // unk_A0

	uint32_t languageCount;
	uint32_t voiceLimitCount;
	uint32_t duckCount;
	uint32_t controllerCount;
	uint32_t busCount;
	char gap_FC[2];
	bool hasAllBusNames;
	bool hasAllUnkNames; // unk_80
	uint8_t exportedBusCount;
	uint8_t unkCount_101; // unk_A8
    // there's more but idk what they are
};

static_assert(offsetof(MilesProjectHeader_v46_s, convolutionSettingsCount) == 0xE0);

class CMilesAudioProject
{
public:
	CMilesAudioProject() {};
	~CMilesAudioProject() = default;

	const bool ParseFile(const std::filesystem::path& path);
	const bool ParseFromHeader();

	void DrawFileInfoWindow();

	int GetVersion() const { return fileVersion; };

	const std::vector<std::string>& GetLanguageNames() const { return languageNames; }

	const char* GetString(uint64_t offset) const
	{
		return reinterpret_cast<const char*>(stringTable) + offset;
	}

	template <typename T>
	const T* GetPtr(uint64_t offset) const
	{
		return reinterpret_cast<const T*>(m_fileBuf.get() + offset);
	}

	template <typename T>
	T* GetPtr(uint64_t offset)
	{
		return reinterpret_cast<T*>(m_fileBuf.get() + offset);
	}

	template <typename T>
	const T* GetPtr(const OffsetPtr_t& ptr) const
	{
		return reinterpret_cast<const T*>(m_fileBuf.get() + ptr.offset);
	}

	template <typename T>
	T* GetPtr(const OffsetPtr_t& ptr)
	{
		return reinterpret_cast<T*>(m_fileBuf.get() + ptr.offset);
	}

	// getters

	const std::filesystem::path& GetFilePath() const { return filePath; };

	const void* GetGraphData() const
	{
		return graphData;
	}

private:

	std::filesystem::path filePath;
	std::shared_ptr<char[]> m_fileBuf;

	std::vector<std::string> languageNames;
	std::vector<MilesController_s> controllers;
	std::vector<MilesBus_s> buses;

	uint32_t buildTag;

	void* graphData;

	const char* stringTable;

	int fileVersion;

	bool hasAllBusNames;


	/*void Construct(const MilesBankHeader_v13_t* const header)
	{

	}

	void Construct(const MilesBankHeader_v28_t* const header)
	{

	}

	void Construct(const MilesBankHeader_v45_t* const header)
	{

	}*/

	void Construct(const MilesProjectHeader_v46_s* const header)
	{
		this->stringTable = GetPtr<const char>(header->strings);
		this->graphData = GetPtr<void>(header->graphData);

		this->languageNames.resize(header->languageCount);
		this->controllers.resize(header->controllerCount);
		this->buses.resize(header->busCount);

		this->hasAllBusNames = header->hasAllBusNames;

		for (uint32_t i = 0; i < header->languageCount; ++i)
		{
			this->languageNames.at(i) = GetString(GetPtr<int>(header->languages)[i]);
		}

		for (uint32_t i = 0; i < header->controllerCount; ++i)
		{
			this->controllers.at(i) = MilesController_s(this->stringTable, GetPtr<MilesController_v46_s>(header->controllers)[i]);
		}

		for (uint32_t i = 0; i < header->busCount; ++i)
		{
			this->buses.at(i) = MilesBus_s(this->stringTable, GetPtr<MilesBus_v46_s>(header->buses)[i]);

			// If the project is compiled without bus names, we have to generate new ones!
			if (!this->hasAllBusNames) this->buses.at(i).name = std::format("Bus {}", i);
		}

		if (!this->hasAllBusNames)
		{
			for (uint32_t i = 0; i < header->exportedBusCount; ++i)
			{
				int* exportedBus = GetPtr<int>(header->busExports) + (i * 2);

				auto& bus = this->buses.at(exportedBus[1]);

				bus.name = GetString(exportedBus[0]);
				bus.isExported = true;
			}
		}
	}
};