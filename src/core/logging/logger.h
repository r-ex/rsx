#pragma once
#include <cstdarg>
#include <stdio.h>

#if _DEBUG
inline void Log(const char* fmt, ...)
{
	va_list args;
	va_start(args, fmt);
	vprintf(fmt, args);
	va_end(args);
}
#else
#define Log(...) ((void)nullptr)
#endif