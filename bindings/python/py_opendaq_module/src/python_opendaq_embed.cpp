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

#include <pybind11/embed.h>
#include <pybind11/eval.h>

#include <py_core_types/py_core_types.h>
#include <py_core_objects/py_core_objects.h>
#include <py_opendaq/py_opendaq.h>
#include <py_core_types/py_opendaq_daq.h>
#include <py_opendaq_module/plugin_base_py.h>

namespace py = pybind11;

/*!
 * Registers an "opendaq" module directly into this process's own module init table (via
 * PYBIND11_EMBEDDED_MODULE, which calls PyImport_AppendInittab() at static-init time, well
 * before PythonRuntime ever calls Py_Initialize()).
 *
 * This is deliberate, not a shortcut: `daqInterfaceIdToClass` and the pybind11 class objects
 * that wrapInterface<T>() registers are plain global/static state. The real `opendaq.so`
 * (py_opendaq_daq, built for standalone Python use) links the exact same py_core_types/
 * py_core_objects/py_opendaq *static* libraries we do here - so if our embedded interpreter
 * imported that .so via a dlopen (the normal `import opendaq` path, resolved through sys.path),
 * it would end up with its own separate copy of all that global state, disjoint from the one
 * our own C++ code (baseObjectToPyObject/pyObjectToBaseObject) reads. Building our own
 * in-process "opendaq" module from the same wrap*() functions - the same ones opendaq_daq.cpp
 * calls - keeps everything in one address space, one copy of the registration state, and
 * incidentally means PythonRuntime never needs to locate an installed opendaq.so on sys.path.
 *
 * Deliberately narrower than the real py_opendaq_daq.cpp: no numpy buffer protocol support
 * (PyBuffer::Buffer::wrap) and no async event queue (declarePyEventQueue/definePyEventQueue) -
 * neither is needed for object marshaling, and both can be added here later if a plugin needs
 * them.
 */
PYBIND11_EMBEDDED_MODULE(opendaq, m)
{
    m.doc() = "openDAQ python bindings (embedded)";

    opendaq_daq_module = m;
    python_class_fraction = py::module_::import("fractions").attr("Fraction");

    wrapDaqComponentCoreTypes(m);
    wrapDaqComponentCoreObjects(m);
    wrapDaqComponentOpenDaq(m);

    // Same Module/FunctionBlock base classes the real pip package exposes (see
    // bindings/python/package/opendaq/{module,function_block}.py) - a plugin subclasses these
    // through duck typing, never through openDAQ's C++ headers, so they have to exist as plain
    // Python here too. Source embedded verbatim at build time - see CMakeLists.txt.
    py::exec(OPENDAQ_MODULE_PY, m.attr("__dict__"));
    py::exec(OPENDAQ_FUNCTION_BLOCK_PY, m.attr("__dict__"));
}

// PYBIND11_EMBEDDED_MODULE's registration above is a side effect of a global object's
// constructor - nothing else in this file is ever called, so nothing naturally creates an
// unresolved symbol reference pulling this .o file in from its static archive. Making sure it's
// linked into any embedding host anyway is handled at the link level instead - see
// py_opendaq_module's target_link_options in CMakeLists.txt (force-loading the whole archive).
