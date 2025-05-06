#pragma once
#include <game/asset.h>

class CSourceModelSource : public CAssetContainer
{
public:
    CSourceModelSource() {};
    ~CSourceModelSource() = default;

    const CAsset::ContainerType GetContainerType() const
    {
        return CAsset::ContainerType::MDL;
    }

    void SetFileName(const char* fileName) { m_fileName = fileName; }
	void SetFilePath(const std::filesystem::path& path) { m_filePath = path; }
    const char* const GetFileName() const { return m_fileName; }
	const std::filesystem::path& GetFilePath() const { return m_filePath; }

private:
	std::filesystem::path m_filePath;
    const char* m_fileName;
};

class CSourceModelAsset : public CAsset
{
public:
	CSourceModelAsset(const studiohdr_short_t* const pStudioHdr) : m_modelParsed(nullptr), m_modelDrawData(nullptr), m_sequences(nullptr), m_numSequences(0)
	{
		const int length = pStudioHdr->length();
        char* tmpBuf = new char[length];
		memcpy_s(tmpBuf, length, reinterpret_cast<const char* const>(pStudioHdr), length);

        SetInternalAssetData(tmpBuf);
		SetAssetVersion(pStudioHdr->version);
		SetAssetGUID(RTech::StringToGuid(pStudioHdr->pszName()));

		// container
		CSourceModelSource* srcMdlSource = new CSourceModelSource;
		SetContainerFile(srcMdlSource);
	}

	~CSourceModelAsset()
	{
		FreeAllocArray(m_assetData);
		FreeAllocVar(m_modelParsed);
		FreeAllocVar(m_modelLoose);
		FreeAllocVar(m_modelDrawData);

		if (m_numModelSkinNames)
		{
			for (int i = 0; i < m_numModelSkinNames; i++)
				FreeAllocArray(m_modelSkinNames[i]);

			FreeAllocArray(m_modelSkinNames);
		}

		for (int i = 0; i < ExtraData::SRCMDL_COUNT; i++)
			FreeAllocArray(m_assetDataExtra[i]);

		FreeAllocArray(m_sequences);
	};

	enum ExtraData
	{
		SRCMDL_VERT,
		SRCMDL_PHYS,
		SRCMDL_ANIM,

		SRCMDL_COUNT,
	};

	uint32_t GetAssetType() const { return 'ldm'; }
	const ContainerType GetAssetContainerType() const { return ContainerType::MDL; }
	std::string GetContainerFileName() const { return static_cast<CSourceModelSource*>(m_containerFile)->GetFileName(); }

	inline const void* GetInternalAssetData() { return m_assetData; }
	inline const uint64_t GetAssetGUID() const { return m_assetGuid; }
	inline char* const GetExtraData(const ExtraData type) { return m_assetDataExtra[type]; }

	inline void SetAssetGUID(const uint64_t guid) { m_assetGuid = guid; }
	inline void SetExtraData(char* const data, const ExtraData type) { m_assetDataExtra[type] = data; }

	inline void SetParsedData(ModelParsedData_t* const parsedData) { m_modelParsed = parsedData; }
	inline void SetLooseData(StudioLooseData_t* const looseData) { m_modelLoose = looseData; }
	inline void SetName(char* const name) { m_modelName = name; }
	void SetSequenceList(uint64_t* guids, const int count)
	{
		m_sequences = guids;
		m_numSequences = count;
	}

	inline ModelParsedData_t* const GetParsedData() const { return m_modelParsed; }
	inline StudioLooseData_t* const GetLooseData() const { return m_modelLoose; }
	inline const char* const GetName() const { return m_modelName; }
	inline const int GetSequenceCount() const { return m_numSequences; }
	inline const uint64_t GetSequenceGUID(const int index) const { return m_sequences[index]; }
	inline const std::vector<ModelBone_t>* const GetRig() const { return m_modelParsed ? &m_modelParsed->bones : nullptr; }
	inline CDXDrawData* const GetDrawData() const { return m_modelDrawData; }

	inline void AllocateDrawData(const uint64_t lod)
	{
		m_modelDrawData = new CDXDrawData();
		m_modelDrawData->meshBuffers.resize(m_modelParsed->lods.at(lod).meshes.size());
		m_modelDrawData->modelName = m_modelName;
	}

	void FixupSkinData();

private:
	uint64_t m_assetGuid;
	char* m_assetDataExtra[ExtraData::SRCMDL_COUNT];

	ModelParsedData_t* m_modelParsed;
	StudioLooseData_t* m_modelLoose;
	CDXDrawData* m_modelDrawData;
	char* m_modelName;

	char** m_modelSkinNames;
	int m_numModelSkinNames;

	int m_numSequences;
	uint64_t* m_sequences;
};

class CSourceSequenceAsset : public CAsset
{
public:
	CSourceSequenceAsset(const CSourceModelAsset* const rigAsset, void* data, const std::string& name)
	{
		SetRigGUID(rigAsset->GetAssetGUID());
		SetContainerFile(rigAsset->GetContainerFile<CSourceModelSource>());
		SetAssetVersion(rigAsset->GetAssetVersion());

		SetInternalAssetData(data);
		SetAssetName(name);
		SetAssetGUID(RTech::StringToGuid(name.c_str()));

		SetUnparsed();
	}

	~CSourceSequenceAsset()
	{
		// m_assetData should be allocated to a model asset.

		FreeAllocVar(m_sequence);
	};

	uint32_t GetAssetType() const { return 'qes'; }
	const ContainerType GetAssetContainerType() const { return ContainerType::MDL; }
	std::string GetContainerFileName() const { return static_cast<CSourceModelSource*>(m_containerFile)->GetFileName(); }

	const void* GetInternalAssetData() { return m_assetData; }
	const uint64_t GetAssetGUID() const { return m_assetGuid; }
	seqdesc_t* const GetSequence() const { return m_sequence; }
	const std::vector<ModelBone_t>* GetRig() { return m_rig; }
	const uint64_t GetRigGUID() const { return m_rigGuid; }

	void SetAssetGUID(const uint64_t guid) { m_assetGuid = guid; }
	void SetSequence(seqdesc_t* const seqdesc) { m_sequence = seqdesc; }
	void SetRig(const std::vector<ModelBone_t>* rig) { m_rig = rig; }

	// anim data
	void SetParsed() { m_animationParsed = true; }
	const bool IsParsed() const { return m_animationParsed; }

private:
	uint64_t m_assetGuid;
	seqdesc_t* m_sequence;
	bool m_animationParsed;

	const std::vector<ModelBone_t>* m_rig;
	uint64_t m_rigGuid;

	void SetUnparsed() { m_animationParsed = false; }
	void SetRigGUID(const uint64_t guid) { m_rigGuid = guid; }
};