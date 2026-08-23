#include <pch.h>
#include <core/fonts/sourcesans.h>
#include <core/fonts/codicons.h>
#include <thirdparty/imgui/misc/imgui_utility.h>
#include <thirdparty/imgui/misc/cpp/imgui_stdlib.h>

#include <game/rtech/cpakfile.h>
#include <game/rtech/utils\utils.h>

#define ImGuiReadSetting(str, var, a, type)  if (sscanf_s(line, str, &a) == 1) { var = static_cast<type>(a); }

extern RSXSettings_t g_rsxSettings;
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
            return &it.second;
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
        AssetTypeBinding_t* const typeBinding = static_cast<AssetTypeBinding_t*>(entry);

        int i;
        ImGuiReadSetting("Setting=%i", typeBinding->e.exportSetting, i, int);
        ImGuiReadSetting("LoadAssetType=%i", typeBinding->_loadAssetType, i, int);

        if (auto assetSettings = g_rsxSettings.assetSettings.find(typeBinding->type); assetSettings != g_rsxSettings.assetSettings.end())
        {
            for (auto& setting : assetSettings->second)
            {
                switch (setting.valueType)
                {
                case UISettingType_e::TYPE_BOOL:
                    ImGuiReadSetting(setting.cfgName, setting.rawValue.boolVal, i, int);
                    break;
                case UISettingType_e::TYPE_U32:
                    ImGuiReadSetting(setting.cfgName, setting.rawValue.u32Val, i, uint32_t);
                    break;
                case UISettingType_e::TYPE_FLOAT32:
                    ImGuiReadSetting(setting.cfgName, setting.rawValue.flVal, i, float);
                    break;

                }
            }
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
        buf->appendf("LoadAssetType=%d\n", it.second._loadAssetType);

        if (auto assetSettings = g_rsxSettings.assetSettings.find(it.first); assetSettings != g_rsxSettings.assetSettings.end())
        {
            for (auto& setting : assetSettings->second)
            {
                // this is bad, but the compiler won't throw a warning because of mismatched fmt vars to types since it's a runtime
                // format string
                buf->appendf(setting.cfgName, setting.rawValue.u32Val);
            }
        }

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
        ImGuiReadSetting("ExportThreads=%u", cfg->exportThreadCount, i, uint32_t);
        ImGuiReadSetting("ParseThreads=%u", cfg->parseThreadCount, i, uint32_t);
        ImGuiReadSetting("CompressionLevel=%u", cfg->compressionLevel, i, uint32_t);
        ImGuiReadSetting("CheckForUpdates=%u", cfg->checkForUpdates, i, int);
    }
}

static void UtilSettings_WriteAll(ImGuiContext* const ctx, ImGuiSettingsHandler* handler, ImGuiTextBuffer* const buf)
{
    UNUSED(ctx);

    buf->reserve(buf->size() + 64);
    buf->appendf("[%s][utils]\n", handler->TypeName );
    buf->appendf("ExportThreads=%u\n", UtilsConfig->exportThreadCount);
    buf->appendf("ParseThreads=%u\n", UtilsConfig->parseThreadCount);
    buf->appendf("CompressionLevel=%u\n", UtilsConfig->compressionLevel);
    buf->appendf("CheckForUpdates=%i\n", UtilsConfig->checkForUpdates ? 1 : 0);
    buf->append("\n");
}

// export settings
static void* ExportSettings_ReadOpen(ImGuiContext* const ctx, ImGuiSettingsHandler* const handler, const char* const name)
{
    UNUSED(handler);
    UNUSED(ctx);
    UNUSED(name);

    return &g_rsxSettings;
}

static void ExportSettings_ReadLine(ImGuiContext* const ctx, ImGuiSettingsHandler* const handler, void* const entry, const char* const line)
{
    UNUSED(handler);
    UNUSED(ctx);

    if (entry)
    {
        RSXSettings_t* const settings = static_cast<RSXSettings_t*>(entry);

        int i;
        ImGuiReadSetting("ExportPathsFull=%i",              settings->exportPathsFull, i, int);
        ImGuiReadSetting("ExportAssetDeps=%i",              settings->exportAssetDeps, i, int);
        ImGuiReadSetting("DisableCacheNames=%i",            settings->disableCachedNames, i, int);


        ImGuiReadSetting("ExportTextureNameSetting=%u",     settings->exportTextureNameSetting, i, uint32_t);
        ImGuiReadSetting("ExportNormalRecalcSetting=%u",    settings->exportNormalRecalcSetting, i, uint32_t);
        ImGuiReadSetting("ExportMaterialTextures=%i",       settings->exportMaterialTextures, i, int);

        ImGuiReadSetting("QCMajorVersion=%u",               settings->qcMajorVersion, i, uint16_t);
        ImGuiReadSetting("QCMinorVersion=%u",               settings->qcMinorVersion, i, uint16_t);

        ImGuiReadSetting("ExportRigSequences=%i",           settings->exportRigSequences, i, int);
        ImGuiReadSetting("ExportModelSkin=%i",              settings->exportModelSkin, i, int);
        ImGuiReadSetting("ExportTruncatedMaterials=%i",     settings->exportModelMatsTruncated, i, int);
        ImGuiReadSetting("ExportQCIFiles=%i",               settings->exportQCIFiles, i, int);

        ImGuiReadSetting("BridgePortNum=%i", settings->bridgePort, i, uint16_t);

        //ImGuiReadSetting("UseOrigScriptExportExtensions=%i", settings->useOrigScriptExportExtensions, i, int);
        settings->useOrigScriptExportExtensions = true; // cant decide if i wanna keep this setting or not so let's just force it to true for now
    }
}

static void ExportSettings_WriteAll(ImGuiContext* const ctx, ImGuiSettingsHandler* handler, ImGuiTextBuffer* const buf)
{
    UNUSED(ctx);

    buf->reserve(buf->size() + (48 * 12));
    buf->appendf("[%s][general]\n", handler->TypeName);
    
    buf->appendf("ExportPathsFull=%i\n",            g_rsxSettings.exportPathsFull);
    buf->appendf("ExportAssetDeps=%i\n",            g_rsxSettings.exportAssetDeps);
    buf->appendf("DisableCacheNames=%i\n",          g_rsxSettings.disableCachedNames);

    buf->appendf("ExportTextureNameSetting=%u\n",   g_rsxSettings.exportTextureNameSetting);
    buf->appendf("ExportNormalRecalcSetting=%u\n",  g_rsxSettings.exportNormalRecalcSetting);
    buf->appendf("ExportMaterialTextures=%i\n",     g_rsxSettings.exportMaterialTextures);

    buf->appendf("QCMajorVersion=%u\n",             g_rsxSettings.qcMajorVersion);
    buf->appendf("QCMinorVersion=%u\n",             g_rsxSettings.qcMinorVersion);

    buf->appendf("ExportRigSequences=%i\n",         g_rsxSettings.exportRigSequences);
    buf->appendf("ExportModelSkin=%i\n",            g_rsxSettings.exportModelSkin);
    buf->appendf("ExportTruncatedMaterials=%i\n",   g_rsxSettings.exportModelMatsTruncated);
    buf->appendf("ExportQCIFiles=%i\n",             g_rsxSettings.exportQCIFiles);

    buf->appendf("UseOrigScriptExportExtensions=%i\n", g_rsxSettings.useOrigScriptExportExtensions);

    buf->appendf("BridgePortNum=%i\n", g_rsxSettings.bridgePort);

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
        ImGuiReadSetting("PreviewCullDistance=%f",  settings->previewCullDistance, i, float);
        ImGuiReadSetting("PreviewMovementSpeed=%f", settings->previewMovementSpeed, i, float);
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

bool ImGuiCustomTextFilter::Draw(const char* label, const char* hint, float width)
{
    if (width != 0.0f)
    {
        ImGui::SetNextItemWidth(width);
    }

    const bool valChanged = ImGui::InputTextWithHint(label, hint, &inputBuf);
    if (valChanged)
    {
        Build();
    }

    return valChanged;
}

void ImGuiCustomTextFilter::TxtRange::split(const char separator, std::vector<TxtRange>* out) const
{
    assert(out->size() == 0);

    const char* wb = b;
    const char* we = wb;
    while (we < e)
    {
        if (*we == separator)
        {
            out->push_back(TxtRange(wb, we));
            wb = (we + 1);
        }
        we++;
    }

    if (wb != we)
    {
        out->push_back(TxtRange(wb, we));
    }
}

void ImGuiCustomTextFilter::Build()
{
    filters.clear();
    TxtRange txtInputRange(inputBuf.c_str(), inputBuf.end()._Ptr);
    txtInputRange.split(',', &filters);

    grepCnt = 0;
    for (TxtRange& f : filters)
    {
        while (f.b < f.e && charIsBlank(f.b[0]))
        {
            f.b++;
        }

        while (f.e > f.b && charIsBlank(f.e[-1]))
        {
            f.e--;
        }

        if (f.empty())
        {
            continue;
        }

        if (f.b[0] != '-')
        {
            grepCnt += 1;
        }
    }
}

bool ImGuiCustomTextFilter::PassFilter(const char* text, const char* text_end) const
{
    if (filters.size() == 0)
    {
        return true;
    }

    if (text == nullptr)
    {
        text = "";
        text_end = "";
    }

    for (const TxtRange& f : filters)
    {
        if (f.b == f.e)
        {
            continue;
        }

        if (f.b[0] == '-')
        {
            // exclude lookup
            if (ImStristr(text, text_end, f.b + 1, f.e) != NULL)
            {
                return false;
            }
        }
        else
        {
            // normal grep lookup
            if (ImStristr(text, text_end, f.b, f.e) != NULL)
            {
                return true;
            }
        }
    }

    // implicit * grep
    if (grepCnt == 0)
    {
        return true;
    }

    return false;
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

    ImFontConfig config;
    config.MergeMode = true;
    config.OversampleH = 2;
    config.OversampleV = 2;

    this->defaultFont = io.Fonts->AddFontFromMemoryCompressedTTF(SourceSansProRegular_compressed_data, sizeof(SourceSansProRegular_compressed_data), 18.f, NULL, io.Fonts->GetGlyphRangesDefault());
    
    char* systemRootPath = nullptr;
    size_t systemRootPathLen = 0;
    assertm(_dupenv_s(&systemRootPath, &systemRootPathLen, "SYSTEMROOT") == 0, "couldn't get systemroot env var");

    const std::string fontsDir = std::string(systemRootPath) + "\\Fonts\\";
    io.Fonts->AddFontFromFileTTF((fontsDir + "simsun.ttc").c_str(), 18.f, &config, io.Fonts->GetGlyphRangesChineseFull());
    io.Fonts->AddFontFromFileTTF((fontsDir + "malgun.ttf").c_str(), 18.f, &config, io.Fonts->GetGlyphRangesJapanese());
    io.Fonts->AddFontFromFileTTF((fontsDir + "micross.ttf").c_str(), 18.f, &config, io.Fonts->GetGlyphRangesCyrillic());
    io.Fonts->AddFontFromMemoryCompressedBase85TTF(Codicons_compressed_data, 14.f, &config);

    ImGuiStyle& style = ImGui::GetStyle();

    style.WindowMenuButtonPosition = ImGuiDir_None;
    ImVec4* colors = style.Colors;

    // Catppuccin Mocha Palette
    // --------------------------------------------------------
    const ImVec4 base = ImVec4(0.117f, 0.117f, 0.172f, 1.0f); // #1e1e2e
    const ImVec4 mantle = ImVec4(0.109f, 0.109f, 0.156f, 1.0f); // #181825
    const ImVec4 surface0 = ImVec4(0.200f, 0.207f, 0.286f, 1.0f); // #313244
    const ImVec4 surface1 = ImVec4(0.247f, 0.254f, 0.337f, 1.0f); // #3f4056
    const ImVec4 surface2 = ImVec4(0.290f, 0.301f, 0.388f, 1.0f); // #4a4d63
    const ImVec4 overlay0 = ImVec4(0.396f, 0.403f, 0.486f, 1.0f); // #65677c
    const ImVec4 overlay2 = ImVec4(0.576f, 0.584f, 0.654f, 1.0f); // #9399b2
    const ImVec4 text = ImVec4(0.803f, 0.815f, 0.878f, 1.0f); // #cdd6f4
    const ImVec4 subtext0 = ImVec4(0.639f, 0.658f, 0.764f, 1.0f); // #a3a8c3
    const ImVec4 mauve = ImVec4(0.796f, 0.698f, 0.972f, 1.0f); // #cba6f7
    const ImVec4 peach = ImVec4(0.980f, 0.709f, 0.572f, 1.0f); // #fab387
    const ImVec4 yellow = ImVec4(0.980f, 0.913f, 0.596f, 1.0f); // #f9e2af
    const ImVec4 green = ImVec4(0.650f, 0.890f, 0.631f, 1.0f); // #a6e3a1
    const ImVec4 teal = ImVec4(0.580f, 0.886f, 0.819f, 1.0f); // #94e2d5
    const ImVec4 darkTeal = ImVec4(0.157f, 0.353f, 0.282f, 1.0f); // #285a48
    const ImVec4 sapphire = ImVec4(0.458f, 0.784f, 0.878f, 1.0f); // #74c7ec
    const ImVec4 blue = ImVec4(0.533f, 0.698f, 0.976f, 1.0f); // #89b4fa
    const ImVec4 lavender = ImVec4(0.709f, 0.764f, 0.980f, 1.0f); // #b4befe

    // Main window and backgrounds
    colors[ImGuiCol_WindowBg] = base;
    colors[ImGuiCol_ChildBg] = base;
    colors[ImGuiCol_PopupBg] = surface0;
    colors[ImGuiCol_Border] = surface1;
    colors[ImGuiCol_BorderShadow] = ImVec4(0.0f, 0.0f, 0.0f, 0.0f);
    colors[ImGuiCol_FrameBg] = surface0;
    colors[ImGuiCol_FrameBgHovered] = surface1;
    colors[ImGuiCol_FrameBgActive] = surface2;
    colors[ImGuiCol_TitleBg] = mantle;
    colors[ImGuiCol_TitleBgActive] = surface0;
    colors[ImGuiCol_TitleBgCollapsed] = mantle;
    colors[ImGuiCol_MenuBarBg] = mantle;
    colors[ImGuiCol_ScrollbarBg] = surface0;
    colors[ImGuiCol_ScrollbarGrab] = surface2;
    colors[ImGuiCol_ScrollbarGrabHovered] = overlay0;
    colors[ImGuiCol_ScrollbarGrabActive] = overlay2;
    colors[ImGuiCol_CheckMark] = green;
    colors[ImGuiCol_SliderGrab] = sapphire;
    colors[ImGuiCol_SliderGrabActive] = blue;
    colors[ImGuiCol_Button] = surface0;
    colors[ImGuiCol_ButtonHovered] = surface1;
    colors[ImGuiCol_ButtonActive] = surface2;
    colors[ImGuiCol_Header] = surface0;
    colors[ImGuiCol_HeaderHovered] = surface1;
    colors[ImGuiCol_HeaderActive] = surface2;
    colors[ImGuiCol_Separator] = surface1;
    colors[ImGuiCol_SeparatorHovered] = mauve;
    colors[ImGuiCol_SeparatorActive] = mauve;
    colors[ImGuiCol_ResizeGrip] = surface2;
    colors[ImGuiCol_ResizeGripHovered] = mauve;
    colors[ImGuiCol_ResizeGripActive] = mauve;
    colors[ImGuiCol_Tab] = surface0;
    colors[ImGuiCol_TabHovered] = surface2;
    colors[ImGuiCol_TabActive] = surface1;
    colors[ImGuiCol_TabUnfocused] = surface0;
    colors[ImGuiCol_TabUnfocusedActive] = surface1;
    colors[ImGuiCol_DockingPreview] = sapphire;
    colors[ImGuiCol_DockingEmptyBg] = base;
    colors[ImGuiCol_PlotLines] = blue;
    colors[ImGuiCol_PlotLinesHovered] = peach;
    colors[ImGuiCol_PlotHistogram] = darkTeal;
    colors[ImGuiCol_PlotHistogramHovered] = green;
    colors[ImGuiCol_TableHeaderBg] = surface0;
    colors[ImGuiCol_TableBorderStrong] = surface1;
    colors[ImGuiCol_TableBorderLight] = surface0;
    colors[ImGuiCol_TableRowBg] = ImVec4(0.0f, 0.0f, 0.0f, 0.0f);
    colors[ImGuiCol_TableRowBgAlt] = ImVec4(1.0f, 1.0f, 1.0f, 0.005f);
    colors[ImGuiCol_TextSelectedBg] = surface2;
    colors[ImGuiCol_DragDropTarget] = yellow;
    colors[ImGuiCol_NavHighlight] = lavender;
    colors[ImGuiCol_NavWindowingHighlight] = ImVec4(1.0f, 1.0f, 1.0f, 0.7f);
    colors[ImGuiCol_NavWindowingDimBg] = ImVec4(0.8f, 0.8f, 0.8f, 0.2f);
    colors[ImGuiCol_ModalWindowDimBg] = ImVec4(0.0f, 0.0f, 0.0f, 0.35f);
    colors[ImGuiCol_Text] = text;
    colors[ImGuiCol_TextDisabled] = subtext0;

    // Rounded corners
    style.WindowRounding = 6.0f;
    style.ChildRounding = 6.0f;
    style.FrameRounding = 4.0f;
    style.PopupRounding = 4.0f;
    style.ScrollbarRounding = 9.0f;
    style.GrabRounding = 4.0f;
    style.TabRounding = 4.0f;

    // Padding and spacing
    style.WindowPadding = ImVec2(8.0f, 8.0f);
    style.FramePadding = ImVec2(5.0f, 3.0f);
    style.ItemSpacing = ImVec2(8.0f, 4.0f);
    style.ItemInnerSpacing = ImVec2(4.0f, 4.0f);
    style.IndentSpacing = 21.0f;
    style.ScrollbarSize = 14.0f;
    style.GrabMinSize = 10.0f;

    // Borders
    style.WindowBorderSize = 1.0f;
    style.ChildBorderSize = 1.0f;
    style.PopupBorderSize = 1.0f;
    style.FrameBorderSize = 0.0f;
    style.TabBorderSize = 0.0f;
}

namespace ImGuiExt {
    void HelpMarker(const char* const desc)
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

    void Tooltip(const char* const text)
    {
        assert(text);

        if (ImGui::IsItemHovered(ImGuiHoveredFlags_DelayShort) && ImGui::BeginTooltip())
        {
            ImGui::PushTextWrapPos(ImGui::GetFontSize() * 35.0f);
            ImGui::TextUnformatted(text);
            ImGui::PopTextWrapPos();
            ImGui::EndTooltip();
        }
    }
};

const ProgressBarEvent_t* const ImGuiHandler::AddProgressBarEvent(const char* const eventName, const uint32_t eventNum, std::atomic<uint32_t>* const remainingEvents, const bool isInverted)
{
    // Don't bother doing any of this if ImGui isn't running - it's not going to get rendered anyway
    if (noImGui)
        return nullptr;

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
        event->fnCancelEvents = nullptr; // These progress bar events are currently used for loading assets, which requires a bit more cleanup to cancel
                                         // So for now, only export events (using ImGuiHandler::AddProgressBarEvent as defined in imgui_utility.h) are cancellable

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
    if (!event || noImGui)
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
    // If the app is running without ImGui being initialised, don't try and run this method
    if (noImGui)
        return;

    std::unique_lock<std::mutex> lock(eventMutex);

    bool foundTopLevelBar = false;
    for (int i = 0; i < PB_SIZE; ++i)
    {
        ProgressBarEvent_t* const event = &pbEvents[i];
        if (!event->slotIsUsed)
            continue;

        if (!foundTopLevelBar)
        {
            ImVec2 viewportCenter = ImGui::GetMainViewport()->GetWorkCenter();

            ImGui::SetNextWindowSize(ImVec2(0, 0));
            ImGui::SetNextWindowPos(viewportCenter, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));

            if (!ImGui::Begin(event->eventName, nullptr, ImGuiWindowFlags_NoDocking | ImGuiWindowFlags_NoResize
                | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoSavedSettings
                | ImGuiWindowFlags_NoNav | ImGuiWindowFlags_NoScrollWithMouse
                | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse))
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
        ImGuiExt::ProgressBarCentered(progressFraction, ImVec2(485, 48), std::format("{}/{}", leftOverEvents, numEvents).c_str(), event);

        foundTopLevelBar = true;
    }

    if(foundTopLevelBar)
        ImGui::End();
}

// size_arg (for each axis) < 0.0f: align to end, 0.0f: auto, > 0.0f: specified size
void ImGuiExt::ProgressBarCentered(float fraction, const ImVec2& size_arg, const char* overlay, ProgressBarEvent_t* event)
{
    if (g_pImGuiHandler->NoImGui())
        return;

    using namespace ImGui;

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

    if (event && event->fnCancelEvents)
    {
        // https://github.com/ocornut/imgui/issues/4157
        float cancelButtonWidth = ImGui::CalcTextSize("Cancel").x + style.FramePadding.x * 2.f;
        float widthNeeded = style.ItemSpacing.x + cancelButtonWidth - 5.f;
        ImGui::SetCursorPosX(ImGui::GetCursorPosX() + ImGui::GetContentRegionAvail().x - widthNeeded);

        if (ImGui::Button("Cancel"))
            event->fnCancelEvents(event);
    }
}

bool ImGuiExt::Timeline(const char* strId, float currentTime, float endTime, size_t endFrame, const std::vector<AudioMarker_s>& markers, const ImVec2& size_arg, size_t* o_seekFrame)
{
    if (g_pImGuiHandler->NoImGui())
        return false;

    using namespace ImGui;

    ImGuiWindow* window = GetCurrentWindow();
    if (window->SkipItems)
        return false;

    ImGuiContext& g = *GImGui;
    const ImGuiStyle& style = g.Style;

    const ImGuiID id = window->GetID(strId);

    const ImVec2 pos = window->DC.CursorPos;
    const ImVec2 size = CalcItemSize(size_arg, CalcItemWidth(), g.FontSize + style.FramePadding.y * 2.0f);
    ImRect bb(pos, pos + size);
    ItemSize(size, style.FramePadding.y);
    if (!ItemAdd(bb, 0))
        return false;

    bool timelineHovered;
    bool held;
    ButtonBehavior(bb, id, &timelineHovered, &held);

    // Render
    RenderFrame(bb.Min, bb.Max, GetColorU32(ImGuiCol_FrameBg), true, style.FrameRounding);
    bb.Expand(ImVec2(-style.FrameBorderSize, -style.FrameBorderSize));

    const float fraction = endTime != 0.f ? ImSaturate(currentTime / endTime) : 0.f;
    RenderRectFilledRangeH(window->DrawList, bb, GetColorU32(ImGuiCol_PlotHistogram), 0.0f, fraction, style.FrameRounding);

    constexpr float markerLineThickness = 1.5f;
    constexpr float markerArrowSize = 5.f;

    size_t hoveredMarker = UINT64_MAX;
    float shortestDistanceToMarker = 5.f; // max distance to a marker is 5px

    if (endFrame != 0)
    {
        if (timelineHovered)
        {
            const float mouseX = g.IO.MousePos.x;

            size_t i = 0;

            // Precalculate which marker is hovered
            for (auto& marker : markers)
            {
                const float markerFrac = static_cast<float>(marker.framePosition) / endFrame;
                const float markerX = ImLerp(bb.Min.x, bb.Max.x, ImSaturate(markerFrac));
                const float distance = ImFabs(mouseX - (markerX + (markerLineThickness/2.f)));

                if (distance < shortestDistanceToMarker)
                {
                    hoveredMarker = i;
                    shortestDistanceToMarker = distance;
                }

                i++;
            }
        }
        
        size_t i = 0;

        window->DrawList->PushClipRect(bb.Min, bb.Max, true);
        for (auto& marker : markers)
        {
            const float markerFrac = static_cast<float>(marker.framePosition) / endFrame;

            // lerping or larping
            const float markerX = ImLerp(bb.Min.x, bb.Max.x, ImSaturate(markerFrac));

            const ImU32 markerColour = hoveredMarker == i ? GetColorU32(ImGuiCol_PlotLinesHovered) : GetColorU32(ImGuiCol_PlotLines);

            // Adjust the marker colour based on if we have passed this marker or not
            const ImU32 adjustedColour = fraction > markerFrac ? (markerColour & 0xFFFFFF) | 0x4f000000 : markerColour;

            // Line
            window->DrawList->AddLine(ImVec2(markerX, bb.Min.y), ImVec2(markerX, bb.Max.y), adjustedColour, markerLineThickness);
        
            // Arrow
            window->DrawList->AddTriangleFilled(
                ImVec2(markerX - (markerArrowSize / 2.f), bb.Min.y),
                ImVec2(markerX + (markerArrowSize / 2.f) + (markerLineThickness / 2.f), bb.Min.y),
                ImVec2(markerX + (markerLineThickness / 2.f), bb.Min.y + markerArrowSize),
                adjustedColour
            );
        
            i++;
        }
        window->DrawList->PopClipRect();
    }

    if (held)
    {
        size_t seekFrame = 0;
        if (hoveredMarker != UINT64_MAX)
            seekFrame = markers.at(hoveredMarker).framePosition;
        else
        {
            const float mouseX = g.IO.MousePos.x;
            const float distance = mouseX - bb.Min.x;

            const float seekFraction = distance / size.x;

            seekFrame = static_cast<size_t>(seekFraction * endFrame);
        }

        *o_seekFrame = seekFrame;
    }


    return held;
}

ImGuiHandler::ImGuiHandler()
{
    const uint32_t totalThreadCount = CThread::GetConCurrentThreads();

    // need at least one thread.
    cfg.exportThreadCount = 1u;
    cfg.parseThreadCount = std::max(totalThreadCount >> 1u, 1u);

    // standard config setting for compression
    cfg.compressionLevel = eCompressionLevel::CMPR_LVL_VERYFAST;

    cfg.checkForUpdates = true;

    memset(pbEvents, 0, sizeof(pbEvents));
    for (int8_t i = PB_SIZE - 1; i >= 0; --i) // in reverse order
    {
        pbAvailSlots.push(static_cast<int8_t>(i));
    }

    noImGui = false;
}

ImGuiHandler* g_pImGuiHandler = new ImGuiHandler();