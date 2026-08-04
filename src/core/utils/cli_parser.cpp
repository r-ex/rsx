#include <pch.h>
#include <core/utils/cli_parser.h>
#include <core/utils/utils_general.h>

std::unordered_set<uint32_t> CLI_GetCommaSeparatedAssetTypes(const CCommandLine* const cli, const char* const paramName)
{
	std::unordered_set<uint32_t> types;

	const char* const typeString = cli->GetParamValue(paramName);

	if (!typeString)
		return types;

	std::istringstream ss;
	ss.str(typeString);

	for (std::string line; std::getline(ss, line, ',');)
	{
		const size_t typeLength = line.length();
		// Types must be 4 characters or fewer
		if (typeLength > 4 || typeLength == 0)
			continue;
		
		// there MUST be a better way of doing this
		const char a = line[0];
		const char b = typeLength >= 2 ? line[1] : '\0';
		const char c = typeLength >= 3 ? line[2] : '\0';
		const char d = typeLength >= 4 ? line[3] : '\0';

		types.insert(MAKEFOURCC(a, b, c, d));
	}

	// this code is terrible
	if (types.size() == 0)
	{
		bool foundComma = false;

		// Only need to search 5 characters. The max length of asset type is 4 characters, so if the 5th is not a comma, then there is no valid type
		for (int i = 0; i < strnlen_s(typeString, 5); ++i)
		{
			if (typeString[i] == ',')
				foundComma = true;
		}

		// If there's no delimiter, then there is only one specified type so add it to the vector
		if (!foundComma)
			types.insert(MAKEFOURCC(typeString[0], typeString[1], typeString[2], typeString[3]));
	}

	return types;
}

void GetTextFilterForExport(const char* filterString, TextFilter* filter)
{
	if(filterString)
		filter->Build(filterString);
}

