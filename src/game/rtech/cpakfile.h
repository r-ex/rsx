#pragma once
#include <game/rtech/utils/utils.h>
#include <game/asset.h>

// maximum number of asset types that can be registered at a time
#define PAK_MAX_ASSET_TYPES 64
#define PAK_ASSET_TYPE_MASK (PAK_MAX_ASSET_TYPES - 1)

#define PAK_HEADER_FLAGS_RTECH_ENCODED (1<<8)
#define PAK_HEADER_FLAGS_OODLE_ENCODED (1<<9)
#define PAK_HEADER_FLAGS_ZSTD_ENCODED (1<<15)
#define PAK_HEADER_FLAGS_COMPRESSED (PAK_HEADER_FLAGS_RTECH_ENCODED | PAK_HEADER_FLAGS_OODLE_ENCODED | PAK_HEADER_FLAGS_ZSTD_ENCODED)

// maximum number of "segments" that can be present in a pak
#define PAK_MAX_SEGMENTS 20

// maximum number of segment "collections"
#define PAK_MAX_SEGMENT_COLLECTIONS 4

struct PakPatchFileHdr_t
{
    __int64 cmpSize;
    __int64 dcmpSize;
};

struct PakPatchDataHdr_t
{
    int patchDataStreamSize;
    int patchPageCount;
};

#define SF_CPU  (1 << 0) // any data other than headers - "cpu" in rson
#define SF_TEMP (1 << 1) // unk - "temp" in rson
#define SF_SERVER (1 << 5) // data that is only used on the server? - "server" in rson
#define SF_CLIENT (1 << 6) // data that is only used on the client? - "client" in rson
#define SF_DEV  (1 << 7) // dev-only data. typically excluded from non-debug builds - "dev" in rson

#define SF_TYPE_MASK (SF_CPU | SF_TEMP)

// header struct for each "virtual segment"
// flags - segment flags, see above
// align - byte alignment for pages when loaded into memory
// size is the total size of all pages that point to this segment
struct PakSegmentHdr_t
{
    int flags;
    uint32_t align;
    int64_t size;

    FORCEINLINE const __int8 GetType() const
    {
        return flags & SF_TYPE_MASK;
    }

    FORCEINLINE const bool IsHeaderSegment() const
    {
        return GetType() == 0;
    }
};

static_assert(sizeof(PakSegmentHdr_t) == 0x10, "Invalid PakSegmentHdr_t struct");

// header struct for each page
// each page header corresponds to a separate section of data
// segment - index into array of segment headers
// align - byte alignment for page when loaded into memory?
// size - size of data in this page
struct PakPageHdr_t
{
    int segment;
    int align;
    unsigned int size;
};

static_assert(sizeof(PakPageHdr_t) == 0xC, "Invalid PakPageHdr_t struct");

// PagePtr readers.
#define IS_PAGE_PTR_INVALID(pagePtr) (!pagePtr.ptr)
#define READ_DEREF_OFFSET(pagePtr, type) *reinterpret_cast<type*>(pagePtr.ptr); pagePtr.ptr += sizeof(type)
#define READ_OFFSET(pagePtr, type) reinterpret_cast<type*>(pagePtr.ptr); pagePtr.ptr += sizeof(type)
// represents a pointer to data in an RPak data page
// in file, this is index/offset
// when loaded, this is converted to an actual pointer to be used by the engine
union PagePtr_t
{
    struct
    {
        int index;
        int offset;
    };

    char* ptr;
};

// header is used to locate all pointers before assets are loaded, so that the engine can easily follow the pointer without page lookup
typedef PagePtr_t PakPointerHdr_t;

// header is used to locate all GUID dependencies (i.e. where an asset uses the GUID of another asset to point to its header)
// GUIDs are converted to pointers at pak load
typedef PagePtr_t PakGuidRefHdr_t;

union AssetGuid_t
{
    size_t guid;
    void* ptr;
};

#define HASH_INDEX_FOR_ASSET_TYPE(type) (((0x1020601 * (type)) >> 24) & PAK_ASSET_TYPE_MASK);

struct PakAsset_v6_t
{
    uint64_t guid;
    char unk0[8];

    // page with SF_HEAD flag
    PagePtr_t headPagePtr;

    // technically "cpu" but this is confusing so just "data". SF_CPU
    PagePtr_t dataPagePtr;

    // if the pointer has failed to resolve, this won't help
    bool HasValidDataPagePointer() const { return dataPagePtr.ptr != 0; };

    __int64 starpakOffset;

    uint16_t pageEnd;

    // value is decremented every time a dependency finishes processing its own dependencies
    uint16_t remainingDependencyCount;

    // really not sure why this is a thing
    // index into array of asset indexes - indicates which LOCAL assets are dependent on this asset (reference by GUID)
    uint32_t dependentsIndex;

    // index into array of ptrs - items point to the page locations at which this asset has a GUID reference (to be converted to a pointer at load)
    uint32_t dependenciesIndex;

    int dependentsCount;

    uint32_t dependenciesCount;

    uint32_t headerStructSize;

    int version;

    // 4-char abbreviation of asset type
    uint32_t type;
};
static_assert(sizeof(PakAsset_v6_t) == 0x48);

struct PakAsset_v8_t
{
    uint64_t guid;
    char unk0[8];

    // page with SF_HEAD flag
    PagePtr_t headPagePtr;

    // technically "cpu" but this is confusing so just "data". SF_CPU
    PagePtr_t dataPagePtr;

    // if the pointer has failed to resolve, this won't help
    bool HasValidDataPagePointer() const { return dataPagePtr.ptr != 0; };

    __int64 starpakOffset;
    __int64 optStarpakOffset;

    uint16_t pageEnd;

    // value is decremented every time a dependency finishes processing its own dependencies
    uint16_t remainingDependencyCount;

    // really not sure why this is a thing
    // index into array of asset indexes - indicates which LOCAL assets are dependent on this asset (reference by GUID)
    uint32_t dependentsIndex;

    // index into array of ptrs - items point to the page locations at which this asset has a GUID reference (to be converted to a pointer at load)
    uint32_t dependenciesIndex;

    int dependentsCount;

    uint16_t dependenciesCount;
    uint16_t unk2; // might be padding in s3. sometimes used as a separate var in later versions

    uint32_t headerStructSize;

    uint8_t version;

    // 4-char abbreviation of asset type
    uint32_t type;
};
static_assert(sizeof(PakAsset_v8_t) == 0x50);

struct PakAsset_t
{
    PakAsset_t() = default;
    PakAsset_t(PakAsset_v6_t* asset) : assetPtr(asset), guid(asset->guid), headPagePtr(asset->headPagePtr), dataPagePtr(asset->dataPagePtr), starpakOffset(asset->starpakOffset), optStarpakOffset(-1), pageEnd(asset->pageEnd),
        remainingDependencyCount(asset->remainingDependencyCount), dependentsIndex(asset->dependentsIndex), dependenciesIndex(asset->dependenciesIndex), dependentsCount(asset->dependentsCount), dependenciesCount(static_cast<uint16_t>(asset->dependenciesCount)),
        unk2(0), headerStructSize(asset->headerStructSize), version(asset->version), type(asset->type) {};

    PakAsset_t(PakAsset_v8_t* asset) : assetPtr(asset), guid(asset->guid), headPagePtr(asset->headPagePtr), dataPagePtr(asset->dataPagePtr), starpakOffset(asset->starpakOffset), optStarpakOffset(asset->optStarpakOffset), pageEnd(asset->pageEnd),
        remainingDependencyCount(asset->remainingDependencyCount), dependentsIndex(asset->dependentsIndex), dependenciesIndex(asset->dependenciesIndex), dependentsCount(asset->dependentsCount), dependenciesCount(static_cast<uint16_t>(asset->dependenciesCount)),
        unk2(asset->unk2), headerStructSize(asset->headerStructSize), version(asset->version), type(asset->type) {};

    uint64_t guid;

    void* assetPtr;
    inline PakAsset_v6_t* GetPakAssetV6() const { return reinterpret_cast<PakAsset_v6_t*>(assetPtr); };
    inline PakAsset_v8_t* GetPakAssetV8() const { return reinterpret_cast<PakAsset_v8_t*>(assetPtr); };

    // page with SF_HEAD flag
    PagePtr_t headPagePtr;
    static inline PagePtr_t* HeadPage(void* asset, const short pakVersion) { return pakVersion > 7 ? &reinterpret_cast<PakAsset_v8_t*>(asset)->headPagePtr : &reinterpret_cast<PakAsset_v6_t*>(asset)->headPagePtr; };

    // technically "cpu" but this is confusing so just "data". SF_CPU
    PagePtr_t dataPagePtr;
    static inline PagePtr_t* DataPage(void* asset, const short pakVersion) { return pakVersion > 7 ? &reinterpret_cast<PakAsset_v8_t*>(asset)->dataPagePtr : &reinterpret_cast<PakAsset_v6_t*>(asset)->dataPagePtr; };

    // if the pointer has failed to resolve, this won't help
    bool HasValidDataPagePointer() const { return dataPagePtr.ptr != 0; };

    __int64 starpakOffset;
    __int64 optStarpakOffset;

    uint16_t pageEnd;

    // value is decremented every time a dependency finishes processing its own dependencies
    uint16_t remainingDependencyCount;

    // really not sure why this is a thing
    // index into array of asset indexes - indicates which LOCAL assets are dependent on this asset (reference by GUID)
    uint32_t dependentsIndex;

    // index into array of ptrs - items point to the page locations at which this asset has a GUID reference (to be converted to a pointer at load)
    uint32_t dependenciesIndex;

    int dependentsCount;

    uint16_t dependenciesCount;
    uint16_t unk2; // might be padding in s3. sometimes used as a separate var in later versions

    uint32_t headerStructSize;
    static inline const uint32_t HeaderStructSize(void* asset, const short pakVersion) { return pakVersion > 7 ? reinterpret_cast<PakAsset_v8_t*>(asset)->headerStructSize : reinterpret_cast<PakAsset_v6_t*>(asset)->headerStructSize; };

    int version;
    static inline const int Version(void* asset, const short pakVersion) { return pakVersion > 7 ? reinterpret_cast<PakAsset_v8_t*>(asset)->version : reinterpret_cast<PakAsset_v6_t*>(asset)->version; };

    // 4-char abbreviation of asset type
    uint32_t type;
    static inline const uint32_t Type(void* asset, const short pakVersion) { return pakVersion > 7 ? reinterpret_cast<PakAsset_v8_t*>(asset)->type : reinterpret_cast<PakAsset_v6_t*>(asset)->type; };
};

constexpr int pakFileMagic = MAKEFOURCC('R', 'P', 'a', 'k');

struct PakHdr_v6_t
{
public:
    int magic;
    short version;
    short flags;
    uint64_t createdTime;
    uint64_t crc;

    int64_t size; // data size

    char pad_0020[16];

    int streamingFilesBufSize;

    int numSegments;
    int numPages;

    int numPointers;
    int numAssets;
    int numGuidRefs;
    int numDependencies;

    int numExternalAssetRefs;
    int externalAssetRefsSize;

    int unk_0054;
};
static_assert(sizeof(PakHdr_v6_t) == 0x58);

struct PakHdr_v7_t
{
public:
    int magic;
    short version;
    short flags;
    uint64_t createdTime;
    uint64_t crc;

    int64_t cmpSize; // compressed data size

    char pad_0020[8];

    int64_t dcmpSize; // decompressed file size. will be the same as cmpSize if compressed flag is not set

    char pad_0030[8];

    short streamingFilesBufSize;

    uint16_t numSegments;
    uint16_t numPages;

    // (01).rpak - 1: base
    // (02).rpak - 2: base, (01)
    // (03).rpak - 3: base, (01), (02)
    short patchCount; // actually might be the number of pak files "under" this one

    int numPointers;
    int numAssets;
    int numGuidRefs;
    int numDependencies;

    int numExternalAssetRefs;
    int externalAssetRefsSize;
};
static_assert(sizeof(PakHdr_v7_t) == 0x58);

struct PakHdr_v8_t
{
    int magic;
    short version;
    short flags;
    uint64_t createdTime;
    uint64_t crc;

    __int64 cmpSize; // compressed data size

    char gap_20[16];

    __int64 dcmpSize; // decompressed file size. will be the same as cmpSize if compressed flag is not set
    char gap_38[16];

    short streamingFilesBufSize;
    short optStreamingFilesBufSize;

    // not sure if these are actually unsigned?
    uint16_t numSegments;
    uint16_t numPages;

    // (01).rpak - 1: base
    // (02).rpak - 2: base, (01)
    // (03).rpak - 3: base, (01), (02)
    short patchCount; // actually might be the number of pak files "under" this one

    char pad[2];

    int numPointers;
    int numAssets;
    int numGuidRefs;
    int numDependencies;

    // unused in r5
    int numExternalAssetRefs;
    int externalAssetRefsSize;

    char gap_6C[8];

    //int unkCount; // unknown, unused in n1094
    uint32_t unkDataSize_74;
    uint32_t unkDataSize_78;

    char gap_7C[4];
};
static_assert(sizeof(PakHdr_v8_t) == 0x80, "Invalid PakHdr_t struct");

struct PakHdr_t;

struct PakHdr_t
{
    PakHdr_t(const PakHdr_v6_t* const pakhdr) : magic(pakhdr->magic), version(pakhdr->version), flags(pakhdr->flags), createdTime(pakhdr->createdTime), crc(pakhdr->crc), cmpSize(pakhdr->size), dcmpSize(pakhdr->size), pakHdrSize(sizeof(PakHdr_v6_t)), pakAssetSize(sizeof(PakAsset_v6_t)),
        streamingFilesBufSize(static_cast<short>(pakhdr->streamingFilesBufSize)), optStreamingFilesBufSize(0), numSegments(static_cast<uint16_t>(pakhdr->numSegments)), numPages(static_cast<uint16_t>(pakhdr->numPages)),
        patchCount(0), numPointers(pakhdr->numPointers), numAssets(pakhdr->numAssets), numGuidRefs(pakhdr->numGuidRefs), numDependencies(pakhdr->numDependencies), numExternalAssetRefs(pakhdr->numExternalAssetRefs), externalAssetRefsSize(pakhdr->externalAssetRefsSize), pakPtr(pakhdr), pad(-1),
        unkDataSize_74(0), unkDataSize_78(0) {};

    PakHdr_t(const PakHdr_v7_t* const pakhdr) : magic(pakhdr->magic), version(pakhdr->version), flags(pakhdr->flags), createdTime(pakhdr->createdTime), crc(pakhdr->crc), cmpSize(pakhdr->cmpSize), dcmpSize(pakhdr->dcmpSize), pakHdrSize(sizeof(PakHdr_v7_t)), pakAssetSize(sizeof(PakAsset_v6_t)),
        streamingFilesBufSize(pakhdr->streamingFilesBufSize), optStreamingFilesBufSize(0), numSegments(pakhdr->numSegments), numPages(pakhdr->numPages), patchCount(pakhdr->patchCount),
        numPointers(pakhdr->numPointers), numAssets(pakhdr->numAssets), numGuidRefs(pakhdr->numGuidRefs), numDependencies(pakhdr->numDependencies), numExternalAssetRefs(pakhdr->numExternalAssetRefs), externalAssetRefsSize(pakhdr->externalAssetRefsSize), pakPtr(pakhdr), pad(-1),
        unkDataSize_74(0), unkDataSize_78(0) {};

    PakHdr_t(const PakHdr_v8_t* const pakhdr) : magic(pakhdr->magic), version(pakhdr->version), flags(pakhdr->flags), createdTime(pakhdr->createdTime), crc(pakhdr->crc), cmpSize(pakhdr->cmpSize), dcmpSize(pakhdr->dcmpSize), pakHdrSize(sizeof(PakHdr_v8_t)), pakAssetSize(sizeof(PakAsset_v8_t)),
        streamingFilesBufSize(pakhdr->streamingFilesBufSize), optStreamingFilesBufSize(pakhdr->optStreamingFilesBufSize), numSegments(pakhdr->numSegments), numPages(pakhdr->numPages), patchCount(pakhdr->patchCount),
        numPointers(pakhdr->numPointers), numAssets(pakhdr->numAssets), numGuidRefs(pakhdr->numGuidRefs), numDependencies(pakhdr->numDependencies), numExternalAssetRefs(pakhdr->numExternalAssetRefs), externalAssetRefsSize(pakhdr->externalAssetRefsSize), pakPtr(pakhdr), pad(-1),
        unkDataSize_74(pakhdr->unkDataSize_74), unkDataSize_78(pakhdr->unkDataSize_78) {};

    int magic;
    short version;
    short flags;
    uint64_t createdTime;
    uint64_t crc;

    __int64 cmpSize; // compressed data size

    __int64 dcmpSize; // decompressed file size. will be the same as cmpSize if compressed flag is not set

    const void* pakPtr; // pointer to pak buffer

    size_t pakHdrSize;
    size_t pakAssetSize;

    short streamingFilesBufSize;
    short optStreamingFilesBufSize;

    // not sure if these are actually unsigned?
    uint16_t numSegments;
    uint16_t numPages;

    // (01).rpak - 1: base
    // (02).rpak - 2: base, (01)
    // (03).rpak - 3: base, (01), (02)
    short patchCount; // actually might be the number of pak files "under" this one
    short pad;

    int numPointers;
    int numAssets;
    int numGuidRefs;
    int numDependencies;

    // unused in r5
    int numExternalAssetRefs;
    int externalAssetRefsSize;

    uint32_t unkDataSize_74;
    uint32_t unkDataSize_78;

public:

    // size of main pak header + patch headers
    inline size_t GetHeaderSize() const
    {
        size_t totalSize = pakHdrSize;

        if (patchCount > 0)
        {
            totalSize += sizeof(PakPatchDataHdr_t);

            totalSize += sizeof(PakPatchFileHdr_t) * patchCount;

            totalSize += sizeof(uint16_t) * patchCount;
        }
        return totalSize;
    }

    inline const char* GetHeaderEnd() const
    {
        return reinterpret_cast<const char*>(pakPtr) + GetHeaderSize();
    }

    inline size_t GetNonPatchedDataSize() const
    {
        size_t totalSize = GetHeaderSize();

        totalSize += streamingFilesBufSize;
        totalSize += optStreamingFilesBufSize;

        totalSize += sizeof(PakSegmentHdr_t) * numSegments;
        totalSize += sizeof(PakPageHdr_t) * numPages;
        totalSize += sizeof(PakPointerHdr_t) * numPointers;

        totalSize += static_cast<size_t>(pakAssetSize) * numAssets;

        return totalSize;
    }

    size_t GetNonPagedDataSize() const
    {
        size_t totalSize = GetNonPatchedDataSize();

        totalSize += sizeof(PakGuidRefHdr_t) * numGuidRefs;
        totalSize += sizeof(int) * numDependencies;

        totalSize += sizeof(int) * numExternalAssetRefs;
        totalSize += externalAssetRefsSize;

        // if we don't skip these it will cause issues with everything when present.
        totalSize += unkDataSize_74;
        totalSize += unkDataSize_78;

        return totalSize;
    }

    inline const PakPatchDataHdr_t* GetPatchDataHeader() const
    {
        assert(patchCount > 0);
        return reinterpret_cast<const PakPatchDataHdr_t*>((char*)pakPtr + pakHdrSize);
    }

    inline const size_t GetPatchDataSize() const
    {
        return patchCount > 0 ? GetPatchDataHeader()->patchDataStreamSize : 0;
    }

    inline const PakPatchFileHdr_t* GetPatchFileHeaders() const
    {
        return reinterpret_cast<const PakPatchFileHdr_t*>(&GetPatchDataHeader()[1]);
    }

    inline const PakPatchFileHdr_t* GetPatchFileHeader(int i) const
    {
        assert(i < patchCount);
        return GetPatchFileHeaders() + i;
    }

    inline const uint16_t* GetPatchFileIndices() const
    {
        return reinterpret_cast<const uint16_t*>(GetPatchFileHeaders() + patchCount);
    }

    inline const char* GetStreamingFilePaths() const
    {
        return reinterpret_cast<const char*>(GetHeaderEnd());
    }

    inline const char* GetOptStreamingFilePaths() const
    {
        return reinterpret_cast<const char*>(GetStreamingFilePaths() + streamingFilesBufSize);
    }

    inline const PakSegmentHdr_t* GetSegmentHeaders() const
    {
        const char* headerEnd = reinterpret_cast<const char*>((char*)pakPtr + pakHdrSize);

        if (patchCount > 0)
            headerEnd = reinterpret_cast<const char*>(GetPatchFileIndices() + patchCount);

        const char* segmentHeaders = headerEnd + streamingFilesBufSize + optStreamingFilesBufSize;
        return reinterpret_cast<const PakSegmentHdr_t*>(segmentHeaders);
    }

    inline const PakPageHdr_t* GetPageHeaders() const
    {
        return reinterpret_cast<const PakPageHdr_t*>(GetSegmentHeaders() + numSegments);
    }

    inline const size_t GetContainedPageDataSize() const
    {
        int firstPage = 0;
        if (patchCount > 0)
            firstPage = GetPatchDataHeader()->patchPageCount;

        size_t dataSize = 0;
        for (int i = firstPage; i < numPages; ++i)
        {
            const PakPageHdr_t& page = GetPageHeaders()[i];
            dataSize += page.size;
        }

        return dataSize;
    }

    inline const PakPointerHdr_t* GetPointerHeaders() const
    {
        return reinterpret_cast<const PakPointerHdr_t*>(GetPageHeaders() + numPages);
    }

    inline const void* GetAssets() const
    {
        return reinterpret_cast<const void*>(GetPointerHeaders() + numPointers);
    }

    inline const PakGuidRefHdr_t* GetGuidRefHeaders() const
    {
        return reinterpret_cast<const PakGuidRefHdr_t*>(reinterpret_cast<const char*>(GetAssets()) + (numAssets * pakAssetSize));
    }

    inline const int* GetDependentAssetData() const
    {
        return reinterpret_cast<const int*>(GetGuidRefHeaders() + numGuidRefs);
    }

    inline const int* GetExternalAssetRefOffsets() const
    {
        return reinterpret_cast<const int*>(GetDependentAssetData() + numDependencies);
    }

    inline const char* GetExternalAssetRef() const
    {
        return reinterpret_cast<const char*>(GetExternalAssetRefOffsets() + numExternalAssetRefs);
    }

    inline const char* GetPatchStreamData() const
    {
        return reinterpret_cast<const char*>(pakPtr) + GetNonPagedDataSize();
    }
};
//static_assert(sizeof(PakHdr_t) == 0x80, "Invalid PakHdr_t struct");

#define IS_ASSET_PTR_INVALID(ptr) (ptr.offset == 0 || ptr.size == 0)
union AssetPtr_t
{
    struct
    {
        uint64_t offset;
        uint64_t size;
    };

    char* ptr;
};

struct StarPak_t
{
    std::unordered_map<uint64_t, size_t> parsedOffsets;
    std::string filePath;
};

#if defined(PAKLOAD_PATCHING_ANY)
struct SegmentCollection_t
{
    enum eType
    {
        SCT_HEAD = 0, // Asset headers
        SCT_CPU = 1, // "cpu" segments
        SCT_UNUSED = 2, // Unused
        SCT_TEMP = 3, // "temp" segments
    };

    size_t dataSize;
    uint32_t dataAlignment;

    // !! aligned to dataAlignment !!
    // !! DO NOT USE REGULAR FREE  !!
    char* buffer;
};

// Info stored for each asset type that is present in a loaded pakfile.
struct PakLoadedAssetTypeInfo_t
{
    // When a pak is loaded, we allocate each loaded asset type a portion of the "HEADER" segment collection
    // equal to the size of the asset header multiplied by the number of occurrences of the asset type in the pak.
    // At this step, the game would usually allocate a small amount of extra memory for the creation of a native class instance
    // using the regular header struct at the start of the struct.
    // Since we wrap PakAsset_t with CPakAsset in this tool, we don't allocate this extra memory, as our extended asset data
    // is accessed with CPakAsset::m_ExtraData.

    // This variable stores the next offset for a header of this asset type in the "HEADER" segment collection
    size_t offsetToNextHeaderInCollection;
    size_t assetCount;

    // This is used for the calculation of the "next header offset in collection" values
    // and the size of the full "HEADER" segment collection.
    uint32_t headerSize;

    uint32_t version;
};
#endif // #if defined(PAKLOAD_PATCHING_ANY)

class CPakFile : public CAssetContainer
{
public:
    CPakFile();
    ~CPakFile();

    const CAsset::ContainerType GetContainerType() const { return CAsset::ContainerType::PAK; };

    const bool ParseFileBuffer(const std::string& path);
    static const bool DecompressFileBuffer(const char* fileBuffer, std::shared_ptr<char[]>* outBuffer);

#if defined(PAKLOAD_PATCHING_ANY)
public:
    typedef bool(*PatchFunc_t)(CPakFile* const pak, size_t* const numRemainingFileBufferBytes);

    friend bool PatchCmd_0(CPakFile* const pak, size_t* const numRemainingFileBufferBytes);
    friend bool PatchCmd_1(CPakFile* const pak, size_t* const numRemainingFileBufferBytes);
    friend bool PatchCmd_2(CPakFile* const pak, size_t* const numRemainingFileBufferBytes);
    friend bool PatchCmd_3(CPakFile* const pak, size_t* const numRemainingFileBufferBytes);
    friend bool PatchCmd_4_5(CPakFile* const pak, size_t* const numRemainingFileBufferBytes);
    friend bool PatchCmd_6(CPakFile* const pak, size_t* const numRemainingFileBufferBytes);
#endif // #if defined(PAKLOAD_PATCHING_ANY)

private:
    std::string m_FilePath;

    PakHdr_t* m_pHeader;

    PakPatchDataHdr_t* m_pPatchDataHeader;
    PakPatchFileHdr_t* m_pPatchFileHeaders;

    char* patchStreamCursor; // pointer that moves through the patch data buffer as it is processed
#if defined(PAKLOAD_PATCHING_ANY)
    // Base pointer for the patch data buffer.
    // This data contains an encoded set of instructions/commands that tell the code how to modify the earlier
    // versions of this pak file to bring their data to the latest version.
    std::shared_ptr<char[]> patchDataBuffer;

    // large buffers containing all segment/page data of each type (head, cpu, temp)
    SegmentCollection_t segmentCollections[4];
#endif // #if defined(PAKLOAD_PATCHING_ANY)

#if defined(PAKLOAD_PATCHING_ANY)
protected:
    struct
    {
        char commands[64];
        char unk[64];
        char unk2[256];
        char unk3[256];

        RBitRead bitbuf;

        size_t numBytesToPatch; // patch operand value - number of bytes used by this operation
        size_t numBytesToSkip; // number of bytes to skip patching for

        size_t numRemainingFileBufferBytes; // number of bytes left in the whole file buffer

        size_t offsetInFileBuffer;

        size_t patchDestinationSize; // number of bytes left to write at the patch destination


        // source in the patch stream's replacement data
        char* patchReplacementData; // pointer to the location that patch data will be taken from

        char* patchOriginalData; // source for data to patch within the pak data
        char* patchDestination; // pointer to the patch destination that will receive the patch data

        PatchFunc_t patchFunc;
    } p;

    // This represents the number of assets in the pak that have had all of their required pages patched
    // into their respective segment collections.
    int numAssetsWithProcessedPages;

    // This is the number of assets that have had their headers copied into the "HEADER" segment collection.
    int numProcessedAssets;

    // Number of pages that have been patched into their respective segment collections.
    int numProcessedPages;

    // Number of page pointers that have been converted from INDEX-OFFSET pairs into native pointers.
    int numResolvedPointers;

    // Number of page pointers that have been adjusted for the move from a dedicated page into their type's allocated
    // header data in the "HEADER" segment collection
    int numShiftedPointers;

    // 
    std::unordered_map<uint32_t, PakLoadedAssetTypeInfo_t> loadedAssetTypeInfo;

    FORCEINLINE std::shared_ptr<char[]> GetDecompressedBuffer() { return m_Buf; };

    FORCEINLINE void SetPatchBytesToSkip(size_t numBytes) { p.numBytesToSkip = numBytes; };
    FORCEINLINE void SetPatchSourceData(void* pointer) { p.patchOriginalData = reinterpret_cast<char*>(pointer); };

    FORCEINLINE void SetPatchDestination(void* pointer, size_t destinationSize)
    {
        p.patchDestination = reinterpret_cast<char*>(pointer);
        p.patchDestinationSize = destinationSize;
    };

    void SetPatchCommand(const int8_t cmd);

    const int ResolvePointers();
    template<class PakAsset> const int FindNextAssetWithUnpatchedPages();
    template<class PakAsset> bool ProcessNextPage();

#endif // #if defined(PAKLOAD_PATCHING_ANY)
private:

    std::vector<std::unique_ptr<StarPak_t>> m_vStarPaks;
    std::vector<std::unique_ptr<StarPak_t>> m_vOptStarPaks;

    PakSegmentHdr_t* m_pSegmentHeaders;

    PakPageHdr_t* m_pPageHeaders;
    PakPointerHdr_t* m_pPointerHeaders;

    void* m_pAssetsRaw;
    PakAsset_t* m_pAssetsInternal;

#if defined(PAKLOAD_PATCHING_ANY)
    // pointers to each asset entry, sorted by header index and offset
    std::vector<void*> sortedAssetPointers;
#endif // #if defined(PAKLOAD_PATCHING_ANY)

    PakGuidRefHdr_t* m_pGuidRefHeaders;
    int* m_pDependentAssets;

    // offset into char array
    int* m_pExternalAssetRefOffsets;
    char* m_pExternalAssetRefs;

    // vector of pointers to the start of each page
    std::vector<char*> pageBuffers;
    int firstPageIdx;

    std::shared_ptr<char[]> m_Buf;

private:

    // Populates CPakFile members from file
    const bool ParseFromFile(const std::string& filePath, std::shared_ptr<char[]>& buf);
    const bool ParseStreamedFile(const std::string& fileName, bool opt);

#if defined(PAKLOAD_PATCHING_ANY)
    void ParsePatchEditStream();
    const bool DecodePatchCommands();

    void CreateHeaderSegmentCollection();
    void AllocateSegments();
#endif

    const bool ParsePakFileHeader(const char* buf, const short version);
    const bool ParsePakFileHeader(const char* buf);

#if !defined(PAKLOAD_PATCHING_ANY) || defined(PAKLOAD_LOADING_V6)
    const bool LoadNonPatched();
#endif // #if !defined(PAKLOAD_PATCHING_ANY) || defined(PAKLOAD_LOADING_V6)
#if defined(PAKLOAD_PATCHING_ANY)
    template<class PakAsset> const bool LoadAndPatchAssetData();
    template<class PakHdr, class PakAsset> const bool LoadAndPatchPakFileData();

    void CalculateLoadedAssetTypeInfo();
    template<class PakAsset> void SortAssetsByHeaderPointer();

    // [rika]: const cast?
    inline void ResetHeaders()
    {
        // header handled differently
        m_pSegmentHeaders = const_cast<PakSegmentHdr_t*>(m_pHeader->GetSegmentHeaders());
        m_pPageHeaders = const_cast<PakPageHdr_t*>   (m_pHeader->GetPageHeaders());
        m_pPointerHeaders = const_cast<PakPointerHdr_t*>(m_pHeader->GetPointerHeaders());
        m_pAssetsRaw = const_cast<void*>           (m_pHeader->GetAssets());
        m_pGuidRefHeaders = const_cast<PakGuidRefHdr_t*>(m_pHeader->GetGuidRefHeaders());
        m_pDependentAssets = const_cast<int*>            (m_pHeader->GetDependentAssetData());
    }
#endif // #if defined(PAKLOAD_PATCHING_ANY)

    void ProcessAssets();

public:
    inline PakHdr_t* header() const { return m_pHeader; };
    inline PakAsset_t* internalAssets() const { return m_pAssetsInternal; };
    inline void* rawAsset(const size_t idx) const { return reinterpret_cast<char*>(m_pAssetsRaw) + (header()->pakAssetSize * idx); }
    inline StarPak_t* getStarPak(int idx, bool opt) const
    {
        auto vPak = opt ? &m_vOptStarPaks : &m_vStarPaks;
        if (vPak->size() <= idx)
            return nullptr;

        return vPak->at(idx).get();
    }

    inline const int* const dependents() const { return m_pDependentAssets; }; // [rika]: TEMP! rexx is gonna kill me!!!
    inline const PakGuidRefHdr_t* const guidRefs() const { return m_pGuidRefHeaders; };

    inline int assetCount() const { return m_pHeader->numAssets; };
    inline int segmentCount() const { return m_pHeader->numSegments; };
    inline int pageCount() const { return m_pHeader->numPages; };
    inline int pointerCount() const { return m_pHeader->numPointers; };
    inline int patchCount() const { return m_pHeader->patchCount; };
    inline int firstPageIndex() const { return firstPageIdx; };

    inline void* getPointerToPageOffset(const PagePtr_t& pagePointer)
    {
        assert(pagePointer.index < pageBuffers.size());
        return pageBuffers[pagePointer.index] + pagePointer.offset;
    }
//#if defined(PAKLOAD_PATCHING_ANY)

    std::string getPakStem() const
    {
        const std::string stem = std::filesystem::path(this->m_FilePath).stem().string();

        if (patchCount() == 0)
            return stem;

        auto endIterator = stem.end();
        for (size_t i = 0; i < stem.length(); ++i)
        {
            if (stem[i] == '(')
            {
                endIterator = stem.begin() + i;
                break;
            }
        }

        const std::string pakStem = std::string(stem.begin(), endIterator); // remove "(xx)" patch number suffix from the stem

        return pakStem;
    }
//#endif // #if defined(PAKLOAD_PATCHING_ANY)
};

#if defined(PAKLOAD_PATCHING_ANY)
template<class PakAsset>
const int CPakFile::FindNextAssetWithUnpatchedPages()
{
    for (int i = this->numAssetsWithProcessedPages; i < this->assetCount(); ++i)
    {
        const PakAsset* const asset = reinterpret_cast<PakAsset*>(this->sortedAssetPointers[i]);
        if (asset->pageEnd > this->numProcessedPages)
            return i;

        this->numAssetsWithProcessedPages++;
    }

    return this->assetCount();
}

template<class PakAsset>
bool CPakFile::ProcessNextPage()
{
    // resolve pointers in all of the newly processed pages
    this->numShiftedPointers = this->numResolvedPointers = this->ResolvePointers();

    // get the correct value of numProcessedAssets according to the number of processed pages
    this->FindNextAssetWithUnpatchedPages<PakAsset>();

    if (this->numProcessedPages == this->pageCount()) // all pages are patched!
        return false;

    int nextPageIdx = this->numProcessedPages + this->firstPageIdx;
    if (nextPageIdx >= this->pageCount())
        nextPageIdx -= this->pageCount();

    this->numProcessedPages++;

    const PakPageHdr_t* const nextPage = &this->m_pPageHeaders[nextPageIdx];
    const PakSegmentHdr_t* const segment = &this->m_pSegmentHeaders[nextPage->segment];

    if (segment->IsHeaderSegment())
    {
        const PakAsset* const asset = reinterpret_cast<PakAsset*>(this->sortedAssetPointers[this->numProcessedAssets]);
        PakLoadedAssetTypeInfo_t& assetTypeInfo = this->loadedAssetTypeInfo.at(asset->type);

        // Get the next usable offset in the HEADER segment collection for this asset type.
        char* const destination = this->segmentCollections[SegmentCollection_t::eType::SCT_HEAD].buffer + assetTypeInfo.offsetToNextHeaderInCollection;
        this->SetPatchDestination(destination, asset->headerStructSize);

        // Advance the offset for this asset type to account for the asset header we will write on the next patch operation.
        assetTypeInfo.offsetToNextHeaderInCollection += asset->headerStructSize;
    }
    else
    {
        this->SetPatchDestination(this->pageBuffers[nextPageIdx], nextPage->size);
    }

    return true;
}

template<class PakAsset>
const bool CPakFile::LoadAndPatchAssetData()
{
    while (this->numAssetsWithProcessedPages < this->assetCount())
    {
        if (this->p.patchDestinationSize + this->p.numBytesToSkip == 0)
        {
            if (!this->ProcessNextPage<PakAsset>())
                break;

            continue;
        }

        if (!this->DecodePatchCommands())
        {
            assertm(false, "failed to decode patch command");
            return false;
        }

        // patching begins with the pages of the top patch before
        // wrapping back around to page 0 in the base rpak
        // i.e., when numProcessedAssets is 0, the page being patched is the one at [firstPageIdx]
        int shiftedPageIndex = ((this->firstPageIdx - 1) + this->numProcessedPages);
        if (shiftedPageIndex >= this->pageCount())
            shiftedPageIndex -= this->pageCount();

        const PakPageHdr_t* const page = &header()->GetPageHeaders()[shiftedPageIndex];
        const PakSegmentHdr_t* const segment = &header()->GetSegmentHeaders()[page->segment];

        if (!segment->IsHeaderSegment())
        {
            if (!this->ProcessNextPage<PakAsset>())
                break;

            continue;
        }

        PakAsset* const asset = reinterpret_cast<PakAsset*>(this->sortedAssetPointers[this->numProcessedAssets]);
        const uint32_t originalHeaderOffset = asset->headPagePtr.offset;

        //#undef LODWORD
#define LODWORD(x)  (*((uint32_t*)&(x)))
        // We must update the asset's offset to point to the header's new location in the segment collection buffer, as the buffer for all asset header pages
        // has been set to the SCT_HEAD segment collection buffer.
        const uint32_t newOffsetFromCollectionBufferToHeader = LODWORD(this->p.patchDestination) - asset->headerStructSize - LODWORD(this->segmentCollections[0].buffer);

        asset->headPagePtr.offset = newOffsetFromCollectionBufferToHeader;

        // Get the size of the difference between the old offset and the new one so that any pointers within the header can be updated.
        const uint32_t shiftSize = newOffsetFromCollectionBufferToHeader - originalHeaderOffset;

        for (int i = this->numShiftedPointers; i < this->pointerCount(); i++)
        {
            PakPointerHdr_t* const ptr = &this->m_pPointerHeaders[i];

            // If this pointer is not in the page that we are about to shift, we can break out of the loop
            // as we have finished shifting pointers. This is possible as pointers must be sorted in ascending order.
            if (ptr->index != shiftedPageIndex)
                break;

            // Get the distance between this pointer's offset and the start of the asset header that is about to be shifted.
            const uint32_t gapBetweenHeaderAndPointer = ptr->offset - originalHeaderOffset;

            // If the distance between the start of the header and the offset of the pointer is greater than or equal to the size of the header
            // then the pointer is not located within the header, so we can safely ignore this pointer and any subsequent pointers.
            if (gapBetweenHeaderAndPointer >= asset->headerStructSize)
                break;

            PagePtr_t* const pagePtr = reinterpret_cast<PagePtr_t*>((this->p.patchDestination - asset->headerStructSize) + gapBetweenHeaderAndPointer);

            ptr->offset += shiftSize;

            if (pagePtr->index == shiftedPageIndex)
                pagePtr->offset += shiftSize;

            this->numShiftedPointers++;
        }

        for (uint16_t i = 0; i < asset->dependenciesCount; ++i)
        {
            PakGuidRefHdr_t* const guidRef = &this->m_pGuidRefHeaders[asset->dependenciesIndex + i];
            if (guidRef->index == shiftedPageIndex)
                guidRef->offset += shiftSize;
        }

        const int nextAssetIndex = ++this->numProcessedAssets;
        const PakAsset* nextAsset = nullptr;

        // If there is a next asset to patch and it is within the page that we are currently dealing with.
        if (nextAssetIndex < this->assetCount() && (nextAsset = reinterpret_cast<PakAsset*>(this->sortedAssetPointers[nextAssetIndex]), nextAsset->headPagePtr.index == shiftedPageIndex))
        {
            this->SetPatchBytesToSkip(static_cast<size_t>(nextAsset->headPagePtr.offset - originalHeaderOffset - asset->headerStructSize));

            PakLoadedAssetTypeInfo_t* const assetTypeInfo = &this->loadedAssetTypeInfo[nextAsset->type];
            this->SetPatchDestination(
                this->segmentCollections[SegmentCollection_t::eType::SCT_HEAD].buffer + assetTypeInfo->offsetToNextHeaderInCollection,
                nextAsset->headerStructSize
            );

            assetTypeInfo->offsetToNextHeaderInCollection += nextAsset->headerStructSize;
        }
        else
        {
            if (!this->ProcessNextPage<PakAsset>())
                break;

            continue;
        }
    }

    return true;
}
#endif

class CPakAsset;

// pak asset wrapper class
class CPakAsset : public CAsset
{
private:
    //CPakFile* m_containerFile;
    //PakAsset_t* m_assetData;
    //std::string m_assetName;

    std::shared_ptr<void> m_ExtraData;

    // indicates whether this asset has been exported successfully since it was loaded
    // stores the result of whether the last export attempt succeeded
    //bool m_exported;
public:
    CPakAsset(CPakFile* pak, PakAsset_t* asset, std::string name)
    {
        // set initial export status to false
        SetExportedStatus(false);

        SetAssetName(name);
        SetAssetVersion(asset->version);
        SetInternalAssetData(asset);
        SetContainerFile(pak);
    };

    CPakAsset() = default;
    ~CPakAsset() = default;

    //void SetAssetName(const std::string& path) { m_assetName = std::filesystem::path(path).make_preferred().string(); }

    uint32_t GetAssetType() const { return data()->type; }

    const uint64_t GetAssetGUID() const { return data()->guid; }

    const ContainerType GetAssetContainerType() const
    {
        return ContainerType::PAK;
    };

    std::string GetContainerFileName() const
    {
        return pak()->getPakStem() + ".rpak";
    }

    uint64_t guid() { return data()->guid; };

    const int version() { return data()->version; };


    PakAsset_t* const data() { return static_cast<PakAsset_t*>(m_assetData); };
    const PakAsset_t* const data() const { return static_cast<const PakAsset_t*>(m_assetData); };

    char* const extraData() { return reinterpret_cast<char*>(m_ExtraData.get()); };

    template <typename T>
    void setExtraData(T* const data)
    {
        std::shared_ptr<T> ptr(data);
        m_ExtraData = std::move(ptr);
    };

    void* const header() { return data()->headPagePtr.ptr; };
    char* const cpu() { return data()->dataPagePtr.ptr; };

    __int64 starpakOffset() const
    {
        if (data()->starpakOffset == -1)
            return -1;

        return data()->starpakOffset & 0xFFFFFFFFFFFFF000;
    }

    int starpakIdx() const
    {
        if (data()->starpakOffset == -1)
            return -1;

        return data()->starpakOffset & 0xFFF;
    }

    __int64 optStarpakOffset() const
    {
        if (data()->optStarpakOffset == -1)
            return -1;

        return data()->optStarpakOffset & 0xFFFFFFFFFFFFF000;
    }

    int optStarpakIdx() const
    {
        if (data()->optStarpakOffset == -1)
            return -1;

        return data()->optStarpakOffset & 0xFFF;
    }

    void getDependencies(std::vector<AssetGuid_t>& guidsArray)
    {
        const uint16_t numDeps = data()->dependenciesCount;
        guidsArray.resize(numDeps);

        const PakGuidRefHdr_t* const guidRefs = &pak()->guidRefs()[data()->dependenciesIndex];

        for (int i = 0; i < numDeps; ++i)
        {
            const AssetGuid_t* pGuid = reinterpret_cast<AssetGuid_t*>(pak()->getPointerToPageOffset(guidRefs[i]));
            guidsArray[i] = *pGuid;
        }
    }

    inline const StarPak_t* getStarPak(const bool opt) const
    {
        const int pakIdx = opt ? optStarpakIdx() : starpakIdx();
        const StarPak_t* const pakEntry = pak()->getStarPak(pakIdx, opt);

        if (!pakEntry)
            return nullptr;

        return pakEntry;
    }

    AssetPtr_t getStarPakStreamEntry(const bool opt)
    {
        const StarPak_t* const pakEntry = getStarPak(opt);
        if (!pakEntry)
            return {};

        const __int64 seek = opt ? optStarpakOffset() : starpakOffset();
        const auto it = pakEntry->parsedOffsets.find(seek);
        if (it == pakEntry->parsedOffsets.end())
            return {};

        return { { it->first, it->second } };
    }

    std::unique_ptr<char[]> getStarPakData(const uint64_t offset, const uint64_t size, const bool opt) const
    {
        const StarPak_t* const pakEntry = getStarPak(opt);
        if (!pakEntry)
            return nullptr;

        assertm(offset > 0, "starpak offset can't be zero.");
        assertm(size > 0, "starpak size can't be zero.");

        StreamIO file;
        if (!file.open(pakEntry->filePath, eStreamIOMode::Read))
            return nullptr;

        file.seek(offset);

        std::unique_ptr<char[]> data(new char[size]);
        file.read(data.get(), size);

        return data;
    }

    const char* getStarPakName(const bool opt) const
    {
        const StarPak_t* const pakEntry = getStarPak(opt);

        if (!pakEntry)
            return nullptr;

        const std::string& filePath = pakEntry->filePath;

        size_t pos = filePath.rfind('\\');

        if (pos == std::string_view::npos)
            pos = filePath.rfind('/');

        if (pos != std::string_view::npos)
            return &filePath[pos+1]; // +1 to skip the separator.
        else
            return filePath.c_str();
    }

private:
    CPakFile* const pak() { return static_cast<CPakFile*>(m_containerFile); };
    const CPakFile* const pak() const { return static_cast<const CPakFile*>(m_containerFile); };

};


// used for type init funcs
typedef void(*PakTypeInitFunc_t)(void);
