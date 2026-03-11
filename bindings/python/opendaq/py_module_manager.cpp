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

#include <pybind11/pybind11.h>
#include <pybind11/gil.h>

#include "py_opendaq/py_opendaq.h"

namespace py = pybind11;

template <typename ModuleManagerClass>
void definePythonModuleSupport(pybind11::module_ m, ModuleManagerClass moduleManagerClass)
{
    using namespace daq;

    m.def("add_python_module",
          [](daq::IModuleManager* manager, daq::IContext* context, py::object pyModule)
          {
              addPythonModuleToManager(manager, context, std::move(pyModule));
          },
          py::arg("module_manager"),
          py::arg("context"),
          py::arg("module"),
          R"doc(
Register a Python-defined module with the module manager.

The Python object must have attributes: name (str), version (tuple of 3 ints or IVersionInfo), id (str).
Override these methods to customize behavior:
  - on_get_available_function_block_types() -> dict[str, IFunctionBlockType]
  - on_create_function_block(id, parent, local_id, config) -> IFunctionBlock or None
)doc");

    // Overload add_module to accept Python module (with .context): instance.module_manager.add_module(MyModule(instance.context))
    moduleManagerClass.def("add_module",
            [](daq::IModuleManager* manager, py::object arg)
            {
                try
                {
                    auto* asModule = arg.cast<daq::IModule*>();
                    if (asModule)
                    {
                        py::gil_scoped_release release;
                        ModuleManagerPtr::Borrow(manager).addModule(asModule);
                        return;
                    }
                }
                catch (const py::cast_error&)
                {
                }
                // Python module: must have .context set (e.g. MyModule(instance.context))
                if (!py::hasattr(arg, "context"))
                    throw std::invalid_argument("Python module must have 'context' attribute (e.g. MyModule(instance.context))");
                py::object contextObj = arg.attr("context");
                if (contextObj.is_none())
                    throw std::invalid_argument("Python module context must not be None");
                daq::IContext* context = contextObj.cast<daq::IContext*>();
                addPythonModuleToManager(manager, context, std::move(arg));
            },
            py::arg("module"),
            "Add a module. Accepts IModule (native) or Python Module with .context set (e.g. MyModule(instance.context)).");
}

template void definePythonModuleSupport<PyDaqIntf<daq::IModuleManager, daq::IBaseObject>>(
    pybind11::module_ m,
    PyDaqIntf<daq::IModuleManager, daq::IBaseObject> moduleManagerClass);
