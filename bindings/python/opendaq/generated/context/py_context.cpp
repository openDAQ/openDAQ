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

PyDaqIntf<daq::IContext, daq::IBaseObject> declareIContext(pybind11::module_ m)
{
    return wrapInterface<daq::IContext, daq::IBaseObject>(m, "IContext");
}

void defineIContext(pybind11::module_ m, PyDaqIntf<daq::IContext, daq::IBaseObject> cls)
{
    cls.doc() = "The Context serves as a container for the Scheduler and Logger. It originates at the instance, and is passed to the root device, which forwards it to components such as function blocks and signals.";

    m.def("Context", &daq::Context_Create);

    cls.def_property_readonly("scheduler",
        [](daq::IContext *object)
        {
            const auto objectPtr = daq::ContextPtr::Borrow(object);
            return objectPtr.getScheduler().detach();
        },
        py::return_value_policy::take_ownership,
        "Gets the scheduler.");
    cls.def_property_readonly("logger",
        [](daq::IContext *object)
        {
            const auto objectPtr = daq::ContextPtr::Borrow(object);
            return objectPtr.getLogger().detach();
        },
        py::return_value_policy::take_ownership,
        "Gets the logger.");
    cls.def_property_readonly("module_manager",
        [](daq::IContext *object)
        {
            const auto objectPtr = daq::ContextPtr::Borrow(object);
            return baseObjectToPyObject(objectPtr.getModuleManager());
        },
        py::return_value_policy::take_ownership,
        "Gets the Module Manager as a Base Object.");
    cls.def_property_readonly("type_manager",
        [](daq::IContext *object)
        {
            const auto objectPtr = daq::ContextPtr::Borrow(object);
            return objectPtr.getTypeManager().detach();
        },
        py::return_value_policy::take_ownership,
        "Gets the Type Manager.");
}
