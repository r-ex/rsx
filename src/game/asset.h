// This file is made available as part of the reSource Xtractor (RSX) asset extractor
// Licensed under AGPLv3. Details available at https://github.com/r-ex/rsx/blob/main/LICENSE

#pragma once
#include <string>
#include <game/rtech/utils/utils.h>
#include <filesystem>
#include <iomanip>
#include <time.h>

extern ExportSettings_t g_ExportSettings;

enum class AssetType_t
{
	// model
	// pak
	MDL_ = MAKEFOURCC('m', 'd', 'l', '_'),
	ARIG = MAKEFOURCC('a', 'r', 'i', 'g'),
	ASEQ = MAKEFOURCC('a', 's', 'e', 'q'),
	ASQD = MAKEFOURCC('a', 's', 'q', 'd'),
	ANIR = MAKEFOURCC('a', 'n', 'i', 'r'),
	// non pak
	MDL  = MAKEFOURCC('m', 'd', 'l', '\0'),	
	SEQ  = MAKEFOURCC('s', 'e', 'q', '\0'),

	// texture/material
	MATL = MAKEFOURCC('m', 'a', 't', 'l'),
	MSNP = MAKEFOURCC('m', 's', 'n', 'p'),
	MT4A = MAKEFOURCC('m', 't', '4', 'a'),
	TXTR = MAKEFOURCC('t', 'x', 't', 'r'),
	TXAN = MAKEFOURCC('t', 'x', 'a', 'n'),
	TXLS = MAKEFOURCC('t', 'x', 'l', 's'),
	TXTX = MAKEFOURCC('t', 'x', 't', 'x'),
	UIMG = MAKEFOURCC('u', 'i', 'm', 'g'),
	UIIA = MAKEFOURCC('u', 'i', 'i', 'a'),
	FONT = MAKEFOURCC('f', 'o', 'n', 't'),

	// particle (texture)
	EFCT = MAKEFOURCC('e', 'f', 'c', 't'),
	RPSK = MAKEFOURCC('r', 'p', 's', 'k'),

	// dx/shader
	SHDR = MAKEFOURCC('s', 'h', 'd', 'r'),
	SHDS = MAKEFOURCC('s', 'h', 'd', 's'),

	// ui
	UI   = MAKEFOURCC('u', 'i', '\0', '\0'),
	HSYS = MAKEFOURCC('h', 's', 'y', 's'),
	RLCD = MAKEFOURCC('r', 'l', 'c', 'd'),
	RTK  = MAKEFOURCC('r', 't', 'k', '\0'),

	// pak
	PTCH = MAKEFOURCC('P', 't', 'c', 'h'),
	VERS = MAKEFOURCC('v', 'e', 'r', 's'), // patch version

	// descriptors (stats, specs, etc)
	DTBL = MAKEFOURCC('d', 't', 'b', 'l'),
	STGS = MAKEFOURCC('s', 't', 'g', 's'),
	STLT = MAKEFOURCC('s', 't', 'l', 't'),
	RSON = MAKEFOURCC('r', 's', 'o', 'n'),
	SUBT = MAKEFOURCC('s', 'u', 'b', 't'),
	LOCL = MAKEFOURCC('l', 'o', 'c', 'l'),

	// vpk
	WRAP = MAKEFOURCC('w', 'r', 'a', 'p'),
	WEPN = MAKEFOURCC('w', 'e', 'p', 'n'),
	IMPA = MAKEFOURCC('i', 'm', 'p', 'a'),

	// map
	RMAP = MAKEFOURCC('r', 'm', 'a', 'p'),
	LLYR = MAKEFOURCC('l', 'l', 'y', 'r'),

	// odl
	ODLA = MAKEFOURCC('o', 'd', 'l', 'a'),
	ODLC = MAKEFOURCC('o', 'd', 'l', 'c'),
	ODLP = MAKEFOURCC('o', 'd', 'l', 'p'),

	// unknown
	REFM = MAKEFOURCC('r', 'e', 'f', 'm'), // 'refm' "refmap/grx/grx_refs.rpak"
	HCXT = MAKEFOURCC('h', 'c', 'x', 't'), // 'hcxt' ?

	// audio
	ASRC = MAKEFOURCC('a', 's', 'r', 'c'), // source
	AEVT = MAKEFOURCC('a', 'e', 'v', 't'), // event

	// bluepoint pak
	BPWF = MAKEFOURCC('b', 'p', 'w', 'f'),
};

static std::map<AssetType_t, Color4> s_AssetTypeColours =
{
	// only color assets we support (for export or preview)!

	// model (reds)
	// pak
	{ AssetType_t::MDL_, Color4(240, 60,  50) },
	{ AssetType_t::ARIG, Color4(220, 75,  10) },
	{ AssetType_t::ASEQ, Color4(220, 75, 109) },
	{ AssetType_t::ASQD, Color4(200, 90, 150) },
	{ AssetType_t::ANIR, Color4(200, 100, 130) },
	// non pak
	{ AssetType_t::MDL,  Color4(240, 60,  50) },
	{ AssetType_t::SEQ,	 Color4(220, 75, 109) },

	// texture/ui (blues)
	{ AssetType_t::MATL, Color4(26,  122, 138) },
	{ AssetType_t::MSNP, Color4(70,  120, 180) },
	// mt4a
	{ AssetType_t::TXTR, Color4(0,   106, 255) },
	{ AssetType_t::TXAN, Color4(100, 175, 200) },
	{ AssetType_t::TXLS, Color4(75,  140, 240) },
	// txtx
	{ AssetType_t::UIMG, Color4(114, 142, 230) },
	{ AssetType_t::UIIA, Color4(114, 142, 230) },
	{ AssetType_t::FONT, Color4(100, 130, 200) },

	// particle (texture)
	//{ AssetType_t::EFCT, Color4(9,   222, 192) },
	// rpsk

	// dx/shader (orples)
	{ AssetType_t::SHDS, Color4(130, 111, 151) },
	{ AssetType_t::SHDR, Color4(168, 142, 171) },

	// ui (greens)
	{ AssetType_t::UI,   Color4(25,  180,  25) },
	// hsys
	// rlcd
	{ AssetType_t::RTK,  Color4(114, 197, 130) },

	// pak
	// Ptch
	// vers

	// descriptors (yellows)
	{ AssetType_t::DTBL, Color4(220, 196, 0) },
	{ AssetType_t::STGS, Color4(255, 196, 0) },
	{ AssetType_t::STLT, Color4(255, 127, 0) },
	{ AssetType_t::RSON, Color4(200, 150, 25) },
	{ AssetType_t::SUBT, Color4(240, 130, 15) },
	{ AssetType_t::LOCL, Color4(225, 145, 10) },

	// vpk (pinks)
	{ AssetType_t::WRAP, Color4(250, 120, 120) },
	{ AssetType_t::WEPN, Color4(255, 122, 204) },
	{ AssetType_t::IMPA, Color4(255,  50, 220) },

	// map
	//{ PakAssetType_t::RMAP, Color4(131 ,69, 255) },
	// llyr

	// odl
	// odla
	// odlc
	// odlp

	// audio
	{ AssetType_t::ASRC, Color4(91,  52, 252) },
	{ AssetType_t::AEVT, Color4(91,  52, 252) },

	// bluepoint
	// bpwf
};


static const std::map<AssetType_t, const char*> s_AssetTypePaths =
{
	// model
	{ AssetType_t::MDL_, "mdl" },
	{ AssetType_t::ARIG, "animrig" },
	{ AssetType_t::ASEQ, "animseq" },
	{ AssetType_t::ASQD, "animseq_data" },
	{ AssetType_t::ANIR, "anim_recording" },
	{ AssetType_t::MDL, "models" },
	{ AssetType_t::SEQ, "models" },

	// texture/ui
	{ AssetType_t::MATL, "material" },
	{ AssetType_t::MSNP, "material_snapshot" }, // "material snapshot" 
	// mt4a "material for aspect"
	{ AssetType_t::TXTR, "texture" },
	{ AssetType_t::TXAN, "texture_anim" },
	{ AssetType_t::TXLS, "texture_list" },
	{ AssetType_t::TXLS, "texture_extension" },
	{ AssetType_t::UIMG, "ui_image_atlas" },
	{ AssetType_t::UIIA, "ui_image" },
	{ AssetType_t::FONT, "ui_font_atlas" },

	// particle (texture)
	{ AssetType_t::EFCT, "effect" },
	{ AssetType_t::RPSK, "particle_script" },

	// dx/shader
	{ AssetType_t::SHDS, "shaderset" },
	{ AssetType_t::SHDR, "shader" },

	// ui
	{ AssetType_t::RTK,  "ui_rtk" },
	{ AssetType_t::HSYS, "highlight_system" },
	{ AssetType_t::RLCD, "lcd_screen_effect" },
	{ AssetType_t::UI,   "ui" },

	// pak
	{ AssetType_t::PTCH, "patch_master" },
	{ AssetType_t::VERS, "patch_version" },
	// Ptch "Pak Patch"
	// vers "Pak Version"

	// descriptors
	{ AssetType_t::DTBL, "datatable" },
	{ AssetType_t::STGS, "settings" },
	{ AssetType_t::STLT, "settings_layout" },
	{ AssetType_t::RSON, "rson" }, // 'maps'/'scripts'
	{ AssetType_t::SUBT, "subtitles" },
	{ AssetType_t::LOCL, "localization" },

	// vpk
	//{ AssetType_t::WRAP, nullptr },
	{ AssetType_t::WEPN, "weapon_definition" },
	{ AssetType_t::IMPA, "impact" },
	
	// map
	{ AssetType_t::RMAP, "map" },
	{ AssetType_t::LLYR, "llayer" },

	// odl
	{ AssetType_t::ODLA, "odl_asset" },
	{ AssetType_t::ODLC, "odl_ctx" },
	{ AssetType_t::ODLP, "odl_pak" },
};

struct AssetVersion_t
{
	AssetVersion_t() : majorVer(0), minorVer(0) {};
	AssetVersion_t(int majorVersion) : majorVer(majorVersion), minorVer(0) {};
	AssetVersion_t(int majorVersion, int minorVersion) : majorVer(majorVersion), minorVer(minorVersion) {};

	int majorVer;
	int minorVer;

	std::string ToString() const
	{
		return minorVer == 0 ? std::format("v{}", majorVer) : std::format("v{}.{}", majorVer, minorVer);
	}
};

static const char* s_LogLevelNames[] = {
	"INFO",
	"WARNING",
	"ERROR",
};

static uint32_t s_LogLevelColours[] = {
	0xFFFFFFFF,
	0xFFFF00FF,
	0xFF0000FF,
};

struct ContainerMessage_t
{
	enum class MessageType_e : int8_t
	{
		MSG_INVALID = -1,
		MSG_INFO = 0,
		MSG_WARNING,
		MSG_ERROR,
	};

	ContainerMessage_t(MessageType_e _type, const char* _sourceName, const char* _message) : type(_type), sourceName(_sourceName), message(_message) {};

	~ContainerMessage_t()
	{
		free((void*)timestampStr);
		free((void*)sourceName);
		free((void*)message);
	}

	const char* timestampStr;
	const char* sourceName;
	const char* message;
	MessageType_e type;
};

class CAssetContainer;

class CAsset
{
public:

	enum ContainerType
	{
		PAK,
		MDL,
		AUDIO,
		BP_PAK,


		_COUNT
	};

	virtual ~CAsset() {};

	// Get the display name for this asset.
	const std::string& GetAssetName() const { return m_assetName; }
	const AssetVersion_t& GetAssetVersion() const { return m_assetVersion; }
	virtual const uint64_t GetAssetGUID() const = 0;

	// Get the type of container file that this asset came from.
	virtual const ContainerType GetAssetContainerType() const = 0;

	// Get the type of this asset within the container file. (e.g., model, material, sound)
	virtual uint32_t GetAssetType() const = 0;

	const bool GetExportedStatus() const { return m_exported; }

	const bool GetPostLoadStatus() const { return m_postLoadStatus; }

	void* GetAssetData() const { return m_assetData; }

	// Get the name of the file that contains this asset.
	virtual std::string GetContainerFileName() const = 0;

	void* GetContainerFile() const { return m_containerFile; }

	template <typename T>
	T* GetContainerFile() const { return static_cast<T*>(m_containerFile); }

	// setters

	void SetAssetName(const std::string& name, bool addToCache=false)
	{
		m_assetName = std::filesystem::path(name).make_preferred().string();

		if (addToCache)
			g_cacheDBManager.Add(name);
	};

	void SetAssetNameFromCache()
	{
		if (g_ExportSettings.disableCachedNames)
			return;

		if (auto entry = g_cacheDBManager.TryGetEntry(GetAssetGUID()))
		{
			m_assetName = std::filesystem::path(entry->origString)
				.make_preferred()
				.string();
		}
	}

	void SetAssetVersion(const AssetVersion_t& version)
	{
		m_assetVersion = version;
	}

	//void SetAssetName(const std::filesystem::path& path)
	//{
	//	m_assetName = std::filesystem::path(path).make_preferred().string();
	//}

	// Set the pointer to the internal asset data that was used for creating this asset.
	void SetInternalAssetData(void* ptr)
	{
		m_assetData = ptr;
	}

	// Set the pointer to the class of the file that contains this asset.
	void SetContainerFile(CAssetContainer* ptr)
	{
		m_containerFile = ptr;
	}

	void SetExportedStatus(bool exported)
	{
		m_exported = exported;
	}

	// TODO TODO TODO:
	// init this as false on bpk/bsp/audio/mdl!
	void SetPostLoadStatus(bool postLoaded)
	{
		m_postLoadStatus = postLoaded;
	}

private:
	std::string m_assetName;
	AssetVersion_t m_assetVersion;

	bool m_exported;

	bool m_postLoadStatus;

protected:
	void* m_assetData;

	void* m_containerFile;
};

class CAssetContainer
{
public:
	typedef CAsset::ContainerType ContainerType;

	virtual ~CAssetContainer() {};

	virtual const ContainerType GetContainerType() const = 0;

	void SetFilePath(const std::filesystem::path& filePath)
	{
		m_filePath = filePath;
	}

	const std::filesystem::path& GetFilePath() const
	{
		return m_filePath;
	}

private:
	std::filesystem::path m_filePath;

};

// functions for asset loading.
typedef void(*AssetLoadFunc_t)(CAssetContainer* container, CAsset* asset);

typedef void(*AssetLoadCallback_t)(CAsset* asset);

// functions for previewing the asset
typedef void* (*AssetPreviewFunc_t)(CAsset* const asset, const bool firstFrameForAsset);

// functions around exporting the asset.
typedef bool(*AssetExportFunc_t)(CAsset* const asset, const int setting);

#define REGISTER_TYPE(type) g_assetData.m_assetTypeBindings[type.type] = type

struct AssetTypeBinding_t
{
	const char* name;
	uint32_t type;
	uint32_t headerAlignment;
	AssetLoadFunc_t loadFunc; // void(*AssetLoadFunc_t)(CAssetContainer* container, CAsset* asset);
	AssetLoadFunc_t postLoadFunc; // void(*AssetLoadFunc_t)(CAssetContainer* container, CAsset* asset);
	AssetPreviewFunc_t previewFunc; // void* (*AssetPreviewFunc_t)(CAsset* const asset, const bool firstFrameForAsset);

	struct
	{
		AssetExportFunc_t exportFunc;
		int exportSetting;
		const char** exportSettingArr;
		size_t exportSettingArrSize;
	} e;
};

class CGlobalAssetData
{
public:
	std::vector<CAssetContainer*> v_assetContainers;

	struct AssetLookup_t
	{
		uint64_t m_guid;
		CAsset* m_asset;
	};

	std::vector<AssetLookup_t> v_assets;
	std::map<uint32_t, AssetTypeBinding_t> m_assetTypeBindings;

	// map of pak crc to status of whether the pak has already been loaded
	std::unordered_map<uint64_t, bool> m_pakLoadStatusMap;

	std::unordered_map<std::string, uint8_t> m_patchMasterEntries;

	std::unordered_map<uint64_t, std::unordered_set<AssetLoadCallback_t>> m_assetPostLoadCallbacks;

	CAssetContainer* m_pakPatchMaster;

	ContainerMessage_t* m_logMessages;
	uint32_t m_numLogMessages;

	bool m_donePostLoad : 1;
	bool m_doneLoad : 1;

	void AddAssetPostLoadCallback(uint64_t guid, AssetLoadCallback_t callback)
	{
		if (m_assetPostLoadCallbacks.find(guid) == m_assetPostLoadCallbacks.end())
			m_assetPostLoadCallbacks.emplace(guid, std::unordered_set<AssetLoadCallback_t>());

		m_assetPostLoadCallbacks[guid].insert(callback);
	}

	CAsset* const FindAssetByGUID(const uint64_t guid)
	{
		const auto it = std::ranges::find(v_assets, guid, &AssetLookup_t::m_guid);
		return it != v_assets.end()
			? it->m_asset : nullptr;
	}

	template<typename T>
	T* const FindAssetByGUID(const uint64_t guid)
	{
		const auto it = std::ranges::find(v_assets, guid, &AssetLookup_t::m_guid);
		return it != v_assets.end()
			&& it->m_asset->GetAssetContainerType() == CAsset::ContainerType::PAK
			? static_cast<T*>(it->m_asset) : nullptr;
	}

	void ClearAssetData()
	{
		for (const auto& lookup : v_assets)
		{
			delete lookup.m_asset;
		}
		v_assets.clear();
		v_assets.shrink_to_fit();

		for (CAssetContainer* container : v_assetContainers)
		{
			delete container;
		}
		v_assetContainers.clear();
		v_assetContainers.shrink_to_fit();

		if (m_pakPatchMaster)
		{
			delete m_pakPatchMaster;
			m_pakPatchMaster = NULL;
		}

		m_patchMasterEntries.clear();
		m_pakLoadStatusMap.clear();

		m_donePostLoad = false;
	}

	void ProcessAssetsPostLoad();



#define GET_LOG_MSG_VARIADIC(args, returnVar, fmt) std::vector<char> buf(1+std::vsnprintf(NULL, 0, fmt, args)); \
											   va_list args2; \
											   va_copy(args2, args); \
											   va_end(args); \
											   std::vsnprintf(buf.data(), buf.size(), fmt, args2); \
											   va_end(args2); \
											   returnVar = std::string(buf.begin(), buf.end())
	// Logging functions
	void Log_Info(const CAssetContainer* const container, const char* fmt, ...)
	{
		va_list args;
		va_start(args, fmt);

		std::string msg;
		GET_LOG_MSG_VARIADIC(args, msg, fmt);

		const std::string sourceName = container ? container->GetFilePath().filename().string() : "N/A";

		Log("[%s] %s\n", sourceName.c_str(), msg.c_str());
		LogMessages_Append(ContainerMessage_t::MessageType_e::MSG_INFO, sourceName, msg);
	}

	void Log_Warning(const CAssetContainer* const container, const char* fmt, ...)
	{
		va_list args;

		va_start(args, fmt);
		
		std::string msg;
		GET_LOG_MSG_VARIADIC(args, msg, fmt);

		const std::string sourceName = container ? container->GetFilePath().filename().string() : "N/A";

		Log("WARNING [%s]: %s\n", sourceName.c_str(), msg.c_str());

		LogMessages_Append(ContainerMessage_t::MessageType_e::MSG_WARNING, sourceName, msg);
	}

	void Log_Error(const CAssetContainer* const container, const char* fmt, ...)
	{
		va_list args;
		va_start(args, fmt);

		std::string msg;
		GET_LOG_MSG_VARIADIC(args, msg, fmt);

		const std::string sourceName = container ? container->GetFilePath().filename().string() : "N/A";

		Log("ERROR [%s]: %s\n", sourceName.c_str(), msg.c_str());

		LogMessages_Append(ContainerMessage_t::MessageType_e::MSG_ERROR, sourceName, msg);
	}

	const ContainerMessage_t* GetLogMessages() const
	{
		return m_logMessages;
	}

	const uint32_t GetNumLogMessages() const
	{
		return m_numLogMessages;
	}

	void LogMessages_Append(ContainerMessage_t::MessageType_e type, const std::string& sourceName, const std::string& msg)
	{
		// If there's no log messages ptr, try and allocate a placeholder so we can write something into the array
		if (!m_logMessages)
			m_logMessages = reinterpret_cast<ContainerMessage_t*>(calloc(0, sizeof(ContainerMessage_t)));

		// If there's still no log messages ptr just return since logs are not critical
		if (!m_logMessages)
			return;

		m_logMessages = reinterpret_cast<ContainerMessage_t*>(realloc(m_logMessages, sizeof(ContainerMessage_t) * (m_numLogMessages + 1)));

		const time_t t = std::time(nullptr);
		tm tm;
		if (localtime_s(&tm, &t))
		{
			assertm(0, "failed to get time");
			return;
		}

		std::ostringstream oss;
		oss << std::put_time(&tm, "%H:%M:%S");

		ContainerMessage_t* m = &m_logMessages[m_numLogMessages];
		m->type = type;
		m->timestampStr = _strdup(oss.str().c_str());
		m->message = _strdup(msg.c_str());
		m->sourceName = _strdup(sourceName.c_str());

		m_numLogMessages++;
	}

	void FreeLogMessages()
	{
		if (m_logMessages && m_numLogMessages > 0)
		{
			for (uint32_t i = 0; i < m_numLogMessages; ++i)
			{
				free((void*)m_logMessages->message);
				free((void*)m_logMessages->message);
			}

			free(m_logMessages);

			m_numLogMessages = 0;
			m_logMessages = nullptr;
		}
	}
};

extern CGlobalAssetData g_assetData;
