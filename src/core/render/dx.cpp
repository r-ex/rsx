#include <pch.h>
#include <core/render/dx.h>
#include <core/input/input.h>

#include <thirdparty/directxtex/DirectXTex.h>

#include <wincodec.h>

#if _DEBUG
#pragma comment(lib, "thirdparty/directxtex/DirectXTex_x64d.lib")
#else
#pragma comment(lib, "thirdparty/directxtex/DirectXTex_x64r.lib")
#endif

#define ToScratchImage reinterpret_cast<DirectX::ScratchImage*>(m_texture)

extern CDXParentHandler* g_dxHandler;
extern PreviewSettings_t g_PreviewSettings;

CTexture::CTexture(const char* const buf, const size_t bufSize, const size_t width, const size_t height, const DXGI_FORMAT imgFormat, const size_t arraySize, const size_t mipLevels) : m_width(width), m_height(height), m_shaderResourceView(nullptr)
{
    InitTexture(buf, bufSize, width, height, imgFormat, arraySize, mipLevels);
}

CTexture::CTexture(ID3D11Device* const device, const char* const buf, const size_t bufSize, const size_t width, const size_t height, const DXGI_FORMAT imgFormat, const size_t arraySize, const size_t mipLevels) : m_width(width), m_height(height), m_shaderResourceView(nullptr)
{
    InitTexture(buf, bufSize, width, height, imgFormat, arraySize, mipLevels);
    CreateShaderResourceView(device);
}

CTexture::~CTexture()
{
    if (m_texture)
    {
        delete ToScratchImage;
    }
    m_texture = nullptr;

    if (m_shaderResourceView)
    {
        m_shaderResourceView->Release();
    }
    m_shaderResourceView = nullptr;
}

void CTexture::InitTexture(const char* const buf, const size_t bufSize, const size_t width, const size_t height, const DXGI_FORMAT imgFormat, const size_t arraySize, const size_t mipLevels)
{
    m_texture = new DirectX::ScratchImage();

    DirectX::TexMetadata data = {};
    data.width = width;
    data.height = height;
    data.depth = 1; // Need depth of 1 otherwise no data creation.
    data.arraySize = arraySize;
    data.format = imgFormat;
    data.mipLevels = mipLevels;
    data.dimension = DirectX::TEX_DIMENSION_TEXTURE2D;

    ToScratchImage->Initialize(data);
    if (buf)
        CopyRawToTexture(buf, bufSize * arraySize);
}

const size_t CTexture::GetSlicePitch() const
{
    return ToScratchImage->GetImages()->slicePitch;
}

uint8_t* CTexture::GetPixels()
{
    return ToScratchImage->GetImages()->pixels;
}

const uint8_t* CTexture::GetPixels() const
{
    return ToScratchImage->GetImages()->pixels;
}

void CTexture::CopyRawToTexture(const char* const buf, const size_t bufSize)
{
    std::memcpy(ToScratchImage->GetImages()->pixels, buf, bufSize);
}

void CTexture::CopySourceTextureSlice(CTexture* const src, const size_t x, const size_t y, const size_t w, const size_t h, const size_t offsetX, const size_t offsetY)
{
    DirectX::CopyRectangle(*reinterpret_cast<DirectX::ScratchImage*>(src->GetTexture())->GetImage(0, 0, 0), { x, y, w, h }, *ToScratchImage->GetImage(0, 0, 0), DirectX::TEX_FILTER_DEFAULT, offsetX, offsetY);
}

bool CTexture::CreateShaderResourceView(ID3D11Device* const device)
{
    assertm(device, "Invalid device.");
    assertm(m_texture, "Invalid scratch image.");
    return SUCCEEDED(DirectX::CreateShaderResourceView(device, ToScratchImage->GetImages(), ToScratchImage->GetImageCount(), ToScratchImage->GetMetadata(), &m_shaderResourceView));
}

bool CTexture::ExportAsPng(const std::filesystem::path& exportPath)
{
    if (!IsValid32bppFormat())
    {
        const DXGI_FORMAT convertFormat = DirectX::IsSRGB(ToScratchImage->GetMetadata().format) ? DXGI_FORMAT_B8G8R8A8_UNORM_SRGB : DXGI_FORMAT_B8G8R8A8_UNORM;
        if (!ConvertToFormat(convertFormat))
        {
            assertm(false, "Converting the texture format failed.");
            return false;
        }
    }

    const HRESULT res = DirectX::SaveToWICFile(*ToScratchImage->GetImages(), DirectX::WIC_FLAGS::WIC_FLAGS_FORCE_SRGB, DirectX::GetWICCodec(DirectX::WICCodecs::WIC_CODEC_PNG), exportPath.wstring().c_str(), nullptr, 
        [](IPropertyBag2* props)
        {
            PROPBAG2 options{};
            VARIANT varValues{};
            options.pstrName = (LPOLESTR)L"FilterOption";
            varValues.vt = VT_UI1;
            varValues.bVal = WICPngFilterOption::WICPngFilterUp;

            props->Write(1u, &options, &varValues);
        });

    return SUCCEEDED(res);
}

bool CTexture::ExportAsDds(const std::filesystem::path& exportPath)
{
    return SUCCEEDED(DirectX::SaveToDDSFile(ToScratchImage->GetImages(), ToScratchImage->GetImageCount(), ToScratchImage->GetMetadata(), DirectX::DDS_FLAGS::DDS_FLAGS_NONE, exportPath.wstring().c_str()));
}

bool CTexture::ConvertToFormat(const DXGI_FORMAT format)
{
    if (ToScratchImage->GetMetadata().format == format)
        return true;

    DXGI_FORMAT decompressFormat = DXGI_FORMAT::DXGI_FORMAT_UNKNOWN;
    if (!DirectX::IsCompressed(format))
        decompressFormat = format;

    const size_t imgCount = ToScratchImage->GetImageCount();

    // Try to decompress current txtr.
    if (DirectX::IsCompressed(ToScratchImage->GetMetadata().format))
    {
        std::unique_ptr<DirectX::ScratchImage> tempImage = std::make_unique<DirectX::ScratchImage>();
        const HRESULT res = DirectX::Decompress(ToScratchImage->GetImages(), imgCount, ToScratchImage->GetMetadata(), decompressFormat, *reinterpret_cast<DirectX::ScratchImage*>(tempImage.get()));
        if (SUCCEEDED(res))
        {
            delete ToScratchImage;
            m_texture = tempImage.release();
        }
        else
        {
            assertm(false, "Decompress failed.");
            return false;
        }
    }

    // Compress txtr if target format is compressed.
    if (DirectX::IsCompressed(format))
    {
        std::unique_ptr<DirectX::ScratchImage> tempImage = std::make_unique<DirectX::ScratchImage>();
        const HRESULT res = DirectX::Compress(ToScratchImage->GetImages(), imgCount, ToScratchImage->GetMetadata(), format, DirectX::TEX_COMPRESS_FLAGS::TEX_COMPRESS_DEFAULT, DirectX::TEX_THRESHOLD_DEFAULT, *reinterpret_cast<DirectX::ScratchImage*>(tempImage.get()));
        if (SUCCEEDED(res))
        {
            delete ToScratchImage;
            m_texture = tempImage.release();
        }
        else
        {
            assertm(false, "Compress failed.");
            return false;
        }
    }
    else if (ToScratchImage->GetMetadata().format != format)
    {
        std::unique_ptr<DirectX::ScratchImage> tempImage = std::make_unique<DirectX::ScratchImage>();
        const HRESULT res = DirectX::Convert(ToScratchImage->GetImages(), imgCount, ToScratchImage->GetMetadata(), format, DirectX::TEX_FILTER_FLAGS::TEX_FILTER_DEFAULT, DirectX::TEX_THRESHOLD_DEFAULT, *reinterpret_cast<DirectX::ScratchImage*>(tempImage.get()));
        if (SUCCEEDED(res))
        {
            delete ToScratchImage;
            m_texture = tempImage.release();
        }
        else
        {
            assertm(false, "Convert failed.");
            return false;
        }
    }

    return true;
}

const size_t CTexture::GetBpp(const DXGI_FORMAT format)
{
    return DirectX::BitsPerPixel(format);
}

const size_t CTexture::GetPixelBlock(const DXGI_FORMAT format)
{
    // within the range of a BC format?
    if ((format >= DXGI_FORMAT_BC1_TYPELESS && format <= DXGI_FORMAT_BC5_SNORM) || format >= DXGI_FORMAT_BC6H_TYPELESS && format <= DXGI_FORMAT_BC7_UNORM_SRGB)
        return 4;

    return 1; // not a BC format
}

const int CTexture::Morton(uint32_t i, uint32_t sx, uint32_t sy)
{
    int v0 = 1;
    int v1 = 1;
    int v2 = i;
    int v3 = sx;
    int v4 = sy;
    int v5 = 0;
    int v6 = 0;
    while (v3 > 1 || v4 > 1)
    {
        if (v3 > 1)
        {
            v5 += v1 * (v2 & 1);
            v2 >>= 1;
            v1 *= 2;
            v3 >>= 1;
        }
        if (v4 > 1)
        {
            v6 += v0 * (v2 & 1);
            v2 >>= 1;
            v0 *= 2;
            v4 >>= 1;
        }
    }
    return v6 * sx + v5;
}

// https://www.tech-artists.org/t/how-to-calculate-the-blue-channel-for-normal-map/4436/7
inline float GetNormalZFromXY(const float x, const float y)
{
    const float xm = (2.0f * x) - 1.0f;
    const float ym = (2.0f * y) - 1.0f;

    const float a = 1.f - (xm * xm) - (ym * ym);

    // normalized (?) can't be a valid blue value if it's above 1.0f anyway.
    if (a < 0.0f)
        return 0.5f;

    const float sq = sqrtf(a);

    return (sq / 2.0f) + 0.5f;
}

void CTexture::ConvertNormalOpenDX()
{
    const DXGI_FORMAT format = ToScratchImage->GetMetadata().format;

    // not a valid normal texture for this function.
    if (format > DXGI_FORMAT::DXGI_FORMAT_BC5_SNORM || format < DXGI_FORMAT::DXGI_FORMAT_BC5_TYPELESS)
        return;
    
    ConvertToFormat(DXGI_FORMAT::DXGI_FORMAT_R8G8B8A8_UNORM);

    for (size_t i = 0; i < (ToScratchImage->GetPixelsSize()); i += 4)
    {
        uint8_t* pixels = &GetPixels()[i];

        const float x = static_cast<float>(pixels[0]) / 255.0f; // r
        const float y = static_cast<float>(pixels[1]) / 255.0f; // g

        const float z = GetNormalZFromXY(x, y);

        pixels[2] = static_cast<uint8_t>(z * 255.0f);
    }

    // I'd prefer to use this since uncompressed normals are fat, but it's way too slow.
    //ConvertToFormat(DXGI_FORMAT::DXGI_FORMAT_BC7_UNORM);

    return;
}

void CTexture::ConvertNormalOpenGL()
{
    const DXGI_FORMAT format = ToScratchImage->GetMetadata().format;

    // not a valid normal texture for this function.
    if (format > DXGI_FORMAT::DXGI_FORMAT_BC5_SNORM || format < DXGI_FORMAT::DXGI_FORMAT_BC5_TYPELESS)
        return;
    
    ConvertToFormat(DXGI_FORMAT::DXGI_FORMAT_R8G8B8A8_UNORM);

    for (size_t i = 0; i < (ToScratchImage->GetPixelsSize()); i += 4)
    {
        uint8_t* pixels = &GetPixels()[i];

        const float x = static_cast<float>(pixels[0]) / 255.0f; // r
        const float y = static_cast<float>(255 - pixels[1]) / 255.0f; // g

        const float z = GetNormalZFromXY(x, y);

        pixels[1] = static_cast<uint8_t>(y * 255.0f);
        pixels[2] = static_cast<uint8_t>(z * 255.0f);
    }

    // I'd prefer to use this since uncompressed normals are fat, but it's way too slow.
    //ConvertToFormat(DXGI_FORMAT::DXGI_FORMAT_BC7_UNORM);

    return;
}

bool CTexture::IsValid32bppFormat()
{
    const DXGI_FORMAT format = ToScratchImage->GetMetadata().format;
    switch (format)
    {
    case DXGI_FORMAT::DXGI_FORMAT_R8G8B8A8_UNORM:
    case DXGI_FORMAT::DXGI_FORMAT_B8G8R8A8_UNORM:
    case DXGI_FORMAT::DXGI_FORMAT_B8G8R8X8_UNORM:
        return true;
    default:
        return false;
    }

    unreachable();
}

// Pitch: rotation.x
// Yaw: rotation.y
// Roll: rotation.z
void CDXCamera::AddRotation(float yaw, float pitch, float roll)
{
    rotation.y += yaw;
    if (rotation.y > (XM_PI))
        rotation.y -= 2.f * XM_PI;

    if (rotation.y < (-XM_PI))
        rotation.y += 2.f * XM_PI;

    rotation.x = std::clamp(rotation.x + pitch, -DEG2RAD(90), DEG2RAD(90));

    rotation.z += roll;
}

XMMATRIX CDXCamera::GetViewMatrix()
{
    const XMMATRIX rotMatrix = XMMatrixRotationRollPitchYaw(rotation.x, rotation.y, rotation.z);

    target = XMVector3TransformCoord(XMVectorSet(0, 0, 1, 0), rotMatrix);
    up = XMVector3TransformCoord(XMVectorSet(0, 1, 0, 0), rotMatrix);

    target = position.AsXMVector() + target;

    return XMMatrixLookAtLH(position.AsXMVector(), target, up);
}

bool CDXParentHandler::SetupDeviceD3D()
{
    assertm((m_windowHandle != nullptr), "Didn't initialize window handle.");
    assertm((m_pDevice == nullptr), "DX device was already setup.");

    DXGI_SWAP_CHAIN_DESC swapChainDesc = {};
    swapChainDesc.BufferCount = 2u;
    swapChainDesc.BufferDesc.Width = 0u;
    swapChainDesc.BufferDesc.Height = 0u;
    swapChainDesc.BufferDesc.Format = DXGI_FORMAT_R16G16B16A16_FLOAT;
    swapChainDesc.BufferDesc.RefreshRate.Numerator = 60u;
    swapChainDesc.BufferDesc.RefreshRate.Denominator = 1u;
    swapChainDesc.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;
    swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    swapChainDesc.OutputWindow = m_windowHandle;
    swapChainDesc.SampleDesc.Count = 1u;
    swapChainDesc.SampleDesc.Quality = 0u;
    swapChainDesc.Windowed = true;
    swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;

    D3D_FEATURE_LEVEL featureLevel;
    const D3D_FEATURE_LEVEL featureLevelArr[2] = { D3D_FEATURE_LEVEL_11_0, D3D_FEATURE_LEVEL_10_0 };

    UINT deviceFlags = 0;

#ifdef _DEBUG
    deviceFlags |= D3D11_CREATE_DEVICE_DEBUG;
#endif

    // [rika]: this is likely causing our issue with a white screen on igpus, todo look into an adapter here
    if (D3D11CreateDeviceAndSwapChain(nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, deviceFlags, featureLevelArr, ARRSIZE(featureLevelArr),
        D3D11_SDK_VERSION, &swapChainDesc, &m_pSwapChain, &m_pDevice, &featureLevel, &m_pDeviceContext) != S_OK)
    {
        assertm(false, "Failed to setup device and swapchain.");
        return false;
    }

    RECT windowRect = {};
    GetWindowRect(m_windowHandle, &windowRect);

    const uint16_t width = static_cast<uint16_t>(windowRect.right - windowRect.left);
    const uint16_t height = static_cast<uint16_t>(windowRect.bottom - windowRect.top);

    if (!CreateMainView(width, height))
        return false;

    if (!CreateMisc())
        return false;

    return true;
}

bool CDXParentHandler::CreateMainView(const uint16_t w, const uint16_t h)
{
    ID3D11Texture2D* pBackBuffer = nullptr;
    if (FAILED(m_pSwapChain->GetBuffer(0u, IID_PPV_ARGS(&pBackBuffer))))
    {
        assertm(false, "Failed to get swapchain buffer.");
        return false;
    }

    if (FAILED(m_pDevice->CreateRenderTargetView(pBackBuffer, nullptr, &m_pMainView)))
    {
        assertm(false, "Failed to create render target view.");
        return false;
    }

    this->CreateDepthBuffer(pBackBuffer);
    {
        D3D11_RASTERIZER_DESC desc = {};
        desc.FillMode = D3D11_FILL_SOLID;
        desc.CullMode = D3D11_CULL_NONE;

        if (FAILED(m_pDevice->CreateRasterizerState(&desc, &m_pRasterizerState)))
        {
            assertm(false, "Failed to create rasterizer state.");
            return false;
        }
    }

    {
        D3D11_DEPTH_STENCIL_DESC desc = {};
        desc.DepthEnable = true;
        desc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL;
        desc.DepthFunc = D3D11_COMPARISON_LESS;

        if (FAILED(m_pDevice->CreateDepthStencilState(&desc, &m_pDepthStencilState)))
            assertm(false, "Failed to create depth stencil state.");
    }

    {
        // eventually there should be multiple sampler states that are chosen depending on material flags for model preview
        D3D11_SAMPLER_DESC desc = {};
        desc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
        desc.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;
        desc.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;
        desc.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;
        desc.MipLODBias = 0.f;
        desc.MaxAnisotropy = 1;
        desc.ComparisonFunc = D3D11_COMPARISON_ALWAYS;
        desc.BorderColor[0] = 0.f;
        desc.BorderColor[1] = 0.f;
        desc.BorderColor[2] = 0.f;
        desc.BorderColor[3] = 0.f;
        desc.MinLOD = 0.f;
        desc.MaxLOD = D3D11_FLOAT32_MAX;

        if (FAILED(m_pDevice->CreateSamplerState(&desc, &m_pSamplerState)))
            assertm(false, "Failed to create sampler state.");
    }

    {
        D3D11_SAMPLER_DESC desc = {};
        desc.Filter = D3D11_FILTER_COMPARISON_MIN_MAG_LINEAR_MIP_POINT;
        desc.AddressU = D3D11_TEXTURE_ADDRESS_CLAMP;
        desc.AddressV = D3D11_TEXTURE_ADDRESS_CLAMP;
        desc.AddressW = D3D11_TEXTURE_ADDRESS_CLAMP;
        desc.MipLODBias = 0.f;
        desc.MaxAnisotropy = 1;
        desc.ComparisonFunc = D3D11_COMPARISON_LESS;
        desc.BorderColor[0] = 0.f;
        desc.BorderColor[1] = 0.f;
        desc.BorderColor[2] = 0.f;
        desc.BorderColor[3] = 0.f;
        desc.MinLOD = 0.f;
        desc.MaxLOD = D3D11_FLOAT32_MAX;

        if (FAILED(m_pDevice->CreateSamplerState(&desc, &m_pSamplerCmpState)))
            assertm(false, "Failed to create sampler comparison state (shadowmapSampler)");
    }

    m_wndProjMatrix = XMMatrixPerspectiveFovLH(0.25f * XM_PI, static_cast<float>(w) / h, 0.1f, g_PreviewSettings.previewCullDistance);

    pBackBuffer->Release();
    return true;
}

bool CDXParentHandler::CreateDepthBuffer(ID3D11Texture2D* const frameBuffer)
{
    ID3D11Texture2D* depthBuffer = nullptr;
    D3D11_TEXTURE2D_DESC depthBufferDesc = {};

    // copy desc from the frame buffer
    frameBuffer->GetDesc(&depthBufferDesc);

    // https://learn.microsoft.com/en-us/windows/win32/direct3d11/d3d10-graphics-programming-guide-depth-stencil
    depthBufferDesc.Format = DXGI_FORMAT_R32_TYPELESS;
    depthBufferDesc.BindFlags = D3D11_BIND_DEPTH_STENCIL;

    HRESULT hr = m_pDevice->CreateTexture2D(&depthBufferDesc, NULL, &depthBuffer);
    if (FAILED(hr) || !depthBuffer)
    {
        assertm(false, "Failed to create depth buffer.");
        return false;
    }

    D3D11_DEPTH_STENCIL_VIEW_DESC depthStencilViewDesc = {};
    depthStencilViewDesc.Format = DXGI_FORMAT_D32_FLOAT;
    depthStencilViewDesc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;
    depthStencilViewDesc.Texture2D.MipSlice = 0u; // depth buffer does not have mips so make sure it uses the right data

    hr = m_pDevice->CreateDepthStencilView(depthBuffer, &depthStencilViewDesc, &m_pDepthStencilView);
    if (FAILED(hr))
        assertm(false, "Failed to create depth stencil view.");

    return true;
}

bool CDXParentHandler::CreateMisc()
{
    assertm(!m_pShaderManager, "Shader Manager already exists.");

    m_pShaderManager = new CDXShaderManager();

    m_pCamera = new CDXCamera();

    m_pCamera->position = Vector(0, 0, -50.f);

    return true;
}

void CDXParentHandler::CleanupD3D()
{
    // Cleanup main render target first.
    DX_RELEASE_PTR(m_pMainView);
    DX_RELEASE_PTR(m_pDevice);
    DX_RELEASE_PTR(m_pDeviceContext);
    DX_RELEASE_PTR(m_pSwapChain);
    DX_RELEASE_PTR(m_pDepthStencilView);
    DX_RELEASE_PTR(m_pDepthStencilState);
    DX_RELEASE_PTR(m_pRasterizerState);
    DX_RELEASE_PTR(m_pSamplerState);
    DX_RELEASE_PTR(m_pSamplerCmpState);
}

void CDXParentHandler::HandleResize(const uint16_t x, const uint16_t y)
{
    DX_RELEASE_PTR(m_pMainView);
    m_pSwapChain->ResizeBuffers(0u, static_cast<uint32_t>(x), static_cast<uint32_t>(y), DXGI_FORMAT_UNKNOWN, 0u);
    CreateMainView(x, y);
}

std::shared_ptr<CTexture> CDXParentHandler::CreateRenderTexture(const char* const buf, const size_t bufSize, const int width, const int height, const DXGI_FORMAT imgFormat, const size_t arraySize, const size_t mipLevels)
{
    return std::make_shared<CTexture>(m_pDevice, buf, bufSize, static_cast<size_t>(width), static_cast<size_t>(height), imgFormat, arraySize, mipLevels);
}

void CDXCamera::Move(float dt)
{
    const float yaw = this->rotation.y;

    // calculate x and z movement components based on horizontal rotation
    const float x = sin(yaw);
    const float z = cos(yaw);

    // components for normal movement (left/right)
    const float nx = sin(DEG2RAD(90) - yaw);
    const float nz = cos(DEG2RAD(90) - yaw);

    Vector& pos = this->position;

    float moveSpeed = g_PreviewSettings.previewMovementSpeed;

    if(g_pInput->GetKeyState(CInput::KeyCode_t::KEY_CONTROL))
        moveSpeed = moveSpeed * PREVIEW_SPEED_MULT;

    if (g_pInput->GetKeyState(CInput::KeyCode_t::KEY_SPACE))
        pos.y += moveSpeed * dt;

    if (g_pInput->IsShiftPressed())
        pos.y -= moveSpeed * dt;

    if (g_pInput->GetKeyState(CInput::KeyCode_t::KEY_W))
    {
        pos.x += moveSpeed * dt * x;
        pos.z += moveSpeed * dt * z;
    }

    if (g_pInput->GetKeyState(CInput::KeyCode_t::KEY_A))
    {
        pos.x += moveSpeed * dt * -nx;
        pos.z += moveSpeed * dt * nz;
    }

    if (g_pInput->GetKeyState(CInput::KeyCode_t::KEY_S))
    {
        pos.x -= moveSpeed * dt * x;
        pos.z -= moveSpeed * dt * z;
    }

    if (g_pInput->GetKeyState(CInput::KeyCode_t::KEY_D))
    {
        pos.x -= moveSpeed * dt * -nx;
        pos.z -= moveSpeed * dt * nz;
    }
}

inline void CalculateCameraRelativeToClipMatrix(XMMATRIX* a, const XMMATRIX* a2, const XMMATRIX* a3)
{
    a->r[0].m128_f32[0] = ((a2->r[0].m128_f32[0] * a3->r[0].m128_f32[0]) + (a3->r[0].m128_f32[1] * a2->r[1].m128_f32[0]))
        + (a3->r[0].m128_f32[2] * a2->r[2].m128_f32[0]);
    a->r[0].m128_f32[1] = ((a2->r[1].m128_f32[1] * a3->r[0].m128_f32[1]) + (a2->r[0].m128_f32[1] * a3->r[0].m128_f32[0]))
        + (a3->r[0].m128_f32[2] * a2->r[2].m128_f32[1]);
    a->r[0].m128_f32[2] = ((a3->r[0].m128_f32[1] * a2->r[1].m128_f32[2]) + (a3->r[0].m128_f32[0] * a2->r[0].m128_f32[2]))
        + (a3->r[0].m128_f32[2] * a2->r[2].m128_f32[2]);
    a->r[0].m128_i32[3] = a3->r[0].m128_i32[3];
    a->r[1].m128_f32[0] = ((a3->r[1].m128_f32[1] * a2->r[1].m128_f32[0]) + (a3->r[1].m128_f32[0] * a2->r[0].m128_f32[0]))
        + (a3->r[1].m128_f32[2] * a2->r[2].m128_f32[0]);
    a->r[1].m128_f32[1] = ((a2->r[1].m128_f32[1] * a3->r[1].m128_f32[1]) + (a2->r[0].m128_f32[1] * a3->r[1].m128_f32[0]))
        + (a3->r[1].m128_f32[2] * a2->r[2].m128_f32[1]);
    a->r[1].m128_f32[2] = ((a3->r[1].m128_f32[1] * a2->r[1].m128_f32[2]) + (a3->r[1].m128_f32[0] * a2->r[0].m128_f32[2]))
        + (a3->r[1].m128_f32[2] * a2->r[2].m128_f32[2]);
    a->r[1].m128_i32[3] = a3->r[1].m128_i32[3];
    a->r[2].m128_f32[0] = ((a2->r[0].m128_f32[0] * a3->r[2].m128_f32[0]) + (a3->r[2].m128_f32[1] * a2->r[1].m128_f32[0]))
        + (a2->r[2].m128_f32[0] * a3->r[2].m128_f32[2]);
    a->r[2].m128_f32[1] = ((a2->r[1].m128_f32[1] * a3->r[2].m128_f32[1]) + (a2->r[0].m128_f32[1] * a3->r[2].m128_f32[0]))
        + (a3->r[2].m128_f32[2] * a2->r[2].m128_f32[1]);
    a->r[2].m128_f32[2] = ((a2->r[1].m128_f32[2] * a3->r[2].m128_f32[1]) + (a3->r[2].m128_f32[0] * a2->r[0].m128_f32[2]))
        + (a2->r[2].m128_f32[2] * a3->r[2].m128_f32[2]);
    a->r[2].m128_i32[3] = a3->r[2].m128_i32[3];
    a->r[3].m128_f32[0] = ((a3->r[3].m128_f32[1] * a2->r[1].m128_f32[0]) + (a3->r[3].m128_f32[0] * a2->r[0].m128_f32[0]))
        + (a3->r[3].m128_f32[2] * a2->r[2].m128_f32[0]);
    a->r[3].m128_f32[1] = ((a2->r[1].m128_f32[1] * a3->r[3].m128_f32[1]) + (a2->r[0].m128_f32[1] * a3->r[3].m128_f32[0]))
        + (a3->r[3].m128_f32[2] * a2->r[2].m128_f32[1]);
    a->r[3].m128_f32[2] = ((a3->r[3].m128_f32[1] * a2->r[1].m128_f32[2]) + (a3->r[3].m128_f32[0] * a2->r[0].m128_f32[2]))
        + (a3->r[3].m128_f32[2] * a2->r[2].m128_f32[2]);
}

void CDXCamera::UpdateCommonCameraData()
{
    CBufCommonPerCamera& data = commonCameraData;

    data.c_cameraOrigin = this->position;
    data.c_cameraRelativeToClipPrevFrame = data.c_cameraRelativeToClip;

    //data.c_cameraRelativeToClip = ( - );
    //data.c_cameraRelativeToClip.r[0].m128_f32[3] = 0;
    //data.c_cameraRelativeToClip.r[1].m128_f32[3] = 0;
    //data.c_cameraRelativeToClip.r[2].m128_f32[3] = 1;
    //data.c_cameraRelativeToClip.r[3].m128_f32[3] = 0;
    //data.c_cameraRelativeToClip = XMMatrixTranspose(data.c_cameraRelativeToClip);

    const XMMATRIX view = g_dxHandler->GetCamera()->GetViewMatrix();
    const XMMATRIX proj = g_dxHandler->GetProjMatrix();
    CalculateCameraRelativeToClipMatrix(&data.c_cameraRelativeToClip, &view, &proj);


    data.c_cameraOriginPrevFrame = this->position;
    data.c_frameNum++;

    data.c_clipPlane = { NAN, NAN, NAN, NAN };

    data.c_skyColor = { 0.15321f, 0.16049f, 0.17255f };
    data.c_envMapLightScale = 1.f;
    data.c_sunColor = { 2.23922f, 2.08272f, 1.80694f };

    data.c_maxLightingValue = 5.f;

    data.c_zNear = 0.1f;

    data.c_viewportMinMaxZ = float2(0.9f, 1.f);
    data.c_viewportScale = float2(1.f, 1.f);
    data.c_rcpViewportScale = float2(1.f, 1.f);
    data.c_framebufferViewportScale = float2(1.f, 1.f);
    data.c_rcpFramebufferViewportScale = float2(1.f, 1.f);
    data.c_separateIndirectDiffuse = 1;


    data.c_renderTargetSize = float2(1920, 1080);
    data.c_rcpRenderTargetSize = float2(1.f/1920, 1.f/1080);

    data.c_cameraLenY = 1.f;
    data.c_cameraRcpLenY = 1.f / data.c_cameraLenY;
}

void CDXCamera::CommitCameraDataBufferUpdates()
{
    this->UpdateCommonCameraData();

    if (!this->bufCommonPerCamera)
    {

        D3D11_BUFFER_DESC desc = {};

        desc.Usage = D3D11_USAGE_DYNAMIC;
        desc.ByteWidth = sizeof(CBufCommonPerCamera);
        desc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
        desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
        desc.MiscFlags = 0;

        D3D11_SUBRESOURCE_DATA srd{ &this->commonCameraData };

        if (FAILED(g_dxHandler->GetDevice()->CreateBuffer(&desc, &srd, &bufCommonPerCamera)))
            return;
    }
    else
    {
        ID3D11DeviceContext* const ctx = g_dxHandler->GetDeviceContext();

        D3D11_MAPPED_SUBRESOURCE buf;
        ctx->Map(this->bufCommonPerCamera, 0, D3D11_MAP_WRITE_DISCARD, 0, &buf);

        memcpy(buf.pData, &this->commonCameraData, sizeof(CBufCommonPerCamera));

        ctx->Unmap(this->bufCommonPerCamera, 0);
    }
}
