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

#include "py_core_objects/py_core_objects.h"
#include "py_core_types/py_converter.h"

PyDaqIntf<daq::IPropertyValueEventArgs, daq::IEventArgs> declareIPropertyValueEventArgs(pybind11::module_ m)
{
    py::enum_<daq::PropertyEventType>(m, "PropertyEventType")
        .value("Update", daq::PropertyEventType::Update)
        .value("Clear", daq::PropertyEventType::Clear)
        .value("Read", daq::PropertyEventType::Read);

    return wrapInterface<daq::IPropertyValueEventArgs, daq::IEventArgs>(m, "IPropertyValueEventArgs");
}

void defineIPropertyValueEventArgs(pybind11::module_ m, PyDaqIntf<daq::IPropertyValueEventArgs, daq::IEventArgs> cls)
{
    cls.doc() = "";

    m.def("PropertyValueEventArgs", &daq::PropertyValueEventArgs_Create);

    cls.def_property_readonly("property",
        [](daq::IPropertyValueEventArgs *object)
        {
            const auto objectPtr = daq::PropertyValueEventArgsPtr::Borrow(object);
            return objectPtr.getProperty().detach();
        },
        py::return_value_policy::take_ownership,
        "");
    cls.def_property("value",
        [](daq::IPropertyValueEventArgs *object)
        {
            const auto objectPtr = daq::PropertyValueEventArgsPtr::Borrow(object);
            return baseObjectToPyObject(objectPtr.getValue());
        },
        [](daq::IPropertyValueEventArgs *object, const py::object& value)
        {
            const auto objectPtr = daq::PropertyValueEventArgsPtr::Borrow(object);
            objectPtr.setValue(pyObjectToBaseObject(value));
        },
        py::return_value_policy::take_ownership,
        "");
    cls.def_property_readonly("property_event_type",
        [](daq::IPropertyValueEventArgs *object)
        {
            const auto objectPtr = daq::PropertyValueEventArgsPtr::Borrow(object);
            return objectPtr.getPropertyEventType();
        },
        "");
    cls.def_property_readonly("is_updating",
        [](daq::IPropertyValueEventArgs *object)
        {
            const auto objectPtr = daq::PropertyValueEventArgsPtr::Borrow(object);
            return objectPtr.getIsUpdating();
        },
        "");
}
