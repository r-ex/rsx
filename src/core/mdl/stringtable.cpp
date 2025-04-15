#include <pch.h>

#include <core/mdl/stringtable.h>

bool StringTable::AddString(char* baseIn, int* offsetIn, const char* strIn)
{
	stringentry_t entry(baseIn, offsetIn, strIn);

	int i = 0;
	for (auto& str : strings)
	{
		if (!strncmp(entry.string, str.string, MAX_PATH))
		{
			entry.dupeIndex = i;
			break;
		}

		i++;
	}

	strings.push_back(entry);
	return entry.dupeIndex > -1 ? true : false; // true if dupe
}

char* StringTable::WriteStrings(char* buf)
{
	for (auto& entry : strings)
	{
		if (entry.dupeIndex > -1)
		{
			stringentry_t& parent = strings.at(entry.dupeIndex);
			*entry.offset = static_cast<int>(parent.address - entry.base);

			continue;
		}

		entry.address = buf;

		if (entry.offset)
		{
			*entry.offset = static_cast<int>(entry.address - entry.base);
		}

		if (entry.string)
		{
			size_t leng = strnlen(entry.string, MAX_PATH);
			strncpy_s(buf, leng + 1, entry.string, leng + 1);

			buf += leng;
		}

		// null terminator
		*buf = '\0';
		buf++;
	}

	return buf;
}