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

struct PakAssetValidationValues_t
{
	// Version -> Vector of known header sizes
	std::unordered_map<int, std::vector<int>> knownVersions;
	
	bool expectStarpakOffset : 1;  // Expect that the asset type can have a starpak offset
	bool requireStarpakOffset : 1; // Require that the asset type has a starpak offset

	bool expectOptStarpakOffset : 1;  // Expect that the asset type can have an opt starpak offset
	bool requireOptStarpakOffset : 1; // Require that the asset type has an opt starpak offset

	bool expectDataPage : 1; // Expect that the asset type can have a data (cpu) page
	bool requireDataPage : 1; // Require that the asset type has a data (cpu) page

	PakAssetValidationValues_t() : knownVersions(),
		expectStarpakOffset(false), requireStarpakOffset(false),
		expectOptStarpakOffset(false), requireOptStarpakOffset(false),
		expectDataPage(false), requireDataPage(false) {
	};
};

static std::map<AssetType_t, PakAssetValidationValues_t> VALIDATION_DATA_BANK = {
};

// Now this is going to be one veeeery big function...
void CreateValidationDataBank()
{
	auto& v = VALIDATION_DATA_BANK;

	// Settings Layout
	auto& stlt = v[AssetType_t::STLT];
	stlt.knownVersions[0] = { sizeof(SettingsLayoutHeader_v0_t) };

	// Shader
	auto& shdr = v[AssetType_t::SHDR];
	shdr.knownVersions[8] = { sizeof(ShaderAssetHeader_v8_t) };
	shdr.knownVersions[12] = { sizeof(ShaderAssetHeader_v12_t) };
	shdr.knownVersions[13] = { sizeof(ShaderAssetHeader_v13_t) };
	shdr.knownVersions[14] = { sizeof(ShaderAssetHeader_v14_t) };
	shdr.knownVersions[15] = { sizeof(ShaderAssetHeader_v15_t) };
	shdr.knownVersions[16] = { sizeof(ShaderAssetHeader_v15_t) };
	shdr.knownVersions[17] = { sizeof(ShaderAssetHeader_v15_t) };
	shdr.expectDataPage = true;

	// Shader Set
	auto& shds = v[AssetType_t::SHDS];
	shds.knownVersions[8] = { sizeof(ShaderSetAssetHeader_v8_t) };
	shds.knownVersions[11] = { sizeof(ShaderSetAssetHeader_v11_t) };
	shds.knownVersions[12] = { sizeof(ShaderSetAssetHeader_v12_t) };
	shds.knownVersions[13] = { sizeof(ShaderSetAssetHeader_v12_t), sizeof(ShaderSetAssetHeader_v13_t) };
	shds.knownVersions[14] = { sizeof(ShaderSetAssetHeader_v14_t) };
	shds.knownVersions[15] = { sizeof(ShaderSetAssetHeader_v15_t) };

	// Subtitles
	auto& subt = v[AssetType_t::SUBT];
	subt.knownVersions[0] = { sizeof(SubtitlesAssetHeader_v0_t) };
	subt.knownVersions[1] = { sizeof(SubtitlesAssetHeader_v0_t) };

	// Texture
	auto& txtr = v[AssetType_t::TXTR];
	txtr.expectStarpakOffset = true;
	txtr.expectOptStarpakOffset = true;
	txtr.expectDataPage = true;
	txtr.requireDataPage = true;

	txtr.knownVersions[8] = { sizeof(TextureAssetHeader_v8_t) };
	txtr.knownVersions[9] = { sizeof(TextureAssetHeader_v9_t) };
	txtr.knownVersions[10] = { sizeof(TextureAssetHeader_v10_t) };

	// Texture Animation
	auto& txan = v[AssetType_t::TXAN];
	txan.knownVersions[1] = { sizeof(TextureAnimAssetHeader_v1_t) };

	// Texture List
	auto& txls = v[AssetType_t::TXLS];
	txls.knownVersions[1] = { sizeof(TextureListHeader_v1_s) };
}

bool ValidateLoadedPakFiles()
{
	CreateValidationDataBank();
	printf("\nVALIDATION: Checking %lld assets across %lld files\n", g_assetData.GetNumAssets(), g_assetData.GetNumContainers());

	uint32_t numSegmentErrors = 0;
	uint32_t numAssetErrors = 0;

	std::unordered_map<uint32_t, PakLoadedAssetTypeInfo_t> foundAssetTypes;

	for (CAssetContainer* container : g_assetData.v_assetContainers)
	{
		if (container->GetContainerType() != CAssetContainer::ContainerType::PAK)
			continue;

		CPakFile* pakFile = reinterpret_cast<CPakFile*>(container);

		const std::string stemString = container->GetFilePath().stem().string();
		
		printf("\tChecking container file: %s\n", stemString.c_str());
	
		if (pakFile->segmentPaddingTooBig)
			printf("\t\tError: segment padding is too big\n");
		if (pakFile->segmentPaddingTooSmall)
			printf("\t\tError: segment padding is too small\n");
		
		numSegmentErrors += pakFile->segmentPaddingTooBig + pakFile->segmentPaddingTooSmall;

		for (auto& [type, loadedInfo] : pakFile->GetLoadedAssetTypeInfo())
		{
			const std::string assetTypeFourCC = fourCCToString(type);
			if (loadedInfo.inconsistentHeaderSize)
				printf("\t\tError: asset type '%s' has inconsistent header sizes\n", assetTypeFourCC.c_str());
			if (loadedInfo.inconsistentVersions)
				printf("\t\tError: asset type '%s' has inconsistent asset versions\n", assetTypeFourCC.c_str());
		
			numAssetErrors += loadedInfo.inconsistentHeaderSize + loadedInfo.inconsistentVersions;

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

		printf("\t%s: v%u, %llu assets\n", assetTypeFourCC.c_str(), loadedInfo.version, loadedInfo.assetCount);
	}

	const uint32_t numContainerErrors = g_assetData.m_numFailedContainerLoads + numSegmentErrors + numAssetErrors;
	printf("Found %i problems:\n\t%u segment padding errors, %u asset data errors, %u files failed to load\n", numContainerErrors, numSegmentErrors, numAssetErrors, g_assetData.m_numFailedContainerLoads);

	return true;
}