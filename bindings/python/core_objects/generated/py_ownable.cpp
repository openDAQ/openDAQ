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

PyDaqIntf<daq::IOwnable, daq::IBaseObject> declareIOwnable(pybind11::module_ m)
{
    return wrapInterface<daq::IOwnable, daq::IBaseObject>(m, "IOwnable");
}

void defineIOwnable(pybind11::module_ m, PyDaqIntf<daq::IOwnable, daq::IBaseObject> cls)
{
    cls.doc() = "An ownable object can have IPropertyObject as the owner.";

    cls.def_property("owner",
        nullptr,
        [](daq::IOwnable *object, daq::IPropertyObject* owner)
        {
            const auto objectPtr = daq::OwnablePtr::Borrow(object);
            objectPtr.setOwner(owner);
        },
        "Sets the owner of the object.");
}
