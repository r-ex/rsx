#include <pch.h>
#include "event.h"
#include <game/asset.h>
#include "miles.h"

#include <thirdparty/rad_lzb_simple/rad_lzb_simple.h>

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

void* PreviewAudioEventAsset(CAsset* const asset, const bool firstFrameForAsset)
{
	CMilesAudioAsset* audioAsset = reinterpret_cast<CMilesAudioAsset*>(asset);
	MilesEvent_s* event = reinterpret_cast<MilesEvent_s*>(audioAsset->GetAssetData());
	CMilesAudioBank* audioBank = asset->GetContainerFile<CMilesAudioBank>();

	if (firstFrameForAsset && !event->parsedActions)
	{
		if (!event->ParseActions())
			printf("Failed to parse!\n");

		for (auto& action : event->actions)
		{
			printf("%c%u: %u bytes\n", action->isLastAction ? '*' : ' ', action->actionType, action->dataSizeDwords * 4);


			switch (action->actionType)
			{
			case 8:
			{
				EventAction_8_s* act = reinterpret_cast<EventAction_8_s*>(action);

				printf("\t[0] = %i\n", act->unk_4[0]);
				printf("\t[1] = %i\n", act->unk_4[1]);

				printf("\t[EV] = %s\n", audioBank->GetString(act->eventNameOffset));

				break;
			}
			}
		}
		printf("\n");
	}

	return nullptr;
}

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

	exportPath.append(aevtPath.filename().string());

	if (!CreateDirectories(exportPath))
	{
		assertm(false, "Failed to create asset type directory.");
		return false;
	}

	size_t i = 0;
	for (auto& it : event->actions)
	{
		std::filesystem::path actionPath = exportPath;
		actionPath.append(std::format("{}_{}.bin", i, (uint32_t)it->actionType));

		StreamIO sio(actionPath, eStreamIOMode::Write);

		sio.write(reinterpret_cast<const char*>(it), it->dataSizeDwords * 4);
		sio.close();
		i++;
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