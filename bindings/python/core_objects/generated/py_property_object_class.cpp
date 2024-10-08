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
 * Copyright 2022-2024 openDAQ d.o.o.
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

PyDaqIntf<daq::IPropertyObjectClass, daq::IType> declareIPropertyObjectClass(pybind11::module_ m)
{
    return wrapInterface<daq::IPropertyObjectClass, daq::IType>(m, "IPropertyObjectClass");
}

void defineIPropertyObjectClass(pybind11::module_ m, PyDaqIntf<daq::IPropertyObjectClass, daq::IType> cls)
{
    cls.doc() = "Container of properties that can be used as a base class when instantiating a Property object.";

    m.def("PropertyObjectClassFromBuilder", &daq::PropertyObjectClassFromBuilder_Create);

    cls.def_property_readonly("parent_name",
        [](daq::IPropertyObjectClass *object)
        {
            py::gil_scoped_release release;
            const auto objectPtr = daq::PropertyObjectClassPtr::Borrow(object);
            return objectPtr.getParentName().toStdString();
        },
        "Gets the name of the parent of the property class.");
    cls.def("get_property",
        [](daq::IPropertyObjectClass *object, std::variant<daq::IString*, py::str, daq::IEvalValue*>& propertyName)
        {
            py::gil_scoped_release release;
            const auto objectPtr = daq::PropertyObjectClassPtr::Borrow(object);
            return objectPtr.getProperty(getVariantValue<daq::IString*>(propertyName)).detach();
        },
        py::arg("property_name"),
        "Gets the class's property with the given name.");
    cls.def("has_property",
        [](daq::IPropertyObjectClass *object, std::variant<daq::IString*, py::str, daq::IEvalValue*>& propertyName)
        {
            py::gil_scoped_release release;
            const auto objectPtr = daq::PropertyObjectClassPtr::Borrow(object);
            return objectPtr.hasProperty(getVariantValue<daq::IString*>(propertyName));
        },
        py::arg("property_name"),
        "Checks if the property is registered.");
    cls.def("get_properties",
        [](daq::IPropertyObjectClass *object, const bool includeInherited)
        {
            py::gil_scoped_release release;
            const auto objectPtr = daq::PropertyObjectClassPtr::Borrow(object);
            return objectPtr.getProperties(includeInherited).detach();
        },
        py::arg("include_inherited"),
        "Gets the list of properties added to the class.");
}
