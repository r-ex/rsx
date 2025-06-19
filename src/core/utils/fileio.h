#pragma once

enum class eStreamIOMode : uint8_t
{
    None,
    Read,
    Write
};

class StreamIO
{
public:
    StreamIO() : writer(), reader(), filePath(), currentMode(eStreamIOMode::None) {};
    StreamIO(const std::string& path, const eStreamIOMode mode)
    {
        open(path, mode);
    }

    StreamIO(const std::filesystem::path& path, const eStreamIOMode mode)
    {
        open(path.string(), mode);
    }

    // opens a file with either read or write mode. Returns whether
    // the open operation was successful
    bool open(const std::string& path, const eStreamIOMode mode)
    {
        // Write mode
        if (mode == eStreamIOMode::Write)
        {
            currentMode = mode;
            // check if we had a previously opened file to close it
            if (writer.is_open())
                writer.close();

            writer.open(path.c_str(), std::ios::binary);
            if (!writer.is_open())
            {
                currentMode = eStreamIOMode::None;
            }
        }
        // Read mode
        else if (mode == eStreamIOMode::Read)
        {
            currentMode = mode;
            // check if we had a previously opened file to close it
            if (reader.is_open())
                reader.close();

            reader.open(path.c_str(), std::ios::binary);
            if (!reader.is_open())
            {
                currentMode = eStreamIOMode::None;
            }
        }

        // if the mode is still the NONE/initial one -> we failed
        return currentMode == eStreamIOMode::None ? false : true;
    }

    // closes the file
    void close()
    {
        if (currentMode == eStreamIOMode::Write)
        {
            writer.close();
        }
        else if (currentMode == eStreamIOMode::Read)
        {
            reader.close();
        }
    }

    // checks whether we're allowed to write or not.
    bool checkWritabilityStatus()
    {
        if (currentMode != eStreamIOMode::Write)
        {
            return false;
        }
        return true;
    }

    // helper to check if we're allowed to read
    bool checkReadabilityStatus()
    {
        if (currentMode != eStreamIOMode::Read)
        {
            return false;
        }

        // check if we hit the end of the file.
        if (reader.eof())
        {
            reader.close();
            currentMode = eStreamIOMode::None;
            return false;
        }

        return true;
    }

    // so we can check if we hit the end of the file
    bool eof()
    {
        return reader.eof();
    }

    // Generic write method that will write any value to a file (except a string,
    template <typename T>
    void write(T& value)
    {
        if (!checkWritabilityStatus())
            return;

        // write the value to the file.
        writer.write((const char*)&value, sizeof(value));
    }

    void write(const char* data, size_t len)
    {
        if (!checkWritabilityStatus())
            return;

        writer.write(data, len);
    }

    void writeString(std::string str)
    {
        if (!checkWritabilityStatus())
            return;

        // first add a \0 at the end of the string so we can detect
        // the end of string when reading it
        str += '\0';

        // create char pointer from string.
        char* text = (char*)(str.c_str());
        // find the length of the string.
        size_t size = str.size();

        // write the whole string including the null.
        writer.write((const char*)text, size);
    }

    // reads any type of value except strings.
    template <typename T>
    T read()
    {
        if (checkReadabilityStatus())
        {
            T value;
            reader.read((char*)&value, sizeof(value));
            return value;
        }

        return T();
    }

    // reads any type of value except strings.
    template <typename T>
    void read(T& value)
    {
        if (checkReadabilityStatus())
        {
            reader.read((char*)&value, sizeof(value));
        }
    }

    void read(char* const buf, const size_t size)
    {
        if (checkReadabilityStatus())
        {
            reader.read(buf, size);
        }
    }

    const std::string readString()
    {
        if (checkReadabilityStatus())
        {
            char c;
            std::string result = "";
            while (!reader.eof() && (c = read<char>()) != '\0')
            {
                result += c;
            }

            return result;
        }
        return "";
    }

    void readString(std::string& result)
    {
        if (checkReadabilityStatus())
        {
            char c;
            result = "";
            while (!reader.eof() && (c = read<char>()) != '\0')
            {
                result += c;
            }
        }
    }

    const size_t tell()
    {
        switch (currentMode)
        {
        case eStreamIOMode::Write:
            return writer.tellp();
        case eStreamIOMode::Read:
            return reader.tellg();
        default:
            return 0;
        }
    }

    const size_t size()
    {
        // Get the current position
        const size_t currentPosition = tell();

        // Seek to the end of the file
        seek(0u, std::ios::end);

        // Get the size of the file
        const size_t fileSize = tell();

        // Seek back to the original position
        seek(currentPosition);

        return fileSize;
    }

    void seek(const size_t off, std::ios::seekdir dir = std::ios::beg)
    {
        switch (currentMode)
        {
        case eStreamIOMode::Write:
            writer.seekp(off, dir);
            break;
        case eStreamIOMode::Read:
            reader.seekg(off, dir);
            break;
        default:
            break;
        }
    }

    std::ifstream* const R()
    {
        if (currentMode != eStreamIOMode::Read)
        {
            return nullptr;
        }

        return &reader;
    }
    std::ofstream* const W()
    {
        if (currentMode != eStreamIOMode::Write)
        {
            return nullptr;
        }

        return &writer;
    }

private:
    std::ofstream writer;
    std::ifstream reader;
    std::string filePath;
    eStreamIOMode currentMode;
};

bool CreateDirectories(const std::filesystem::path& exportPath);
bool RestoreCurrentWorkingDirectory();

namespace FileSystem
{
    bool ReadFileData(const std::string& filePath, std::shared_ptr<char[]>* buffer);
}
