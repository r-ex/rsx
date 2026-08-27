#pragma once


// this enum only contains types from pseudocode
// i've only seen 0/2/8/9/10/11/13 in a file
enum EventActionType_e
{
	ACTION_0 = 0,
	ACTION_1 = 1,
	ACTION_2 = 2,
	ACTION_8 = 8,
	ACTION_9 = 9,
	ACTION_A = 0xA,
	ACTION_B = 0xB,
	ACTION_C = 0xC,
	ACTION_D = 0xD,
};

// 0: Variable size
// 2: 24 bytes
// 8: 16 bytes
// 11: Variable size
// 13: 24 bytes
// 

struct EventActionBase_s
{
	uint8_t actionType : 4; // @ 0
	uint8_t isLastAction : 1;
	uint8_t pad : 3;

	uint8_t unk_1; // @ 1

	uint16_t dataSizeDwords; // @ 2 - number of dwords used for this action's data
};

struct EventAction_0_s : public EventActionBase_s
{

};

struct EventAction_8_s : public EventActionBase_s
{
	char unk_4[8];
	uint32_t eventNameOffset;
};
static_assert(offsetof(EventAction_8_s, eventNameOffset) == 0xC);

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