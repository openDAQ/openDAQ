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

PyDaqIntf<daq::IEvalValue, daq::IBaseObject> declareIEvalValue(pybind11::module_ m)
{
    return wrapInterface<daq::IEvalValue, daq::IBaseObject>(m, "IEvalValue");
}

void defineIEvalValue(pybind11::module_ m, PyDaqIntf<daq::IEvalValue, daq::IBaseObject> cls)
{
    cls.doc() = "Dynamic expression evaluator";

    m.def("EvalValue", [](std::variant<daq::IString*, py::str, daq::IEvalValue*>& eval){
        return daq::EvalValue_Create(getVariantValue<daq::IString*>(eval));
    }, py::arg("eval"));

    m.def("EvalValueArgs", [](std::variant<daq::IString*, py::str, daq::IEvalValue*>& eval, std::variant<daq::IList*, py::list, daq::IEvalValue*>& args){
        return daq::EvalValueArgs_Create(getVariantValue<daq::IString*>(eval), getVariantValue<daq::IList*>(args));
    }, py::arg("eval"), py::arg("args"));

    m.def("EvalValueFunc", [](std::variant<daq::IString*, py::str, daq::IEvalValue*>& eval, daq::IFunction* func){
        return daq::EvalValueFunc_Create(getVariantValue<daq::IString*>(eval), func);
    }, py::arg("eval"), py::arg("func"));


    cls.def_property_readonly("eval",
        [](daq::IEvalValue *object)
        {
            py::gil_scoped_release release;
            const auto objectPtr = daq::EvalValuePtr::Borrow(object);
            return objectPtr.getEval().toStdString();
        },
        "Gets the expression.");
    cls.def_property_readonly("result",
        [](daq::IEvalValue *object)
        {
            py::gil_scoped_release release;
            const auto objectPtr = daq::EvalValuePtr::Borrow(object);
            return baseObjectToPyObject(objectPtr.getResult());
        },
        py::return_value_policy::take_ownership,
        "Gets the result of the expression.");
    cls.def("clone_with_owner",
        [](daq::IEvalValue *object, daq::IPropertyObject* owner)
        {
            py::gil_scoped_release release;
            const auto objectPtr = daq::EvalValuePtr::Borrow(object);
            return objectPtr.cloneWithOwner(owner).detach();
        },
        py::arg("owner"),
        "Clones the object and attaches an owner.");
    cls.def("get_parse_error_code",
        [](daq::IEvalValue *object)
        {
            py::gil_scoped_release release;
            const auto objectPtr = daq::EvalValuePtr::Borrow(object);
            objectPtr.getParseErrorCode();
        },
        "Returns the parse error code.");
    cls.def_property_readonly("property_references",
        [](daq::IEvalValue *object)
        {
            py::gil_scoped_release release;
            const auto objectPtr = daq::EvalValuePtr::Borrow(object);
            return objectPtr.getPropertyReferences().detach();
        },
        py::return_value_policy::take_ownership,
        "Returns the names of all properties referenced by the eval value.");
    cls.def_property_readonly("result_no_lock",
        [](daq::IEvalValue *object)
        {
            py::gil_scoped_release release;
            const auto objectPtr = daq::EvalValuePtr::Borrow(object);
            return baseObjectToPyObject(objectPtr.getResultNoLock());
        },
        py::return_value_policy::take_ownership,
        "");
}
