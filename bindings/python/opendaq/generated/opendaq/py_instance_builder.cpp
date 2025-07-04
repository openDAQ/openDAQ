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

PyDaqIntf<daq::IInstanceBuilder, daq::IBaseObject> declareIInstanceBuilder(pybind11::module_ m)
{
    return wrapInterface<daq::IInstanceBuilder, daq::IBaseObject>(m, "IInstanceBuilder");
}

void defineIInstanceBuilder(pybind11::module_ m, PyDaqIntf<daq::IInstanceBuilder, daq::IBaseObject> cls)
{
    cls.doc() = "Builder component of Instance objects. Contains setter methods to configure the Instance parameters, such as Context (Logger, Scheduler, ModuleManager) and RootDevice. Contains a  `build` method that builds the Instance object.";

    m.def("InstanceBuilder", &daq::InstanceBuilder_Create);

    cls.def("build",
        [](daq::IInstanceBuilder *object)
        {
            py::gil_scoped_release release;
            const auto objectPtr = daq::InstanceBuilderPtr::Borrow(object);
            return objectPtr.build().detach();
        },
        "Builds and returns an Instance object using the currently set values of the Builder.");
    cls.def("add_config_provider",
        [](daq::IInstanceBuilder *object, daq::IConfigProvider* configProvider)
        {
            py::gil_scoped_release release;
            const auto objectPtr = daq::InstanceBuilderPtr::Borrow(object);
            objectPtr.addConfigProvider(configProvider);
        },
        py::arg("config_provider"),
        "Populates internal options dictionary with values from set config provider");
    cls.def_property("context",
        [](daq::IInstanceBuilder *object)
        {
            py::gil_scoped_release release;
            const auto objectPtr = daq::InstanceBuilderPtr::Borrow(object);
            return objectPtr.getContext().detach();
        },
        [](daq::IInstanceBuilder *object, daq::IContext* context)
        {
            py::gil_scoped_release release;
            const auto objectPtr = daq::InstanceBuilderPtr::Borrow(object);
            objectPtr.setContext(context);
        },
        py::return_value_policy::take_ownership,
        "Returns a context object of the instance. / Sets the Context object of the instance. This overwrites other context related settings such as logger, scheduler and module manager settings.");
    cls.def_property("logger",
        [](daq::IInstanceBuilder *object)
        {
            py::gil_scoped_release release;
            const auto objectPtr = daq::InstanceBuilderPtr::Borrow(object);
            return objectPtr.getLogger().detach();
        },
        [](daq::IInstanceBuilder *object, daq::ILogger* logger)
        {
            py::gil_scoped_release release;
            const auto objectPtr = daq::InstanceBuilderPtr::Borrow(object);
            objectPtr.setLogger(logger);
        },
        py::return_value_policy::take_ownership,
        "Gets the Logger of the Instance. Returns nullptr if custom logger has not been set / Sets the custom Logger for the Instance. This logger will be used for logging messages related to the Instance and its components. When configured, the `Logger sink` will be ignored, as it is in use only with the default Instance logger.");
    cls.def_property("global_log_level",
        [](daq::IInstanceBuilder *object)
        {
            py::gil_scoped_release release;
            const auto objectPtr = daq::InstanceBuilderPtr::Borrow(object);
            return objectPtr.getGlobalLogLevel();
        },
        [](daq::IInstanceBuilder *object, daq::LogLevel logLevel)
        {
            py::gil_scoped_release release;
            const auto objectPtr = daq::InstanceBuilderPtr::Borrow(object);
            objectPtr.setGlobalLogLevel(logLevel);
        },
        "Gets the default Logger global level of Instance / Sets the Logger global log level for the Instance. All log messages with a severity level equal to or higher than the specified level will be processed.");
    cls.def("set_component_log_level",
        [](daq::IInstanceBuilder *object, std::variant<daq::IString*, py::str, daq::IEvalValue*>& component, daq::LogLevel logLevel)
        {
            py::gil_scoped_release release;
            const auto objectPtr = daq::InstanceBuilderPtr::Borrow(object);
            objectPtr.setComponentLogLevel(getVariantValue<daq::IString*>(component), logLevel);
        },
        py::arg("component"), py::arg("log_level"),
        "Sets The Logger level for a specific component of the Instance. Log messages related to that component will be processed according to the specified log level.");
    cls.def_property_readonly("components_log_level",
        [](daq::IInstanceBuilder *object)
        {
            py::gil_scoped_release release;
            const auto objectPtr = daq::InstanceBuilderPtr::Borrow(object);
            return objectPtr.getComponentsLogLevel().detach();
        },
        py::return_value_policy::take_ownership,
        "Gets the dictionary of component names and log level which will be added to logger components");
    cls.def("add_logger_sink",
        [](daq::IInstanceBuilder *object, daq::ILoggerSink* sink)
        {
            py::gil_scoped_release release;
            const auto objectPtr = daq::InstanceBuilderPtr::Borrow(object);
            objectPtr.addLoggerSink(sink);
        },
        py::arg("sink"),
        "Adds the logger sink of the default Instance logger. If Logger has been set, configuring of the Logger sink has no effect in building Instance.");
    cls.def("set_sink_log_level",
        [](daq::IInstanceBuilder *object, daq::ILoggerSink* sink, daq::LogLevel logLevel)
        {
            py::gil_scoped_release release;
            const auto objectPtr = daq::InstanceBuilderPtr::Borrow(object);
            objectPtr.setSinkLogLevel(sink, logLevel);
        },
        py::arg("sink"), py::arg("log_level"),
        "Sets the sink logger level of the default Instance logger. If Logger has been set, configuring of the Logger sink has no effect in building Instance.");
    cls.def_property_readonly("logger_sinks",
        [](daq::IInstanceBuilder *object)
        {
            py::gil_scoped_release release;
            const auto objectPtr = daq::InstanceBuilderPtr::Borrow(object);
            return objectPtr.getLoggerSinks().detach();
        },
        py::return_value_policy::take_ownership,
        "Gets the list of logger sinks for the default Instance logger.");
    cls.def_property("module_path",
        [](daq::IInstanceBuilder *object)
        {
            py::gil_scoped_release release;
            const auto objectPtr = daq::InstanceBuilderPtr::Borrow(object);
            return objectPtr.getModulePath().toStdString();
        },
        [](daq::IInstanceBuilder *object, std::variant<daq::IString*, py::str, daq::IEvalValue*>& path)
        {
            py::gil_scoped_release release;
            const auto objectPtr = daq::InstanceBuilderPtr::Borrow(object);
            objectPtr.setModulePath(getVariantValue<daq::IString*>(path));
        },
        "Gets the path for the default ModuleManager of Instance. / Sets the path for the default ModuleManager of the Instance. If Module manager has been set, configuring of Module path has no effect in building Instance.");
    cls.def("add_module_path",
        [](daq::IInstanceBuilder *object, std::variant<daq::IString*, py::str, daq::IEvalValue*>& path)
        {
            py::gil_scoped_release release;
            const auto objectPtr = daq::InstanceBuilderPtr::Borrow(object);
            objectPtr.addModulePath(getVariantValue<daq::IString*>(path));
        },
        py::arg("path"),
        "Add the path for the default ModuleManager of the Instance. If Module manager has been set, configuring of Module path has no effect in building Instance.");
    cls.def_property_readonly("module_paths_list",
        [](daq::IInstanceBuilder *object)
        {
            py::gil_scoped_release release;
            const auto objectPtr = daq::InstanceBuilderPtr::Borrow(object);
            return objectPtr.getModulePathsList().detach();
        },
        py::return_value_policy::take_ownership,
        "Get the list of paths for the default ModuleManager of the Instance. If Module manager has been set, configuring of Module path has no effect in building Instance.");
    cls.def_property("module_manager",
        [](daq::IInstanceBuilder *object)
        {
            py::gil_scoped_release release;
            const auto objectPtr = daq::InstanceBuilderPtr::Borrow(object);
            return objectPtr.getModuleManager().detach();
        },
        [](daq::IInstanceBuilder *object, daq::IModuleManager* moduleManager)
        {
            py::gil_scoped_release release;
            const auto objectPtr = daq::InstanceBuilderPtr::Borrow(object);
            objectPtr.setModuleManager(moduleManager);
        },
        py::return_value_policy::take_ownership,
        "Gets the custom ModuleManager of Instance / Sets The custom ModuleManager for the Instance.");
    cls.def_property("authentication_provider",
        [](daq::IInstanceBuilder *object)
        {
            py::gil_scoped_release release;
            const auto objectPtr = daq::InstanceBuilderPtr::Borrow(object);
            return objectPtr.getAuthenticationProvider().detach();
        },
        [](daq::IInstanceBuilder *object, daq::IAuthenticationProvider* authenticationProvider)
        {
            py::gil_scoped_release release;
            const auto objectPtr = daq::InstanceBuilderPtr::Borrow(object);
            objectPtr.setAuthenticationProvider(authenticationProvider);
        },
        py::return_value_policy::take_ownership,
        "Gets the AuthenticationProvider of Instance / Sets the AuthenticationProvider for the Instance.");
    cls.def_property("scheduler_worker_num",
        [](daq::IInstanceBuilder *object)
        {
            py::gil_scoped_release release;
            const auto objectPtr = daq::InstanceBuilderPtr::Borrow(object);
            return objectPtr.getSchedulerWorkerNum();
        },
        [](daq::IInstanceBuilder *object, const size_t numWorkers)
        {
            py::gil_scoped_release release;
            const auto objectPtr = daq::InstanceBuilderPtr::Borrow(object);
            objectPtr.setSchedulerWorkerNum(numWorkers);
        },
        "Gets the amount of worker threads in the scheduler of Instance. / Sets the number of worker threads in the scheduler of the Instance. If Scheduler has been set, configuring of Scheduler worker num has no effect in building Instance.");
    cls.def_property("scheduler",
        [](daq::IInstanceBuilder *object)
        {
            py::gil_scoped_release release;
            const auto objectPtr = daq::InstanceBuilderPtr::Borrow(object);
            return objectPtr.getScheduler().detach();
        },
        [](daq::IInstanceBuilder *object, daq::IScheduler* scheduler)
        {
            py::gil_scoped_release release;
            const auto objectPtr = daq::InstanceBuilderPtr::Borrow(object);
            objectPtr.setScheduler(scheduler);
        },
        py::return_value_policy::take_ownership,
        "Gets the custom scheduler of Instance / Sets the custom scheduler of Instance");
    cls.def_property("default_root_device_local_id",
        [](daq::IInstanceBuilder *object)
        {
            py::gil_scoped_release release;
            const auto objectPtr = daq::InstanceBuilderPtr::Borrow(object);
            return objectPtr.getDefaultRootDeviceLocalId().toStdString();
        },
        [](daq::IInstanceBuilder *object, std::variant<daq::IString*, py::str, daq::IEvalValue*>& localId)
        {
            py::gil_scoped_release release;
            const auto objectPtr = daq::InstanceBuilderPtr::Borrow(object);
            objectPtr.setDefaultRootDeviceLocalId(getVariantValue<daq::IString*>(localId));
        },
        "Gets the default root device local id / Sets the local id for default device. Has no effect if `Root device` has been congigured.");
    cls.def("set_root_device",
        [](daq::IInstanceBuilder *object, std::variant<daq::IString*, py::str, daq::IEvalValue*>& connectionString, daq::IPropertyObject* config)
        {
            py::gil_scoped_release release;
            const auto objectPtr = daq::InstanceBuilderPtr::Borrow(object);
            objectPtr.setRootDevice(getVariantValue<daq::IString*>(connectionString), config);
        },
        py::arg("connection_string"), py::arg("config") = nullptr,
        "Sets the connection string for a device that replaces the default openDAQ root device. When the instance is created, a connection to the device with the given connection string will be established, and the device will be placed at the root of the component tree structure.");
    cls.def_property_readonly("root_device",
        [](daq::IInstanceBuilder *object)
        {
            py::gil_scoped_release release;
            const auto objectPtr = daq::InstanceBuilderPtr::Borrow(object);
            return objectPtr.getRootDevice().toStdString();
        },
        "Gets the connection string for the default root device of Instance.");
    cls.def_property_readonly("root_device_config",
        [](daq::IInstanceBuilder *object)
        {
            py::gil_scoped_release release;
            const auto objectPtr = daq::InstanceBuilderPtr::Borrow(object);
            return objectPtr.getRootDeviceConfig().detach();
        },
        py::return_value_policy::take_ownership,
        "Gets the configuration property object for the default root device of Instance.");
    cls.def_property("default_root_device_info",
        [](daq::IInstanceBuilder *object)
        {
            py::gil_scoped_release release;
            const auto objectPtr = daq::InstanceBuilderPtr::Borrow(object);
            return objectPtr.getDefaultRootDeviceInfo().detach();
        },
        [](daq::IInstanceBuilder *object, daq::IDeviceInfo* deviceInfo)
        {
            py::gil_scoped_release release;
            const auto objectPtr = daq::InstanceBuilderPtr::Borrow(object);
            objectPtr.setDefaultRootDeviceInfo(deviceInfo);
        },
        py::return_value_policy::take_ownership,
        "Gets the default device info of Instance / Sets the default device info of Instance. If device info has been set, method getInfo of Instance will return set device info if Root Device has not been set");
    cls.def_property_readonly("options",
        [](daq::IInstanceBuilder *object)
        {
            py::gil_scoped_release release;
            const auto objectPtr = daq::InstanceBuilderPtr::Borrow(object);
            return objectPtr.getOptions().detach();
        },
        py::return_value_policy::take_ownership,
        "Gets the dictionary of instance options");
    cls.def("enable_standard_providers",
        [](daq::IInstanceBuilder *object, const bool flag)
        {
            py::gil_scoped_release release;
            const auto objectPtr = daq::InstanceBuilderPtr::Borrow(object);
            objectPtr.enableStandardProviders(flag);
        },
        py::arg("flag"),
        "Allows enabling or disabling standard configuration providers, including JsonConfigProvider, based on the specified flag.");
    cls.def_property_readonly("discovery_servers",
        [](daq::IInstanceBuilder *object)
        {
            py::gil_scoped_release release;
            const auto objectPtr = daq::InstanceBuilderPtr::Borrow(object);
            return objectPtr.getDiscoveryServers().detach();
        },
        py::return_value_policy::take_ownership,
        "Gets the dictionary of discovery servers");
    cls.def("add_discovery_server",
        [](daq::IInstanceBuilder *object, std::variant<daq::IString*, py::str, daq::IEvalValue*>& serverName)
        {
            py::gil_scoped_release release;
            const auto objectPtr = daq::InstanceBuilderPtr::Borrow(object);
            objectPtr.addDiscoveryServer(getVariantValue<daq::IString*>(serverName));
        },
        py::arg("server_name"),
        "Adds a discovery server to the context");
    cls.def_property("using_scheduler_main_loop",
        [](daq::IInstanceBuilder *object)
        {
            py::gil_scoped_release release;
            const auto objectPtr = daq::InstanceBuilderPtr::Borrow(object);
            return objectPtr.getUsingSchedulerMainLoop();
        },
        [](daq::IInstanceBuilder *object, const bool useMainLoop)
        {
            py::gil_scoped_release release;
            const auto objectPtr = daq::InstanceBuilderPtr::Borrow(object);
            objectPtr.setUsingSchedulerMainLoop(useMainLoop);
        },
        "Checks whether the scheduler will be created with main loop support. / Enables or disables usage of the scheduler's main loop.");
}
