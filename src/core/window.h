#pragma once

const POINT GetCenterOfNearestScreen(const POINT& windowSize);

void HandleOpenFileDialog(const HWND windowHandle);
void HandleModelDialog(const HWND windowHandle);

const HWND SetupWindow();