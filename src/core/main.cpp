#include <pch.h>
#include <core/crashhandler.h>

#include <thirdparty/imgui/imgui.h>
#include <thirdparty/imgui/backends/imgui_impl_win32.h>
#include <thirdparty/imgui/backends/imgui_impl_dx11.h>

#include <thirdparty/imgui/misc/imgui_utility.h>

#include <core/render/dx.h>
#include <core/input/input.h>
#include <core/cache/cachedb.h>
#include <core/utils/cli_parser.h>
#include <core/utils/exportsettings.h>
#include <core/filehandling/load.h>

#include <core/splash.h>
#include <core/window.h>
#include <core/render.h>
#include <iostream>
#include <game/rtech/utils/bsp/bspflags.h>
#include "version.h"
#include "update/update.h"
#include <game/asset.h>
#include <core/logging/logger.h>
#include "bridge/bridge.h"

#pragma warning(push, 0)
#pragma warning( disable: 4127 )

CDXParentHandler* g_dxHandler;
std::atomic<uint32_t> g_maxConcurrentThreadCount = 1u;

CBufferManager g_BufferManager; // called constructor on init.

// Handle CLI to only init certain asset types.
static void RegisterAssetTypeBindings(const CCommandLine* const cli)
{
    UNUSED(cli);

    // import func
    // model
    extern void InitModelAssetType();
    extern void InitAnimRigAssetType();
    extern void InitAnimSeqAssetType();
    extern void InitAnimSeqDataAssetType();
    extern void InitAnimRecordingAssetType();

    extern void InitSourceModelAssetType();
    extern void InitSourceSequenceAssetType();

    // texture/material
    extern void InitMaterialAssetType();
    extern void InitMaterialSnapshotAssetType();
    // mt4a
    extern void InitTextureAssetType();
    extern void InitTextureAnimationAssetType();
    extern void InitTextureListAssetType();
    extern void InitUIImageAtlasAssetType();
    extern void InitUIImageAssetType(); 
    extern void InitUIFontAtlasAssetType();

    // particle (texture)
    extern void InitEffectAssetType();
    extern void InitParticleScriptAssetType();
    
    // dx/shader
    extern void InitShaderAssetType();
    extern void InitShaderSetAssetType();
    
    // ui
    extern void InitUIAssetType();

    // rlcd
    extern void InitLcdScreenEffectAssetType();
    extern void InitRTKAssetType();
    
    // pak
    extern void InitPatchMasterAssetType();
    // vers
    
    // descriptors (stats, specs, etc)
    extern void InitDatatableAssetType();
    extern void InitSettingsAssetType();
    extern void InitSettingsLayoutAssetType();
    extern void InitRSONAssetType();
    extern void InitSubtitlesAsset();
    extern void InitLoclAssetType();

    extern void InitODLAssetType();
    
    // vpk
    extern void InitWrapAssetType();
    extern void InitWeaponDefinitionAssetType();
    extern void InitImpactAssetType();
    
    // map
    extern void InitMapAssetType();
    // llyr
    
    // audio
    extern void InitAudioSourceAssetType();

    // bluepoint
    extern void InitBluepointWrappedFileAssetType();

    extern void InitCubeAssetType();


    // call func
    // model
    InitModelAssetType();
    InitAnimRigAssetType();
    InitAnimSeqAssetType();
    InitAnimSeqDataAssetType();
    InitAnimRecordingAssetType();

    InitSourceModelAssetType();
    InitSourceSequenceAssetType();

    // texture/material
    InitMaterialAssetType();
    InitMaterialSnapshotAssetType();

    // mt4a
    InitTextureAssetType();
    InitTextureAnimationAssetType();
    InitTextureListAssetType();
    InitUIImageAtlasAssetType();
    InitUIImageAssetType();
    InitUIFontAtlasAssetType();

    // particle (texture)
    InitEffectAssetType();
    InitParticleScriptAssetType();

    // dx/shader
    InitShaderAssetType();
    InitShaderSetAssetType();

    // ui
    InitUIAssetType();
    // hsys
    InitLcdScreenEffectAssetType();
    InitRTKAssetType();

    // pak
    InitPatchMasterAssetType();
    // vers

    // descriptors (stats, specs, etc)
    InitDatatableAssetType();
    InitSettingsAssetType();
    InitSettingsLayoutAssetType();
    InitRSONAssetType();
    InitSubtitlesAsset();
    InitLoclAssetType();

    InitODLAssetType();

    // vpk
    InitWrapAssetType();
    InitWeaponDefinitionAssetType();
    InitImpactAssetType();

    // map
    InitMapAssetType();
    // llyr

    // audio
    InitAudioSourceAssetType();

    // bluepoint
    InitBluepointWrappedFileAssetType();

    InitCubeAssetType();

#if defined(DEBUG_NO_ASEQ_POSTLOAD)
    g_assetData.Log_Warning(nullptr, "Built with DEBUG_NO_ASEQ_POSTLOAD. Animation Sequence (aseq) assets will not work properly!");
    printf("\n");
#endif
}

#if defined(NDEBUG) && defined(_WIN32)
// https://stackoverflow.com/a/57241985
void CreateConsole()
{
    if (AttachConsole(ATTACH_PARENT_PROCESS))
        return;

    if (!AllocConsole())
        return;

    // std::cout, std::clog, std::cerr, std::cin
    FILE* fDummy;
    freopen_s(&fDummy, "CONOUT$", "w", stdout);
    freopen_s(&fDummy, "CONOUT$", "w", stderr);
    freopen_s(&fDummy, "CONIN$", "r", stdin);
    std::cout.clear();
    std::clog.clear();
    std::cerr.clear();
    std::cin.clear();

    // std::wcout, std::wclog, std::wcerr, std::wcin
    HANDLE hConOut = CreateFile(_T("CONOUT$"), GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    HANDLE hConIn = CreateFile(_T("CONIN$"), GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    SetStdHandle(STD_OUTPUT_HANDLE, hConOut);
    SetStdHandle(STD_ERROR_HANDLE, hConOut);
    SetStdHandle(STD_INPUT_HANDLE, hConIn);
    std::wcout.clear();
    std::wclog.clear();
    std::wcerr.clear();
    std::wcin.clear();
}
#endif

void RunWindowMsgLoop()
{
    bool quit = false;
    while (!quit)
    {
        MSG msg;
        while (PeekMessage(&msg, 0, 0, 0, PM_REMOVE))
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);

            if (msg.message == WM_QUIT)
                quit = true;
        }
        if (quit)
            break;

        HandleRenderFrame();
    }
}

int main(int argc, char* argv[])
{
    CCommandLine cli(argc, argv);

#if !defined(_DEBUG) && defined(_WIN32)
    if (IS_NOGUI(&cli))
        CreateConsole();
#endif

#if !defined(_DEBUG) // Allow the VS debugger to control its working directory
    // this is needed to properly support drag'n'drop, it changes the current working directory to the file you drag into the exe
    // HOWEVER: this should not apply when nogui is specified, as it is expected that shorter, relative file paths can be used when running CLI
    if (!IS_NOGUI(&cli) && !RestoreCurrentWorkingDirectory())
    {
        return EXIT_FAILURE;
    }
#endif

#if defined(_WIN32)
    // https://github.com/microsoft/DirectXTex/wiki/DirectXTex#initialization
    // [rika]: supposed to be done per thread but it Just Works so I'm not messing with it
    HRESULT hr = CoInitializeEx(nullptr, COINIT_MULTITHREADED);
    if (FAILED(hr))
    {
        assertm(false, "failed to initialize COM");
        return EXIT_FAILURE;
    }
#endif

#ifdef EXCEPTION_HANDLER
    g_CrashHandler.Init();
#endif

    g_rsxSettings.SetDefaultValues(&cli);

    g_maxConcurrentThreadCount = CThread::GetConCurrentThreads();

    g_cacheDBManager.LoadFromFile((std::filesystem::current_path() / "rsx_cache_db.bin").string());

    RegisterAssetTypeBindings(&cli);

    if (!IS_NOGUI(&cli))
    {
#if defined(SPLASHSCREEN)
        DrawSplashScreen();
#endif

        const HWND windowHandle = SetupWindow();

        g_dxHandler = new CDXParentHandler(windowHandle);
        if (!g_dxHandler->SetupDeviceD3D())
        {
            assertm(false, _T("Failed to setup D3D."));
            delete g_dxHandler;
            return EXIT_FAILURE;
        }

        CUIState& uiState = g_dxHandler->GetUIState();

        // If there is no new version detected, this will stay as nullptr
        uiState.newVersionType = nullptr;

#if !defined(_DEBUG) && !defined(NO_LIBCURL) // Update Checking should not run on debug as we probably don't care about updating
        if (!IS_NOGUI(&cli) && UtilsConfig->checkForUpdates && GetLatestGitHubReleaseInformation(&uiState.newVersionReleaseInfo))
        {
            const char* newVersionType = nullptr;
            if ((newVersionType = uiState.newVersionReleaseInfo.GetHighestDifferingVersionNumber()))
            {
                uiState.newVersionType = newVersionType;

                Log("** Found a new %s update: %s\n", newVersionType, uiState.newVersionReleaseInfo.tagName.c_str());
            }
        }
#endif

        g_pInput->Init(windowHandle);

        ShowWindow(windowHandle, SW_SHOWDEFAULT);
        UpdateWindow(windowHandle);

        ImGui::CreateContext();
        g_pImGuiHandler->SetStyle();
        g_pImGuiHandler->SetupHandler();

        ImGuiIO& io = ImGui::GetIO();
        io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard
            /*| ImGuiConfigFlags_NavEnableGamepad*/
            | ImGuiConfigFlags_DockingEnable
            | ImGuiConfigFlags_ViewportsEnable;

        GImGui->NavCursorVisible = false; // was NavDisableHighlight

        ImGui_ImplWin32_Init(windowHandle);
        ImGui_ImplDX11_Init(g_dxHandler->GetDevice(), g_dxHandler->GetDeviceContext());
    }
    else
    {
        g_dxHandler = new CDXParentHandler(NULL);

        g_pImGuiHandler->SetNoImGui(true);

        const uint32_t totalThreadCount = CThread::GetConCurrentThreads();

        if (const char* const numParseThreads = cli.GetParamValue("--parsethreads"))
            UtilsConfig->parseThreadCount = clamp(static_cast<uint32_t>(atoi(numParseThreads)), 1u, totalThreadCount);

        if (const char* const numExportThreads = cli.GetParamValue("--exportthreads"))
            UtilsConfig->exportThreadCount = clamp(static_cast<uint32_t>(atoi(numExportThreads)), 1u, totalThreadCount);
    }

    HandleLoadFromCommandLine(&cli);

    if (!IS_NOGUI(&cli))
    {
        CThread sockThread(Bridge_SetupSocketThread);
        sockThread.detach();

        RunWindowMsgLoop();
    }

    g_cacheDBManager.SaveToFile((std::filesystem::current_path() / RSX_CACHE_DB_FILENAME).string());

    if (!IS_NOGUI(&cli))
    {
        ImGui_ImplDX11_Shutdown();
        ImGui_ImplWin32_Shutdown();
        ImGui::DestroyContext();
    }

    delete g_dxHandler;

	return EXIT_SUCCESS;
}

int WINAPI WinMain(
    _In_ HINSTANCE /*hInstance*/,
    _In_opt_ HINSTANCE /*hPrevInstance*/,
    _In_ LPSTR     /*lpCmdLine*/,
    _In_ int       /*nCmdShow*/
)
{
    return main(__argc, __argv);
};

#pragma warning(pop)
