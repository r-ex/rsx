#include <pch.h>

#include <core/utils/keyvalue_parser.h>

namespace kv_parser
{
	Token_t::Token_t(const char** input, const TokenState_t tokenState) : token(nullptr), numTokens(0), state(tokenState), numChildren(0), children(nullptr)
	{	
		assertm(IsEOF(**input) == false, "tried to read end of file");

		// get our token
		char* tokie = GetToken(input);
		numTokens = TokenCountFromToken(tokie);
		token = new TokenData_t[numTokens]{};

		// token is expected to be value
		if (state == TOKEN_VALUE)
		{
			HandleValueToken(tokie);
			FreeAllocArray(tokie);
			return;
		}

		// probably supported but I am not doing this right now
		assertm(numTokens == 1, "multiple tokens as a key");

		token[0].data.data_string = tokie;
		token[0].dataType = TOKEN_STRING;

		// set value data
		if (IsArray(**input) == false)
		{
			numChildren = 1;
			children = new Token_t[numChildren]{};

			children[0] = Token_t(input, TOKEN_VALUE);

			return;
		}

		// value is array
		state = TOKEN_ARRAY;
		children = new Token_t[maxNumChildren]{};

		const char* array = *input;
		array++; // skip past curly bracket

		SeekThroughWhitespace(&array);

		while (IsArray(array[0]) == false)
		{
			assertm(numChildren < maxNumChildren, "max children exceeded");
			children[numChildren] = Token_t(&array, TOKEN_KEY);
			numChildren++;
		}

		// shrink children
		ShrinkChildren();

		array++; // skip past curly bracket
		SeekThroughWhitespace(&array);
		*input = array;
	}

	// write this to a text file
	void Token_t::SerializeToken(CTextBuffer* const text) const
	{
		text->WriteCharacter('\"');

		for (int16_t i = 0; i < numTokens; i++)
		{
			if (i > 0)
			{
				text->WriteCharacter(',');
			}

			const TokenData_t* const tokie = token + i;

			switch (tokie->dataType)
			{
			case TOKEN_NONE:
			{
				break;
			}
			case TOKEN_STRING:
			{
				text->WriteString(tokie->data.data_string);
				break;
			}
			case TOKEN_INT:
			{
				text->WriteFormatted("%i", tokie->data.data_int);
				break;
			}
			case TOKEN_FLOAT:
			{
				text->WriteFormatted("%f", tokie->data.data_float);
				break;
			}
			}
		}

		text->WriteCharacter('\"');
	}

	void Token_t::Serialize(CTextBuffer* const text) const
	{
		if (state != TOKEN_VALUE)
		{
			text->WriteIndentation();
		}

		SerializeToken(text);

		switch (state)
		{
		case TOKEN_VALUE:
		{
			text->WriteCharacter('\n');
			break;
		}
		case TOKEN_KEY:
		{
			text->WriteCharacter(' ');
			children[0].Serialize(text);

			break;
		}
		case TOKEN_ARRAY:
		{
			text->WriteCharacter('\n');
			text->WriteIndentation();
			PTEXTBUFFER_WRITE_STRING_CT(text, "{\n");

			text->IncreaseIndenation();
			for (int i = 0; i < numChildren; i++)
			{
				children[i].Serialize(text);
			}
			text->DecreaseIndenation();

			text->WriteIndentation();
			PTEXTBUFFER_WRITE_STRING_CT(text, "}\n");

			break;
		}
		}
	}

	void Token_t::ReplaceToken(const void* const value, const TokenType_t tokenType, const int idx)
	{
		TokenData_t* const tokie = token + idx;

		switch (tokie->dataType)
		{
		case TOKEN_STRING:
		{
			FreeAllocArray(tokie->data.data_string);
			break;
		}
		default:
		{
			break;
		}
		}

		switch (tokenType)
		{
		case TOKEN_NONE:
		{
			tokie->data.data_none = nullptr;
			break;
		}
		case TOKEN_STRING:
		{
			const char* const string = reinterpret_cast<const char* const>(value);
			size_t length = strnlen_s(string, maxStringLength);
			char* tmp = new char[length + 1] {};
			strncpy_s(tmp, length + 1, string, length);
			tokie->data.data_string = tmp;

			break;
		}
		case TOKEN_INT:
		{
			tokie->data.data_int = *reinterpret_cast<const int* const>(value);
			break;
		}
		case TOKEN_FLOAT:
		{
			tokie->data.data_float = *reinterpret_cast<const float* const>(value);
			break;
		}
		default:
		{
			assertm(false, "invalid type");
			break;
		}
		}

		tokie->dataType = tokenType;
	}

	// get the number of tokens from a string and put null terminators between them
	const int16_t Token_t::TokenCountFromToken(char* tokie)
	{
		int16_t tokenCount = 1;
		while (tokie[0])
		{
			if (IsComma(tokie[0]))
			{
				// for reading later
				tokie[0] = '\0';
				tokenCount++;
			}

			tokie++;
		}

		return tokenCount;
	}

	const bool Token_t::IsWhitespace(const char character) const
	{
		switch (character)
		{
		case '\t':
		case '\n':
		case ' ':
		{
			return true;
		}
		}

		return false;
	}

	const bool Token_t::IsArray(const char character) const
	{
		switch (character)
		{
		case '{':
		case '}':
		{
			return true;
		}
		}

		return false;
	}

	void Token_t::HandleValueToken(char* tokie)
	{
		for (int16_t i = 0; i < numTokens; i++)
		{
			// seek to the next value
			if (i > 0)
			{
				// seek through the current token
				while (tokie[0] != '\0')
				{
					tokie++;
				}

				// skip over the null terminator for the previous token
				tokie++;
			}

			// determine what type of token this is
			errno = 0;
			char* endOfString = nullptr;
			size_t depth = 0ull;

			const size_t length = strnlen_s(tokie, maxStringLength);

			const int iVal = strtol(tokie, &endOfString, 10);
			depth = endOfString - tokie;
			if (length && errno != ERANGE && depth == length)
			{
				token[i].data.data_int = iVal;
				token[i].dataType = TOKEN_INT;

				continue;
			}

			// token can only be represented as floating point number
			const float flVal = strtof(tokie, &endOfString);
			depth = endOfString - tokie;
			if (length && errno != ERANGE && depth == length)
			{
				token[i].data.data_float = flVal;
				token[i].dataType = TOKEN_FLOAT;

				continue;
			}

			const size_t bufSize = length + 1;
			token[i].data.data_string = new char[bufSize];
			token[i].dataType = TOKEN_STRING;

			memset(token[i].data.data_string, 0, bufSize);
			strncpy_s(token[i].data.data_string, bufSize, tokie, length);
		}
	}

	// skip through the quotes around a token
	void Token_t::SeekThroughQuotes(const char** input) const
	{
		const char* value = *input;

		assertm(value[0] == '\"', "token was not quoted");
		value++;

		// seek to the next quotation mark
		while (value[0] != '\"')
		{
			value++;
		}

		// position after quotation mark
		value++;

		*input = value;
	}

	// skip through the whitespace around a token
	void Token_t::SeekThroughWhitespace(const char** input) const
	{
		const char* value = *input;

		while (IsWhitespace(value[0]))
		{
			value++;
		}

		*input = value;
	}

	// get a quoted token
	char* Token_t::GetQuotedToken(const char** input) const
	{
		const char* start = *input;
		SeekThroughQuotes(input);
		const char* end = *input;

		// tuck in to exclude quotation marks
		start++;
		end--;

		const size_t length = end - start;
		char* out = new char[length + 1]{};
		memcpy_s(out, length + 1, start, length);

		return out;
	}

	// get white space terminated token
	char* Token_t::GetWhitespaceToken(const char** input) const
	{
		const char* const start = *input;
		const char* end = start;

		while (IsWhitespace(*end) == false)
		{
			end++;
		}

		const size_t length = end - start;
		char* out = new char[length + 1] {};
		memcpy_s(out, length + 1, start, length);

		*input = end;
		return out;
	}

	char* Token_t::GetToken(const char** input) const
	{
		const char* current = *input;
		char* tokie = nullptr;

		if (IsArray(current[0]))
		{
			return nullptr;
		}

		const bool quoted = IsQuoted(current);
		if (quoted)
		{
			tokie = GetQuotedToken(&current);
		}
		else
		{
			tokie = GetWhitespaceToken(&current);
		}

		// skip to the next token for our next operation
		SeekThroughWhitespace(&current);

		assertm(tokie, "token was nullptr");

		*input = current;
		return tokie;
	}

	void Token_t::ShrinkChildren()
	{
		Token_t* tmp = new Token_t[numChildren]{};
		for (int i = 0; i < numChildren; i++)
		{
			tmp[i] = children[i];
		}

		FreeAllocArray(children);
		children = tmp;
	}
}