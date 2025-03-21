#include <opendaq/utils/thread_ex.h>
#include <opendaq/utils/thread_name.h>
#include <cassert>
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

void ThreadEx::setThreadName(const char* name)
{
    daq::utils::setThreadName(name);
}

const std::thread::id ThreadEx::getThreadId() const
{
    return currentThread->get_id();
}

END_NAMESPACE_UTILS
