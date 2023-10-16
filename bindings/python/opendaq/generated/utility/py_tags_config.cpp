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

PyDaqIntf<daq::ITagsConfig, daq::ITags> declareITagsConfig(pybind11::module_ m)
{
    return wrapInterface<daq::ITagsConfig, daq::ITags>(m, "ITagsConfig");
}

void defineITagsConfig(pybind11::module_ m, PyDaqIntf<daq::ITagsConfig, daq::ITags> cls)
{
    cls.doc() = "";

    m.def("Tags", &daq::Tags_Create);
    m.def("TagsFromExisting", &daq::TagsFromExisting_Create);

    cls.def("add",
        [](daq::ITagsConfig *object, const std::string& name)
        {
            const auto objectPtr = daq::TagsConfigPtr::Borrow(object);
            objectPtr.add(name);
        },
        py::arg("name"),
        "Adds a new tag to the list.");
    cls.def("remove",
        [](daq::ITagsConfig *object, const std::string& name)
        {
            const auto objectPtr = daq::TagsConfigPtr::Borrow(object);
            objectPtr.remove(name);
        },
        py::arg("name"),
        "Removes a new tag from the list.");
}
