#include <pch.h>
#include "event.h"
#include <game/asset.h>
#include "miles.h"

#include <thirdparty/rad_lzb_simple/rad_lzb_simple.h>
#include <implot/implot.h>

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

		this->actions.push_back({ reinterpret_cast<EventActionBase_s*>(actionData), nullptr });

		cursor += actionSize;

		if (base->isLastAction)
			break;
	}

	parsedActions = true;

	return true;
}

struct ActionPreviewData_0_s
{
	MilesValueGraph_s* pitchGraph;
	Vector2D pitchMins;
	Vector2D pitchMaxs;

	MilesValueGraph_s* volumeGraph;
	Vector2D volumeMins;
	Vector2D volumeMaxs;
};

void* PreviewAudioEventAsset(CAsset* const asset, const bool firstFrameForAsset)
{
	CMilesAudioAsset* audioAsset = reinterpret_cast<CMilesAudioAsset*>(asset);
	MilesEvent_s* event = reinterpret_cast<MilesEvent_s*>(audioAsset->GetAssetData());
	CMilesAudioBank* audioBank = asset->GetContainerFile<CMilesAudioBank>();

	const ImVec2 avail = ImGui::GetContentRegionAvail();

	if (firstFrameForAsset)
	{
		if (!event->parsedActions)
		{
			if (!event->ParseActions())
				printf("Failed to parse!\n");
		}

		for (auto& [action, previewData] : event->actions)
		{
			if (action->actionType == 0)
			{
				EventAction_0_s* act = reinterpret_cast<EventAction_0_s*>(action);

				if (previewData) delete previewData;

				ActionPreviewData_0_s* pd = new ActionPreviewData_0_s();
				previewData = pd;

				if (act->graphFlags & ACT_GRAPHFLAG_PITCH)
				{
					pd->pitchGraph = reinterpret_cast<MilesValueGraph_s*>((char*)audioBank->GetGraphData() + act->pitch.graphOffset);

					const std::pair<Vector2D, Vector2D> minsMaxs = pd->pitchGraph->MinsMaxs();

					pd->pitchMins = minsMaxs.first;
					pd->pitchMaxs = minsMaxs.second;
				}

				if (act->graphFlags & ACT_GRAPHFLAG_VOLUME)
				{
					pd->volumeGraph = reinterpret_cast<MilesValueGraph_s*>((char*)audioBank->GetGraphData() + act->volume.graphOffset);

					const std::pair<Vector2D, Vector2D> minsMaxs = pd->volumeGraph->MinsMaxs();

					pd->volumeMins = minsMaxs.first;
					pd->volumeMaxs = minsMaxs.second;
				}

			}
		}
	}

	size_t i = 0;
	for (auto& [action, previewData] : event->actions)
	{
		if (action->actionType == 0)
		{
			ActionPreviewData_0_s* pd = reinterpret_cast<ActionPreviewData_0_s*>(previewData);

			const bool hasAnyGraphs = (pd->pitchGraph != nullptr && pd->pitchGraph->numPoints != 0) || (pd->volumeGraph != nullptr && pd->volumeGraph->numPoints != 0);

			auto lambda = [](int idx, void* data) {
				MilesValueGraph_s* graph = reinterpret_cast<MilesValueGraph_s*>(data);

				const float startX = graph->XValues()[0];
				const float endX = graph->XValues()[graph->numPoints - 1];

				const uint32_t totalSamples = std::max(((graph->numPoints) * 10) - 1, 1);

				const float thisX = std::lerp(startX, endX, (idx) / (float)totalSamples);

				int chosenPoint = graph->numPoints-1;
				float minX = -1, maxX = -1;

				for (int i = 0; i < graph->numPoints; ++i)
				{
					maxX = graph->XValues()[i];

					// when we get the first point's X that is larger than this sample's X
					// get the index of the previous point since its segment will contain this sample
					if (maxX > thisX)
					{
						chosenPoint = i-1;

						minX = graph->XValues()[chosenPoint];
						break;
					}
				}

				const MilesGraphSegment_s* segment = &graph->Segments()[chosenPoint];


				return ImPlotPoint(thisX, segment->Sample(thisX, minX, maxX));
			};

			auto scatterLambda = [](int idx, void* data) {
				MilesValueGraph_s* graph = reinterpret_cast<MilesValueGraph_s*>(data);

				const float startX = graph->XValues()[idx];
				
				const MilesGraphSegment_s* segment = &graph->Segments()[idx];

				return ImPlotPoint(startX, segment->start);
				};

			ImPlot::PushStyleVar(ImPlotStyleVar_FitPadding, ImVec2(0.1f, 0.1f));

			if (hasAnyGraphs && ImPlot::BeginSubplots(std::format("Action {} Controller Graphs", i).c_str(), 1, 2, avail))
			{
				if (pd->pitchGraph && pd->pitchGraph->numPoints != 0 && ImPlot::BeginPlot(std::format("Pitch##Action{}", i).c_str())) {
					ImPlot::SetupAxes(pd->pitchGraph->baseControllerNameOffset != UINT32_MAX ? audioBank->GetString(pd->pitchGraph->baseControllerNameOffset) : "n/a", "Pitch (st)", ImPlotAxisFlags_AutoFit, ImPlotAxisFlags_AutoFit);

					ImPlot::PlotScatterG("##pitchScatter", scatterLambda, pd->pitchGraph, pd->pitchGraph->numPoints);
					ImPlot::PlotLineG("##pitchLine", lambda, pd->pitchGraph, std::max((pd->pitchGraph->numPoints) * 10, 1));
					ImPlot::EndPlot();
				}

				if (pd->volumeGraph && pd->volumeGraph->numPoints != 0 && ImPlot::BeginPlot(std::format("Volume##Action{}", i).c_str())) {
					ImPlot::SetupAxes(pd->volumeGraph->baseControllerNameOffset != UINT32_MAX ? audioBank->GetString(pd->volumeGraph->baseControllerNameOffset) : "n/a", "Volume (dB)", ImPlotAxisFlags_AutoFit, ImPlotAxisFlags_AutoFit);

					ImPlot::PlotScatterG("##volScatter", scatterLambda, pd->volumeGraph, pd->volumeGraph->numPoints);
					ImPlot::PlotLineG("##volLine", lambda, pd->volumeGraph, std::max((pd->volumeGraph->numPoints) * 10, 1), {});
					ImPlot::EndPlot();
				}

				ImPlot::EndSubplots();
			}
			ImPlot::PopStyleVar();

		}
		i++;
	}


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
	CMilesAudioBank* audioBank = asset->GetContainerFile<CMilesAudioBank>();

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

	for (auto& [action, previewData] : event->actions)
	{
		if (!shouldWrite && types.contains(action->actionType))
			shouldWrite = true;

		MilesEvent_WriteActionToRSONStream(rson, audioAsset, it);
		MilesEvent_WriteActionToRSONStream(rson, audioAsset, action);
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