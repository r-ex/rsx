#include <pch.h>
#include "event.h"
#include <game/asset.h>
#include "miles.h"

#include <thirdparty/rad_lzb_simple/rad_lzb_simple.h>
#include <imgui/misc/imcurve_editor.hpp>

bool MilesEvent_s::ParseActions()
{
	decompressedData = std::make_shared<char[]>(decompressedSize);

	// If not compressed, the original data ptr has the full decompressed data of the event's actions playlist
	if (!IsCompressed())
		memcpy_s(decompressedData.get(), decompressedSize, originalData, decompressedSize);
	else
	{
		const SINTa consumed = rr_lzb_simple_decode(originalData, compressedSize, decompressedData.get(), decompressedSize);

		if (consumed != compressedSize)
			return false;
	}

	const char* cursor = decompressedData.get();
	while (true)
	{
		const EventActionBase_s* base = reinterpret_cast<const EventActionBase_s*>(cursor);
		const size_t actionSize = base->dataSizeDwords * sizeof(DWORD);

		char* actionData = new char[actionSize];

		memcpy_s(actionData, actionSize, base, actionSize);

		this->actions.push_back(reinterpret_cast<EventActionBase_s*>(actionData));

		cursor += actionSize;

		if (base->isLastAction)
			break;
	}

	parsedActions = true;

	return true;
}

static ImCurveEditor<float> pitchGraph;
static ImCurveEditor<float> volumeGraph;

void* PreviewAudioEventAsset(CAsset* const asset, const bool firstFrameForAsset)
{
	CMilesAudioAsset* audioAsset = reinterpret_cast<CMilesAudioAsset*>(asset);
	MilesEvent_s* event = reinterpret_cast<MilesEvent_s*>(audioAsset->GetAssetData());
	CMilesAudioBank* audioBank = asset->GetContainerFile<CMilesAudioBank>();

	if (firstFrameForAsset)
	{
		pitchGraph = {};

		if (!event->parsedActions)
		{
			if (!event->ParseActions())
				printf("Failed to parse!\n");
		}

		for (auto& action : event->actions)
		{
			if (action->actionType == 0)
			{
				EventAction_0_s* act = reinterpret_cast<EventAction_0_s*>(action);

				if (act->graphFlags & 4)
				{
					ImCurve<float> pitchCurve;

					MilesValueGraph_s* graph = reinterpret_cast<MilesValueGraph_s*>((char*)audioBank->GetGraphData() + act->pitch.graphOffset);

					pitchCurve.Points.resize(graph->numValues);
					for (int i = 0; i < graph->numValues; ++i)
					{
						pitchCurve.Points[i].Points[ImCurvePointType_Start] = { graph->XValues()[i], graph->Segments()[i].start };
					}

					for(int i = 0; i < graph->numValues; ++i)
					{
						auto& segment = graph->Segments()[i];
						auto& point = pitchCurve.Points[i];

						switch (segment.mode)
						{
						case 0:
							point.SetInterpolationType(ImCurveInterpolationType_Linear);
							break;
						case 2:
							point.SetInterpolationType(ImCurveInterpolationType_Square);
							break;
						case 3:
							assert(i != graph->numValues - 1);
							point.SetInterpolationType(ImCurveInterpolationType_Quadratic, pitchCurve.Points[i+1]);
							break;
						}

					}
					pitchGraph = ImCurveEditor<float>(pitchCurve);
				}
			}
		}
	}

	pitchGraph.Draw("Pitch Graph");

	return nullptr;
}

extern void MilesEvent_WriteActionToRSONStream(std::stringstream& rson, CMilesAudioAsset* asset, const EventActionBase_s* const action);

bool ExportAudioEventAsset(CAsset* const asset, int type)
{
	UNUSED(type);
	CMilesAudioAsset* audioAsset = reinterpret_cast<CMilesAudioAsset*>(asset);
	MilesEvent_s* event = reinterpret_cast<MilesEvent_s*>(audioAsset->GetAssetData());

	if (!event->parsedActions && !event->ParseActions())
	{
		printf("Failed to parse!\n");
		return false;
	}
	//CMilesAudioBank* audioBank = asset->GetContainerFile<CMilesAudioBank>();

	// Create exported path + asset path.
	std::filesystem::path exportPath = g_rsxSettings.GetExportDirectory();
	const std::filesystem::path aevtPath(audioAsset->GetAssetName());

	// truncate paths?
	if (g_rsxSettings.exportPathsFull)
		exportPath.append(aevtPath.parent_path().string());
	else
		exportPath.append("events");

	if (!CreateDirectories(exportPath))
	{
		assertm(false, "Failed to create asset type directory.");
		return false;
	}

	exportPath.append(aevtPath.filename().string() + ".rson");

	std::stringstream rson;

	rson
		<< "eventName: " << aevtPath.filename() << "\n"
		<< "actions:\n[\n";

	const std::unordered_set<uint8_t> types = { 13 };
	bool shouldWrite = false;

	for (auto& it : event->actions)
	{
		if (!shouldWrite && types.contains(it->actionType))
			shouldWrite = true;

		MilesEvent_WriteActionToRSONStream(rson, audioAsset, it);
	}

	rson << "]\n";

	if (shouldWrite)
	{
		StreamIO sio(exportPath, eStreamIOMode::Write);

		sio.write(rson.str().c_str(), rson.str().length());

		sio.close();

		//size_t i = 0;
		//for (auto& it : event->actions)
		//{
		//	if (!types.contains(it->actionType))
		//		continue;

		//	exportPath.replace_filename(std::format("{}_{}.{}.bin", aevtPath.filename().string(), i, (int)it->actionType));
		//	sio = StreamIO(exportPath, eStreamIOMode::Write);

		//	sio.write((char*)it, it->dataSizeDwords * 4);

		//	sio.close();

		//	i++;
		//}
	}

	return true;
}

void InitAudioEventAssetType()
{
	AssetTypeBinding_t type =
	{
		.name = "Audio Event",
		.type = 'tvea',
		.headerAlignment = 1,
		.loadFunc = nullptr,
		.postLoadFunc = nullptr,
		.previewFunc = PreviewAudioEventAsset,
		.e = { ExportAudioEventAsset, 0, nullptr, 0ull },
	};

	REGISTER_TYPE(type);
}