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

#include "py_core_types/py_core_types.h"
#include "py_core_types/py_converter.h"

PyDaqIntf<daq::IEnumeration, daq::IBaseObject> declareIEnumeration(pybind11::module_ m)
{
    return wrapInterface<daq::IEnumeration, daq::IBaseObject>(m, "IEnumeration");
}

void defineIEnumeration(pybind11::module_ m, PyDaqIntf<daq::IEnumeration, daq::IBaseObject> cls)
{
    cls.doc() = "Enumerations are immutable objects that encapsulate a value within a predefined set of named integral constants. These constants are predefined in an Enumeration type with the same name as the Enumeration.";

    m.def("Enumeration", &daq::Enumeration_Create);
    m.def("EnumerationWithType", &daq::EnumerationWithType_Create);

    cls.def_property_readonly("enumeration_type",
        [](daq::IEnumeration *object)
        {
            const auto objectPtr = daq::EnumerationPtr::Borrow(object);
            return objectPtr.getEnumerationType().detach();
        },
        py::return_value_policy::take_ownership,
        "Gets the Enumeration's type.");
    cls.def_property_readonly("value",
        [](daq::IEnumeration *object)
        {
            const auto objectPtr = daq::EnumerationPtr::Borrow(object);
            return objectPtr.getValue().toStdString();
        },
        "Gets the Enumeration value as String containing the name of the enumerator constant.");
}
