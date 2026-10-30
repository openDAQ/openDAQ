/*
 * Copyright 2022-2026 openDAQ d.o.o.
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
#include "py_core_types/py_function.h"

PyDaqIntf<daq::IMultiReader2Params, daq::IBaseObject> declareIMultiReader2Params(pybind11::module_ m)
{
    py::enum_<daq::MultiReader2ErrorPolicy>(m, "MultiReader2ErrorPolicy")
        .value("Invalidate", daq::MultiReader2ErrorPolicy::Invalidate)
        .value("Exclude", daq::MultiReader2ErrorPolicy::Exclude);

    return wrapInterface<daq::IMultiReader2Params, daq::IBaseObject>(m, "IMultiReader2Params");
}

void defineIMultiReader2Params(pybind11::module_ m, PyDaqIntf<daq::IMultiReader2Params, daq::IBaseObject> cls)
{
    cls.doc() = "The setup a reader applies at construction and on `configure`. The reader keeps no reference to the object.";

    m.def("MultiReader2Params", &daq::MultiReader2Params_Create);

    cls.def_property("inputs",
        [](daq::IMultiReader2Params *object)
        {
            py::gil_scoped_release release;
            const auto objectPtr = daq::MultiReader2ParamsPtr::Borrow(object);
            return objectPtr.getInputs().detach();
        },
        [](daq::IMultiReader2Params *object, std::variant<daq::IList*, py::list, daq::IEvalValue*>& inputs)
        {
            py::gil_scoped_release release;
            const auto objectPtr = daq::MultiReader2ParamsPtr::Borrow(object);
            objectPtr.setInputs(getVariantValue<daq::IList*>(inputs));
        },
        py::return_value_policy::take_ownership,
        "Gets the inputs in slot order: all signals or all input ports, without duplicates.");
    cls.def_property("main_input",
        [](daq::IMultiReader2Params *object)
        {
            py::gil_scoped_release release;
            const auto objectPtr = daq::MultiReader2ParamsPtr::Borrow(object);
            return objectPtr.getMainInput().detach();
        },
        [](daq::IMultiReader2Params *object, daq::IComponent* input)
        {
            py::gil_scoped_release release;
            const auto objectPtr = daq::MultiReader2ParamsPtr::Borrow(object);
            objectPtr.setMainInput(input);
        },
        py::return_value_policy::take_ownership,
        "Gets the input everything aligns to.");
    cls.def_property("accepts_descriptor",
        [](daq::IMultiReader2Params *object)
        {
            py::gil_scoped_release release;
            const auto objectPtr = daq::MultiReader2ParamsPtr::Borrow(object);
            return objectPtr.getAcceptsDescriptor().detach();
        },
        [](daq::IMultiReader2Params *object, std::variant<daq::IFunction*, py::function>& acceptsDescriptor)
        {
            // A plain Python callable is wrapped as daq.Function wraps it
            const daq::FunctionPtr function = std::holds_alternative<daq::IFunction*>(acceptsDescriptor)
                                                  ? daq::FunctionPtr(std::get<daq::IFunction*>(acceptsDescriptor))
                                                  : daq::FunctionPtr(PyFunction_Create<PyConverter>(std::get<py::function>(acceptsDescriptor)));
            py::gil_scoped_release release;
            const auto objectPtr = daq::MultiReader2ParamsPtr::Borrow(object);
            objectPtr.setAcceptsDescriptor(function);
        },
        py::return_value_policy::take_ownership,
        "Gets the owner's judgement of an input's descriptors; null accepts all. Set it to a daq.Function or a plain callable.");
    cls.def_property("value_read_type",
        [](daq::IMultiReader2Params *object)
        {
            py::gil_scoped_release release;
            const auto objectPtr = daq::MultiReader2ParamsPtr::Borrow(object);
            return objectPtr.getValueReadType();
        },
        [](daq::IMultiReader2Params *object, daq::SampleType valueReadType)
        {
            py::gil_scoped_release release;
            const auto objectPtr = daq::MultiReader2ParamsPtr::Borrow(object);
            objectPtr.setValueReadType(valueReadType);
        },
        "Gets the sample type of the read buffers; Float64 by default.");
    cls.def_property("min_read_count",
        [](daq::IMultiReader2Params *object)
        {
            py::gil_scoped_release release;
            const auto objectPtr = daq::MultiReader2ParamsPtr::Borrow(object);
            return objectPtr.getMinReadCount();
        },
        [](daq::IMultiReader2Params *object, const size_t count)
        {
            py::gil_scoped_release release;
            const auto objectPtr = daq::MultiReader2ParamsPtr::Borrow(object);
            objectPtr.setMinReadCount(count);
        },
        "Gets the smallest count a read returns: at least 1, 1 by default.");
    cls.def_property("error_policy",
        [](daq::IMultiReader2Params *object)
        {
            py::gil_scoped_release release;
            const auto objectPtr = daq::MultiReader2ParamsPtr::Borrow(object);
            return objectPtr.getErrorPolicy();
        },
        [](daq::IMultiReader2Params *object, daq::MultiReader2ErrorPolicy policy)
        {
            py::gil_scoped_release release;
            const auto objectPtr = daq::MultiReader2ParamsPtr::Borrow(object);
            objectPtr.setErrorPolicy(policy);
        },
        "Gets the error policy; Invalidate by default.");
}
