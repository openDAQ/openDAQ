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

#include <py_opendaq_module/python_runtime.h>

#include <opendaq/scheduler_factory.h>
#include <opendaq/logger_factory.h>

BEGIN_NAMESPACE_OPENDAQ

PythonRuntime& PythonRuntime::instance()
{
    // Intentionally leaked: never finalized, lives until process exit.
    // See the class comment for why.
    static PythonRuntime* runtime = new PythonRuntime();
    return *runtime;
}

PythonRuntime::PythonRuntime()
{
    // Decided once, before dispatchThread exists, from whichever thread constructs this
    // singleton first - process-global state, so it doesn't matter which thread asks.
    const bool ownsInterpreter = !Py_IsInitialized();

    // In the owning case, Py_Initialize() must run on dispatchThread itself, not on whatever
    // thread happens to construct PythonRuntime first. CPython's two GIL APIs don't mix safely
    // on the same OS thread: gil_scoped_release (PyEval_SaveThread) detaches the calling
    // thread's state without clearing its GILState bookkeeping, so if that *same* thread later
    // calls gil_scoped_acquire (PyGILState_Ensure) - which pybind11 does internally, e.g. inside
    // error_already_set::what() when a test or caller merely prints/logs an exception -
    // PyGILState_Ensure finds no properly-registered state for it and tries to create a new
    // one, which crashes. A thread that never calls Py_Initialize itself has no such stale
    // state, so its first gil_scoped_acquire works exactly like dispatchThread's always have.
    // Constructing the interpreter from dispatchThread's own entry point sidesteps the whole
    // problem: the only thread that ever touches the interpreter directly is the one thread
    // that created it, repeatedly acquiring/releasing the GIL for each dispatched work item,
    // which is the supported pattern.
    //
    // In the attached case, dispatchThread never calls Py_Initialize at all, so it has no such
    // implicit GIL-holding state to begin with: its first touch of the interpreter is a plain
    // gil_scoped_acquire (PyGILState_Ensure), exactly like any other C thread calling into an
    // already-running interpreter for the first time. That must be released the same way
    // (PyGILState_Release, by destroying the RAII object) rather than via gil_scoped_release
    // (PyEval_SaveThread) - mixing the two on a thread whose state came from PyGILState_Ensure
    // is the same bookkeeping corruption described above, just approached from the other side.
    //
    // In the owning case, python_opendaq_embed.cpp's PYBIND11_EMBEDDED_MODULE(opendaq, ...)
    // registration must already have run by the time import("opendaq") below executes - that
    // happens as a static-initializer side effect, well before this constructor, as long as
    // that translation unit was actually linked into the final binary. This code is shared with
    // attach-mode consumers (e.g. py_opendaq_daq) that must NOT link it (see the comment on
    // daq::py_opendaq_module_core's target in CMakeLists.txt), so this constructor can't force
    // that the way it once did (calling a keep-alive function defined there) - only an
    // owning-mode consumer would ever be able to resolve such a call, and an attach-mode one
    // would fail to link entirely. Ensuring python_opendaq_embed.cpp is actually linked in for
    // owning-mode consumers is instead handled at the link level - see py_opendaq_module's
    // target_link_options (force-loading the whole archive, since static linkers otherwise drop
    // an object file nothing references a symbol from, which is true of that entire file).
    std::promise<void> ready;
    auto readyFuture = ready.get_future();

    dispatchThread = std::thread(
        [this, &ready, ownsInterpreter]
        {
            // pyObjectToBaseObject()/baseObjectToPyObject() (used to marshal every ObjectPtr
            // crossing the C++/Python boundary) only work once the daq_core_types/daq_opendaq
            // pybind11 classes have been registered with this interpreter. Importing "opendaq"
            // triggers that registration the first time it runs, and is a harmless cache hit on
            // every import after: in the owning case it resolves to the in-process module
            // PYBIND11_EMBEDDED_MODULE(opendaq, ...) in python_opendaq_embed.cpp registers via
            // PyImport_AppendInittab() - unlike a real `import opendaq` resolved via sys.path,
            // that runs in this same process image, so it populates the same copy of
            // daqInterfaceIdToClass (and friends) that our own marshaling code reads. In the
            // attached case, "opendaq" is normally already in sys.modules by the time this runs
            // (the host's own entry point, e.g. the real opendaq.so, is what got the
            // interpreter running in the first place), so this just returns the cached module -
            // whichever code linked this translation unit into that host is responsible for it
            // being the exact same py_core_types/py_core_objects/py_opendaq objects the host's
            // own module uses, not a separate copy (see the class comment).
            if (ownsInterpreter)
            {
                interpreter = std::make_unique<pybind11::scoped_interpreter>();
                pybind11::module_::import("opendaq");

                const auto logger = Logger();
                loggerComponent = logger.getOrAddComponent("PythonRuntime");
                dispatchScheduler = SchedulerWithMainLoop(logger, 0);

                ready.set_value();

                // Release the GIL that scoped_interpreter's constructor left this thread
                // holding; each dispatched work item re-acquires it for just its own duration.
                pybind11::gil_scoped_release release;
                dispatchScheduler.runMainLoop();
            }
            else
            {
                // Attached: some other thread already owns interpreter startup/shutdown.
                {
                    pybind11::gil_scoped_acquire gil;

                    pybind11::module_::import("opendaq");

                    const auto logger = Logger();
                    loggerComponent = logger.getOrAddComponent("PythonRuntime");
                    dispatchScheduler = SchedulerWithMainLoop(logger, 0);
                }
                // gil released here (PyGILState_Release) - see the constructor comment for why
                // this must not be a gil_scoped_release instead.

                ready.set_value();
                dispatchScheduler.runMainLoop();
            }
        });

    readyFuture.wait();

    // Detached, not joined: this runtime is a deliberately leaked singleton (see the class
    // comment) that never asks dispatchThread to stop, so nothing ever joins it. Left joinable,
    // that's not just an unused capability - in the attached case specifically, a host process
    // that's a plain Python script hangs on exit (CPython's own interpreter shutdown blocks
    // waiting on it) unless it's explicitly detached first.
    dispatchThread.detach();
}

std::string PythonRuntime::nextPluginModuleName()
{
    return "opendaq_py_plugin_" + std::to_string(pluginCounter++);
}

END_NAMESPACE_OPENDAQ
