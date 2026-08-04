#pragma once

class CCommandLine
{
public:
	CCommandLine(int _argc, char* _argv[]) : argc((uint32_t)_argc), argv(_argv)
	{
		assert(_argc >= 0);
	};

	const char* const GetSelfPath() const
	{
		assert(argc > 0);
		return argv[0];
	}

	const bool HasParam(const char* const param) const
	{
		assert(param);
		for (uint32_t i = 1; i < argc; ++i)
		{
			if (strcmp(argv[i], param) == 0)
			{
				return true;
			}
		}

		return false;
	}

	const uint32_t GetParamIdx(const char* const param) const
	{
		assert(param);
		for (uint32_t i = 1; i < argc; ++i)
		{
			if (strcmp(argv[i], param) == 0)
			{
				return i;
			}
		}

		return UINT32_MAX;
	}

	const char* const GetParamValue(const char* const param) const
	{
		assert(param);
		const uint32_t idx = GetParamIdx(param);
		return (idx != UINT32_MAX && idx != argc-1) ? GetParamValue(idx+1) : nullptr;
	}

	const char* const GetParamValue(const uint32_t idx) const
	{
		if (idx < 0 || idx >= argc)
		{
			assertm(false, "range check failure");
			return nullptr;
		}

		return argv[idx];
	}

	inline uint32_t GetArgC() const
	{
		return argc;
	}

	inline uint32_t GetFirstNonFlagArgIdx() const
	{
		uint32_t i = 1;
		// Skip the first argument, as it's always our own executable path
		for (i = 1; i < argc; ++i)
		{
			// If the arg starts with a dash, then we need to keep looking
			if (argv[i][0] == '-')
			{
				// If the arg starts with TWO dashes, we should skip an extra arg as long as the subsequent conditions hold true
				if (strlen(argv[i]) > 1 && argv[i][1] == '-')
				{
					// If this arg is not the final arg, then check if the next arg starts with a dash
					// If the next arg does start with a dash, then we should not skip the extra arg here.
					if (i != argc - 1 && argv[i + 1][0] != '-')
						i++;
				}
			}
			else // No dash: we've found the first non-flag argument
				break;
		}
		return i;
	}

private:
	uint32_t argc;
	char** argv;
};

std::unordered_set<uint32_t> CLI_GetCommaSeparatedAssetTypes(const CCommandLine* const cli, const char* const paramName);
void GetTextFilterForExport(const char* filterString, TextFilter* filter);