#include <pch.h>
#include "event.h"
#include <game/asset.h>
#include "miles.h"

#include <thirdparty/rad_lzb_simple/rad_lzb_simple.h>
#include <implot/implot.h>

static ImPlotPoint MilesGraphPlot_Scatter(int idx, void* data)
{
	MilesValueGraph_s* graph = reinterpret_cast<MilesValueGraph_s*>(data);

	const float startX = graph->XValues()[idx];

	const MilesGraphSegment_s* segment = &graph->Segments()[idx];

	return ImPlotPoint(startX, segment->start);
}

static ImPlotPoint MilesGraphPlot_Line(int idx, void* data)
{
	MilesValueGraph_s* graph = reinterpret_cast<MilesValueGraph_s*>(data);

	const float startX = graph->XValues()[0];
	const float endX = graph->XValues()[graph->numPoints - 1];

	const uint32_t totalSamples = std::max(((graph->numPoints) * 10) - 1, 1);

	const float thisX = std::lerp(startX, endX, (idx) / (float)totalSamples);

	int chosenPoint = graph->numPoints - 1;
	float minX = -1, maxX = -1;

	for (int i = 0; i < graph->numPoints; ++i)
	{
		maxX = graph->XValues()[i];

		// when we get the first point's X that is larger than this sample's X
		// get the index of the previous point since its segment will contain this sample
		if (maxX > thisX)
		{
			chosenPoint = i - 1;

			minX = graph->XValues()[chosenPoint];
			break;
		}
	}

	const MilesGraphSegment_s* segment = &graph->Segments()[chosenPoint];


	return ImPlotPoint(thisX, segment->Sample(thisX, minX, maxX));

}

bool MilesEvent_s::ParseActions()
{
	// v13 has no compression
	if (version == 13)
	{
		const char* cursor = reinterpret_cast<const char*>(originalData);

		while (true)
		{
			const EventActionBase_v13_s* base = reinterpret_cast<const EventActionBase_v13_s*>(cursor);
			const size_t actionSize = base->dataSizeDwords * sizeof(DWORD);

			char* actionData = new char[actionSize + 2]; // + 2 is to upgrade from v13 to new (generic) action header
			// The version can still be identified by the version field in both the audio bank and MilesEvent_s struct

			// This data offset is so that the actual data is in the right place in the buffer so that the
			// base header can be replaced with the generic one
			memcpy_s(actionData+2, actionSize, base, actionSize);

			// Upgrade v13 struct to generic
			EventActionBase_s* newBase = reinterpret_cast<EventActionBase_s*>(actionData);
			newBase->actionType = base->actionType;
			newBase->isLastAction = base->isLastAction;
			newBase->dataSizeDwords = base->dataSizeDwords;
			newBase->unk_1 = 0;
			newBase->pad = 0;

			this->actions.push_back({ newBase, nullptr });

			cursor += actionSize;

			if (base->isLastAction)
				break;
		}
	}
	else
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
	}

	parsedActions = true;

	return true;
}

void ParsedSourceSelector::Construct(CMilesAudioBank* bank, SourceSelector_s* sel, char* base)
{
	type = sel->type;
	isList = sel->type != 0;
	weight = sel->weight;

	if (!isList)
	{
		if (sel->unk_4 != -1)
			name = bank->GetSourceName(static_cast<uint32_t>(sel->unk_4));
		else
			name = "(no source)";
	}
	else
	{
		for (uint16_t i = 0; i < sel->childCount; ++i)
		{
			SourceSelector_s* v16 = reinterpret_cast<SourceSelector_s*>(
				base + (sizeof(uint32_t) * sel->ChildOffsets()[i]));

			ParsedSourceSelector pss;
			pss.Construct(bank, v16, base);

			children.emplace_back(pss);
		}
	}
}

static ParsedSourceSelector __fastcall ParseRootSelector(CMilesAudioBank* bank, char* a3)
{
	SourceSelector_s* v8 = (SourceSelector_s*)a3;
	ParsedSourceSelector parsed;
	parsed.Construct(bank, v8, a3);

	return parsed;
}

void ParsedSourceSelector::DrawSelectorChances(const uint32_t siblingWeightTotal) const
{
	if (siblingWeightTotal == 0)
		return;

	ImGui::SameLine();
	ImGui::TextDisabled(std::format("({}%%)", (100.f * weight) / siblingWeightTotal).c_str());
}

void ParsedSourceSelector::Draw(const uint32_t siblingWeightTotal) const
{
	if (!isList)
	{
		ImGui::TreeNodeEx(name.c_str(), ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_NoTreePushOnOpen);
		DrawSelectorChances(siblingWeightTotal);
	}
	else
	{
		const bool isOpen = ImGui::TreeNodeEx(std::format("List (type {})", type).c_str(), ImGuiTreeNodeFlags_DefaultOpen);

		// always draw weights even if the node is closed
		DrawSelectorChances(siblingWeightTotal);

		if (!isOpen)
			return;

		// precalculate total of all children so that each child can know their worth >:)
		const uint32_t childWeightTotal = GetSelectorsWeightTotal(children);

		size_t childIdx = 0;
		for (const auto& child : children)
		{
			ImGui::PushID(static_cast<int>(childIdx++));
			child.Draw(childWeightTotal);
			ImGui::PopID();
		}

		ImGui::TreePop();
	}
}

void ParsedSourceState::Draw()
{
	if (ImGui::TreeNodeEx(name.c_str(), ImGuiTreeNodeFlags_DefaultOpen))
	{
		const uint32_t weightTotal = GetSelectorsWeightTotal(selectors);

		size_t selIdx = 0;
		for (const auto& sel : selectors)
		{
			ImGui::PushID(static_cast<int>(selIdx++));
			sel.Draw(weightTotal);
			ImGui::PopID();
		}

		ImGui::TreePop();
	}
}

std::vector<ParsedSourceState> EventAction_0_s::GetSourceStates(CMilesAudioBank* bank)
{
	PlayActionState* actionState = Offset<PlayActionState>(sourceStatesOffset * sizeof(uint32_t));

	std::vector<ParsedSourceState> states;
	for(uint32_t i = 0; i < unk_1; ++i)
	{
		ParsedSourceState state;

		if (actionState->wordE & 1)
			break;

		const short v32 = actionState->word12;

		if ((v32 & 0x20) != 0 || !actionState->dword44)
			break;

		if (actionState->unkCount_4)
		{
			state.name = bank->GetString(actionState->nameOffset);

			if ((v32 & 0x10) != 0)
				break;

			if (!actionState->dword40)
				break;
				
			int v35 = 0;
			struct_v1* v1 = Offset<struct_v1>(sizeof(DWORD) * (unkDwordOffset_78 + actionState->unsigned___int86));
			do
			{
				if (v1->unkOffset != -1)
				{
					char* v2 = Offset<char>(sizeof(DWORD) * (sourceSelectorOffset + v1->unkOffset));
					ParsedSourceSelector sel = ParseRootSelector(bank, v2);

					state.selectors.push_back(sel);
				}

				v35++;
				v1++;
			} while (v35 < actionState->unkCount_4);
		}

		states.push_back(state);

		const uint32_t baseStructSize = (actionState->wordE & 0x100) ? 104u : sizeof(PlayActionState);

		const size_t nextStateOffset = baseStructSize + (36ull * (actionState->unsigned___int88 + actionState->unsigned___int89));

		actionState = (PlayActionState*)((uintptr_t)actionState + nextStateOffset);
	}

	return states;
}

extern MilesEventActionParseResult_e ActionPlay_Parse(CMilesAudioAsset* asset, void* actionData, void** out_previewData);

void* PreviewAudioEventAsset(CAsset* const asset, const bool firstFrameForAsset)
{
	CMilesAudioAsset* audioAsset = reinterpret_cast<CMilesAudioAsset*>(asset);
	MilesEvent_s* event = reinterpret_cast<MilesEvent_s*>(audioAsset->GetAssetData());
	CMilesAudioBank* audioBank = asset->GetContainerFile<CMilesAudioBank>();

	if (firstFrameForAsset)
	{
		if (!event->parsedActions)
		{
			if (!event->ParseActions())
				printf("Failed to parse!\n");
		}

		for (auto& [action, previewData] : event->actions)
		{
			MilesEventActionParseResult_e res = MilesEventActionParseResult_e::RESULT_UNSUPPORTED;
			
			switch (action->actionType)
			{
			case 0:
			{
				res = ActionPlay_Parse(audioAsset, action, &previewData);

				break;
			}
			}

			if (previewData)
			{
				ActionPreviewData_s* pd = reinterpret_cast<ActionPreviewData_s*>(previewData);

				pd->parseResult = res;
			}
			else
			{
				ActionPreviewData_s* pd = new ActionPreviewData_s;

				pd->parseResult = res;

				previewData = pd;				
			}
		}
	}

	size_t i = 0;
	for (auto& [action, previewData] : event->actions)
	{
		ActionPreviewData_s* basePreviewData = reinterpret_cast<ActionPreviewData_s*>(previewData);

		const std::string title = std::format(
			"Action #{}{}", i + 1,
			s_eventPreviewNames.contains((EventActionType_e)action->actionType)
				? ": " + std::string(s_eventPreviewNames.at((EventActionType_e)action->actionType))
				: ": Type " + std::to_string(action->actionType)
		);

		if (firstFrameForAsset)
			ImGui::SetNextItemOpen(true, ImGuiCond_Always);

		if (!ImGui::CollapsingHeader(title.c_str(), ImGuiTreeNodeFlags_DefaultOpen))
		{
			i++;
			continue;
		}

		ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(15.f, 10.f));

		if(ImGui::BeginChild(title.c_str(), ImVec2(0, 0), ImGuiChildFlags_Borders | ImGuiChildFlags_AutoResizeY))
		{
			const ImVec2 avail = ImGui::GetContentRegionAvail();

			// If the action's preview data has not been parsed successfully, display the error message.
			// This will likely always be due to unsupported types or versions
			if (basePreviewData->parseResult != MilesEventActionParseResult_e::RESULT_SUCCESS)
			{
				ImGui::TextDisabled("Unable to preview: %s", s_parseResultMessages.at(basePreviewData->parseResult));
			}
			else
			{
				switch (action->actionType)
				{
				case 0:
				{
					ActionPreviewData_0_s* pd = reinterpret_cast<ActionPreviewData_0_s*>(previewData);

					ImGui::SeparatorText("Audio Sources");
					for (auto& state : pd->states)
					{
						state.Draw();
					}

					const bool hasAnyGraphs = (
						(pd->pitchGraph != nullptr && pd->pitchGraph->numPoints != 0) ||
						(pd->volumeGraph != nullptr && pd->volumeGraph->numPoints != 0)
						);

					if (hasAnyGraphs)
					{
						ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 5.f);
						ImGui::SeparatorText("Controller Graphs");
						ImGui::PushStyleColor(ImGuiCol_Text, ImGui::GetColorU32(ImGuiCol_TextDisabled));
						ImGui::TextWrapped("These graphs show how the event properties on the Y axis (e.g., pitch or volume) are changed based on the dynamic value of a controller");
						ImGui::PopStyleColor();

						ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 5.f);

						ImPlot::PushStyleVar(ImPlotStyleVar_FitPadding, ImVec2(0.1f, 0.1f));
						if (pd->pitchGraph && pd->pitchGraph->numPoints != 0 && ImPlot::BeginPlot(std::format("Pitch##Action{}", i).c_str(), ImVec2(avail.x / 2, avail.x / 2))) {
							ImPlot::SetupAxes(pd->pitchGraph->baseControllerNameOffset != UINT32_MAX ? audioBank->GetString(pd->pitchGraph->baseControllerNameOffset) : "n/a", "Pitch (st)", ImPlotAxisFlags_AutoFit, ImPlotAxisFlags_AutoFit);

							ImPlot::PlotScatterG("##pitchScatter", MilesGraphPlot_Scatter, pd->pitchGraph, pd->pitchGraph->numPoints);
							ImPlot::PlotLineG("##pitchLine", MilesGraphPlot_Line, pd->pitchGraph, std::max((pd->pitchGraph->numPoints) * 10, 1));
							ImPlot::EndPlot();
							ImGui::SameLine();
						}

						if (pd->volumeGraph && pd->volumeGraph->numPoints != 0 && ImPlot::BeginPlot(std::format("Volume##Action{}", i).c_str(), ImVec2(avail.x / 2, avail.x / 2))) {
							ImPlot::SetupAxes(pd->volumeGraph->baseControllerNameOffset != UINT32_MAX ? audioBank->GetString(pd->volumeGraph->baseControllerNameOffset) : "n/a", "Volume (dB)", ImPlotAxisFlags_AutoFit, ImPlotAxisFlags_AutoFit);

							ImPlot::PlotScatterG("##volScatter", MilesGraphPlot_Scatter, pd->volumeGraph, pd->volumeGraph->numPoints);
							ImPlot::PlotLineG("##volLine", MilesGraphPlot_Line, pd->volumeGraph, std::max((pd->volumeGraph->numPoints) * 10, 1), {});
							ImPlot::EndPlot();
						}
						ImPlot::PopStyleVar();
					}

					break;
				}
				case 8:
				{
					ImGui::PushStyleColor(ImGuiCol_Text, ImGui::GetColorU32(ImGuiCol_TextDisabled));
					ImGui::TextWrapped("This event action causes the following additional events to be played:");
					ImGui::PopStyleColor();
					EventAction_8_s* act = reinterpret_cast<EventAction_8_s*>(action);

					for (uint32_t j = 0; j < act->eventCount; ++j)
					{
						ImGui::BulletText(audioBank->GetString(act->eventNameOffset[j]));
					}

					break;
				}
				case 11:
				{
					EventAction_11_s* act = reinterpret_cast<EventAction_11_s*>(action);
					for (uint32_t j = 0; j < act->unk_1; ++j)
					{
						ImGui::Text("%s", audioBank->GetString(act->controllerNameOffset[j]));
					}
					break;
				}
				default:
				{
					ImGui::TextDisabled("No preview is available for this action type.");
					break;
				}
				}
			}
		}
		ImGui::EndChild();

		ImGui::PopStyleVar();


		i++;
	}

	return nullptr;
}

extern void MilesEvent_WriteActionToRSONStream(std::stringstream& rson, CMilesAudioAsset* asset, const EventActionBase_s* const action);

static bool ExportAudioEventAsset(CAsset* const asset, int type)
{
	UNUSED(type); UNUSED(asset);
	return false;
	//CMilesAudioAsset* audioAsset = reinterpret_cast<CMilesAudioAsset*>(asset);
	//MilesEvent_s* event = reinterpret_cast<MilesEvent_s*>(audioAsset->GetAssetData());

	//if (!event->parsedActions && !event->ParseActions())
	//{
	//	printf("Failed to parse!\n");
	//	return false;
	//}
	////CMilesAudioBank* audioBank = asset->GetContainerFile<CMilesAudioBank>();

	//// Create exported path + asset path.
	//std::filesystem::path exportPath = g_rsxSettings.GetExportDirectory();
	//const std::filesystem::path aevtPath(audioAsset->GetAssetName());

	//// truncate paths?
	//if (g_rsxSettings.exportPathsFull)
	//	exportPath.append(aevtPath.parent_path().string());
	//else
	//	exportPath.append("events");

	//if (!CreateDirectories(exportPath))
	//{
	//	assertm(false, "Failed to create asset type directory.");
	//	return false;
	//}

	//exportPath.append(aevtPath.filename().string() + ".rson");

	//std::stringstream rson;

	//rson
	//	<< "eventName: " << aevtPath.filename() << "\n"
	//	<< "actions:\n[\n";

	//const std::unordered_set<uint8_t> types = {  };
	//bool shouldWrite = false;

	//for (auto& [action, previewData] : event->actions)
	//{
	//	if (!shouldWrite && types.contains(action->actionType))
	//		shouldWrite = true;

	//	MilesEvent_WriteActionToRSONStream(rson, audioAsset, action);
	//}

	//rson << "]\n";

	//if (shouldWrite)
	//{
	//	StreamIO sio(exportPath, eStreamIOMode::Write);

	//	sio.write(rson.str().c_str(), rson.str().length());

	//	sio.close();

	//	//size_t i = 0;
	//	//for (auto& it : event->actions)
	//	//{
	//	//	if (!types.contains(it->actionType))
	//	//		continue;

	//	//	exportPath.replace_filename(std::format("{}_{}.{}.bin", aevtPath.filename().string(), i, (int)it->actionType));
	//	//	sio = StreamIO(exportPath, eStreamIOMode::Write);

	//	//	sio.write((char*)it, it->dataSizeDwords * 4);

	//	//	sio.close();

	//	//	i++;
	//	//}
	//}

	//return true;
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