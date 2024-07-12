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

PyDaqIntf<daq::IAwaitable, daq::IBaseObject> declareIAwaitable(pybind11::module_ m)
{
    return wrapInterface<daq::IAwaitable, daq::IBaseObject>(m, "IAwaitable");
}

void defineIAwaitable(pybind11::module_ m, PyDaqIntf<daq::IAwaitable, daq::IBaseObject> cls)
{
    cls.doc() = "";

    cls.def("cancel",
        [](daq::IAwaitable *object)
        {
            const auto objectPtr = daq::AwaitablePtr::Borrow(object);
            return objectPtr.cancel();
        },
        "Cancels the outstanding work if it has not already started.");
    cls.def("wait",
        [](daq::IAwaitable *object)
        {
            const auto objectPtr = daq::AwaitablePtr::Borrow(object);
            objectPtr.wait();
        },
        "");
    cls.def_property_readonly("result",
        [](daq::IAwaitable *object)
        {
            const auto objectPtr = daq::AwaitablePtr::Borrow(object);
            return baseObjectToPyObject(objectPtr.getResult());
        },
        py::return_value_policy::take_ownership,
        "Waits until the awaitable has a valid result and retrieves it or re-throws the exception that occurred during the execution.");
    cls.def("has_completed",
        [](daq::IAwaitable *object)
        {
            const auto objectPtr = daq::AwaitablePtr::Borrow(object);
            return objectPtr.hasCompleted();
        },
        "Checks if the execution has already finished.");
}
