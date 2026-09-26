#pragma once
#include <d3d11.h>
#include <core/render/dxshader.h>
#include <DirectXMath.h>
#include <dxgi.h>

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
    CTexture(void* scratchImg, ID3D11Device* const device);
    ~CTexture();

    bool CreateShaderResourceView(ID3D11Device* const device);
    bool ExportAsPng(const std::filesystem::path& exportPath);
    bool ExportAsDds(const std::filesystem::path& exportPath);
    bool ConvertToFormat(const DXGI_FORMAT format);
    void CopySourceTextureSlice(CTexture* const src, const size_t x, const size_t y, const size_t w, const size_t h, const size_t offsetX, const size_t offsetY);
    void CopyRawToTexture(const char* const buf, const size_t bufSize);
    
    //inline ID3D11ShaderResourceView* const GetSRV() const { return m_shaderResourceView; };
    ID3D11ShaderResourceView* const GetSRV();
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
    const ID3D11Device* m_currentDevice;
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

    ID3D11Buffer* weightsBuffer;
    ID3D11ShaderResourceView* weightsSRV;

    ID3D11Buffer* uberStaticBuf;
    ID3D11Buffer* uberDynamicBuf;

    ID3D11VertexShader* vertexShader;
    ID3D11PixelShader* pixelShader;

    ID3D11InputLayout* inputLayout;

    std::vector<DXDrawDataTexture_t> textures;

    size_t numIndices;

    uint32_t vertexStride;

    DXGI_FORMAT indexFormat;

    Vector modelMins;
    Vector modelMaxs;

    bool visible : 1;
    bool doFrustumCulling : 1;
    bool hasGameShaders : 1;
    bool wireframe : 1;
};

// Mesh draw data for ""debug draw"" primitives
struct DXMeshDrawData_DebugPrim_t
{
    DXMeshDrawData_DebugPrim_t(ID3D11Buffer* vertexBuf, ID3D11Buffer* indexBuf, DXGI_FORMAT indexFmt, D3D_PRIMITIVE_TOPOLOGY topo, UINT numIndices,UINT numVertices, float lifetime, bool isVisible, bool isWireframe, bool isIndexed, bool noDepthTest) :
        vertexBuffer(vertexBuf), indexBuffer(indexBuf),
        indexFormat(indexFmt), primTopology(topo),
        numIndices(numIndices), numVertices(numVertices),
        lifeRemaining(lifetime), visible(isVisible),
        wireframe(isWireframe), indexed(isIndexed),
        noDepthTest(noDepthTest)
    {};

    DXMeshDrawData_DebugPrim_t() = default;

    ID3D11Buffer* vertexBuffer;
    ID3D11Buffer* indexBuffer;

    DXGI_FORMAT indexFormat;
    D3D_PRIMITIVE_TOPOLOGY primTopology;

    UINT numIndices;
    UINT numVertices;
    float lifeRemaining;

    bool visible : 1;
    bool wireframe : 1;
    bool indexed : 1;
    bool noDepthTest : 1;
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
    XMFLOAT4X4 c_cameraRelativeToClip;
    int c_frameNum;
    float3 c_cameraOriginPrevFrame;
    XMFLOAT4X4 c_cameraRelativeToClipPrevFrame;
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

struct DXBone_t
{
    const char* name;
    Vector pos;
    Quaternion quat;
    Vector scale;

    int parent;
};

class CDXDrawData
{
public:

    enum DrawDataType_e
    {
        MODEL,
        TEXTURE,
    };

    CDXDrawData() : bones(),
        transformsBuffer(nullptr), modelInstanceBuffer(nullptr),
        boneMatrixBuffer(nullptr), boneMatrixSRV(nullptr),
        inputLayout(nullptr), pixelShader(nullptr), vertexShader(nullptr),
        modelName(""), position(0.f), dataType(DrawDataType_e::MODEL)
    {};

    ~CDXDrawData()
    {
        DX_RELEASE_PTR(transformsBuffer);
        DX_RELEASE_PTR(modelInstanceBuffer);
        DX_RELEASE_PTR(inputLayout);
        DX_RELEASE_PTR(boneMatrixBuffer);
        for (auto& meshBuffer : meshBuffers)
        {
            DX_RELEASE_PTR(meshBuffer.vertexBuffer);
            DX_RELEASE_PTR(meshBuffer.indexBuffer);
            DX_RELEASE_PTR(meshBuffer.weightsBuffer);
        }
    }

    std::vector<DXMeshDrawData_t> meshBuffers;
    std::vector<DXMeshDrawData_DebugPrim_t> debugPrims;

    std::vector<DXBone_t> bones;
    std::vector<XMMATRIX> boneInverseBindMatrices;


    ID3D11Buffer* transformsBuffer;
    ID3D11Buffer* modelInstanceBuffer;

    ID3D11Buffer* boneMatrixBuffer;
    ID3D11ShaderResourceView* boneMatrixSRV;

    std::shared_ptr<CTexture> previewTexture;

    ID3D11InputLayout* inputLayout;

    CShader* pixelShader;
    CShader* vertexShader;

    std::unordered_map<uint8_t, ID3D11ShaderResourceView*> pixelShaderResources;
    std::unordered_map<uint8_t, ID3D11ShaderResourceView*> vertexShaderResources;

    std::string modelName;

    Vector position;

    DrawDataType_e dataType;

    void SetPSResource(uint8_t bindPoint, ID3D11ShaderResourceView* srv)
    {
        pixelShaderResources[bindPoint] = srv;
    };

    void SetVSResource(uint8_t bindPoint, ID3D11ShaderResourceView* srv)
    {
        vertexShaderResources[bindPoint] = srv;
    };

    void DrawLine(const Vector& start, const Vector& end, uint32_t col, bool noDepthTest = false, float width = 1.f, float duration = 0.f);
};

#define CAMERA_DEFAULT_DISTANCE 25.f

class CDXCamera
{
public:
    CDXCamera() : distanceToPivot(CAMERA_DEFAULT_DISTANCE) {};

    void Move(float dt);

    void AddRotation(float yaw, float pitch, float roll);

    XMMATRIX GetViewMatrix();
    //float* GetViewMatrixFloat();

    void CommitCameraDataBufferUpdates();

private:
    void UpdateCommonCameraData();

public:

    XMVECTOR target;
    XMVECTOR up;

    Vector position;
    Vector rotation; // pitch yaw roll

    float distanceToPivot;

    ID3D11Buffer* bufCommonPerCamera;

    CBufCommonPerCamera commonCameraData;

    XMMATRIX viewMatrix;
};

class CDXParentHandler
{
public:
	CDXParentHandler() = default;
	CDXParentHandler(const HWND windowHandle) : m_windowHandle(windowHandle), m_pDevice(nullptr), m_pDeviceContext(nullptr), m_pSwapChain(nullptr), m_pMainView(nullptr), m_pRasterizerState(nullptr), m_pShaderManager(nullptr), m_pCamera(nullptr), m_CubemapTex(nullptr), m_CubemapTexSRV(nullptr)
    {
        m_UIState.ClearAssetData();
    };

	~CDXParentHandler() { CleanupD3D(); };

	bool SetupDeviceD3D();
    void UpdateProjectionMatrix();
	void CleanupD3D();
    void CleanupForPreviewResize()
    {
        DX_RELEASE_PTR(m_previewState.frameBuffer);
        DX_RELEASE_PTR(m_previewState.frameBufferSRV);
        DX_RELEASE_PTR(m_previewState.previewRTV);
        DX_RELEASE_PTR(m_previewState.previewDSV);
        DX_RELEASE_PTR(m_previewState.previewDepthBuffer);
    };

	void HandleResize(const uint16_t x, const uint16_t y);
	bool HandleWindowChange(HWND hWnd);

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
    inline ID3D11DepthStencilState* GetDepthStencilState(bool depthTest=true) { return depthTest ? m_pDepthStencilState : m_pDepthStencilStateNoDepthTest; };
    inline ID3D11RasterizerState* GetRasterizerState() const { return m_pRasterizerState; };
    inline ID3D11RasterizerState* GetRasterizerStateWireFrame() const { return m_pRasterizerStateWF; };
    inline ID3D11SamplerState* GetSamplerState() const { return m_pSamplerState; };
    inline ID3D11SamplerState* GetSamplerComparisonState() const { return m_pSamplerCmpState; };

    inline ID3D11Texture2D* GetPreviewFrameBuffer() const { return m_previewState.frameBuffer; };
    inline ID3D11ShaderResourceView* GetPreviewFrameBufferSRV() const { return m_previewState.frameBufferSRV; };
    inline ID3D11RenderTargetView* GetPreviewRTV() const { return m_previewState.previewRTV; };
    inline ID3D11DepthStencilView* GetPreviewDSV() const { return m_previewState.previewDSV; };

    inline const uint32_t GetActiveMonitor() const { return m_activeMonitor; }
    // returns true if the monitors have a shared adapter
    inline bool MonitorHasSameAdapter(const uint32_t mon0, const uint32_t mon1) const
    {
        assertm(mon0 < m_numMonitors, "monitor 0 was out of range");
        assertm(mon1 < m_numMonitors, "monitor 1 was out of range");

        return m_pMonitors[mon0].m_pAdapter == m_pMonitors[mon1].m_pAdapter;
    };

    inline const XMMATRIX& GetProjMatrix() { return m_projectionMatrix; };
    inline const float* GetProjMatrixFloat() const { return (float*)&m_projectionMatrix; };

    inline CDXShaderManager* GetShaderManager() const { return m_pShaderManager; };

    inline CDXCamera* GetCamera() const { return m_pCamera; };

    inline CDXScene& GetScene() { return m_Scene; };

    inline CUIState& GetUIState() { return m_UIState; };

    inline ID3D11ShaderResourceView* GetCubemapSRV() { return m_CubemapTexSRV; };
    inline ID3D11ShaderResourceView* GetShadowMapSRV() { return m_shadowMapTexSRV; };
    inline ID3D11ShaderResourceView* GetCloudMaskSRV() { return m_cloudMaskSRV; };
    inline ID3D11ShaderResourceView* GetCSMDepthAtlasSamplerSRV() { return m_CSMDepthAtlasSamplerSRV; };
    inline ID3D11ShaderResourceView* GetStaticShadowTexSRV() { return m_staticShadowTextureSRV; };


    bool CreateViewForSceneWindow(const uint16_t w, const uint16_t h);
private:
    bool CreateDepthBuffer(ID3D11Texture2D* const frameBuffer, ID3D11Texture2D** depthBuffer, ID3D11DepthStencilView** depthStencilView);
	bool CreateMainView(const uint16_t w, const uint16_t h);

    bool CreateMisc();

    bool SetupAdapters();
    bool SetupMonitors();
    bool MonitorExists(HMONITOR hmonitor, size_t& index);
    bool SetActiveMonitor();
    bool SetupSwapchain();

	HWND m_windowHandle;

	ID3D11Device* m_pDevice;
	ID3D11DeviceContext* m_pDeviceContext;
	IDXGISwapChain* m_pSwapChain;
	ID3D11RenderTargetView* m_pMainView;
    ID3D11DepthStencilView* m_pDepthStencilView;
    ID3D11Texture2D* m_pDepthBuffer;

    ID3D11DepthStencilState* m_pDepthStencilStateNoDepthTest; // depth disabled
    ID3D11DepthStencilState* m_pDepthStencilState; // depth enabled

    ID3D11RasterizerState* m_pRasterizerState; // main rasterizer state
    ID3D11RasterizerState* m_pRasterizerStateWF; // main rasterizer state
    ID3D11SamplerState* m_pSamplerState;
    ID3D11SamplerState* m_pSamplerCmpState; // sampler comparison state because obviously we need this?

    struct MonitorAdapter_t
    {
        MonitorAdapter_t() : m_pAdapter(nullptr), m_pMonitor(nullptr) {};

        IDXGIAdapter1* m_pAdapter;
        HMONITOR m_pMonitor;
    };

    IDXGIAdapter1** m_ppAdapters;
    size_t m_numAdapters;
    MonitorAdapter_t* m_pMonitors;
    uint32_t m_numMonitors;
    uint32_t m_activeMonitor;

    uint16_t renderWidth;
    uint16_t renderHeight;

    XMMATRIX m_projectionMatrix;

    CDXShaderManager* m_pShaderManager;

    CDXCamera* m_pCamera;

    ID3D11Texture2D* m_CubemapTex;
    ID3D11ShaderResourceView* m_CubemapTexSRV;

    ID3D11Texture2D* m_shadowMapTex;
    ID3D11ShaderResourceView* m_shadowMapTexSRV;

    ID3D11Texture2D* m_cloudMask;
    ID3D11ShaderResourceView* m_cloudMaskSRV;

    ID3D11Texture2D* m_CSMDepthAtlasSampler;
    ID3D11ShaderResourceView* m_CSMDepthAtlasSamplerSRV;

    ID3D11Texture2D* m_staticShadowTexture;
    ID3D11ShaderResourceView* m_staticShadowTextureSRV;

    struct {
        ID3D11Texture2D* frameBuffer;
        ID3D11ShaderResourceView* frameBufferSRV;
        ID3D11RenderTargetView* previewRTV;
        ID3D11DepthStencilView* previewDSV;
        ID3D11Texture2D* previewDepthBuffer;
    } m_previewState;


    CDXScene m_Scene;

    CUIState m_UIState;
};

