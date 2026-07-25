// This file is probably in the wrong place; I'll figure it out later.

#include <pch.h>
#include <imgui.h>
#include <game/asset.h>
#include "core/render/uistate.h"
#include <game/rtech/cpakfile.h>
#include <game/rtech/assets/settings.h>
#include <core/render/dx.h>
#include <game/rtech/assets/localization.h>
#include <game/rtech/assets/odl_asset.h>
#include <imgui_internal.h>
#include <misc/imgui_utility.h>
#include <game/rtech/assets/ui_image.h>

constexpr uint32_t CHARSKIN_SHARED_FAVORITES_SAID = 0x053a2537;
constexpr uint32_t WEPSKIN_SHARED_FAVORITES_SAID = 0x447d095c;

extern CDXParentHandler* g_dxHandler;

static const std::unordered_map<char, uint32_t> s_qualityInts = {
    {'N', 0u}, // NONE
    {'C', 1u}, // COMMON
    {'R', 2u}, // RARE
    {'E', 3u}, // EPIC
    {'L', 4u}, // LEGENDARY
    {'M', 5u}, // MYTHIC
    {'I', 6u}, // ICONIC
};

static const std::unordered_map<char, ImVec4> s_qualityColours = {
    {'N', ImVec4(1.f, 1.f, 1.f, 1.f)},          // NONE:      <255,255,255>
    {'C', ImVec4(0.765f, 0.847f, 0.851f, 1.f)}, // COMMON:    <195,216,217>
    {'R', ImVec4(0.384f, 0.784f, 1.f, 1.f)},    // RARE:      <98,200,255>
    {'E', ImVec4(0.784f, 0.302f, 1.f, 1.f)},    // EPIC:      <200,77,255>
    {'L', ImVec4(1.f, 0.804f, 0.235f, 1.f)},    // LEGENDARY: <255,205,60>
    {'M', ImVec4(1.f, 0.231f, 0.263f, 1.f)},    // MYTHIC:    <255,59,67>
    {'I', ImVec4(0.f, 1.f, 0.8f, 1.f)},         // ICONIC:    <0, 255, 204>
};

// temporary macro to check if the stgs bug comes back
// replace with assertm when found
#define validate_stgs_kv(a,b) if(!(a)) { for(int i = 0; i < skinAsset->_numFields; ++i) { printf("%s\n", skinAsset->_fields[i].key); } }; assert(a);

#define XSTGS_VALUE(asset, key, varname, type, outVar) SettingsKVValue_t* varname = nullptr; if(asset->GetSettingValue(key, &varname)) outVar = varname->getValue<type>();
#define STGS_VALUE(outVar, asset, key, type) XSTGS_VALUE(asset, key, UNIQUE_VAR(), type, outVar)

// takes a vector of keys that have uiia paths as a value and adds them all to the cache db
static void IFW_CacheIconNames(SettingsAsset* settings, std::vector<std::string> argNames)
{
    SettingsKVValue_t* value = nullptr;
    for (auto& it : argNames)
    {
        if (settings->GetSettingValue(it.c_str(), &value) && value->getValue<const char*>()[0] != '\0')
            g_cacheDBManager.Add("ui_image\\" + std::string(value->getValue<const char*>()) + ".rpak");
    }
}

static void ItemflavWindow_GetCharacterDataFromSettings(CAsset* asset)
{
    CUIState& uiState = g_dxHandler->GetUIState();

    CUI_ItemflavCharacter* character = nullptr;

    int characterIdx = 0;

    for (characterIdx = 0; characterIdx < uiState.itemflavData.characterData.size(); ++characterIdx)
    {
        if (uiState.itemflavData.characterData[characterIdx].settingsAssetGuid == asset->GetAssetGUID())
        {
            character = &uiState.itemflavData.characterData[characterIdx];
            break;
        }
    }

    assertm(character, "uh oh");

    character->settingsAsset = asset;

    SettingsAsset* const settingsAsset = reinterpret_cast<SettingsAsset*>(reinterpret_cast<CPakAsset*>(asset)->extraData());

    if (settingsAsset->_numFields == 0)
        settingsAsset->ParseSettingsData();

    STGS_VALUE(character->characterName, settingsAsset, "localizationKey_NAME", const char*);
    STGS_VALUE(character->characterDesc, settingsAsset, "localizationKey_DESCRIPTION_SHORT", const char*);
    STGS_VALUE(character->shippingStatus, settingsAsset, "shippingStatus", const char*);
    STGS_VALUE(character->icon, settingsAsset, "icon", const char*);

    IFW_CacheIconNames(settingsAsset, {
        "icon", "defaultCharacterSelectPreview", "legendLockerOfferIcon",
        "characterUnlockStoreIcon", "characterLockIcon", "characterSelectIcon",
        "galleryPortraitBackground", "galleryPortrait"
    });

    SettingsKVValue_t* value = nullptr;
    if (settingsAsset->GetSettingValue("skins", &value))
    {
        assert(value->type == eSettingsFieldType::ST_ARRAY);

        character->skins.resize(value->numChildren);

        size_t j = 0;
        for (auto& skin : character->skins)
        {
            SettingsKVValue_t* const skinObj = &value->getValue<SettingsKVValue_t*>()[j];

            // first field in the object is always the settings asset path
            skin.assetPath = skinObj->getValue<SettingsKVField_t*>()[0].getValue<const char*>();
            skin.settingsAsset = g_assetData.FindAsset(skin.assetPath);

            SettingsAsset* skinAsset = reinterpret_cast<SettingsAsset*>(reinterpret_cast<CPakAsset*>(skin.settingsAsset)->extraData());

            j++; // increment before we could possibly continue

            // _shared_favorites
            if (skinAsset->uniqueId == CHARSKIN_SHARED_FAVORITES_SAID)
            {
                skin.includeInList = false;
                continue;
            }

            if (skinAsset->_numFields == 0)
                skinAsset->ParseSettingsData();

            SettingsKVValue_t* skinNameValue = nullptr;
            validate_stgs_kv(skinAsset->GetSettingValue("localizationKey_NAME", &skinNameValue), "Failed to get skin name");

            // If there's no localization key for this skin, fall back on the "skin name" field.
            // This should really only be applicable for DUMMIE's colour skins, since they are never shown in the regular cosmetics menu
            // and therefore don't need a localized name.
            if (skinNameValue->getValue<const char*>()[0] == '\0')
                skinAsset->GetSettingValue("skinName", &skinNameValue);

            SettingsKVValue_t* qualityValue = nullptr;
            validate_stgs_kv(skinAsset->GetSettingValue("quality", &qualityValue), "Failed to get skin quality");

            SettingsKVValue_t* armsModelValue = nullptr;
            validate_stgs_kv(skinAsset->GetSettingValue("armsModel", &armsModelValue), "Failed to get arms model");

            SettingsKVValue_t* bodyModelValue = nullptr;
            validate_stgs_kv(skinAsset->GetSettingValue("bodyModel", &bodyModelValue), "Failed to get body model");

            SettingsKVValue_t* armsModelOdlValue = nullptr;
            validate_stgs_kv(skinAsset->GetSettingValue("armsModel_odlBlob", &armsModelOdlValue), "Failed to get arms model odlBlob");

            SettingsKVValue_t* bodyModelOdlValue = nullptr;
            validate_stgs_kv(skinAsset->GetSettingValue("bodyModel_odlBlob", &bodyModelOdlValue), "Failed to get body model odlBlob");

            skin.localizationKey_NAME = skinNameValue->getValue<const char*>();
            skin.quality = qualityValue->getValue<const char*>();
            skin.qualityIndex = s_qualityInts.at(skin.quality[0]);
            skin.armsModel = armsModelValue->getValue<const char*>();
            skin.bodyModel = bodyModelValue->getValue<const char*>();
            skin.includeInList = true;

            CPakAsset* armsModelOdlAsset = reinterpret_cast<CPakAsset*>(g_assetData.FindAsset(armsModelOdlValue->getValue<const char*>()));
            CPakAsset* bodyModelOdlAsset = reinterpret_cast<CPakAsset*>(g_assetData.FindAsset(bodyModelOdlValue->getValue<const char*>()));

            if (armsModelOdlAsset)
                skin.armsModelPak = armsModelOdlAsset->extraData<ODLAsset*>()->GetPakName();
            if (bodyModelOdlAsset)
                skin.bodyModelPak = bodyModelOdlAsset->extraData<ODLAsset*>()->GetPakName();
        }

        std::sort(character->skins.begin(), character->skins.end(),
            [](const CUI_ItemflavCharacterSkin& a, const CUI_ItemflavCharacterSkin& b)
            {
                if (a.armsModelPak < b.armsModelPak) return true;
                if (b.armsModelPak < a.armsModelPak) return false;

                return a.qualityIndex < b.qualityIndex;
            }
        );
    }

    CAsset* iconAsset = g_assetData.FindAsset(std::format("ui_image/{}.rpak", character->icon));

    if (iconAsset)
    {
        extern std::shared_ptr<CTexture> UIIA_GetHighestTexture(CPakAsset*, UIImageAsset*);

        CPakAsset* pakAsset = reinterpret_cast<CPakAsset*>(iconAsset);
        UIImageAsset* uiia = pakAsset->extraData<UIImageAsset*>();

        character->iconTexture = UIIA_GetHighestTexture(pakAsset, uiia);
        if (!character->iconTexture)
            Log("WARNING: Failed to load uiia for %s\n", character->icon);
    }
}

static void ItemflavWindow_GetWeaponDataFromSettings(CAsset* asset)
{
    CUIState& uiState = g_dxHandler->GetUIState();

    CUI_ItemflavWeapon* weapon = nullptr;

    size_t weaponIdx = 0;

    for (weaponIdx = 0; weaponIdx < uiState.itemflavData.weaponData.size(); ++weaponIdx)
    {
        if (uiState.itemflavData.weaponData[weaponIdx].settingsAssetGuid == asset->GetAssetGUID())
        {
            weapon = &uiState.itemflavData.weaponData[weaponIdx];
            break;
        }
    }

    assertm(weapon, "uh oh");

    weapon->settingsAsset = asset;

    SettingsAsset* const settingsAsset = reinterpret_cast<SettingsAsset*>(reinterpret_cast<CPakAsset*>(asset)->extraData());

    if (settingsAsset->_numFields == 0)
        settingsAsset->ParseSettingsData();

    STGS_VALUE(weapon->weaponName, settingsAsset, "localizationKey_NAME", const char*);
    STGS_VALUE(weapon->icon, settingsAsset, "icon", const char*);

    // non-const so that we can update this if the weapon is melee and we are able to get a name from the first skin entry
    bool noLocalizedName = !weapon->weaponName || weapon->weaponName[0] == '\0';

    SettingsKVValue_t* value = nullptr;
    if (settingsAsset->GetSettingValue("offhandWeaponClassname", &value))
        weapon->weaponClassname = value->getValue<const char*>();
    else if (settingsAsset->GetSettingValue("entityClassname", &value))
        weapon->weaponClassname = value->getValue<const char*>();

    // If there's no localized weapon name (which i think only really applies to most melee weapons), then use the classname
    if (noLocalizedName && weapon->weaponClassname)
        weapon->weaponName = weapon->weaponClassname;

    STGS_VALUE(weapon->weaponDesc, settingsAsset, "localizationKey_DESCRIPTION_SHORT", const char*);
    STGS_VALUE(weapon->shippingWeapon, settingsAsset, "shippingWeapon", bool);

    if (settingsAsset->GetSettingValue("itemType", &value))
        weapon->isMelee = value->getValue<const char*>()[0] == 'm'; // melee_weapon or loot_main_weapon

    IFW_CacheIconNames(settingsAsset, { "icon", "legendLockerOfferIcon" });

    if (settingsAsset->GetSettingValue("skins", &value))
    {
        assert(value->type == eSettingsFieldType::ST_ARRAY);

        weapon->skins.resize(value->numChildren);

        size_t j = 0;
        for (auto& skin : weapon->skins)
        {
            SettingsKVValue_t* const skinObj = &value->getValue<SettingsKVValue_t*>()[j];

            // first field in the object is always the settings asset path
            skin.assetPath = skinObj->getValue<SettingsKVField_t*>()[0].getValue<const char*>();
            skin.settingsAsset = g_assetData.FindAsset(skin.assetPath);

            SettingsAsset* skinAsset = reinterpret_cast<SettingsAsset*>(reinterpret_cast<CPakAsset*>(skin.settingsAsset)->extraData());

            j++; // increment before we could possibly continue

            // _shared_favorites
            if (skinAsset->uniqueId == WEPSKIN_SHARED_FAVORITES_SAID)
            {
                skin.includeInList = false;
                skin.viewModel = "none";
                continue;
            }

            if (skinAsset->_numFields == 0)
                skinAsset->ParseSettingsData();

            SettingsKVValue_t* skinNameValue = nullptr;
            validate_stgs_kv(skinAsset->GetSettingValue("localizationKey_NAME", &skinNameValue), "Failed to get skin name");

            // If this is a skin for a melee weapon which has no localized name, use this skin's localized name
            if (weapon->isMelee && noLocalizedName && skinNameValue->getValue<const char*>()[0] != '\0')
                weapon->weaponName = skinNameValue->getValue<const char*>();

            // If there's no localization key for this skin, fall back on the "skin name" field.
            // This should really only be applicable for DUMMIE's colour skins, since they are never shown in the regular cosmetics menu
            // and therefore don't need a localized name.
            if (skinNameValue->getValue<const char*>()[0] == '\0')
                skinAsset->GetSettingValue("skinName", &skinNameValue);

            SettingsKVValue_t* qualityValue = nullptr;
            validate_stgs_kv(skinAsset->GetSettingValue("quality", &qualityValue), "Failed to get skin quality");

            SettingsKVValue_t* viewModelValue = nullptr;
            validate_stgs_kv(skinAsset->GetSettingValue("viewModel", &viewModelValue), "Failed to get view model");

            SettingsKVValue_t* worldModelValue = nullptr;
            validate_stgs_kv(skinAsset->GetSettingValue("worldModel", &worldModelValue), "Failed to get world model");

            skin.localizationKey_NAME = skinNameValue->getValue<const char*>();
            skin.quality = qualityValue->getValue<const char*>();
            skin.qualityIndex = s_qualityInts.at(skin.quality[0]);
            skin.viewModel = viewModelValue->getValue<const char*>();
            skin.worldModel = worldModelValue->getValue<const char*>();
            skin.includeInList = true;

            // only try and get a localized name from skins once
            noLocalizedName = false;
        }

        std::sort(weapon->skins.begin(), weapon->skins.end(),
            [](const CUI_ItemflavWeaponSkin& a, const CUI_ItemflavWeaponSkin& b)
            {
                return a.qualityIndex < b.qualityIndex;
            }
        );
    }
}

static void ItemflavWindow_LocalizationPostLoadCallback(CAsset* asset)
{
    CUIState& uiState = g_dxHandler->GetUIState();

    uiState.itemflavData.localizationAsset = asset;
}

static std::string Localize(const char* key)
{
    const char* origKey = key;

    if (strnlen(origKey, 1024) <= 1)
        return origKey;

    // skip over the 
    if (key[0] == '#')
        key++;

    CUIState& uiState = g_dxHandler->GetUIState();

    if (uiState.itemflavData.localizationAsset)
    {
        const LocalizationAsset* loc = reinterpret_cast<CPakAsset*>(uiState.itemflavData.localizationAsset)->extraData<LocalizationAsset*>();
        
        if (auto it = loc->entryMap.find(RTech::StringToGuid(key)); it != loc->entryMap.end())
        {
                return it->second;
        }
    }

    return origKey;
}

static void ItemflavWindow_RefreshData(CUIState* uiState)
{
    CUI_ItemflavWindowData* const flavData = &uiState->itemflavData;

    uiState->itemFlavorListAsset = g_assetData.FindAsset("settings\\base_itemflavors.rpak");

    // If the list asset has now been found, populate the character list
    if (uiState->itemFlavorListAsset)
    {
        if (CAsset* localizationAsset = g_assetData.FindAsset("localization\\localization_english.rpak"))
            ItemflavWindow_LocalizationPostLoadCallback(localizationAsset);

        CPakAsset* const itemflavListAsset = reinterpret_cast<CPakAsset*>(uiState->itemFlavorListAsset);
        const SettingsAsset* const itemflavList = itemflavListAsset->extraData<SettingsAsset*>();

        SettingsKVValue_t* value = nullptr;
        if (itemflavList->GetSettingValue("characters", &value))
        {
            const uint32_t numCharacters = value->numChildren;
            const SettingsKVValue_t* const arrayElems = value->getValue<SettingsKVValue_t*>();

            flavData->characterData.resize(numCharacters);

            for (uint32_t j = 0; j < numCharacters; ++j)
            {
                const SettingsKVField_t* const elemFields = arrayElems[j].getValue<SettingsKVField_t*>();

                assert(arrayElems[j].numChildren > 0);
                if (arrayElems[j].numChildren == 0)
                    break;

                CUI_ItemflavCharacter* const character = &flavData->characterData[j];

                character->assetPath = elemFields[0].getValue<const char*>();
                character->settingsAssetGuid = RTech::StringToGuid(character->assetPath);

                // If the settings asset is already loaded, try and populate the data now
                // Otherwise set up a callback for when it does get loaded
                if (CAsset* settingsAsset = g_assetData.FindAssetByGUID(character->settingsAssetGuid))
                    ItemflavWindow_GetCharacterDataFromSettings(settingsAsset);
                else
                    g_assetData.AddAssetPostLoadCallback(character->settingsAssetGuid, ItemflavWindow_GetCharacterDataFromSettings);
            }
        }

        if (itemflavList->GetSettingValue("weapons", &value))
        {
            const uint32_t numWeapons = value->numChildren;
            const SettingsKVValue_t* const arrayElems = value->getValue<SettingsKVValue_t*>();

            flavData->weaponData.resize(numWeapons);

            for (uint32_t j = 0; j < numWeapons; ++j)
            {
                const SettingsKVField_t* const elemFields = arrayElems[j].getValue<SettingsKVField_t*>();

                assert(arrayElems[j].numChildren > 0);
                if (arrayElems[j].numChildren == 0)
                    break;

                CUI_ItemflavWeapon* const weapon = &flavData->weaponData.at(j);

                weapon->assetPath = elemFields[0].getValue<const char*>();
                weapon->settingsAssetGuid = RTech::StringToGuid(weapon->assetPath);

                // If the settings asset is already loaded, try and populate the data now
                // Otherwise set up a callback for when it does get loaded
                if (CAsset* settingsAsset = g_assetData.FindAssetByGUID(weapon->settingsAssetGuid))
                    ItemflavWindow_GetWeaponDataFromSettings(settingsAsset);
                else
                    g_assetData.AddAssetPostLoadCallback(weapon->settingsAssetGuid, ItemflavWindow_GetWeaponDataFromSettings);
            }
        }

        if (itemflavList->GetSettingValue("meleeWeapons", &value))
        {
            const uint32_t numWeapons = value->numChildren;
            const SettingsKVValue_t* const arrayElems = value->getValue<SettingsKVValue_t*>();

            const size_t numExistingWeapons = flavData->weaponData.size();

            // add on to the existing weapon data for melee weapons
            flavData->weaponData.resize(numWeapons + numExistingWeapons);

            for (uint32_t j = 0; j < numWeapons; ++j)
            {
                const SettingsKVField_t* const elemFields = arrayElems[j].getValue<SettingsKVField_t*>();

                assert(arrayElems[j].numChildren > 0);
                if (arrayElems[j].numChildren == 0)
                    break;

                CUI_ItemflavWeapon* const weapon = &flavData->weaponData.at(j + numExistingWeapons);

                weapon->assetPath = elemFields[0].getValue<const char*>();
                weapon->settingsAssetGuid = RTech::StringToGuid(weapon->assetPath);

                // If the settings asset is already loaded, try and populate the data now
                // Otherwise set up a callback for when it does get loaded
                if (CAsset* settingsAsset = g_assetData.FindAssetByGUID(weapon->settingsAssetGuid))
                    ItemflavWindow_GetWeaponDataFromSettings(settingsAsset);
                else
                    g_assetData.AddAssetPostLoadCallback(weapon->settingsAssetGuid, ItemflavWindow_GetWeaponDataFromSettings);
            }
        }
    }
}

static void ItemflavWnd_LegendsTab(CUIState* uiState)
{
    CUI_ItemflavWindowData* flavData = &uiState->itemflavData;

    if (flavData->localizationAsset)
    {
        if (ImGui::BeginCombo("##characters", flavData->selectedCharacterName.c_str(), ImGuiComboFlags_WidthFitPreview))
        {
            for (int i = 0; i < flavData->characterData.size(); ++i)
            {
                CUI_ItemflavCharacter* const character = &flavData->characterData[i];
                const bool isSelected = i == uiState->itemflavData.selectedCharacterIdx;

                const std::string charName = Localize(character->characterName);

                if (ImGui::Selectable(charName.c_str(), isSelected))
                {
                    flavData->selectedCharacterIdx = i;
                    flavData->selectedCharacterName = charName;
                }

                if (isSelected)
                    ImGui::SetItemDefaultFocus();
            }

            ImGui::EndCombo();
        }

        if (flavData->selectedCharacterIdx != -1)
        {
            // Display a small icon for the currently selected character
            CUI_ItemflavCharacter* character = &flavData->characterData[flavData->selectedCharacterIdx];
            ImGui::SameLine();
            if (character->iconTexture)
            {
                ID3D11ShaderResourceView* const srv = character->iconTexture->GetSRV();
                if (srv)
                {
                    ImGui::Image(srv, ImVec2(25, 25));
                    ImGui::SameLine();
                }
            }

            ImGui::TextUnformatted(Localize(character->characterDesc).c_str());

            ImGui::TextUnformatted("Search"); ImGui::SameLine();

            static ImGuiCustomTextFilter skinFilter;

            // OR case if we load a pak and the filter is not cleared yet.
            if (skinFilter.Draw("##SkinFilter", "incl,-excl", -1.f))
            {
                for (auto& skin : character->skins)
                {
                    if (!skin.localizationKey_NAME) continue;
                    skin.includeInList = (!skinFilter.IsActive()) || skinFilter.PassFilter(Localize(skin.localizationKey_NAME).c_str()) || skinFilter.PassFilter(skin.armsModel) || skinFilter.PassFilter(skin.bodyModel);
                }
            }

            const char* prevPakName = nullptr;

            bool headerOpen = false;
            bool lastTableRet = false;

            size_t i = 0;
            for (auto& skin : character->skins)
            {
                i++; // increment here so we don't have to do it in every continue branch

                SettingsAsset* const skinSettings = reinterpret_cast<CPakAsset*>(skin.settingsAsset)->extraData<SettingsAsset*>();

                // _shared_favorites is not an actual character skin. fuck bakery
                if (skinSettings->uniqueId == CHARSKIN_SHARED_FAVORITES_SAID)
                    continue;

                if (!skin.includeInList && skinFilter.IsActive())
                    continue;

                // If this skin's arms model pak name is different to the one before it, split the following skins into a new group
                if (!prevPakName || _stricmp(skin.armsModelPak, prevPakName))
                {
                    // If we aren't the first table, and the last table returned true (and was open), we need to end the previous table
                    if (prevPakName && lastTableRet && headerOpen)
                    {
                        ImGui::EndTable();

                        // Reset LTR to false so that we don't hit the EndTable call at the end of the loop
                        lastTableRet = false;
                    }

                    // If we aren't the first table, and the previous header was open, add padding before this header 
                    if (prevPakName != nullptr && headerOpen)
                        ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 10.f);

                    headerOpen = ImGui::CollapsingHeader(skin.armsModelPak);

                    // If the user has opened the header, setup the table for this group
                    if (headerOpen)
                    {
                        ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 10.f);

                        ImGui::Text("These skins can be found by opening file '%s':", skin.armsModelPak);

                        ImGui::PushID(static_cast<ImGuiID>(i | 0x10000));
                        if (ImGui::Button("Load Assets"))
                        {
                            // Full pak path for the localization_english.rpak file in the same directory as common.rpak
                            const std::filesystem::path fullPakPath = reinterpret_cast<CPakFile*>(reinterpret_cast<CPakAsset*>(uiState->itemFlavorListAsset)->GetContainerFile())->GetFilePath().parent_path() / skin.armsModelPak;

                            extern void HandlePakLoad(std::vector<std::string> filePaths);

                            CThread(HandlePakLoad, std::vector<std::string>{ fullPakPath.string() }).detach();
                        }
                        ImGui::PopID();

                        lastTableRet = ImGui::BeginTableEx("SkinsTable", static_cast<ImGuiID>(i | 0x8000), 4, ImGuiTableFlags_BordersOuter | ImGuiTableFlags_BordersInner);
                        if (lastTableRet)
                        {
                            ImGui::TableSetupColumn("Quality", ImGuiTableColumnFlags_WidthFixed, 100.f, 0);
                            ImGui::TableSetupColumn("Name", ImGuiTableColumnFlags_WidthFixed, 0, 1);
                            ImGui::TableSetupColumn("Arms Model", ImGuiTableColumnFlags_WidthFixed, 0, 2);
                            ImGui::TableSetupColumn("Body Model", ImGuiTableColumnFlags_WidthFixed, 0, 3);

                            ImGui::TableSetupScrollFreeze(0, 1);
                            ImGui::TableHeadersRow();
                        }
                    }
                }

                ImGui::PushID(static_cast<int>(i));

                if (headerOpen)
                {
                    ImGui::TableNextRow();

                    const std::string skinName = Localize(skin.localizationKey_NAME);

                    if (ImGui::TableSetColumnIndex(0))
                    {
                        ImGui::PushStyleColor(ImGuiCol_Text, s_qualityColours.at(skin.quality[0]));

                        ImGui::TextUnformatted(skin.quality);

                        ImGui::PopStyleColor();
                    }

                    if (ImGui::TableSetColumnIndex(1))
                        ImGui::Selectable(skinName.c_str(), false, ImGuiSelectableFlags_SpanAllColumns);

                    if (ImGui::TableSetColumnIndex(2))
                        ImGui::TextUnformatted(GetStringAfterLastSlash(skin.armsModel));

                    if (ImGui::TableSetColumnIndex(3))
                        ImGui::TextUnformatted(GetStringAfterLastSlash(skin.bodyModel));
                }

                ImGui::PopID();

                prevPakName = skin.armsModelPak;
            }

            // If the last table was opened successfully, end it!
            if (lastTableRet && headerOpen) ImGui::EndTable();
        }
    }
}

static void ItemflavWnd_WeaponsTab(CUIState* uiState)
{
    CUI_ItemflavWindowData* flavData = &uiState->itemflavData;

    if (flavData->localizationAsset)
    {
        if (ImGui::BeginCombo("##weapons", flavData->selectedWeaponName.c_str(), ImGuiComboFlags_WidthFitPreview))
        {
            for (int i = 0; i < flavData->weaponData.size(); ++i)
            {
                CUI_ItemflavWeapon* const weapon = &flavData->weaponData.at(i);
                const bool isSelected = i == uiState->itemflavData.selectedWeaponIdx;

                const std::string charName = std::format("{}{}", weapon->isMelee ? "[MELEE] " : "", Localize(weapon->weaponName));

                if (ImGui::Selectable(charName.c_str(), isSelected))
                {
                    flavData->selectedWeaponIdx = i;
                    flavData->selectedWeaponName = charName;
                }

                ImGui::SetItemTooltip("%s", weapon->weaponClassname);

                if (isSelected)
                    ImGui::SetItemDefaultFocus();
            }

            ImGui::EndCombo();
        }

        if (flavData->selectedWeaponIdx != -1)
        {
            CUI_ItemflavWeapon* weapon = &flavData->weaponData[flavData->selectedWeaponIdx];
            ImGui::SameLine();
            ImGui::TextUnformatted(Localize(weapon->weaponDesc).c_str());

            ImGui::TextUnformatted("Search"); ImGui::SameLine();

            static ImGuiCustomTextFilter skinFilter;

            // OR case if we load a pak and the filter is not cleared yet.
            if (skinFilter.Draw("##SkinFilter", "incl,-excl", -1.f))
            {
                for (auto& skin : weapon->skins)
                {
                    if (!skin.localizationKey_NAME) continue;
                    skin.includeInList = (!skinFilter.IsActive()) || skinFilter.PassFilter(Localize(skin.localizationKey_NAME).c_str()) || skinFilter.PassFilter(skin.viewModel) || skinFilter.PassFilter(skin.worldModel);
                }
            }

            if (ImGui::BeginTable("WeaponSkinsTable", 4, ImGuiTableFlags_BordersOuter | ImGuiTableFlags_BordersInner))
            {
                ImGui::TableSetupColumn("Quality", ImGuiTableColumnFlags_WidthFixed, 100.f, 0);
                ImGui::TableSetupColumn("Name", ImGuiTableColumnFlags_WidthFixed, 0, 1);
                ImGui::TableSetupColumn("View Model", ImGuiTableColumnFlags_WidthFixed, 0, 2);
                ImGui::TableSetupColumn("World Model", ImGuiTableColumnFlags_WidthFixed, 0, 3);

                ImGui::TableSetupScrollFreeze(0, 1);
                ImGui::TableHeadersRow();

                size_t i = 0;
                for (auto& skin : weapon->skins)
                {
                    i++; // increment here so we don't have to do it in every continue branch

                    SettingsAsset* const skinSettings = reinterpret_cast<CPakAsset*>(skin.settingsAsset)->extraData<SettingsAsset*>();

                    if (skinSettings->uniqueId == WEPSKIN_SHARED_FAVORITES_SAID)
                        continue;

                    if (!skin.includeInList && skinFilter.IsActive())
                        continue;

                    ImGui::PushID(static_cast<int>(i));

                    ImGui::TableNextRow();

                    const std::string skinName = Localize(skin.localizationKey_NAME);

                    if (ImGui::TableSetColumnIndex(0))
                    {
                        ImGui::PushStyleColor(ImGuiCol_Text, s_qualityColours.at(skin.quality[0]));

                        ImGui::TextUnformatted(skin.quality);

                        ImGui::PopStyleColor();
                    }

                    if (ImGui::TableSetColumnIndex(1))
                        ImGui::Selectable(skinName.c_str(), false, ImGuiSelectableFlags_SpanAllColumns);

                    if (ImGui::TableSetColumnIndex(2))
                        ImGui::TextUnformatted(GetStringAfterLastSlash(skin.viewModel));

                    if (ImGui::TableSetColumnIndex(3))
                        ImGui::TextUnformatted(GetStringAfterLastSlash(skin.worldModel));

                    ImGui::PopID();
                }
                ImGui::EndTable();
            }
        }
    }
}

void ItemflavWnd_Draw(CUIState* uiState)
{
    ImGui::SetNextWindowSize(ImVec2(0.f, 500.f), ImGuiCond_Always);
    if (ImGui::Begin("Skin Finder", &uiState->itemflavWindowVisible, ImGuiWindowFlags_NoCollapse))
    {
        ImGui::Text("This menu contains a list of all registered cosmetic items associated with each game character.\n"
            "For all features to work properly, RSX must load common.rpak, common_early.rpak, and localization_english.rpak");

        CUI_ItemflavWindowData* flavData = &uiState->itemflavData;

        // Always render the refresh button, but also try to grab everything the first time the window is shown
        if ((ImGui::Button("Refresh data") || !flavData->triedToInitialise) && g_assetData.m_donePostLoad)
        {
            CThread([uiState]() {
                uiState->isLoading = true;
                ItemflavWindow_RefreshData(uiState);
                uiState->isLoading = false;
            }).detach();
            

            flavData->triedToInitialise = true;
        }

        if (!flavData->triedToInitialise || uiState->isLoading)
            ImGui::TextDisabled("Gathering data...");
        else if (ImGui::BeginTabBar("##SkinFinderTabs"))
        {
            if (ImGui::BeginTabItem("Legends"))
            {
                ItemflavWnd_LegendsTab(uiState);

                ImGui::EndTabItem();
            }

            if (ImGui::BeginTabItem("Weapons"))
            {
                ItemflavWnd_WeaponsTab(uiState);
                ImGui::EndTabItem();
            }

            ImGui::EndTabBar();
        }
    }
    ImGui::End();
}
