#include <pch.h>
#include <core/fonts/sourcesans.h>
#include <thirdparty/imgui/misc/imgui_utility.h>

#include <game/rtech/cpakfile.h>
#include <game/rtech/utils\utils.h>

extern ExportSettings_t g_ExportSettings;

static void* AssetSettings_ReadOpen(ImGuiContext* const ctx, ImGuiSettingsHandler* const handler, const char* const name)
{
    UNUSED(handler);
    UNUSED(ctx);

    for (auto& it : g_assetData.m_assetTypeBindings)
    {
        std::string asset = fourCCToString(it.first);
        if (strcmp(asset.c_str(), name) == NULL)
        {
            return &it.second.e.exportSetting;
        }
    }

    return nullptr;
}

static void AssetSettings_ReadLine(ImGuiContext* const ctx, ImGuiSettingsHandler* const handler, void* const entry, const char* const line)
{
    UNUSED(handler);
    UNUSED(ctx);

    if (entry)
    {
        int* const exportSetting = static_cast<int*>(entry);
        if (int i; sscanf_s(line, "Setting=%u", &i) == 1)
        {
            *exportSetting = i;
        }
    }
}

static void AssetSettings_WriteAll(ImGuiContext* const ctx, ImGuiSettingsHandler* const handler, ImGuiTextBuffer* const buf)
{
    UNUSED(ctx);

    buf->reserve(buf->size() + 50);
    for (auto& it : g_assetData.m_assetTypeBindings)
    {
        buf->appendf("[%s][%s]\n", handler->TypeName, fourCCToString(it.first).c_str());
        buf->appendf("Setting=%d\n", it.second.e.exportSetting);
        buf->append("\n");
    }
}

static void* UtilSettings_ReadOpen(ImGuiContext* const ctx, ImGuiSettingsHandler* const handler, const char* const name)
{
    UNUSED(handler);
    UNUSED(ctx);
    UNUSED(name);

    return UtilsConfig;
}

static void UtilSettings_ReadLine(ImGuiContext* const ctx, ImGuiSettingsHandler* const handler, void* const entry, const char* const line)
{
    UNUSED(handler);
    UNUSED(ctx);

    if (entry)
    {
        ImGuiHandler::UtilsSettings_t* const cfg = static_cast<ImGuiHandler::UtilsSettings_t*>(entry);

        uint32_t i;
        if (sscanf_s(line, "ExportThreads=%u", &i) == 1)
        {
            cfg->exportThreadCount = i;
        }

        if (sscanf_s(line, "ParseThreads=%u", &i) == 1)
        {
            cfg->parseThreadCount = i;
        }
    }
}

static void UtilSettings_WriteAll(ImGuiContext* const ctx, ImGuiSettingsHandler* handler, ImGuiTextBuffer* const buf)
{
    UNUSED(ctx);

    buf->reserve(buf->size() + 50);
    buf->appendf("[%s][utils]\n", handler->TypeName );
    buf->appendf("ExportThreads=%u\n", UtilsConfig->exportThreadCount);
    buf->appendf("ParseThreads=%u\n", UtilsConfig->parseThreadCount);
    buf->append("\n");
}

static void* ExportSettings_ReadOpen(ImGuiContext* const ctx, ImGuiSettingsHandler* const handler, const char* const name)
{
    UNUSED(handler);
    UNUSED(ctx);
    UNUSED(name);

    return &g_ExportSettings;
}

#define ImGuiReadSetting(str, var, a)  if (sscanf_s(line, str, &a) == 1) { var = i; }
static void ExportSettings_ReadLine(ImGuiContext* const ctx, ImGuiSettingsHandler* const handler, void* const entry, const char* const line)
{
    UNUSED(handler);
    UNUSED(ctx);

    if (entry)
    {
        ExportSettings_t* const settings = static_cast<ExportSettings_t*>(entry);

        int i;
        ImGuiReadSetting("ExportPathsFull=%i",              settings->exportPathsFull, i);
        ImGuiReadSetting("ExportAssetDeps=%i",              settings->exportAssetDeps, i);
        ImGuiReadSetting("ExportSeqsWithRig=%i",            settings->exportSeqsWithRig, i);
        ImGuiReadSetting("ExportTxtrWithMat=%i",            settings->exportTxtrWithMat, i);
        ImGuiReadSetting("UseSemanticTextureNames=%i",      settings->useSemanticTextureNames, i);

        ImGuiReadSetting("ExportNormalRecalcSetting=%i",    settings->exportNormalRecalcSetting, i);
    }
}

static void ExportSettings_WriteAll(ImGuiContext* const ctx, ImGuiSettingsHandler* handler, ImGuiTextBuffer* const buf)
{
    UNUSED(ctx);

    buf->reserve(buf->size() + 256);
    buf->appendf("[%s][general]\n", handler->TypeName);
    
    buf->appendf("ExportPathsFull=%i\n",            g_ExportSettings.exportPathsFull);
    buf->appendf("ExportAssetDeps=%i\n",            g_ExportSettings.exportAssetDeps);
    buf->appendf("ExportSeqsWithRig=%i\n",          g_ExportSettings.exportSeqsWithRig);
    buf->appendf("ExportTxtrWithMat=%i\n",          g_ExportSettings.exportTxtrWithMat);
    buf->appendf("UseSemanticTextureNames=%i\n",    g_ExportSettings.useSemanticTextureNames);

    buf->appendf("ExportNormalRecalcSetting=%i\n",  g_ExportSettings.exportNormalRecalcSetting);

    buf->appendf("\n");
}

void ImGuiHandler::SetupHandler()
{
    ImGuiIO& io = ImGui::GetIO();
    io.IniFilename = "imgui.ini";

    ImGuiSettingsHandler assetSettingsHandler = {};
    assetSettingsHandler.TypeName = "AssetSettings";
    assetSettingsHandler.TypeHash = ImHashStr("AssetSettings");
    assetSettingsHandler.ReadOpenFn = AssetSettings_ReadOpen;
    assetSettingsHandler.ReadLineFn = AssetSettings_ReadLine;
    assetSettingsHandler.WriteAllFn = AssetSettings_WriteAll;

    ImGuiSettingsHandler utilSettingsHandler = {};
    utilSettingsHandler.TypeName = "UtilSettings";
    utilSettingsHandler.TypeHash = ImHashStr("UtilSettings");
    utilSettingsHandler.ReadOpenFn = UtilSettings_ReadOpen;
    utilSettingsHandler.ReadLineFn = UtilSettings_ReadLine;
    utilSettingsHandler.WriteAllFn = UtilSettings_WriteAll;

    ImGuiSettingsHandler exportSettingsHandler = {};
    exportSettingsHandler.TypeName = "ExportSettings";
    exportSettingsHandler.TypeHash = ImHashStr("ExportSettings");
    exportSettingsHandler.ReadOpenFn = ExportSettings_ReadOpen;
    exportSettingsHandler.ReadLineFn = ExportSettings_ReadLine;
    exportSettingsHandler.WriteAllFn = ExportSettings_WriteAll;

    ImGui::AddSettingsHandler(&assetSettingsHandler);
    ImGui::AddSettingsHandler(&utilSettingsHandler);
    ImGui::AddSettingsHandler(&exportSettingsHandler);
    ImGui::LoadIniSettingsFromDisk(io.IniFilename);
}

void ImGuiHandler::SetStyle()
{
    ImGuiIO& io = ImGui::GetIO();

    this->defaultFont = io.Fonts->AddFontFromMemoryCompressedTTF(SourceSansProRegular_compressed_data, sizeof(SourceSansProRegular_compressed_data), 18.f);

    ImGuiStyle& style = ImGui::GetStyle();
    ImVec4* colors = style.Colors;

    colors[ImGuiCol_Text] = ImVec4(0.81f, 0.81f, 0.81f, 1.00f);
    colors[ImGuiCol_TextDisabled] = ImVec4(0.56f, 0.56f, 0.56f, 1.00f);
    colors[ImGuiCol_TextSelectedBg] = ImVec4(0.12f, 0.37f, 0.75f, 0.50f);
    colors[ImGuiCol_WindowBg] = ImVec4(0.13f, 0.15f, 0.18f, 1.00f);
    colors[ImGuiCol_ChildBg] = ImVec4(0.00f, 0.00f, 0.00f, 0.78f);
    colors[ImGuiCol_PopupBg] = ImVec4(0.11f, 0.13f, 0.17f, 1.00f);
    colors[ImGuiCol_Border] = ImVec4(0.61f, 0.61f, 0.61f, 0.50f);
    colors[ImGuiCol_BorderShadow] = ImVec4(0.04f, 0.04f, 0.04f, 0.00f);
    colors[ImGuiCol_FrameBg] = ImVec4(0.00f, 0.00f, 0.00f, 0.78f);
    colors[ImGuiCol_FrameBgHovered] = ImVec4(0.04f, 0.06f, 0.10f, 1.00f);
    colors[ImGuiCol_FrameBgActive] = ImVec4(0.04f, 0.07f, 0.12f, 1.00f);
    colors[ImGuiCol_TitleBg] = ImVec4(0.26f, 0.51f, 0.78f, 1.00f);
    colors[ImGuiCol_TitleBgActive] = ImVec4(0.26f, 0.51f, 0.78f, 1.00f);
    colors[ImGuiCol_MenuBarBg] = ImVec4(0.11f, 0.13f, 0.17f, 1.00f);
    colors[ImGuiCol_ScrollbarBg] = ImVec4(0.16f, 0.20f, 0.24f, 1.00f);
    colors[ImGuiCol_ScrollbarGrab] = ImVec4(0.23f, 0.36f, 0.51f, 1.00f);
    colors[ImGuiCol_ScrollbarGrabHovered] = ImVec4(0.30f, 0.46f, 0.65f, 1.00f);
    colors[ImGuiCol_ScrollbarGrabActive] = ImVec4(0.31f, 0.49f, 0.69f, 1.00f);
    colors[ImGuiCol_SliderGrab] = ImVec4(0.31f, 0.43f, 0.43f, 1.00f);
    colors[ImGuiCol_SliderGrabActive] = ImVec4(0.41f, 0.56f, 0.57f, 1.00f);
    colors[ImGuiCol_Button] = ImVec4(0.31f, 0.43f, 0.43f, 1.00f);
    colors[ImGuiCol_ButtonHovered] = ImVec4(0.38f, 0.52f, 0.53f, 1.00f);
    colors[ImGuiCol_ButtonActive] = ImVec4(0.41f, 0.56f, 0.57f, 1.00f);
    colors[ImGuiCol_Header] = ImVec4(0.31f, 0.43f, 0.43f, 1.00f);
    colors[ImGuiCol_HeaderHovered] = ImVec4(0.38f, 0.53f, 0.53f, 1.00f);
    colors[ImGuiCol_HeaderActive] = ImVec4(0.41f, 0.56f, 0.57f, 1.00f);
    colors[ImGuiCol_Separator] = ImVec4(0.53f, 0.53f, 0.57f, 0.50f);
    colors[ImGuiCol_ResizeGrip] = ImVec4(0.81f, 0.81f, 0.81f, 0.50f);
    colors[ImGuiCol_Tab] = ImVec4(0.31f, 0.43f, 0.43f, 1.00f);
    colors[ImGuiCol_TabHovered] = ImVec4(0.38f, 0.53f, 0.53f, 1.00f);
    colors[ImGuiCol_TabActive] = ImVec4(0.41f, 0.56f, 0.57f, 1.00f);
    colors[ImGuiCol_TableHeaderBg] = ImVec4(0.14f, 0.19f, 0.24f, 1.00f);
    colors[ImGuiCol_TableBorderStrong] = ImVec4(0.20f, 0.26f, 0.33f, 1.00f);
    colors[ImGuiCol_TableBorderLight] = ImVec4(0.22f, 0.29f, 0.37f, 1.00f);
    colors[ImGuiCol_TableRowBgAlt] = colors[ImGuiCol_TableRowBg];
    colors[ImGuiCol_PlotHistogram] = ImVec4(0.31f, 0.43f, 0.43f, 1.00f);

    style.WindowBorderSize = 1.0f;
    style.FrameBorderSize = 0.0f;
    style.ChildBorderSize = 0.0f;
    style.PopupBorderSize = 1.0f;
    style.TabBorderSize = 1.0f;

    style.WindowRounding = 4.0f;
    style.FrameRounding = 1.0f;
    style.ChildRounding = 1.0f;
    style.PopupRounding = 3.0f;
    style.TabRounding = 1.0f;
    style.ScrollbarRounding = 3.0f;
}

void ImGuiHandler::HelpMarker(const char* const desc)
{
    assert(desc);
    ImGui::TextDisabled("(?)");
    if (ImGui::IsItemHovered(ImGuiHoveredFlags_DelayShort) && ImGui::BeginTooltip())
    {
        ImGui::PushTextWrapPos(ImGui::GetFontSize() * 35.0f);
        ImGui::TextUnformatted(desc);
        ImGui::PopTextWrapPos();
        ImGui::EndTooltip();
    }
}

const ProgressBarEvent_t* const ImGuiHandler::AddProgressBarEvent(const char* const eventName, const uint32_t eventNum, std::atomic<uint32_t>* const remainingEvents, const bool isInverted, const bool hasCloseButton)
{
    std::unique_lock<std::mutex> lock(eventMutex);

    if (eventNum != 0 && !pbAvailSlots.empty())
    {
        const uint8_t idx = pbAvailSlots.top();
        pbAvailSlots.pop();

        ProgressBarEvent_t* const event = &pbEvents[idx];
        event->isInverted = isInverted;
        event->eventName = eventName;
        event->eventNum = eventNum;
        event->remainingEvents = remainingEvents;
        event->eventClass = nullptr;
        event->fnRemainingEvents = nullptr;
        event->hasCloseButton = hasCloseButton;

        event->slotIsUsed = true;
        return event;
    }
    else
    {
        return nullptr;
    }

    unreachable();
}

void ImGuiHandler::FinishProgressBarEvent(const ProgressBarEvent_t* const event)
{
    if (!event)
        return;

    assert(event->slotIsUsed);
    std::unique_lock<std::mutex> lock(eventMutex);
    const uint8_t eventIdx = static_cast<uint8_t>(event - pbEvents);
    assert(eventIdx >= 0 || eventIdx < PB_SIZE);

    // already resets slotIsUsed to false
    memset(&pbEvents[eventIdx], 0, sizeof(ProgressBarEvent_t));
    pbAvailSlots.push(eventIdx);
}

void ImGuiHandler::HandleProgressBar()
{
    std::unique_lock<std::mutex> lock(eventMutex);

    bool foundTopLevelBar = false;
    for (int i = 0; i < PB_SIZE; ++i)
    {
        ProgressBarEvent_t* const event = &pbEvents[i];
        if (!event->slotIsUsed)
            continue;

        bool hasCloseButton = event->hasCloseButton;
        bool isOpen = true;

        ImVec2 winPos = ImGui::GetCursorPos();
        const ImVec2 regAvail = ImGui::GetContentRegionAvail();
        const ImVec2 winSize = ImGui::GetWindowSize();

        constexpr ImVec2 eventWinSize = { 500, 84 };
        winPos.x += ((ImGui::GetContentRegionAvail().x + eventWinSize.x) * 0.5f) + (winSize.x - regAvail.x) * (i * 0.5f);
        winPos.y += ((ImGui::GetContentRegionAvail().y + eventWinSize.y) * 0.5f) + (winSize.y - regAvail.y) * (i * 0.5f);

        if (!foundTopLevelBar)
        {
            ImGui::SetNextWindowSize(ImVec2{ 0, 0 });
            ImGui::SetNextWindowPos(winPos, ImGuiCond_Appearing);
            ImGui::SetNextWindowFocus(); // Set always on top

            if (!ImGui::Begin(event->eventName, hasCloseButton ? &isOpen : nullptr, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoNav | ImGuiWindowFlags_NoNavInputs | ImGuiWindowFlags_NoNavFocus | ImGuiWindowFlags_NoScrollWithMouse | ImGuiWindowFlags_NoCollapse))
                continue;
        }
        else
        {
            ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 2.f);
            ImGui::Separator();
            ImGui::TextUnformatted(event->eventName);
        }

        const uint32_t numEvents = event->eventNum;
        const uint32_t remainingEvents = event->getRemainingEvents();

        const uint32_t leftOverEvents = event->isInverted ? remainingEvents : numEvents - remainingEvents;
        const float progressFraction = std::clamp(static_cast<float>(leftOverEvents) / static_cast<float>(numEvents), 0.0f, 1.0f);
        std::stringstream progressText;
        progressText << leftOverEvents << "/" << numEvents;
        ImGui::ProgressBar(progressFraction, ImVec2(485, 48), progressText.str().c_str());

        // Handle close button click
        if (hasCloseButton && !isOpen)
        {
            // Signal the associated task to stop
            if (event->eventClass)
            {
                CParallelTask* parallelTask = reinterpret_cast<CParallelTask*>(event->eventClass);
                parallelTask->requestStop();
            }
        }

        foundTopLevelBar = true;
    }

    if (foundTopLevelBar)
        ImGui::End();
}

ImGuiHandler::ImGuiHandler()
{
    const uint32_t totalThreadCount = CThread::GetConCurrentThreads();

    // need at least one thread.
    cfg.exportThreadCount = 1u;
    cfg.parseThreadCount = std::max(totalThreadCount >> 1u, 1u);

    memset(pbEvents, 0, sizeof(pbEvents));
    for (int8_t i = PB_SIZE - 1; i >= 0; --i) // in reverse order
    {
        pbAvailSlots.push(static_cast<int8_t>(i));
    }
}

ImGuiHandler* g_pImGuiHandler = new ImGuiHandler();