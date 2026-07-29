#pragma once
#include "ValveFileVDF/vdf_parser.hpp"

const std::unordered_map<std::string, const char*> STEAM_APP_IDS = {
	//{"1454890", "Titanfall"}, // Titanfall - we're not quite there yet with R1 support so let's hold off for now!
	{"1237970", "Titanfall2"}, // Titanfall 2
	{"1172470", "Apex Legends"}, // Apex Legends
};

enum class GameFinderGame_e : uint32_t
{
	INVALID = 0,

	TITANFALL_1, // erm actually it's called Titanfall
	TITANFALL_2,

	APEX_LEGENDS = 5, // i can be very funny sometimes
};

enum class GameDistributionPlatform_e : uint32_t
{
	STEAM,
	EA
};

struct GameFinderResults_s
{
	struct GameDescriptor_s
	{
		std::filesystem::path gamePath;
		GameFinderGame_e gameType;
		GameDistributionPlatform_e gameDistributionPlatform;
	};

	std::vector<GameDescriptor_s> gameDescriptors;
};

inline void GameFinder_AddGameTypeFromFiles(GameFinderResults_s::GameDescriptor_s* const gameDescriptor)
{
	// Discover the type of game! Sure I could just use the steam app id to determine this
	// but like what if I didn't do that

	/*
	if (std::filesystem::exists(gameDescriptor->gamePath / "Titanfall.exe"))
	{
		gameDescriptor->gameType = GameFinderGame_e::TITANFALL_1;
	}
	else
	*/
	if (std::filesystem::exists(gameDescriptor->gamePath / "Titanfall2.exe"))
	{
		gameDescriptor->gameType = GameFinderGame_e::TITANFALL_2;
	}
	else if (std::filesystem::exists(gameDescriptor->gamePath / "r5apexdata.bin"))
	{
		gameDescriptor->gameType = GameFinderGame_e::APEX_LEGENDS;
	}
}

inline bool GameFinder_FindAllCompatibleSteamGames(GameFinderResults_s* const results)
{
	assert(results);

	wchar_t buf[1024];
	DWORD bufSize = ARRAYSIZE(buf);

	if (RegGetValueW(HKEY_CURRENT_USER, L"Software\\Valve\\Steam", L"SteamPath", RRF_RT_ANY, NULL, &buf, &bufSize))
	{
		Log("GameFinder: Failed to locate steam installation\n");
		return false;
	}

	std::filesystem::path steamInstallationPath = buf;
	
	if (!std::filesystem::exists(steamInstallationPath / "steamapps" / "libraryfolders.vdf"))
	{
		Log("GameFinder: Failed to locate libraryfolders.vdf within steamapps directory\n");
		return false;
	}

	std::ifstream lfStream(steamInstallationPath / "steamapps" / "libraryfolders.vdf", std::ios::in);

	if (!lfStream.is_open())
	{
		Log("GameFinder: Failed to open libraryfolders.vdf\n");
		return false;
	}

	/*
	"libraryfolders"
	{
		"0"
		{
			"path" "C:\\Program Files (x86)\\Steam"
			// ...
			"apps"
			{
				"appid1" "size"
				"appid2" "size"
				// ...
			}
		}
	}
	*/

	auto root = tyti::vdf::read(lfStream);

	assert(root.name == "libraryfolders");

	for (auto& child : root.childs)
	{
		assert(child.second->childs.contains("apps"));

		std::filesystem::path libraryPath = child.second->attribs["path"];
		libraryPath /= "steamapps\\common";

		for (auto& app : child.second->childs["apps"]->attribs)
		{
			if (auto it = STEAM_APP_IDS.find(app.first); it != STEAM_APP_IDS.end())
			{
				std::filesystem::path gamePath = libraryPath / it->second;
				if (std::filesystem::exists(gamePath))
				{
					GameFinderResults_s::GameDescriptor_s gameDescriptor;
					gameDescriptor.gamePath = gamePath;
					gameDescriptor.gameDistributionPlatform = GameDistributionPlatform_e::STEAM;
					GameFinder_AddGameTypeFromFiles(&gameDescriptor);

					results->gameDescriptors.emplace_back(gameDescriptor);
				}
			}
		}
	}


	return true;
}

inline bool GameFinder_FindAllCompatibleEAGames(GameFinderResults_s* const results)
{
	assert(results);

	constexpr std::wstring_view EAInstallerDataNames[] =
	{
		L"Titanfall2",
		L"Apex"
	};

	HKEY parentKey;
	if (RegOpenKeyExW(HKEY_LOCAL_MACHINE, L"Software\\Respawn", 0, KEY_READ, &parentKey) != ERROR_SUCCESS)
	{
		Log("GameFinder: No Software\\Respawn subkey.\n");
		return false;
	}

	wchar_t subKeyName[MAX_PATH];
	DWORD subKeyNameLen = ARRAYSIZE(subKeyName);

	for (DWORD idx = 0;; ++idx)
	{
		// RegEnumKeyExW expects the subKeyNameLen to be the initial size of subKeyName.
		subKeyNameLen = ARRAYSIZE(subKeyName);

		LSTATUS result = RegEnumKeyExW(parentKey, idx, subKeyName, &subKeyNameLen, nullptr, nullptr, nullptr, nullptr);
		if (result == ERROR_NO_MORE_ITEMS)
		{
			break;
		}

		if (result != ERROR_SUCCESS)
		{
			Log("GameFinder: Unexpected result in regkey enumeration of %ld.\n", result );
			break;
		}

		const auto it = std::ranges::find(EAInstallerDataNames, subKeyName);
		if (it != std::end(EAInstallerDataNames))
		{
			HKEY subKey;
			if (result = RegOpenKeyExW(parentKey, subKeyName, 0, KEY_READ, &subKey);
				result != ERROR_SUCCESS)
			{
				Log("GameFinder: Opening subkey %lc failed with %ld\n", subKeyName, result);
				return false;
			}

			wchar_t installDir[MAX_PATH];
			DWORD installDirLen = ARRAYSIZE(installDir);
			if (result = RegGetValueW(subKey, nullptr, L"Install Dir", RRF_RT_REG_SZ, nullptr, installDir, &installDirLen);
				result != ERROR_SUCCESS)
			{
				Log("GameFinder: Failed to get 'Install Dir' string from %lc, failing with %ld\n", subKeyName, result);
				return false;
			}

			std::filesystem::path gamePath = installDir;
			if (std::filesystem::exists(gamePath))
			{
				GameFinderResults_s::GameDescriptor_s gameDescriptor;
				gameDescriptor.gamePath = gamePath;
				gameDescriptor.gameDistributionPlatform = GameDistributionPlatform_e::EA;
				GameFinder_AddGameTypeFromFiles(&gameDescriptor);

				results->gameDescriptors.emplace_back(gameDescriptor);
			}

			RegCloseKey(subKey);
		}
	}
	RegCloseKey(parentKey);

	return true;
}