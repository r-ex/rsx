#pragma once
#include <game/rtech/cpakfile.h>

#if defined(PAKLOAD_PATCHING_ANY)
enum ePakPatchCommand_t
{
	CMD_0,
	CMD_1,
	CMD_2,
	CMD_3,
	CMD_4,
	CMD_5,
	CMD_6,
	_CMD_COUNT,
};

#define CMD_INVALID -1
static const int s_patchCmdToBytesToProcess[] = { CMD_INVALID, CMD_INVALID, CMD_INVALID, CMD_INVALID, 3, 7, 6, 0 };
#undef CMD_INVALID

extern const CPakFile::PatchFunc_t g_pakPatchApi[];
#endif // #if defined(PAKLOAD_PATCHING_ANY)