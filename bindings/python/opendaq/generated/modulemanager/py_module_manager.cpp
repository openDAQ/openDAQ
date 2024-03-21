//------------------------------------------------------------------------------
// <auto-generated>
//     This code was generated by a tool.
//
//     Changes to this file may cause incorrect behavior and will be lost if
//     the code is regenerated.
//
//     RTGen (PythonGenerator).
// </auto-generated>
//------------------------------------------------------------------------------

/*
 * Copyright 2022-2023 Blueberry d.o.o.
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

#include "py_opendaq/py_opendaq.h"
#include "py_core_types/py_converter.h"

PyDaqIntf<daq::IModuleManager, daq::IBaseObject> declareIModuleManager(pybind11::module_ m)
{
    return wrapInterface<daq::IModuleManager, daq::IBaseObject>(m, "IModuleManager");
}

void defineIModuleManager(pybind11::module_ m, PyDaqIntf<daq::IModuleManager, daq::IBaseObject> cls)
{
    cls.doc() = "Loads all available modules in a implementation-defined manner. User can also side-load custom modules via `addModule` call.";

    m.def("ModuleManager", &daq::ModuleManager_Create);
    m.def("ModuleManagerMultiplePaths", &daq::ModuleManagerMultiplePaths_Create);

    cls.def_property_readonly("modules",
        [](daq::IModuleManager *object)
        {
            const auto objectPtr = daq::ModuleManagerPtr::Borrow(object);
            return objectPtr.getModules().detach();
        },
        py::return_value_policy::take_ownership,
        "Retrieves all modules known to the manager. Whether they were found or side-loaded.");
    cls.def("add_module",
        [](daq::IModuleManager *object, daq::IModule* module)
        {
            const auto objectPtr = daq::ModuleManagerPtr::Borrow(object);
            objectPtr.addModule(module);
        },
        py::arg("module"),
        "Side-load a custom module in run-time from memory that was not found by default.");
    cls.def("load_modules",
        [](daq::IModuleManager *object, daq::IContext* context)
        {
            const auto objectPtr = daq::ModuleManagerPtr::Borrow(object);
            objectPtr.loadModules(context);
        },
        py::arg("context"),
        "Loads all modules from the directory path specified during manager construction. The Context is passed to all loaded modules for internal use.");
}
