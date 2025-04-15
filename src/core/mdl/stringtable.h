#pragma once

struct stringentry_t
{
	stringentry_t(char* baseIn, int* offsetIn, const char* strIn) : base(baseIn), offset(offsetIn), string(strIn), address(nullptr), dupeIndex(-1) {};

	char* base; // the base for our offset
	int* offset; // pointer to our value holding the offset
	const char* string; // the actual string

	char* address; // position of the string once it is written.

	int dupeIndex; // index of the parent string
};

class StringTable
{
public:
	bool AddString(char* baseIn, int* offsetIn, const char* strIn);
	char* WriteStrings(char* buf);

private:
	std::vector<stringentry_t> strings;

};