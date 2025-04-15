#include <pch.h>
#include <core/filehandling/load.h>
#include <core/filehandling/export.h>

extern CBufferManager* g_BufferManager;

extern std::atomic<bool> inJobAction;

void HandleFileLoad(std::vector<std::string> filePaths)
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
        else
            Log("Invalid file extension found in path: %s.\n", path.c_str());
    }

    for (uint32_t i = 0; i < CAsset::ContainerType::_COUNT; ++i)
    {
        switch (i)
        {
        case CAsset::ContainerType::PAK:
            HandlePakLoad(pathsByExtension[i]);
            break;
        case CAsset::ContainerType::AUDIO:
            HandleMBNKLoad(pathsByExtension[i]);
            break;
        case CAsset::ContainerType::MDL:
            HandleModelLoad(pathsByExtension[i]);
            break;
        }
    }

    g_assetData.ProcessAssetsPostLoad();
}

void HandleOpenFileDialog(const HWND windowHandle)
{
    // We are in pak load now.
    inJobAction = true;

    g_BufferManager = new CBufferManager();
    CManagedBuffer* fileNames = g_BufferManager->ClaimBuffer();
    memset(fileNames->Buffer(), 0, CBufferManager::MaxBufferSize());

    OPENFILENAMEA openFileName = {};

    openFileName.lStructSize = sizeof(OPENFILENAMEA);
    openFileName.hwndOwner = windowHandle;
    openFileName.lpstrFilter = "reSource Asset Files (*.rpak, *.mbnk, *.mdl)\0*.RPAK;*.MBNK;*.MDL\0";
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

    g_BufferManager->RelieveBuffer(fileNames);
    delete g_BufferManager;

    // We are done with pak loading.
    inJobAction = false;
}