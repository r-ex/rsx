// This file is made available as part of the reSource Xtractor (RSX) asset extractor
// Licensed under AGPLv3. Details available at https://github.com/r-ex/rsx/blob/main/LICENSE

#include "pch.h"

#include <game/asset.h>
#include <misc/imgui_utility.h>
#include "rtech/utils/utils.h"

#ifndef BUILD_NOGUI
#include <misc/ImGuiNotify.hpp>
#endif
#include <core/bridge/bridge.h>

void CGlobalAssetData::ProcessAssetsPostLoad()
{
    this->m_donePostLoad = false;

    struct TypeRange_t
    {
        uint32_t type;
        size_t start;
        size_t end;
    };

    // find if type is in custom order.
    auto isInCustomOrder = [](const uint32_t type) -> bool
        {
            return std::ranges::find(s_postLoadOrderOverrides, type) != s_postLoadOrderOverrides.end();
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
            if (!it->second._loadAssetType)
                continue;

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
                        pAssetLookup->m_asset->SetPostLoadStatus(true);
                    }

                }, threadCount);

            std::string eventName = std::format("Processing Assets Prioritized Post Load... ({})", fourCCToString(it->first)).c_str();
            const ProgressBarEvent_t* const processingAssetsEvent = g_pImGuiHandler->AddProgressBarEvent(eventName.c_str(), static_cast<uint32_t>(range.end + 1), &assetIdx, true);
            parallelTask.execute();
            parallelTask.wait();
            g_pImGuiHandler->FinishProgressBarEvent(processingAssetsEvent);
        }
    }

    const uint32_t leftOverAssets = static_cast<uint32_t>(v_assets.size());
    assetIdx = typeRanges.empty() ? 0u : static_cast<uint32_t>(typeRanges.back().end); // last asset we processed after custom order.

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
                        if (!it->second._loadAssetType)
                            continue;
                        
                        it->second.postLoadFunc(pAssetLookup->m_asset->GetContainerFile<CAssetContainer>(), pAssetLookup->m_asset);
                    }
                    pAssetLookup->m_asset->SetPostLoadStatus(true);
                }
            }, threadCount);

        const ProgressBarEvent_t* const processingAssetsEvent = g_pImGuiHandler->AddProgressBarEvent("Processing Assets Post Load...", leftOverAssets, &assetIdx, true);
        parallelTask.execute();
        parallelTask.wait();
        g_pImGuiHandler->FinishProgressBarEvent(processingAssetsEvent);

        this->m_donePostLoad = true; // Record that we've finished post-load so that ODL paks can handle their own post-loading later on

        auto& postLoadFinishCallbacks = g_assetData.m_postLoadFinishCallbacks;
        std::unordered_map<void(*)(), bool>::iterator it = postLoadFinishCallbacks.begin();
        while (it != postLoadFinishCallbacks.end())
        {
            it->first();

            // If the callback is marked as single use, erase it after call
            if (it->second)
                it = postLoadFinishCallbacks.erase(it);
            else it++;
        }
    }
}

// Logging functions
void CGlobalAssetData::Log_Info(const CAssetContainer* const container, const char* fmt, ...)
{
    va_list args;
    va_start(args, fmt);

    std::string msg;
    GET_LOG_MSG_VARIADIC(args, msg, fmt);

    const std::string sourceName = container ? " [" + container->GetFilePath().filename().string() + "]: " : "";

    Log("%s%s\n", sourceName.c_str(), msg.c_str());
    LogMessages_Append(ContainerMessage_t::MessageType_e::MSG_INFO, sourceName, msg);

#ifndef BUILD_NOGUI
    ImGui::InsertNotification({ ImGuiToastType::Info, 3000, "%s", msg.c_str() });
#endif
}

void CGlobalAssetData::Log_Warning(const CAssetContainer* const container, const char* fmt, ...)
{
    va_list args;

    va_start(args, fmt);

    std::string msg;
    GET_LOG_MSG_VARIADIC(args, msg, fmt);

    const std::string sourceName = container ? " [" + container->GetFilePath().filename().string() + "]" : "";

    Log("WARNING%s: %s\n", sourceName.c_str(), msg.c_str());

    m_logErrorListInfo.AddWarning();

    LogMessages_Append(ContainerMessage_t::MessageType_e::MSG_WARNING, sourceName, msg);

#ifndef BUILD_NOGUI
    ImGui::InsertNotification({ ImGuiToastType::Warning, 5000, "%s", msg.c_str() });
#endif
}

void CGlobalAssetData::Log_Error(const CAssetContainer* const container, const char* fmt, ...)
{
    va_list args;
    va_start(args, fmt);

    std::string msg;
    GET_LOG_MSG_VARIADIC(args, msg, fmt);

    const std::string sourceName = container ? " [" + container->GetFilePath().filename().string() + "]" : "";

    Log("ERROR%s: %s\n", sourceName.c_str(), msg.c_str());

    m_logErrorListInfo.AddError();

    LogMessages_Append(ContainerMessage_t::MessageType_e::MSG_ERROR, sourceName, msg);

#ifndef BUILD_NOGUI
    ImGui::InsertNotification({ ImGuiToastType::Error, 5000, "%s", msg.c_str() });
#endif
}

void CGlobalAssetData::LogMessages_Append(ContainerMessage_t::MessageType_e type, const std::string& sourceName, const std::string& msg)
{
    const time_t t = std::time(nullptr);
    tm tm;
    if (localtime_s(&tm, &t))
    {
        assertm(0, "failed to get time");
        return;
    }

    std::ostringstream oss;
    oss << std::put_time(&tm, "%H:%M:%S");

    std::lock_guard lock(m_logMutex);

    ContainerMessage_t* const newMessages = reinterpret_cast<ContainerMessage_t*>(realloc(m_logMessages, sizeof(ContainerMessage_t) * (m_numLogMessages + 1)));

    if (!newMessages)
        return;

    m_logMessages = newMessages;

    ContainerMessage_t* m = &m_logMessages[m_numLogMessages];
    m->type = type;
    m->timestampStr = _strdup(oss.str().c_str());
    m->message = _strdup(msg.c_str());
    m->sourceName = _strdup(sourceName.c_str());

    g_bridgeData.PostNotification(sourceName, msg);

    m_numLogMessages++;
}


CGlobalAssetData g_assetData;