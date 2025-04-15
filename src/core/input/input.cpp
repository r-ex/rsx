#include <pch.h>
#include <core/input/input.h>
#include <core/render/dx.h>

#include <hidusage.h>
#include <imgui.h>

extern CDXParentHandler* g_dxHandler;

LPARAM CInput::WndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	UNUSED(hwnd);
	UNUSED(lParam);

	switch (uMsg)
	{
	case WM_KEYDOWN:
	case WM_KEYUP:
	{
		KeyCode_t key = static_cast<KeyCode_t>(wParam);

		bool keyState = uMsg == WM_KEYDOWN;

		this->OnKeyStateChanged(key, keyState);

		break;
	}
	case WM_INPUT:
	{
		// if no mouse capture, break early so we don't have any expensive allocation or retrieval
		if (!mouseCaptured)
			break;

		// get the size of the buffer we need
		UINT size = 0;
		GetRawInputData((HRAWINPUT)lParam, RID_INPUT, NULL, &size, sizeof(RAWINPUTHEADER));

		char* data = new char[size];

		GetRawInputData((HRAWINPUT)lParam, RID_INPUT, data, &size, sizeof(RAWINPUTHEADER));
		RAWINPUT* rawInput = reinterpret_cast<RAWINPUT*>(data);

		if (rawInput->header.dwType == RIM_TYPEMOUSE)
		{
			this->mousedx = rawInput->data.mouse.lLastX;
			this->mousedy = rawInput->data.mouse.lLastY;
		}

		delete[] data;

		break;
	}
	case WM_RBUTTONDOWN:
	case WM_RBUTTONUP:
	{
		this->OnMouseStateChanged(MouseButton_t::RIGHT, uMsg == WM_RBUTTONDOWN);
		break;
	}
	case WM_LBUTTONDOWN:
	case WM_LBUTTONUP:
	{
		this->OnMouseStateChanged(MouseButton_t::LEFT, uMsg == WM_LBUTTONDOWN);
		break;
	}
	case WM_KILLFOCUS:
	{
		this->ClearKeyStates();
		break;
	}
	}

	return 0;
}

void CInput::Init(HWND hwnd) const
{
	RAWINPUTDEVICE rid{};
	rid.usUsagePage = 1;
	rid.usUsage = HID_USAGE_GENERIC_MOUSE;
	rid.dwFlags = 0;
	rid.hwndTarget = hwnd;

	RegisterRawInputDevices(&rid, 1, sizeof(rid));
}

void CInput::Frame(float dt)
{
	//UNUSED(dt);

	g_dxHandler->GetCamera()->AddRotation(mousedx * 0.5f * dt, mousedy * 0.5f * dt, 0);

	if (this->mouseCaptured)
	{
		if (GetActiveWindow() != NULL)
		{
			RECT rect{};
			GetWindowRect(g_dxHandler->GetWindowHandle(), &rect);
			int cx = (rect.left + rect.right) / 2;
			int cy = (rect.bottom + rect.top) / 2;
			SetCursorPos(cx, cy);
		}
	}

	mousedx = 0;
	mousedy = 0;
}

void CInput::OnKeyStateChanged(CInput::KeyCode_t key, bool state)
{
	// if state isnt changing dont bother running any further code
	// this prevents any future input callbacks from being called multiple times incorrectly
	if (GetKeyState(key) == state)
		return;

	// if ImGui is capturing keyboard input, we don't want to accept any input changes
	if (ImGui::GetIO().WantCaptureKeyboard)
		return;

	keyStates[key] = state;
}

void CInput::OnMouseStateChanged(CInput::MouseButton_t button, bool state)
{
	if (GetMouseButtonState(button) == state)
		return;

	// Only allow a mouse button to be released when ImGui has focus. this may cause more problems than it solves but meh
	if (state && ImGui::GetIO().WantCaptureMouse)
		return;

	if (button == MouseButton_t::RIGHT && state == true)
	{
		this->SetCursorVisible(mouseCaptured);

		if (mouseCaptured)
			ImGui::GetIO().ConfigFlags &= ~ImGuiConfigFlags_NoMouse;
		else
			ImGui::GetIO().ConfigFlags |= ImGuiConfigFlags_NoMouse;

		mouseCaptured = !mouseCaptured;
	}

	mouseStates[button] = state;
}

CInput* g_pInput = new CInput();