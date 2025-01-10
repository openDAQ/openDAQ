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

#include "py_core_objects/py_core_objects.h"
#include "py_core_types/py_converter.h"
#include "py_core_objects/py_variant_extractor.h"

PyDaqIntf<daq::ICoreEventArgs, daq::IEventArgs> declareICoreEventArgs(pybind11::module_ m)
{
    return wrapInterface<daq::ICoreEventArgs, daq::IEventArgs>(m, "ICoreEventArgs");
}

void defineICoreEventArgs(pybind11::module_ m, PyDaqIntf<daq::ICoreEventArgs, daq::IEventArgs> cls)
{
    cls.doc() = "Arguments object that defines a Core event type, and provides a set of parameters specific to a given event type.";

    m.def("CoreEventArgs", [](daq::CoreEventId eventId, std::variant<daq::IString*, py::str, daq::IEvalValue*>& eventName, std::variant<daq::IDict*, py::dict>& parameters){
        return daq::CoreEventArgs_Create(eventId, getVariantValue<daq::IString*>(eventName), getVariantValue<daq::IDict*>(parameters));
    }, py::arg("event_id"), py::arg("event_name"), py::arg("parameters"));

    m.def("CoreEventArgsPropertyValueChanged", [](daq::IPropertyObject* propOwner, std::variant<daq::IString*, py::str, daq::IEvalValue*>& propName, const py::object& value, std::variant<daq::IString*, py::str, daq::IEvalValue*>& path){
        return daq::CoreEventArgsPropertyValueChanged_Create(propOwner, getVariantValue<daq::IString*>(propName), pyObjectToBaseObject(value), getVariantValue<daq::IString*>(path));
    }, py::arg("prop_owner"), py::arg("prop_name"), py::arg("value"), py::arg("path"));

    m.def("CoreEventArgsPropertyObjectUpdateEnd", [](daq::IPropertyObject* propOwner, std::variant<daq::IDict*, py::dict>& updatedProperties, std::variant<daq::IString*, py::str, daq::IEvalValue*>& path){
        return daq::CoreEventArgsPropertyObjectUpdateEnd_Create(propOwner, getVariantValue<daq::IDict*>(updatedProperties), getVariantValue<daq::IString*>(path));
    }, py::arg("prop_owner"), py::arg("updated_properties"), py::arg("path"));

    m.def("CoreEventArgsPropertyAdded", [](daq::IPropertyObject* propOwner, daq::IProperty* prop, std::variant<daq::IString*, py::str, daq::IEvalValue*>& path){
        return daq::CoreEventArgsPropertyAdded_Create(propOwner, prop, getVariantValue<daq::IString*>(path));
    }, py::arg("prop_owner"), py::arg("prop"), py::arg("path"));

    m.def("CoreEventArgsPropertyRemoved", [](daq::IPropertyObject* propOwner, std::variant<daq::IString*, py::str, daq::IEvalValue*>& propName, std::variant<daq::IString*, py::str, daq::IEvalValue*>& path){
        return daq::CoreEventArgsPropertyRemoved_Create(propOwner, getVariantValue<daq::IString*>(propName), getVariantValue<daq::IString*>(path));
    }, py::arg("prop_owner"), py::arg("prop_name"), py::arg("path"));

    m.def("CoreEventArgsTypeAdded", &daq::CoreEventArgsTypeAdded_Create);
    m.def("CoreEventArgsTypeRemoved", [](std::variant<daq::IString*, py::str, daq::IEvalValue*>& typeName){
        return daq::CoreEventArgsTypeRemoved_Create(getVariantValue<daq::IString*>(typeName));
    }, py::arg("type_name"));


    cls.def_property_readonly("parameters",
        [](daq::ICoreEventArgs *object)
        {
            py::gil_scoped_release release;
            const auto objectPtr = daq::CoreEventArgsPtr::Borrow(object);
            return objectPtr.getParameters().detach();
        },
        py::return_value_policy::take_ownership,
        "Gets the parameters of the core event.");
}
