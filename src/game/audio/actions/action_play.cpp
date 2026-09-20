#include "pch.h"
#include <game/audio/miles.h>
#include <game/audio/event.h>

static MilesEventActionParseResult_e ActionPlay_Parse_v13(CMilesAudioAsset* asset, EventAction_0_v13_s* action, ActionPreviewData_0_s* previewData)
{
	CMilesAudioBank* audioBank = asset->GetContainerFile<CMilesAudioBank>();

	//previewData->states = action->GetSourceStates(audioBank);

	if (action->graphFlags & ACT_GRAPHFLAG_PITCH)
	{
		previewData->pitchGraph = reinterpret_cast<MilesValueGraph_s*>((char*)audioBank->GetGraphData() + action->pitch.graphOffset);

		const std::pair<Vector2D, Vector2D> minsMaxs = previewData->pitchGraph->MinsMaxs();

		previewData->pitchMins = minsMaxs.first;
		previewData->pitchMaxs = minsMaxs.second;
	}

	if (action->graphFlags & ACT_GRAPHFLAG_VOLUME)
	{
		previewData->volumeGraph = reinterpret_cast<MilesValueGraph_s*>((char*)audioBank->GetGraphData() + action->volume.graphOffset);

		const std::pair<Vector2D, Vector2D> minsMaxs = previewData->volumeGraph->MinsMaxs();

		previewData->volumeMins = minsMaxs.first;
		previewData->volumeMaxs = minsMaxs.second;
	}

	return MilesEventActionParseResult_e::RESULT_SUCCESS;
}

// not actually from v49 but idk when the struct was first used
static MilesEventActionParseResult_e ActionPlay_Parse_v49(CMilesAudioAsset* asset, EventAction_0_s* action, ActionPreviewData_0_s* previewData)
{
	CMilesAudioBank* audioBank = asset->GetContainerFile<CMilesAudioBank>();

	previewData->states = action->GetSourceStates(audioBank);

	if (action->graphFlags & ACT_GRAPHFLAG_PITCH)
	{
		previewData->pitchGraph = reinterpret_cast<MilesValueGraph_s*>((char*)audioBank->GetGraphData() + action->pitch.graphOffset);

		const std::pair<Vector2D, Vector2D> minsMaxs = previewData->pitchGraph->MinsMaxs();

		previewData->pitchMins = minsMaxs.first;
		previewData->pitchMaxs = minsMaxs.second;
	}

	if (action->graphFlags & ACT_GRAPHFLAG_VOLUME)
	{
		previewData->volumeGraph = reinterpret_cast<MilesValueGraph_s*>((char*)audioBank->GetGraphData() + action->volume.graphOffset);

		const std::pair<Vector2D, Vector2D> minsMaxs = previewData->volumeGraph->MinsMaxs();

		previewData->volumeMins = minsMaxs.first;
		previewData->volumeMaxs = minsMaxs.second;
	}

	return MilesEventActionParseResult_e::RESULT_SUCCESS;
}


MilesEventActionParseResult_e ActionPlay_Parse(CMilesAudioAsset* asset, void* actionData, void** out_previewData)
{
	if (!out_previewData)
		return MilesEventActionParseResult_e::RESULT_INVALID_PREVIEW_DATA;

	if (*out_previewData) delete reinterpret_cast<ActionPreviewData_0_s*>(*out_previewData);

	ActionPreviewData_0_s* pd = new ActionPreviewData_0_s();
	*out_previewData = pd;

	switch (asset->GetAssetVersion().majorVer)
	{
	case 13:
		return ActionPlay_Parse_v13(asset, reinterpret_cast<EventAction_0_v13_s*>(actionData), pd);
	case 49:
		return ActionPlay_Parse_v49(asset, reinterpret_cast<EventAction_0_s*>(actionData), pd);
	}

	return MilesEventActionParseResult_e::RESULT_UNSUPPORTED;
}