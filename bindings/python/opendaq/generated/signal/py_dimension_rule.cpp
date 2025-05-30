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

#include "py_opendaq/py_opendaq.h"
#include "py_core_types/py_converter.h"
#include "py_core_objects/py_variant_extractor.h"

PyDaqIntf<daq::IDimensionRule, daq::IBaseObject> declareIDimensionRule(pybind11::module_ m)
{
    py::enum_<daq::DimensionRuleType>(m, "DimensionRuleType")
        .value("Other", daq::DimensionRuleType::Other)
        .value("Linear", daq::DimensionRuleType::Linear)
        .value("Logarithmic", daq::DimensionRuleType::Logarithmic)
        .value("List", daq::DimensionRuleType::List);

    return wrapInterface<daq::IDimensionRule, daq::IBaseObject>(m, "IDimensionRule");
}

void defineIDimensionRule(pybind11::module_ m, PyDaqIntf<daq::IDimensionRule, daq::IBaseObject> cls)
{
    cls.doc() = "Rule that defines the labels (alternatively called bins, ticks) of a dimension.";

    m.def("LinearDimensionRule", [](std::variant<daq::INumber*, double, int64_t, daq::IEvalValue*>& delta, std::variant<daq::INumber*, double, int64_t, daq::IEvalValue*>& start, const size_t size){
        return daq::LinearDimensionRule_Create(getVariantValue<daq::INumber*>(delta), getVariantValue<daq::INumber*>(start), size);
    }, py::arg("delta"), py::arg("start"), py::arg("size"));

    m.def("ListDimensionRule", [](std::variant<daq::IList*, py::list, daq::IEvalValue*>& list){
        return daq::ListDimensionRule_Create(getVariantValue<daq::IList*>(list));
    }, py::arg("list"));

    m.def("LogarithmicDimensionRule", [](std::variant<daq::INumber*, double, int64_t, daq::IEvalValue*>& delta, std::variant<daq::INumber*, double, int64_t, daq::IEvalValue*>& start, std::variant<daq::INumber*, double, int64_t, daq::IEvalValue*>& base, const size_t size){
        return daq::LogarithmicDimensionRule_Create(getVariantValue<daq::INumber*>(delta), getVariantValue<daq::INumber*>(start), getVariantValue<daq::INumber*>(base), size);
    }, py::arg("delta"), py::arg("start"), py::arg("base"), py::arg("size"));

    m.def("DimensionRule", [](daq::DimensionRuleType type, std::variant<daq::IDict*, py::dict>& parameters){
        return daq::DimensionRule_Create(type, getVariantValue<daq::IDict*>(parameters));
    }, py::arg("type"), py::arg("parameters"));

    m.def("DimensionRuleFromBuilder", &daq::DimensionRuleFromBuilder_Create);

    cls.def_property_readonly("type",
        [](daq::IDimensionRule *object)
        {
            py::gil_scoped_release release;
            const auto objectPtr = daq::DimensionRulePtr::Borrow(object);
            return objectPtr.getType();
        },
        "Gets the type of the dimension rule.");
    cls.def_property_readonly("parameters",
        [](daq::IDimensionRule *object)
        {
            py::gil_scoped_release release;
            const auto objectPtr = daq::DimensionRulePtr::Borrow(object);
            return objectPtr.getParameters().detach();
        },
        py::return_value_policy::take_ownership,
        "Gets a dictionary of string-object key-value pairs representing the parameters used to evaluate the rule.");
}
