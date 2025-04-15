#include <pch.h>
#include <core/window.h>
#include <core/render/dx.h>
#include <thirdparty/imgui/imgui.h>
#include <core/input/input.h>
#include <resource.h>

extern std::atomic<bool> inJobAction;

const POINT GetCenterOfNearestScreen(const POINT& windowSize)
{
    POINT curPos = {};
    GetCursorPos(&curPos);

    const HMONITOR monitorHandle = MonitorFromPoint(curPos, MONITOR_DEFAULTTONEAREST);
    assert(monitorHandle);

    MONITORINFO monitorInfo = {};
    monitorInfo.cbSize = sizeof(MONITORINFO);
    GetMonitorInfo(monitorHandle, &monitorInfo);

    curPos.x = std::max(monitorInfo.rcWork.left, (monitorInfo.rcWork.left + monitorInfo.rcWork.right) / 2 - windowSize.x / 2);
    curPos.y = std::max(monitorInfo.rcWork.top, (monitorInfo.rcWork.top + monitorInfo.rcWork.bottom) / 2 - windowSize.y / 2);
    return curPos;
}


extern CDXParentHandler* g_dxHandler;
extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
LRESULT CALLBACK WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    if (ImGui_ImplWin32_WndProcHandler(hWnd, msg, wParam, lParam))
        return true;

    switch (msg)
    {
    case WM_SIZE:
    {
        if (wParam == SIZE_MINIMIZED)
            return 0;

        assertm(g_dxHandler, "DX Global is invalid");
        g_dxHandler->HandleResize(LOWORD(lParam), HIWORD(lParam));
        return 0;
    }
    case WM_SYSCOMMAND:
    {
        if ((wParam & 0xfff0) == SC_KEYMENU) // Disable ALT application menu
            return 0;
        break;
    }
    case WM_DESTROY:
    {
        PostQuitMessage(0);
        return 0;
    }
    case WM_KEYDOWN:
    case WM_KEYUP:
    case WM_INPUT:
    case WM_KILLFOCUS:

    case WM_RBUTTONDOWN:
    case WM_RBUTTONUP:
    case WM_LBUTTONDOWN:
    case WM_LBUTTONUP:
    {
        return g_pInput->WndProc(hWnd, msg, wParam, lParam);
    }
    }

    return DefWindowProcW(hWnd, msg, wParam, lParam);
}

const HWND SetupWindow()
{
    WNDCLASSEXW windowClass;
    windowClass.cbSize = sizeof(WNDCLASSEXW);
    windowClass.style = CS_CLASSDC;
    windowClass.lpfnWndProc = WndProc;
    windowClass.cbClsExtra = 0;
    windowClass.cbWndExtra = 0;
    windowClass.hInstance = GetModuleHandle(nullptr);
    windowClass.hIcon = LoadIcon(windowClass.hInstance, MAKEINTRESOURCE(IDI_ICON1));
    windowClass.hCursor = nullptr;
    windowClass.hbrBackground = nullptr;
    windowClass.lpszMenuName = nullptr;
    windowClass.lpszClassName = L"reSource Xtractor";
    windowClass.hIconSm = nullptr;

    constexpr POINT windowSize = { 1336, 768 };
    const POINT windowPos = GetCenterOfNearestScreen(windowSize);

    RegisterClassExW(&windowClass);

    return CreateWindowW(
        windowClass.lpszClassName,
        L"reSource Xtractor",
        WS_OVERLAPPEDWINDOW,
        windowPos.x, windowPos.y,
        windowSize.x, windowSize.y,
        nullptr, nullptr,
        windowClass.hInstance,
        nullptr
    );;
}
