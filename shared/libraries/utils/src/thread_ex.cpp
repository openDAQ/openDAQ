#include <opendaq/utils/thread_ex.h>
#include <cassert>
#include <cstring>
#include <stdexcept>

#if defined(_WIN32) && !defined(__WINPTHREADS_VERSION)
    #include <Windows.h>
#endif

using namespace std;

BEGIN_NAMESPACE_UTILS

ThreadEx::ThreadEx()
    : terminated{}
    , finished{}
    , priority{ThreadExPriority::normal}
{
}

ThreadEx::~ThreadEx()
{
    assert(!currentThread);
}

void ThreadEx::start()
{
    if (currentThread)
        throw std::runtime_error("Thread is already started.");

    finished = false;
    terminated = false;
    currentThread = std::make_unique<thread>(&ThreadEx::mainThread, this);

#if defined(_WIN32) && !defined(__WINPTHREADS_VERSION)
    const int winThreadPriorities[7] = {THREAD_PRIORITY_IDLE,
                                        THREAD_PRIORITY_LOWEST,
                                        THREAD_PRIORITY_BELOW_NORMAL,
                                        THREAD_PRIORITY_NORMAL,
                                        THREAD_PRIORITY_ABOVE_NORMAL,
                                        THREAD_PRIORITY_HIGHEST,
                                        THREAD_PRIORITY_TIME_CRITICAL};
    SetThreadPriority(currentThread->native_handle(), winThreadPriorities[static_cast<int>(priority)]);
#else
    // TODO: not implemented for linux
    #pragma message("Thread priorities are not implemented for linux")
#endif
}

void ThreadEx::stop()
{
    terminate();
    waitFor();
}

void ThreadEx::terminate()
{
    terminated = true;
}

void ThreadEx::waitFor()
{
    if (currentThread)
    {
        if (currentThread->joinable())
            currentThread->join();
        currentThread.reset();
    }
}

const ThreadExPriority& ThreadEx::getPriority() const
{
    return priority;
}

void ThreadEx::setPriority(const ThreadExPriority priority)
{
    this->priority = priority;
}

std::recursive_mutex& ThreadEx::getLock()
{
    return lock;
}

const std::atomic<bool>& ThreadEx::getTerminated() const
{
    return terminated;
}

const std::atomic<bool>& ThreadEx::getFinished() const
{
    return finished;
}

bool ThreadEx::getStarted() const
{
    return (bool) currentThread;
}

void ThreadEx::mainThread()
{
    execute();
    finished = true;
}

#define DELPHI_WORKAROUND  //delphi recognises only naming defined from exception

#if !defined DELPHI_WORKAROUND
#if defined _WIN32 || defined __CYGWIN__
extern "C" typedef HRESULT(WINAPI *t_SetThreadDescription)(HANDLE, PCWSTR);
extern "C" typedef HRESULT(WINAPI *t_GetThreadDescription)(HANDLE, PWSTR*);
#endif
#endif

//https://github.com/wolfpld/tracy/blob/2a61c0a45fbd682bf225f0373d8a373a0b631cff/common/TracySystem.cpp
void ThreadEx::setThreadName(const char* name)
{
#if defined _WIN32 || defined __CYGWIN__
#   if !defined DELPHI_WORKAROUND
    static auto _SetThreadDescription = (t_SetThreadDescription)GetProcAddress(GetModuleHandleA("kernel32.dll"), "SetThreadDescription");
    if (_SetThreadDescription)
    {
        wchar_t buf[256];
        mbstowcs(buf, name, 256);
        _SetThreadDescription(GetCurrentThread(), buf);
    }
    else
    {
#   endif
#  if defined _MSC_VER
        const DWORD MS_VC_EXCEPTION = 0x406D1388;
#    pragma pack( push, 8 )
        struct THREADNAME_INFO
        {
            DWORD dwType;
            LPCSTR szName;
            DWORD dwThreadID;
            DWORD dwFlags;
        };
#    pragma pack(pop)

        DWORD ThreadId = GetCurrentThreadId();
        THREADNAME_INFO info;
        info.dwType = 0x1000;
        info.szName = name;
        info.dwThreadID = ThreadId;
        info.dwFlags = 0;

        __try
        {
            RaiseException(MS_VC_EXCEPTION, 0, sizeof(info) / sizeof(ULONG_PTR), (ULONG_PTR*)&info);
        }
        __except (EXCEPTION_EXECUTE_HANDLER)
        {
        }
#  endif
#  if !defined DELPHI_WORKAROUND
    }
#  endif 
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

const std::thread::id ThreadEx::getThreadId() const
{
    return currentThread->get_id();
}

END_NAMESPACE_UTILS
