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

#include <pybind11/pybind11.h>

#include <atomic>
#include <optional>
#include <thread>

// Python is exiting once its atexit hook runs. The hook waits until all guards created before it are destroyed,
// so Python doesn't finalize while one exists. The finalizing thread can still create guards until finalization
// completes. Exiting is never reset, so if Python is initialized again in the same process, wrapped Python
// objects leak and Python callbacks fail.
class PyFinalization
{
public:
    // Converts to false if Python is exiting, except on the finalizing thread until finalization completes, or
    // on a thread that holds another guard which converts to true.
    // Unless created on the finalizing thread during finalization, holds Python finalization back while it exists.
    class Guard
    {
    public:
        Guard();
        ~Guard();

        Guard(const Guard&) = delete;
        Guard& operator=(const Guard&) = delete;

        explicit operator bool() const;

    private:
        bool pyAlive;
    };

    void registerAtExit();

private:
    bool enter();
    void leave();

    std::atomic<bool> exiting = false;
    std::atomic<bool> finalized = false;
    std::atomic<int> guardCount = 0;
    std::thread::id finalizingThread;
    // Guards on this thread that converted to true. While one exists, Python doesn't finalize.
    static thread_local int aliveGuards;
};

PyFinalization& pyFinalization();

// Takes the GIL unless Python is exiting, in which case it converts to false. Once Python finalizes, taking
// the GIL would hang forever, and deadlock the finalizing thread if that one joins this thread. Unless created
// on the finalizing thread during finalization, holds Python finalization back while it exists, so it must not
// wait for anything that waits for Python to exit.
class PyGilAcquireUnlessExiting
{
public:
    PyGilAcquireUnlessExiting();

    PyGilAcquireUnlessExiting(const PyGilAcquireUnlessExiting&) = delete;
    PyGilAcquireUnlessExiting& operator=(const PyGilAcquireUnlessExiting&) = delete;

    explicit operator bool() const;

private:
    PyFinalization::Guard guard;
    std::optional<pybind11::gil_scoped_acquire> gil;
};

#define PY_ACQUIRE_GIL_OR_RETURN()     \
    PyGilAcquireUnlessExiting _gil; \
    if (!_gil)                      \
        return DAQ_MAKE_ERROR_INFO(OPENDAQ_ERR_INVALIDSTATE, "Python is exiting")
