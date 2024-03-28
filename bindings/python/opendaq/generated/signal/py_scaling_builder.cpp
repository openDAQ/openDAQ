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

PyDaqIntf<daq::IScalingBuilder, daq::IBaseObject> declareIScalingBuilder(pybind11::module_ m)
{
    return wrapInterface<daq::IScalingBuilder, daq::IBaseObject>(m, "IScalingBuilder");
}

void defineIScalingBuilder(pybind11::module_ m, PyDaqIntf<daq::IScalingBuilder, daq::IBaseObject> cls)
{
    cls.doc() = "Configuration component of Scaling objects. Contains setter methods that allow for Scaling parameter configuration, and a `build` method that builds the Scaling object.";

    m.def("ScalingBuilder", &daq::ScalingBuilder_Create);
    m.def("ScalingBuilderFromExisting", &daq::ScalingBuilderFromExisting_Create);

    cls.def("build",
        [](daq::IScalingBuilder *object)
        {
            const auto objectPtr = daq::ScalingBuilderPtr::Borrow(object);
            return objectPtr.build().detach();
        },
        "Builds and returns a Scaling object using the currently set values of the Builder.");
    cls.def_property("input_data_type",
        [](daq::IScalingBuilder *object)
        {
            const auto objectPtr = daq::ScalingBuilderPtr::Borrow(object);
            return objectPtr.getInputDataType();
        },
        [](daq::IScalingBuilder *object, daq::SampleType type)
        {
            const auto objectPtr = daq::ScalingBuilderPtr::Borrow(object);
            objectPtr.setInputDataType(type);
        },
        "Gets the scaling's input data type. / Sets the scaling's input data type.");
    cls.def_property("output_data_type",
        [](daq::IScalingBuilder *object)
        {
            const auto objectPtr = daq::ScalingBuilderPtr::Borrow(object);
            return objectPtr.getOutputDataType();
        },
        [](daq::IScalingBuilder *object, daq::ScaledSampleType type)
        {
            const auto objectPtr = daq::ScalingBuilderPtr::Borrow(object);
            objectPtr.setOutputDataType(type);
        },
        "Gets the scaling's output data type. / Sets the scaling's output data type.");
    cls.def_property("scaling_type",
        [](daq::IScalingBuilder *object)
        {
            const auto objectPtr = daq::ScalingBuilderPtr::Borrow(object);
            return objectPtr.getScalingType();
        },
        [](daq::IScalingBuilder *object, daq::ScalingType type)
        {
            const auto objectPtr = daq::ScalingBuilderPtr::Borrow(object);
            objectPtr.setScalingType(type);
        },
        "Gets the type of the scaling that determines how the scaling parameters should be interpreted and how the scaling should be calculated. / Sets the type of the scaling that determines how the scaling parameters should be interpreted and how the scaling should be calculated.");
    cls.def_property("parameters",
        [](daq::IScalingBuilder *object)
        {
            const auto objectPtr = daq::ScalingBuilderPtr::Borrow(object);
            return objectPtr.getParameters().detach();
        },
        [](daq::IScalingBuilder *object, daq::IDict* parameters)
        {
            const auto objectPtr = daq::ScalingBuilderPtr::Borrow(object);
            objectPtr.setParameters(parameters);
        },
        py::return_value_policy::take_ownership,
        "Gets the list of parameters that are used to calculate the scaling in conjunction with the input data. / Gets the list of parameters that are used to calculate the scaling in conjunction with the input data.");
    cls.def("add_parameter",
        [](daq::IScalingBuilder *object, const std::string& name, const py::object& parameter)
        {
            const auto objectPtr = daq::ScalingBuilderPtr::Borrow(object);
            objectPtr.addParameter(name, pyObjectToBaseObject(parameter));
        },
        py::arg("name"), py::arg("parameter"),
        "Adds a string-object pair parameter to the Dictionary of Scaling parameters.");
    cls.def("remove_parameter",
        [](daq::IScalingBuilder *object, const std::string& name)
        {
            const auto objectPtr = daq::ScalingBuilderPtr::Borrow(object);
            objectPtr.removeParameter(name);
        },
        py::arg("name"),
        "Removes the parameter with the given name from the Dictionary of Scaling parameters.");
}
