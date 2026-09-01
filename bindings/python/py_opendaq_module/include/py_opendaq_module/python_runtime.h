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
 * @brief Process-wide embedded Python interpreter plus the single dispatch thread that is
 * allowed to touch it.
 *
 * openDAQ callbacks (module/function block virtuals) can be invoked from arbitrary native
 * threads (scheduler workers, streaming threads). Instead of acquiring the GIL ad-hoc on
 * whichever thread happens to call in - which would reintroduce a global lock on paths that
 * were deliberately made lock-free - every call into Python is marshaled through
 * IScheduler::scheduleWorkOnMainLoop() onto one dedicated thread that owns the interpreter.
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
    static PythonRuntime& instance();

    PythonRuntime(const PythonRuntime&) = delete;
    PythonRuntime& operator=(const PythonRuntime&) = delete;

    /*!
     * @brief Runs @p fn on the dispatch thread with the GIL held, blocks the calling thread
     * until it completes, and re-throws whatever @p fn threw (including Python exceptions,
     * which surface as pybind11::error_already_set - a std::exception) on the calling thread.
     *
     * This is the only way any code in this library may touch the Python interpreter.
     */
    template <typename Fn>
    auto run(Fn&& fn) -> std::invoke_result_t<Fn>
    {
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
    // the constructor for why.
    std::unique_ptr<pybind11::scoped_interpreter> interpreter;
    LoggerComponentPtr loggerComponent;
    SchedulerPtr dispatchScheduler;
    std::thread dispatchThread;
    size_t pluginCounter = 0;
};

END_NAMESPACE_OPENDAQ
