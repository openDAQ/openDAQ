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

PyDaqIntf<daq::IModule, daq::IBaseObject> declareIModule(pybind11::module_ m)
{
    return wrapInterface<daq::IModule, daq::IBaseObject>(m, "IModule");
}

void defineIModule(pybind11::module_ m, PyDaqIntf<daq::IModule, daq::IBaseObject> cls)
{
    cls.doc() = "A module is an object that provides device and function block factories. The object is usually implemented in an external dynamic link / shared library. IModuleManager is responsible for loading all modules.";

    cls.def_property_readonly("version_info",
        [](daq::IModule *object)
        {
            const auto objectPtr = daq::ModulePtr::Borrow(object);
            return objectPtr.getVersionInfo().detach();
        },
        py::return_value_policy::take_ownership,
        "Retrieves the module version information.");
    cls.def_property_readonly("name",
        [](daq::IModule *object)
        {
            const auto objectPtr = daq::ModulePtr::Borrow(object);
            return objectPtr.getName().toStdString();
        },
        "Gets the module name.");
    cls.def_property_readonly("id",
        [](daq::IModule *object)
        {
            const auto objectPtr = daq::ModulePtr::Borrow(object);
            return objectPtr.getId().toStdString();
        },
        "Gets the module id.");
    cls.def_property_readonly("available_devices",
        [](daq::IModule *object)
        {
            const auto objectPtr = daq::ModulePtr::Borrow(object);
            return objectPtr.getAvailableDevices().detach();
        },
        py::return_value_policy::take_ownership,
        "Returns a list of known devices info. The implementation can start discovery in background and only return the results in this function.");
    cls.def_property_readonly("available_device_types",
        [](daq::IModule *object)
        {
            const auto objectPtr = daq::ModulePtr::Borrow(object);
            return objectPtr.getAvailableDeviceTypes().detach();
        },
        py::return_value_policy::take_ownership,
        "Returns a dictionary of known and available device types this module can create.");
    cls.def("create_device",
        [](daq::IModule *object, const std::string& connectionString, daq::IComponent* parent, daq::IPropertyObject* config)
        {
            const auto objectPtr = daq::ModulePtr::Borrow(object);
            return objectPtr.createDevice(connectionString, parent, config).detach();
        },
        py::arg("connection_string"), py::arg("parent"), py::arg("config") = nullptr,
        "Creates a device object that can communicate with the device described in the specified connection string. The device object is not automatically added as a sub-device of the caller, but only returned by reference.");
    cls.def_property_readonly("available_function_block_types",
        [](daq::IModule *object)
        {
            const auto objectPtr = daq::ModulePtr::Borrow(object);
            return objectPtr.getAvailableFunctionBlockTypes().detach();
        },
        py::return_value_policy::take_ownership,
        "Returns a dictionary of known and available function block types this module can create.");
    cls.def("create_function_block",
        [](daq::IModule *object, const std::string& id, daq::IComponent* parent, const std::string& localId, daq::IPropertyObject* config)
        {
            const auto objectPtr = daq::ModulePtr::Borrow(object);
            return objectPtr.createFunctionBlock(id, parent, localId, config).detach();
        },
        py::arg("id"), py::arg("parent"), py::arg("local_id"), py::arg("config") = nullptr,
        "Creates and returns a function block with the specified id. The function block is not automatically added to the FB list of the caller.");
    cls.def_property_readonly("available_server_types",
        [](daq::IModule *object)
        {
            const auto objectPtr = daq::ModulePtr::Borrow(object);
            return objectPtr.getAvailableServerTypes().detach();
        },
        py::return_value_policy::take_ownership,
        "Returns a dictionary of known and available servers types that this module can create.");
    cls.def("create_server",
        [](daq::IModule *object, const std::string& serverTypeId, daq::IDevice* rootDevice, daq::IPropertyObject* config)
        {
            const auto objectPtr = daq::ModulePtr::Borrow(object);
            return objectPtr.createServer(serverTypeId, rootDevice, config).detach();
        },
        py::arg("server_type_id"), py::arg("root_device"), py::arg("config") = nullptr,
        "Creates and returns a server with the specified server type. To prevent cyclic reference, we should not use the Instance instead of rootDevice.");
    cls.def("create_streaming",
        [](daq::IModule *object, const std::string& connectionString, daq::IPropertyObject* config)
        {
            const auto objectPtr = daq::ModulePtr::Borrow(object);
            return objectPtr.createStreaming(connectionString, config).detach();
        },
        py::arg("connection_string"), py::arg("config") = nullptr,
        "Creates and returns a streaming object using the specified connection string and config object.");
    cls.def("create_connection_string",
        [](daq::IModule *object, daq::IServerCapability* serverCapability)
        {
            const auto objectPtr = daq::ModulePtr::Borrow(object);
            return objectPtr.createConnectionString(serverCapability).toStdString();
        },
        py::arg("server_capability"),
        "Creates and returns a connection string from the specified server capability object.");
    cls.def_property_readonly("available_streaming_types",
        [](daq::IModule *object)
        {
            const auto objectPtr = daq::ModulePtr::Borrow(object);
            return objectPtr.getAvailableStreamingTypes().detach();
        },
        py::return_value_policy::take_ownership,
        "Returns a dictionary of known and available streaming types that this module (client) can create.");
}
