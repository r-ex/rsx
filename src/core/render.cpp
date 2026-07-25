#include <pch.h>

#include <core/render.h>

#include <imgui.h>
#include <backends/imgui_impl_dx11.h>
#include <backends/imgui_impl_win32.h>
#include <misc/imgui_utility.h>
#include <game/rtech/utils/bsp/bspflags.h>

#include <core/render/dx.h>
#include <core/window.h>
#include <core/input/input.h>

#include <core/filehandling/export.h>

#include <game/rtech/cpakfile.h>
#include <game/rtech/assets/model.h>
#include <game/rtech/assets/texture.h>
#include <game/rtech/assets/settings.h>
#include <core/render/preview/preview.h>

#include "render/ui/styles.h"
#include <core/fonts/codicons.h>

#include <core/utils/gamefinder.hpp>
#include <misc/ImGuiNotify.hpp>

extern CDXParentHandler* g_dxHandler;
extern std::atomic<uint32_t> g_maxConcurrentThreadCount;
extern RSXSettings_t g_rsxSettings;

PreviewSettings_t g_PreviewSettings { .previewCullDistance = PREVIEW_CULL_DEFAULT, .previewMovementSpeed = PREVIEW_SPEED_DEFAULT };

CPreviewDrawData g_currentPreviewDrawData;

std::atomic<bool> inJobAction = false;

enum AssetColumn_t
{
    AC_Type,
    AC_Name,
    AC_GUID,
    AC_File,

    _AC_COUNT,
};

struct AssetCompare_t
{
    const ImGuiTableSortSpecs* sortSpecs;

    // TODO: get rid of pak-specific stuff
    bool operator() (const CGlobalAssetData::AssetLookup_t& a, const CGlobalAssetData::AssetLookup_t& b) const
    {
        const CAsset* assetA = a.m_asset;
        const CAsset* assetB = b.m_asset;

        for (int n = 0; n < sortSpecs->SpecsCount; n++)
        {
            // Here we identify columns using the ColumnUserID value that we ourselves passed to TableSetupColumn()
            // We could also choose to identify columns based on their index (sort_spec->ColumnIndex), which is simpler!
            const ImGuiTableColumnSortSpecs* sort_spec = &sortSpecs->Specs[n];
            __int64 delta = 0;
            switch (sort_spec->ColumnUserID)
            {
            case AssetColumn_t::AC_Type:   delta = static_cast<int64_t>(_byteswap_ulong(assetA->GetAssetType())) - _byteswap_ulong(assetB->GetAssetType());                        break; // no overflow please
            case AssetColumn_t::AC_Name:   delta = _stricmp(assetA->GetAssetName().c_str(), assetB->GetAssetName().c_str());                        break; // no overflow please
            case AssetColumn_t::AC_GUID:   delta = assetA->GetAssetGUID() - assetB->GetAssetGUID();     break;
            case AssetColumn_t::AC_File:   delta = _stricmp(assetA->GetContainerFileName().c_str(), assetB->GetContainerFileName().c_str());     break;
            default: IM_ASSERT(0); break;
            }
            if (delta > 0)
                return (sort_spec->SortDirection == ImGuiSortDirection_Ascending) ? false : true;
            if (delta < 0)
                return (sort_spec->SortDirection == ImGuiSortDirection_Ascending) ? true : false;
        }

        return (static_cast<int64_t>(assetA->GetAssetType()) - assetB->GetAssetType()) > 0;
    }
};

void ColouredTextForAssetType(const CAsset* const asset)
{
    bool colouredText = false;

    switch (asset->GetAssetContainerType())
    {
    case CAsset::ContainerType::PAK:
    case CAsset::ContainerType::AUDIO:
    case CAsset::ContainerType::MDL:
    case CAsset::ContainerType::BP_PAK:
    {
        const AssetType_t assetType = static_cast<AssetType_t>(asset->GetAssetType());
        if (s_AssetTypeColours.contains(assetType))
        {
            colouredText = true;

            const Color4& col = s_AssetTypeColours.at(assetType);
            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(col.r, col.g, col.b, col.a));
        }

        ImGui::Text("%s %s", fourCCToString(asset->GetAssetType()).c_str(), asset->GetAssetVersion().ToString().c_str());
        break;
    }
    [[unlikely]] default:
    {
        ImGui::TextUnformatted("unimpl");
        break;
    }
    }

    if (colouredText)
        ImGui::PopStyleColor();
}

// [ Preview ] Render the asset dependencies table in the preview window for the provided asset
void PreviewWnd_AssetDepsTbl(CAsset* asset)
{
    CPakAsset* pakAsset = static_cast<CPakAsset*>(asset);

    std::vector<AssetGuid_t> dependencies;
    pakAsset->getDependencies(dependencies);

    constexpr ImGuiTableFlags tableFlags =
        ImGuiTableFlags_Resizable | ImGuiTableFlags_Reorderable | ImGuiTableFlags_Hideable
        | ImGuiTableFlags_RowBg | ImGuiTableFlags_Borders | ImGuiTableFlags_NoBordersInBody
        | ImGuiTableFlags_ScrollY | ImGuiTableFlags_SizingFixedFit;

    const ImVec2 outerSize = ImVec2(0.f, ImGui::GetTextLineHeightWithSpacing() * 12.f);

    constexpr int numColumns = 5;

    if (ImGui::TreeNodeEx("Asset Dependencies", ImGuiTreeNodeFlags_SpanAvailWidth))
    {
        if (ImGui::BeginTable("Assets", numColumns, tableFlags, outerSize))
        {
            ImGui::TableSetupColumn("Type", ImGuiTableColumnFlags_NoResize | ImGuiTableColumnFlags_WidthFixed | ImGuiTableColumnFlags_NoHide, 0.f, 0);
            ImGui::TableSetupColumn("Name", ImGuiTableColumnFlags_NoResize | ImGuiTableColumnFlags_WidthFixed | ImGuiTableColumnFlags_NoHide, 0.f, 1);
            ImGui::TableSetupColumn("Pak", ImGuiTableColumnFlags_NoResize | ImGuiTableColumnFlags_WidthFixed | ImGuiTableColumnFlags_NoHide, 0.f, 2);
            ImGui::TableSetupColumn("Status", ImGuiTableColumnFlags_NoResize | ImGuiTableColumnFlags_WidthFixed | ImGuiTableColumnFlags_NoHide, 0.f, 3);
            ImGui::TableSetupColumn("", ImGuiTableColumnFlags_NoResize | ImGuiTableColumnFlags_WidthFixed | ImGuiTableColumnFlags_NoHide, 0.f, 4);
            ImGui::TableSetupScrollFreeze(1, 1);

            ImGui::TableHeadersRow();

            for (int d = 0; d < dependencies.size(); ++d)
            {
                AssetGuid_t guid = dependencies[d];
                CAsset* depAsset = g_assetData.FindAssetByGUID<CPakAsset>(guid.guid);

#if _DEBUG
                // [rika]: cannot access depAsset if nullptr
                if (depAsset)
                    assert(depAsset->GetAssetContainerType() == CAsset::ContainerType::PAK);
#endif // _DEBUG

                ImGui::PushID(d);

                ImGui::TableNextRow(ImGuiTableRowFlags_None, 0.f);

                // Asset Type
                if (ImGui::TableSetColumnIndex(0))
                {
                    ImGui::AlignTextToFramePadding();
                    if (depAsset)
                        ColouredTextForAssetType(depAsset);
                    else
                        ImGui::TextUnformatted("n/a");
                }

                // Asset GUID
                if (ImGui::TableSetColumnIndex(1))
                {
                    ImGui::AlignTextToFramePadding();
                    if (depAsset)
                        ImGui::TextUnformatted(depAsset->GetAssetName().c_str());
                    else
                        ImGui::Text("%016llX", guid.guid);
                }

                // Asset Container Name (e.g. common.rpak, general_stream.mstr)
                if (ImGui::TableSetColumnIndex(2))
                {
                    ImGui::AlignTextToFramePadding();
                    if (!depAsset)
                        ImGui::TextUnformatted("n/a");
                    else
                        ImGui::TextUnformatted(depAsset->GetContainerFileName().c_str());
                }

                // Export Status
                if (ImGui::TableSetColumnIndex(3))
                {
                    ImGui::AlignTextToFramePadding();
                    if (depAsset)
                    {
                        if (depAsset->GetExportedStatus())
                        {
                            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.f, 1.f, 1.f, 1.f));
                            ImGui::TextUnformatted("Exported");
                        }
                        else
                        {
                            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.f, 1.f, 0.f, 1.f));
                            ImGui::TextUnformatted("Loaded");
                        }
                    }
                    else
                    {
                        ImGui::PushStyleColor(ImGuiCol_Text, Styles::TEXTCOL_NOT_LOADED);
                        ImGui::TextUnformatted("Not Loaded");
                    }

                    ImGui::PopStyleColor();
                }

                if (ImGui::TableSetColumnIndex(4))
                {
                    ImGui::AlignTextToFramePadding();
                    if (!depAsset)
                        ImGui::BeginDisabled();

                    if (ImGui::Button("Export"))
                        CThread(HandleExportBindingForAsset, depAsset, g_rsxSettings.exportAssetDeps).detach();

                    if (!depAsset)
                        ImGui::EndDisabled();
                }

                ImGui::PopID();
            }

            ImGui::EndTable();
        }

        ImGui::TreePop();
    }
}

void SettingsWnd_Draw(CUIState* uiState)
{
    constexpr uint32_t minThreads = 1u;

    ImGui::SetNextWindowSize(ImVec2(0.f, 500.f), ImGuiCond_FirstUseEver);
    if (ImGui::Begin("Settings", &uiState->settingsWindowVisible, ImGuiWindowFlags_NoDocking | ImGuiWindowFlags_NoCollapse))
    {
#if !defined(_DEBUG) && !defined(NO_LIBCURL)
        // ===============================================================================================================
        ImGui::SeparatorText("General");

        ImGui::Checkbox("Check for updates", &UtilsConfig->checkForUpdates);
        ImGui::SameLine();
        ImGuiExt::HelpMarker("RSX will check for updates against the GitHub repository when opened.\nIf there is a new update available, a message will be displayed on the RSX welcome dialog box");
#endif
        
        // ===============================================================================================================
        ImGui::SeparatorText("Export");

        ImGui::Checkbox("Export full asset paths", &g_rsxSettings.exportPathsFull);
        ImGui::SameLine();
        ImGuiExt::HelpMarker("Enables exporting of assets to their full path within the export directory, as shown by the listed asset names.\nWhen disabled, assets will be exported into the root-level of a folder named after the asset's type (e.g. \"material/\",\"ui_image/\").");

        ImGui::Checkbox("Export asset dependencies", &g_rsxSettings.exportAssetDeps);
        ImGui::SameLine();
        ImGuiExt::HelpMarker("Enables exporting of all dependencies that are associated with any asset that is being exported.");

        ImGui::Checkbox("Disable CacheDB loading", &g_rsxSettings.disableCachedNames);
        ImGui::SameLine();
        ImGuiExt::HelpMarker("Disables loading names from the cache file. The cache will still be updated, but RSX will not display any cached data");

        // texture settings
        ImGui::SeparatorText("Export (Textures)");

        ImGui::Combo("Material Texture Naming", reinterpret_cast<int*>(&g_rsxSettings.exportTextureNameSetting), s_TextureExportNameSetting, static_cast<int>(ARRAYSIZE(s_TextureExportNameSetting)));
        ImGui::SameLine();
        ImGuiExt::HelpMarker("Naming scheme for exporting textures via materials options are as follows:\nGUID: exports only using the asset's GUID as a name.\nReal: exports texture using a real name (asset name or guid if no name).\nText: exports the texture with a text name always, generating one if there is none provided.\nSemantic: exports with a generated name all the time, useful for models.");

        ImGui::Combo("Normal Recalculation", reinterpret_cast<int*>(&g_rsxSettings.exportNormalRecalcSetting), s_NormalExportRecalcSetting, static_cast<int>(ARRAYSIZE(s_NormalExportRecalcSetting)));
        ImGui::SameLine();
        ImGuiExt::HelpMarker("None: exports the normal as it is stored.\nDirectX: exports with a generated blue channel.\nOpenGL: exports with a generated blue channel and inverts the green channel.");

        ImGui::Checkbox("Export Material Textures", &g_rsxSettings.exportMaterialTextures);
        ImGui::SameLine();
        ImGuiExt::HelpMarker("Enables exporting of all textures that are associated with any material asset that is being exported.");

        // model settings
        ImGui::SeparatorText("Export (Models)");

        ImGui::Checkbox("Export Sequences", &g_rsxSettings.exportRigSequences);
        ImGui::SameLine();
        ImGuiExt::HelpMarker("Enables exporting of all animation sequences that are associated with any rig or model asset that is being exported.");

        ImGui::Checkbox("Export Skin", &g_rsxSettings.exportModelSkin);
        ImGui::SameLine();
        ImGuiExt::HelpMarker("Enables exporting a model with the previewed skin.");

        ImGui::Checkbox("Truncate Materials", &g_rsxSettings.exportModelMatsTruncated);
        ImGui::SameLine();
        ImGuiExt::HelpMarker("Truncates material names on SMD.");

        ImGui::Checkbox("Enable QCI Files", &g_rsxSettings.exportQCIFiles);
        ImGui::SameLine();
        ImGuiExt::HelpMarker("QC file will be split into multiple include files.");

        // hidden for now
        //ImGui::SeparatorText("Export (Wrapped Files)");

        //ImGui::Checkbox("Use original script extensions", &g_rsxSettings.useOrigScriptExportExtensions);
        //ImGui::SameLine();
        //ImGuiExt::HelpMarker("Enables the usage of the game's original script file extensions. e.g., .nut.ui instead of .ui.nut");

        ImGui::PushItemWidth(48.0f);
        ImGui::InputScalar("##QCTargetMajor", ImGuiDataType_U16, reinterpret_cast<uint16_t*>(&g_rsxSettings.qcMajorVersion), nullptr, nullptr, "%u", ImGuiInputTextFlags_CharsDecimal);
        ImGui::SameLine();
        ImGui::InputScalar("##QCTargetMinor", ImGuiDataType_U16, reinterpret_cast<uint16_t*>(&g_rsxSettings.qcMinorVersion), nullptr, nullptr, "%u", ImGuiInputTextFlags_CharsDecimal);
        ImGui::PopItemWidth();
        ImGui::SameLine();
        ImGui::Text("QC Target Version");
        ImGui::SameLine();
        ImGuiExt::HelpMarker("Desired version for QC files to be compatible with.");

        // physics settings
        ImGui::InputInt("Physics contents filter", reinterpret_cast<int*>(&g_rsxSettings.exportPhysicsContentsFilter), 1, 100, ImGuiInputTextFlags_CharsHexadecimal);
        ImGui::SameLine();
        ImGuiExt::HelpMarker("Filter physics meshes in or out based on selected contents.");

        ImGui::Checkbox("Physics contents filter exclusive", &g_rsxSettings.exportPhysicsFilterExclusive);
        ImGui::SameLine();
        ImGuiExt::HelpMarker("Exclude physics meshes containing any of the specified contents.");

        ImGui::Checkbox("Physics contents filter require all", &g_rsxSettings.exportPhysicsFilterAND);
        ImGui::SameLine();
        ImGuiExt::HelpMarker("Filter only physics meshes containing all specified contents.");

        // ===============================================================================================================
        ImGui::SeparatorText("Parsing");

        ImGui::Combo("Compression Level", reinterpret_cast<int*>(&UtilsConfig->compressionLevel), s_CompressionLevelSetting, static_cast<int>(ARRAYSIZE(s_CompressionLevelSetting)));
        ImGui::SameLine();
        ImGuiExt::HelpMarker("Specifies the compression level used when storing parsed assets in memory.\nWARNING: Modify only if you know what you’re doing; otherwise, you may run out of memory.\nNone: no compression.\nSuper Fast: Fastest level with the lowest compression ratio.\nVery Fast: Standard setting; fastest level with a decent compression ratio.\nFast: Fastest level with a good compression ratio.\nNormal: Standard LZ speed with the highest compression ratio.");

        ImGui::SliderScalar("Parse Threads", ImGuiDataType_U32, &UtilsConfig->parseThreadCount, &minThreads, reinterpret_cast<int*>(&g_maxConcurrentThreadCount));
        ImGui::SameLine();
        ImGuiExt::HelpMarker("The number of CPU threads that will be used for loading files.\n\nIn general, the higher the number, the faster RSX will be able to load the selected files.");

        ImGui::SliderScalar("Export Threads", ImGuiDataType_U32, &UtilsConfig->exportThreadCount, &minThreads, reinterpret_cast<int*>(&g_maxConcurrentThreadCount));
        ImGui::SameLine();
        ImGuiExt::HelpMarker("The number of CPU threads that will be used for exporting assets.\n\nA higher number of threads will usually make RSX export assets more quickly, however the increased disk usage may cause decreased performance.");

        // ===============================================================================================================
        ImGui::SeparatorText("Preview");

        if (ImGui::SliderFloat("Cull Distance", &g_PreviewSettings.previewCullDistance, PREVIEW_CULL_MIN, PREVIEW_CULL_MAX))
            g_dxHandler->UpdateProjectionMatrix();

        ImGui::SameLine();
        ImGuiExt::HelpMarker("Distance at which render of 3D objects will stop.");

        // ===============================================================================================================
        ImGui::SeparatorText("Asset Settings");

        for (auto& [fourCC, binding] : g_assetData.m_assetTypeBindings)
        {
            if (binding.e.exportSettingArr)
            {
                bool colouredText = false;
                const AssetType_t assetType = static_cast<AssetType_t>(fourCC);
                if (s_AssetTypeColours.contains(assetType))
                {
                    colouredText = true;

                    const Color4& col = s_AssetTypeColours.at(assetType);
                    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(col.r, col.g, col.b, col.a));
                }
                ImGui::SeparatorText(std::format("{} ({})", binding.name, fourCCToString(fourCC, true)).c_str());

                if (colouredText) ImGui::PopStyleColor();

                // Prevent the asset status from being toggled while files are loading, as this could cause some serious inconsistencies
                // if changed while assets are already being processed.
                // This setting must also not be changed while files are already opened, as otherwise RSX may attempt to preview an asset
                // that has not been loaded
                ImGui::BeginDisabled(inJobAction || g_assetData.v_assetContainers.size() != 0);

                ImGui::SetCursorPosX(ImGui::GetCursorPosX() + 25.f);
                ImGui::Checkbox(std::format("Load Asset Type##_{}", binding.name).c_str(), &binding._loadAssetType);

                ImGui::EndDisabled();

                ImGui::SameLine();
                ImGuiExt::HelpMarker("This setting determines whether this asset type should be processed by RSX when loading asset files.\n\nIMPORTANT: This setting can only be changed before selecting pak files to open.");


                ImGui::SetCursorPosX(ImGui::GetCursorPosX() + 25.f);
                ImGui::TextUnformatted("Format"); ImGui::SameLine();

                ImGui::SetCursorPosX(ImGui::GetCursorPosX() + 25.f);
                ImGui::Combo(std::format("##ExportFormat_{}", binding.name).c_str(), &binding.e.exportSetting, binding.e.exportSettingArr, static_cast<int>(binding.e.exportSettingArrSize));
            
                if (auto settingsIt = g_rsxSettings.assetSettings.find(fourCC); settingsIt != g_rsxSettings.assetSettings.end())
                {
                    std::vector<UISetting_t>& vec = settingsIt->second;

                    for (auto& setting : vec)
                    {
                        ImGui::SetCursorPosX(ImGui::GetCursorPosX() + 25.f);
                        switch (setting.valueType)
                        {
                        case UISettingType_e::TYPE_BOOL:
                        {
                            ImGui::Checkbox(setting.displayName, &setting.rawValue.boolVal);
                            break;
                        }
                        }
                    }
                }
            }
        }
    }
    ImGui::End();
}
extern void ItemflavWnd_Draw(CUIState*);
extern void LogWnd_Draw(CUIState*);

static std::deque<CAsset*> s_selectedAssets;
static std::vector<CGlobalAssetData::AssetLookup_t> s_filteredAssets;
static CAsset* s_prevRenderInfoAsset = nullptr;

void ApplySelectionRequests(ImGuiMultiSelectIO* ms_io, std::deque<CAsset*>& selectedAssets, std::vector<CGlobalAssetData::AssetLookup_t>& pakAssets)
{
    for (ImGuiSelectionRequest& req : ms_io->Requests)
    {
        if (req.Type == ImGuiSelectionRequestType_SetAll) {
            selectedAssets.clear();
            if (req.Selected) {
                for (int n = 0; n < pakAssets.size(); n++)
                {
                    selectedAssets.insert(selectedAssets.end(), pakAssets[n].m_asset);
                }
            }
        }

        if (req.Type == ImGuiSelectionRequestType_SetRange)
        {
            for (int n = (int)req.RangeFirstItem; n <= (int)req.RangeLastItem; n++)
            {
                auto iter = std::find(selectedAssets.begin(), selectedAssets.end(), pakAssets[n].m_asset);
                const bool isSelected = iter != selectedAssets.end();

                if (!isSelected && req.Selected) selectedAssets.insert(selectedAssets.end(), pakAssets[n].m_asset);
                if (isSelected && !req.Selected) selectedAssets.erase(iter);
            }
        }
    }
}

// Reset the load state of the UI so that we can prepare to un/load a new set of files
void ClearLoadState()
{
    s_selectedAssets.clear();
    s_filteredAssets.clear();
    s_prevRenderInfoAsset = nullptr;
    g_assetData.ClearAssetData();
    g_dxHandler->GetUIState().ClearAssetData();
}

void ShowOpenFileDialog()
{
    ClearLoadState();

    CThread(HandleOpenFileDialog, g_dxHandler->GetWindowHandle()).detach();
}

void MainWnd_MenuBar()
{
    if (ImGui::BeginMainMenuBar())
    {
        if (ImGui::BeginMenu("File"))
        {
            if (ImGui::MenuItem("Open", "CTRL+O", false, !inJobAction))
                ShowOpenFileDialog();
            if (inJobAction) ImGui::SetItemTooltip("Unable to open new files while file loading is still in progress");


            if (ImGui::MenuItem("Unload Files", "CTRL+W", false, !inJobAction))
            {
                if (g_assetData.v_assets.size() > 0)
                    g_assetData.Log_Info(nullptr, "Unloaded %lld asset%s from %lld container file%s", g_assetData.v_assets.size(), g_assetData.v_assets.size() == 1 ? "" : "s", g_assetData.v_assetContainers.size(), g_assetData.v_assetContainers.size() == 1 ? "" : "s");

                ClearLoadState();
            }
            if (inJobAction) ImGui::SetItemTooltip("Unable to open new files while file loading is still in progress");

            ImGui::EndMenu();
        }

        if (ImGui::BeginMenu("Edit"))
        {
            CUIState& uiState = g_dxHandler->GetUIState();
            if (ImGui::MenuItem("Settings"))
                uiState.ShowSettingsWindow(true);

            //if (ImGui::MenuItem("Skin Finder"))
            //    uiState.ShowItemflavWindow(true);

            if (ImGui::MenuItem("Logs"))
                uiState.ShowLogWindow(true);

            ImGui::EndMenu();
        }
        
        const std::pair<int, int> errorCounts = g_assetData.m_logErrorListInfo.GetPair();
        
        ImGui::SetCursorPosX(ImGui::GetCursorPosX() + 5.f);
        ImGuiExt::IconText(ICON_CI_WARNING, ImVec4(1.f, 1.f, 0.f, 1.f));  ImGui::SameLine();
        ImGui::Text("%i", errorCounts.first); ImGui::SameLine();
        ImGuiExt::IconText(ICON_CI_ERROR, ImVec4(1.f, 0.f, 0.f, 1.f)); ImGui::SameLine();
        ImGui::Text("%i", errorCounts.second);

#if _DEBUG
        IMGUI_RIGHT_ALIGN_FOR_TEXT("Avg 1.000 ms/frame (100.0 FPS)"); // [rexx]: i hate this actually

        ImGuiIO& io = ImGui::GetIO();
        ImGui::Text("Avg %.3f ms/frame (%.1f FPS)", 1000.0f / io.Framerate, io.Framerate);
#endif
        ImGui::EndMainMenuBar();
    }
}

extern void HandlePakLoad(std::vector<std::string> filePaths);

static void MainWnd_LaunchSkinFinderForGame(const std::filesystem::path& dirPath, GameFinderGame_e gameType)
{
    if (gameType == GameFinderGame_e::APEX_LEGENDS)
    {
        const std::vector<std::string> filePaths = {
            (dirPath / "paks/Win64/localization_english.rpak").string(),
            (dirPath / "paks/Win64/common_early.rpak").string(),
            (dirPath / "paks/Win64/common.rpak").string(),
            (dirPath / "paks/Win64/ui.rpak").string(),
        };

        CThread([](const std::vector<std::string> filePaths) {
            inJobAction = true;

            ClearLoadState();

            HandlePakLoad(std::move(filePaths));
            g_assetData.ProcessAssetsPostLoad();

            inJobAction = false;
            }, std::move(filePaths)).detach();
    }
};

#define SHOW_WELCOME_BOX (!inJobAction && g_assetData.v_assetContainers.empty())

static void MainWnd_WelcomeBox()
{
    // Don't bother showing the welcome box if we're in file load. We can't do shit anyway...
    if (inJobAction)
        return;

    static bool firstTimeWelcoming = true;
    static GameFinderResults_s gfResults = {};
    static int64_t selectedGameDirectoryIdx = -1;
    if (SHOW_WELCOME_BOX)
    {
        // There's no point in rediscovering the game installations if the user loads files and then unloads them, since
        // it's incredibly unlikely that in that time they have un/installed a new compatible game, and doing this minimises
        // registry/FS accesses
        if (firstTimeWelcoming)
        {
            GameFinder_FindAllCompatibleSteamGames(&gfResults);

            firstTimeWelcoming = false;
        }

        ImGui::SetNextWindowPos(ImGui::GetMainViewport()->GetCenter(), ImGuiCond_Always, ImVec2(0.5f, 0.5f));

        ImGui::SetNextWindowSize(ImVec2(600, 0), ImGuiCond_Always);

        // [Dialog] Welcome Message
        if (ImGui::Begin("reSource Xtractor", nullptr, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse))
        {
            ImGui::PushTextWrapPos();
            ImGui::TextUnformatted(RSX_WELCOME_MESSAGE);
            ImGui::PopTextWrapPos();

            ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 5.f);

            if (ImGui::Button("Open File..."))
                ShowOpenFileDialog();

            if (gfResults.gamePaths.size() > 0)
            {
                ImGui::Separator();
                ImGui::TextUnformatted("Compatible Game Installations");

                if (ImGui::BeginTable("Assets", 2, ImGuiTableFlags_BordersOuter))
                {
                    ImGui::TableSetupColumn("", ImGuiTableColumnFlags_WidthFixed, 0, 0);
                    ImGui::TableSetupColumn("Path", ImGuiTableColumnFlags_WidthFixed, 0, 1);

                    ImGui::TableSetupScrollFreeze(0, 1);
                    ImGui::TableHeadersRow();

                    int i = 0;
                    for (auto& path : gfResults.gamePaths)
                    {
                        ImGui::PushID(i);
                        ImGui::TableNextRow();

                        if (ImGui::TableSetColumnIndex(0))
                        {
                            ImGui::TextUnformatted("Steam");
                        }

                        if (ImGui::TableSetColumnIndex(1))
                        {
                            const bool selected = selectedGameDirectoryIdx == i;
                            if (ImGui::Selectable(path.string().c_str(), selected, ImGuiSelectableFlags_SpanAllColumns))
                                selectedGameDirectoryIdx = i;
                        }

                        ImGui::PopID();

                        i++;
                    }

                    ImGui::EndTable();
                };

                if (selectedGameDirectoryIdx != -1)
                {
                    const char* buttonLabel = "Open";
                    switch (gfResults.gameTypes[selectedGameDirectoryIdx])
                    {
                    case GameFinderGame_e::TITANFALL_1: // wait r1 doesn't have rpaks... good thing the gamefinder doesn't search for r1 yet!
                    case GameFinderGame_e::TITANFALL_2:
                        buttonLabel = "Open common.rpak";
                        break;
                    case GameFinderGame_e::APEX_LEGENDS:
                        buttonLabel = "Open Skin Finder";
                        break;
                    }

                    ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 5.f);
                    if (ImGui::Button(buttonLabel))
                    {
                        g_assetData.AddPostLoadFinishedCallback([]() {
                            CThread([]() {
                                g_dxHandler->GetUIState().ShowItemflavWindow(true);
                                }).detach();
                            }, true);

                        MainWnd_LaunchSkinFinderForGame(gfResults.gamePaths[selectedGameDirectoryIdx], gfResults.gameTypes[selectedGameDirectoryIdx]);
                    }
                }
            }

            CUIState& uiState = g_dxHandler->GetUIState();

            if (UtilsConfig->checkForUpdates && uiState.newVersionType)
            {
                ImGui::Separator();
                ImGui::Text("There's a new update available for RSX! Download the latest %s version from", uiState.newVersionType);
                ImGui::SameLine();
                ImGui::SetCursorPosX(ImGui::GetCursorPosX() - 5.f);
                ImGui::TextLinkOpenURL("here!", uiState.newVersionReleaseInfo.htmlUrl.c_str());
            }

            ImGui::End();
        }
    }
}

static void MainWnd_AssetListMenuBar(std::vector<CGlobalAssetData::AssetLookup_t>& pakAssets)
{
    if (ImGui::BeginMenuBar())
    {
        if (ImGui::BeginMenu("Export"))
        {
            const bool multipleAssetsSelected = s_selectedAssets.size() > 1;

            if (ImGui::Selectable(multipleAssetsSelected ? "Export selected assets" : "Export selected asset"))
            {
                if (!s_selectedAssets.empty())
                {
                    std::deque<CAsset*> cpyAssets;
                    cpyAssets.insert(cpyAssets.end(), s_selectedAssets.begin(), s_selectedAssets.end());
                    CThread(HandlePakAssetExportList, std::move(cpyAssets), g_rsxSettings.exportAssetDeps).detach();
                    s_selectedAssets.clear();
                }
            }

            if (ImGui::Selectable("Export all for selected type", false, multipleAssetsSelected ? ImGuiSelectableFlags_Disabled : 0))
            {
                if (s_selectedAssets.size() == 1)
                {
                    const uint32_t desiredType = s_selectedAssets[0]->GetAssetType();
                    auto allAssetsOfDesiredType = pakAssets | std::ranges::views::filter([desiredType](const CGlobalAssetData::AssetLookup_t& a)
                        {
                            return a.m_asset->GetAssetType() == desiredType;
                        });

                    std::vector<CGlobalAssetData::AssetLookup_t> allAssets(allAssetsOfDesiredType.begin(), allAssetsOfDesiredType.end());
                    CThread(HandleExportSelectedAssetType, std::move(allAssets), g_rsxSettings.exportAssetDeps).detach();
                    s_selectedAssets.clear();
                }
            }

            if (ImGui::Selectable("Export all for selected pak", false, multipleAssetsSelected ? ImGuiSelectableFlags_Disabled : 0))
            {
                if (s_selectedAssets.size() == 1 && s_selectedAssets[0]->GetAssetContainerType() == CAsset::ContainerType::PAK)
                {
                    const CPakFile* desiredPak = s_selectedAssets[0]->GetContainerFile<const CPakFile>();
                    auto allAssetsOfDesiredType = pakAssets | std::ranges::views::filter([desiredPak](const CGlobalAssetData::AssetLookup_t& a)
                        {
                            return a.m_asset->GetAssetContainerType() == CAsset::ContainerType::PAK && a.m_asset->GetContainerFile<const CPakFile>() == desiredPak;
                        });

                    std::vector<CGlobalAssetData::AssetLookup_t> allAssets(allAssetsOfDesiredType.begin(), allAssetsOfDesiredType.end());
                    CThread(HandleExportSelectedAssetType, std::move(allAssets), g_rsxSettings.exportAssetDeps).detach();
                    s_selectedAssets.clear();
                }
            }

            if (ImGui::Selectable("Export all for selected pak and type", false, multipleAssetsSelected ? ImGuiSelectableFlags_Disabled : 0))
            {
                if (s_selectedAssets.size() == 1 && s_selectedAssets[0]->GetAssetContainerType() == CAsset::ContainerType::PAK)
                {
                    const CPakFile* desiredPak = s_selectedAssets[0]->GetContainerFile<const CPakFile>();
                    const uint32_t desiredType = s_selectedAssets[0]->GetAssetType();
                    auto allAssetsOfDesiredType = pakAssets | std::ranges::views::filter([desiredPak, desiredType](const CGlobalAssetData::AssetLookup_t& a)
                        {
                            return a.m_asset->GetAssetContainerType() == CAsset::ContainerType::PAK
                                && a.m_asset->GetContainerFile<CPakFile>() == desiredPak
                                && a.m_asset->GetAssetType() == desiredType;
                        });

                    std::vector<CGlobalAssetData::AssetLookup_t> allAssets(allAssetsOfDesiredType.begin(), allAssetsOfDesiredType.end());
                    CThread(HandleExportSelectedAssetType, std::move(allAssets), g_rsxSettings.exportAssetDeps).detach();
                    s_selectedAssets.clear();
                }
            }

            if (ImGui::Selectable("Export all"))
                CThread(HandleExportAllPakAssets, &pakAssets, g_rsxSettings.exportAssetDeps).detach();

            // Exports the names of all assets in the currently shown filtered asset list (i.e., search results)
            if (ImGui::Selectable("Export list of asset names..."))
                CThread(HandleListExportPakAssets, g_dxHandler->GetWindowHandle(), &pakAssets).detach();

            ImGui::EndMenu();
        }

        const std::string assetCountText = std::format("{} assets", !FilterConfig->textFilter.IsActive() ? g_assetData.v_assets.size() : s_filteredAssets.size());

        const float availX = ImGui::GetContentRegionAvail().x;
        const float sizeX = ImGui::CalcTextSize(assetCountText.c_str()).x;
        ImGui::SetCursorPosX(ImGui::GetCursorPosX() + (availX-sizeX));

        ImGui::TextDisabled("%s", assetCountText.c_str());

        ImGui::EndMenuBar();
    }
}

static ImVec2 s_previousAvailableSizeForPreview(-1, -1);

void HandleRenderFrame()
{
    ImGui_ImplDX11_NewFrame();
    ImGui_ImplWin32_NewFrame();
    ImGui::NewFrame();

    // These shortcuts shouldn't be allowed to work when we are already processing a file load
    if (!inJobAction)
    {
        // CTRL + O : Open files
        if (ImGui::Shortcut(ImGuiMod_Ctrl | ImGuiKey_O, ImGuiInputFlags_RouteGlobal))
            ShowOpenFileDialog();

        // CTRL + W : Unload files
        if (ImGui::Shortcut(ImGuiMod_Ctrl | ImGuiKey_W, ImGuiInputFlags_RouteGlobal))
            ClearLoadState();
    }

    // create a docking area across the entire viewport
    if (ImGui::GetIO().ConfigFlags & ImGuiConfigFlags_DockingEnable)
    {
        const ImGuiID dockspaceId = ImGui::GetID("MainDockSpace");

        // If the dockspace hasn't been created yet
        if (ImGui::DockBuilderGetNode(dockspaceId) == nullptr)
        {
            DockBuilder(dockspaceId)
                .Window("Scene", true)
                .Window("Skin Finder", true)
                .DockLeft(0.25f)
                    .Window("Asset List")
                    .Done()
                .DockRight(0.25f)
                    .Window("Asset Info");
        }

        ImGui::DockSpaceOverViewport(dockspaceId, NULL, ImGuiDockNodeFlags_PassthruCentralNode, 0);
    }

    // while ImGui is using keyboard input, we should not accept any keyboard input, but we should also clear all  
    // existing key states, as otherwise we can end up moving forever (until ImGui loses focus) in model preview if
    // the movement keys are held when ImGui starts capturing keyboard
    if (ImGui::GetIO().WantCaptureKeyboard)
        g_pInput->ClearKeyStates();

    g_pInput->Frame(ImGui::GetIO().DeltaTime);

    g_dxHandler->GetCamera()->Move(ImGui::GetIO().DeltaTime);

    CUIState& uiState = g_dxHandler->GetUIState();

#if defined(DEBUG_IMGUI_DEMO)
    ImGui::ShowDemoWindow();
#endif

    MainWnd_MenuBar();

    MainWnd_WelcomeBox();

    CDXDrawData* previewDrawData = nullptr;

    const bool shouldPopulateAssetWindows = !inJobAction && !g_assetData.v_assetContainers.empty();

    if (!SHOW_WELCOME_BOX && ImGui::Begin("Asset List", nullptr, ImGuiWindowFlags_MenuBar) && shouldPopulateAssetWindows)
    {
        std::vector<CGlobalAssetData::AssetLookup_t>& pakAssets = FilterConfig->textFilter.IsActive() ? s_filteredAssets : g_assetData.v_assets;

        MainWnd_AssetListMenuBar(pakAssets);

        // OR case if we load a pak and the filter is not cleared yet.
        if (FilterConfig->textFilter.Draw("##Filter", "Filter (incl,-excl)", -1.f) || (s_filteredAssets.empty() && FilterConfig->textFilter.IsActive()))
        {
            s_filteredAssets.clear();
            for (auto& it : g_assetData.v_assets)
            {
                const std::string& assetName = it.m_asset->GetAssetName();

                if (FilterConfig->textFilter.PassFilter(assetName.c_str()))
                    s_filteredAssets.push_back(it);
                else
                {
                    const char* const inputText = FilterConfig->textFilter.inputBuf.c_str();
                    const size_t inputLen = FilterConfig->textFilter.inputBuf.size();

                    char* end;
                    const uint64_t guid = strtoull(inputText, &end, 0);

                    if (end == &inputText[inputLen])
                    {
                        if (guid == RTech::StringToGuid(assetName.c_str()))
                            s_filteredAssets.push_back(it);
                    }
                }
            }

            // Shrink capacity to match new size.
            s_filteredAssets.shrink_to_fit();
        }

        ImGui::PushFont(NULL, 16.f);
        ImGui::TextDisabled("Double-click the name of an asset to export");
        ImGui::PopFont();

        constexpr int numColumns = AssetColumn_t::_AC_COUNT;
        if (ImGui::BeginTable("Assets", numColumns, ImGuiTableFlags_Hideable | ImGuiTableFlags_Sortable | ImGuiTableFlags_BordersInnerV | ImGuiTableFlags_BordersOuterH | ImGuiTableFlags_BordersOuterV | ImGuiTableFlags_ScrollY | ImGuiTableFlags_Resizable | ImGuiTableFlags_Reorderable))
        {
            ImGui::TableSetupColumn("Type", ImGuiTableColumnFlags_WidthFixed, 0, AssetColumn_t::AC_Type);
            ImGui::TableSetupColumn("Name", ImGuiTableColumnFlags_WidthStretch | ImGuiTableColumnFlags_DefaultSort, 0, AssetColumn_t::AC_Name);
            ImGui::TableSetupColumn("GUID", ImGuiTableColumnFlags_WidthFixed | ImGuiTableColumnFlags_DefaultHide, 0, AssetColumn_t::AC_GUID);
            ImGui::TableSetupColumn("File", ImGuiTableColumnFlags_WidthStretch, 0, AssetColumn_t::AC_File);

            ImGui::TableSetupScrollFreeze(0, 1);
            ImGui::TableHeadersRow();

            ImGuiTableSortSpecs* sortSpecs = ImGui::TableGetSortSpecs();

            if (sortSpecs && sortSpecs->SpecsDirty && pakAssets.size() > 1)
            {
                std::sort(pakAssets.begin(), pakAssets.end(), AssetCompare_t(sortSpecs));
                sortSpecs->SpecsDirty = false;
            }

            ImGuiMultiSelectFlags flags = ImGuiMultiSelectFlags_ClearOnEscape | ImGuiMultiSelectFlags_BoxSelect1d;
            ImGuiMultiSelectIO* ms_io = ImGui::BeginMultiSelect(flags, static_cast<int>(s_selectedAssets.size()), static_cast<int>(pakAssets.size()));

            ApplySelectionRequests(ms_io, s_selectedAssets, pakAssets);

            // arbitrary large number that will never happen
            static unsigned int lastSelectionSize = UINT32_MAX;

            ImGuiListClipper clipper;
            clipper.Begin(static_cast<int>(pakAssets.size()));
            while (clipper.Step())
            {
                for (int rowNum = clipper.DisplayStart; rowNum < clipper.DisplayEnd; rowNum++)
                {
                    CAsset* const asset = pakAssets[rowNum].m_asset;

                    // previously this was GUID_pakCRC but realistically the pak filename also works instead of the crc (though it may be slower for lookup?)
                    ImGui::PushID(std::format("{:X}_{}", asset->GetAssetGUID(), rowNum).c_str());

                    ImGui::TableNextRow();

                    auto typeBinding = g_assetData.m_assetTypeBindings.find(asset->GetAssetType());
                    const bool knownAssetType = typeBinding != g_assetData.m_assetTypeBindings.end();

                    if (ImGui::TableSetColumnIndex(AssetColumn_t::AC_Type))
                    {
                        ColouredTextForAssetType(asset);

                        if (typeBinding != g_assetData.m_assetTypeBindings.end())
                            ImGuiExt::Tooltip(typeBinding->second.name);
                    }

                    if (ImGui::TableSetColumnIndex(AssetColumn_t::AC_Name))
                    {
                        const bool isSelected = std::find(s_selectedAssets.begin(), s_selectedAssets.end(), asset) != s_selectedAssets.end();
                        ImGui::SetNextItemSelectionUserData(rowNum);

                        // If this is a known asset type but the user has chosen not to load it, change the asset's name to red
                        // so that they know the asset will not work correctly
                        if(knownAssetType && !typeBinding->second._loadAssetType)
                            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 0.f, 0.f, 1.f));

                        if (ImGui::Selectable(asset->GetAssetName().c_str(), isSelected, ImGuiSelectableFlags_AllowDoubleClick))
                        {
                            if (ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left))
                            {
                                // if the double clicked asset is not in the list, add it
                                // this is a very strange case but the only way you can export multiple assets by double clicking is by
                                // holding shift or ctrl while double clicking, which may cause the hovered asset to be deselected before export
                                if (!isSelected)
                                    s_selectedAssets.insert(s_selectedAssets.end(), asset);

                                CThread(HandlePakAssetExportList, std::move(s_selectedAssets), g_rsxSettings.exportAssetDeps).detach();

                                s_selectedAssets.clear();
                            }
                        }
                        if (knownAssetType && !typeBinding->second._loadAssetType)
                            ImGui::PopStyleColor();

                        // Context menu (right-click)
                        if (ImGui::BeginPopupContextItem())
                        {
                            if (ImGui::Selectable("Copy asset names"))
                            {
                                std::stringstream nameStream;
                                for (auto& it : s_selectedAssets)
                                {
                                    nameStream << it->GetAssetName() << "\n";
                                }

                                ImGui::SetClipboardText(nameStream.str().c_str());

                                ImGui::InsertNotification({ ImGuiToastType::Success, 1000, 100.f, "Copied!", });

                                ImGui::CloseCurrentPopup();
                            }

                            if (ImGui::Selectable("Copy asset guids"))
                            {
                                std::stringstream guidStream;
                                for (auto& it : s_selectedAssets)
                                {
                                    guidStream << "0x" << std::uppercase << std::hex << it->GetAssetGUID() << "\n";
                                }

                                ImGui::SetClipboardText(guidStream.str().c_str());

                                ImGui::InsertNotification({ ImGuiToastType::Success, 1000, 100.f, "Copied!" });

                                ImGui::CloseCurrentPopup();
                            }

                            if (ImGui::Selectable("Export selected assets"))
                            {
                                ImGui::CloseCurrentPopup();

                                CThread(HandlePakAssetExportList, std::move(s_selectedAssets), g_rsxSettings.exportAssetDeps).detach();
                                s_selectedAssets.clear();
                            }

                            ImGui::EndPopup();
                        }
                    }

                    if (ImGui::TableSetColumnIndex(AssetColumn_t::AC_GUID))
                        ImGui::Text("%016llX", asset->GetAssetGUID());

                    if (ImGui::TableSetColumnIndex(AssetColumn_t::AC_File))
                        ImGui::TextUnformatted(asset->GetContainerFileName().c_str());

                    ImGui::PopID();
                }
            }
            ms_io = ImGui::EndMultiSelect();

            ApplySelectionRequests(ms_io, s_selectedAssets, pakAssets);

            ImGui::EndTable();
        }
    }
    if (!SHOW_WELCOME_BOX) ImGui::End();

    // This window must be drawn before "Scene", as the scene relies on previewDrawData already being set from here
    if (!SHOW_WELCOME_BOX && ImGui::Begin("Asset Info", nullptr, ImGuiWindowFlags_MenuBar) && shouldPopulateAssetWindows)
    {
        CAsset* const firstAsset = s_selectedAssets.empty() ? nullptr : *s_selectedAssets.begin();
        if (ImGui::BeginMenuBar())
        {
            if (ImGui::BeginMenu("File"))
            {
                if (ImGui::BeginMenu("Export"))
                {
                    // File > Export > Quick Export
                    // Option to "quickly" export the asset to the exported_files directory
                    // in the format defined by the "Export Options" menu.
                    if (ImGui::MenuItem("Quick Export"))
                        CThread(HandleExportBindingForAsset, std::move(firstAsset), g_rsxSettings.exportAssetDeps).detach();

                    ImGui::EndMenu();
                }
                ImGui::EndMenu();
            }
            ImGui::EndMenuBar();
        }

        const std::string assetName = !firstAsset ? "(none)" : firstAsset->GetAssetName();
        const std::string assetGuidStr = !firstAsset ? "(none)" : std::format("{:X}", firstAsset->GetAssetGUID());

        ImGuiExt::ConstTextInputLeft("Name", assetName.c_str(), 50);
        ImGuiExt::ConstTextInputLeft("GUID", assetGuidStr.c_str(), 50);

        // Dependency information only exists for PAK assets
        if (firstAsset && firstAsset->GetAssetContainerType() == CAsset::ContainerType::PAK)
        {
            const PakAsset_t* const pakAsset = static_cast<CPakAsset*>(firstAsset)->data();

            ImGui::Text("Number of Dependencies: %hu", pakAsset->dependenciesCount);
            ImGui::SameLine();
            ImGuiExt::HelpMarker("This is the number of assets in the same .rpak file as this asset that are required by this asset");

            ImGui::Text("Number of Dependent Assets: %hu", pakAsset->dependentsCount);
            ImGui::SameLine();
            ImGuiExt::HelpMarker("This is the number of assets in the same .rpak file as this asset that use this asset");

            PreviewWnd_AssetDepsTbl(firstAsset);
        }

        ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 5.f);

        ImGui::Separator();

        ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 5.f);
        ImGui::Dummy(ImVec2(0, 0)); // Required just in case there are no ImGui elements in the below preview call
        if (firstAsset)
        {
            const uint32_t type = firstAsset->GetAssetType();
            if (auto it = g_assetData.m_assetTypeBindings.find(type); it != g_assetData.m_assetTypeBindings.end())
            {
                if (it->second._loadAssetType)
                {
                    if (it->second.previewFunc)
                    {
                        // First frame is a special case, we wanna reset some settings for the preview function.
                        const bool firstFrameForAsset = firstAsset != s_prevRenderInfoAsset;

                        previewDrawData = reinterpret_cast<CDXDrawData*>(it->second.previewFunc(static_cast<CPakAsset*>(firstAsset), firstFrameForAsset));
                        s_prevRenderInfoAsset = firstAsset;

                    }
                    else ImGui::Text("Asset type '%s' does not currently support Asset Preview.", fourCCToString(type).c_str());
                }
                else ImGui::Text("This asset type was not loaded because of your current settings.\nYou can change this by visiting Settings > %s (%s) > Load Asset Type", it->second.name, fourCCToString(type, true).c_str());
            }
            else ImGui::Text("Asset type '%s' is not currently supported.", fourCCToString(type).c_str());
        }
        else ImGui::TextUnformatted("No asset selected.");
    }
    if(!SHOW_WELCOME_BOX) ImGui::End();

    // The "scene" preview window must always be in the center.
    // Setting NoMove seems to be the best way to stop it from being undocked
    if (!SHOW_WELCOME_BOX && ImGui::Begin("Scene", nullptr, ImGuiWindowFlags_NoMove))
    {
        const ImVec2 avail = ImGui::GetContentRegionAvail();

        if (avail != s_previousAvailableSizeForPreview || !g_dxHandler->GetPreviewRTV())
        {
            g_dxHandler->CleanupForPreviewResize();
            g_dxHandler->CreateViewForSceneWindow(static_cast<uint16_t>(avail.x), static_cast<uint16_t>(avail.y));
        }

        s_previousAvailableSizeForPreview = avail;

        if (previewDrawData)
        {
            switch (previewDrawData->dataType)
            {
            case CDXDrawData::DrawDataType_e::MODEL:
                Preview_Model(previewDrawData, ImGui::GetIO().DeltaTime);
                break;
            case CDXDrawData::DrawDataType_e::TEXTURE:
                Preview_Texture(previewDrawData, ImGui::GetIO().DeltaTime);
                break;
            default:
            {
                assertm(0, "Invalid preview draw data type. Expected either MODEL or TEXTURE");
                break;
            }
            }
        }
    }
    if (!SHOW_WELCOME_BOX) ImGui::End();

    if (uiState.settingsWindowVisible)
        SettingsWnd_Draw(&uiState);

    if (!SHOW_WELCOME_BOX && uiState.itemflavWindowVisible)
        ItemflavWnd_Draw(&uiState);

    if (uiState.logWindowVisible)
        LogWnd_Draw(&uiState);

    g_pImGuiHandler->HandleProgressBar();

    ImGui::RenderNotifications();

    ImGui::Render();

    if (ImGui::GetIO().ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
    {
        ImGui::UpdatePlatformWindows();
        ImGui::RenderPlatformWindowsDefault();
    }

    ID3D11DeviceContext* const ctx = g_dxHandler->GetDeviceContext();

    ID3D11RenderTargetView* const mainView = g_dxHandler->GetMainView();
    static constexpr float clear_color_with_alpha[4] = { 0.01f, 0.01f, 0.01f, 1.00f };

    ctx->OMSetRenderTargets(1, &mainView, g_dxHandler->GetDepthStencilView());
    ctx->ClearRenderTargetView(mainView, clear_color_with_alpha);
    ctx->ClearDepthStencilView(g_dxHandler->GetDepthStencilView(), D3D11_CLEAR_DEPTH, 1, 0);

    LONG width = 0ul;
    LONG height = 0ul;
    g_dxHandler->GetWindowSize(&width, &height);

    const D3D11_VIEWPORT vp = {
        0, 0,
        static_cast<float>(width),
        static_cast<float>(height),
        0, 1
    };

    ctx->RSSetViewports(1u, &vp);

    ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());

    g_dxHandler->GetSwapChain()->Present(1u, 0u);
}
