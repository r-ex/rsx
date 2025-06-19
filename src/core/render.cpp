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

extern CDXParentHandler* g_dxHandler;
extern std::atomic<uint32_t> maxConcurrentThreads;

ExportSettings_t g_ExportSettings{ .previewedSkinIndex = 0, .exportNormalRecalcSetting = eNormalExportRecalc::NML_RECALC_NONE, .exportTextureNameSetting = eTextureExportName::TXTR_NAME_TEXT,
    .exportPathsFull = false, .exportAssetDeps = false, .exportRigSequences = true, .exportModelSkin = false, .exportMaterialTextures = true, .exportPhysicsContentsFilter = static_cast<uint32_t>(TRACE_MASK_ALL) };
PreviewSettings_t g_PreviewSettings { .previewCullDistance = PREVIEW_CULL_DEFAULT, .previewMovementSpeed = PREVIEW_SPEED_DEFAULT };

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


// TODO: Add shortcut handling for copying asset names with CTRL+C when assets are selected and asset list is focused.
//void HandleImGuiShortcuts()
//{
//
//}

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
    default:
    {
        ImGui::Text("unimpl");
        break;
    }
    }

    if (colouredText)
        ImGui::PopStyleColor();
}

void CreatePakAssetDependenciesTable(CAsset* asset)
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

                if (ImGui::TableSetColumnIndex(0))
                {
                    ImGui::AlignTextToFramePadding();
                    if (depAsset)
                        ColouredTextForAssetType(depAsset);
                    else
                        ImGui::TextUnformatted("n/a");
                }

                if (ImGui::TableSetColumnIndex(1))
                {
                    ImGui::AlignTextToFramePadding();
                    if (depAsset)
                        ImGui::TextUnformatted(depAsset->GetAssetName().c_str());
                    else
                        ImGui::Text("%016llX", guid.guid);
                }

                if (ImGui::TableSetColumnIndex(2))
                {
                    ImGui::AlignTextToFramePadding();
                    if (!depAsset)
                        ImGui::TextUnformatted("n/a");
                    else
                        ImGui::TextUnformatted(depAsset->GetContainerFileName().c_str());
                }

                if (ImGui::TableSetColumnIndex(3))
                {
                    ImGui::AlignTextToFramePadding();
                    if (depAsset)
                    {
                        if (depAsset->GetExportedStatus())
                        {
                            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.f, 1.f, 1.f, 1.f));
                            ImGui::TextUnformatted("Exported");
                            ImGui::PopStyleColor();
                        }
                        else
                        {
                            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.f, 1.f, 0.f, 1.f));
                            ImGui::TextUnformatted("Loaded");
                            ImGui::PopStyleColor();
                        }
                    }
                    else
                    {
                        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.f, 0.f, 0.f, 1.f));
                        ImGui::TextUnformatted("Not Loaded");
                        ImGui::PopStyleColor();
                    }
                }

                if (ImGui::TableSetColumnIndex(4))
                {
                    ImGui::AlignTextToFramePadding();
                    if (!depAsset)
                        ImGui::BeginDisabled();

                    if (ImGui::Button("Export"))
                        CThread(HandleExportBindingForAsset, depAsset, g_ExportSettings.exportAssetDeps).detach();

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

void HandleRenderFrame()
{
    ImGui_ImplDX11_NewFrame();
    ImGui_ImplWin32_NewFrame();
    ImGui::NewFrame();

    // while ImGui is using keyboard input, we should not accept any keyboard input, but we should also clear all  
    // existing key states, as otherwise we can end up moving forever (until ImGui loses focus) in model preview if
    // the movement keys are held when ImGui starts capturing keyboard
    if (ImGui::GetIO().WantCaptureKeyboard)
    {
        g_pInput->ClearKeyStates();

        //HandleImGuiShortcuts();
    }

    if (g_pInput->mouseCaptured)
        g_pInput->Frame(ImGui::GetIO().DeltaTime);

    g_dxHandler->GetCamera()->Move(ImGui::GetIO().DeltaTime);

    CUIState& uiState = g_dxHandler->GetUIState();

    //ImGui::ShowDemoWindow();

    static std::deque<CAsset*> selectedAssets;
    static std::vector<CGlobalAssetData::AssetLookup_t> filteredAssets;
    static CAsset* prevRenderInfoAsset = nullptr;

    CDXDrawData* previewDrawData = nullptr;
    if (ImGui::BeginMainMenuBar())
    {

        if (ImGui::BeginMenu("File"))
        {
            if (ImGui::MenuItem("Open"))
            {
                if (!inJobAction)
                {
                    // Reset selected asset to avoid crash.
                    selectedAssets.clear();
                    filteredAssets.clear();
                    prevRenderInfoAsset = nullptr;
                    g_assetData.ClearAssetData();

                    // We kinda leak the thread here but it's okay, we want it to keep executing.
                    CThread(HandleOpenFileDialog, g_dxHandler->GetWindowHandle()).detach();
                }
            }

            if (ImGui::MenuItem("Unload Files"))
            {
                if (!inJobAction)
                {
                    selectedAssets.clear();
                    filteredAssets.clear();
                    prevRenderInfoAsset = nullptr;
                    g_assetData.ClearAssetData();
                }
            }

            ImGui::EndMenu();
        }

        if (ImGui::BeginMenu("Edit"))
        {
            //if (ImGui::MenuItem("Copy Asset Names", "CTRL+C"))
            //{
            // 
            //}

            if (ImGui::MenuItem("Settings"))
                uiState.ShowSettingsWindow(true);

            ImGui::EndMenu();
        }

#if _DEBUG
        IMGUI_RIGHT_ALIGN_FOR_TEXT("Avg 1.000 ms/frame (100.0 FPS)"); // [rexx]: i hate this actually

        ImGuiIO& io = ImGui::GetIO();
        ImGui::Text("Avg %.3f ms/frame (%.1f FPS)", 1000.0f / io.Framerate, io.Framerate);
#endif
        ImGui::EndMainMenuBar();
    }

    // [rexx]: yes, these branches could be structured a bit better
    //         but otherwise we get very deep indentation which looks ugly
    if (!inJobAction && g_assetData.v_assetContainers.empty())
    {
        ImGui::SetNextWindowPos(ImGui::GetMainViewport()->GetCenter(), ImGuiCond_Always, ImVec2(0.5f, 0.5f));

        ImGui::SetNextWindowSize(ImVec2(600, 0), ImGuiCond_Always);

        if (ImGui::Begin("reSource Xtractor", nullptr, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse))
        {
            ImGui::PushTextWrapPos();
            ImGui::TextUnformatted(RSX_WELCOME_MESSAGE);
            ImGui::PopTextWrapPos();

            ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 5.f);

            if (ImGui::Button("Open File..."))
            {
                // Reset selected asset to avoid crash.
                selectedAssets.clear();
                filteredAssets.clear();
                prevRenderInfoAsset = nullptr;
                g_assetData.ClearAssetData();

                // We kinda leak the thread here but it's okay, we want it to keep executing.
                CThread(HandleOpenFileDialog, g_dxHandler->GetWindowHandle()).detach();
            }

            ImGui::End();
        }
    }


    // Only render window if we have a pak loaded and if we aren't currently in pakload.
    if (!inJobAction && !g_assetData.v_assetContainers.empty())
    {
        ImGui::SetNextWindowSize(ImVec2(800, 400), ImGuiCond_FirstUseEver);
        if (ImGui::Begin("Asset List", nullptr, ImGuiWindowFlags_MenuBar))
        {
            std::vector<CGlobalAssetData::AssetLookup_t>& pakAssets = FilterConfig->textFilter.IsActive() ? filteredAssets : g_assetData.v_assets;

            if (ImGui::BeginMenuBar())
            {
                if (ImGui::BeginMenu("Export"))
                {
                    const bool multipleAssetsSelected = selectedAssets.size() > 1;

                    if (ImGui::Selectable("Export selected"))
                    {
                        if (!selectedAssets.empty())
                        {
                            std::deque<CAsset*> cpyAssets;
                            cpyAssets.insert(cpyAssets.end(), selectedAssets.begin(), selectedAssets.end());
                            CThread(HandlePakAssetExportList, std::move(cpyAssets), g_ExportSettings.exportAssetDeps).detach();
                            selectedAssets.clear();
                        }
                    }

                    // Option is only valid if one asset is selected
                    if (ImGui::Selectable("Export all for selected type", false, multipleAssetsSelected ? ImGuiSelectableFlags_Disabled : 0))
                    {
                        if (selectedAssets.size() == 1)
                        {
                            const uint32_t desiredType = selectedAssets[0]->GetAssetType();
                            auto allAssetsOfDesiredType = pakAssets | std::ranges::views::filter([desiredType](const CGlobalAssetData::AssetLookup_t& a)
                                {
                                    return a.m_asset->GetAssetType() == desiredType;
                                });

                            std::vector<CGlobalAssetData::AssetLookup_t> allAssets(allAssetsOfDesiredType.begin(), allAssetsOfDesiredType.end());
                            CThread(HandleExportSelectedAssetType, std::move(allAssets), g_ExportSettings.exportAssetDeps).detach();
                            selectedAssets.clear();
                        }
                    }

                    // Option is only valid if one asset is selected
                    // TODO: rename to "for selected file" to allow for all audio assets to be exported?
                    if (ImGui::Selectable("Export all for selected pak", false, multipleAssetsSelected ? ImGuiSelectableFlags_Disabled : 0))
                    {
                        if (selectedAssets.size() == 1 && selectedAssets[0]->GetAssetContainerType() == CAsset::ContainerType::PAK)
                        {
                            const CPakFile* desiredPak = selectedAssets[0]->GetContainerFile<const CPakFile>();
                            auto allAssetsOfDesiredType = pakAssets | std::ranges::views::filter([desiredPak](const CGlobalAssetData::AssetLookup_t& a)
                                {
                                    return a.m_asset->GetAssetContainerType() == CAsset::ContainerType::PAK && a.m_asset->GetContainerFile<const CPakFile>() == desiredPak;
                                });

                            std::vector<CGlobalAssetData::AssetLookup_t> allAssets(allAssetsOfDesiredType.begin(), allAssetsOfDesiredType.end());
                            CThread(HandleExportSelectedAssetType, std::move(allAssets), g_ExportSettings.exportAssetDeps).detach();
                            selectedAssets.clear();
                        }
                    }

                    // Option is only valid if one asset is selected
                    // TODO: rename to "for selected file" to allow for all audio assets to be exported?
                    if (ImGui::Selectable("Export all for selected pak and type", false, multipleAssetsSelected ? ImGuiSelectableFlags_Disabled : 0))
                    {
                        if (selectedAssets.size() == 1 && selectedAssets[0]->GetAssetContainerType() == CAsset::ContainerType::PAK)
                        {
                            const CPakFile* desiredPak = selectedAssets[0]->GetContainerFile<const CPakFile>();
                            const uint32_t desiredType = selectedAssets[0]->GetAssetType();
                            auto allAssetsOfDesiredType = pakAssets | std::ranges::views::filter([desiredPak, desiredType](const CGlobalAssetData::AssetLookup_t& a)
                                {
                                    return a.m_asset->GetAssetContainerType() == CAsset::ContainerType::PAK
                                        && a.m_asset->GetContainerFile<CPakFile>() == desiredPak
                                        && a.m_asset->GetAssetType() == desiredType;
                                });

                            std::vector<CGlobalAssetData::AssetLookup_t> allAssets(allAssetsOfDesiredType.begin(), allAssetsOfDesiredType.end());
                            CThread(HandleExportSelectedAssetType, std::move(allAssets), g_ExportSettings.exportAssetDeps).detach();
                            selectedAssets.clear();
                        }
                    }

                    if (ImGui::Selectable("Export all"))
                    {
                        CThread(HandleExportAllPakAssets, &pakAssets, g_ExportSettings.exportAssetDeps).detach();
                    }

                    // Exports the names of all assets in the currently shown filtered asset list (i.e., search results)
                    if (ImGui::Selectable("Export list of assets"))
                    {
                        CThread(HandleListExportPakAssets, g_dxHandler->GetWindowHandle(), &pakAssets).detach();
                    }

                    ImGui::EndMenu();
                }
                ImGui::EndMenuBar();
            }

            // OR case if we load a pak and the filter is not cleared yet.
            if (FilterConfig->textFilter.Draw("##Filter", -1.f) || (filteredAssets.empty() && FilterConfig->textFilter.IsActive()))
            {
                filteredAssets.clear();
                for (auto& it : g_assetData.v_assets)
                {
                    const std::string& assetName = it.m_asset->GetAssetName();

                    if (FilterConfig->textFilter.PassFilter(assetName.c_str()))
                        filteredAssets.push_back(it);
                    else
                    {
                        const char* const inputText = FilterConfig->textFilter.InputBuf;
                        const size_t inputLen = strlen(inputText);

                        char* end;
                        const uint64_t guid = strtoull(inputText, &end, 0);

                        if (end == &inputText[inputLen])
                        {
                            if (guid == RTech::StringToGuid(assetName.c_str()))
                                filteredAssets.push_back(it);
                        }
                    }
                }

                // Shrink capacity to match new size.
                filteredAssets.shrink_to_fit();
            }

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

                ImGuiListClipper clipper;
                clipper.Begin(static_cast<int>(pakAssets.size()));
                while (clipper.Step())
                {
                    for (int rowNum = clipper.DisplayStart; rowNum < clipper.DisplayEnd; rowNum++)
                    {
                        CAsset* const asset = pakAssets[rowNum].m_asset;

                        // temp just to deal with compile issue while the class is modified

                        // previously this was GUID_pakCRC but realistically the pak filename also works instead of the crc (though it may be slower for lookup?)
                        ImGui::PushID(std::format("{:X}_{}", asset->GetAssetGUID(), asset->GetContainerFileName()).c_str());

                        ImGui::TableNextRow();

                        if (ImGui::TableSetColumnIndex(AssetColumn_t::AC_Type))
                        {
                            ColouredTextForAssetType(asset);
                        }

                        if (ImGui::TableSetColumnIndex(AssetColumn_t::AC_Name))
                        {
                            const bool isSelected = std::find(selectedAssets.begin(), selectedAssets.end(), asset) != selectedAssets.end();
                            if (ImGui::Selectable(asset->GetAssetName().c_str(), isSelected, ImGuiSelectableFlags_AllowDoubleClick))
                            {
                                if (ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left))
                                {
                                    selectedAssets.clear();
                                    selectedAssets.insert(selectedAssets.end(), asset);

                                    CThread(HandleExportBindingForAsset, std::move(*selectedAssets.begin()), g_ExportSettings.exportAssetDeps).detach();
                                }
                                else
                                {
                                    if (!ImGui::GetIO().KeyCtrl)
                                    {
                                        selectedAssets.clear();
                                    }
                                    selectedAssets.insert(selectedAssets.end(), asset);
                                }
                            }
                        }

                        if (ImGui::TableSetColumnIndex(AssetColumn_t::AC_GUID))
                            ImGui::Text("%016llX", asset->GetAssetGUID());

                        if (ImGui::TableSetColumnIndex(AssetColumn_t::AC_File))
                            ImGui::TextUnformatted(asset->GetContainerFileName().c_str());

                        ImGui::PopID();
                    }
                }

                ImGui::EndTable();
            }
        }
        ImGui::End();

        ImGui::SetNextWindowSize(ImVec2(800, 400), ImGuiCond_FirstUseEver);
        ImGui::SetNextWindowPos(ImGui::GetMainViewport()->GetCenter(), ImGuiCond_FirstUseEver, ImVec2(0.5f, 0.5f));

        CAsset* const firstAsset = selectedAssets.empty() ? nullptr : *selectedAssets.begin();
        
        if (ImGui::Begin("Asset Info", nullptr, ImGuiWindowFlags_MenuBar))
        {
            if (ImGui::BeginMenuBar())
            {
                if (ImGui::BeginMenu("File"))
                {
                    if (ImGui::BeginMenu("Export"))
                    {
                        if (ImGui::MenuItem("Quick Export"))
                        {
                            // Option to "quickly" export the asset to the exported_files directory
                            // in the format defined by the "Export Options" menu.

                            CThread(HandleExportBindingForAsset, std::move(firstAsset), g_ExportSettings.exportAssetDeps).detach();
                        }

                        if (ImGui::MenuItem("Export as..."))
                        {
                            // Option to export the selected asset as a different format to the format
                            // selected in "Export Options" to a different location to usual.
                        }
                        ImGui::EndMenu();
                    }
                    //ShowExampleMenuFile();
                    ImGui::EndMenu();
                }
                //if (ImGui::BeginMenu("Edit"))
                //{
                //    if (ImGui::MenuItem("Undo", "CTRL+Z")) {}
                //    if (ImGui::MenuItem("Redo", "CTRL+Y", false, false)) {}  // Disabled item
                //    ImGui::Separator();
                //    if (ImGui::MenuItem("Cut", "CTRL+X")) {}
                //    if (ImGui::MenuItem("Copy", "CTRL+C")) {}
                //    if (ImGui::MenuItem("Paste", "CTRL+V")) {}
                //    ImGui::EndMenu();
                //}
                ImGui::EndMenuBar();
            }

            const std::string assetName = !firstAsset ? "(none)" : firstAsset->GetAssetName(); // std::format("{} ({:X})", , firstAsset->data()->guid);
            const std::string assetGuidStr = !firstAsset ? "(none)" : std::format("{:X}", firstAsset->GetAssetGUID());

            ImGuiConstTextInputLeft("Asset Name", assetName.c_str(), 70);
            ImGuiConstTextInputLeft("Asset GUID", assetGuidStr.c_str(), 70);

            if (firstAsset && firstAsset->GetAssetContainerType() == CAsset::ContainerType::PAK)
            {
                const PakAsset_t* const pakAsset = static_cast<CPakAsset*>(firstAsset)->data();

                ImGui::Text("remainingDependencyCount: %hu", pakAsset->remainingDependencyCount);
                ImGui::Text("dependenciesCount: %hu", pakAsset->dependenciesCount);

                ImGui::Text("dependenciesIndex: %u", pakAsset->dependenciesIndex);
                ImGui::Text("dependentsIndex: %u", pakAsset->dependentsIndex);

                CreatePakAssetDependenciesTable(firstAsset);
            }

            ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 5.f);

            ImGui::Separator();

            ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 5.f);

            if (firstAsset)
            {

                const uint32_t type = firstAsset->GetAssetType();
                if (auto it = g_assetData.m_assetTypeBindings.find(type); it != g_assetData.m_assetTypeBindings.end())
                {
                    if (it->second.previewFunc)
                    {
                        // First frame is a special case, we wanna reset some settings for the preview function.
                        const bool firstFrameForAsset = firstAsset != prevRenderInfoAsset;

                        previewDrawData = reinterpret_cast<CDXDrawData*>(it->second.previewFunc(static_cast<CPakAsset*>(firstAsset), firstFrameForAsset));
                        prevRenderInfoAsset = firstAsset;
                    }
                    else
                    {
                        ImGui::Text("Asset type '%s' does not currently support Asset Preview.", fourCCToString(type).c_str());
                    }
                }
                else
                {
                    ImGui::Text("Asset type '%s' is not currently supported.", fourCCToString(type).c_str());
                }
            }
            else
            {
                ImGui::TextUnformatted("No asset selected.");
            }
        }

        ImGui::End();
    }

    constexpr uint32_t minThreads = 1u;

    if (uiState.settingsWindowVisible)
    {
        ImGui::SetNextWindowSize(ImVec2(0.f, 0.f), ImGuiCond_Always);
        if (ImGui::Begin("Settings", &uiState.settingsWindowVisible, ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize))
        {
            // ===============================================================================================================
            ImGui::SeparatorText("Export");

            ImGui::Checkbox("Export full asset paths", &g_ExportSettings.exportPathsFull);
            ImGui::SameLine();
            g_pImGuiHandler->HelpMarker("Enables exporting of assets to their full path within the export directory, as shown by the listed asset names.\nWhen disabled, assets will be exported into the root-level of a folder named after the asset's type (e.g. \"material/\",\"ui_image/\").");
            
            ImGui::Checkbox("Export asset dependencies", &g_ExportSettings.exportAssetDeps);
            ImGui::SameLine();
            g_pImGuiHandler->HelpMarker("Enables exporting of all dependencies that are associated with any asset that is being exported.");

            ImGui::Checkbox("Export AnimRig sequences", &g_ExportSettings.exportRigSequences);
            ImGui::SameLine();
            g_pImGuiHandler->HelpMarker("Enables exporting of all animation sequences that are associated with any AnimRig (and MDL) asset that is being exported.");

            ImGui::Checkbox("Export Model Skin", &g_ExportSettings.exportModelSkin);
            ImGui::SameLine();
            g_pImGuiHandler->HelpMarker("Enables exporting a model with the previewed skin.");

            ImGui::Checkbox("Export Material Textures", &g_ExportSettings.exportMaterialTextures);
            ImGui::SameLine();
            g_pImGuiHandler->HelpMarker("Enables exporting of all textures that are associated with any material asset that is being exported.");

            ImGui::Combo("Material Texture Names", reinterpret_cast<int*>(&g_ExportSettings.exportTextureNameSetting), s_TextureExportNameSetting, static_cast<int>(ARRAYSIZE(s_TextureExportNameSetting)));
            ImGui::SameLine();
            g_pImGuiHandler->HelpMarker("Naming scheme for exporting textures via materials options are as follows:\nGUID: exports only using the asset's GUID as a name.\nReal: exports texture using a real name (asset name or guid if no name).\nText: exports the texture with a text name always, generating one if there is none provided.\nSemantic: exports with a generated name all the time, useful for models.");

            ImGui::Combo("Normal Recalc", reinterpret_cast<int*>(&g_ExportSettings.exportNormalRecalcSetting), s_NormalExportRecalcSetting, static_cast<int>(ARRAYSIZE(s_NormalExportRecalcSetting)));
            ImGui::SameLine();
            g_pImGuiHandler->HelpMarker("None: exports the normal as it is stored.\nDirectX: exports with a generated blue channel.\nOpenGL: exports with a generated blue channel and inverts the green channel.");

            ImGui::InputInt("Physics contents filter", reinterpret_cast<int*>(&g_ExportSettings.exportPhysicsContentsFilter), 1, 100, ImGuiInputTextFlags_CharsHexadecimal);
            ImGui::SameLine();
            g_pImGuiHandler->HelpMarker("Filter physics meshes in or out based on selected contents.");

            ImGui::Checkbox("Physics contents filter exclusive", &g_ExportSettings.exportPhysicsFilterExclusive);
            ImGui::SameLine();
            g_pImGuiHandler->HelpMarker("Exclude physics meshes containing any of the specified contents.");

            ImGui::Checkbox("Physics contents filter require all", &g_ExportSettings.exportPhysicsFilterAND);
            ImGui::SameLine();
            g_pImGuiHandler->HelpMarker("Filter only physics meshes containing all specified contents.");

            // ===============================================================================================================
            ImGui::SeparatorText("Threads");

            ImGui::SliderScalar("Parse Threads", ImGuiDataType_U32, &UtilsConfig->parseThreadCount, &minThreads, reinterpret_cast<int*>(&maxConcurrentThreads));
            ImGui::SameLine();
            g_pImGuiHandler->HelpMarker("The number of CPU threads that will be used for loading files.\n\nIn general, the higher the number, the faster RSX will be able to load the selected files.");

            ImGui::SliderScalar("Export Threads", ImGuiDataType_U32, &UtilsConfig->exportThreadCount, &minThreads, reinterpret_cast<int*>(&maxConcurrentThreads));
            ImGui::SameLine();
            g_pImGuiHandler->HelpMarker("The number of CPU threads that will be used for exporting assets.\n\nA higher number of threads will usually make RSX export assets more quickly, however the increased disk usage may cause decreased performance.");

            // ===============================================================================================================
            ImGui::SeparatorText("Preview");

            ImGui::SliderFloat("Cull Distance", &g_PreviewSettings.previewCullDistance, PREVIEW_CULL_MIN, PREVIEW_CULL_MAX);
            ImGui::SameLine();
            g_pImGuiHandler->HelpMarker("Distance at which render of 3D objects will stop. Note: only updated on startup.\n"); // todo: recreate projection matrix ?

            ImGui::SliderFloat("Movement Speed", &g_PreviewSettings.previewMovementSpeed, PREVIEW_SPEED_MIN, PREVIEW_SPEED_MAX);
            ImGui::SameLine();
            g_pImGuiHandler->HelpMarker("Speed at which the camera moves through the 3D scene.\n");
            
            // ===============================================================================================================
            ImGui::SeparatorText("Export Formats");

            for (auto& it : g_assetData.m_assetTypeBindings)
            {
                if (it.second.e.exportSettingArr)
                {
                    ImGui::Combo(fourCCToString(it.first).c_str(), &it.second.e.exportSetting, it.second.e.exportSettingArr, static_cast<int>(it.second.e.exportSettingArrSize));
                }
            }

            ImGui::End();
        }
    }

    g_pImGuiHandler->HandleProgressBar();

    ImDrawList* bgDrawList = ImGui::GetBackgroundDrawList();
    if (previewDrawData)
    {
        //const CDXCamera* camera = g_dxHandler->GetCamera();
        //bgDrawList->AddText(
        //    ImGui::GetFont(), 15.f,
        //    ImVec2(10, 20), 0xFFFFFFFF,
        //    std::format("pos: {:.3f} {:.3f} {:.3f}\nrot: {:.3f} {:.3f} {:.3f}",
        //        camera->position.x, camera->position.y, camera->position.z,
        //        camera->rotation.x, camera->rotation.y, camera->rotation.z
        //    ).c_str());
        //bgDrawList->AddText(ImGui::GetFont(), 20.f, ImVec2(10, 50), 0xFF0000FF, previewDrawData->modelName);

        bgDrawList->AddText(
            ImGui::GetFont(), 15.f,
            ImVec2(10, 20), 0xFFFFFFFF,
            std::format("right click to toggle preview mouse control").c_str());
    }

    ImGui::Render();

    ID3D11RenderTargetView* const mainView = g_dxHandler->GetMainView();
    static constexpr float clear_color_with_alpha[4] = { 0.01f, 0.01f, 0.01f, 1.00f };
    g_dxHandler->GetDeviceContext()->OMSetRenderTargets(1, &mainView, g_dxHandler->GetDepthStencilView());
    g_dxHandler->GetDeviceContext()->ClearRenderTargetView(mainView, clear_color_with_alpha);
    g_dxHandler->GetDeviceContext()->ClearDepthStencilView(g_dxHandler->GetDepthStencilView(), D3D11_CLEAR_DEPTH, 1, 0);

    LONG width = 0ul;
    LONG height = 0ul;
    g_dxHandler->GetWindowSize(&width, &height);

    const D3D11_VIEWPORT vp = {
        0, 0,
        static_cast<float>(width),
        static_cast<float>(height),
        0, 1
    };
    ID3D11Device* const device = g_dxHandler->GetDevice();
    ID3D11DeviceContext* const ctx = g_dxHandler->GetDeviceContext();

#if !defined(ADVANCED_MODEL_PREVIEW)
    UNUSED(device);
#endif

    g_dxHandler->GetDeviceContext()->RSSetViewports(1u, &vp);
    g_dxHandler->GetDeviceContext()->RSSetState(g_dxHandler->GetRasterizerState());
    g_dxHandler->GetDeviceContext()->OMSetDepthStencilState(g_dxHandler->GetDepthStencilState(), 1u);

#if defined(ADVANCED_MODEL_PREVIEW)
    g_dxHandler->GetCamera()->CommitCameraDataBufferUpdates();

    CDXScene& scene = g_dxHandler->GetScene();

    if (scene.globalLights.size() == 0)
    {
        HardwareLight& light = scene.globalLights.emplace_back();

        light.pos = { 0, 10.f, 0 };
        light.rcpMaxRadius = 1 / 100.f;
        light.rcpMaxRadiusSq = 1 / (light.rcpMaxRadius * light.rcpMaxRadius);
        light.attenLinear = -1.95238f;
        light.attenQuadratic = 0.95238f;
        light.specularIntensity = 1.f;
        light.color = { 1.f, 1.f, 1.f };
    }

    if (scene.NeedsLightingUpdate())
        g_dxHandler->GetScene().CreateOrUpdateLights(device, ctx);
#endif

    if (previewDrawData)
    {
        const CDXCamera* camera = g_dxHandler->GetCamera();

        const CShader* const vertexShader = g_dxHandler->GetShaderManager()->LoadShaderFromString("shaders/model_vs", s_PreviewVertexShader, eShaderType::Vertex);
        const CShader* const pixelShader = g_dxHandler->GetShaderManager()->LoadShaderFromString("shaders/model_ps", s_PreviewPixelShader, eShaderType::Pixel);

        if (vertexShader && pixelShader)
        {
            ctx->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

            ctx->IASetInputLayout(vertexShader->GetInputLayout());

            ctx->VSSetShader(vertexShader->Get<ID3D11VertexShader>(), nullptr, 0u);
            ctx->PSSetShader(pixelShader->Get<ID3D11PixelShader>(), nullptr, 0u);

            assertm(previewDrawData->transformsBuffer, "uh oh something very bad happened!!!!!!");
            ID3D11Buffer* const transformsBuffer = previewDrawData->transformsBuffer;

            ctx->VSSetConstantBuffers(0u, 1u, &transformsBuffer);

            UINT offset = 0u;

            for (size_t i = 0; i < previewDrawData->meshBuffers.size(); ++i)
            {
                const DXMeshDrawData_t& meshDrawData = previewDrawData->meshBuffers[i];

                // if this mesh is not visible, don't draw it!
                if (!meshDrawData.visible)
                    continue;

                ID3D11Buffer* sharedConstBuffers[] = {
                    camera->bufCommonPerCamera,
                    previewDrawData->transformsBuffer,
                };

#if defined(ADVANCED_MODEL_PREVIEW)
                const bool useAdvancedModelPreview = meshDrawData.vertexShader && meshDrawData.pixelShader;
#else
                constexpr bool useAdvancedModelPreview = false;
#endif

                // if we have a custom vertex shader and/or pixel shader for this mesh from the draw data, use it
                // otherwise we can fall back on the base shaders
                if (useAdvancedModelPreview)
                {
                    ctx->VSSetShader(meshDrawData.vertexShader, nullptr, 0u);
                    ctx->IASetInputLayout(meshDrawData.inputLayout);

                    ctx->VSSetConstantBuffers(2u, ARRSIZE(sharedConstBuffers), sharedConstBuffers);

                    ctx->VSSetShaderResources(60u, 1u, &previewDrawData->boneMatrixSRV);
                    ctx->VSSetShaderResources(62u, 1u, &previewDrawData->boneMatrixSRV);

                    ctx->IASetVertexBuffers(0u, 1u, &meshDrawData.vertexBuffer, &meshDrawData.vertexStride, &offset);
                }
                else
                {
                    constexpr UINT stride = sizeof(Vertex_t);
                    //ctx->VSSetConstantBuffers(2u, 1u, &camera->bufCommonPerCamera);
                    ctx->IASetVertexBuffers(0u, 1u, &meshDrawData.vertexBuffer, &stride, &offset);
                }

                ID3D11SamplerState* const samplerState = g_dxHandler->GetSamplerState();

                if (useAdvancedModelPreview)
                {
                    ctx->PSSetShader(meshDrawData.pixelShader, nullptr, 0u);

                    ID3D11SamplerState* samplers[] = {
                        g_dxHandler->GetSamplerComparisonState(),
                        samplerState,
                        samplerState,
                    };
                    ctx->PSSetSamplers(0, ARRSIZE(samplers), samplers);

                    if (meshDrawData.uberStaticBuf)
                        ctx->PSSetConstantBuffers(0u, 1u, &meshDrawData.uberStaticBuf);
                    ctx->PSSetConstantBuffers(2u, ARRSIZE(sharedConstBuffers), sharedConstBuffers);

#if defined(ADVANCED_MODEL_PREVIEW)
                    scene.BindLightsSRV(ctx);
#endif
                }
                else
                {
                    ctx->PSSetSamplers(0, 1, &samplerState);
                }

                for (auto& tex : meshDrawData.textures)
                {
                    ID3D11ShaderResourceView* const textureSRV = tex.texture
                        ? tex.texture.get()->GetSRV()
                        : nullptr;

                    ctx->PSSetShaderResources(tex.resourceBindPoint, 1u, &textureSRV);
                }

                ctx->IASetIndexBuffer(meshDrawData.indexBuffer, DXGI_FORMAT_R16_UINT, 0u);
                ctx->DrawIndexed(static_cast<UINT>(meshDrawData.numIndices), 0u, 0u);
            }
        }
        else
        {
            assertm(0, "Failed to load shaders for model preview.");
        }
    }

    ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());

    g_dxHandler->GetSwapChain()->Present(1u, 0u);
}