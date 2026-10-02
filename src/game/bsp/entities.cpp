#include <pch.h>
#include "entities.h"

bool BSPEntity_s::GetValue(const std::string_view& key, std::string* outValue) const
{
	for (const auto& [entKey, entValue] : keyValues)
	{
		if (entKey == key)
		{
			*outValue = entValue;
			return true;
		}
	}

	return false;
}

static FORCEINLINE void SkipLine(const std::string_view& str, size_t& pos)
{
	pos = str.find('\n', pos);

	// if no newline is found, clamp to the end of the string
	if (pos == std::string::npos)
		pos = str.length();
}

static FORCEINLINE void SkipWhitespaceAndComments(const std::string_view& str, size_t& pos)
{
	while (pos < str.length())
	{
		if (isspace(static_cast<unsigned char>(str[pos])))
			++pos;
		else if (str.compare(pos, 2, "//") == 0)
			SkipLine(str, pos);
		else
			break;
	}
}

static FORCEINLINE bool ParseError(const std::string_view& str, size_t& pos, const char* errorMsg)
{
	UNUSED(str); UNUSED(pos); UNUSED(errorMsg);
	Log("BSP: failed to parse ent file: %s (line %u)", errorMsg, std::count(str.begin(), str.begin() + pos, '\n') + 1);

	return false;
}

static FORCEINLINE bool ParseString(const std::string_view& str, size_t& pos, std::string& outStr, const char** outMsg)
{
	if (pos >= str.size() || str[pos] != '"')
	{
		*outMsg = "expected a string literal";
		return false;
	}

	const size_t end = str.find('"', pos + 1);

	if (end == std::string_view::npos)
	{
		*outMsg = "found unterminated string literal";
		return false;
	}

	outStr = str.substr(pos + 1, end - (pos + 1));
	pos = end + 1;

	return true;
}

bool BSP_ParseEntities(const std::string_view& text, std::vector<BSPEntity_s>& outEntities)
{
	size_t pos = 0;

	// .ent files must start with a header line, e.g. "ENTITIES01" or "ENTITIES02 num_models=28", which we need to skip
	// because we don't care about it
	if (text.compare(pos, 8, "ENTITIES") == 0)
		SkipLine(text, pos);
	else
		return ParseError(text, pos, "expected ENTITIESxx header at the beginning of the file");

	std::vector<BSPEntity_s> entities;

	while (true)
	{
		SkipWhitespaceAndComments(text, pos);

		if (pos >= text.size())
			break;

		if (text[pos] != '{')
			return ParseError(text, pos, "expected '{'");

		++pos; // skip {

		BSPEntity_s& entity = entities.emplace_back();

		// Parse entity keys/values
		while (true)
		{
			SkipWhitespaceAndComments(text, pos);

			if (pos >= text.size())
				return ParseError(text, pos, "entity is not terminated with '}'");

			if (text[pos] == '}')
			{
				++pos;
				break;
			}

			const char* errorMsg;

			std::string key;
			// parse the key of the kv pair
			if (!ParseString(text, pos, key, &errorMsg))
				return ParseError(text, pos, errorMsg);

			SkipWhitespaceAndComments(text, pos);

			std::string value;
			if (!ParseString(text, pos, value, &errorMsg))
				return ParseError(text, pos, errorMsg);

			entity.keyValues.emplace_back(std::move(key), std::move(value));
		}
	}

	outEntities = std::move(entities);

	return true;
}
