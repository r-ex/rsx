#include <pch.h>
#include <core/crashhandler.h>

#include <thirdparty/imgui/imgui.h>
#include <thirdparty/imgui/misc/imgui_utility.h>
#include <thirdparty/imgui/backends/imgui_impl_win32.h>
#include <thirdparty/imgui/backends/imgui_impl_dx11.h>

#include <core/render/dx.h>
#include <core/input/input.h>
#include <core/cache/cachedb.h>
#include <core/utils/cli_parser.h>
#include <core/filehandling/load.h>

#include <core/splash.h>
#include <core/window.h>
#include <core/render.h>

CDXParentHandler* g_dxHandler;
std::atomic<uint32_t> maxConcurrentThreads = 1u;

CBufferManager g_BufferManager; // called constructor on init.

// Handle CLI to only init certain asset types.
static void HandleAssetRegistration(const CCommandLine* const cli)
{
    UNUSED(cli);

    // import func
    // model
    extern void InitModelAssetType();
    extern void InitAnimRigAssetType();
    extern void InitAnimSeqAssetType();
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
    // hsys
    // rlcd
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


    // call func
    // model
    InitModelAssetType();
    InitAnimRigAssetType();
    InitAnimSeqAssetType();
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
    // rlcd
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
}

int main(int argc, char* argv[])
{
    CCommandLine cli(argc, argv);

    // we want the visual studio debugger to be able to control the working directory
#if defined(NDEBUG)
    // this is needed to properly support drag'n'drop, it changes the current working directory to the file you drag into the exe
    if (!RestoreCurrentWorkingDirectory())
    {
        return EXIT_FAILURE;
    }
#endif // #if defined(NDEBUG)

#ifdef EXCEPTION_HANDLER
    g_CrashHandler.Init();
#endif

    g_cacheDBManager.LoadFromFile((std::filesystem::current_path() / "rsx_cache_db.bin").string());

    // init pak asset types
    HandleAssetRegistration(&cli);

    // get max con-current threads.
    maxConcurrentThreads = std::max(1u, CThread::GetConCurrentThreads());

#if defined(SPLASHSCREEN)
    DrawSplashScreen(); // draw splashscreen now for 2~ seconds
#endif // #if defined(SPLASHSCREEN)

    const HWND windowHandle = SetupWindow();

    g_dxHandler = new CDXParentHandler(windowHandle);
    if (!g_dxHandler->SetupDeviceD3D())
    {
        assertm(false, _T("Failed to setup D3D."));
        delete g_dxHandler;
        return EXIT_FAILURE;
    }
    g_pInput->Init(windowHandle);

    ShowWindow(windowHandle, SW_SHOWDEFAULT);
    UpdateWindow(windowHandle);

    ImGui::CreateContext();
    g_pImGuiHandler->SetStyle();
    g_pImGuiHandler->SetupHandler();

    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard /*| ImGuiConfigFlags_NavEnableGamepad*/;

    GImGui->NavDisableHighlight = true;

    ImGui_ImplWin32_Init(windowHandle);
    ImGui_ImplDX11_Init(g_dxHandler->GetDevice(), g_dxHandler->GetDeviceContext());

    // call after initializing dx and gui otherwise you will crash
    HandleLoadFromCommandLine(&cli);

    MSG msg = {};
    while (msg.message != WM_QUIT)
    {
        while (PeekMessage(&msg, 0, 0, 0, PM_REMOVE))
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);

            if (msg.message == WM_QUIT)
                break;
        }

        HandleRenderFrame();
    }

    g_cacheDBManager.SaveToFile((std::filesystem::current_path() / "rsx_cache_db.bin").string());

    ImGui_ImplDX11_Shutdown();
    ImGui_ImplWin32_Shutdown();
    ImGui::DestroyContext();

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