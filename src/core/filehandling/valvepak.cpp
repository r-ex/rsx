#include <pch.h>

#include <thirdparty/imgui/misc/imgui_utility.h>

#include <core/filehandling/load.h>
#include <core/filehandling/export.h>

#include <game/vpk/vpk.h>

void HandleVPKLoad(std::vector<std::string> filePaths)
{
    std::atomic<uint32_t> pakfileLoadingProgress = 0;
    const ProgressBarEvent_t* const pakfileLoadProgressBar = g_pImGuiHandler->AddProgressBarEvent("Loading Valve Package Files..", static_cast<uint32_t>(filePaths.size()), &pakfileLoadingProgress, true);

    for (std::string& path : filePaths)
    {
        if (!std::filesystem::exists(path))
            return;

        CVPKPackage* pakfile = new CVPKPackage;

        pakfile->SetFilePath(path);

        if (!pakfile->ParseFromFile(path))
        {
            assertm(false, "failed to parse valve pakfile");

            delete pakfile;

            continue;
        }

        g_assetData.v_assetContainers.emplace_back(pakfile);

        ++pakfileLoadingProgress;
    }

    g_pImGuiHandler->FinishProgressBarEvent(pakfileLoadProgressBar);
}