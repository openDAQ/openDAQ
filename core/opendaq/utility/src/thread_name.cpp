#include <opendaq/thread_name.h>

#if defined(_WIN32) && !defined(__WINPTHREADS_VERSION)
#include <Windows.h>
#endif

#define DELPHI_WORKAROUND  // delphi recognises only naming defined from exception

#if !defined DELPHI_WORKAROUND
#if defined _WIN32 || defined __CYGWIN__
extern "C" typedef HRESULT(WINAPI* t_SetThreadDescription)(HANDLE, PCWSTR);
extern "C" typedef HRESULT(WINAPI* t_GetThreadDescription)(HANDLE, PWSTR*);
#endif
#endif

#if defined _GNU_SOURCE && !defined __EMSCRIPTEN__ && !defined __CYGWIN__
#include <pthread.h>
#endif

extern "C" PUBLIC_EXPORT void daqNameThread(const char* name)
{
#if defined _WIN32 || defined __CYGWIN__
#if !defined DELPHI_WORKAROUND
    static auto _SetThreadDescription = (t_SetThreadDescription) GetProcAddress(GetModuleHandleA("kernel32.dll"), "SetThreadDescription");
    if (_SetThreadDescription)
    {
        wchar_t buf[256];
        mbstowcs(buf, name, 256);
        _SetThreadDescription(GetCurrentThread(), buf);
    }
    else
    {
#endif
#if defined _MSC_VER
        const DWORD MS_VC_EXCEPTION = 0x406D1388;
#pragma pack(push, 8)
        struct THREADNAME_INFO
        {
            DWORD dwType;
            LPCSTR szName;
            DWORD dwThreadID;
            DWORD dwFlags;
        };
#pragma pack(pop)

        DWORD ThreadId = GetCurrentThreadId();
        THREADNAME_INFO info;
        info.dwType = 0x1000;
        info.szName = name;
        info.dwThreadID = ThreadId;
        info.dwFlags = 0;

        __try
        {
            RaiseException(MS_VC_EXCEPTION, 0, sizeof(info) / sizeof(ULONG_PTR), (ULONG_PTR*) &info);
        }
        __except (EXCEPTION_EXECUTE_HANDLER)
        {
        }
#endif
#if !defined DELPHI_WORKAROUND
    }
#endif
#elif defined _GNU_SOURCE && !defined __EMSCRIPTEN__ && !defined __CYGWIN__
    {
        const auto sz = ::strlen(name);
        if (sz <= 15)
        {
            pthread_setname_np(pthread_self(), name);
        }
        else
        {
            char buf[16];
            memcpy(buf, name, 15);
            buf[15] = '\0';
            pthread_setname_np(pthread_self(), buf);
        }
    }
#endif
}
