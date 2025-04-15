#pragma once
#include <map>

class CInput
{
public:
	enum class KeyCode_t
	{
		KEY_SHIFT = VK_SHIFT,
		KEY_SPACE = VK_SPACE,

		KEY_0 = '0',
		KEY_1,
		KEY_2,
		KEY_3,
		KEY_4,
		KEY_5,
		KEY_6,
		KEY_7,
		KEY_8,
		KEY_9,

		KEY_A = 'A',
		KEY_B,
		KEY_C,
		KEY_D,
		KEY_E,
		KEY_F,
		KEY_G,
		KEY_H,
		KEY_I,
		KEY_J,
		KEY_K,
		KEY_L,
		KEY_M,
		KEY_N,
		KEY_O,
		KEY_P,
		KEY_Q,
		KEY_R,
		KEY_S,
		KEY_T,
		KEY_U,
		KEY_V,
		KEY_W,
		KEY_X,
		KEY_Y,
		KEY_Z,

		KEY_LWIN = VK_LWIN,

		KEY_LSHIFT = VK_LSHIFT,
		KEY_CONTROL = VK_CONTROL,
	};

	enum class MouseButton_t
	{
		LEFT = 0,
		MIDDLE = 1,
		RIGHT = 2,

	};

	CInput() : mousedx(0), mousedy(0), keyboardCaptured(true), mouseCaptured(false), keyStates() {};

	LPARAM WndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

	void Init(HWND hwnd) const;
	void Frame(float dt);

	void OnKeyStateChanged(KeyCode_t key, bool state);
	void OnMouseStateChanged(MouseButton_t button, bool state);
public:
	// get key state without any of the checks for whether we want the input
	inline bool GetKeyStateRaw(KeyCode_t key)
	{
		return keyStates[key];
	}

	inline bool GetKeyState(KeyCode_t key)
	{
		return keyboardCaptured && (!keyStates[KeyCode_t::KEY_LWIN] && keyStates[key]);
	};

	inline bool IsShiftPressed()
	{
		return GetKeyState(KeyCode_t::KEY_SHIFT) || GetKeyState(KeyCode_t::KEY_LSHIFT);
	}

	inline void ClearKeyStates()
	{
		if(keyStates.size() > 0)
			keyStates.clear();
	}

	inline bool GetMouseButtonState(MouseButton_t button)
	{
		return mouseStates[button];
	}

	inline void SetCursorVisible(bool state)
	{
		if (state)
			ShowCursor(TRUE);
		else
			while (ShowCursor(FALSE) >= 0);
	}

public:

	// mouse deltas
	int mousedx;
	int mousedy;

	bool keyboardCaptured; // is the input being captured by the window
	bool mouseCaptured;

private:
	std::map<KeyCode_t, bool> keyStates;
	std::map<MouseButton_t, bool> mouseStates;
};



extern CInput* g_pInput;