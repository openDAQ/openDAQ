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

#include "py_core_objects/py_core_objects.h"
#include "py_core_types/py_converter.h"
#include "py_core_objects/py_variant_extractor.h"

PyDaqIntf<daq::IArgumentInfo, daq::IBaseObject> declareIArgumentInfo(pybind11::module_ m)
{
    return wrapInterface<daq::IArgumentInfo, daq::IBaseObject>(m, "IArgumentInfo");
}

void defineIArgumentInfo(pybind11::module_ m, PyDaqIntf<daq::IArgumentInfo, daq::IBaseObject> cls)
{
    cls.doc() = "Provides the name and type of a single function/procedure argument";

    m.def("ArgumentInfo", [](std::variant<daq::IString*, py::str, daq::IEvalValue*>& name, daq::CoreType type){
        return daq::ArgumentInfo_Create(getVariantValue<daq::IString*>(name), type);
    }, py::arg("name"), py::arg("type"));


    cls.def_property_readonly("name",
        [](daq::IArgumentInfo *object)
        {
            const auto objectPtr = daq::ArgumentInfoPtr::Borrow(object);
            return objectPtr.getName().toStdString();
        },
        "Gets the name of the argument.");
    cls.def_property_readonly("type",
        [](daq::IArgumentInfo *object)
        {
            const auto objectPtr = daq::ArgumentInfoPtr::Borrow(object);
            return objectPtr.getType();
        },
        "Gets the core type of the argument.");
}
