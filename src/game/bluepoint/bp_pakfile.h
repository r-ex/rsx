#pragma once
#include <game/asset.h>

#define BP_PAK_ID       MAKEFOURCC('X', 'B', 'A', 'R')
#define BP_PAK_VER_R1   6

#define BP_PAK_FILENAME_SIZE 64

struct bpkfile_v6_t
{
    int hash[2]; // first 8 bytes of the sha1 hash of the filename

    int unk_8; // flags?

    int chunkStart;
    int decompressedSize;
    int dataStartOffset;
    int dataEndOffset;

    void swap()
    {
        ISWAP32(this->unk_8);

        ISWAP32(this->chunkStart);
        ISWAP32(this->decompressedSize);
        ISWAP32(this->dataStartOffset);
        ISWAP32(this->dataEndOffset);
    }

    inline const bool IsCompressed() const { return decompressedSize != dataEndOffset - dataStartOffset; }
    inline const int DataSize() const { return IsCompressed() ? (dataEndOffset - dataStartOffset) : decompressedSize; }
};

struct bpkpatch_t
{
    char filename[BP_PAK_FILENAME_SIZE];

    int unk_40;
    int unk_44;
    int unk_48;
    int unk_4C;

    void swap()
    {
        ISWAP32(this->unk_40);
        ISWAP32(this->unk_44);
        ISWAP32(this->unk_48);
        ISWAP32(this->unk_4C);
    }
};

struct bpkhdr_short_t
{
    int id;
    int version;
};

struct bpkhdr_v6_t
{
    int id; // 'XBAR' (Xbox Archive)
    int version; // '6', previously '0' (MGS)
    int fileCount;

    int unk;        

    int chunkSize; // max size a chunk can be
    int chunkCount;

    int dataOffset;
    int dataSize;
    int patchCount; // number of pak files that have been patched

    inline bpkfile_v6_t* const pFile(const int idx) { return reinterpret_cast<bpkfile_v6_t* const>((char*)this + sizeof(bpkhdr_v6_t)) + idx; }
    inline int* const pUnk(const int idx) { return reinterpret_cast<int* const>(pFile(fileCount)) + idx; }
    inline int* const pChunkSize(const int idx) { return pUnk(fileCount) + idx; }
    inline bpkpatch_t* const pPatch(const int idx) { return patchCount ? reinterpret_cast<bpkpatch_t* const>(pChunkSize(fileCount)) + idx : nullptr; }
    // there is data after patches in paks with patches, patchdata?

    inline char* const pChunkData() { return reinterpret_cast<char* const>((char*)this + dataOffset); }

    void swap()
    {
        ISWAP32(this->version);
        ISWAP32(this->fileCount);

        ISWAP32(this->unk);

        ISWAP32(this->chunkSize);
        ISWAP32(this->chunkCount);

        ISWAP32(this->dataOffset);
        ISWAP32(this->dataSize);
        ISWAP32(this->patchCount);

        for (int i = 0; i < fileCount; i++)
        {
            pFile(i)->swap();
            ISWAP32(*pUnk(i));
        }

        for (int i = 0; i < chunkCount; i++)
            ISWAP32(*pChunkSize(i));

        for (int i = 0; i < patchCount; i++)
            pPatch(i)->swap();
    }
};

class CBluepointPakfile : public CAssetContainer
{
public:
    CBluepointPakfile() : m_version(-1), m_fileCount(0), m_patchCount(0), m_files(nullptr), m_patches(nullptr), m_chunkSize(0)
    {

    }

    ~CBluepointPakfile() = default;

    const CAsset::ContainerType GetContainerType() const
    {
        return CAsset::ContainerType::BP_PAK;
    }

    void SetFileName(const char* fileName) { strncpy_s(m_fileName, 64, fileName, strnlen_s(fileName, 64)); }
    void SetFilePath(const std::filesystem::path& path) { m_filePath = path; }
    const char* const GetFileName() const { return m_fileName; }
    const std::filesystem::path& GetFilePath() const { return m_filePath; }

    inline const int Version() const { return m_version; }
    inline const int FileCount() const { return m_fileCount; }
    inline void* const Files() { return m_files; }
    inline char* const FileBuf() { return m_Buf.get(); }

    inline const int GetMaxChunkSize() const { return m_chunkSize; }

    bool ParseFromFile();

    struct Chunk_t
    {
        char* data;
        int dataSize;

        int pad;
    };

    inline const Chunk_t* const GetChunk(const int idx) const { return &m_chunks.at(idx); }

private:
    std::filesystem::path m_filePath;
    char m_fileName[BP_PAK_FILENAME_SIZE];

    int m_version;

    int m_fileCount;
    int m_patchCount;

    void* m_files;
    bpkpatch_t* m_patches;

    int m_chunkSize;
    std::vector<Chunk_t> m_chunks;

    std::shared_ptr<char[]> m_Buf;
};

class CBluepointWrappedFile : public CAsset
{
public:
    CBluepointWrappedFile(CBluepointPakfile* const pakfile, void* const file)
    {
        SetContainerFile(pakfile);
        SetAssetVersion({ pakfile->Version() });
        SetInternalAssetData(file);
    }

    ~CBluepointWrappedFile()
    {

    };

    uint32_t GetAssetType() const { return 'fwpb'; }
    const ContainerType GetAssetContainerType() const { return ContainerType::BP_PAK; }
    std::string GetContainerFileName() const { return static_cast<CBluepointPakfile*>(m_containerFile)->GetFileName(); }

    const void* GetInternalAssetData() { return m_assetData; }
    const uint64_t GetAssetGUID() const { return m_assetGuid; }

    inline void MakeGuid64FromGuid32(const int hash0, const int hash1)
    {
        m_assetHash[0] = hash0;
        m_assetHash[1] = hash1;
    }

    inline void ParseFromBpkfile(bpkfile_v6_t* const file)
    {
        CBluepointPakfile* const pakfile = GetContainerFile<CBluepointPakfile>();

#ifdef _DEBUG
        char* const filedata = pakfile->FileBuf() + file->dataStartOffset;
        const CBluepointPakfile::Chunk_t* const firstChunk = pakfile->GetChunk(file->chunkStart);

        assertm(filedata == firstChunk->data, "ptr mismatch");
#endif //  _DEBUG

        m_dataSizeDecompressed = file->decompressedSize;
        m_dataSizeCompressed = file->DataSize();
        m_dataIsCompressed = file->IsCompressed();

        m_chunkCount = m_dataSizeDecompressed > pakfile->GetMaxChunkSize() ? (m_dataSizeDecompressed / pakfile->GetMaxChunkSize()) + 1 : 1;
        m_chunkIndex = file->chunkStart;

        m_unk_8 = file->unk_8;
        //if (m_unk_8)
        //    Log("bpkfile unk_8 %lx\n", m_unk_8);
    }

    inline const int GetCompSize() const { return m_dataSizeCompressed; }
    inline const int GetDecompSize() const { return m_dataSizeDecompressed; }
    inline const bool IsCompressed() const { return m_dataIsCompressed; }

    inline const int GetChunkCount() const { return m_chunkCount; }
    inline const int GetFirstChunkIndex() const { return m_chunkIndex; }

private:
    union {
        uint64_t m_assetGuid;
        int m_assetHash[2];
    };

    int m_chunkCount;
    int m_chunkIndex;

    int m_dataSizeDecompressed;
    int m_dataSizeCompressed;
    bool m_dataIsCompressed;

    int m_unk_8; // weird guy
};