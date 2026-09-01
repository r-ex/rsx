#include <pch.h>
#include "event.h"
#include "miles.h"

static void MilesAction_2(std::stringstream& rson, CMilesAudioAsset* asset, const EventActionBase_s* const action)
{
	const EventAction_2_s* act = reinterpret_cast<const EventAction_2_s*>(action);
	CMilesAudioBank* audioBank = asset->GetContainerFile<CMilesAudioBank>();

	// i think someNameOffset is for an event name since it begins with /general/
	rson << "\t\tevent: " << ((act->someNameOffset == UINT32_MAX) ? "(none)" : audioBank->GetString(act->someNameOffset)) << "\n"
		<< "\t\tflags: " << std::hex << (int)act->flags << "\n"
		<< "\t\tunk_9: " << (int)act->byte9 << "\n"
		<< "\t\tunk_A: " << act->int16A << "\n"
		<< "\t\tunk_C: " << act->unk_C << "\n"
		<< "\t\tunk_10: " << act->unkFloat_10 << "\n"
		<< "\t\tunk_14: " << act->unk_14 << "\n";	
}

static void MilesAction_8(std::stringstream& rson, CMilesAudioAsset* asset, const EventActionBase_s* const action)
{
	const EventAction_8_s* act = reinterpret_cast<const EventAction_8_s*>(action);
	CMilesAudioBank* audioBank = asset->GetContainerFile<CMilesAudioBank>();

	rson << "\t\tevents:";
	if (act->eventCount == 0)
		rson << " []\n";
	else
	{
		rson << "\n\t\t[\n";
		for (uint32_t i = 0; i < act->eventCount; ++i)
		{
			rson << "\t\t\t\"" << audioBank->GetString(act->eventNameOffset[i]) << "\"\n";
		}
		rson << "\t\t]\n";
	}
}

static void MilesAction_9(std::stringstream& rson, CMilesAudioAsset* asset, const EventActionBase_s* const action)
{
	const EventAction_9_s* act = reinterpret_cast<const EventAction_9_s*>(action);
	CMilesAudioBank* audioBank = asset->GetContainerFile<CMilesAudioBank>();

	rson << "\t\tcontroller: " << audioBank->GetString(act->stringOffset) << "\n"
		<< "\t\tduration: " << act->duration << "\n"
		<< "\t\tdelay: " << act->delay << "\n"
		<< "\t\tvalue: " << act->newValue << "\n";
}

static void MilesAction_11(std::stringstream& rson, CMilesAudioAsset* asset, const EventActionBase_s* const action)
{
	const EventAction_11_s* act = reinterpret_cast<const EventAction_11_s*>(action);
	CMilesAudioBank* audioBank = asset->GetContainerFile<CMilesAudioBank>();

	uint32_t count = (act->unk_1);

	rson << "\t\tcontrollers:";
	if (count == 0)
		rson << " []\n";
	else
	{
		rson << "\n\t\t[\n";
		for (uint32_t i = 0; i < count; ++i)
		{
			rson << "\t\t\t\"" << audioBank->GetString(act->controllerNameOffset[i]) << "\"\n";
		}
		rson << "\t\t]\n";
	}
}

void MilesEvent_WriteActionToRSONStream(std::stringstream& rson, CMilesAudioAsset* asset, const EventActionBase_s* const action)
{
	rson << "\t{\n";
	rson << "\t\ttype: " << (s_eventExportTypes.contains((EventActionType_e)action->actionType) ? s_eventExportTypes.at((EventActionType_e)action->actionType) : std::format("unnamed_{}", action->actionType)) << "\n";

	switch (action->actionType)
	{
	case 2:
	{
		MilesAction_2(rson, asset, action);
		break;
	}
	case 8:
	{
		MilesAction_8(rson, asset, action);
		break;
	}
	case 9:
	{
		MilesAction_9(rson, asset, action);
		break;
	}
	case 11:
	{
		MilesAction_11(rson, asset, action);
		break;
	}
	}

	rson << "\t}\n";
}