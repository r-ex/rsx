#pragma once
#include <core/update/update.h>

struct CUI_ItemflavCharacterSkin
{
	const char* assetPath;

	const char* localizationKey_NAME;
	const char* quality;

	const char* armsModel;
	const char* bodyModel;

	const char* armsModelPak;
	const char* bodyModelPak;

	void* settingsAsset;

	uint32_t qualityIndex;
	bool includeInList;
};
class CTexture;
struct CUI_ItemflavCharacter
{
	std::shared_ptr<CTexture> iconTexture;

	const char* assetPath;

	uint64_t settingsAssetGuid;
	void* settingsAsset;

	const char* characterName;
	const char* characterDesc;
	const char* shippingStatus;
	const char* icon;

	std::vector<CUI_ItemflavCharacterSkin> skins;
};

struct CUI_ItemflavWeaponSkin
{
	const char* assetPath;

	const char* localizationKey_NAME;
	const char* quality;

	const char* viewModel;
	const char* worldModel;

	void* settingsAsset;

	uint32_t qualityIndex;
	bool includeInList;
};

struct CUI_ItemflavWeapon
{
	std::shared_ptr<CTexture> iconTexture;

	const char* assetPath;

	uint64_t settingsAssetGuid;
	void* settingsAsset;

	const char* weaponName;
	const char* weaponDesc;
	const char* weaponClassname;
	const char* icon;

	std::vector<CUI_ItemflavWeaponSkin> skins;

	bool shippingWeapon;
	bool isMelee;
};

struct CUI_ItemflavWindowData
{
	bool triedToInitialise;

	std::vector<CUI_ItemflavCharacter> characterData;
	std::vector<CUI_ItemflavWeapon> weaponData;

	void* localizationAsset;

	std::string selectedCharacterName;
	std::string selectedWeaponName;

	int selectedCharacterIdx;
	int selectedWeaponIdx;

	void Reset()
	{
		triedToInitialise = false;

		localizationAsset = nullptr;
		selectedCharacterName = "(none)";
		selectedWeaponName = "(none)";
		selectedCharacterIdx = -1;
		selectedWeaponIdx = -1;

		characterData.clear();
		weaponData.clear();
	}
};

class CUIState
{
public:
	inline void ShowSettingsWindow(bool state) { settingsWindowVisible = state; };
	inline void ShowItemflavWindow(bool state) { itemflavWindowVisible = state; };
	inline void ShowLogWindow(bool state) { logWindowVisible = state; };

	inline void ClearAssetData()
	{
		itemFlavorListAsset = nullptr;

		itemflavData.Reset();

		itemflavWindowVisible = false;
	}

	CUIState() : settingsWindowVisible(false), itemflavWindowVisible(false),
		logWindowVisible(false), sceneWindowHovered(false), itemFlavorListAsset(nullptr),
		itemflavData{}
	{

		itemflavData.Reset();
	}

public:
	bool settingsWindowVisible;
	bool itemflavWindowVisible;
	bool logWindowVisible;
	bool sceneWindowHovered;

	void* itemFlavorListAsset;

	CUI_ItemflavWindowData itemflavData;

	const char* newVersionType;
	GitHubReleaseInfo_s newVersionReleaseInfo;

	std::atomic<bool> isLoading;
};