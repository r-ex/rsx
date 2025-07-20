#include <pch.h>
#include <core/fonts/sourcesans.h>
#include <thirdparty/imgui/misc/imgui_utility.h>

#include <game/rtech/cpakfile.h>
#include <game/rtech/utils\utils.h>

#define ImGuiReadSetting(str, var, a)  if (sscanf_s(line, str, &a) == 1) { var = a; }

extern ExportSettings_t g_ExportSettings;
extern PreviewSettings_t g_PreviewSettings;

// asset settings
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

        int i;
        ImGuiReadSetting("Setting=%u", *exportSetting, i);
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

// utils settings
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
        ImGuiReadSetting("ExportThreads=%u", cfg->exportThreadCount, i);
        ImGuiReadSetting("ParseThreads=%u", cfg->parseThreadCount, i);
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

// export settings
static void* ExportSettings_ReadOpen(ImGuiContext* const ctx, ImGuiSettingsHandler* const handler, const char* const name)
{
    UNUSED(handler);
    UNUSED(ctx);
    UNUSED(name);

    return &g_ExportSettings;
}

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
        ImGuiReadSetting("ExportRigSequences=%i",           settings->exportRigSequences, i);
        ImGuiReadSetting("ExportModelSkin=%i",              settings->exportModelSkin, i);
        ImGuiReadSetting("ExportMaterialTextures=%i",       settings->exportMaterialTextures, i);

        ImGuiReadSetting("ExportTextureNameSetting=%i",     settings->exportTextureNameSetting, i);
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
    buf->appendf("ExportRigSequences=%i\n",         g_ExportSettings.exportRigSequences);
    buf->appendf("ExportModelSkin=%i\n",            g_ExportSettings.exportModelSkin);
    buf->appendf("ExportMaterialTextures=%i\n",     g_ExportSettings.exportMaterialTextures);

    buf->appendf("ExportTextureNameSetting=%i\n",   g_ExportSettings.exportTextureNameSetting);
    buf->appendf("ExportNormalRecalcSetting=%i\n",  g_ExportSettings.exportNormalRecalcSetting);

    // [rika]: there is no reason the other settings could not be saved in the future, it just seemed unneeded to save them for now.

    buf->appendf("\n");
}

// preview settings
static void* PreviewSettings_ReadOpen(ImGuiContext* const ctx, ImGuiSettingsHandler* const handler, const char* const name)
{
    UNUSED(handler);
    UNUSED(ctx);
    UNUSED(name);

    return &g_PreviewSettings;
}

static void PreviewSettings_ReadLine(ImGuiContext* const ctx, ImGuiSettingsHandler* const handler, void* const entry, const char* const line)
{
    UNUSED(handler);
    UNUSED(ctx);

    if (entry)
    {
        PreviewSettings_t* const settings = static_cast<PreviewSettings_t*>(entry);

        float i;
        ImGuiReadSetting("PreviewCullDistance=%f",  settings->previewCullDistance, i);
        ImGuiReadSetting("PreviewMovementSpeed=%f", settings->previewMovementSpeed, i);;
    }
}

static void PreviewSettings_WriteAll(ImGuiContext* const ctx, ImGuiSettingsHandler* handler, ImGuiTextBuffer* const buf)
{
    UNUSED(ctx);

    buf->reserve(buf->size() + 128);
    buf->appendf("[%s][general]\n", handler->TypeName);
    
    buf->appendf("PreviewCullDistance=%f\n",    g_PreviewSettings.previewCullDistance);
    buf->appendf("PreviewMovementSpeed=%f\n",   g_PreviewSettings.previewMovementSpeed);

    buf->appendf("\n");
}

// custom imgui widgets

// size_arg (for each axis) < 0.0f: align to end, 0.0f: auto, > 0.0f: specified size
void ImGui::ProgressBarCentered(float fraction, const ImVec2& size_arg, const char* overlay)
{
    ImGuiWindow* window = GetCurrentWindow();
    if (window->SkipItems)
        return;

    ImGuiContext& g = *GImGui;
    const ImGuiStyle& style = g.Style;

    ImVec2 pos = window->DC.CursorPos;
    ImVec2 size = CalcItemSize(size_arg, CalcItemWidth(), g.FontSize + style.FramePadding.y * 2.0f);
    ImRect bb(pos, pos + size);
    ItemSize(size, style.FramePadding.y);
    if (!ItemAdd(bb, 0))
        return;

    // Render
    fraction = ImSaturate(fraction);
    RenderFrame(bb.Min, bb.Max, GetColorU32(ImGuiCol_FrameBg), true, style.FrameRounding);
    bb.Expand(ImVec2(-style.FrameBorderSize, -style.FrameBorderSize));
    const ImVec2 fill_br = ImVec2(ImLerp(bb.Min.x, bb.Max.x, fraction), bb.Max.y);
    RenderRectFilledRangeH(window->DrawList, bb, GetColorU32(ImGuiCol_PlotHistogram), 0.0f, fraction, style.FrameRounding);

    // Default displaying the fraction as percentage string, but user can override it
    char overlay_buf[32];
    if (!overlay)
    {
        ImFormatString(overlay_buf, IM_ARRAYSIZE(overlay_buf), "%.0f%%", fraction * 100 + 0.01f);
        overlay = overlay_buf;
    }

    ImVec2 overlay_size = CalcTextSize(overlay, NULL);
    if (overlay_size.x > 0.0f)
        RenderTextClipped(ImVec2(bb.Min.x, bb.Min.y), bb.Max, overlay, NULL, &overlay_size, ImVec2(0.5f, 0.5f), &bb);
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

    // [rika]: go to for adding simple saved settings
    // related to exporting assets (not auto generated format settings (AssetSettings))
    ImGuiSettingsHandler exportSettingsHandler = {};
    exportSettingsHandler.TypeName = "ExportSettings";
    exportSettingsHandler.TypeHash = ImHashStr("ExportSettings");
    exportSettingsHandler.ReadOpenFn = ExportSettings_ReadOpen;
    exportSettingsHandler.ReadLineFn = ExportSettings_ReadLine;
    exportSettingsHandler.WriteAllFn = ExportSettings_WriteAll;

    // related to preview (in the 3D viewport)
    ImGuiSettingsHandler previewSettingsHandler = {};
    previewSettingsHandler.TypeName = "PreviewSettings";
    previewSettingsHandler.TypeHash = ImHashStr("PreviewSettings");
    previewSettingsHandler.ReadOpenFn = PreviewSettings_ReadOpen;
    previewSettingsHandler.ReadLineFn = PreviewSettings_ReadLine;
    previewSettingsHandler.WriteAllFn = PreviewSettings_WriteAll;

    ImGui::AddSettingsHandler(&assetSettingsHandler);
    ImGui::AddSettingsHandler(&utilSettingsHandler);
    ImGui::AddSettingsHandler(&exportSettingsHandler);
    ImGui::AddSettingsHandler(&previewSettingsHandler);
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

const ProgressBarEvent_t* const ImGuiHandler::AddProgressBarEvent(const char* const eventName, const uint32_t eventNum, std::atomic<uint32_t>* const remainingEvents, const bool isInverted)
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
        const ProgressBarEvent_t* const event = &pbEvents[i];
        if (!event->slotIsUsed)
            continue;

        if (!foundTopLevelBar)
        {
            ImVec2 viewportCenter = ImGui::GetMainViewport()->GetWorkCenter();

            ImGui::SetNextWindowSize(ImVec2(0, 0));
            ImGui::SetNextWindowPos(viewportCenter, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));

            if (!ImGui::Begin(event->eventName, nullptr, ImGuiWindowFlags_NoDocking | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoNav | ImGuiWindowFlags_NoScrollWithMouse))
                continue;
        }
        else
        {
            ImGui::Separator();
            ImGui::Text(event->eventName);
        }

        const uint32_t numEvents = event->eventNum;
        const uint32_t remainingEvents = event->getRemainingEvents();

        const uint32_t leftOverEvents = event->isInverted ? remainingEvents : numEvents - remainingEvents;
        const float progressFraction = std::clamp(static_cast<float>(leftOverEvents) / static_cast<float>(numEvents), 0.0f, 1.0f);
        ImGui::ProgressBarCentered(progressFraction, ImVec2(485, 48), std::format("{}/{}", leftOverEvents, numEvents).c_str());

        foundTopLevelBar = true;
    }

    if(foundTopLevelBar)
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