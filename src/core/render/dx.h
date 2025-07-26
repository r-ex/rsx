#pragma once
#include <d3d11.h>
#include <core/render/dxshader.h>
#include <DirectXMath.h>

#include <core/math/vector.h>
#include <core/math/matrix.h>
#include <core/render/dxutils.h>
#include <core/render/dxscene.h>

#include <core/render/uistate.h>

using namespace DirectX;

class CTexture
{
public:
    CTexture() = default;
    CTexture(const char* const buf, const size_t bufSize, const size_t width, const size_t height, const DXGI_FORMAT imgFormat, const size_t arraySize, const size_t mipLevels);
    CTexture(ID3D11Device* const device, const char* const buf, const size_t bufSize, const size_t width, const size_t height, const DXGI_FORMAT imgFormat, const size_t arraySize, const size_t mipLevels);
    ~CTexture();

    bool CreateShaderResourceView(ID3D11Device* const device);
    bool ExportAsPng(const std::filesystem::path& exportPath);
    bool ExportAsDds(const std::filesystem::path& exportPath);
    bool ConvertToFormat(const DXGI_FORMAT format);
    void CopySourceTextureSlice(CTexture* const src, const size_t x, const size_t y, const size_t w, const size_t h, const size_t offsetX, const size_t offsetY);
    void CopyRawToTexture(const char* const buf, const size_t bufSize);
    
    inline ID3D11ShaderResourceView* const GetSRV() const { return m_shaderResourceView; };
    inline void* const GetTexture() const { return m_texture; };
    inline const int GetWidth() const { return static_cast<int>(m_width); };
    inline const int GetHeight() const { return static_cast<int>(m_height); };
    const size_t GetSlicePitch() const;

    uint8_t* GetPixels();
    const uint8_t* GetPixels() const;

    static const size_t GetBpp(const DXGI_FORMAT format); // gets bits per bixel
    static const size_t GetPixelBlock(const DXGI_FORMAT format); // gets the number of pixels in a block
    static const int Morton(uint32_t i, uint32_t sx, uint32_t sy);

    void ConvertNormalOpenDX(); // adds blue channel
    void ConvertNormalOpenGL(); // adds blue channel and inverts green

private:
    bool IsValid32bppFormat();
    void InitTexture(const char* const buf, const size_t bufSize, const size_t width, const size_t height, const DXGI_FORMAT imgFormat, const size_t arraySize, const size_t mipLevels);

    size_t m_width;
    size_t m_height;

    void* m_texture;
    ID3D11ShaderResourceView* m_shaderResourceView;
};

struct DXDrawDataTexture_t
{
    uint32_t resourceBindPoint;
    std::shared_ptr<CTexture> texture;
};

struct DXMeshDrawData_t
{
    ID3D11Buffer* vertexBuffer;
    ID3D11Buffer* indexBuffer;

    ID3D11Buffer* uberStaticBuf;

    ID3D11VertexShader* vertexShader;
    ID3D11PixelShader* pixelShader;

    ID3D11InputLayout* inputLayout;

    //std::shared_ptr<CTexture> albedoTexture;
    std::vector<DXDrawDataTexture_t> textures;

    size_t numIndices;

    uint32_t vertexStride;

    DXGI_FORMAT indexFormat;

    Vector modelMins;
    Vector modelMaxs;

    bool visible;
    bool doFrustumCulling;
};

struct VS_TransformConstants
{
    XMMATRIX modelMatrix;
    XMMATRIX viewMatrix;
    XMMATRIX projectionMatrix;
};

struct CBufCommonPerCamera
{
    float c_gameTime;
    float3 c_cameraOrigin;
    XMMATRIX c_cameraRelativeToClip;
    int c_frameNum;
    float3 c_cameraOriginPrevFrame;
    XMMATRIX c_cameraRelativeToClipPrevFrame;
    float4 c_clipPlane;

    struct FogUnion
    {
        float4 k0;
        float4 k1;
        float4 k2;
        float4 k3;
        float4 k4;
    } c_fogParams;

    float3 c_skyColor;
    float c_shadowBleedFudge;
    float c_envMapLightScale;
    float3 c_sunColor;
    float3 c_sunDir;
    float c_minShadowVariance;

    struct CSMConstants_t
    {
        float3 shadowRelConst;
        bool enableShadows;
        float3 shadowRelForX;
        int cascade3LogicData;
        float3 shadowRelForY;
        float cascadeWeightScale;
        float3 shadowRelForZ;
        float cascadeWeightBias;
        float4 normCoordsScale12;
        float4 normCoordsBias12;
        float2 normToAtlasCoordsScale0;
        float2 normToAtlasCoordsBias0;
        float4 normToAtlasCoordsScale12;
        float4 normToAtlasCoordsBias12;
        float4 normCoordsScaleBias3;
        float4 shadowRelToZ3;
        float2 normToAtlasCoordsScale3;
        float2 normToAtlasCoordsBias3;
        float3 cascade3LightDir;
        float worldUnitSizeInDepthStatic;
        float3 cascade3LightX;
        float cascade3CotFovX;
        float3 cascade3LightY;
        float cascade3CotFovY;
        float2 cascade3ZNearFar;
        float2 unused_2;
        float4 normCoordsScaleBiasStatic;
        float4 shadowRelToZStatic;
    } c_csm;

    uint32_t c_lightTilesX;
    float c_maxLightingValue;
    uint32_t c_debugShaderFlags;
    uint32_t c_branchFlags;
    float2 c_renderTargetSize;
    float2 c_rcpRenderTargetSize;
    float2 c_cloudRelConst;
    float2 c_cloudRelForX;
    float2 c_cloudRelForY;
    float2 c_cloudRelForZ;
    float c_sunHighlightSize;
    float c_forceExposure;
    float c_zNear;
    float c_gameTimeLastFrame;
    float2 c_viewportMinMaxZ;
    float2 c_viewportBias;
    float2 c_viewportScale;
    float2 c_rcpViewportScale;
    float2 c_framebufferViewportScale;
    float2 c_rcpFramebufferViewportScale;
    int c_separateIndirectDiffuse;
    float c_cameraLenY;
    float c_cameraRcpLenY;
    int c_debugInt;
    float c_debugFloat;
    float3 unused;
};
static_assert(sizeof(CBufCommonPerCamera) == 752);

struct CBufModelInstance
{
    struct ModelInstance
    {
        XMFLOAT3X4 objectToCameraRelative;
        XMFLOAT3X4 objectToCameraRelativePrevFrame;
        XMFLOAT4 diffuseModulation;
        int cubemapID;
        int lightmapID;
        XMFLOAT2 unused;

        struct ModelInstanceLighting
        {
            XMFLOAT4 ambientSH[3];
            XMFLOAT4 skyDirSunVis;
            XMUINT4 packedLightData;
        } lighting;

    } c_modelInst;
};
static_assert(sizeof(CBufModelInstance) == 208);

struct DXShaderResourceDesc_t
{
    UINT bindPoint;
    ID3D11ShaderResourceView* resourceView;
};

class CDXDrawData
{
public:
    CDXDrawData() = default;

    ~CDXDrawData()
    {
        DX_RELEASE_PTR(transformsBuffer);
        DX_RELEASE_PTR(inputLayout);
        for (auto& meshBuffer : meshBuffers)
        {
            DX_RELEASE_PTR(meshBuffer.vertexBuffer);
            DX_RELEASE_PTR(meshBuffer.indexBuffer);
        }
    }

    std::vector<DXMeshDrawData_t> meshBuffers;

    ID3D11Buffer* transformsBuffer;

    ID3D11Buffer* boneMatrixBuffer;
    ID3D11ShaderResourceView* boneMatrixSRV;

    ID3D11InputLayout* inputLayout;

    CShader* pixelShader;
    CShader* vertexShader;

    std::vector<DXShaderResourceDesc_t> pixelShaderResources;
    std::vector<DXShaderResourceDesc_t> vertexShaderResources;

    char* modelName;

    Vector position;
};

class CDXCamera
{
public:
    CDXCamera() = default;

    void Move(float dt);

    void AddRotation(float yaw, float pitch, float roll);

    XMMATRIX GetViewMatrix();

    void CommitCameraDataBufferUpdates();

private:
    void UpdateCommonCameraData();

public:

    XMVECTOR target;
    XMVECTOR up;

    Vector position;
    Vector rotation; // pitch yaw roll

    ID3D11Buffer* bufCommonPerCamera;

    CBufCommonPerCamera commonCameraData;
};

class CDXParentHandler
{
public:
	CDXParentHandler() = default;
	CDXParentHandler(const HWND windowHandle) : m_windowHandle(windowHandle), m_pDevice(nullptr), m_pDeviceContext(nullptr), m_pSwapChain(nullptr), m_pMainView(nullptr), m_pRasterizerState(nullptr), m_pShaderManager(nullptr), m_pCamera(nullptr) {};
	~CDXParentHandler() { CleanupD3D(); };

	bool SetupDeviceD3D();
	void CleanupD3D();
	void HandleResize(const uint16_t x, const uint16_t y);

    std::shared_ptr<CTexture> CreateRenderTexture(const char* const buf, const size_t bufSize, const int width, const int height, const DXGI_FORMAT imgFormat, const size_t arraySize, const size_t mipLevels);

    inline HWND GetWindowHandle() const { return m_windowHandle; };
    inline bool GetWindowSize(LONG* _Out_ width, LONG* _Out_ height)
    {
        RECT windowRect{};

        bool ret = GetWindowRect(m_windowHandle, &windowRect);
        if(width)
            *width = windowRect.right - windowRect.left;
        if (height)
            *height = windowRect.bottom - windowRect.top;
        return ret;
    }

	inline ID3D11Device* GetDevice() const { return m_pDevice; };
	inline ID3D11DeviceContext* GetDeviceContext() const { return m_pDeviceContext; };
	inline IDXGISwapChain* GetSwapChain() const { return m_pSwapChain; };
	inline ID3D11RenderTargetView* GetMainView() const { return m_pMainView; };
    inline ID3D11DepthStencilView* GetDepthStencilView() const { return m_pDepthStencilView; };
    inline ID3D11DepthStencilState* GetDepthStencilState() { return m_pDepthStencilState; };
    inline ID3D11RasterizerState* GetRasterizerState() const { return m_pRasterizerState; };
    inline ID3D11SamplerState* GetSamplerState() const { return m_pSamplerState; };
    inline ID3D11SamplerState* GetSamplerComparisonState() const { return m_pSamplerCmpState; };

    inline const XMMATRIX& GetProjMatrix() { return m_wndProjMatrix; };

    inline CDXShaderManager* GetShaderManager() const { return m_pShaderManager; };

    inline CDXCamera* GetCamera() const { return m_pCamera; };

    inline CDXScene& GetScene() { return m_Scene; };

    inline CUIState& GetUIState() { return m_UIState; };

private:
    bool CreateDepthBuffer(ID3D11Texture2D* frameBuffer);
	bool CreateMainView(const uint16_t w, const uint16_t h);

    bool CreateMisc();

	HWND m_windowHandle;

	ID3D11Device* m_pDevice;
	ID3D11DeviceContext* m_pDeviceContext;
	IDXGISwapChain* m_pSwapChain;
	ID3D11RenderTargetView* m_pMainView;
    ID3D11DepthStencilView* m_pDepthStencilView;
    ID3D11DepthStencilState* m_pDepthStencilState; // depth enabled
    ID3D11RasterizerState* m_pRasterizerState; // main rasterizer state
    ID3D11SamplerState* m_pSamplerState;
    ID3D11SamplerState* m_pSamplerCmpState; // sampler comparison state because obviously we need this?

    XMMATRIX m_wndProjMatrix;

    CDXShaderManager* m_pShaderManager;

    CDXCamera* m_pCamera;

    CDXScene m_Scene;

    CUIState m_UIState;
};

