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

PyDaqIntf<daq::ISignalConfig, daq::ISignal> declareISignalConfig(pybind11::module_ m)
{
    return wrapInterface<daq::ISignalConfig, daq::ISignal>(m, "ISignalConfig");
}

void defineISignalConfig(pybind11::module_ m, PyDaqIntf<daq::ISignalConfig, daq::ISignal> cls)
{
    cls.doc() = "The configuration component of a Signal. Allows for configuration of its properties, managing its streaming sources, and sending packets through its connections.";

    m.def("Signal", &daq::Signal_Create);
    m.def("SignalWithDescriptor", &daq::SignalWithDescriptor_Create);

    cls.def_property("descriptor",
        nullptr,
        [](daq::ISignalConfig *object, daq::IDataDescriptor* descriptor)
        {
            const auto objectPtr = daq::SignalConfigPtr::Borrow(object);
            objectPtr.setDescriptor(descriptor);
        },
        "Sets the data descriptor.");
    cls.def_property("domain_signal",
        nullptr,
        [](daq::ISignalConfig *object, daq::ISignal* signal)
        {
            const auto objectPtr = daq::SignalConfigPtr::Borrow(object);
            objectPtr.setDomainSignal(signal);
        },
        "Sets the domain signal reference.");
    cls.def_property("related_signals",
        nullptr,
        [](daq::ISignalConfig *object, daq::IList* signals)
        {
            const auto objectPtr = daq::SignalConfigPtr::Borrow(object);
            objectPtr.setRelatedSignals(signals);
        },
        "Sets the list of related signals.");
    cls.def("add_related_signal",
        [](daq::ISignalConfig *object, daq::ISignal* signal)
        {
            const auto objectPtr = daq::SignalConfigPtr::Borrow(object);
            objectPtr.addRelatedSignal(signal);
        },
        py::arg("signal"),
        "Adds a related signal to the list of related signals.");
    cls.def("remove_related_signal",
        [](daq::ISignalConfig *object, daq::ISignal* signal)
        {
            const auto objectPtr = daq::SignalConfigPtr::Borrow(object);
            objectPtr.removeRelatedSignal(signal);
        },
        py::arg("signal"),
        "Removes a signal from the list of related signal.");
    cls.def("clear_related_signals",
        [](daq::ISignalConfig *object)
        {
            const auto objectPtr = daq::SignalConfigPtr::Borrow(object);
            objectPtr.clearRelatedSignals();
        },
        "Clears the list of related signals.");
    cls.def("send_packet",
        [](daq::ISignalConfig *object, daq::IPacket* packet)
        {
            const auto objectPtr = daq::SignalConfigPtr::Borrow(object);
            objectPtr.sendPacket(packet);
        },
        py::arg("packet"),
        "Sends a packet through all connections of the signal.");
    cls.def("send_packets",
        [](daq::ISignalConfig *object, daq::IList* packets)
        {
            const auto objectPtr = daq::SignalConfigPtr::Borrow(object);
            objectPtr.sendPackets(packets);
        },
        py::arg("packets"),
        "Sends multiple packets through all connections of the signal.");
}
