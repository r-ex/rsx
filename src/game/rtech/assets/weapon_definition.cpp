#include "pch.h"
#include <game/rtech/assets/weapon_definition.h>

extern ExportSettings_t g_ExportSettings;

void LoadWeaponDefinitionAsset(CAssetContainer* const pak, CAsset* const asset)
{
    UNUSED(pak);
    UNUSED(asset);

    CPakAsset* pakAsset = static_cast<CPakAsset*>(asset);
    const WepnAssetHeader_v1_t* const header = reinterpret_cast<const WepnAssetHeader_v1_t* const>(pakAsset->header());

    const std::string assetName = "weapon/" + std::string(header->weaponName) + ".rpak";
    pakAsset->SetAssetName(assetName);
}

std::string R_GetWeaponDefinitionAsString(const WepnData_v1_t* const key, const std::string& indentStr)
{
    std::string retVal = indentStr + "\"" + key->name + "\"\n" + indentStr + "{\n";

    // first loop over child values
    for (int j = 0; j < key->numValues; ++j)
    {
        retVal += indentStr + "\t// val unk: " + std::to_string(key->unk_20[j]) + "\n";
        WepnKeyValue_v1_t* val = &key->childKVPairs[j];
        retVal += indentStr + "\t\"" + std::string(val->key) + "\" \"";

        if (val->valueType == WepnValueType_t::STRING)
            retVal += std::string(val->value.strVal);
        else if (val->valueType == WepnValueType_t::INTEGER)
            retVal += std::to_string(val->value.intVal);
        else if (val->valueType == WepnValueType_t::FLT)
            retVal += std::to_string(val->value.fltVal);
        else
        {
            retVal += "\"unk\" // " + std::string(val->key) + ": unknown. rawval: " + std::format("{:X}", val->value.rawVal);
            Log("unknown var type: %s %i %llX\n", val->key, val->valueType, val->value.rawVal);
        }
        retVal += "\"\n";
    }

    // add a newline to separate the values and the objects
    retVal += "\n";

    // then loop over child objects
    for (int i = 0; i < key->numChildren; ++i)
    {
        WepnData_v1_t* childKey = &key->childObjects[i];

        retVal += indentStr + "\t// child unk: " + std::to_string(key->unk_28[i]) + "\n";

        retVal += R_GetWeaponDefinitionAsString(childKey, indentStr + "\t");

        if (i != key->numChildren - 1)
            retVal += "\n"; // as long as we aren't the last child object, add a new line as a separator
    }

    retVal += indentStr + "}\n";

    return retVal;
}

bool ExportWeaponDefinitionAsset(CAsset* const asset, const int setting)
{
    UNUSED(asset);
    UNUSED(setting);

    CPakAsset* pakAsset = static_cast<CPakAsset*>(asset);
    const WepnAssetHeader_v1_t* const header = reinterpret_cast<WepnAssetHeader_v1_t* const>(pakAsset->header());
    
    const std::string wepnTxt = R_GetWeaponDefinitionAsString(header->rootKey, "");

    // Create exported path + asset path.
    std::filesystem::path exportPath = std::filesystem::current_path().append(EXPORT_DIRECTORY_NAME);
    const std::filesystem::path wepnPath(asset->GetAssetName());

    if (g_ExportSettings.exportPathsFull)
        exportPath.append(wepnPath.parent_path().string());
    else
        exportPath.append("weapon/");

    if (!CreateDirectories(exportPath))
    {
        assertm(false, "Failed to create asset's directory.");
        return false;
    }

    exportPath.append(wepnPath.filename().string());
    exportPath.replace_extension(".txt");

    StreamIO out;
    if (!out.open(exportPath.string(), eStreamIOMode::Write))
    {
        assertm(false, "Failed to open file for write.");
        return false;
    }

    out.write(wepnTxt.c_str(), wepnTxt.length());
    out.close();

    return false;
}

void InitWeaponDefinitionAssetType()
{
    static const char* settings[] = { "TXT" };
    AssetTypeBinding_t type =
    {
        .type = 'npew',
        .headerAlignment = 8,
        .loadFunc = LoadWeaponDefinitionAsset,
        .postLoadFunc = nullptr,
        .previewFunc = nullptr,
        .e = { ExportWeaponDefinitionAsset, 0, settings, ARRSIZE(settings) },
    };

    REGISTER_TYPE(type);
}