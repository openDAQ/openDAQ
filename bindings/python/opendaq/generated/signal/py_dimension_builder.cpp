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

#include "py_opendaq/py_opendaq.h"
#include "py_core_types/py_converter.h"

PyDaqIntf<daq::IDimensionBuilder, daq::IBaseObject> declareIDimensionBuilder(pybind11::module_ m)
{
    return wrapInterface<daq::IDimensionBuilder, daq::IBaseObject>(m, "IDimensionBuilder");
}

void defineIDimensionBuilder(pybind11::module_ m, PyDaqIntf<daq::IDimensionBuilder, daq::IBaseObject> cls)
{
    cls.doc() = "Configuration component of Dimension objects. Contains setter methods that allow for Dimension parameter configuration, and a `build` method that builds the Dimension.";

    m.def("DimensionBuilder", &daq::DimensionBuilder_Create);
    m.def("DimensionBuilderFromExisting", &daq::DimensionBuilderFromExisting_Create);

    cls.def("build",
        [](daq::IDimensionBuilder *object)
        {
            const auto objectPtr = daq::DimensionBuilderPtr::Borrow(object);
            return objectPtr.build().detach();
        },
        "Builds and returns a Dimension object using the currently set values of the Builder.");
    cls.def_property("name",
        [](daq::IDimensionBuilder *object)
        {
            const auto objectPtr = daq::DimensionBuilderPtr::Borrow(object);
            return objectPtr.getName().toStdString();
        },
        [](daq::IDimensionBuilder *object, const std::string& name)
        {
            const auto objectPtr = daq::DimensionBuilderPtr::Borrow(object);
            objectPtr.setName(name);
        },
        "Gets the name of the dimension. / Sets the name of the dimension.");
    cls.def_property("unit",
        [](daq::IDimensionBuilder *object)
        {
            const auto objectPtr = daq::DimensionBuilderPtr::Borrow(object);
            return objectPtr.getUnit().detach();
        },
        [](daq::IDimensionBuilder *object, daq::IUnit* unit)
        {
            const auto objectPtr = daq::DimensionBuilderPtr::Borrow(object);
            objectPtr.setUnit(unit);
        },
        py::return_value_policy::take_ownership,
        "Gets the unit of the dimension's labels. / Sets the unit of the dimension's labels.");
    cls.def_property("rule",
        [](daq::IDimensionBuilder *object)
        {
            const auto objectPtr = daq::DimensionBuilderPtr::Borrow(object);
            return objectPtr.getRule().detach();
        },
        [](daq::IDimensionBuilder *object, daq::IDimensionRule* rule)
        {
            const auto objectPtr = daq::DimensionBuilderPtr::Borrow(object);
            objectPtr.setRule(rule);
        },
        py::return_value_policy::take_ownership,
        "Gets the rule that defines the labels and size of the dimension. / Sets the rule that defines the labels and size of the dimension.");
}
