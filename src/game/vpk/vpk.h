#pragma once
#include <game/asset.h>

constexpr uint32_t VPK_FILE_MAGIC = 0x55AA1234;
constexpr uint16_t VPK_MAJOR_VERS = 2;
constexpr uint16_t VPK_MINOR_VERS = 3;
constexpr uint16_t VPK_CHUNK_TERM = UINT16_MAX;

enum class VPKFileType_e
{
	UNKNOWN = 0, // if there's no special parsed data
	BSP,      // wrap asset is a base BSP file and contains a CBSPData pointer
	TEXT,
};

const static std::unordered_map<std::string, VPKFileType_e> s_vpkFileTypes = {
	{".bsp", VPKFileType_e::BSP},
	{".txt", VPKFileType_e::TEXT},
	{".nut", VPKFileType_e::TEXT},
	{".gnut", VPKFileType_e::TEXT},
	{".res", VPKFileType_e::TEXT},
	{".ent", VPKFileType_e::TEXT},
};

#pragma pack(push, 1)
struct VPKDirHeader_s
{
	uint32_t magic;
	
	uint16_t majorVer;
	uint16_t minorVer;

	uint32_t dirTreeSize;
	uint32_t sigSize; // dunno what this means
};

struct VPKEntryBlock_s
{
	uint32_t fileCRC;
	uint16_t preloadSize;
	uint16_t archiveIdx;
};

struct VPKDataChunk_s
{
	uint32_t loadFlags;
	uint16_t texFlags;

	uint64_t dataOffset;
	uint64_t cmpSize;
	uint64_t dcmpSize;
};
#pragma pack(pop)

class CVPKPackage;
class CVPKFile : public CAsset
{
public:
	CVPKFile() = default;
	~CVPKFile() = default;

	uint32_t GetAssetType() const { return 'fkpv'; };
	const uint64_t GetAssetGUID() const { return fileCRC; };

	const ContainerType GetAssetContainerType() const
	{
		return ContainerType::VPK;
	}

	std::string GetContainerFileName() const { return "n/a"; };

	FORCEINLINE void AddDataChunk(const VPKDataChunk_s& chunk)
	{
		dataChunks.emplace_back(chunk);
	}

	void SetFileType(VPKFileType_e type)
	{
		fileType = type;
	}

	void SetFileCRC(uint32_t crc)
	{
		fileCRC = crc;
	}

	const std::vector<VPKDataChunk_s>& GetDataChunks() const
	{
		return dataChunks;
	}

private:

	std::vector<VPKDataChunk_s> dataChunks;

	uint32_t fileCRC;

	VPKFileType_e fileType;

	CVPKPackage* const package() { return static_cast<CVPKPackage*>(m_containerFile); };
};

class CVPKPackage : public CAssetContainer
{
public:
	CVPKPackage() = default;
	~CVPKPackage()
	{
		for (auto& it : this->assets)
		{
			delete it;
		}
	};

	const CAsset::ContainerType GetContainerType() const { return CAsset::ContainerType::VPK; };

	void ProcessAssets();
	bool ParseFromFile(const std::string& filePath);

private:
	std::shared_ptr<char[]> m_fileBuf;

	std::vector<CAsset*> assets;

	uint16_t majorVer;
	uint16_t minorVer;
};
