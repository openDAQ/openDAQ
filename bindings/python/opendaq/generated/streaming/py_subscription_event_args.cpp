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

PyDaqIntf<daq::ISubscriptionEventArgs, daq::IEventArgs> declareISubscriptionEventArgs(pybind11::module_ m)
{
    py::enum_<daq::SubscriptionEventType>(m, "SubscriptionEventType")
        .value("Subscribed", daq::SubscriptionEventType::Subscribed)
        .value("Unsubscribed", daq::SubscriptionEventType::Unsubscribed);

    return wrapInterface<daq::ISubscriptionEventArgs, daq::IEventArgs>(m, "ISubscriptionEventArgs");
}

void defineISubscriptionEventArgs(pybind11::module_ m, PyDaqIntf<daq::ISubscriptionEventArgs, daq::IEventArgs> cls)
{
    cls.doc() = "";

    m.def("SubscriptionEventArgs", [](std::variant<daq::IString*, py::str, daq::IEvalValue*>& streamingConnectionString, daq::SubscriptionEventType type){
        return daq::SubscriptionEventArgs_Create(getVariantValue<daq::IString*>(streamingConnectionString), type);
    }, py::arg("streaming_connection_string"), py::arg("type"));


    cls.def_property_readonly("streaming_connection_string",
        [](daq::ISubscriptionEventArgs *object)
        {
            py::gil_scoped_release release;
            const auto objectPtr = daq::SubscriptionEventArgsPtr::Borrow(object);
            return objectPtr.getStreamingConnectionString().toStdString();
        },
        "");
    cls.def_property_readonly("subscription_event_type",
        [](daq::ISubscriptionEventArgs *object)
        {
            py::gil_scoped_release release;
            const auto objectPtr = daq::SubscriptionEventArgsPtr::Borrow(object);
            return objectPtr.getSubscriptionEventType();
        },
        "");
}
