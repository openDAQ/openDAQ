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
#include "py_core_objects/py_variant_extractor.h"

void definePropertySearchFilterFactories(pybind11::module_ m)
{
    m.def("VisiblePropertyFilter", &daq::VisiblePropertyFilter_Create);
    m.def("ReadOnlyPropertyFilter", &daq::ReadOnlyPropertyFilter_Create);
    m.def("NamePropertyFilter", [](std::variant<daq::IString*, py::str, daq::IEvalValue*>& regex){
        return daq::NamePropertyFilter_Create(getVariantValue<daq::IString*>(regex));
    }, py::arg("regex"));
}
