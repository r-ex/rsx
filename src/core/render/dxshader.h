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

    // enum value '0x8', and '0x9' observed in shader version 15. unsupported
    Unk_8 = 8,
    Unk_9 = 9,
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

    ID3DBlob* m_bytecodeBlob;

    eShaderType m_type;

public:
    inline const std::string& GetName() const { return m_name; };
    inline ID3D11InputLayout* GetInputLayout() const { return m_inputLayout; };
    
    void SetInputLayout(ID3D11InputLayout* layout) { assert(m_type == eShaderType::Vertex);  m_inputLayout = layout; };

    ID3DBlob* GetBytecodeBlob() const { return m_bytecodeBlob; };

    template<typename T>
    inline T* Get() const
    {
#if _DEBUG
        assert(this);

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
    CShader* LoadShader(const std::string& path, eShaderType type, D3D11_INPUT_ELEMENT_DESC* inputLayoutDesc = nullptr, UINT numInputElements = 0u);
    CShader* LoadShaderFromString(const std::string& path, const std::string& sourceString, eShaderType type, D3D11_INPUT_ELEMENT_DESC* inputLayoutDesc = nullptr, UINT numInputElements = 0u);

    void Explode() { m_shaders.clear(); }

private:
    CShader* GetShaderByPath(const std::string& path);

private:
    std::unordered_map<std::string, CShader*> m_shaders;
};

static D3D11_INPUT_ELEMENT_DESC s_DefaultInputLayoutDesc[] = {
    { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0},
    { "NORMAL", 0, DXGI_FORMAT_R32_UINT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0},
    { "COLOR", 0, DXGI_FORMAT_R8G8B8A8_UINT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0},
    { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0},
    { "UNUSED", 0, DXGI_FORMAT_R32_UINT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0},
    { "BLENDWEIGHT", 0, DXGI_FORMAT_R16G16_SINT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0},
    { "BLENDINDICES", 0, DXGI_FORMAT_R8G8B8A8_UINT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0},
};

#if !defined(ADVANCED_MODEL_PREVIEW)

// File: model_ps.hlsl
constexpr static const char s_PreviewPixelShader[] = {
"struct VS_Output\n"
"{\n"
"float4 position : SV_POSITION;\n"
"float3 worldPosition : POSITION;\n"
"float4 color : COLOR;\n"
"float3 normal : NORMAL;\n"
"float2 uv : TEXCOORD;\n"
"};\n"
"Texture2D baseTexture : register(t0);\n"
"SamplerState texSampler : register(s0);\n"
"float4 ps_main(VS_Output input) : SV_Target\n"
"{\n"
"return baseTexture.Sample(texSampler, input.uv);\n"
"}\n"
};

// File: model_vs.hlsl
constexpr static const char s_PreviewVertexShader[] = {
"struct VS_Input\n"
"{\n"
"float3 position : POSITION;\n"
"uint normal : NORMAL;\n"
"uint4 color : COLOR;\n"
"float2 uv : TEXCOORD;\n"
"uint weights : UNUSED;\n"
"int2 weights_2_electric_boogaloo : BLENDWEIGHT;\n"
"uint4 blendIndices : BLENDINDICES;\n"
"};\n"
"struct VS_Output\n"
"{\n"
"float4 position : SV_POSITION;\n"
"float3 worldPosition : POSITION;\n"
"float4 color : COLOR;\n"
"float3 normal : NORMAL;\n"
"float2 uv : TEXCOORD;\n"
"};\n"
"cbuffer VS_TransformConstants : register(b0)\n"
"{\n"
"float4x4 modelMatrix;\n"
"float4x4 viewMatrix;\n"
"float4x4 projectionMatrix;\n"
"};\n"
"struct VertexWeight_t\n"
"{\n"
"float weight;\n"
"int bone;\n"
"};\n"
"StructuredBuffer<VertexWeight_t> g_weights : register(t61);\n"
"StructuredBuffer<float4x4> g_boneMatrix : register(t60);\n"
"VS_Output vs_main(VS_Input input)\n"
"{\n"
"VS_Output output;\n"
"bool sign = (input.normal >> 28) & 1;\n"
"float val1 = sign ? -255.f : 255.f;\n"
"float val2 = ((input.normal >> 19) & 0x1FF) - 256.f;\n"
"float val3 = ((input.normal >> 10) & 0x1FF) - 256.f;\n"
"int idx1 = (input.normal >> 29) & 3;\n"
"int idx2 = (0x124u >> (2 * idx1 + 2)) & 3;\n"
"int idx3 = (0x124u >> (2 * idx1 + 4)) & 3;\n"
"float normalised = rsqrt((255.f * 255.f) + (val2 * val2) + (val3 * val3));\n"
"float3 vals = float3(val1 * normalised, val2 * normalised, val3 * normalised);\n"
"float3 normal;\n"
"if (idx1 == 0)\n"
"{\n"
"normal.x = vals[idx1];\n"
"normal.y = vals[idx2];\n"
"normal.z = vals[idx3];\n"
"}\n"
"else if (idx1 == 1)\n"
"{\n"
"normal.x = vals[idx3];\n"
"normal.y = vals[idx1];\n"
"normal.z = vals[idx2];\n"
"}\n"
"else\n"
"{\n"
"normal.x = vals[idx2];\n"
"normal.y = vals[idx3];\n"
"normal.z = vals[idx1];\n"
"}\n"
#if defined(HAS_BONED_MODELS) // lmao boned
"uint weightCount = input.weights & 0xFF;\n"
"uint weightIndex = (input.weights >> 8) & 0xFFFFFF;\n"
"float3 weightedPos = float3(0.f, 0.f, 0.f);\n"
"float3 weightedNml = float3(0.f, 0.f, 0.f);\n"
"for (uint i = 0; i < weightCount; ++i)\n"
"{\n"
"VertexWeight_t wgt = g_weights[weightIndex + i];\n"
"float4x4 boneMat = g_boneMatrix[wgt.bone];\n"
"weightedPos += wgt.weight * mul(float4(input.position, 1.f), boneMat).xyz;\n"
"weightedNml += wgt.weight * mul(normal, (float3x3)boneMat);\n"
"}\n"
#else
"float3 weightedPos = input.position;\n"
"float3 weightedNml = normal;\n"
#endif
"float4 worldPos = float4(weightedPos.xzy, 1.f);\n"
"output.position = mul(worldPos, modelMatrix);\n"
"output.position = mul(output.position, viewMatrix);\n"
"output.position = mul(output.position, projectionMatrix);\n"
"output.worldPosition = worldPos.xyz;\n"
"output.color = float4(input.color.r / 255.f, input.color.g / 255.f, input.color.b / 255.f, input.color.a / 255.f);\n"
"output.normal = normalize(mul(weightedNml.xzy, (float3x3)modelMatrix));\n"
"output.uv = input.uv;\n"
"return output;\n"
"}\n"
};
#else
// temp amp shaders
constexpr static const char s_PreviewPixelShader[] = {
    "struct VS_Output\n"
    "{\n"
    "    float4 uv : TEXCOORD0;\n"
    "    float3 uv4 : TEXCOORD4;\n"
    "    float3 normal : NORMAL;\n"
    "    float3 tangent : TANGENT;\n"
    "    float3 binorm : BINORMAL;\n"
    "    float4 uv6 : TEXCOORD6;\n"
    "    float4 position : SV_Position0;\n"
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
    "    float2 uv : TEXCOORD0;\n"
    "};\n"
    "struct VS_Output\n"
    "{\n"
    "    float4 v0 : TEXCOORD0;\n"
    "    float3 v1 : TEXCOORD4;\n"
    "    float3 v2 : NORMAL;\n"
    "    float3 v3 : TANGENT;\n"
    "    float3 v4 : BINORMAL;\n"
    "    float4 v5 : TEXCOORD6;\n"
    "    float4 v6 : SV_Position0;\n"
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
    "    output.v1 = float3(0,0,0);\n"
    "    output.v4 = float3(0,0,0);\n"
    "    output.v3 = float3(0,0,0);\n"
    "    output.v5 = float4(0,0,0,0);\n"
    "    float3 pos = float3(-input.position.x, input.position.y, input.position.z);\n"
    "    output.v6 = mul(float4(pos, 1.f), modelMatrix);\n"
    "    output.v6 = mul(output.v6, viewMatrix);\n"
    "    output.v6 = mul(output.v6, projectionMatrix);\n"
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
    "    output.v2 = normalize(mul(tmp, modelMatrix));\n"
    "    output.v0 = float4(input.uv.x, input.uv.y, input.uv.x, input.uv.y);\n"
    "    return output;\n"
    "}"
};
#endif

// File auto-generated by RSX script shaderstr.py
// Comments have been automatically stripped
// File: amp_vs.hlsl
constexpr static const char s_AdvancedPreviewVertexShader[] = {
"struct VS_Input"
"{"
"float3 position : POSITION;"
"uint normal : NORMAL;"
"uint color : COLOR;"
"float2 uv : TEXCOORD;"
"};"
"struct VS_Output"
"{"
"float4 v0 : TEXCOORD0;"
"float3 v1 : TEXCOORD4;"
"float3 v2 : NORMAL;"
"float3 v3 : TANGENT;"
"float3 v4 : BINORMAL;"
"float4 v5 : TEXCOORD6;"
"float4 v6 : SV_Position0;"
"};"
"cbuffer CBufCommonPerCamera : register(b2)"
"{"
"float c_gameTime : packoffset(c0);"
"float3 c_cameraOrigin : packoffset(c0.y);"
"row_major float4x4 c_cameraRelativeToClip : packoffset(c1);"
"int c_frameNum : packoffset(c5);"
"float3 c_cameraOriginPrevFrame : packoffset(c5.y);"
"row_major float4x4 c_cameraRelativeToClipPrevFrame : packoffset(c6);"
"float4 c_clipPlane : packoffset(c10);"
"struct"
"{"
"float4 k0;"
"float4 k1;"
"float4 k2;"
"float4 k3;"
"float4 k4;"
"} c_fogParams : packoffset(c11);"
"float3 c_skyColor : packoffset(c16);"
"float c_shadowBleedFudge : packoffset(c16.w);"
"float c_envMapLightScale : packoffset(c17);"
"float3 c_sunColor : packoffset(c17.y);"
"float3 c_sunDir : packoffset(c18);"
"float c_minShadowVariance : packoffset(c18.w);"
"struct"
"{"
"float3 shadowRelConst;"
"bool enableShadows;"
"float3 shadowRelForX;"
"int cascade3LogicData;"
"float3 shadowRelForY;"
"float cascadeWeightScale;"
"float3 shadowRelForZ;"
"float cascadeWeightBias;"
"float4 normCoordsScale12;"
"float4 normCoordsBias12;"
"float2 normToAtlasCoordsScale0;"
"float2 normToAtlasCoordsBias0;"
"float4 normToAtlasCoordsScale12;"
"float4 normToAtlasCoordsBias12;"
"float4 normCoordsScaleBias3;"
"float4 shadowRelToZ3;"
"float2 normToAtlasCoordsScale3;"
"float2 normToAtlasCoordsBias3;"
"float3 cascade3LightDir;"
"float worldUnitSizeInDepthStatic;"
"float3 cascade3LightX;"
"float cascade3CotFovX;"
"float3 cascade3LightY;"
"float cascade3CotFovY;"
"float2 cascade3ZNearFar;"
"float2 unused_2;"
"float4 normCoordsScaleBiasStatic;"
"float4 shadowRelToZStatic;"
"} c_csm : packoffset(c19);"
"uint c_lightTilesX : packoffset(c37);"
"float c_maxLightingValue : packoffset(c37.y);"
"uint c_debugShaderFlags : packoffset(c37.z);"
"uint c_branchFlags : packoffset(c37.w);"
"float2 c_renderTargetSize : packoffset(c38);"
"float2 c_rcpRenderTargetSize : packoffset(c38.z);"
"float2 c_cloudRelConst : packoffset(c39);"
"float2 c_cloudRelForX : packoffset(c39.z);"
"float2 c_cloudRelForY : packoffset(c40);"
"float2 c_cloudRelForZ : packoffset(c40.z);"
"float c_sunHighlightSize : packoffset(c41);"
"float c_forceExposure : packoffset(c41.y);"
"float c_zNear : packoffset(c41.z);"
"float c_gameTimeLastFrame : packoffset(c41.w);"
"float2 c_viewportMinMaxZ : packoffset(c42);"
"float2 c_viewportBias : packoffset(c42.z);"
"float2 c_viewportScale : packoffset(c43);"
"float2 c_rcpViewportScale : packoffset(c43.z);"
"float2 c_framebufferViewportScale : packoffset(c44);"
"float2 c_rcpFramebufferViewportScale : packoffset(c44.z);"
"int c_separateIndirectDiffuse : packoffset(c45);"
"float c_cameraLenY : packoffset(c45.y);"
"float c_cameraRcpLenY : packoffset(c45.z);"
"int c_debugInt : packoffset(c45.w);"
"float c_debugFloat : packoffset(c46);"
"float3 unused : packoffset(c46.y);"
"}"
"cbuffer CBufModelInstance : register(b3)"
"{"
"struct"
"{"
"row_major float3x4 objectToCameraRelative;"
"row_major float3x4 objectToCameraRelativePrevFrame;"
"float4 diffuseModulation;"
"int cubemapID;"
"int lightmapID;"
"float2 unused;"
"struct"
"{"
"float4 ambientSH[3];"
"float4 skyDirSunVis;"
"uint4 packedLightData;"
"} lighting;"
"} c_modelInst : packoffset(c0);"
"}"
"cbuffer VS_TransformConstants : register(b0)"
"{"
"float4x4 modelMatrix;"
"float4x4 viewMatrix;"
"float4x4 projectionMatrix;"
"};"
"VS_Output vs_main(VS_Input input)"
"{"
"VS_Output output;"
"float3 posToCamRelPrevFrame = mul(c_modelInst.objectToCameraRelativePrevFrame, float4(input.position, 1.0));"
"output.v5 = mul(c_cameraRelativeToClipPrevFrame, float4(posToCamRelPrevFrame, 1.0));"
"float3 pos = float3(-input.position.x, input.position.y, input.position.z);"
"output.v6 = mul(float4(pos, 1.f), modelMatrix);"
"output.v6 = mul(output.v6, viewMatrix);"
"output.v6 = mul(output.v6, projectionMatrix);"
"output.v0.xy = input.uv;"
"output.v0.zw = 0.0;"
"output.v1 = output.v6;"
"output.v3 = float3(0,0,0);"
"output.v4 = float3(0,0,0);"
"bool sign = (input.normal >> 28) & 1;"
"float val1 = sign ? -255.f : 255.f;"
"float val2 = ((input.normal >> 19) & 0x1FF) - 256.f;"
"float val3 = ((input.normal >> 10) & 0x1FF) - 256.f;"
"int idx1 = (input.normal >> 29) & 3;"
"int idx2 = (0x124u >> (2 * idx1 + 2)) & 3;"
"int idx3 = (0x124u >> (2 * idx1 + 4)) & 3;"
"float normalised = rsqrt((255.f * 255.f) + (val2 * val2) + (val3 * val3));"
"float3 vals = float3(val1 * normalised, val2 * normalised, val3 * normalised);"
"float3 tmp;"
"if (idx1 == 0)"
"{"
"tmp.x = vals[idx1];"
"tmp.y = vals[idx2];"
"tmp.z = vals[idx3];"
"}"
"else if (idx1 == 1)"
"{"
"tmp.x = vals[idx3];"
"tmp.y = vals[idx1];"
"tmp.z = vals[idx2];"
"}"
"else if (idx1 == 2)"
"{"
"tmp.x = vals[idx2];"
"tmp.y = vals[idx3];"
"tmp.z = vals[idx1];"
"}"
"output.v2 = normalize(mul((float3x3)c_modelInst.objectToCameraRelative, tmp));"
"return output;"
"}"
};

// File: bsp_ps.hlsl
constexpr static const char s_BSPPixelShader[] = {
"struct VS_Output\n"
"{\n"
"float4 position : SV_POSITION;\n"
"float3 worldPosition : POSITION;\n"
"float4 color : COLOR;\n"
"float3 normal : NORMAL;\n"
"float2 uv : TEXCOORD;\n"
"};\n"
"Texture2D baseTexture : register(t0);\n"
"SamplerState texSampler : register(s0);\n"
"float4 ps_main(VS_Output input) : SV_Target\n"
"{\n"
"float4 col = baseTexture.Sample(texSampler, input.uv);\n"
"const float3 lightDir = normalize(float3(0.4f, 1.f, 0.3f));\n"
"const float Id = 0.35f + (0.65f * saturate(dot(input.normal, lightDir)));\n"
"return float4(Id * col.rgb * float3(1.f, 1.f, 1.f), 1.f);\n"
"}\n"
};

// File: vertexLitBump_vs.hlsl
constexpr static const char s_VertexLitBumpShader[] = {
"struct VS_Input\n"
"{\n"
"float3 position : POSITION;\n"
"float3 normal : NORMAL;\n"
"float2 uv : TEXCOORD0;\n"
"uint color : COLOR;\n"
"float2 lmapUV : TEXCOORD1;\n"
"uint unk : UNK;\n"
"};\n"
"struct VS_Output\n"
"{\n"
"float4 position : SV_POSITION;\n"
"float3 worldPosition : POSITION;\n"
"float4 color : COLOR;\n"
"float3 normal : NORMAL;\n"
"float2 uv : TEXCOORD;\n"
"};\n"
"cbuffer VS_TransformConstants : register(b0)\n"
"{\n"
"float4x4 modelMatrix;\n"
"float4x4 viewMatrix;\n"
"float4x4 projectionMatrix;\n"
"};\n"
"StructuredBuffer<float3> vertexBuffer : register(t0);\n"
"StructuredBuffer<float3> normalBuffer : register(t1);\n"
"VS_Output vs_main(VS_Input input)\n"
"{\n"
"VS_Output output;\n"
"float3 pos = input.position.xzy;\n"
"output.position = mul(float4(pos, 1.f), modelMatrix);\n"
"output.position = mul(output.position, viewMatrix);\n"
"output.position = mul(output.position, projectionMatrix);\n"
"output.worldPosition = mul(float4(pos, 1.f), modelMatrix).xyz;\n"
"output.color = float4((input.color & 0xff) / 255.f, ((input.color >> 8) & 0xff) / 255.f, ((input.color >> 16) & 0xff) / 255.f, ((input.color >> 24) & 0xff) / 255.f);\n"
"output.normal = normalize(mul(input.normal.xzy, (float3x3)modelMatrix));\n"
"output.uv = input.uv;\n"
"return output;\n"
"}\n"
};

// File: vertexLitFlat_vs.hlsl
constexpr static const char s_VertexLitFlatShader[] = {
"struct VS_Input\n"
"{\n"
"float3 position : POSITION;\n"
"float3 normal : NORMAL;\n"
"float2 uv : TEXCOORD;\n"
"uint unk : UNK;\n"
"};\n"
"struct VS_Output\n"
"{\n"
"float4 position : SV_POSITION;\n"
"float3 worldPosition : POSITION;\n"
"float4 color : COLOR;\n"
"float3 normal : NORMAL;\n"
"float2 uv : TEXCOORD;\n"
"};\n"
"cbuffer VS_TransformConstants : register(b0)\n"
"{\n"
"float4x4 modelMatrix;\n"
"float4x4 viewMatrix;\n"
"float4x4 projectionMatrix;\n"
"};\n"
"StructuredBuffer<float3> vertexBuffer : register(t0);\n"
"StructuredBuffer<float3> normalBuffer : register(t1);\n"
"VS_Output vs_main(VS_Input input)\n"
"{\n"
"VS_Output output;\n"
"float3 pos = input.position.xzy;\n"
"output.position = mul(float4(pos, 1.f), modelMatrix);\n"
"output.position = mul(output.position, viewMatrix);\n"
"output.position = mul(output.position, projectionMatrix);\n"
"output.worldPosition = mul(float4(pos, 1.f), modelMatrix).xyz;\n"
"output.color = float4(0.5f, 0.5f, 1.f, 1.f);\n"
"output.normal = normalize(mul(input.normal.xzy, (float3x3)modelMatrix));\n"
"output.uv = input.uv;\n"
"return output;\n"
"}\n"
};

// File: vertexUnlitTS_vs.hlsl
constexpr static const char s_VertexUnlitTSShader[] = {
"struct VS_Input\n"
"{\n"
"float3 position : POSITION;\n"
"float3 normal : NORMAL;\n"
"float2 uv : TEXCOORD;\n"
"uint2 unk : UNK;\n"
"};\n"
"struct VS_Output\n"
"{\n"
"float4 position : SV_POSITION;\n"
"float3 worldPosition : POSITION;\n"
"float4 color : COLOR;\n"
"float3 normal : NORMAL;\n"
"float2 uv : TEXCOORD;\n"
"};\n"
"cbuffer VS_TransformConstants : register(b0)\n"
"{\n"
"float4x4 modelMatrix;\n"
"float4x4 viewMatrix;\n"
"float4x4 projectionMatrix;\n"
"};\n"
"StructuredBuffer<float3> vertexBuffer : register(t0);\n"
"StructuredBuffer<float3> normalBuffer : register(t1);\n"
"VS_Output vs_main(VS_Input input)\n"
"{\n"
"VS_Output output;\n"
"float3 pos = input.position.xzy;\n"
"output.position = mul(float4(pos, 1.f), modelMatrix);\n"
"output.position = mul(output.position, viewMatrix);\n"
"output.position = mul(output.position, projectionMatrix);\n"
"output.worldPosition = mul(float4(pos, 1.f), modelMatrix).xyz;\n"
"output.color = float4(0.5f, 0.5f, 0.5f, 1.f);\n"
"output.normal = normalize(mul(input.normal.xzy, (float3x3)modelMatrix));\n"
"output.uv = input.uv;\n"
"return output;\n"
"}\n"
};
