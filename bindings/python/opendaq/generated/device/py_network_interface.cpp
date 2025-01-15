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

PyDaqIntf<daq::INetworkInterface, daq::IBaseObject> declareINetworkInterface(pybind11::module_ m)
{
    return wrapInterface<daq::INetworkInterface, daq::IBaseObject>(m, "INetworkInterface");
}

void defineINetworkInterface(pybind11::module_ m, PyDaqIntf<daq::INetworkInterface, daq::IBaseObject> cls)
{
    cls.doc() = "Provides an interface to manipulate the configuration of a device's (server's) network interface. Offers methods to update the IP configuration and retrieve the currently active one, if the corresponding feature supported by the device. Additionally, includes a helper method to create a prebuilt property object with valid default configuration.";

    m.def("NetworkInterface", &daq::NetworkInterface_Create);

    cls.def("request_current_configuration",
        [](daq::INetworkInterface *object)
        {
            py::gil_scoped_release release;
            const auto objectPtr = daq::NetworkInterfacePtr::Borrow(object);
            return objectPtr.requestCurrentConfiguration().detach();
        },
        "Requests the currently active configuration for the network interface.");
    cls.def("submit_configuration",
        [](daq::INetworkInterface *object, daq::IPropertyObject* config)
        {
            py::gil_scoped_release release;
            const auto objectPtr = daq::NetworkInterfacePtr::Borrow(object);
            objectPtr.submitConfiguration(config);
        },
        py::arg("config"),
        "Submits a new configuration for the network interface.");
    cls.def("create_default_configuration",
        [](daq::INetworkInterface *object)
        {
            py::gil_scoped_release release;
            const auto objectPtr = daq::NetworkInterfacePtr::Borrow(object);
            return objectPtr.createDefaultConfiguration().detach();
        },
        "Creates a property object containing default configuration values for a network interface.");
}
