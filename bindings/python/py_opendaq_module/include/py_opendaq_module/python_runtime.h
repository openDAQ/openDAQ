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
#include <pybind11/embed.h>
#include <opendaq/scheduler_ptr.h>
#include <opendaq/work_factory.h>
#include <opendaq/logger_component_ptr.h>
#include <opendaq/custom_log.h>

#include <future>
#include <thread>
#include <type_traits>
#include <utility>

BEGIN_NAMESPACE_OPENDAQ

/*!
 * @brief The single dispatch thread that is allowed to touch the Python interpreter, plus -
 * when nothing has initialized Python yet - the embedded interpreter itself.
 *
 * openDAQ callbacks (module/function block virtuals) can be invoked from arbitrary native
 * threads (scheduler workers, streaming threads). Instead of acquiring the GIL ad-hoc on
 * whichever thread happens to call in - which would reintroduce a global lock on paths that
 * were deliberately made lock-free - every call into Python is marshaled through
 * IScheduler::scheduleWorkOnMainLoop() onto one dedicated thread that owns the interpreter.
 *
 * Two ways this ends up running:
 *  - Embedded: a C++ host with no Python of its own. The constructor calls Py_Initialize()
 *    (via pybind11::scoped_interpreter) on dispatchThread itself and imports the in-process
 *    "opendaq" module registered by python_opendaq_embed.cpp.
 *  - Attached: this code is itself linked into a Python extension module (e.g. the real
 *    opendaq.so), so Py_IsInitialized() is already true by the time PythonRuntime is first
 *    constructed - some other thread (the process's main thread) owns interpreter startup/
 *    shutdown. dispatchThread never touches Py_Initialize/Py_Finalize; it only acquires/
 *    releases the GIL for its own dispatched work, same as any other C thread calling into
 *    Python. Whatever pybind11 class registration (daqInterfaceIdToClass and friends) object
 *    marshaling needs must already be populated by that point - see the comment on
 *    python_opendaq_embed.cpp for why that means this code must link the exact same
 *    py_core_types/py_core_objects/py_opendaq objects the host's Python module does, not a
 *    separate copy.
 *
 *    Attached mode also relies on pybind11's own internals (detail::get_internals(), which
 *    every gil_scoped_acquire needs) already being bootstrapped by some thread with a stable,
 *    long-lived Python registration before dispatchThread ever touches Python. That bootstrap
 *    briefly acquires the GIL through its own throwaway RAII guard, and on a thread with no
 *    prior Python history (a bare std::thread's first-ever touch, which dispatchThread's would
 *    be) that guard's teardown leaves pybind11's internal thread-state pointer dangling,
 *    corrupting every gil_scoped_acquire after it. In the real host, `import opendaq` on the
 *    host's own main thread - which has exactly that kind of stable registration, from
 *    Py_Initialize() - already bootstraps this as a side effect of registering every wrapped
 *    type, long before this runtime's dispatch thread exists, so it is a non-issue there. It
 *    only bites a minimal test harness that constructs PythonRuntime as the first pybind11
 *    touch in the whole process - see test_python_runtime_attach.cpp for how that repros it and
 *    works around it by importing "opendaq" on the main thread first.
 *
 * The runtime is a deliberately leaked Meyer's singleton: it is created on first use and lives
 * until process exit. It is never finalized, so there is no shutdown ordering to get right
 * between it and the py::object handles held by PythonModule/PythonFunctionBlock instances.
 * openDAQ has no module-unload mechanism today; if one is added, this should become a
 * ref-counted runtime instead (weak_ptr singleton), at which point re-initializing the
 * interpreter after a full finalize needs to be validated against whatever plugin dependencies
 * (e.g. numpy) are in use, since CPython does not guarantee that is safe for every C extension.
 */
class PythonRuntime
{
public:
    /*!
     * @brief Returns the process-wide runtime, constructing it on first call.
     *
     * The calling thread must not already hold the GIL the first time this is ever called in a
     * process: construction blocks that thread until its new dispatch thread finishes its own
     * setup, which requires acquiring the GIL - something it can never do while the calling
     * thread is both holding it and blocked waiting. Unlike run() (which detects and handles
     * this automatically - see its own comment), that check can't help here: dispatchThread
     * doesn't exist yet, so there's no fn() to just run locally instead. In the attached case in
     * particular (see the class comment), the calling thread commonly does hold the GIL - e.g.
     * inside a pybind11 `.def()` lambda - so that caller is responsible for a
     * `gil_scoped_release` first, at least for whichever call ends up being the first ever
     * construction.
     */
    static PythonRuntime& instance();

    PythonRuntime(const PythonRuntime&) = delete;
    PythonRuntime& operator=(const PythonRuntime&) = delete;

    /*!
     * @brief Runs @p fn on the dispatch thread with the GIL held, blocks the calling thread
     * until it completes, and re-throws whatever @p fn threw (including Python exceptions,
     * which surface as pybind11::error_already_set - a std::exception) on the calling thread.
     *
     * This is the only way any code in this library may touch the Python interpreter - except
     * when the calling thread already holds the GIL, in which case fn() runs right here,
     * synchronously, instead. That's not an optimization, it's required for correctness: if the
     * caller already holds the GIL, dispatching to dispatchThread and blocking on the result
     * would deadlock, since dispatchThread could never acquire a GIL this thread is both
     * holding and blocked waiting to get back. This matters more than it might look like it
     * should: PythonModule/PythonFunctionBlockImpl's destructors call run() to drop their
     * py::object members, and destructors run wherever the *last* reference happens to be
     * dropped - which, for a host that's a plain Python process (attached mode), is routinely
     * from inside ordinary Python refcount-driven cleanup (a script's own variables going out
     * of scope, or interpreter shutdown), always with the GIL already held on whatever thread
     * that is.
     */
    template <typename Fn>
    auto run(Fn&& fn) -> std::invoke_result_t<Fn>
    {
        if (PyGILState_Check())
            return fn();

        using Result = std::invoke_result_t<Fn>;

        std::promise<Result> resultPromise;
        auto resultFuture = resultPromise.get_future();

        auto dispatch = [fn = std::forward<Fn>(fn), &resultPromise]() mutable
        {
            pybind11::gil_scoped_acquire gil;
            try
            {
                if constexpr (std::is_void_v<Result>)
                {
                    fn();
                    resultPromise.set_value();
                }
                else
                {
                    resultPromise.set_value(fn());
                }
            }
            catch (...)
            {
                resultPromise.set_exception(std::current_exception());
            }
        };

        dispatchScheduler.scheduleWorkOnMainLoop(Work(std::move(dispatch)));
        return resultFuture.get();
    }

    /*!
     * @brief Posts @p fn to run on the dispatch thread with the GIL held and returns
     * immediately - for notifications nobody is blocked waiting on (e.g. onPacketReceived).
     * Any exception @p fn lets escape is logged and dropped: there is no caller left to
     * propagate it to. Prefer catching and logging through the caller's own, more specific
     * loggerComponent instead of relying on this - it is a last-resort safety net so a bug here
     * cannot kill the dispatch thread.
     */
    template <typename Fn>
    void post(Fn&& fn)
    {
        auto dispatch = [fn = std::forward<Fn>(fn), this]() mutable
        {
            pybind11::gil_scoped_acquire gil;
            try
            {
                fn();
            }
            catch (const std::exception& e)
            {
                LOG_E("Unhandled exception from an async Python callback: {}", e.what())
            }
            catch (...)
            {
                LOG_E("Unhandled non-standard exception from an async Python callback")
            }
        };

        dispatchScheduler.scheduleWorkOnMainLoop(Work(std::move(dispatch)));
    }

    /*!
     * @brief Returns a process-unique module name to register a loaded plugin file under in
     * sys.modules, so two plugins with the same file basename cannot collide.
     * Must only be called from within a callable passed to run().
     */
    std::string nextPluginModuleName();

private:
    PythonRuntime();

    // Deliberately never touched by any thread other than dispatchThread - see the comment on
    // the constructor for why. Null in the attached case - some other thread owns the
    // interpreter's lifetime then, this runtime only ever borrows it.
    std::unique_ptr<pybind11::scoped_interpreter> interpreter;
    LoggerComponentPtr loggerComponent;
    SchedulerPtr dispatchScheduler;
    std::thread dispatchThread;
    size_t pluginCounter = 0;
};

END_NAMESPACE_OPENDAQ
