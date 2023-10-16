#include <opendaq/utils/timer_thread.h>
#include <stdexcept>
#include <chrono>
#include <stdexcept>
#include <utility>

#ifdef __linux__
    #include <pthread.h>
#else
    #include <algorithm>
#endif

BEGIN_NAMESPACE_UTILS

TimerThread::TimerThread(int intervalMs, CallbackFunction callback, int delayMs, TimerMode timerMode)
    : TimerThread(std::chrono::milliseconds(static_cast<int64_t>(intervalMs)),
                  std::move(callback),
                  delayMs == -1 ? std::chrono::milliseconds(static_cast<int64_t>(intervalMs))
                                : std::chrono::milliseconds(static_cast<int64_t>(delayMs)),
                  timerMode)
{
}

TimerThread::TimerThread(std::chrono::microseconds interval,
                         CallbackFunction callback,
                         std::optional<std::chrono::microseconds> delay,
                         TimerMode timerMode)
    : intervalMcs{interval}
    , delayMcs(delay.has_value() ? delay.value() : interval)
    , timerMode(timerMode)
    , callback{std::move(callback)}
{
#ifdef __linux__
    pthread_condattr_t cvAttr;
    pthread_condattr_init(&cvAttr);
    pthread_condattr_setclock(&cvAttr, CLOCK_MONOTONIC);

    auto nativeCvHandle = doTerminate.native_handle();
    pthread_cond_destroy(nativeCvHandle);
    pthread_cond_init(nativeCvHandle, &cvAttr);
#endif
}

TimerThread::~TimerThread()
{
    TimerThread::stop();
}

void TimerThread::start()
{
    if (getStarted())
        throw std::runtime_error("Thread is already started.");

    noOfCallbacks = 0;
    ThreadEx::start();
}

void TimerThread::stop()
{
    terminate();
    doTerminate.notify_one();

    ThreadEx::stop();
}

int TimerThread::getIntervalMs() const
{
    return static_cast<int>(std::chrono::duration_cast<std::chrono::milliseconds>(intervalMcs.load()).count());
}

void TimerThread::setIntervalMs(const int value)
{
    intervalMcs = std::chrono::milliseconds(value);
}

int TimerThread::getDelayMs() const
{
    return static_cast<int>(std::chrono::duration_cast<std::chrono::milliseconds>(delayMcs.load()).count());
}

void TimerThread::setDelayMs(const int ms)
{
    delayMcs = std::chrono::milliseconds(ms);
}

void TimerThread::setTimerMode(TimerMode mode)
{
    this->timerMode = mode;
}

TimerMode TimerThread::getTimerMode() const
{
    return timerMode;
}

TimerThread::CallbackFunction& TimerThread::getCallback()
{
    return callback;
}

void TimerThread::setCallback(const CallbackFunction& value)
{
    callback = value;
}

int64_t TimerThread::getNoOfCallbacks() const
{
    return noOfCallbacks;
}

void TimerThread::execute()
{
#ifndef __linux__
    auto locked = std::unique_lock<std::mutex>(terminateMutex);

    auto waitUntil = std::chrono::steady_clock::now() + delayMcs.load();

    while (!terminated)
    {
        auto result = doTerminate.wait_until(locked, waitUntil);
        if (result == std::cv_status::timeout)
        {
            ++noOfCallbacks;

            executeTimerCallback();

            if (timerMode == TimerMode::FixedDelay)
                waitUntil = std::chrono::steady_clock::now();
            waitUntil += intervalMcs.load();

            waitUntil = std::max(std::chrono::steady_clock::now(), waitUntil);
        }
    }
#else
    const auto timespecNormalise = [](timespec& ts) {
        ts.tv_sec += ts.tv_nsec / 1'000'000'000;
        ts.tv_nsec %= 1'000'000'000;
    };

    const auto intervalToTimespec = [] (const std::chrono::microseconds& interval) {
        const auto waitPeriodSeconds = std::chrono::duration_cast<std::chrono::seconds>(interval);
        const auto waitPeriodNs = std::chrono::duration_cast<std::chrono::nanoseconds>(interval - waitPeriodSeconds);
        return timespec {
            static_cast<int32_t>(waitPeriodSeconds.count()),
            static_cast<int32_t>(waitPeriodNs.count())
        };
    };

    const auto timespecAdd = [&](timespec& lhs, const timespec& rhs) {
        lhs.tv_sec += rhs.tv_sec;
        lhs.tv_nsec += rhs.tv_nsec;
        timespecNormalise(lhs);
    };

    const timespec delayTs = intervalToTimespec(delayMcs.load());
    const timespec waitTs = intervalToTimespec(intervalMcs.load());

    timespec ts{};
    clock_gettime(CLOCK_MONOTONIC, &ts);
    timespecAdd(ts, delayTs);

    pthread_mutex_lock(terminateMutex.native_handle());
    while (!terminated)
    {
        auto result = pthread_cond_timedwait(doTerminate.native_handle(), terminateMutex.native_handle(), &ts);
        if (result == ETIMEDOUT)
        {
            ++noOfCallbacks;

            executeTimerCallback();

            if (timerMode == TimerMode::FixedDelay)
            {
                clock_gettime(CLOCK_MONOTONIC, &ts);
            }

            timespecAdd(ts, waitTs);
        }
    }
    pthread_mutex_unlock(terminateMutex.native_handle());
#endif
}

void TimerThread::executeTimerCallback()
{
    if (callback)
        callback();
}

END_NAMESPACE_UTILS
