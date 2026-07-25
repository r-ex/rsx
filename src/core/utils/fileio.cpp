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

// time is in ms
HANDLE WaitForFileHandle(const uint32_t waitInterval, const uint32_t maxWaitTime, LPCSTR fileName, DWORD fileDesiredAccess, DWORD fileShareMode, LPSECURITY_ATTRIBUTES fileSecurityAttributes, DWORD fileCreationDisposition, DWORD fileFlagsAndAttributes, HANDLE fileTemplateFile)
{
	HANDLE fileHandle = INVALID_HANDLE_VALUE;
	uint32_t timeWaited = 0u;

	// [rika]: wait for the desired time
	while (maxWaitTime >= timeWaited)
	{
		fileHandle = CreateFileA(fileName, fileDesiredAccess, fileShareMode, fileSecurityAttributes, fileCreationDisposition, fileFlagsAndAttributes, fileTemplateFile);

		if (fileHandle != INVALID_HANDLE_VALUE)
			break;

		const DWORD error = GetLastError();

		// [rika]: sharing violation is good, actually
		if (error != ERROR_SHARING_VIOLATION)
		{
			assertm(false, "invalid file handle");
			Log("failed to get handle for file %s, invalid error code!\n", fileName);
			break;
		}

		Sleep(waitInterval);
		timeWaited += waitInterval;
	}

	return fileHandle;
}

FILE* FileFromHandle(HANDLE handle, const eStreamIOMode mode)
{
	if (handle == INVALID_HANDLE_VALUE)
	{
		assertm(false, "invalid handle");
		return nullptr;
	}

	const int fileDescriptor = _open_osfhandle(reinterpret_cast<intptr_t>(handle), 0);
	if (fileDescriptor == -1)
	{
		assertm(false, "invalid descriptor");
		return nullptr;
	}

	FILE* file = nullptr;
	switch (mode)
	{
	case eStreamIOMode::Write:
	{
		file = _fdopen(fileDescriptor, "w");
		break;
	}
	case eStreamIOMode::Read:
	{
		file = _fdopen(fileDescriptor, "r");
		break;
	}
	default:
	{
		assertm(false, "invalid eStreamIOMode");
		break;
	}
	}

	assertm(file, "invalid file");
	return file;
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

	bool ReadFileData(const std::string& filePath, std::shared_ptr<char[]>* buffer, size_t readSize)
	{
		StreamIO file;
		if (!file.open(filePath, eStreamIOMode::Read))
			return false;

		readSize = std::clamp(readSize, 0ull, file.size());

		*buffer = std::shared_ptr<char[]>(new char[readSize]);
		file.read(buffer->get(), readSize);
		return true;
	}
}