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

#include <pybind11/gil.h>

#include "py_opendaq/py_opendaq.h"
#include "py_core_types/py_converter.h"
#include "py_core_objects/py_variant_extractor.h"

PyDaqIntf<daq::IContext, daq::IBaseObject> declareIContext(pybind11::module_ m)
{
    return wrapInterface<daq::IContext, daq::IBaseObject>(m, "IContext");
}

void defineIContext(pybind11::module_ m, PyDaqIntf<daq::IContext, daq::IBaseObject> cls)
{
    cls.doc() = "The Context serves as a container for the Scheduler and Logger. It originates at the instance, and is passed to the root device, which forwards it to components such as function blocks and signals.";

    m.def("Context", [](daq::IScheduler* Scheduler, daq::ILogger* Logger, daq::ITypeManager* typeManager, daq::IModuleManager* moduleManager, daq::IAuthenticationProvider* authenticationProvider, std::variant<daq::IDict*, py::dict>& options, std::variant<daq::IDict*, py::dict>& discoveryServers){
        return daq::Context_Create(Scheduler, Logger, typeManager, moduleManager, authenticationProvider, getVariantValue<daq::IDict*>(options), getVariantValue<daq::IDict*>(discoveryServers));
    }, py::arg("scheduler"), py::arg("logger"), py::arg("type_manager"), py::arg("module_manager"), py::arg("authentication_provider"), py::arg("options"), py::arg("discovery_servers"));


    cls.def_property_readonly("scheduler",
        [](daq::IContext *object)
        {
            py::gil_scoped_release release;
            const auto objectPtr = daq::ContextPtr::Borrow(object);
            return objectPtr.getScheduler().detach();
        },
        py::return_value_policy::take_ownership,
        "Gets the scheduler.");
    cls.def_property_readonly("logger",
        [](daq::IContext *object)
        {
            py::gil_scoped_release release;
            const auto objectPtr = daq::ContextPtr::Borrow(object);
            return objectPtr.getLogger().detach();
        },
        py::return_value_policy::take_ownership,
        "Gets the logger.");
    cls.def_property_readonly("module_manager",
        [](daq::IContext *object)
        {
            py::gil_scoped_release release;
            const auto objectPtr = daq::ContextPtr::Borrow(object);
            return baseObjectToPyObject(objectPtr.getModuleManager());
        },
        py::return_value_policy::take_ownership,
        "Gets the Module Manager as a Base Object.");
    cls.def_property_readonly("type_manager",
        [](daq::IContext *object)
        {
            py::gil_scoped_release release;
            const auto objectPtr = daq::ContextPtr::Borrow(object);
            return objectPtr.getTypeManager().detach();
        },
        py::return_value_policy::take_ownership,
        "Gets the Type Manager.");
    cls.def_property_readonly("authentication_provider",
        [](daq::IContext *object)
        {
            py::gil_scoped_release release;
            const auto objectPtr = daq::ContextPtr::Borrow(object);
            return objectPtr.getAuthenticationProvider().detach();
        },
        py::return_value_policy::take_ownership,
        "Gets the Authentication provider.");
    cls.def_property_readonly("on_core_event",
        [](daq::IContext *object)
        {
            py::gil_scoped_release release;
            const auto objectPtr = daq::ContextPtr::Borrow(object);
            return objectPtr.getOnCoreEvent().getEventPtr().detach();
        },
        py::return_value_policy::take_ownership,
        "Gets the Core Event object that triggers whenever a change happens within the openDAQ core structure.");
    cls.def_property_readonly("options",
        [](daq::IContext *object)
        {
            py::gil_scoped_release release;
            const auto objectPtr = daq::ContextPtr::Borrow(object);
            return objectPtr.getOptions().detach();
        },
        py::return_value_policy::take_ownership,
        "Gets the dictionary of options");
    cls.def("get_module_options",
        [](daq::IContext *object, std::variant<daq::IString*, py::str, daq::IEvalValue*>& moduleId)
        {
            py::gil_scoped_release release;
            const auto objectPtr = daq::ContextPtr::Borrow(object);
            return objectPtr.getModuleOptions(getVariantValue<daq::IString*>(moduleId)).detach();
        },
        py::arg("module_id"),
        "Retrieves the options associated with the specified module ID.");
    cls.def_property_readonly("discovery_servers",
        [](daq::IContext *object)
        {
            py::gil_scoped_release release;
            const auto objectPtr = daq::ContextPtr::Borrow(object);
            return objectPtr.getDiscoveryServers().detach();
        },
        py::return_value_policy::take_ownership,
        "Gets the dictionary of available discovery services.");
}
