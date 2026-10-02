#pragma once

struct BSPEntity_s
{
	std::vector<std::pair<std::string, std::string>> keyValues;

	// returns true if the key was found, with the value being put in outValue, otherwise false
	bool GetValue(const std::string_view& key, std::string* outValue) const;
};

bool BSP_ParseEntities(const std::string_view& text, std::vector<BSPEntity_s>& outEntities);
