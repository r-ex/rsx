#pragma once


// this enum only contains types from pseudocode
// i've only seen 0/2/8/9/10/11/13 in a file
enum EventActionType_e
{
	ACTION_0 = 0, // event play parameters
	ACTION_1 = 1, // supported but unused in r5 as of s30 apex
	ACTION_2 = 2, // unk
	ACTION_8 = 8, // execute events
	ACTION_9 = 9, // set controller value
	ACTION_A = 0xA, // unk
	ACTION_B = 0xB, // unk - something to do with controller names
	ACTION_C = 0xC, // unk - i can't find any code for this but i'm sure it's there somewhere
	ACTION_D = 0xD, // unk - has a count for the number of controllers in ACTION 11. lots of code in event processing func
};

struct EventActionBase_s
{
	uint8_t actionType : 4; // @ 0
	uint8_t isLastAction : 1;
	uint8_t pad : 3;

	uint8_t unk_1; // @ 1

	uint16_t dataSizeDwords; // @ 2 - number of dwords used for this action's data

	template<typename T>
	T* Offset(size_t offset)
	{
		return reinterpret_cast<T*>((char*)this + offset);
	}
};

// vars using this union type can either be a static float value (flValue), or they can point to a graph curve to calculate the value
// this is determined using a bit flag on action::graphFlags
union GraphValue_u
{
	float flValue;
	int graphOffset; // offset within the bank's graph data
};

struct GraphValues_s
{
	float* xValues;
	float* yValues;

	uint32_t numValues;
};

constexpr int ACT_GRAPHFLAG_VOLUME = 1 << 0;
constexpr int ACT_GRAPHFLAG_PITCH = 1 << 2;
constexpr int ACT_GRAPHFLAG_4C = 1 << 9;
constexpr int ACT_GRAPHFLAG_50 = 1 << 10;


struct PlayActionState
{
	int nameOffset; // "NewState", "State 1", "State 2", "State 3"
	char unkCount_4;
	char pad_5;
	unsigned __int8 unsigned___int86;
	char pad_7;
	unsigned __int8 unsigned___int88;
	unsigned __int8 unsigned___int89;
	char gapA[4];
	uint16_t wordE;
	char gap10[2];
	uint16_t word12;
	char gap14[44];
	int dword40;
	int dword44;

	char gap_58[8];
};
static_assert(offsetof(PlayActionState, unsigned___int88) == 8);
static_assert(offsetof(PlayActionState, unsigned___int89) == 9);
static_assert(offsetof(PlayActionState, dword44) == 0x44);
static_assert(sizeof(PlayActionState) == 80);

struct struct_v1
{
	int unkOffset;
	char gap_4[0x20];
};
static_assert(sizeof(struct_v1) == 36);

struct SourceSelector_s
{
	uint8_t type : 3;
	uint8_t unk_0 : 4;
	uint8_t weight;
	uint16_t childCount;
	int unk_4;

	uint16_t* ChildOffsets()
	{
		return reinterpret_cast<uint16_t*>((char*)this + 0xC);
	}

};
class CMilesAudioBank;

struct ParsedSourceSelector
{
	void Construct(CMilesAudioBank* bank, SourceSelector_s* sel, char* base);
	std::vector<ParsedSourceSelector> children;

	std::string name;

	uint8_t weight;

	bool isList;

	void Draw();
};

struct ParsedSourceState
{
	std::string name;
	std::vector<ParsedSourceSelector> selectors;

	void Draw();
};

struct EventAction_0_s : public EventActionBase_s
{
	char unk_0[28];
	uint32_t unkBankOffset_20;
	char unk_24[16];
	uint8_t unkCount_34;
	uint8_t pad_35;
	uint16_t unk_36;
	char unk_38[4];
	GraphValue_u pitch; // 0x4 - pitch?
	GraphValue_u volume; // 0x1 - volume?
	float unk_44;
	float unk_48;
	GraphValue_u unkGraphVal_4C; // 0x200 - unk
	GraphValue_u unkGraphVal_50; // 0x400 - unk
	char unk_54[24];
	int unkGraphOffset_6C;
	char unk_70[2];
	uint16_t unkDwordOffset_72;
	uint16_t unkDwordOffset_74;
	uint16_t unkDwordOffset_76;
	uint16_t unkDwordOffset_78;
	uint16_t unkDwordOffset_7A;
	uint16_t unkDwordOffset_7C;
	uint16_t unkDwordOffset_7E;
	uint32_t graphFlags;
	char gap_84[24];

	std::vector<ParsedSourceState> GetSourceStates(CMilesAudioBank* bank);

};
static_assert(offsetof(EventAction_0_s, unkGraphVal_4C) == 0x4C);
static_assert(offsetof(EventAction_0_s, gap_84) == 0x84);

struct EventAction_2_s : public EventActionBase_s
{
	uint32_t someNameOffset;
	uint8_t flags;
	uint8_t byte9;
	__int16 int16A;
	uint32_t unk_C;
	float unkFloat_10;
	uint32_t unk_14;
};

struct EventAction_8_s : public EventActionBase_s
{
	uint32_t eventCount;
	uint32_t b;
	uint32_t eventNameOffset[1];
};
static_assert(offsetof(EventAction_8_s, eventNameOffset) == 0xC);

// Set Controller Value
struct EventAction_9_s : public EventActionBase_s
{
	float duration;
	float delay;
	float newValue;
	int stringOffset;
	int unk_14;
};

struct EventAction_11_s : public EventActionBase_s
{
	uint32_t controllerNameOffset[1]; // controllerNames[unk_1]
};

struct EventAction_13_s : public EventActionBase_s
{
	uint8_t unk_4;
	uint8_t controllerCount;
};


static const std::unordered_map<EventActionType_e, const char*> s_eventExportTypes =
{
	{ EventActionType_e::ACTION_0, "play" },
	{ EventActionType_e::ACTION_8, "playEvents" },
	{ EventActionType_e::ACTION_9, "setControllerValue" },
	{ EventActionType_e::ACTION_B, "referencedControllers" },
};

static const std::unordered_map<EventActionType_e, const char*> s_eventPreviewNames =
{
	{ EventActionType_e::ACTION_0, "Play" },
	{ EventActionType_e::ACTION_8, "Play Events" },
	{ EventActionType_e::ACTION_9, "Set Controller Value" },
	{ EventActionType_e::ACTION_B, "Referenced Controllers (meta)" },
};

struct MilesEvent_s
{
	~MilesEvent_s()
	{
		for (auto& [action, previewData] : actions)
		{
			if (action) delete[] (char*)action;
			if (previewData) delete previewData;
		}

		actions.clear();
	}

	const void* originalData;
	std::shared_ptr<char[]> decompressedData;

	std::vector<std::pair<EventActionBase_s*, void*>> actions;

	const uint16_t decompressedSize;
	const uint16_t compressedSize;

	bool parsedActions;

	bool ParseActions();

	//
	FORCEINLINE bool IsCompressed() const { return decompressedSize != compressedSize; };
};

struct MilesValueGraph_s;

// Action Preview Data
struct ActionPreviewData_0_s
{
	MilesValueGraph_s* pitchGraph;
	Vector2D pitchMins;
	Vector2D pitchMaxs;

	MilesValueGraph_s* volumeGraph;
	Vector2D volumeMins;
	Vector2D volumeMaxs;

	std::vector<ParsedSourceState> states;
};