#include "pch.h"

#include <thirdparty/imgui/misc/imgui_utility.h>

#include <core/filehandling/load.h>
#include <core/filehandling/export.h>

#include <game/audio/miles.h>

void HandleMBNKLoad(std::vector<std::string> filePaths)
{
	std::atomic<uint32_t> bankLoadingProgress = 0;
	const ProgressBarEvent_t* const bankLoadProgressBar = g_pImGuiHandler->AddProgressBarEvent("Loading Audio Banks..", static_cast<uint32_t>(filePaths.size()), &bankLoadingProgress, true);

	Log("Started MBNK load.\n");

	for (const std::string& path : filePaths)
	{
		if (CMilesAudioBank* const bank = new CMilesAudioBank(); bank->ParseFile(path))
		{
			g_assetData.v_assetContainers.emplace_back(bank);
		}
		else
		{
			Log("bank failed to load!\n");
			delete bank;
		}

		++bankLoadingProgress;
	}

	g_pImGuiHandler->FinishProgressBarEvent(bankLoadProgressBar);
}