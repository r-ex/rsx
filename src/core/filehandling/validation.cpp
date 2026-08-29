#include <pch.h>
#include <core/filehandling/validation.h>
#include <game/asset.h>
#include <game/rtech/cpakfile.h>


#include <game/rtech/assets/settings_layout.h>
#include <game/rtech/assets/shader.h>
#include <game/rtech/assets/shaderset.h>
#include <game/rtech/assets/subtitles.h>
#include <game/rtech/assets/texture.h>
#include <game/rtech/assets/texture_anim.h>
#include <game/rtech/assets/texture_list.h>

// We need to support multiple header sizes as the same asset version can have multiple header sizes due to sub versions.
#define DECLARE_ASSET_VERSION_HEADER_SIZE(asset, version, ...) constexpr uint32_t asset##_V##version[] = { __VA_ARGS__ }
#define VERSION_ENTRY(asset, version) { version, asset##_V##version, sizeof(asset##_V##version) / sizeof(asset##_V##version[0]) }
#define DECLARE_VERSION_FOR_VALIDATION(asset) .knownVersions = asset##_VERSIONS, .knownVersionCount = (sizeof(asset##_VERSIONS) / sizeof((asset##_VERSIONS)[0]))
#define ADD_TO_BANK(asset) { AssetType_t::##asset, asset##_VALIDATION }

struct KnownVersion_t
{
	uint32_t version;
	const uint32_t* headerSizes;
	uint32_t headerSizeCount;
};

struct PakAssetValidationValues_t
{
	const KnownVersion_t* knownVersions;
	uint32_t knownVersionCount;

	bool expectStarpakOffset : 1;  // Expect that the asset type can have a starpak offset
	bool requireStarpakOffset : 1; // Require that the asset type has a starpak offset

	bool expectOptStarpakOffset : 1;  // Expect that the asset type can have an opt starpak offset
	bool requireOptStarpakOffset : 1; // Require that the asset type has an opt starpak offset

	bool expectDataPage : 1; // Expect that the asset type can have a data (cpu) page
	bool requireDataPage : 1; // Require that the asset type has a data (cpu) page
};

struct PakAssetValidationPair_t
{
	AssetType_t assetType;
	PakAssetValidationValues_t validationValues;
};

// ---------------------------------------------------------------------------------------------------------------------------------------- Settings Layout Begin
DECLARE_ASSET_VERSION_HEADER_SIZE(STLT, 0, sizeof(SettingsLayoutHeader_v0_t));
constexpr KnownVersion_t STLT_VERSIONS[] =
{
	VERSION_ENTRY(STLT, 0),
};
constexpr PakAssetValidationValues_t STLT_VALIDATION =
{
	DECLARE_VERSION_FOR_VALIDATION(STLT),

	.expectStarpakOffset = false,
	.requireStarpakOffset = false,

	.expectOptStarpakOffset = false,
	.requireOptStarpakOffset = false,

	.expectDataPage = false,
	.requireDataPage = false
};
// ---------------------------------------------------------------------------------------------------------------------------------------- Settings Layout End


// ---------------------------------------------------------------------------------------------------------------------------------------- Shader Begin
DECLARE_ASSET_VERSION_HEADER_SIZE(SHDR, 8, sizeof(ShaderAssetHeader_v8_t));
DECLARE_ASSET_VERSION_HEADER_SIZE(SHDR, 12, sizeof(ShaderAssetHeader_v12_t));
DECLARE_ASSET_VERSION_HEADER_SIZE(SHDR, 13, sizeof(ShaderAssetHeader_v13_t));
DECLARE_ASSET_VERSION_HEADER_SIZE(SHDR, 14, sizeof(ShaderAssetHeader_v14_t));
DECLARE_ASSET_VERSION_HEADER_SIZE(SHDR, 15, sizeof(ShaderAssetHeader_v15_t));
DECLARE_ASSET_VERSION_HEADER_SIZE(SHDR, 16, sizeof(ShaderAssetHeader_v15_t));
DECLARE_ASSET_VERSION_HEADER_SIZE(SHDR, 17, sizeof(ShaderAssetHeader_v15_t));
DECLARE_ASSET_VERSION_HEADER_SIZE(SHDR, 19, sizeof(ShaderAssetHeader_v14_t));
constexpr KnownVersion_t SHDR_VERSIONS[] =
{
	VERSION_ENTRY(SHDR, 8),
	VERSION_ENTRY(SHDR, 12),
	VERSION_ENTRY(SHDR, 13),
	VERSION_ENTRY(SHDR, 14),
	VERSION_ENTRY(SHDR, 15),
	VERSION_ENTRY(SHDR, 16),
	VERSION_ENTRY(SHDR, 17),
	VERSION_ENTRY(SHDR, 19),
};
constexpr PakAssetValidationValues_t SHDR_VALIDATION =
{
	DECLARE_VERSION_FOR_VALIDATION(SHDR),

	.expectStarpakOffset = false,
	.requireStarpakOffset = false,

	.expectOptStarpakOffset = false,
	.requireOptStarpakOffset = false,

	.expectDataPage = true,
	.requireDataPage = false
};
// ---------------------------------------------------------------------------------------------------------------------------------------- Shader End


// ---------------------------------------------------------------------------------------------------------------------------------------- Shader Set Begin
DECLARE_ASSET_VERSION_HEADER_SIZE(SHDS, 8, sizeof(ShaderSetAssetHeader_v8_t));
DECLARE_ASSET_VERSION_HEADER_SIZE(SHDS, 11, sizeof(ShaderSetAssetHeader_v11_t));
DECLARE_ASSET_VERSION_HEADER_SIZE(SHDS, 12, sizeof(ShaderSetAssetHeader_v12_t));
DECLARE_ASSET_VERSION_HEADER_SIZE(SHDS, 13, sizeof(ShaderSetAssetHeader_v12_t), sizeof(ShaderSetAssetHeader_v13_t));
DECLARE_ASSET_VERSION_HEADER_SIZE(SHDS, 14, sizeof(ShaderSetAssetHeader_v14_t));
DECLARE_ASSET_VERSION_HEADER_SIZE(SHDS, 15, sizeof(ShaderSetAssetHeader_v15_t));

constexpr KnownVersion_t SHDS_VERSIONS[] =
{
	VERSION_ENTRY(SHDS, 8),
	VERSION_ENTRY(SHDS, 11),
	VERSION_ENTRY(SHDS, 12),
	VERSION_ENTRY(SHDS, 13),
	VERSION_ENTRY(SHDS, 14),
	VERSION_ENTRY(SHDS, 15),
};
constexpr PakAssetValidationValues_t SHDS_VALIDATION =
{
	DECLARE_VERSION_FOR_VALIDATION(SHDS),

	.expectStarpakOffset = false,
	.requireStarpakOffset = false,

	.expectOptStarpakOffset = false,
	.requireOptStarpakOffset = false,

	.expectDataPage = false,
	.requireDataPage = false
};
// ---------------------------------------------------------------------------------------------------------------------------------------- Shader Set End


// ----------------------------------------------------------------------------------------------------------------------------------------  Subtitles Begin
DECLARE_ASSET_VERSION_HEADER_SIZE(SUBT, 0, sizeof(SubtitlesAssetHeader_v0_t));
DECLARE_ASSET_VERSION_HEADER_SIZE(SUBT, 1, sizeof(SubtitlesAssetHeader_v0_t));
constexpr KnownVersion_t SUBT_VERSIONS[] =
{
	VERSION_ENTRY(SUBT, 0),
	VERSION_ENTRY(SUBT, 1),
};
constexpr PakAssetValidationValues_t SUBT_VALIDATION =
{
	DECLARE_VERSION_FOR_VALIDATION(SUBT),

	.expectStarpakOffset = false,
	.requireStarpakOffset = false,

	.expectOptStarpakOffset = false,
	.requireOptStarpakOffset = false,

	.expectDataPage = false,
	.requireDataPage = false
};
// ----------------------------------------------------------------------------------------------------------------------------------------  Subtitles End


// ----------------------------------------------------------------------------------------------------------------------------------------  Texture Begin
DECLARE_ASSET_VERSION_HEADER_SIZE(TXTR, 8, sizeof(TextureAssetHeader_v8_t));
DECLARE_ASSET_VERSION_HEADER_SIZE(TXTR, 9, sizeof(TextureAssetHeader_v9_t));
DECLARE_ASSET_VERSION_HEADER_SIZE(TXTR, 10, sizeof(TextureAssetHeader_v10_t));
constexpr KnownVersion_t TXTR_VERSIONS[] =
{
	VERSION_ENTRY(TXTR, 8),
	VERSION_ENTRY(TXTR, 9),
	VERSION_ENTRY(TXTR, 10),
};
constexpr PakAssetValidationValues_t TXTR_VALIDATION =
{
	DECLARE_VERSION_FOR_VALIDATION(TXTR),

	.expectStarpakOffset = true,
	.requireStarpakOffset = false,

	.expectOptStarpakOffset = true,
	.requireOptStarpakOffset = false,

	.expectDataPage = true,
	.requireDataPage = true
};
// ----------------------------------------------------------------------------------------------------------------------------------------  Texture End


// ----------------------------------------------------------------------------------------------------------------------------------------  Texture Animation Begin
DECLARE_ASSET_VERSION_HEADER_SIZE(TXAN, 1, sizeof(TextureAnimAssetHeader_v1_t));
constexpr KnownVersion_t TXAN_VERSIONS[] =
{
	VERSION_ENTRY(TXAN, 1),
};
constexpr PakAssetValidationValues_t TXAN_VALIDATION =
{
	DECLARE_VERSION_FOR_VALIDATION(TXAN),

	.expectStarpakOffset = false,
	.requireStarpakOffset = false,

	.expectOptStarpakOffset = false,
	.requireOptStarpakOffset = false,

	.expectDataPage = false,
	.requireDataPage = false
};
// ----------------------------------------------------------------------------------------------------------------------------------------  Texture Animation End


// ----------------------------------------------------------------------------------------------------------------------------------------  Texture List Begin
DECLARE_ASSET_VERSION_HEADER_SIZE(TXLS, 1, sizeof(TextureListHeader_v1_s));
constexpr KnownVersion_t TXLS_VERSIONS[] =
{
	VERSION_ENTRY(TXLS, 1),
};
constexpr PakAssetValidationValues_t TXLS_VALIDATION =
{
	DECLARE_VERSION_FOR_VALIDATION(TXLS),

	.expectStarpakOffset = false,
	.requireStarpakOffset = false,

	.expectOptStarpakOffset = false,
	.requireOptStarpakOffset = false,

	.expectDataPage = false,
	.requireDataPage = false
};
// ----------------------------------------------------------------------------------------------------------------------------------------  Texture List End

constexpr PakAssetValidationPair_t VALIDATION_DATA_BANK[] =
{
	ADD_TO_BANK(STLT), // AssetType_T::STLT
	ADD_TO_BANK(SHDR), // AssetType_T::SHDR
	ADD_TO_BANK(SHDS), // AssetType_T::SHDS
	ADD_TO_BANK(SUBT), // AssetType_T::SUBT
	ADD_TO_BANK(TXTR), // AssetType_T::TXTR
	ADD_TO_BANK(TXAN), // AssetType_T::TXAN
	ADD_TO_BANK(TXLS), // AssetType_T::TXLS
};

bool ValidateLoadedPakFiles()
{
	printf("\nVALIDATION: Checking %lld assets across %lld files\n", g_assetData.GetNumAssets(), g_assetData.GetNumContainers());

	uint32_t numSegmentErrors = 0;
	uint32_t numAssetErrors = 0;
	uint32_t numAssetWarnings = 0;

	std::unordered_map<uint32_t, PakLoadedAssetTypeInfo_t> foundAssetTypes;

	for (CAssetContainer* container : g_assetData.v_assetContainers)
	{
		if (container->GetContainerType() != CAssetContainer::ContainerType::PAK)
			continue;

		const CPakFile* const pakFile = reinterpret_cast<CPakFile*>(container);

		const std::string stemString = container->GetFilePath().stem().string();
		
		printf("\tChecking container file: %s\n", stemString.c_str());
	
		if (pakFile->segmentPaddingTooBig)
		{
			printf("\t\tError: segment padding is too big\n");
		}

		if (pakFile->segmentPaddingTooSmall)
		{
			printf("\t\tError: segment padding is too small\n");
		}
		
		numSegmentErrors += pakFile->segmentPaddingTooBig + pakFile->segmentPaddingTooSmall;

		for (auto& [type, loadedInfo] : pakFile->GetLoadedAssetTypeInfo())
		{
			const std::string assetTypeFourCC = fourCCToString(type);
			if (loadedInfo.inconsistentHeaderSize)
			{
				printf("\t\tError: asset type '%s' has inconsistent header sizes\n", assetTypeFourCC.c_str());
			}

			if (loadedInfo.inconsistentVersions)
			{
				printf("\t\tError: asset type '%s' has inconsistent asset versions\n", assetTypeFourCC.c_str());
			}
		
			numAssetErrors += loadedInfo.inconsistentHeaderSize + loadedInfo.inconsistentVersions;

			// Validate against the data bank
			const PakAssetValidationPair_t* const validationPair = std::ranges::find(VALIDATION_DATA_BANK, static_cast<AssetType_t>(type), &PakAssetValidationPair_t::assetType);
			if (validationPair != std::end(VALIDATION_DATA_BANK))
			{
				const PakAssetValidationValues_t& validationValues = validationPair->validationValues;

				const KnownVersion_t* const knownVersion = std::find_if(
					validationValues.knownVersions,
					validationValues.knownVersions + validationValues.knownVersionCount,
					[version = loadedInfo.version](const auto& v)
					{
						return v.version == version;
					}
				);

				// Basically knownVersion != std::end(knownVersions)
				if (knownVersion != (validationValues.knownVersions + validationValues.knownVersionCount))
				{
					bool foundHeaderSize = false;

					for (uint32_t i = 0; i < knownVersion->headerSizeCount; ++i)
					{
						if (knownVersion->headerSizes[i] == loadedInfo.headerSize)
						{
							foundHeaderSize = true;
							break;
						}
					}

					if (!foundHeaderSize)
					{
						printf("\tWarning: asset type '%s' has an unknown header size: v%u, %u bytes\n", assetTypeFourCC.c_str(), loadedInfo.version, loadedInfo.headerSize);
						numAssetWarnings++;
					}
				}
				else
				{
					printf("\tWarning: asset type '%s' has an unknown version and header size: v%u, %u bytes\n", assetTypeFourCC.c_str(), loadedInfo.version, loadedInfo.headerSize);
					numAssetWarnings++;
				}
			}

			if (foundAssetTypes.count(type) == 0)
				foundAssetTypes[type] = loadedInfo;
			else
				foundAssetTypes[type].Merge(loadedInfo);
		}
	}

	printf("Found %llu asset types:\n", foundAssetTypes.size());
	for (auto& [type, loadedInfo] : foundAssetTypes)
	{
		const std::string assetTypeFourCC = fourCCToString(type);

		printf("\t%s: v%u, hdr %u bytes%s, %llu assets\n", assetTypeFourCC.c_str(), loadedInfo.version, loadedInfo.headerSize, loadedInfo.inconsistentHeaderSize ? "*" : "", loadedInfo.assetCount);
	}

	const uint32_t numContainerErrors = g_assetData.m_numFailedContainerLoads + numSegmentErrors + numAssetErrors;
	printf("Found %i problems:\n\t%u segment padding errors, %u asset data errors, %u files failed to load\n", numContainerErrors, numSegmentErrors, numAssetErrors, g_assetData.m_numFailedContainerLoads);
	printf("Found %i warnings:\n\t%u unknown asset versions\n", numAssetWarnings, numAssetWarnings);

	return true;
}