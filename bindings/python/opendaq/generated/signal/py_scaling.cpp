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

#include "py_opendaq/py_opendaq.h"
#include "py_core_types/py_converter.h"
#include "py_core_objects/py_variant_extractor.h"

PyDaqIntf<daq::IScaling, daq::IBaseObject> declareIScaling(pybind11::module_ m)
{
    py::enum_<daq::ScalingType>(m, "ScalingType")
        .value("Other", daq::ScalingType::Other)
        .value("Linear", daq::ScalingType::Linear);

    return wrapInterface<daq::IScaling, daq::IBaseObject>(m, "IScaling");
}

void defineIScaling(pybind11::module_ m, PyDaqIntf<daq::IScaling, daq::IBaseObject> cls)
{
    cls.doc() = "Signal descriptor field that defines a scaling transformation, which should be applied to data carried by the signal's packets when read.";

    m.def("LinearScaling", [](std::variant<daq::INumber*, double, daq::IEvalValue*>& scale, std::variant<daq::INumber*, double, daq::IEvalValue*>& offset, daq::SampleType inputDataType, daq::ScaledSampleType outputDataType){
        return daq::LinearScaling_Create(getVariantValue<daq::INumber*>(scale), getVariantValue<daq::INumber*>(offset), inputDataType, outputDataType);
    }, py::arg("scale"), py::arg("offset"), py::arg("input_data_type"), py::arg("output_data_type"));

    m.def("Scaling", [](daq::SampleType inputDataType, daq::ScaledSampleType outputDataType, daq::ScalingType scalingType, std::variant<daq::IDict*, py::dict>& parameters){
        return daq::Scaling_Create(inputDataType, outputDataType, scalingType, getVariantValue<daq::IDict*>(parameters));
    }, py::arg("input_data_type"), py::arg("output_data_type"), py::arg("scaling_type"), py::arg("parameters"));

    m.def("ScalingFromBuilder", &daq::ScalingFromBuilder_Create);

    cls.def_property_readonly("input_sample_type",
        [](daq::IScaling *object)
        {
            const auto objectPtr = daq::ScalingPtr::Borrow(object);
            return objectPtr.getInputSampleType();
        },
        "Gets the scaling's input data type.");
    cls.def_property_readonly("output_sample_type",
        [](daq::IScaling *object)
        {
            const auto objectPtr = daq::ScalingPtr::Borrow(object);
            return objectPtr.getOutputSampleType();
        },
        "Gets the scaling's output data type.");
    cls.def_property_readonly("type",
        [](daq::IScaling *object)
        {
            const auto objectPtr = daq::ScalingPtr::Borrow(object);
            return objectPtr.getType();
        },
        "Gets the type of the scaling that determines how the scaling parameters should be interpreted and how the scaling should be calculated.");
    cls.def_property_readonly("parameters",
        [](daq::IScaling *object)
        {
            const auto objectPtr = daq::ScalingPtr::Borrow(object);
            return objectPtr.getParameters().detach();
        },
        py::return_value_policy::take_ownership,
        "Gets the dictionary of parameters that are used to calculate the scaling in conjunction with the input data.");
}
