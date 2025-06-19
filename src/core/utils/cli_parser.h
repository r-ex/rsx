#pragma once

class CCommandLine
{
public:
	CCommandLine(const int argc, char* argv[]) : argc(argc), argv(argv)
	{
	};

	const char* const GetSelfPath() const
	{
		assert(argc > 0);
		return argv[0];
	}

	const int HasParam(const char* const param) const
	{
		assert(param);
		for (int i = 1; i < argc; ++i)
		{
			if (strcmp(argv[i], param) == 0)
			{
				return i;
			}
		}

		return -1;
	}


	const char* const GetParamValue(const char* const param) const
	{
		assert(param);
		const int idx = HasParam(param);
		return (idx != -1) ? GetParamValue(idx) : nullptr;
	}

	const char* const GetParamValue(const int idx) const
	{
		if (idx < 0 || idx >= argc)
		{
			assertm(false, "range check failure");
			return nullptr;
		}

		return argv[idx];
	}

	inline const int GetArgC() const
	{
		return argc;
	}

private:
	int argc;
	char** argv;
};