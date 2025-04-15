#include "pch.h"

#include <game/asset.h>
#include <misc/imgui_utility.h>
#include "rtech/utils/utils.h"

static std::vector<uint32_t> postLoadOrder =
{
    'rtxt', // Texture first.
    'gmiu', // UI Atlas after.

    'rdhs', // Shader hdr first.
    'sdhs', // Shader set after.
    'ltam', // Material after.

    // [rika]: aseq after arig/model that way the skeleton is set before parsing
    'gira', // Arig first
    '_ldm', // Model after
    'qesa', // Aseq last

};

void CGlobalAssetData::ProcessAssetsPostLoad()
{
    struct TypeRange_t
    {
        uint32_t type;
        size_t start;
        size_t end;
    };

    // find if type is in custom order.
    auto isInCustomOrder = [](const uint32_t type) -> bool
        {
            return std::ranges::find(postLoadOrder, type) != postLoadOrder.end();
        };

    std::vector<TypeRange_t> typeRanges;
    size_t startIndex = 0;
    size_t currentIndex = 0;

    // we will get the ranges now for each prioritized asset.
    while (currentIndex < v_assets.size())
    {
        const uint32_t currentType = v_assets[currentIndex].m_asset->GetAssetType();
        if (!isInCustomOrder(currentType))
        {
            ++currentIndex;
            continue;
        }

        // Count range.
        while (currentIndex < v_assets.size() && v_assets[currentIndex].m_asset->GetAssetType() == currentType)
        {
            ++currentIndex;
        }

        // Store the range for the current type in the vector using the struct.
        typeRanges.push_back({ currentType, startIndex, currentIndex - 1 });
        startIndex = currentIndex;
    }

    // we only want half of the available threads.
    const uint32_t threadCount = UtilsConfig->parseThreadCount;
    CParallelTask parallelTask(threadCount);

    std::atomic<uint32_t> assetIdx = 0;
    for (const auto& range : typeRanges)
    {
        // check if asset is registered and has post load function.
        if (auto it = m_assetTypeBindings.find(range.type); it != m_assetTypeBindings.end() && it->second.postLoadFunc)
        {
            // to the start of the current asset range.
            assetIdx = static_cast<uint32_t>(range.start);
            parallelTask.addTask([this, range, it, &assetIdx]
                {
                    // our asset count will be the range.end + 1 so we process the count properly.
                    const uint32_t assetCount = static_cast<uint32_t>(range.end + 1);
                    while (assetIdx < assetCount)
                    {
                        const uint32_t assetToProcess = assetIdx++;
                        if (assetToProcess >= assetCount)
                            continue;

                        AssetLookup_t* const pAssetLookup = &this->v_assets[assetToProcess];
                        // temp
                        it->second.postLoadFunc(pAssetLookup->m_asset->GetContainerFile<CAssetContainer>(), pAssetLookup->m_asset);
                    }

                }, threadCount);

            std::string eventName = std::format("Processing Assets Prioritized Post Load.. ({})", fourCCToString(it->first)).c_str();
            const ProgressBarEvent_t* const processingAssetsEvent = g_pImGuiHandler->AddProgressBarEvent(eventName.c_str(), static_cast<uint32_t>(range.end + 1), &assetIdx, true);
            parallelTask.execute();
            parallelTask.wait();
            g_pImGuiHandler->FinishProgressBarEvent(processingAssetsEvent);
        }
    }

    const uint32_t leftOverAssets = static_cast<uint32_t>(v_assets.size());
    assetIdx = typeRanges.empty() ? 0u : static_cast<uint32_t>(typeRanges.back().end); // last asset we processed after custom order.

    // we have to account for that .size() on vector starts from 1.
    if (typeRanges.empty() || leftOverAssets != (assetIdx + 1))
    {
        parallelTask.addTask([this, leftOverAssets, &assetIdx]
            {
                const uint32_t assetCount = leftOverAssets;
                while (assetIdx < assetCount)
                {
                    const uint32_t assetToProcess = assetIdx++;
                    if (assetToProcess >= assetCount)
                        continue;

                    AssetLookup_t* const pAssetLookup = &this->v_assets[assetToProcess];
                    if (auto it = m_assetTypeBindings.find(pAssetLookup->m_asset->GetAssetType()); it != m_assetTypeBindings.end() && it->second.postLoadFunc)
                    {
                        //it->second.postLoadFunc(pAssetLookup->m_asset->pak(), pAssetLookup->m_asset);
                        // temp
                        it->second.postLoadFunc(pAssetLookup->m_asset->GetContainerFile<CAssetContainer>(), pAssetLookup->m_asset);
                    }
                }
            }, threadCount);

        const ProgressBarEvent_t* const processingAssetsEvent = g_pImGuiHandler->AddProgressBarEvent("Processing Assets Post Load..", leftOverAssets, &assetIdx, true);
        parallelTask.execute();
        parallelTask.wait();
        g_pImGuiHandler->FinishProgressBarEvent(processingAssetsEvent);
    }

    //std::sort(m_pakAssets.begin(), m_pakAssets.end(), [](const CGlobalAssetData::AssetLookup_t& a, const CGlobalAssetData::AssetLookup_t& b) { return _stricmp(a.m_asset->name().c_str(), b.m_asset->name().c_str()); });
}

CGlobalAssetData g_assetData;