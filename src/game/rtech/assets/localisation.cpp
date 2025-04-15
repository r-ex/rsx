#include <pch.h>
#include <game/rtech/assets/localisation.h>
#include <game/rtech/cpakfile.h>

extern ExportSettings_t g_ExportSettings;

void LoadLocalisationAsset(CAssetContainer* pak, CAsset* asset)
{
    UNUSED(pak);
    CPakAsset* pakAsset = static_cast<CPakAsset*>(asset);

    LocalisationAsset* loclAsset = nullptr;

    LocalisationHeader_t* hdr = reinterpret_cast<LocalisationHeader_t*>(pakAsset->header());
    loclAsset = new LocalisationAsset(hdr);

    std::string assetName = "localization/" + loclAsset->getName() + ".locl";
    pakAsset->SetAssetName(assetName);
    pakAsset->setExtraData(loclAsset);
}

std::string EscapeLocalisationString(const std::string& str)
{
    std::stringstream outStream;

    for (int i = 0; i < str.length(); ++i)
    {
        unsigned char c = str.at(i);

        // if this char is over ascii then it's a multibyte sequence
        if (c > 0x7F)
        {
            // find the number of bytes to add to the stringstream
            int numBytes = 0;

            if (c <= 0xBF)
                numBytes = 1;
            else if (c >= 0xc2 && c <= 0xdf)      // 0xC2 -> 0xDF - 2 byte sequence
                numBytes = 2;
            else if (c >= 0xe0 && c <= 0xef) // 0xE0 -> 0xEF - 3 byte sequence
                numBytes = 3;
            else if (c >= 0xf0 && c <= 0xf4) // 0xF0 -> 0xF4 - 4 byte sequence
                numBytes = 4;

            for (int j = 0; j < numBytes; ++j)
            {
                outStream << str.at(i + j);
            }

            // add numBytes-1 to the char index
            // since one increment will already be handled by the for loop
            i += numBytes-1;
            continue;
        }

        switch (c)
        {
        case '\0':
            break;
        case '\t':
            outStream << "\\t";
            break;
        case '\n':
            outStream << "\\n";
            break;
        case '\r':
            outStream << "\\r";
        case '\"':
            outStream << "\\\"";
            break;
        default:
        {
            if (!std::isprint(c))
            {
                Log("non printable char @ %i/%lld\n", i, str.length());
                outStream << std::hex << std::setfill('0') << std::setw(2) << "\\x" << (int)c;
            }
            else
                outStream << c;

            break;
        }
        }
    }

    return outStream.str();
}

static const char* const s_PathPrefixLOCL = s_AssetTypePaths.find(AssetType_t::LOCL)->second;
bool ExportLocalisationAsset(CAsset* const asset, const int setting)
{
    UNUSED(setting);

    CPakAsset* pakAsset = static_cast<CPakAsset*>(asset);

    // Create exported path + asset path.
    std::filesystem::path exportPath = std::filesystem::current_path().append(EXPORT_DIRECTORY_NAME); // 
    const std::filesystem::path localizationPath(asset->GetAssetName());

    // truncate paths?
    if (g_ExportSettings.exportPathsFull)
        exportPath.append(localizationPath.parent_path().string());
    else
        exportPath.append(s_PathPrefixLOCL);

    if (!CreateDirectories(exportPath))
    {
        assertm(false, "Failed to create asset type directory.");
        return false;
    }

    const LocalisationAsset* const loclAsset = reinterpret_cast<LocalisationAsset*>(pakAsset->extraData());

    exportPath.append(loclAsset->fileName); // likely quicker than "exportPath.append(localizationPath.stem().string());"
    exportPath.replace_extension(".locl");

    std::ofstream ofs(exportPath, std::ios::out | std::ios::binary);

    ofs << "\"" << loclAsset->fileName << "\"\n{\n";

    for (size_t i = 0; i < loclAsset->numEntries; ++i)
    {
        const LocalisationEntry_t* const entry = &loclAsset->entries[i];

        if (entry->hash == 0)
            continue;

        const std::wstring wideString = &loclAsset->strings[entry->stringStartIndex];

        std::string multiByteString;
        multiByteString.resize(wideString.length() * 2);    // some utf-8 characters are actually more than one byte, up to four bytes. if we don't have enough space, WideCharToMultiByte will not finish its conversion, causing their to be incomplete strings.
                                                            // only allowing for characters up to two bytes currently as it seems to be sufficient

        // windows utf-16 support sucks so convert to multibyte utf8
        WideCharToMultiByte(CP_UTF8, 0, wideString.c_str(), static_cast<int>(wideString.length()), &multiByteString[0], static_cast<int>(multiByteString.length()), (LPCCH)NULL, NULL);

        ofs << "\t\"" << std::hex << entry->hash << "\" \"" << EscapeLocalisationString(multiByteString).c_str() << "\"\n"; // use the .c_str() function here now so it gets null terminated properly (if there's extra data or it's cut off, there will no longer be a [NUL] character in the file.
    }

    ofs << "}";

    ofs.close();

    return true;
}

void InitLoclAssetType()
{
    AssetTypeBinding_t type =
    {
        .type = 'lcol',
        .headerAlignment = 8,
        .loadFunc = LoadLocalisationAsset,
        .postLoadFunc = nullptr,
        .previewFunc = nullptr,
        .e = { ExportLocalisationAsset, 0, nullptr, 0ull },
    };

    REGISTER_TYPE(type);
}