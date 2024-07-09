/*
 * Copyright 2022-2024 openDAQ d. o. o.
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
 * @file    thread_ex.h
 * @author  Tomaž Kos
 * @date    16/01/2018
 * @version 1.0
 *
 * @brief ThreadEx is an abstract class that enables creation of separate threads of execution in an application.
 *
 * @section DESCRIPTION
 *
 * Create a descendant of ThreadEx to represent an execution thread in a multi-threaded application. Each new instance of a TThread
 * descendant is a new thread of execution. Multiple instances of a TThread derived class make an application multi-threaded.
 *
 * Following are issues and recommendations to be aware of when using threads:
 *  - Keeping track of too many threads consumes CPU time
 *  - When multiple threads update the same resources, they must be synchronized to avoid conflicts.
 *  - Most methods that access an object and update a form must only be called from within the main thread or use a synchronization object.
 *
 * Define the thread object's Execute method by inserting the code that should execute when the thread is executed.
 */

#pragma once

#include <opendaq/utils/utils.h>
#include <thread>
#include <atomic>
#include <mutex>

BEGIN_NAMESPACE_UTILS

enum class ThreadExPriority : int
{
    idle = 0,
    lowest = 1,
    low = 2,
    normal = 3,
    high = 4,
    highest = 5,
    timeCritical = 6
};

/**
 * @brief ThreadEx class for working with threads.
 */
class ThreadEx
{
public:
    ThreadEx();
    virtual ~ThreadEx();

    /**
     * @brief Starts the execution of a thread.
     */
    virtual void start();

    /**
     * @brief Terminate and wait for the thread to terminate.
     */
    virtual void stop();

    /**
     * @brief Terminate sets the thread's Terminated property to true, signaling that the thread should be terminated as soon as possible.
     */
    void terminate();

    /**
     * @brief Waits for the thread to terminate.
     */
    void waitFor();

    /**
     * @brief Get thread's scheduling priority relative to other threads in the process.
     * @return Current ThreadExPriority value
     */
    const ThreadExPriority& getPriority() const;

    /**
     * @brief Set thread's scheduling priority relative to other threads in the process.
     * @param priority a new priority value.
     */
    void setPriority(ThreadExPriority priority);

    /**
     * @brief Get thread's locking object.
     * @return std::recursive_mutex& locking thread object
     */
    std::recursive_mutex& getLock();

    /**
     * @brief Returns thread's terminated signal.
     * @return Terminated signal.
     */
    const std::atomic<bool>& getTerminated() const;

    /**
     * @brief Returns true if thread has already been finished.
     * @return Finished status.
     */
    const std::atomic<bool>& getFinished() const;

    /**
     * @brief Returns true if thread is started.
     * @return Thread is started.
     */
    bool getStarted() const;

    /**
     * @brief Returns thread id
     */
    const std::thread::id getThreadId() const;

protected:
    mutable std::recursive_mutex lock;
    std::atomic<bool> terminated;
    std::atomic<bool> finished;

    /**
     * @brief Provides an abstract or pure virtual method to contain the code which executes when the thread is run.
     */
    virtual void execute() = 0;

    /**
     * @brief Set current thread name (you must call it from the thread)
     */
    void setThreadName(const char* name);

private:
    std::unique_ptr<std::thread> currentThread;
    ThreadExPriority priority;
    void mainThread();
};

END_NAMESPACE_UTILS
