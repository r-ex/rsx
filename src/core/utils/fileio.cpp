#include <pch.h>
#include <core/utils/fileio.h>

std::mutex dirMutex;
bool CreateDirectories(const std::filesystem::path& exportPath)
{
    std::lock_guard<std::mutex> lock(dirMutex);
    if (!std::filesystem::exists(exportPath))
    {
        if (!std::filesystem::create_directories(exportPath))
        {
            return false;
        }
    }
    return true;
}

bool RestoreCurrentWorkingDirectory()
{
    wchar_t processDirectory[MAX_PATH];
    const DWORD processPathSize = GetModuleFileNameW(nullptr, processDirectory, MAX_PATH);
    if (processPathSize == 0)
    {
        assertm(false, "failed to get process path.");
        return false;
    }

    std::filesystem::path exePath(processDirectory);
    std::filesystem::current_path(exePath.parent_path()); // this sets the current working directory

    return true;
}

namespace FileSystem
{

    bool ReadFileData(const std::string& filePath, std::shared_ptr<char[]>* buffer)
    {
        StreamIO file;
        if (!file.open(filePath, eStreamIOMode::Read))
            return false;

        *buffer = std::shared_ptr<char[]>(new char[file.size()]);
        file.read(buffer->get(), file.size());
        return true;
    }
}