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

#include "py_core_types/py_core_types.h"
#include "py_core_types/py_converter.h"

PyDaqIntf<daq::IStructType, daq::IType> declareIStructType(pybind11::module_ m)
{
    return wrapInterface<daq::IStructType, daq::IType>(m, "IStructType");
}

void defineIStructType(pybind11::module_ m, PyDaqIntf<daq::IStructType, daq::IType> cls)
{
    cls.doc() = "Struct types define the fields (names and value types, as well as optional default values) of Structs with a name matching that of the Struct type.";

    m.def("StructType", &daq::StructType_Create);
    m.def("StructTypeNoDefaults", &daq::StructTypeNoDefaults_Create);

    cls.def_property_readonly("field_names",
        [](daq::IStructType *object)
        {
            py::gil_scoped_release release;
            const auto objectPtr = daq::StructTypePtr::Borrow(object);
            return objectPtr.getFieldNames().detach();
        },
        py::return_value_policy::take_ownership,
        "Gets the list of field names.");
    cls.def_property_readonly("field_default_values",
        [](daq::IStructType *object)
        {
            py::gil_scoped_release release;
            const auto objectPtr = daq::StructTypePtr::Borrow(object);
            return objectPtr.getFieldDefaultValues().detach();
        },
        py::return_value_policy::take_ownership,
        "Gets the list of field default values.");
    cls.def_property_readonly("field_types",
        [](daq::IStructType *object)
        {
            py::gil_scoped_release release;
            const auto objectPtr = daq::StructTypePtr::Borrow(object);
            return objectPtr.getFieldTypes().detach();
        },
        py::return_value_policy::take_ownership,
        "Gets the list of field types.");
}
