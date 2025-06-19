#include <pch.h>

#include <thirdparty/imgui/misc/imgui_utility.h>

#include <core/filehandling/load.h>
#include <core/filehandling/export.h>

#include <game/rtech/assets/model.h>
#include <game/model/sourcemodel.h>

void HandleMDLLoad(std::vector<std::string> filePaths)
{
    std::vector<uint64_t> guids; // assets to load
    guids.reserve(filePaths.size());

    std::atomic<uint32_t> modelLoadingProgress = 0;
    const ProgressBarEvent_t* const modelLoadProgressBar = g_pImGuiHandler->AddProgressBarEvent("Loading Model Files..", static_cast<uint32_t>(filePaths.size()), &modelLoadingProgress, true);

    for (std::string& path : filePaths)
    {
        if (!std::filesystem::exists(path))
        {
            assertm(false, "model file did not exist");
            continue;
        }

        const size_t fileSize = std::filesystem::file_size(path);
        std::unique_ptr<char[]> fileBuf = std::make_unique<char[]>(fileSize);

        StreamIO fileIn(path, eStreamIOMode::Read);
        fileIn.R()->read(fileBuf.get(), fileSize);

        const studiohdr_short_t* const pStudioHdr = reinterpret_cast<const studiohdr_short_t* const>(fileBuf.get());

        if (pStudioHdr->id != MODEL_FILE_ID)
        {
            assertm(false, "invalid file");
            continue;
        }

        // only handle supported versions
        switch (pStudioHdr->version)
        {
        case 52: // r1
        case 53: // r2
        case 63: // r1x360
        {
            break;
        }
        case 54: // r5 (should be pak only)
        {
            Log("Studio version 54 is only supported through RPak export, skipping...\n");
            continue;
        }
        default:
        {
            Log("Studio version %i is not supported, skipping...\n", pStudioHdr->version);
            continue;
        }
        }

        CSourceModelAsset* srcMdlAsset = new CSourceModelAsset(pStudioHdr);
        CSourceModelSource* const srcMdlSource = static_cast<CSourceModelSource*>(srcMdlAsset->GetContainerFile<CAssetContainer>());

        srcMdlSource->SetFilePath(path);

        g_assetData.v_assets.emplace_back(srcMdlAsset->GetAssetGUID(), srcMdlAsset);
        g_assetData.v_assetContainers.emplace_back(srcMdlSource);
        guids.push_back(srcMdlAsset->GetAssetGUID());

        // parse sequences
        const std::filesystem::path srcMdlPath = (pStudioHdr->pszName());

        switch (pStudioHdr->version)
        {
        case 52:
        {
            break;
        }
        case 53:
        {
            // use the model we stored so we don't point to bad data
            r2::studiohdr_t* const pLocalHdr = reinterpret_cast<r2::studiohdr_t* const>(srcMdlAsset->GetAssetData());

            uint64_t* sequences = new uint64_t[pLocalHdr->numlocalseq];

            assertm(s_AssetTypePaths.contains(AssetType_t::SEQ), "somehow missing prefix");
            const char* const s_SeqPrefix = s_AssetTypePaths.find(AssetType_t::SEQ)->second;
            const std::string basePath(std::format("{}/{}/{}", s_SeqPrefix, srcMdlPath.parent_path().string(), srcMdlPath.stem().string()));

            for (int i = 0; i < pLocalHdr->numlocalseq; i++)
            {
                r2::mstudioseqdesc_t* const pSeqdesc = pLocalHdr->pSeqdesc(i);
                const std::string seqPath = std::format("{}/{}.seq", basePath, pSeqdesc->pszLabel());

                CSourceSequenceAsset* srcSeqAsset = new CSourceSequenceAsset(srcMdlAsset, pSeqdesc, seqPath);

                const uint64_t guid = srcSeqAsset->GetAssetGUID();
                g_assetData.v_assets.emplace_back(guid, srcSeqAsset);
                guids.push_back(guid);

                sequences[i] = guid;
            }

            srcMdlAsset->SetSequenceList(sequences, pLocalHdr->numlocalseq);

            break;
        }
        case 63:
        {
            break;
        }
        default:
        {
            assertm(false, "should not be hit");
            break;
        }
        }

        ++modelLoadingProgress;
    }

    g_pImGuiHandler->FinishProgressBarEvent(modelLoadProgressBar);

    for (const uint64_t& guid : guids)
    {
        CAsset* const asset = g_assetData.FindAssetByGUID(guid);

        assertm(asset->GetAssetContainerType() == CAsset::ContainerType::MDL, "invalid container");

        if (auto it = g_assetData.m_assetTypeBindings.find(asset->GetAssetType()); it != g_assetData.m_assetTypeBindings.end())
        {
            if (it->second.loadFunc)
                it->second.loadFunc(asset->GetContainerFile<CAssetContainer>(), asset);
        }
    }
}