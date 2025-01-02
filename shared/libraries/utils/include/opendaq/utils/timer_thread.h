/*
 * Copyright 2022-2025 openDAQ d.o.o.
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

/**
 * @file    timer_thread.h
 * @author  Tomaž Kos
 * @date    16/01/2018
 * @version 1.0
 *
 * @brief Provides a mechanism for executing a method on a thread pool thread at specified intervals.
 */

#pragma once

#include <opendaq/utils/thread_ex.h>
#include <atomic>
#include <mutex>
#include <chrono>
#include <functional>
#include <condition_variable>
#include <optional>

BEGIN_NAMESPACE_UTILS

enum class TimerMode : int
{
    FixedDelay = 0,
    FixedRate = 1
};

/**
 * @brief Provides a mechanism for executing a method on a thread pool thread at specified intervals.
 */
class TimerThread : public ThreadEx
{
public:
    using CallbackFunction = std::function<void(void)>;

    /**
     * @brief Initializes a new instance of the TimerThread class with default 1000 milliseconds period.
     * @param intervalMs a interval in milliseconds.
     * @param callback a callback function you want the TimerThread to execute.
     * @param delayMs start delay (amount of time before first execution) in milliseconds. If negative, then the interval time is used for start delay.
     * @param timerMode a timer mode.
     */
    explicit TimerThread(int intervalMs = 1000,
                         CallbackFunction callback = nullptr,
                         int delayMs = -1,
                         TimerMode timerMode = TimerMode::FixedDelay);

    /**
     * @brief Initializes a new instance of the TimerThread class.
     * @param interval a interval in microseconds.
     * @param callback a callback function you want the TimerThread to execute.
     * @param delay start delay (amount of time before first execution) in microseconds. If empty, then the interval time is used for start delay.
     * @param timerMode a timer mode.
     */
    explicit TimerThread(std::chrono::microseconds interval,
                         CallbackFunction callback = nullptr,
                         std::optional<std::chrono::microseconds> delay = std::nullopt,
                         TimerMode timerMode = TimerMode::FixedDelay);

    ~TimerThread() override;

    /**
     * @brief Starts thread.
     */
    void start() override;

    /**
     * @brief Stops thread.
     */
    void stop() override;

    /**
     * @brief Get the amount of time in milliseconds.
     * @return Interval time in milliseconds.
     */
    int getIntervalMs() const;

    /**
     * @brief Set the amount of time in milliseconds.
     * @param value a interval in milliseconds.
     */
    void setIntervalMs(const int value);

    /**
     * @brief Get start delay (amount of time until first execution).
     * @return Delay time in milliseconds.
     */
    int getDelayMs() const;

    /**
     * @brief Set start delay (the amount of time until first execution).
     * @param ms a interval in milliseconds.
     */
    void setDelayMs(int ms);

    /**
     * @brief Set timer mode.
     * @param mode timer mode
     */
    void setTimerMode(TimerMode mode);

    /**
     * @brief Get timer mode. Default timer mode is TimerMode::FixedDelay.
     * @return Timer mode
     */
    TimerMode getTimerMode() const;

    /**
     * @brief Get callback function.
     * @return Callback function.
     */
    CallbackFunction& getCallback();

    /**
     * @brief Set callback function.
     * @param value A function that will be called every interval.
     */
    void setCallback(const CallbackFunction& value);

    /**
     * @brief Function returns how many times Execute function was called.
     * @return number of calls
     */
    std::int64_t getNoOfCallbacks() const;

protected:
    void execute() override;
    virtual void executeTimerCallback();

private:
    std::mutex terminateMutex;
    std::condition_variable doTerminate;
    std::atomic<int64_t> noOfCallbacks{0};
    std::atomic<std::chrono::microseconds> intervalMcs;
    std::atomic<std::chrono::microseconds> delayMcs;
    TimerMode timerMode;
    CallbackFunction callback;
};

END_NAMESPACE_UTILS
