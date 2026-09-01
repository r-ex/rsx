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
};

// vars using this union type can either be a static float value (flValue), or they can point to a graph curve to calculate the value
// this is determined using a bit flag on action::graphFlags
union GraphValue_u
{
	float flValue;
	int graphOffset; // offset within the bank's graph data
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
	{ EventActionType_e::ACTION_8, "executeEvents" },
	{ EventActionType_e::ACTION_9, "setControllerValue" },
	{ EventActionType_e::ACTION_B, "unkControllers" },
};

struct MilesEvent_s
{
	~MilesEvent_s()
	{
		for (auto& action : actions)
		{
			if (action) delete[] (char*)action;
		}

		actions.clear();
	}

	const void* originalData;
	std::shared_ptr<char[]> decompressedData;

	std::vector<EventActionBase_s*> actions;

	const uint16_t decompressedSize;
	const uint16_t compressedSize;

	bool parsedActions;

	bool ParseActions();

	//
	FORCEINLINE bool IsCompressed() const { return decompressedSize != compressedSize; };
};