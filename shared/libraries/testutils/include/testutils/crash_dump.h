/*
 * Copyright 2022-2026 openDAQ d.o.o.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#pragma once

// Writes a minidump and prints the stack to stderr when a test process hits an access violation, before gtest reports the exception,
// and prints the stack when the process aborts. Windows only; the dump goes to OPENDAQ_TEST_DUMP_DIR, else %LOCALAPPDATA%\CrashDumps,
// else the working directory.
// Header-only, since test binaries reach the listener through the header-only test utilities as well.

#if defined(_MSC_VER)

#ifndef WIN32_LEAN_AND_MEAN
    #define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
    #define NOMINMAX
#endif
#include <windows.h>
#include <dbghelp.h>
#include <atomic>
#include <csignal>
#include <cstdio>
#include <string>

#pragma comment(lib, "dbghelp.lib")

namespace daq::test_utils
{

inline std::wstring crashDumpDirectory()
{
    wchar_t buffer[MAX_PATH];
    DWORD len = GetEnvironmentVariableW(L"OPENDAQ_TEST_DUMP_DIR", buffer, MAX_PATH);
    if (len > 0 && len < MAX_PATH)
        return buffer;
    len = GetEnvironmentVariableW(L"LOCALAPPDATA", buffer, MAX_PATH);
    if (len > 0 && len < MAX_PATH)
        return std::wstring(buffer) + L"\\CrashDumps";
    return L".";
}

// The stack of the calling thread; in a vectored handler it runs through the exception dispatcher into the faulting frames
inline void printStackTrace(const char* reason)
{
    // DbgHelp is single-threaded
    static std::atomic_flag busy = ATOMIC_FLAG_INIT;
    while (busy.test_and_set())
        Sleep(1);

    const HANDLE process = GetCurrentProcess();
    static const bool symbolsLoaded = [process]
    {
        SymSetOptions(SYMOPT_UNDNAME | SYMOPT_LOAD_LINES | SYMOPT_DEFERRED_LOADS);
        return SymInitialize(process, nullptr, TRUE) != FALSE;
    }();

    void* frames[62];
    const USHORT count = RtlCaptureStackBackTrace(0, 62, frames, nullptr);
    fprintf(stderr, "%s on thread %lu, stack:\n", reason, GetCurrentThreadId());

    alignas(SYMBOL_INFO) char symbolBuffer[sizeof(SYMBOL_INFO) + 512];
    auto* symbol = reinterpret_cast<SYMBOL_INFO*>(symbolBuffer);
    for (USHORT i = 0; i < count; ++i)
    {
        const auto address = reinterpret_cast<DWORD64>(frames[i]);
        symbol->SizeOfStruct = sizeof(SYMBOL_INFO);
        symbol->MaxNameLen = 511;
        DWORD64 symbolOffset = 0;
        const char* name = symbolsLoaded && SymFromAddr(process, address, &symbolOffset, symbol) ? symbol->Name : "?";

        IMAGEHLP_LINE64 line{};
        line.SizeOfStruct = sizeof(line);
        DWORD lineOffset = 0;
        if (symbolsLoaded && SymGetLineFromAddr64(process, address, &lineOffset, &line))
            fprintf(stderr, "  #%02u %s (%s:%lu)\n", i, name, line.FileName, line.LineNumber);
        else
            fprintf(stderr, "  #%02u %s +0x%llx\n", i, name, static_cast<unsigned long long>(symbolOffset));
    }
    fflush(stderr);
    busy.clear();
}

inline void crashDumpAbortHandler(int)
{
    printStackTrace("Abort");
}

inline LONG CALLBACK crashDumpVectoredHandler(PEXCEPTION_POINTERS info)
{
    static std::atomic<int> dumpsWritten{0};

    // No stack overflow: the handler runs on the faulting thread and needs stack itself
    const DWORD code = info->ExceptionRecord->ExceptionCode;
    if (code != EXCEPTION_ACCESS_VIOLATION && code != EXCEPTION_ILLEGAL_INSTRUCTION &&
        code != EXCEPTION_ARRAY_BOUNDS_EXCEEDED && code != EXCEPTION_INT_DIVIDE_BY_ZERO)
        return EXCEPTION_CONTINUE_SEARCH;

    const int index = dumpsWritten++;
    if (index >= 5)
        return EXCEPTION_CONTINUE_SEARCH;

    char reason[96];
    snprintf(reason, sizeof(reason), "Exception 0x%08lX at %p", code, info->ExceptionRecord->ExceptionAddress);
    printStackTrace(reason);

    wchar_t exePath[MAX_PATH];
    GetModuleFileNameW(nullptr, exePath, MAX_PATH);
    std::wstring exeName = exePath;
    exeName = exeName.substr(exeName.find_last_of(L"\\/") + 1);

    const std::wstring directory = crashDumpDirectory();
    CreateDirectoryW(directory.c_str(), nullptr);

    const std::wstring fileName = directory + L"\\" + exeName + L"." + std::to_wstring(GetCurrentProcessId()) + L"." +
                                  std::to_wstring(index) + L".dmp";
    HANDLE file = CreateFileW(fileName.c_str(), GENERIC_WRITE, 0, nullptr, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
    if (file == INVALID_HANDLE_VALUE)
        return EXCEPTION_CONTINUE_SEARCH;

    MINIDUMP_EXCEPTION_INFORMATION exceptionInfo{GetCurrentThreadId(), info, FALSE};
    const auto type = static_cast<MINIDUMP_TYPE>(MiniDumpWithIndirectlyReferencedMemory | MiniDumpScanMemory | MiniDumpWithThreadInfo |
                                                 MiniDumpWithHandleData | MiniDumpWithUnloadedModules);
    MiniDumpWriteDump(GetCurrentProcess(), GetCurrentProcessId(), file, type, &exceptionInfo, nullptr, nullptr);
    CloseHandle(file);

    return EXCEPTION_CONTINUE_SEARCH;
}

inline void installCrashDumpHandler()
{
    static bool installed = false;
    if (!installed)
    {
        AddVectoredExceptionHandler(1, crashDumpVectoredHandler);
        // abort() runs the handler and then ends the process, without the Debug CRT message box that blocks an unattended run
        _set_abort_behavior(0, _WRITE_ABORT_MSG | _CALL_REPORTFAULT);
        signal(SIGABRT, crashDumpAbortHandler);
        installed = true;
    }
}

}

#else

namespace daq::test_utils
{

inline void installCrashDumpHandler()
{
}

}

#endif
