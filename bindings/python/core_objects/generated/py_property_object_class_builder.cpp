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

PyDaqIntf<daq::IPropertyObjectClassBuilder, daq::IBaseObject> declareIPropertyObjectClassBuilder(pybind11::module_ m)
{
    return wrapInterface<daq::IPropertyObjectClassBuilder, daq::IBaseObject>(m, "IPropertyObjectClassBuilder");
}

void defineIPropertyObjectClassBuilder(pybind11::module_ m, PyDaqIntf<daq::IPropertyObjectClassBuilder, daq::IBaseObject> cls)
{
    cls.doc() = "The builder interface of Property object classes. Allows for their modification and building of Property object classes.";

    m.def("PropertyObjectClassBuilder", [](std::variant<daq::IString*, py::str, daq::IEvalValue*>& name){
        return daq::PropertyObjectClassBuilder_Create(getVariantValue<daq::IString*>(name));
    }, py::arg("name"));

    m.def("PropertyObjectClassBuilderWithManager", [](daq::ITypeManager* manager, std::variant<daq::IString*, py::str, daq::IEvalValue*>& name){
        return daq::PropertyObjectClassBuilderWithManager_Create(manager, getVariantValue<daq::IString*>(name));
    }, py::arg("manager"), py::arg("name"));


    cls.def("build",
        [](daq::IPropertyObjectClassBuilder *object)
        {
            py::gil_scoped_release release;
            const auto objectPtr = daq::PropertyObjectClassBuilderPtr::Borrow(object);
            return objectPtr.build().detach();
        },
        "Builds and returns a Property object class using the currently set values of the Builder.");
    cls.def_property("name",
        [](daq::IPropertyObjectClassBuilder *object)
        {
            py::gil_scoped_release release;
            const auto objectPtr = daq::PropertyObjectClassBuilderPtr::Borrow(object);
            return objectPtr.getName().toStdString();
        },
        [](daq::IPropertyObjectClassBuilder *object, std::variant<daq::IString*, py::str, daq::IEvalValue*>& className)
        {
            py::gil_scoped_release release;
            const auto objectPtr = daq::PropertyObjectClassBuilderPtr::Borrow(object);
            objectPtr.setName(getVariantValue<daq::IString*>(className));
        },
        "Gets the name of the property class. / Sets the name of the property class.");
    cls.def_property("parent_name",
        [](daq::IPropertyObjectClassBuilder *object)
        {
            py::gil_scoped_release release;
            const auto objectPtr = daq::PropertyObjectClassBuilderPtr::Borrow(object);
            return objectPtr.getParentName().toStdString();
        },
        [](daq::IPropertyObjectClassBuilder *object, std::variant<daq::IString*, py::str, daq::IEvalValue*>& parentName)
        {
            py::gil_scoped_release release;
            const auto objectPtr = daq::PropertyObjectClassBuilderPtr::Borrow(object);
            objectPtr.setParentName(getVariantValue<daq::IString*>(parentName));
        },
        "Gets the name of the parent of the property class. / Gets the name of the parent of the property class.");
    cls.def("add_property",
        [](daq::IPropertyObjectClassBuilder *object, daq::IProperty* property)
        {
            py::gil_scoped_release release;
            const auto objectPtr = daq::PropertyObjectClassBuilderPtr::Borrow(object);
            objectPtr.addProperty(property);
        },
        py::arg("property"),
        "Adds a property to the class.");
    cls.def_property_readonly("properties",
        [](daq::IPropertyObjectClassBuilder *object)
        {
            py::gil_scoped_release release;
            const auto objectPtr = daq::PropertyObjectClassBuilderPtr::Borrow(object);
            return objectPtr.getProperties().detach();
        },
        py::return_value_policy::take_ownership,
        "Gets the dictionary of properties");
    cls.def("remove_property",
        [](daq::IPropertyObjectClassBuilder *object, std::variant<daq::IString*, py::str, daq::IEvalValue*>& propertyName)
        {
            py::gil_scoped_release release;
            const auto objectPtr = daq::PropertyObjectClassBuilderPtr::Borrow(object);
            objectPtr.removeProperty(getVariantValue<daq::IString*>(propertyName));
        },
        py::arg("property_name"),
        "Removes a property with the given name from the class.");
    cls.def_property("property_order",
        [](daq::IPropertyObjectClassBuilder *object)
        {
            py::gil_scoped_release release;
            const auto objectPtr = daq::PropertyObjectClassBuilderPtr::Borrow(object);
            return objectPtr.getPropertyOrder().detach();
        },
        [](daq::IPropertyObjectClassBuilder *object, std::variant<daq::IList*, py::list, daq::IEvalValue*>& orderedPropertyNames)
        {
            py::gil_scoped_release release;
            const auto objectPtr = daq::PropertyObjectClassBuilderPtr::Borrow(object);
            objectPtr.setPropertyOrder(getVariantValue<daq::IList*>(orderedPropertyNames));
        },
        py::return_value_policy::take_ownership,
        "Gets a custom order of properties as defined in the list of property names. / Sets a custom order of properties as defined in the list of property names.");
    cls.def_property_readonly("manager",
        [](daq::IPropertyObjectClassBuilder *object)
        {
            py::gil_scoped_release release;
            const auto objectPtr = daq::PropertyObjectClassBuilderPtr::Borrow(object);
            return objectPtr.getManager().detach();
        },
        py::return_value_policy::take_ownership,
        "Gets a type manager");
}
