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

PyDaqIntf<daq::IConnectedClientInfo, daq::IPropertyObject> declareIConnectedClientInfo(pybind11::module_ m)
{
    return wrapInterface<daq::IConnectedClientInfo, daq::IPropertyObject>(m, "IConnectedClientInfo");
}

void defineIConnectedClientInfo(pybind11::module_ m, PyDaqIntf<daq::IConnectedClientInfo, daq::IPropertyObject> cls)
{
    cls.doc() = "";

    m.def("ConnectedClientInfo", [](std::variant<daq::IString*, py::str, daq::IEvalValue*>& url, daq::ProtocolType protocolType, std::variant<daq::IString*, py::str, daq::IEvalValue*>& protocolName, std::variant<daq::IString*, py::str, daq::IEvalValue*>& clientType, std::variant<daq::IString*, py::str, daq::IEvalValue*>& hostName){
        return daq::ConnectedClientInfo_Create(getVariantValue<daq::IString*>(url), protocolType, getVariantValue<daq::IString*>(protocolName), getVariantValue<daq::IString*>(clientType), getVariantValue<daq::IString*>(hostName));
    }, py::arg("url"), py::arg("protocol_type"), py::arg("protocol_name"), py::arg("client_type"), py::arg("host_name"));


    cls.def_property_readonly("address",
        [](daq::IConnectedClientInfo *object)
        {
            py::gil_scoped_release release;
            const auto objectPtr = daq::ConnectedClientInfoPtr::Borrow(object);
            return objectPtr.getAddress().toStdString();
        },
        "Gets the client address string.");
    cls.def_property_readonly("protocol_type",
        [](daq::IConnectedClientInfo *object)
        {
            py::gil_scoped_release release;
            const auto objectPtr = daq::ConnectedClientInfoPtr::Borrow(object);
            return objectPtr.getProtocolType();
        },
        "Gets the type of protocol used by the client.");
    cls.def_property_readonly("protocol_name",
        [](daq::IConnectedClientInfo *object)
        {
            py::gil_scoped_release release;
            const auto objectPtr = daq::ConnectedClientInfoPtr::Borrow(object);
            return objectPtr.getProtocolName().toStdString();
        },
        "Gets the name of the protocol used by the client.");
    cls.def_property_readonly("client_type_name",
        [](daq::IConnectedClientInfo *object)
        {
            py::gil_scoped_release release;
            const auto objectPtr = daq::ConnectedClientInfoPtr::Borrow(object);
            return objectPtr.getClientTypeName().toStdString();
        },
        "Gets the type of connected configuration connection client.");
    cls.def_property_readonly("host_name",
        [](daq::IConnectedClientInfo *object)
        {
            py::gil_scoped_release release;
            const auto objectPtr = daq::ConnectedClientInfoPtr::Borrow(object);
            return objectPtr.getHostName().toStdString();
        },
        "Gets the client host name.");
}
