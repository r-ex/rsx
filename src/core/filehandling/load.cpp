#include <pch.h>
#include <core/filehandling/load.h>
#include <core/filehandling/export.h>
#include <core/utils/cli_parser.h>
#include <core/filehandling/validation.h>

extern CBufferManager g_BufferManager;

extern std::atomic<bool> inJobAction;

typedef void(HandleFileLoadCallback_t)(const CCommandLine* const);

typedef std::array<std::vector<std::string>, CAsset::ContainerType::_COUNT> PathExtensionArray_t;

void GroupPathByExtension(PathExtensionArray_t* pathsByExtension, const std::filesystem::path& path)
{
    const std::string extension = path.extension().string();

    if (extension == ".rpak")
        (*pathsByExtension)[CAsset::ContainerType::PAK].emplace_back(path.string());
    else if (extension == ".mbnk")
        (*pathsByExtension)[CAsset::ContainerType::AUDIO].emplace_back(path.string());
    else if (extension == ".mdl")
        (*pathsByExtension)[CAsset::ContainerType::MDL].emplace_back(path.string());
    else if (extension == ".bpk")
        (*pathsByExtension)[CAsset::ContainerType::BP_PAK].emplace_back(path.string());
    else if (extension == ".vpk")
        (*pathsByExtension)[CAsset::ContainerType::VPK].emplace_back(path.string());
}

// Update "load asset type" settings for all type bindings according to whether they are specified in the --loadwhitelist param
static void CLI_HandleAssetTypeWhitelist(const CCommandLine* const cli)
{
    if (!cli)
        return;

    if (!IS_NOGUI(cli))
        return;

    // Omitted --loadwhitelist must keep the default (_loadAssetType = true).
    // Treating a missing flag as an empty set disabled every type, so nogui
    // -export skipped loadFunc/exportFunc and wrote no files with no error.
    const char* const typeString = cli->GetParamValue("--loadwhitelist");
    if (!typeString)
        return;

    const std::unordered_set<uint32_t> filterTypes = CLI_GetCommaSeparatedAssetTypes(cli, "--loadwhitelist");

    printf("LOAD: Applying --loadwhitelist \"%s\" (%lld valid type%s)\n",
        typeString,
        static_cast<long long>(filterTypes.size()),
        filterTypes.size() == 1 ? "" : "s");

    for (auto& [fourCC, binding] : g_assetData.m_assetTypeBindings)
    {
        binding._loadAssetType = filterTypes.contains(fourCC);
    }
}

static void HandleFileLoad(std::vector<std::string> filePaths, HandleFileLoadCallback_t cb = nullptr, const CCommandLine* const cli = nullptr)
{
    PathExtensionArray_t pathsByExtension;

    for (auto& path : filePaths)
    {
        std::filesystem::path fsPath(path);

        // If the path is a directory, try to load all paks in the directory
        if (std::filesystem::is_directory(path))
        {
            for (const auto& dirEntry : std::filesystem::directory_iterator(fsPath))
            {
                if(dirEntry.is_regular_file())
                    GroupPathByExtension(&pathsByExtension, dirEntry.path());
            }
        }
        else GroupPathByExtension(&pathsByExtension, fsPath);
    }

    CLI_HandleAssetTypeWhitelist(cli);

    g_assetData.m_validate = cli && cli->HasParam("-validate");
    g_assetData.m_validateAssetLoading = cli && cli->HasParam("-validateload");

    for (uint32_t i = 0; i < CAsset::ContainerType::_COUNT; ++i)
    {
        // [rika]: we should skip a function if we don't have files for it
        if (pathsByExtension[i].empty())
            continue;

        switch (i)
        {
        case CAsset::ContainerType::PAK:
            HandlePakLoad(pathsByExtension[i]);
            break;
        case CAsset::ContainerType::AUDIO:
            HandleMBNKLoad(pathsByExtension[i]);
            break;
        case CAsset::ContainerType::MDL:
            HandleMDLLoad(pathsByExtension[i]);
            break;
        case CAsset::ContainerType::BP_PAK:
            HandleBPKLoad(pathsByExtension[i]);
            break;
        case CAsset::ContainerType::VPK:
            HandleVPKLoad(pathsByExtension[i]);
            break;
        }
    }
    
    // Only run post-load if we are running with GUI, or if CLI has requested the export of some assets
    if(!cli || !IS_NOGUI(cli) || cli->HasParam("-export"))
        g_assetData.ProcessAssetsPostLoad();

    // This callback is only really needed for CLI, since users aren't able to access the assets before postloading anyway
    if (IS_NOGUI(cli))
    {
        if (cb && cli)
            cb(cli);
    }
}

bool FilterAssetsByType_CommandLine(
    const CCommandLine* const cli,
    const std::vector<CGlobalAssetData::AssetLookup_t>& assetsIn,
    std::vector<CGlobalAssetData::AssetLookup_t>& assetsOut)
{
    std::unordered_set<uint32_t> filterTypes = CLI_GetCommaSeparatedAssetTypes(cli, "--exporttypes");
    if (filterTypes.size() != 0)
    {
        printf("\nEXPORT: Filtering assets for export using type string \"%s\" (%lld valid type%s)\n", cli->GetParamValue("--exporttypes"), filterTypes.size(), filterTypes.size() == 1 ? "" : "s");

        for (auto& it : assetsIn)
        {
            bool addedToFilter = false;
            for (const uint32_t type : filterTypes)
            {
                if (it.m_asset->GetAssetType() == type)
                {
                    addedToFilter = true;
                    break;
                }
            }

            if (addedToFilter)
                assetsOut.push_back(it);
        }

        return true;
    }

    return false;
}

bool FilterAssetsByNames_CommandLine(
    const CCommandLine* const cli,
    const std::vector<CGlobalAssetData::AssetLookup_t>& assetsIn,
    std::vector<CGlobalAssetData::AssetLookup_t>& assetsOut)
{
    const char* const filterString = cli->GetParamValue("--exportfilter");

    TextFilter nameFilter = {};
    GetTextFilterForExport(filterString, &nameFilter);

    if (!assetsIn.empty() && nameFilter.IsActive())
    {
        assetsOut.clear();

        for (auto& it : assetsIn)
        {
            const std::string& assetName = it.m_asset->GetAssetName();

            if (nameFilter.PassFilter(assetName.c_str()))
                assetsOut.push_back(it);
            else
            {
                const char* const inputText = nameFilter.inputBuf.c_str();
                const size_t inputLen = nameFilter.inputBuf.size();

                char* end;
                const uint64_t guid = strtoull(inputText, &end, 0);

                if (end == &inputText[inputLen])
                {
                    if (guid == RTech::StringToGuid(assetName.c_str()))
                        assetsOut.push_back(it);
                }
            }
        }

        return true;
    }

    return false;
}


void OnCLILoadComplete(const CCommandLine* const cli)
{
    // Hold this thread until asset loading is done on the newly spawned threads
    while (true)
    {
        // If we are exporting, wait until post load. Otherwise only wait until load is done
        if (g_assetData.m_donePostLoad || (!cli->HasParam("-export") && g_assetData.m_doneLoad))
            break;
    }

    if (cli->HasParam("-validate"))
    {
        ValidateLoadedPakFiles();
    }
    else if (cli->HasParam("-export"))
    {
        std::vector<CGlobalAssetData::AssetLookup_t>& assets = g_assetData.v_assets;
        std::vector<CGlobalAssetData::AssetLookup_t> filteredAssets;

        if (FilterAssetsByType_CommandLine(cli, assets, filteredAssets))
            assets = filteredAssets;

        if (FilterAssetsByNames_CommandLine(cli, assets, filteredAssets))
            assets = filteredAssets;

        if (!assets.empty())
            CThread(HandleExportAllPakAssets, &assets, g_rsxSettings.exportAssetDeps).join();
        else
            Log("No assets to export!\n");
    }

    if (const char* const listPathStr = cli->GetParamValue("--list"))
    {
        std::ofstream ofs(listPathStr, std::ios::out | std::ios::binary);

        const char* const listFormat = cli->GetParamValue("--listformat");

        if (!listFormat || !_stricmp(listFormat, "txt"))
            ExportAssetListTXTToFileStream(&g_assetData.v_assets, &ofs);
        else if (!_stricmp(listFormat, "csv"))
            ExportAssetListCSVToFileStream(&g_assetData.v_assets, &ofs);
    }

    // Writes a file containing info about each asset's dependencies to the provided file path
    if (const char* const depFilePath = cli->GetParamValue("--depfilepath"))
    {
        std::ofstream ofs(depFilePath, std::ios::out | std::ios::binary);

        const char* depFileFormat = cli->GetParamValue("--depfileformat");

        if (!depFileFormat || !_stricmp(depFileFormat, "adjlist"))
            ExportDependenciesToFileStream_AdjList(&g_assetData.v_assets, &ofs);
    }
}

void HandleLoadFromCommandLine(const CCommandLine* const cli)
{
    std::vector<std::string> filePaths;

    for (uint32_t i = cli->GetFirstNonFlagArgIdx(); i < cli->GetArgC(); ++i) // we skip 0 since its selfpath
    {
        std::filesystem::path path = std::filesystem::path(cli->GetParamValue(i));

        if (std::filesystem::exists(path))
        {
            filePaths.emplace_back(path.string());
        }
    }

    CThread thread = CThread(HandleFileLoad, std::move(filePaths), OnCLILoadComplete, cli);

    // If this gets detached when running without the usual windows msg loop to hold up main thread, the main thread will exit
    // and clean up static vars before the other threads have finished execution. This will cause a crash when accessing anything static
    // such as s_AssetTypePaths in pakfile's ProcessAssets
    if (IS_NOGUI(cli))
        thread.join();
    else
        thread.detach();
}

void HandleOpenFileDialog(const HWND windowHandle)
{
    // We are in pak load now.
    inJobAction = true;

    CManagedBuffer* fileNames = g_BufferManager.ClaimBuffer();
    memset(fileNames->Buffer(), 0, CBufferManager::MaxBufferSize());

    OPENFILENAMEA openFileName = {};

    openFileName.lStructSize = sizeof(OPENFILENAMEA);
    openFileName.hwndOwner = windowHandle;
    openFileName.lpstrFilter = "reSource Asset Files (*.rpak, *.mbnk, *.mdl, *.bpk, *_dir.vpk)\0*.RPAK;*.MBNK;*.MDL;*.BPK;*_DIR.VPK\0";
    openFileName.lpstrFile = fileNames->Buffer();
    openFileName.nMaxFile = static_cast<DWORD>(CBufferManager::MaxBufferSize());
    openFileName.Flags = OFN_EXPLORER | OFN_FILEMUSTEXIST | OFN_ALLOWMULTISELECT | OFN_NOCHANGEDIR;
    openFileName.lpstrDefExt = "";

    if (GetOpenFileNameA(&openFileName))
    {
        std::vector<std::string> filePaths;
        std::string directoryPath(fileNames->Buffer());

        // fileNames buffer is a collection of strings
        // the first string is the path of the directory
        // and the following strings are filenames within that directory
        const char* fileNamePtr = fileNames->Buffer() + directoryPath.length() + 1;

        if (*fileNamePtr) // check if there is actually a first filename or if it's just a null byte
        {
            for (; *fileNamePtr; fileNamePtr += strnlen(fileNamePtr, MAX_PATH) + 1)
            {
                std::string filePath = directoryPath + '\\' + fileNamePtr;
                filePaths.push_back(std::move(filePath));
            }
        }
        else
        {
            filePaths.push_back(std::move(directoryPath));
        }

        // We are moving the whole vector out of here into HandlePakLoad.
        HandleFileLoad(std::move(filePaths));
    }

    g_BufferManager.RelieveBuffer(fileNames);

    // We are done with pak loading.
    inJobAction = false;
}

void Bridge_HandleLoad(std::vector<std::string> filePaths)
{
    inJobAction = true;
    HandleFileLoad(std::move(filePaths));
    inJobAction = false;
}