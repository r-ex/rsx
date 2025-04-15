#pragma once
#include <d3d11.h>

enum class eShaderType : uint8_t
{
    Pixel,
    Vertex,
    Geometry,
    Hull,
    Domain,
    Compute,
    Invalid,
    // enum value '0x8', and '0x9' observed in shader version 15.
};

static const char* s_dxShaderTypeNames[] = {
    "PIXEL",
    "VERTEX",
    "GEOMETRY",
    "HULL",
    "DOMAIN",
    "COMPUTE",
};

static const char* s_dxShaderTypeShortNames[] = {
    "ps",
    "vs",
    "gs",
    "hs",
    "ds",
    "cs",
};

inline const char* GetShaderTypeName(eShaderType type)
{ 
    if ((__int8)type >= ARRAYSIZE(s_dxShaderTypeNames))
    {
        return "UNKNOWN";
    }
    assert((__int8)type >= 0 && (__int8)type < ARRAYSIZE(s_dxShaderTypeNames));
    return s_dxShaderTypeNames[(__int8)type];
};

inline const char* GetShaderTypeShortName(eShaderType type)
{
    if ((__int8)type >= ARRAYSIZE(s_dxShaderTypeNames))
    {
        return "_unk";
    }
    assert((__int8)type >= 0 && (__int8)type < ARRAYSIZE(s_dxShaderTypeShortNames));
    return s_dxShaderTypeShortNames[(__int8)type];
};

#define TYPEID_EQ(A, B) typeid(A) == typeid(B)
class CShader
{
    friend class CDXShaderManager; // let shader manager access protected vars

protected:
    CShader() : m_dxShader(nullptr), m_inputLayout(nullptr) {};

    std::string m_name;

    // pointer to some d3d11 shader class instance
    // must be a generic pointer so that it can be used for any type of shader without
    // requiring multiple variables
    void* m_dxShader;
    ID3D11InputLayout* m_inputLayout;

    eShaderType m_type;

public:
    inline const std::string& GetName() const { return m_name; };
    inline ID3D11InputLayout* GetInputLayout() const { return m_inputLayout; };

    template<typename T>
    inline T* Get() const
    {

#if _DEBUG 
        // these checks only really need to be on debug
        // since anything going wrong should be caught in dev
        if (TYPEID_EQ(T, ID3D11PixelShader))
            assert(m_type == eShaderType::Pixel);

        if (TYPEID_EQ(T, ID3D11VertexShader))
            assert(m_type == eShaderType::Vertex);

        if (TYPEID_EQ(T, ID3D11GeometryShader))
            assert(m_type == eShaderType::Geometry);

        if (TYPEID_EQ(T, ID3D11HullShader))
            assert(m_type == eShaderType::Hull);

        if (TYPEID_EQ(T, ID3D11DomainShader))
            assert(m_type == eShaderType::Domain);

        if (TYPEID_EQ(T, ID3D11ComputeShader))
            assert(m_type == eShaderType::Compute);
#endif
        return reinterpret_cast<T*>(m_dxShader);
    };
};
#undef TYPEID_EQ

class CDXShaderManager
{
public:
    CShader* LoadShader(const std::string& path, eShaderType type);
    CShader* LoadShaderFromString(const std::string& path, const std::string& sourceString, eShaderType type);

private:
    CShader* GetShaderByPath(const std::string& path);

private:
    std::unordered_map<std::string, CShader*> m_shaders;
};

constexpr static const char s_PreviewPixelShader[] = {
    "struct VS_Output\n"
    "{\n"
    "    float4 position : SV_POSITION;\n"
    "    float3 worldPosition : POSITION;\n"
    "    float4 color : COLOR;\n"
    "    float3 normal : NORMAL;\n"
    "    float2 uv : TEXCOORD;\n"
    "};\n"
    "Texture2D baseTexture : register(t0);\n"
    "SamplerState texSampler : register(s0);\n"
    "float4 ps_main(VS_Output input) : SV_Target\n"
    "{\n"
    "    return baseTexture.Sample(texSampler, input.uv);\n"
    "}"
};

constexpr static const char s_PreviewVertexShader[] = {
    "struct VS_Input\n"
    "{\n"
    "    float3 position : POSITION;\n"
    "    uint normal : NORMAL;\n"
    "    uint color : COLOR;\n"
    "    float2 uv : TEXCOORD;\n"
    "};\n"
    "struct VS_Output\n"
    "{\n"
    "    float4 position : SV_POSITION;\n"
    "    float3 worldPosition : POSITION;\n"
    "    float4 color : COLOR;\n"
    "    float3 normal : NORMAL;\n"
    "    float2 uv : TEXCOORD;\n"
    "};\n"
    "cbuffer VS_TransformConstants : register(b0)\n"
    "{\n"
    "    float4x4 modelMatrix;\n"
    "    float4x4 viewMatrix;\n"
    "    float4x4 projectionMatrix;\n"
    "};\n"
    "VS_Output vs_main(VS_Input input)\n"
    "{\n"
    "    VS_Output output;\n"
    "    float3 pos = float3(-input.position.x, input.position.y, input.position.z);\n"
    "    output.position = mul(float4(pos, 1.f), modelMatrix);\n"
    "    output.position = mul(output.position, viewMatrix);\n"
    "    output.position = mul(output.position, projectionMatrix);\n"
    "    output.worldPosition = mul(float4(input.position, 1.f), modelMatrix);\n"
    "    output.color = float4((input.color & 0xff) / 255.f, ((input.color << 8) & 0xff) / 255.f, ((input.color << 16) & 0xff) / 255.f, ((input.color << 24) & 0xff) / 255.f);\n"
    "    bool sign = (input.normal >> 28) & 1;\n"
    "    float val1 = sign ? -255.f : 255.f;\n"
    "    float val2 = ((input.normal >> 19) & 0x1FF) - 256.f;\n"
    "    float val3 = ((input.normal >> 10) & 0x1FF) - 256.f;\n"
    "    int idx1 = (input.normal >> 29) & 3;\n"
    "    int idx2 = (0x124u >> (2 * idx1 + 2)) & 3;\n"
    "    int idx3 = (0x124u >> (2 * idx1 + 4)) & 3;\n"
    "    float normalised = rsqrt((255.f * 255.f) + (val2 * val2) + (val3 * val3));\n"
    "    float3 vals = float3(val1 * normalised, val2 * normalised, val3 * normalised);\n"
    "    float3 tmp;\n"
    "    if (idx1 == 0)\n"
    "    {\n"
    "        tmp.x = vals[idx1];\n"
    "        tmp.y = vals[idx2];\n"
    "        tmp.z = vals[idx3];\n"
    "    }\n"
    "    else if (idx1 == 1)\n"
    "    {\n"
    "        tmp.x = vals[idx3];\n"
    "        tmp.y = vals[idx1];\n"
    "        tmp.z = vals[idx2];\n"
    "    }\n"
    "    else if (idx1 == 2)\n"
    "    {\n"
    "        tmp.x = vals[idx2];\n"
    "        tmp.y = vals[idx3];\n"
    "        tmp.z = vals[idx1];\n"
    "    }\n"
    "    output.normal = normalize(mul(tmp, modelMatrix));\n"
    "    output.uv = input.uv;\n"
    "    return output;\n"
    "}"
};