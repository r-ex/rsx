#pragma once
#include <game/asset.h>
#include <game/rtech/assets/model.h>

// erm what the heck
extern inline void ParseVertexFromVTX(Vertex_t* const vert, VertexWeight_t* const weights, Vector2D* const texcoords, ModelMeshData_t* mesh, const OptimizedModel::Vertex_t* const pVertex, const vvd::mstudiovertex_t* const pVerts, const Vector4D* const pTangs, const Color32* const pColors, const Vector2D* const pUVs,
	int& weightIdx, const bool isHwSkinned, const OptimizedModel::BoneStateChangeHeader_t* const pBoneStates);

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
	CSourceModelAsset(const studiohdr_short_t* const pStudioHdr)
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
        if (m_assetData)
            delete[] m_assetData;

		if (m_modelAsset)
			delete m_modelAsset;

		for (int i = 0; i < ExtraData::SRCMDL_COUNT; i++)
		{
			if (m_assetDataExtra[i])
				delete[] m_assetDataExtra[i];
		}
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

	const void* GetInternalAssetData() { return m_assetData; }
	const uint64_t GetAssetGUID() const { return m_assetGuid; }
	ModelAsset* const GetModelAsset() const { return m_modelAsset; }
	char* const GetExtraData(const ExtraData type) { return m_assetDataExtra[type]; }

	void SetAssetGUID(const uint64_t guid) { m_assetGuid = guid; }
	void SetModelAsset(ModelAsset* const mdlAsset) { m_modelAsset = mdlAsset; }
	void SetExtraData(char* const data, const ExtraData type) { m_assetDataExtra[type] = data; }

private:
	uint64_t m_assetGuid;
	ModelAsset* m_modelAsset; // todo: eliminate this

	char* m_assetDataExtra[ExtraData::SRCMDL_COUNT];
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

		if (m_sequence)
			delete m_sequence;
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