//=============================================================================//
//
// Purpose: Crash handler (overrides the game's implementation!)
//
//=============================================================================//
#include <pch.h>
#include <core/crashhandler.h>

#ifdef EXCEPTION_HANDLER
#include <psapi.h>
#include <minidumpapiset.h>

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CCrashHandler::Start()
{
	AcquireSRWLockExclusive(&m_Lock);
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CCrashHandler::End()
{
	ReleaseSRWLockExclusive(&m_Lock);
}

//-----------------------------------------------------------------------------
// Purpose: formats the crasher (module, address and exception)
//-----------------------------------------------------------------------------
void CCrashHandler::FormatCrash()
{
	m_Buffer.append("crash:\n{\n");

	FormatExceptionAddress();
	FormatExceptionCode();

	m_Buffer.append("}\n");
}

//-----------------------------------------------------------------------------
// Purpose: formats the captured callstack
//-----------------------------------------------------------------------------
void CCrashHandler::FormatCallstack()
{
	m_Buffer.append("callstack:\n{\n");

	if (m_nCapturedFrames)
	{
		const PEXCEPTION_RECORD pExceptionRecord = m_pExceptionPointers->ExceptionRecord;

		if (m_ppStackTrace[m_nCapturedFrames - 1] == pExceptionRecord->ExceptionAddress)
		{
			const PCONTEXT pContextRecord = m_pExceptionPointers->ContextRecord;
			MEMORY_BASIC_INFORMATION mbi = { 0 };

			const SIZE_T t = VirtualQuery((LPCVOID)pContextRecord->Rsp, &mbi, sizeof(LPCVOID));

			if (t >= sizeof(mbi)
				&& !(mbi.Protect & PAGE_NOACCESS)
				&& (mbi.Protect & (PAGE_READONLY | PAGE_READWRITE))
				&& (mbi.State & MEM_COMMIT))
			{
				m_Buffer.append("\t// call stack ended; possible return address?\n");
			}
		}
	}
	for (WORD i = 0; i < m_nCapturedFrames; i++)
	{
		FormatExceptionAddress(reinterpret_cast<LPCSTR>(m_ppStackTrace[i]));
	}

	m_Buffer.append("}\n");
}

//-----------------------------------------------------------------------------
// Purpose: formats all the registers and their contents
//-----------------------------------------------------------------------------
void CCrashHandler::FormatRegisters()
{
	m_Buffer.append("registers:\n{\n");
	const PCONTEXT pContextRecord = m_pExceptionPointers->ContextRecord;

	FormatALU("rax", pContextRecord->Rax);
	FormatALU("rbx", pContextRecord->Rbx);
	FormatALU("rcx", pContextRecord->Rcx);
	FormatALU("rdx", pContextRecord->Rdx);
	FormatALU("rsp", pContextRecord->Rsp);
	FormatALU("rbp", pContextRecord->Rbp);
	FormatALU("rsi", pContextRecord->Rsi);
	FormatALU("rdi", pContextRecord->Rdi);
	FormatALU("r8 ", pContextRecord->R8);
	FormatALU("r9 ", pContextRecord->R9);
	FormatALU("r10", pContextRecord->R10);
	FormatALU("r11", pContextRecord->R11);
	FormatALU("r12", pContextRecord->R12);
	FormatALU("r13", pContextRecord->R13);
	FormatALU("r14", pContextRecord->R14);
	FormatALU("r15", pContextRecord->R15);
	FormatALU("rip", pContextRecord->Rip);

	FormatFPU("xmm0 ", &pContextRecord->Xmm0);
	FormatFPU("xmm1 ", &pContextRecord->Xmm1);
	FormatFPU("xmm2 ", &pContextRecord->Xmm2);
	FormatFPU("xmm3 ", &pContextRecord->Xmm3);
	FormatFPU("xmm4 ", &pContextRecord->Xmm4);
	FormatFPU("xmm5 ", &pContextRecord->Xmm5);
	FormatFPU("xmm6 ", &pContextRecord->Xmm6);
	FormatFPU("xmm7 ", &pContextRecord->Xmm7);
	FormatFPU("xmm8 ", &pContextRecord->Xmm8);
	FormatFPU("xmm9 ", &pContextRecord->Xmm9);
	FormatFPU("xmm10", &pContextRecord->Xmm10);
	FormatFPU("xmm11", &pContextRecord->Xmm11);
	FormatFPU("xmm12", &pContextRecord->Xmm12);
	FormatFPU("xmm13", &pContextRecord->Xmm13);
	FormatFPU("xmm14", &pContextRecord->Xmm14);
	FormatFPU("xmm15", &pContextRecord->Xmm15);

	m_Buffer.append("}\n");
}

//-----------------------------------------------------------------------------
// Purpose: formats all loaded modules (verbose)
//-----------------------------------------------------------------------------
void CCrashHandler::FormatModules()
{
	m_Buffer.append("modules:\n{\n");

	const HANDLE hProcess = GetCurrentProcess();

	DWORD cbNeeded;
	const BOOL result = EnumProcessModulesEx(hProcess, m_ppModuleHandles, MAX_MODULE_HANDLES, &cbNeeded, LIST_MODULES_ALL);

	if (result && cbNeeded <= MAX_MODULE_HANDLES && cbNeeded >> 3)
	{
		CHAR szModuleName[MAX_PATH];
		LPSTR pszModuleName;
		MODULEINFO modInfo;

		for (DWORD i = 0, j = cbNeeded >> 3; j; i++, j--)
		{
			const DWORD m = GetModuleFileNameA(m_ppModuleHandles[i], szModuleName, sizeof(szModuleName));

			if ((m - 1) > (sizeof(szModuleName) - 2)) // Too small for buffer.
			{
				snprintf(szModuleName, sizeof(szModuleName), "module@%p", m_ppModuleHandles[i]);
				pszModuleName = szModuleName;
			}
			else
			{
				pszModuleName = strrchr(szModuleName, '\\') + 1;
			}

			K32GetModuleInformation(hProcess, m_ppModuleHandles[i], &modInfo, sizeof(modInfo));

			m_Buffer.append(std::format("\t{:<15}: [{:016X}, {:016X}]\n",
				pszModuleName, uintptr_t(modInfo.lpBaseOfDll), (reinterpret_cast<uintptr_t>(modInfo.lpBaseOfDll) + modInfo.SizeOfImage)));
		}
	}

	m_Buffer.append("}\n");
}

//-----------------------------------------------------------------------------
// Purpose: formats the system information
//-----------------------------------------------------------------------------
void CCrashHandler::FormatSystemInfo()
{

	
}

//-----------------------------------------------------------------------------
// Purpose: formats the build information
//-----------------------------------------------------------------------------
void CCrashHandler::FormatBuildInfo()
{
}

//-----------------------------------------------------------------------------
// Purpose: formats the module, address and exception
//-----------------------------------------------------------------------------
void CCrashHandler::FormatExceptionAddress()
{
	FormatExceptionAddress(static_cast<LPCSTR>(m_pExceptionPointers->ExceptionRecord->ExceptionAddress));
}

//-----------------------------------------------------------------------------
// Purpose: formats the module, address and exception
// Input  : pExceptionAddress - 
//-----------------------------------------------------------------------------
void CCrashHandler::FormatExceptionAddress(const LPCSTR pExceptionAddress)
{
	HMODULE hCrashedModule;
	if (!GetModuleHandleExA(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT, pExceptionAddress, &hCrashedModule))
	{
		m_Buffer.append(std::format("\t!!!unknown-module!!!: {:016X}\n", uintptr_t(pExceptionAddress)));
		return;
	}

	const void* pModuleBase = reinterpret_cast<LPCSTR>(pExceptionAddress - reinterpret_cast<LPCSTR>(hCrashedModule));

	CHAR szCrashedModuleFullName[MAX_PATH];
	if (GetModuleFileNameExA(GetCurrentProcess(), hCrashedModule, szCrashedModuleFullName, sizeof(szCrashedModuleFullName)) - 1 > 0x1FE)
	{
		std::string s = std::format(
			"\tmodule@{:016X}: {:016X}\n", 
			uintptr_t(hCrashedModule), uintptr_t(pModuleBase));
		m_Buffer.append(s);
		return;
	}

	// NOTE: original implementation strips the extension as well, but we keep
	// this in as its useful for when additional modules are loaded that aren't
	// part of the OS or game
	const char* const szCrashedModuleName = strrchr(szCrashedModuleFullName, '\\') + 1;

	m_Buffer.append(std::format("\t{}: {:016X}\n", szCrashedModuleName, uintptr_t(pModuleBase)));
}

//-----------------------------------------------------------------------------
// Purpose: formats the exception code
//-----------------------------------------------------------------------------
void CCrashHandler::FormatExceptionCode()
{
	const DWORD nExceptionCode = m_pExceptionPointers->ExceptionRecord->ExceptionCode;

	if (nExceptionCode > EXCEPTION_IN_PAGE_ERROR)
	{
		//m_Buffer.append(std::format(ExceptionToString(), nExceptionCode));
	}
	else if (nExceptionCode >= EXCEPTION_ACCESS_VIOLATION)
	{
		const CHAR* pszException = "EXCEPTION_IN_PAGE_ERROR";

		if (nExceptionCode == EXCEPTION_ACCESS_VIOLATION)
		{
			pszException = "EXCEPTION_ACCESS_VIOLATION";
		}

		const ULONG_PTR uExceptionInfo0 = m_pExceptionPointers->ExceptionRecord->ExceptionInformation[0];
		const ULONG_PTR uExceptionInfo1 = m_pExceptionPointers->ExceptionRecord->ExceptionInformation[1];

		if (uExceptionInfo0)
		{
			if (uExceptionInfo0 == 1)
			{
				m_Buffer.append(std::format("\t{}(write): {:016X}\n", pszException, uExceptionInfo1));
			}
			else if (uExceptionInfo0 == 8)
			{
				m_Buffer.append(std::format("\t{}(execute): {:016X}\n", pszException, uExceptionInfo1));
			}
			else
			{
				m_Buffer.append(std::format("\t{}(unknown): {:016X}\n", pszException, uExceptionInfo1));
			}
		}
		else
		{
			m_Buffer.append(std::format("\t{}(read): {:016X}\n", pszException, uExceptionInfo1));
		}

		if (uExceptionInfo0 != 8)
		{
			if (IsPageAccessible())
			{
				FormatExceptionAddress();
			}
		}
	}
	else
	{
		//m_Buffer.append(std::format(ExceptionToString(), nExceptionCode));
	}
}

//-----------------------------------------------------------------------------
// Purpose: formats the arithmetic logic register and its content
// Input  : *pszRegister - 
//			nContent - 
//-----------------------------------------------------------------------------
void CCrashHandler::FormatALU(const char* const pszRegister, const DWORD64 nContent)
{
	if (nContent >= 1000000)
	{
		if (nContent > UINT_MAX)
		{
			// Print the full 64bits of the register.
			m_Buffer.append(std::format("\t{} = {:016X}\n", pszRegister, nContent));
		}
		else
		{
			m_Buffer.append(std::format("\t{} = {:08X}\n", pszRegister, nContent));
		}
	}
	else if (nContent >= 10)
	{
		// Print as decimal with a hexadecimal comment.
		m_Buffer.append(std::format("\t{} = {:<6} // {:08X}\n", pszRegister, nContent, nContent));
	}
	else
	{
		// Print as decimal only.
		m_Buffer.append(std::format("\t{} = {:<10}\n", pszRegister, nContent));
	}
}

//-----------------------------------------------------------------------------
// Purpose: formats the floating point register and its content
// Input  : *pszRegister - 
//			*pxContent - 
//-----------------------------------------------------------------------------
void CCrashHandler::FormatFPU(const char* const pszRegister, const M128A* const pxContent)
{
	const DWORD nVec[4] =
	{
		static_cast<DWORD>(pxContent->Low & UINT_MAX),
		static_cast<DWORD>(pxContent->Low >> 32),
		static_cast<DWORD>(pxContent->High & UINT_MAX),
		static_cast<DWORD>(pxContent->High >> 32),
	};

	m_Buffer.append(std::format("\t{} = [ [{:.8g}, {:.8g}, {:.8g}, {:.8g}]", pszRegister,
		*reinterpret_cast<const FLOAT*>(&nVec[0]),
		*reinterpret_cast<const FLOAT*>(&nVec[1]),
		*reinterpret_cast<const FLOAT*>(&nVec[2]),
		*reinterpret_cast<const FLOAT*>(&nVec[3])));

	//const LONG nHighest = abs(LONG(*MaxElementABS(std::begin(nVec), std::end(nVec))));

	//if (nHighest >= 1000000)
	//{
	//	m_Buffer.AppendFormat(", [0x%08X, 0x%08X, 0x%08X, 0x%08X] ]\n",
	//		nVec[0], nVec[1], nVec[2], nVec[3]);
	//}
	//else
	{
		m_Buffer.append(std::format(", [{}, {}, {}, {}] ]\n",
			static_cast<LONG>(nVec[0]), static_cast<LONG>(nVec[1]),
			static_cast<LONG>(nVec[2]), static_cast<LONG>(nVec[3])));
	}
}

//-----------------------------------------------------------------------------
// Purpose: returns the current exception code as string
// Output : exception code, "UNKNOWN_EXCEPTION" if exception code doesn't exist in this context
//-----------------------------------------------------------------------------
const char* CCrashHandler::ExceptionToString(const DWORD nExceptionCode) const
{
	switch (nExceptionCode)
	{
	case EXCEPTION_BREAKPOINT: { return "\tEXCEPTION_BREAKPOINT"               ": {:08X}\n"; };
	case EXCEPTION_SINGLE_STEP: { return "\tEXCEPTION_SINGLE_STEP"              ": {:08X}\n"; };
	case EXCEPTION_ACCESS_VIOLATION: { return "\tEXCEPTION_ACCESS_VIOLATION"         ": {:08X}\n"; };
	case EXCEPTION_IN_PAGE_ERROR: { return "\tEXCEPTION_IN_PAGE_ERROR"            ": {:08X}\n"; };
	case EXCEPTION_ARRAY_BOUNDS_EXCEEDED: { return "\tEXCEPTION_ARRAY_BOUNDS_EXCEEDED"    ": {:08X}\n"; };
	case EXCEPTION_ILLEGAL_INSTRUCTION: { return "\tEXCEPTION_ILLEGAL_INSTRUCTION"      ": {:08X}\n"; };
	case EXCEPTION_INVALID_DISPOSITION: { return "\tEXCEPTION_INVALID_DISPOSITION"      ": {:08X}\n"; };
	case EXCEPTION_NONCONTINUABLE_EXCEPTION: { return "\tEXCEPTION_NONCONTINUABLE_EXCEPTION" ": {:08X}\n"; };
	case EXCEPTION_PRIV_INSTRUCTION: { return "\tEXCEPTION_PRIV_INSTRUCTION"         ": {:08X}\n"; };
	case EXCEPTION_STACK_OVERFLOW: { return "\tEXCEPTION_STACK_OVERFLOW"           ": {:08X}\n"; };
	case EXCEPTION_DATATYPE_MISALIGNMENT: { return "\tEXCEPTION_DATATYPE_MISALIGNMENT"    ": {:08X}\n"; };
	case EXCEPTION_FLT_DENORMAL_OPERAND: { return "\tEXCEPTION_FLT_DENORMAL_OPERAND"     ": {:08X}\n"; };
	case EXCEPTION_FLT_DIVIDE_BY_ZERO: { return "\tEXCEPTION_FLT_DIVIDE_BY_ZERO"       ": {:08X}\n"; };
	case EXCEPTION_FLT_INEXACT_RESULT: { return "\tEXCEPTION_FLT_INEXACT_RESULT"       ": {:08X}\n"; };
	case EXCEPTION_FLT_INVALID_OPERATION: { return "\tEXCEPTION_FLT_INVALID_OPERATION"    ": {:08X}\n"; };
	case EXCEPTION_FLT_OVERFLOW: { return "\tEXCEPTION_FLT_OVERFLOW"             ": {:08X}\n"; };
	case EXCEPTION_FLT_STACK_CHECK: { return "\tEXCEPTION_FLT_STACK_CHECK"          ": {:08X}\n"; };
	case EXCEPTION_FLT_UNDERFLOW: { return "\tEXCEPTION_FLT_UNDERFLOW"            ": {:08X}\n"; };
	case EXCEPTION_INT_DIVIDE_BY_ZERO: { return "\tEXCEPTION_INT_DIVIDE_BY_ZERO"       ": {:08X}\n"; };
	case EXCEPTION_INT_OVERFLOW: { return "\tEXCEPTION_INT_OVERFLOW"             ": {:08X}\n"; };
	default: { return "\tUNKNOWN_EXCEPTION"                  ": {:08X}\n"; };
	}
}

//-----------------------------------------------------------------------------
// Purpose: returns the current exception code as string
//-----------------------------------------------------------------------------
const char* CCrashHandler::ExceptionToString() const
{
	return ExceptionToString(m_pExceptionPointers->ExceptionRecord->ExceptionCode);
}

//-----------------------------------------------------------------------------
// Purpose: tests if memory page is accessible
// Output : true if accessible, false otherwise
//-----------------------------------------------------------------------------
bool CCrashHandler::IsPageAccessible() const
{
	const PCONTEXT pContextRecord = m_pExceptionPointers->ContextRecord;
	MEMORY_BASIC_INFORMATION mbi = { 0 };

	const SIZE_T t = VirtualQuery((LPCVOID)pContextRecord->Rsp, &mbi, sizeof(LPCVOID));
	if (t < sizeof(mbi) || (mbi.Protect & PAGE_NOACCESS) || !((mbi.Protect & PAGE_NOACCESS) | PAGE_READWRITE))
	{
		return false;
	}
	else
	{
		return !(mbi.State & MEM_COMMIT);
	}
}

//-----------------------------------------------------------------------------
// Purpose: captures the callstack
//-----------------------------------------------------------------------------
void CCrashHandler::CaptureCallStack()
{
	m_nCapturedFrames = RtlCaptureStackBackTrace(2, NUM_FRAMES_TO_CAPTURE, m_ppStackTrace, NULL);
}

//-----------------------------------------------------------------------------
// Purpose: writes the stack trace and minidump to the disk
//-----------------------------------------------------------------------------
void CCrashHandler::WriteFile()
{
	std::string outFile;

	CreateDirectories("latest_crash");

	outFile = std::format("{}/{}.txt", "latest_crash", "rsx_crash");
	HANDLE hTxtFile = CreateFile(outFile.c_str(), GENERIC_WRITE, 0, 0, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);

	if (hTxtFile != INVALID_HANDLE_VALUE)
	{
		::WriteFile(hTxtFile, m_Buffer.c_str(), (DWORD)m_Buffer.length(), NULL, NULL);
		CloseHandle(hTxtFile);
	}

	//outFile = std::format("{}/{}.dmp", "latest_crash", "minidump");
	//HANDLE hDmpFile = CreateFile(outFile.c_str(), GENERIC_WRITE, 0, 0, CREATE_NEW, FILE_ATTRIBUTE_NORMAL, 0);

	//if (hDmpFile != INVALID_HANDLE_VALUE)
	//{
	//	MINIDUMP_EXCEPTION_INFORMATION dumpExceptionInfo;
	//	dumpExceptionInfo.ThreadId = GetCurrentThreadId();
	//	dumpExceptionInfo.ExceptionPointers = m_pExceptionPointers;
	//	dumpExceptionInfo.ClientPointers = false;

	//	MiniDumpWriteDump(
	//		GetCurrentProcess(),
	//		GetCurrentProcessId(),
	//		hDmpFile, MiniDumpNormal,
	//		&dumpExceptionInfo, NULL, NULL);

	//	CloseHandle(hDmpFile);
	//}
}

//-----------------------------------------------------------------------------
// Purpose: creates the crashmsg process displaying the error to the user
// the process has to be separate as the current process is getting killed
//-----------------------------------------------------------------------------
void CCrashHandler::CreateMessageProcess()
{
	
}

//-----------------------------------------------------------------------------
// Purpose: calls the crash callback
//-----------------------------------------------------------------------------
void CCrashHandler::CrashCallback()
{
	if (m_pCrashCallback)
	{
		((void(*)(void))m_pCrashCallback)();
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : 
// Output : 
//-----------------------------------------------------------------------------
long __stdcall BottomLevelExceptionFilter(EXCEPTION_POINTERS* const pExceptionInfo)
{
	g_CrashHandler.Start();

	// If the exception couldn't be handled, run the crash callback and
	// terminate the process
	if (g_CrashHandler.GetExit())
	{
		g_CrashHandler.CrashCallback();
		ExitProcess(EXIT_FAILURE);
	}

	g_CrashHandler.Reset();
	g_CrashHandler.SetExceptionPointers(pExceptionInfo);

	// Let the higher level exception handlers deal with this particular
	// exception.
	if (g_CrashHandler.ExceptionToString() == g_CrashHandler.ExceptionToString(0xFFFFFFFF))
	{
		g_CrashHandler.End();
		return EXCEPTION_CONTINUE_SEARCH;
	}

	// Don't run when a debugger is present.
	if (IsDebuggerPresent())
	{
		g_CrashHandler.End();
		return EXCEPTION_CONTINUE_SEARCH;
	}

	g_CrashHandler.SetExit(true);

	g_CrashHandler.CaptureCallStack();

	g_CrashHandler.FormatCrash();
	g_CrashHandler.FormatCallstack();
	g_CrashHandler.FormatRegisters();
	g_CrashHandler.FormatModules();

	g_CrashHandler.WriteFile();

	// End it here, the next recursive call terminates the process.
	g_CrashHandler.End();

	return EXCEPTION_EXECUTE_HANDLER;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CCrashHandler::Init()
{
	InitializeSRWLock(&m_Lock);
	m_hExceptionHandler = AddVectoredExceptionHandler(TRUE, BottomLevelExceptionFilter);
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CCrashHandler::Shutdown()
{
	if (m_hExceptionHandler)
	{
		RemoveVectoredExceptionHandler(m_hExceptionHandler);
		m_hExceptionHandler = nullptr;
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CCrashHandler::Reset()
{

}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CCrashHandler::CCrashHandler()
	: m_ppStackTrace()
	, m_nCapturedFrames(0)
	, m_ppModuleHandles()
	, m_pCrashCallback(nullptr)
	, m_hExceptionHandler(nullptr)
	, m_pExceptionPointers(nullptr)
	, m_bExit(false)
	, m_bMessageCreated(false)
{
	Init();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CCrashHandler::~CCrashHandler()
{
	Shutdown();
}

CCrashHandler g_CrashHandler;
#endif
