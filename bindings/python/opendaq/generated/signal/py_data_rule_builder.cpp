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

PyDaqIntf<daq::IDataRuleBuilder, daq::IBaseObject> declareIDataRuleBuilder(pybind11::module_ m)
{
    return wrapInterface<daq::IDataRuleBuilder, daq::IBaseObject>(m, "IDataRuleBuilder");
}

void defineIDataRuleBuilder(pybind11::module_ m, PyDaqIntf<daq::IDataRuleBuilder, daq::IBaseObject> cls)
{
    cls.doc() = "Configuration component of Data rule objects. Contains setter methods that allow for Data rule parameter configuration, and a `build` method that builds the Data rule.";

    m.def("DataRuleBuilder", &daq::DataRuleBuilder_Create);
    m.def("DataRuleBuilderFromExisting", &daq::DataRuleBuilderFromExisting_Create);

    cls.def("build",
        [](daq::IDataRuleBuilder *object)
        {
            const auto objectPtr = daq::DataRuleBuilderPtr::Borrow(object);
            return objectPtr.build().detach();
        },
        "Builds and returns a Data rule object using the currently set values of the Builder.");
    cls.def_property("type",
        [](daq::IDataRuleBuilder *object)
        {
            const auto objectPtr = daq::DataRuleBuilderPtr::Borrow(object);
            return objectPtr.getType();
        },
        [](daq::IDataRuleBuilder *object, daq::DataRuleType type)
        {
            const auto objectPtr = daq::DataRuleBuilderPtr::Borrow(object);
            objectPtr.setType(type);
        },
        "Gets the type of the data rule. / Sets the type of the data rule.");
    cls.def_property("parameters",
        [](daq::IDataRuleBuilder *object)
        {
            const auto objectPtr = daq::DataRuleBuilderPtr::Borrow(object);
            return objectPtr.getParameters().detach();
        },
        [](daq::IDataRuleBuilder *object, daq::IDict* parameters)
        {
            const auto objectPtr = daq::DataRuleBuilderPtr::Borrow(object);
            objectPtr.setParameters(parameters);
        },
        py::return_value_policy::take_ownership,
        "Gets a dictionary of string-object key-value pairs representing the parameters used to evaluate the rule. / Sets a dictionary of string-object key-value pairs representing the parameters used to evaluate the rule.");
    cls.def("add_parameter",
        [](daq::IDataRuleBuilder *object, const std::string& name, const py::object& parameter)
        {
            const auto objectPtr = daq::DataRuleBuilderPtr::Borrow(object);
            objectPtr.addParameter(name, pyObjectToBaseObject(parameter));
        },
        py::arg("name"), py::arg("parameter"),
        "Adds a string-object pair parameter to the Dictionary of Data rule parameters.");
    cls.def("remove_parameter",
        [](daq::IDataRuleBuilder *object, const std::string& name)
        {
            const auto objectPtr = daq::DataRuleBuilderPtr::Borrow(object);
            objectPtr.removeParameter(name);
        },
        py::arg("name"),
        "Removes the parameter with the given name from the Dictionary of Data rule parameters.");
}
