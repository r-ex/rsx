#include <pch.h>
#include <core/splash.h>
#include <core/window.h>

#include <resource.h>

#if defined(SPLASHSCREEN)
LRESULT CALLBACK WndProcSplash(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    static HBITMAP bitMap = nullptr;
    switch (message)
    {
    case WM_CREATE:
    {
        bitMap = reinterpret_cast<HBITMAP>(LoadImage(GetModuleHandle(nullptr), MAKEINTRESOURCE(IDB_BITMAP1), IMAGE_BITMAP, 0, 0, LR_DEFAULTCOLOR));
        assert(bitMap);
        return 0;
    }
    case WM_DESTROY:
    {
        if (bitMap)
        {
            DeleteObject(bitMap);
            bitMap = nullptr;
        }
        PostQuitMessage(0);
        return 0;
    }
    case WM_CUSTOM_DESTROY: // work around to properly terminate window from another thread
    {
        DestroyWindow(hWnd);
        break;
    }
    case WM_PAINT:
    {
        PAINTSTRUCT ps;
        const HDC hdc = BeginPaint(hWnd, &ps);
        assert(hdc);

        if (bitMap)
        {
            // draw the loaded bitmap image
            const HDC hdcMem = CreateCompatibleDC(hdc);
            if (hdcMem)
            {
                const HBITMAP hbmOld = reinterpret_cast<HBITMAP>(SelectObject(hdcMem, bitMap));

                // get the res of the bitmap
                BITMAP bm;
                GetObject(bitMap, sizeof(bm), &bm);

                RECT clientRect;
                GetClientRect(hWnd, &clientRect);

                // stretch image to window size
                SetStretchBltMode(hdc, HALFTONE);
                StretchBlt(hdc, 0, 0, clientRect.right - clientRect.left, clientRect.bottom - clientRect.top, hdcMem, 0, 0, bm.bmWidth, bm.bmHeight, SRCCOPY);

                SelectObject(hdcMem, hbmOld);
                DeleteDC(hdcMem);
            }
        }

        EndPaint(hWnd, &ps);
        return 0;
    }
    }

    return DefWindowProcW(hWnd, message, wParam, lParam);
}

void DrawSplashScreen()
{
    WNDCLASSEXW windowClass = {};
    windowClass.cbSize = sizeof(WNDCLASSEXW);
    windowClass.style = CS_CLASSDC;
    windowClass.lpfnWndProc = WndProcSplash;
    windowClass.cbClsExtra = 0;
    windowClass.cbWndExtra = 0;
    windowClass.hInstance = GetModuleHandle(nullptr);
    windowClass.hIcon = LoadIcon(windowClass.hInstance, MAKEINTRESOURCE(IDI_ICON1));
    windowClass.hCursor = nullptr;
    windowClass.hbrBackground = nullptr;
    windowClass.lpszMenuName = nullptr;
    windowClass.lpszClassName = L"reSource Xtractor Loading...";
    windowClass.hIconSm = nullptr;

    constexpr POINT windowSize = { 706, 427 };
    const POINT windowPos = GetCenterOfNearestScreen(windowSize);

    RegisterClassExW(&windowClass);
    const HWND windowHandle = CreateWindowW(windowClass.lpszClassName, L"reSource Xtractor Loading...", WS_POPUP, windowPos.x, windowPos.y, windowSize.x, windowSize.y, nullptr, nullptr, windowClass.hInstance, nullptr);
    assert(windowHandle);

    ShowWindow(windowHandle, SW_NORMAL);

    CThread([windowHandle]
        {
            Sleep(2000);
            PostMessageA(windowHandle, WM_CUSTOM_DESTROY, 0, 0);
        }).detach();

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

            Sleep(100);
        }

        UnregisterClassW(windowClass.lpszClassName, windowClass.hInstance);
}
#endif // #if defined(SPLASHSCREEN)
