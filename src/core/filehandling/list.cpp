#include <pch.h>
#include <core/filehandling/export.h>
#include <core/render/dx.h>

extern CBufferManager* g_BufferManager;

void HandleListExportPakAssets(const HWND handle, std::vector<CGlobalAssetData::AssetLookup_t>* assets)
{
    std::vector<std::string> assetNames(assets->size());
    size_t i = 0;
    for (auto& it : *assets)
    {
        assetNames.at(i) = it.m_asset->GetAssetName(); 
        i++;
    }

    HandleListExport(handle, assetNames);
}

void HandleListExport(const HWND handle, std::vector<std::string> listElements)
{
    // We are in pak load now.
    //inJobAction = true;

    g_BufferManager = new CBufferManager();
    CManagedBuffer* fileNames = g_BufferManager->ClaimBuffer();
    memset(fileNames->Buffer(), 0, CBufferManager::MaxBufferSize());

    OPENFILENAMEA openFileName = {};

    openFileName.lStructSize = sizeof(OPENFILENAMEA);
    openFileName.hwndOwner = handle;
    openFileName.lpstrFilter = "Text File (*.txt)\0*.txt\0";
    openFileName.lpstrFile = fileNames->Buffer();
    openFileName.nMaxFile = static_cast<DWORD>(CBufferManager::MaxBufferSize());
    openFileName.Flags = OFN_EXPLORER | OFN_NOCHANGEDIR;
    openFileName.lpstrDefExt = "";

    if (GetSaveFileNameA(&openFileName))
    {
        std::string listPath(fileNames->Buffer());

        std::ofstream ofs(listPath, std::ios::out);

        // Iterate over all elements of our string vector and write them to the selected file
        for (auto& str : listElements)
        {
            ofs << str << "\n";
        }

        ofs.close();

        g_BufferManager->RelieveBuffer(fileNames);

        delete g_BufferManager;
    }
}