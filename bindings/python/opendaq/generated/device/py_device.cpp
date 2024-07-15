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
#include "py_core_objects/py_variant_extractor.h"

PyDaqIntf<daq::IDevice, daq::IFolder> declareIDevice(pybind11::module_ m)
{
    return wrapInterface<daq::IDevice, daq::IFolder>(m, "IDevice");
}

void defineIDevice(pybind11::module_ m, PyDaqIntf<daq::IDevice, daq::IFolder> cls)
{
    cls.doc() = "Represents an openDAQ device. The device contains a list of signals and physical channels. Some devices support adding function blocks, or connecting to devices. The list of available function blocks/devices can be obtained via the `getAvailable` functions, and added via the `add` functions.";

    cls.def_property_readonly("info",
        [](daq::IDevice *object)
        {
            const auto objectPtr = daq::DevicePtr::Borrow(object);
            return objectPtr.getInfo().detach();
        },
        py::return_value_policy::take_ownership,
        "Gets the device info. It contains data about the device such as the device's serial number, location, and connection string.");
    cls.def_property_readonly("domain",
        [](daq::IDevice *object)
        {
            const auto objectPtr = daq::DevicePtr::Borrow(object);
            return objectPtr.getDomain().detach();
        },
        py::return_value_policy::take_ownership,
        "Gets the device's domain data. It allows for querying the device for its domain (time) values.");
    cls.def_property_readonly("inputs_outputs_folder",
        [](daq::IDevice *object)
        {
            const auto objectPtr = daq::DevicePtr::Borrow(object);
            return objectPtr.getInputsOutputsFolder().detach();
        },
        py::return_value_policy::take_ownership,
        "Gets a folder containing channels.");
    cls.def_property_readonly("custom_components",
        [](daq::IDevice *object)
        {
            const auto objectPtr = daq::DevicePtr::Borrow(object);
            return objectPtr.getCustomComponents().detach();
        },
        py::return_value_policy::take_ownership,
        "Gets a list of all components/folders in a device that are not titled 'io', 'sig', 'dev' or 'fb'");
    cls.def_property_readonly("signals",
        [](daq::IDevice *object)
        {
            const auto objectPtr = daq::DevicePtr::Borrow(object);
            return objectPtr.getSignals().detach();
        },
        py::return_value_policy::take_ownership,
        "Gets a list of the device's signals.");
    cls.def("get_signals",
        [](daq::IDevice *object, daq::ISearchFilter* searchFilter)
        {
            const auto objectPtr = daq::DevicePtr::Borrow(object);
            return objectPtr.getSignals(searchFilter).detach();
        },
        py::arg("search_filter") = nullptr,
        "Gets a list of the device's signals.");
    cls.def_property_readonly("signals_recursive",
        [](daq::IDevice *object)
        {
            const auto objectPtr = daq::DevicePtr::Borrow(object);
            return objectPtr.getSignalsRecursive().detach();
        },
        py::return_value_policy::take_ownership,
        "Gets a list of the signals that belong to the device.");
    cls.def("get_signals_recursive",
        [](daq::IDevice *object, daq::ISearchFilter* searchFilter)
        {
            const auto objectPtr = daq::DevicePtr::Borrow(object);
            return objectPtr.getSignalsRecursive(searchFilter).detach();
        },
        py::arg("search_filter") = nullptr,
        "Gets a list of the signals that belong to the device.");
    cls.def_property_readonly("channels",
        [](daq::IDevice *object)
        {
            const auto objectPtr = daq::DevicePtr::Borrow(object);
            return objectPtr.getChannels().detach();
        },
        py::return_value_policy::take_ownership,
        "Gets a flat list of the device's physical channels.");
    cls.def("get_channels",
        [](daq::IDevice *object, daq::ISearchFilter* searchFilter)
        {
            const auto objectPtr = daq::DevicePtr::Borrow(object);
            return objectPtr.getChannels(searchFilter).detach();
        },
        py::arg("search_filter") = nullptr,
        "Gets a flat list of the device's physical channels.");
    cls.def_property_readonly("channels_recursive",
        [](daq::IDevice *object)
        {
            const auto objectPtr = daq::DevicePtr::Borrow(object);
            return objectPtr.getChannelsRecursive().detach();
        },
        py::return_value_policy::take_ownership,
        "Gets a flat list of the device's physical channels. Also finds all visible channels of visible child devices");
    cls.def("get_channels_recursive",
        [](daq::IDevice *object, daq::ISearchFilter* searchFilter)
        {
            const auto objectPtr = daq::DevicePtr::Borrow(object);
            return objectPtr.getChannelsRecursive(searchFilter).detach();
        },
        py::arg("search_filter") = nullptr,
        "Gets a flat list of the device's physical channels. Also finds all visible channels of visible child devices");
    cls.def_property_readonly("devices",
        [](daq::IDevice *object)
        {
            const auto objectPtr = daq::DevicePtr::Borrow(object);
            return objectPtr.getDevices().detach();
        },
        py::return_value_policy::take_ownership,
        "Gets a list of child devices that the device is connected to.");
    cls.def("get_devices",
        [](daq::IDevice *object, daq::ISearchFilter* searchFilter)
        {
            const auto objectPtr = daq::DevicePtr::Borrow(object);
            return objectPtr.getDevices(searchFilter).detach();
        },
        py::arg("search_filter") = nullptr,
        "Gets a list of child devices that the device is connected to.");
    cls.def_property_readonly("available_devices",
        [](daq::IDevice *object)
        {
            const auto objectPtr = daq::DevicePtr::Borrow(object);
            return objectPtr.getAvailableDevices().detach();
        },
        py::return_value_policy::take_ownership,
        "Gets a list of available devices, containing their Device Info.");
    cls.def_property_readonly("available_device_types",
        [](daq::IDevice *object)
        {
            const auto objectPtr = daq::DevicePtr::Borrow(object);
            return objectPtr.getAvailableDeviceTypes().detach();
        },
        py::return_value_policy::take_ownership,
        "Get a dictionary of available device types as <IString, IDeviceType> pairs");
    cls.def("add_device",
        [](daq::IDevice *object, std::variant<daq::IString*, py::str, daq::IEvalValue*>& connectionString, daq::IPropertyObject* config)
        {
            const auto objectPtr = daq::DevicePtr::Borrow(object);
            return objectPtr.addDevice(getVariantValue<daq::IString*>(connectionString), config).detach();
        },
        py::arg("connection_string"), py::arg("config") = nullptr,
        "Connects to a device at the given connection string and returns it.");
    cls.def("remove_device",
        [](daq::IDevice *object, daq::IDevice* device)
        {
            const auto objectPtr = daq::DevicePtr::Borrow(object);
            objectPtr.removeDevice(device);
        },
        py::arg("device"),
        "Disconnects from the device provided as argument and removes it from the internal list of devices.");
    cls.def_property_readonly("function_blocks",
        [](daq::IDevice *object)
        {
            const auto objectPtr = daq::DevicePtr::Borrow(object);
            return objectPtr.getFunctionBlocks().detach();
        },
        py::return_value_policy::take_ownership,
        "Gets the list of added function blocks.");
    cls.def("get_function_blocks",
        [](daq::IDevice *object, daq::ISearchFilter* searchFilter)
        {
            const auto objectPtr = daq::DevicePtr::Borrow(object);
            return objectPtr.getFunctionBlocks(searchFilter).detach();
        },
        py::arg("search_filter") = nullptr,
        "Gets the list of added function blocks.");
    cls.def_property_readonly("available_function_block_types",
        [](daq::IDevice *object)
        {
            const auto objectPtr = daq::DevicePtr::Borrow(object);
            return objectPtr.getAvailableFunctionBlockTypes().detach();
        },
        py::return_value_policy::take_ownership,
        "Gets all function block types that are supported by the device, containing their description.");
    cls.def("add_function_block",
        [](daq::IDevice *object, std::variant<daq::IString*, py::str, daq::IEvalValue*>& typeId, daq::IPropertyObject* config)
        {
            const auto objectPtr = daq::DevicePtr::Borrow(object);
            return objectPtr.addFunctionBlock(getVariantValue<daq::IString*>(typeId), config).detach();
        },
        py::arg("type_id"), py::arg("config") = nullptr,
        "Creates and adds a function block to the device with the provided unique ID and returns it.");
    cls.def("remove_function_block",
        [](daq::IDevice *object, daq::IFunctionBlock* functionBlock)
        {
            const auto objectPtr = daq::DevicePtr::Borrow(object);
            objectPtr.removeFunctionBlock(functionBlock);
        },
        py::arg("function_block"),
        "Removes the function block provided as argument, disconnecting its signals and input ports.");
    cls.def("save_configuration",
        [](daq::IDevice *object)
        {
            const auto objectPtr = daq::DevicePtr::Borrow(object);
            return objectPtr.saveConfiguration().toStdString();
        },
        "Saves the configuration of the device to string.");
    cls.def("load_configuration",
        [](daq::IDevice *object, std::variant<daq::IString*, py::str, daq::IEvalValue*>& configuration)
        {
            const auto objectPtr = daq::DevicePtr::Borrow(object);
            objectPtr.loadConfiguration(getVariantValue<daq::IString*>(configuration));
        },
        py::arg("configuration"),
        "Loads the configuration of the device from string.");
    cls.def_property_readonly("ticks_since_origin",
        [](daq::IDevice *object)
        {
            const auto objectPtr = daq::DevicePtr::Borrow(object);
            return objectPtr.getTicksSinceOrigin();
        },
        "Gets the number of ticks passed since the device's absolute origin.");
    cls.def("add_streaming",
        [](daq::IDevice *object, std::variant<daq::IString*, py::str, daq::IEvalValue*>& connectionString, daq::IPropertyObject* config)
        {
            const auto objectPtr = daq::DevicePtr::Borrow(object);
            return objectPtr.addStreaming(getVariantValue<daq::IString*>(connectionString), config).detach();
        },
        py::arg("connection_string"), py::arg("config") = nullptr,
        "Connects to a streaming at the given connection string, adds it as a streaming source of device and returns created streaming object.");
    cls.def("create_default_add_device_config",
        [](daq::IDevice *object)
        {
            const auto objectPtr = daq::DevicePtr::Borrow(object);
            return objectPtr.createDefaultAddDeviceConfig().detach();
        },
        "Creates config object that can be used when adding a device. Contains Device and Streaming default configuration for all available Device/Streaming types. Also contains general add-device configuration settings.");
}
