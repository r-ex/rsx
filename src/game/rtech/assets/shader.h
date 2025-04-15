#pragma once
#include <d3d11.h>
#include "core/render/dx.h"

struct ShaderAssetHeader_v8_t
{
    char* name;
    eShaderType type;

    int numShaders; // some count of sorts

    void* unk_10;
    int64_t* shaderInputFlags;
};
static_assert(sizeof(ShaderAssetHeader_v8_t) == 32);
static_assert(offsetof(ShaderAssetHeader_v8_t, shaderInputFlags) == 0x18);

struct ShaderAssetHeader_v12_t
{
    char* name;
    eShaderType type;

    char unk_9[7];

    void* unk_10;
    int64_t* shaderInputFlags;
};
static_assert(sizeof(ShaderAssetHeader_v12_t) == 32);
static_assert(offsetof(ShaderAssetHeader_v12_t, shaderInputFlags) == 0x18);

struct ShaderAssetHeader_v13_t
{
    char* name;
    eShaderType type;

    char unk_9[7];

    void* unk_10;
    int64_t* shaderInputFlags;

    uint8_t unk_20[8]; // weird
};
static_assert(sizeof(ShaderAssetHeader_v13_t) == 40);
static_assert(offsetof(ShaderAssetHeader_v13_t, shaderInputFlags) == 0x18);

// R5pc_r5-91_J214_CL1361852_2021_07_13_19_26 (s9 is v13, s10 is v15)
struct ShaderAssetHeader_v14_t
{
    char* name;
    eShaderType type;

    char unk_9[7];

    void* unk_10;
    int64_t* shaderInputFlags;

    uint64_t unk_20; // weird

    void* unk_28; // unk_30 in v15
};
static_assert(sizeof(ShaderAssetHeader_v14_t) == 48);
static_assert(offsetof(ShaderAssetHeader_v14_t, shaderInputFlags) == 0x18);

struct ShaderAssetHeader_v15_t
{
	char* name;
    eShaderType type;

    char unk_9[9];

    uint8_t unk_12[6];

    void* unk_18; // "m_ppShader", "unk_10", sometimes a pointer, sometimes a guid.
    int64_t* shaderInputFlags;

    uint64_t unk_28;

    void* unk_30;
};
static_assert(sizeof(ShaderAssetHeader_v15_t) == 56);
static_assert(offsetof(ShaderAssetHeader_v15_t, shaderInputFlags) == 0x20);

//struct ShaderBuffer_t
//{
//    char* data;
//    int size;
//    int unk; // shrug
//};

struct ShaderAssetCPU_t
{
    char* data;
    int dataSize;
    int unk_C; // varies between shaders?
};

struct ShaderBufEntry_t
{
    const char* buffer;
    int bufferSize;
    int shaderIdx;

    bool isNullBuffer : 1;
    bool isZeroLength : 1;
    bool isRef : 1;
};

class ShaderAsset
{
public:
    ShaderAsset() = default;
    ShaderAsset(ShaderAssetHeader_v8_t* const hdr, ShaderAssetCPU_t* const cpu)  : name(hdr->name), type(hdr->type), inputFlags(hdr->shaderInputFlags), data(cpu->data), dataSize(cpu->dataSize), numShaders(hdr->numShaders){ };
    ShaderAsset(ShaderAssetHeader_v12_t* const hdr, ShaderAssetCPU_t* const cpu) : name(hdr->name), type(hdr->type), inputFlags(hdr->shaderInputFlags), data(cpu->data), dataSize(cpu->dataSize), numShaders(-1)
    {
        if (hdr->unk_9[0] != -1)
        {
            this->numShaders = hdr->unk_9[0];

            return;
        };

        int v6 = 2 * ((hdr->unk_9[1] != 0) + 1);
        if (!hdr->unk_9[2])
            v6 = (hdr->unk_9[1] != 0) + 1;

        int v7 = v6;

        if (hdr->unk_9[3])
            v7 *= 2;

        numShaders = 2 * v7;
        if (!hdr->unk_9[6])
            numShaders = v7;

        memcpy_s(shaderFeatures, sizeof(shaderFeatures), &hdr->unk_9, sizeof(hdr->unk_9));
    };
    ShaderAsset(ShaderAssetHeader_v13_t* const hdr, ShaderAssetCPU_t* const cpu) : name(hdr->name), type(hdr->type), inputFlags(hdr->shaderInputFlags), shaderFeatures{}, data(cpu->data), dataSize(cpu->dataSize), numShaders(-1)
    {
        if (hdr->unk_9[0] != -1)
        {
            this->numShaders = hdr->unk_9[0];

            return;
        };

        int v6 = 2 * ((hdr->unk_9[1] != 0) + 1);
        if (!hdr->unk_9[2])
            v6 = (hdr->unk_9[1] != 0) + 1;

        int v7 = v6;

        if (hdr->unk_9[3])
            v7 *= 2;

        numShaders = 2 * v7;
        if (!hdr->unk_9[6])
            numShaders = v7;

        memcpy_s(shaderFeatures, sizeof(shaderFeatures), &hdr->unk_9, sizeof(hdr->unk_9));
    };
    ShaderAsset(ShaderAssetHeader_v14_t* const hdr, ShaderAssetCPU_t* const cpu) : name(hdr->name), type(hdr->type), inputFlags(hdr->shaderInputFlags), shaderFeatures{}, data(cpu->data), dataSize(cpu->dataSize), numShaders(-1)
    {
        if (hdr->unk_9[0] != -1)
        {
            this->numShaders = hdr->unk_9[0];

            return;
        };

        int v6 = 2 * ((hdr->unk_9[1] != 0) + 1);
        if (!hdr->unk_9[2])
            v6 = (hdr->unk_9[1] != 0) + 1;

        int v7 = v6;

        if (hdr->unk_9[3])
            v7 *= 2;

        numShaders = 2 * v7;
        if (!hdr->unk_9[6])
            numShaders = v7;

        memcpy_s(shaderFeatures, sizeof(shaderFeatures), &hdr->unk_9, sizeof(hdr->unk_9));
    };
    ShaderAsset(ShaderAssetHeader_v15_t* const hdr, ShaderAssetCPU_t* const cpu) : name(hdr->name), type(hdr->type), inputFlags(hdr->shaderInputFlags), shaderFeatures{}, data(cpu->data), dataSize(cpu->dataSize), numShaders(-1)
    {
        int shaderCount = hdr->unk_9[0];
        if (shaderCount == -1)
        {
            shaderCount = (hdr->unk_9[1] != 0) + 1;

            if (hdr->unk_9[2])
                shaderCount *= 2;
            if (hdr->unk_9[3])
                shaderCount *= 2;
            if (hdr->unk_9[4])
                shaderCount *= 2;
            if (hdr->unk_9[7])
                shaderCount *= 2;
        }

        numShaders = shaderCount;
    }

#if defined(ADVANCED_MODEL_PREVIEW) // shader instances are only created when AMP is enabled
    ~ShaderAsset()
    {
        if (!data)
            return;

        switch (type)
        {
        case eShaderType::Pixel:
        {
            pixelShader->Release();
            break;
        }
        case eShaderType::Vertex:
        {
            vertexShader->Release();
            break;
        }
        case eShaderType::Geometry:
        {
            geometryShader->Release();
            break;
        }
        case eShaderType::Hull:
        {
            hullShader->Release();
            break;
        }
        case eShaderType::Domain:
        {
            domainShader->Release();
            break;
        }
        case eShaderType::Compute:
        {
            computeShader->Release();
            break;
        }
        }
    }
#endif

    char* name;
    eShaderType type;
    int numShaders; // number of bytecode buffers
    
    char* data;
    int dataSize;

    int64_t* inputFlags;

    unsigned char shaderFeatures[8];

    union
    {
        ID3D11PixelShader* pixelShader;
        ID3D11VertexShader* vertexShader;
        ID3D11GeometryShader* geometryShader;
        ID3D11HullShader* hullShader;
        ID3D11DomainShader* domainShader;
        ID3D11ComputeShader* computeShader;
    };

    ID3D11InputLayout* vertexInputLayout;

    std::vector<ShaderBufEntry_t> shaderBuffers;
    std::vector<std::string> compilerStrings;
};

// dxbc
//https://github.com/microsoft/D3D12TranslationLayer/blob/master/DxbcParser/include/BlobContainer.h

#define DXBC_FOURCC_NAME    (('C'<<24)+('B'<<16)+('X'<<8)+'D')
#define DXBC_FOURCC_RDEF    (('F'<<24)+('E'<<16)+('D'<<8)+'R')


// D3D10_Type
struct RDEFType
{
    uint16_t Class; // D3D10_SHADER_VARIABLE_CLASS
    uint16_t Type; // D3D10_SHADER_VARIABLE_TYPE
    uint16_t Rows;
    uint16_t Columns;
    uint16_t Elements;
    uint16_t Members;
    uint16_t Offset;
    uint16_t unk_D;
    uint32_t Reserved[4];
    uint32_t NameOffset;

    inline const char* Name(void* rdefBlob) const { return reinterpret_cast<const char*>((char*)rdefBlob + NameOffset); }
};

//D3D11_Constant
struct RDEFConst
{
    uint32_t NameOffset;
    uint32_t StartOffset;
    uint32_t Size;
    D3D10_SHADER_VARIABLE_FLAGS Flags;
    uint32_t TypeOffset;
    uint32_t DefaultValue;
    uint32_t Reserved[4];

    inline const char* Name(void* rdefBlob) const { return reinterpret_cast<const char*>((char*)rdefBlob + NameOffset); }
    RDEFType* pType(void* rdefBlob) const { return reinterpret_cast<RDEFType*>((char*)rdefBlob + TypeOffset); }
};

// D3D10_ConstantBuffer
struct RDefConstBuffer
{
    uint32_t NameOffset;
    uint32_t ConstCount;
    uint32_t ConstOffset;
    uint32_t ConstBufSize;
    D3D10_CBUFFER_TYPE Type;
    D3D10_SHADER_CBUFFER_FLAGS Flags;

    inline const char* Name(void* rdefBlob) const { return reinterpret_cast<const char*>((char*)rdefBlob + NameOffset); }
    inline RDEFConst* pConst(void* rdefBlob, uint32_t i) const { assertm(i < ConstCount, "index exceeded ConstCount"); return reinterpret_cast<RDEFConst*>((char*)rdefBlob + ConstOffset) + i; }

};

// D3D10_ResourceBinding
struct RDEFResourceBinding
{
    uint32_t NameOffset; // from start of RDEFBlobHeader
    D3D_SHADER_INPUT_TYPE Type;
    D3D_RESOURCE_RETURN_TYPE ReturnType;
    D3D10_SRV_DIMENSION Dimension;
    uint32_t NumSamples;
    uint32_t BindPoint;
    uint32_t BindCount;
    D3D_SHADER_INPUT_FLAGS Flags;

    inline const char* Name(void* rdefBlob) const { return reinterpret_cast<const char*>((char*)rdefBlob + NameOffset); }
};

enum ShaderType : uint16_t
{
    ComputeShader = 0x4353,
    DomainShader = 0x4453,
    GeometryShader = 0x4753,
    HullShader = 0x4853,
    VertexShader = 0xFFFE,
    PixelShader = 0xFFFF,
};

// D3D11_RDEF_Header
struct RDEFBlobHeader
{
    uint32_t ConstBufferCount;
    uint32_t ConstBufferOffset;
    uint32_t BoundResourceCount;
    uint32_t BoundResourceOffset;
    uint8_t  VersionMinor;
    uint8_t  VersionMajor;
    ShaderType ShaderType;
    uint32_t CompilerFlags;
    uint32_t CreatorOffset; // name of compiler?
    uint32_t ID; // 'RD11'
    uint32_t unk_20[7];

    const char* CompilerSignature() const { return reinterpret_cast<const char*>((char*)this + CreatorOffset); }
    RDEFResourceBinding* pBoundResource(uint32_t i) const { assertm(i < BoundResourceCount, "index exceeded BoundResourceCount"); return reinterpret_cast<RDEFResourceBinding*>((char*)this + BoundResourceOffset) + i; }
    RDefConstBuffer* pConstBuffer(uint32_t i) const { assertm(i < ConstBufferCount, "index exceeded ConstBufferCount"); return reinterpret_cast<RDefConstBuffer*>((char*)this + ConstBufferOffset) + i; }
};

struct DXBCBlobHeader
{
    uint32_t    BlobFourCC;
    uint32_t    BlobSize;    // Byte count for BlobData

    inline bool isRDEF() const { return BlobFourCC == DXBC_FOURCC_RDEF ? true : false; }
    RDEFBlobHeader* const pRDEFBlob() const { return reinterpret_cast<RDEFBlobHeader*>((char*)this + sizeof(DXBCBlobHeader)); }
};

#define DXBC_HASH_SIZE 16
struct DXBCHash
{
    unsigned char Digest[DXBC_HASH_SIZE];
};

struct DXBCVersion
{
    uint16_t Major;
    uint16_t Minor;
};

struct DXBCHeader
{
    uint32_t    DXBCHeaderFourCC;
    DXBCHash    Hash;
    DXBCVersion Version;
    uint32_t    ContainerSizeInBytes; // Count from start of this header, including all blobs
    uint32_t    BlobCount;
   
    inline bool isValid() const { return DXBCHeaderFourCC == DXBC_FOURCC_NAME ? true : false; }
    uint32_t* const pBlobOffset(uint32_t i) const { assertm(i < BlobCount, "index exceeded BlobCount"); return reinterpret_cast<uint32_t*>((char*)this + sizeof(DXBCHeader)) + i; }
    uint32_t BlobOffset(uint32_t i) const { return *pBlobOffset(i); }
    DXBCBlobHeader* const pBlob(uint32_t i) const { return reinterpret_cast<DXBCBlobHeader*>((char*)this + BlobOffset(i)); }
};

struct ShaderResource
{
    ShaderResource(const char* n, RDEFResourceBinding bind) : name(n), binding(bind) { };
    const char* name;
    RDEFResourceBinding binding;
};

struct TmpConstBufVar
{
    TmpConstBufVar(const char* n, D3D_SHADER_VARIABLE_TYPE t, int s) : name(n), type(t), size(s) {};
    const char* name;
    D3D_SHADER_VARIABLE_TYPE type;
    uint32_t size;
};

class CPakAsset; // hate u

std::map<uint32_t, ShaderResource> ResourceBindingFromDXBlob(CPakAsset* const asset, D3D_SHADER_INPUT_TYPE inputType);
std::vector<TmpConstBufVar> ConstBufVarFromDXBlob(CPakAsset* const asset, const char* constBufName);