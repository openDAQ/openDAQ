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

#include "py_opendaq/py_opendaq.h"
#include "py_core_types/py_converter.h"

PyDaqIntf<daq::IInstanceBuilder, daq::IBaseObject> declareIInstanceBuilder(pybind11::module_ m)
{
    return wrapInterface<daq::IInstanceBuilder, daq::IBaseObject>(m, "IInstanceBuilder");
}

void defineIInstanceBuilder(pybind11::module_ m, PyDaqIntf<daq::IInstanceBuilder, daq::IBaseObject> cls)
{
    cls.doc() = "Builder component of Instance objects. Contains setter methods to configure the Instance parameters, and a `build` method that builds the Instance object.";

    m.def("InstanceBuilder", &daq::InstanceBuilder_Create);

    cls.def("build",
        [](daq::IInstanceBuilder *object)
        {
            const auto objectPtr = daq::InstanceBuilderPtr::Borrow(object);
            return objectPtr.build().detach();
        },
        "Builds and returns a Instance object using the currently set values of the Builder.");
    cls.def_property("logger",
        [](daq::IInstanceBuilder *object)
        {
            const auto objectPtr = daq::InstanceBuilderPtr::Borrow(object);
            return objectPtr.getLogger().detach();
        },
        [](daq::IInstanceBuilder *object, daq::ILogger* logger)
        {
            const auto objectPtr = daq::InstanceBuilderPtr::Borrow(object);
            objectPtr.setLogger(logger);
        },
        py::return_value_policy::take_ownership,
        "Gets the Logger of Instance / Sets the Logger of Instance");
    cls.def_property("global_log_level",
        [](daq::IInstanceBuilder *object)
        {
            const auto objectPtr = daq::InstanceBuilderPtr::Borrow(object);
            return objectPtr.getGlobalLogLevel();
        },
        [](daq::IInstanceBuilder *object, daq::LogLevel logLevel)
        {
            const auto objectPtr = daq::InstanceBuilderPtr::Borrow(object);
            objectPtr.setGlobalLogLevel(logLevel);
        },
        "Gets the Logger global level of Instance / Sets the Logger global level of Instance");
    cls.def("set_component_log_level",
        [](daq::IInstanceBuilder *object, const std::string& component, daq::LogLevel logLevel)
        {
            const auto objectPtr = daq::InstanceBuilderPtr::Borrow(object);
            objectPtr.setComponentLogLevel(component, logLevel);
        },
        py::arg("component"), py::arg("log_level"),
        "Sets the Logger level of Instance component");
    cls.def("set_sink_log_level",
        [](daq::IInstanceBuilder *object, daq::ILoggerSink* sink, daq::LogLevel logLevel)
        {
            const auto objectPtr = daq::InstanceBuilderPtr::Borrow(object);
            objectPtr.setSinkLogLevel(sink, logLevel);
        },
        py::arg("sink"), py::arg("log_level"),
        "Sets the sink logger level of Instance");
    cls.def_property("module_path",
        nullptr,
        [](daq::IInstanceBuilder *object, const std::string& path)
        {
            const auto objectPtr = daq::InstanceBuilderPtr::Borrow(object);
            objectPtr.setModulePath(path);
        },
        "Sets the path for default ModuleManager of Instance. This method would be ignored if was called setModuleManager method");
    cls.def_property("module_manager",
        [](daq::IInstanceBuilder *object)
        {
            const auto objectPtr = daq::InstanceBuilderPtr::Borrow(object);
            return objectPtr.getModuleManager().detach();
        },
        [](daq::IInstanceBuilder *object, daq::IModuleManager* moduleManager)
        {
            const auto objectPtr = daq::InstanceBuilderPtr::Borrow(object);
            objectPtr.setModuleManager(moduleManager);
        },
        py::return_value_policy::take_ownership,
        "Gets the ModuleManager of Instance. / Sets the ModuleManager of Instance.");
    cls.def_property("scheduler",
        [](daq::IInstanceBuilder *object)
        {
            const auto objectPtr = daq::InstanceBuilderPtr::Borrow(object);
            return objectPtr.getScheduler().detach();
        },
        [](daq::IInstanceBuilder *object, daq::IScheduler* scheduler)
        {
            const auto objectPtr = daq::InstanceBuilderPtr::Borrow(object);
            objectPtr.setScheduler(scheduler);
        },
        py::return_value_policy::take_ownership,
        "Gets the module manager of Instance / Sets the module manager of Instance");
    cls.def("set_option",
        [](daq::IInstanceBuilder *object, const std::string& option, const py::object& value)
        {
            const auto objectPtr = daq::InstanceBuilderPtr::Borrow(object);
            objectPtr.setOption(option, pyObjectToBaseObject(value));
        },
        py::arg("option"), py::arg("value"),
        "Sets the option of Instance");
    cls.def_property_readonly("options",
        [](daq::IInstanceBuilder *object)
        {
            const auto objectPtr = daq::InstanceBuilderPtr::Borrow(object);
            return objectPtr.getOptions().detach();
        },
        py::return_value_policy::take_ownership,
        "Gets dictionary of options of Instance");
    cls.def_property("root_device",
        [](daq::IInstanceBuilder *object)
        {
            const auto objectPtr = daq::InstanceBuilderPtr::Borrow(object);
            return objectPtr.getRootDevice().detach();
        },
        [](daq::IInstanceBuilder *object, daq::IDevice* rootDevice)
        {
            const auto objectPtr = daq::InstanceBuilderPtr::Borrow(object);
            objectPtr.setRootDevice(rootDevice);
        },
        py::return_value_policy::take_ownership,
        "Gets the root device of Instance / Sets the root device of Instance");
    cls.def_property("default_root_device_name",
        [](daq::IInstanceBuilder *object)
        {
            const auto objectPtr = daq::InstanceBuilderPtr::Borrow(object);
            return objectPtr.getDefaultRootDeviceName().toStdString();
        },
        [](daq::IInstanceBuilder *object, const std::string& localId)
        {
            const auto objectPtr = daq::InstanceBuilderPtr::Borrow(object);
            objectPtr.setDefaultRootDeviceName(localId);
        },
        "Gets the default root device name / Sets the default root device name");
    cls.def_property("default_root_device_info",
        [](daq::IInstanceBuilder *object)
        {
            const auto objectPtr = daq::InstanceBuilderPtr::Borrow(object);
            return objectPtr.getDefaultRootDeviceInfo().detach();
        },
        [](daq::IInstanceBuilder *object, daq::IDeviceInfo* deviceInfo)
        {
            const auto objectPtr = daq::InstanceBuilderPtr::Borrow(object);
            objectPtr.setDefaultRootDeviceInfo(deviceInfo);
        },
        py::return_value_policy::take_ownership,
        "Gets the default device info of Instance / Sets the default device of Instance");
}
