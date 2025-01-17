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


PyDaqIntf<daq::IServerCapability, daq::IPropertyObject> declareIServerCapability(pybind11::module_ m)
{
    py::enum_<daq::ProtocolType>(m, "ProtocolType")
        .value("Unknown", daq::ProtocolType::Unknown)
        .value("Configuration", daq::ProtocolType::Configuration)
        .value("Streaming", daq::ProtocolType::Streaming)
        .value("ConfigurationAndStreaming", daq::ProtocolType::ConfigurationAndStreaming);

    return wrapInterface<daq::IServerCapability, daq::IPropertyObject>(m, "IServerCapability");
}

void defineIServerCapability(pybind11::module_ m, PyDaqIntf<daq::IServerCapability, daq::IPropertyObject> cls)
{
    cls.doc() = "Represents standard information about a server's capability to support various protocols. The Server Capability object functions as a Property Object, facilitating the inclusion of custom properties of String, Int, Bool, or Float types. This interface serves to store essential details regarding the supported protocol by a device. It adheres to a standardized set of properties, including methods to retrieve information such as the connection string, protocol name, protocol type, connection type, and core events enabled.";

    cls.def_property_readonly("connection_string",
        [](daq::IServerCapability *object)
        {
            py::gil_scoped_release release;
            const auto objectPtr = daq::ServerCapabilityPtr::Borrow(object);
            return objectPtr.getConnectionString().toStdString();
        },
        "Gets the connection string of the device with the current protocol.");
    cls.def_property_readonly("connection_strings",
        [](daq::IServerCapability *object)
        {
            py::gil_scoped_release release;
            const auto objectPtr = daq::ServerCapabilityPtr::Borrow(object);
            return objectPtr.getConnectionStrings().detach();
        },
        py::return_value_policy::take_ownership,
        "Gets the connection string of the device with the current protocol.");
    cls.def_property_readonly("protocol_name",
        [](daq::IServerCapability *object)
        {
            py::gil_scoped_release release;
            const auto objectPtr = daq::ServerCapabilityPtr::Borrow(object);
            return objectPtr.getProtocolName().toStdString();
        },
        "Gets the name of the protocol supported by the device.");
    cls.def_property_readonly("protocol_id",
        [](daq::IServerCapability *object)
        {
            py::gil_scoped_release release;
            const auto objectPtr = daq::ServerCapabilityPtr::Borrow(object);
            return objectPtr.getProtocolId().toStdString();
        },
        "Gets the id of the protocol supported by the device. Should not contain spaces or special characters except for '_' and '-'.");
    cls.def_property_readonly("protocol_type",
        [](daq::IServerCapability *object)
        {
            py::gil_scoped_release release;
            const auto objectPtr = daq::ServerCapabilityPtr::Borrow(object);
            return objectPtr.getProtocolType();
        },
        "Gets the type of protocol supported by the device.");
    cls.def_property_readonly("prefix",
        [](daq::IServerCapability *object)
        {
            py::gil_scoped_release release;
            const auto objectPtr = daq::ServerCapabilityPtr::Borrow(object);
            return objectPtr.getPrefix().toStdString();
        },
        "Gets the prefix of the connection string (eg. \"daq.nd\" or \"daq.opcua\")");
    cls.def_property_readonly("connection_type",
        [](daq::IServerCapability *object)
        {
            py::gil_scoped_release release;
            const auto objectPtr = daq::ServerCapabilityPtr::Borrow(object);
            return objectPtr.getConnectionType().toStdString();
        },
        "Gets the type of connection supported by the device.");
    cls.def_property_readonly("core_events_enabled",
        [](daq::IServerCapability *object)
        {
            py::gil_scoped_release release;
            const auto objectPtr = daq::ServerCapabilityPtr::Borrow(object);
            return objectPtr.getCoreEventsEnabled();
        },
        "Gets the client update method supported by the device.");
    cls.def_property_readonly("addresses",
        [](daq::IServerCapability *object)
        {
            py::gil_scoped_release release;
            const auto objectPtr = daq::ServerCapabilityPtr::Borrow(object);
            return objectPtr.getAddresses().detach();
        },
        py::return_value_policy::take_ownership,
        "Gets the device's list of addresses with the current protocol.");
    cls.def_property_readonly("port",
        [](daq::IServerCapability *object)
        {
            py::gil_scoped_release release;
            const auto objectPtr = daq::ServerCapabilityPtr::Borrow(object);
            return objectPtr.getPort().detach();
        },
        py::return_value_policy::take_ownership,
        "Gets the port of the device with the current protocol.");
    cls.def_property_readonly("address_info",
        [](daq::IServerCapability *object)
        {
            py::gil_scoped_release release;
            const auto objectPtr = daq::ServerCapabilityPtr::Borrow(object);
            return objectPtr.getAddressInfo().detach();
        },
        py::return_value_policy::take_ownership,
        "Gets the list of address information objects.");
    cls.def_property_readonly("protocol_version",
        [](daq::IServerCapability *object)
        {
            py::gil_scoped_release release;
            const auto objectPtr = daq::ServerCapabilityPtr::Borrow(object);
            return objectPtr.getProtocolVersion().toStdString();
        },
        "Gets the protocol version supported by the device's protocol.");
}
