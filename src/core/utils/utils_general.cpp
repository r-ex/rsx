#include <pch.h>
#include <core/utils/utils_general.h>

void WaitForDebugger()
{
    while (!IsDebuggerPresent())
    {
        Sleep(1000);
    }
}