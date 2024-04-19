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
 * Copyright 2022-2024 Blueberry d.o.o.
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

#include "py_core_types/py_core_types.h"
#include "py_core_types/py_converter.h"

PyDaqIntf<daq::IStruct, daq::IBaseObject> declareIStruct(pybind11::module_ m)
{
    return wrapInterface<daq::IStruct, daq::IBaseObject>(m, "IStruct");
}

void defineIStruct(pybind11::module_ m, PyDaqIntf<daq::IStruct, daq::IBaseObject> cls)
{
    cls.doc() = "Structs are immutable objects that contain a set of key-value pairs. The key, as well as the types of each associated value for each struct are defined in advance within a Struct type that has the same name as the Struct.";

    m.def("Struct", &daq::Struct_Create);
    m.def("StructFromBuilder", &daq::StructFromBuilder_Create);

    cls.def_property_readonly("struct_type",
        [](daq::IStruct *object)
        {
            const auto objectPtr = daq::StructPtr::Borrow(object);
            return objectPtr.getStructType().detach();
        },
        py::return_value_policy::take_ownership,
        "Gets the Struct's type.");
    cls.def_property_readonly("field_names",
        [](daq::IStruct *object)
        {
            const auto objectPtr = daq::StructPtr::Borrow(object);
            return objectPtr.getFieldNames().detach();
        },
        py::return_value_policy::take_ownership,
        "Gets a list of all Struct field names.");
    cls.def_property_readonly("field_values",
        [](daq::IStruct *object)
        {
            const auto objectPtr = daq::StructPtr::Borrow(object);
            return objectPtr.getFieldValues().detach();
        },
        py::return_value_policy::take_ownership,
        "Gets a list of all Struct field values.");
    cls.def("get",
        [](daq::IStruct *object, const std::string& name)
        {
            const auto objectPtr = daq::StructPtr::Borrow(object);
            return baseObjectToPyObject(objectPtr.get(name));
        },
        py::arg("name"),
        "Gets the value of a field with the given name.");
    cls.def_property_readonly("as_dictionary",
        [](daq::IStruct *object)
        {
            const auto objectPtr = daq::StructPtr::Borrow(object);
            return objectPtr.getAsDictionary().detach();
        },
        py::return_value_policy::take_ownership,
        "Gets the field names and values of the Struct as a Dictionary.");
    cls.def("has_field",
        [](daq::IStruct *object, const std::string& name)
        {
            const auto objectPtr = daq::StructPtr::Borrow(object);
            return objectPtr.hasField(name);
        },
        py::arg("name"),
        "Checks whether a field with the given name exists in the Struct");
    cls.def("__dir__",
        [](daq::IStruct *object)
        {
            const auto objectPtr = daq::StructPtr::Borrow(object);
            return baseObjectToPyObject(objectPtr.getFieldNames());
        },
        "Gets a list of all Struct field names.");
    cls.def("__getattr__",
        [](daq::IStruct *object, const std::string& name)
        {
            const auto objectPtr = daq::StructPtr::Borrow(object);
            if(!objectPtr.hasField(name))
                throw py::attribute_error("Attribute '" + name + "' not found");
            return baseObjectToPyObject(objectPtr.get(name));
        },
        py::arg("name"),
        "Gets the value of a field with the given name.");
    cls.def("__setattr__",
        [](daq::IStruct *object, const std::string& name, const py::object&)
        {
            const auto objectPtr = daq::StructPtr::Borrow(object);
            if(!objectPtr.hasField(name))
                throw py::attribute_error("Attribute '" + name + "' not found");
            throw py::attribute_error("Attribute '" + name + "' is read-only");
        },
        py::arg("name"), py::arg("value"),
        "Sets the value of a field with the given name.");
    cls.def("__delattr__",
        [](daq::IStruct *object, const std::string& name)
        {
            const auto objectPtr = daq::StructPtr::Borrow(object);
            if(!objectPtr.hasField(name))
                throw py::attribute_error("Attribute '" + name + "' not found");
            throw py::attribute_error("Attribute '" + name + "' is read-only");
        },
        py::arg("name"),
        "Deletes the field with the given name.");
}
