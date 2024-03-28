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
 * Copyright 2022-2024 Blueberry d.o.o.
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

PyDaqIntf<daq::IMirroredSignalPrivate, daq::IBaseObject> declareIMirroredSignalPrivate(pybind11::module_ m)
{
    return wrapInterface<daq::IMirroredSignalPrivate, daq::IBaseObject>(m, "IMirroredSignalPrivate");
}

void defineIMirroredSignalPrivate(pybind11::module_ m, PyDaqIntf<daq::IMirroredSignalPrivate, daq::IBaseObject> cls)
{
    cls.doc() = "Internal functions used by openDAQ core. This interface should never be used in client SDK or module code.";

    cls.def("trigger_event",
        [](daq::IMirroredSignalPrivate *object, daq::IEventPacket* eventPacket)
        {
            const auto objectPtr = daq::MirroredSignalPrivatePtr::Borrow(object);
            return objectPtr.triggerEvent(eventPacket);
        },
        py::arg("event_packet"),
        "Handles event packet e.g. packet with changes of the signals descriptors or signal properties");
    cls.def("add_streaming_source",
        [](daq::IMirroredSignalPrivate *object, daq::IStreaming* streaming)
        {
            const auto objectPtr = daq::MirroredSignalPrivatePtr::Borrow(object);
            objectPtr.addStreamingSource(streaming);
        },
        py::arg("streaming"),
        "Adds streaming source for signal.");
    cls.def("remove_streaming_source",
        [](daq::IMirroredSignalPrivate *object, const std::string& streamingConnectionString)
        {
            const auto objectPtr = daq::MirroredSignalPrivatePtr::Borrow(object);
            objectPtr.removeStreamingSource(streamingConnectionString);
        },
        py::arg("streaming_connection_string"),
        "Removes streaming source for signal.");
    cls.def("subscribe_completed",
        [](daq::IMirroredSignalPrivate *object, const std::string& streamingConnectionString)
        {
            const auto objectPtr = daq::MirroredSignalPrivatePtr::Borrow(object);
            objectPtr.subscribeCompleted(streamingConnectionString);
        },
        py::arg("streaming_connection_string"),
        "Handles the completion of subscription acknowledged by the specified streaming source.");
    cls.def("unsubscribe_completed",
        [](daq::IMirroredSignalPrivate *object, const std::string& streamingConnectionString)
        {
            const auto objectPtr = daq::MirroredSignalPrivatePtr::Borrow(object);
            objectPtr.unsubscribeCompleted(streamingConnectionString);
        },
        py::arg("streaming_connection_string"),
        "Handles the completion of unsubscription acknowledged by the specified streaming source.");
    cls.def_property("mirrored_data_descriptor",
        [](daq::IMirroredSignalPrivate *object)
        {
            const auto objectPtr = daq::MirroredSignalPrivatePtr::Borrow(object);
            return objectPtr.getMirroredDataDescriptor().detach();
        },
        [](daq::IMirroredSignalPrivate *object, daq::IDataDescriptor* descriptor)
        {
            const auto objectPtr = daq::MirroredSignalPrivatePtr::Borrow(object);
            objectPtr.setMirroredDataDescriptor(descriptor);
        },
        py::return_value_policy::take_ownership,
        "");
    cls.def_property("mirrored_domain_signal",
        [](daq::IMirroredSignalPrivate *object)
        {
            const auto objectPtr = daq::MirroredSignalPrivatePtr::Borrow(object);
            return objectPtr.getMirroredDomainSignal().detach();
        },
        [](daq::IMirroredSignalPrivate *object, daq::IMirroredSignalConfig* domainSignal)
        {
            const auto objectPtr = daq::MirroredSignalPrivatePtr::Borrow(object);
            objectPtr.setMirroredDomainSignal(domainSignal);
        },
        py::return_value_policy::take_ownership,
        "");
}
