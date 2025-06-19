#include <pch.h>

#include <thirdparty/imgui/misc/imgui_utility.h>

#include <core/filehandling/load.h>
#include <core/filehandling/export.h>

#include <game/bluepoint/bp_pakfile.h>

void HandleBPKLoad(std::vector<std::string> filePaths)
{
    std::atomic<uint32_t> pakfileLoadingProgress = 0;
    const ProgressBarEvent_t* const pakfileLoadProgressBar = g_pImGuiHandler->AddProgressBarEvent("Loading Bluepoint Pak Files..", static_cast<uint32_t>(filePaths.size()), &pakfileLoadingProgress, true);

    for (std::string& path : filePaths)
    {
        if (!std::filesystem::exists(path))
            return;

        CBluepointPakfile* pakfile = new CBluepointPakfile;

        pakfile->SetFilePath(path);
        pakfile->SetFileName(keepAfterLastSlashOrBackslash(path.c_str())); // this copies into a buffer

        if (!pakfile->ParseFromFile())
        {
            assertm(false, "failed to parse bluepoint pakfile");
            Log("%s failed to load!\n", pakfile->GetFileName());

            delete pakfile;

            continue;
        }

        g_assetData.v_assetContainers.emplace_back(pakfile);

        auto binding = g_assetData.m_assetTypeBindings.find('fwpb');
        if (binding == g_assetData.m_assetTypeBindings.end())
        {
            assertm(false, "no asset binding for bluepoint files");
            break;
        }

        switch (pakfile->Version())
        {
        case BP_PAK_VER_R1:
        {
            bpkfile_v6_t* const files = reinterpret_cast<bpkfile_v6_t* const>(pakfile->Files());
            for (int i = 0; i < pakfile->FileCount(); i++)
            {
                bpkfile_v6_t* const bpkfile = files + i;
                CBluepointWrappedFile* file = new CBluepointWrappedFile(pakfile, bpkfile);

                file->MakeGuid64FromGuid32(bpkfile->hash[0], bpkfile->hash[1]);

                // tempname
                const std::string tmp = std::format("{}/0x{:X}", "bpkfile", file->GetAssetGUID());
                file->SetAssetName(tmp);

                g_assetData.v_assets.emplace_back(file->GetAssetGUID(), file);

                binding->second.loadFunc(pakfile, file);
            }

            break;
        }
        default:
        {
            assertm(false, "invalid version somehow");
            break;
        }
        }

        ++pakfileLoadingProgress;
    }

    g_pImGuiHandler->FinishProgressBarEvent(pakfileLoadProgressBar);
}