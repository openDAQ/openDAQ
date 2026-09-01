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

// Defined in python_opendaq_embed.cpp - see the comment there for why calling this (rather than
// just linking that translation unit in) is necessary.
void keepPythonOpenDaqEmbedLinked();

PythonRuntime& PythonRuntime::instance()
{
    // Intentionally leaked: never finalized, lives until process exit.
    // See the class comment for why.
    static PythonRuntime* runtime = new PythonRuntime();
    return *runtime;
}

PythonRuntime::PythonRuntime()
{
    // Py_Initialize() must run on dispatchThread itself, not on whatever thread happens to
    // construct PythonRuntime first. CPython's two GIL APIs don't mix safely on the same OS
    // thread: gil_scoped_release (PyEval_SaveThread) detaches the calling thread's state
    // without clearing its GILState bookkeeping, so if that *same* thread later calls
    // gil_scoped_acquire (PyGILState_Ensure) - which pybind11 does internally, e.g. inside
    // error_already_set::what() when a test or caller merely prints/logs an exception -
    // PyGILState_Ensure finds no properly-registered state for it and tries to create a new
    // one, which crashes. A thread that never calls Py_Initialize itself has no such stale
    // state, so its first gil_scoped_acquire works exactly like dispatchThread's always have.
    // Constructing the interpreter from dispatchThread's own entry point sidesteps the whole
    // problem: the only thread that ever touches the interpreter directly is the one thread
    // that created it, repeatedly acquiring/releasing the GIL for each dispatched work item,
    // which is the supported pattern.
    keepPythonOpenDaqEmbedLinked();

    std::promise<void> ready;
    auto readyFuture = ready.get_future();

    dispatchThread = std::thread(
        [this, &ready]
        {
            interpreter = std::make_unique<pybind11::scoped_interpreter>();

            // pyObjectToBaseObject()/baseObjectToPyObject() (used to marshal every ObjectPtr
            // crossing the C++/Python boundary) only work once the daq_core_types/daq_opendaq
            // pybind11 classes have been registered with this interpreter. That registration is
            // done by the PYBIND11_EMBEDDED_MODULE(opendaq, ...) block in
            // python_opendaq_embed.cpp, which - unlike a real `import opendaq` resolved via
            // sys.path - runs in this same process image, so it populates the same copy of
            // daqInterfaceIdToClass (and friends) that our own marshaling code reads. Do not
            // swap this for loading the standalone opendaq.so: that library links its own
            // copies of the same static registration state, and object marshaling would
            // silently fall back to the generic IBaseObject wrapper for every interface.
            pybind11::module_::import("opendaq");

            const auto logger = Logger();
            loggerComponent = logger.getOrAddComponent("PythonRuntime");
            dispatchScheduler = SchedulerWithMainLoop(logger, 0);

            ready.set_value();

            // Release the GIL that scoped_interpreter's constructor left this thread holding;
            // each dispatched work item re-acquires it for just its own duration.
            pybind11::gil_scoped_release release;
            dispatchScheduler.runMainLoop();
        });

    readyFuture.wait();
}

std::string PythonRuntime::nextPluginModuleName()
{
    return "opendaq_py_plugin_" + std::to_string(pluginCounter++);
}

END_NAMESPACE_OPENDAQ
