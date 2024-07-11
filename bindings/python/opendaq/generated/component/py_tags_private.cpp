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

PyDaqIntf<daq::ITagsPrivate, daq::IBaseObject> declareITagsPrivate(pybind11::module_ m)
{
    return wrapInterface<daq::ITagsPrivate, daq::IBaseObject>(m, "ITagsPrivate");
}

void defineITagsPrivate(pybind11::module_ m, PyDaqIntf<daq::ITagsPrivate, daq::IBaseObject> cls)
{
    cls.doc() = "Private interface to component tags. Allows for adding/removing tags.";

    cls.def("add",
        [](daq::ITagsPrivate *object, const std::string& name)
        {
            const auto objectPtr = daq::TagsPrivatePtr::Borrow(object);
            objectPtr.add(name);
        },
        py::arg("name"),
        "Adds a new tag to the list.");
    cls.def("remove",
        [](daq::ITagsPrivate *object, const std::string& name)
        {
            const auto objectPtr = daq::TagsPrivatePtr::Borrow(object);
            objectPtr.remove(name);
        },
        py::arg("name"),
        "Removes a new tag from the list.");
    cls.def("replace",
        [](daq::ITagsPrivate *object, daq::IList* tags)
        {
            const auto objectPtr = daq::TagsPrivatePtr::Borrow(object);
            objectPtr.replace(tags);
        },
        py::arg("tags"),
        "Replaces all tags.");
}
