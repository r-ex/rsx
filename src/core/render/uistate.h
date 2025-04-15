#pragma once

class CUIState
{
public:
	inline void ShowSettingsWindow(bool state) { settingsWindowVisible = state; };

public:
	bool settingsWindowVisible;
};