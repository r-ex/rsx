#include <pch.h>
#include <core/filehandling/load.h>
#include <core/filehandling/export.h>
#include <core/utils/cli_parser.h>

extern CBufferManager g_BufferManager;

extern std::atomic<bool> inJobAction;

static void HandleFileLoad(std::vector<std::string> filePaths)
{
    std::vector<std::string> pathsByExtension[CAsset::ContainerType::_COUNT];

    for (auto& path : filePaths)
    {
        const std::string extension = std::filesystem::path(path).extension().string();

        if (extension == ".rpak")
            pathsByExtension[CAsset::ContainerType::PAK].emplace_back(path);
        else if (extension == ".mbnk")
            pathsByExtension[CAsset::ContainerType::AUDIO].emplace_back(path);
        else if (extension == ".mdl")
            pathsByExtension[CAsset::ContainerType::MDL].emplace_back(path);
        else if (extension == ".bpk")
            pathsByExtension[CAsset::ContainerType::BP_PAK].emplace_back(path);
        else
            Log("Invalid file extension found in path: %s.\n", path.c_str());
    }

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
        }
    }

    g_assetData.ProcessAssetsPostLoad();
}

void HandleLoadFromCommandLine(const CCommandLine* const cli)
{
    std::vector<std::string> filePaths;
    for (int i = 1; i < cli->GetArgC(); ++i) // we skip 0 since its selfpath
    {
        std::filesystem::path path = std::filesystem::path(cli->GetParamValue(i));
        if (std::filesystem::exists(path) && std::filesystem::is_regular_file(path))
        {
            filePaths.emplace_back(path.string());
        }
    }

    CThread(HandleFileLoad, std::move(filePaths)).detach();
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
    openFileName.lpstrFilter = "reSource Asset Files (*.rpak, *.mbnk, *.mdl)\0*.RPAK;*.MBNK;*.MDL;*.BPK\0";
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