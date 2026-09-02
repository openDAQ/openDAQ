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

// A separate binary from test_py_opendaq_module: PythonRuntime is a leaked Meyer's singleton,
// so whichever thread constructs it *first* in a process decides whether it owns the
// interpreter or attaches to one that already exists - every other test binary here calls
// PythonRuntime::instance() with no interpreter running yet, which always takes the "owning"
// branch. This binary calls Py_Initialize() itself, in main(), before anything ever touches
// PythonRuntime::instance(), specifically to exercise the "attached" branch - the shape
// PythonRuntime runs in once it is linked into a live Python process (e.g. the real opendaq.so)
// rather than embedded by a C++ host.

#include <gtest/gtest.h>
#include <Python.h>
#include <pybind11/pybind11.h>

#include <py_opendaq_module/python_module.h>
#include <py_opendaq_module/python_runtime.h>
#include <opendaq/context_factory.h>

using namespace daq;

TEST(PythonRuntimeAttachTest, DoesNotOwnAnAlreadyInitializedInterpreter)
{
    ASSERT_TRUE(Py_IsInitialized());
}

TEST(PythonRuntimeAttachTest, DispatchesWorkOntoTheExistingInterpreter)
{
    const int result = PythonRuntime::instance().run([]() { return 1 + 1; });
    ASSERT_EQ(result, 2);
}

TEST(PythonRuntimeAttachTest, LoadsAPluginFile)
{
    const auto module = createPythonModule(NullContext(), std::string(MOCK_MODULE_DIR) + "/mock_module.py");
    const auto info = module.getModuleInfo();

    ASSERT_EQ(info.getName(), "MockPythonModule");
    ASSERT_EQ(info.getId(), "MockPythonModuleId");

    const auto version = info.getVersionInfo();
    ASSERT_EQ(version.getMajor(), 1u);
    ASSERT_EQ(version.getMinor(), 2u);
    ASSERT_EQ(version.getPatch(), 3u);
}

int main(int argc, char** argv)
{
    // Must happen before *anything* touches PythonRuntime::instance() - see the file comment.
    Py_Initialize();

    // pybind11's own internals (detail::get_internals(), which every gil_scoped_acquire needs)
    // are lazily bootstrapped on whichever thread touches pybind11 first - and that bootstrap
    // briefly acquires the GIL itself, in a way that only survives being torn down again if the
    // thread doing it already has a *stable*, long-lived Python registration (which this main
    // thread has, from Py_Initialize() above; a freshly spun OS thread with no prior Python
    // history does not). In the real target scenario - linked into an extension module like the
    // real opendaq.so - this is a non-issue: the host's own `import opendaq` bootstraps
    // pybind11's internals on its main thread (registering every wrapped type) long before
    // PythonRuntime's dispatch thread exists. This import reproduces exactly that, so
    // PythonRuntime's dispatch thread never has to be the one to bootstrap it.
    pybind11::module_::import("opendaq");

    // Py_Initialize() leaves this thread holding the GIL. PythonRuntime's constructor blocks
    // its calling thread until its own new dispatch thread finishes acquiring the GIL for
    // setup - which it can never do while this thread keeps holding it, so without releasing
    // it here, the very first PythonRuntime::instance() call inside RUN_ALL_TESTS() below would
    // deadlock. This is the same requirement PythonRuntime::run() documents for every call
    // after construction too: whichever thread holds the GIL when it triggers PythonRuntime
    // machinery for the first time must give it up first - a real Python host's binding code is
    // expected to do this via gil_scoped_release before calling in.
    PyThreadState* mainThreadState = PyEval_SaveThread();

    ::testing::InitGoogleTest(&argc, argv);
    const int result = RUN_ALL_TESTS();

    PyEval_RestoreThread(mainThreadState);

    // Deliberately no Py_Finalize(): PythonRuntime's dispatch thread still owns interpreter
    // state it never gave up, and PythonRuntime itself is never finalized either (see its
    // class comment) - finalizing here would just race that thread during process teardown.
    return result;
}
