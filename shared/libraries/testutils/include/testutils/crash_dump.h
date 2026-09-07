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

// Writes a minidump when a test process hits an access violation, before gtest reports the exception.
// Windows only; the dump goes to OPENDAQ_TEST_DUMP_DIR, else %LOCALAPPDATA%\CrashDumps, else the working directory.

#if defined(_MSC_VER)

#include <windows.h>
#include <dbghelp.h>
#include <atomic>
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

inline LONG CALLBACK crashDumpVectoredHandler(PEXCEPTION_POINTERS info)
{
    static std::atomic<int> dumpsWritten{0};

    const DWORD code = info->ExceptionRecord->ExceptionCode;
    if (code != EXCEPTION_ACCESS_VIOLATION && code != EXCEPTION_ILLEGAL_INSTRUCTION && code != EXCEPTION_STACK_OVERFLOW &&
        code != EXCEPTION_ARRAY_BOUNDS_EXCEEDED && code != EXCEPTION_INT_DIVIDE_BY_ZERO)
        return EXCEPTION_CONTINUE_SEARCH;

    const int index = dumpsWritten++;
    if (index >= 5)
        return EXCEPTION_CONTINUE_SEARCH;

    wchar_t exePath[MAX_PATH];
    GetModuleFileNameW(nullptr, exePath, MAX_PATH);
    std::wstring exeName = exePath;
    exeName = exeName.substr(exeName.find_last_of(L"\\/") + 1);

    const std::wstring fileName = crashDumpDirectory() + L"\\" + exeName + L"." + std::to_wstring(GetCurrentProcessId()) + L"." +
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
