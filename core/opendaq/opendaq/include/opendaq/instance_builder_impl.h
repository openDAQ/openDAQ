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

#pragma once
#include <opendaq/instance_builder.h>
#include <opendaq/instance_factory.h>
#include <opendaq/config_provider_ptr.h>

BEGIN_NAMESPACE_OPENDAQ

class InstanceBuilderImpl : public ImplementationOf<IInstanceBuilder>
{
public:
    explicit InstanceBuilderImpl();

    ErrCode INTERFACE_FUNC build(IInstance** instance) override;

    ErrCode INTERFACE_FUNC addConfigProvider(IConfigProvider* configProvider) override;

    ErrCode INTERFACE_FUNC setContext(IContext* context) override;
    ErrCode INTERFACE_FUNC getContext(IContext** context) override;

    ErrCode INTERFACE_FUNC setLogger(ILogger* logger) override;
    ErrCode INTERFACE_FUNC getLogger(ILogger** logger) override;
    
    ErrCode INTERFACE_FUNC setGlobalLogLevel(LogLevel logLevel) override;
    ErrCode INTERFACE_FUNC getGlobalLogLevel(LogLevel* logLevel) override;

    ErrCode INTERFACE_FUNC setComponentLogLevel(IString* component, LogLevel logLevel) override;
    ErrCode INTERFACE_FUNC getComponentsLogLevel(IDict** components) override;

    ErrCode INTERFACE_FUNC addLoggerSink(ILoggerSink* sink) override;
    ErrCode INTERFACE_FUNC setSinkLogLevel(ILoggerSink* sink, LogLevel logLevel) override;
    ErrCode INTERFACE_FUNC getLoggerSinks(IList** sinks) override;

    ErrCode INTERFACE_FUNC setModulePath(IString* path) override;
    ErrCode INTERFACE_FUNC getModulePath(IString** path) override;
    ErrCode INTERFACE_FUNC addModulePath(IString* path) override;
    ErrCode INTERFACE_FUNC getModulePathsList(IList** paths) override;

    ErrCode INTERFACE_FUNC setModuleManager(IModuleManager* moduleManager) override;
    ErrCode INTERFACE_FUNC getModuleManager(IModuleManager** moduleManager) override;

    ErrCode INTERFACE_FUNC setAuthenticationProvider(IAuthenticationProvider* authenticationProvider) override;
    ErrCode INTERFACE_FUNC getAuthenticationProvider(IAuthenticationProvider** authenticationProvider) override;

    ErrCode INTERFACE_FUNC setScheduler(IScheduler* scheduler) override;
    ErrCode INTERFACE_FUNC getScheduler(IScheduler** scheduler) override;

    ErrCode INTERFACE_FUNC setSchedulerWorkerNum(SizeT numWorkers) override;
    ErrCode INTERFACE_FUNC getSchedulerWorkerNum(SizeT* numWorkers) override;

    ErrCode INTERFACE_FUNC setUsingSchedulerMainLoop(Bool useMainLoop) override;
    ErrCode INTERFACE_FUNC getUsingSchedulerMainLoop(Bool* useMainLoop) override;

    ErrCode INTERFACE_FUNC setDefaultRootDeviceLocalId(IString* localId) override;
    ErrCode INTERFACE_FUNC getDefaultRootDeviceLocalId(IString** localId) override;

    ErrCode INTERFACE_FUNC setRootDevice(IString* connectionString, IPropertyObject* config) override;
    ErrCode INTERFACE_FUNC getRootDevice(IString** connectionString) override;
    ErrCode INTERFACE_FUNC getRootDeviceConfig(IPropertyObject** config) override;

    ErrCode INTERFACE_FUNC setDefaultRootDeviceInfo(IDeviceInfo* deviceInfo) override;
    ErrCode INTERFACE_FUNC getDefaultRootDeviceInfo(IDeviceInfo** deviceInfo) override;

    ErrCode INTERFACE_FUNC getOptions(IDict** options) override;

    ErrCode INTERFACE_FUNC enableStandardProviders(Bool flag) override;

    ErrCode INTERFACE_FUNC getDiscoveryServers(IList** serverNames) override;
    ErrCode INTERFACE_FUNC addDiscoveryServer(IString* serverName) override;

private:
    static DictPtr<IString, IBaseObject> GetDefaultOptions();

    DictPtr<IString, IBaseObject> getModuleManagerOptions();
    DictPtr<IString, IBaseObject> getSchedulerOptions();
    DictPtr<IString, IBaseObject> getLoggingOptions();
    DictPtr<IString, IBaseObject> getRootDevice();
    DictPtr<IString, IBaseObject> getModules();

    DeviceInfoPtr defaultRootDeviceInfo;

    DictPtr<IString, LogLevel> componentsLogLevel;
    ListPtr<ILoggerSink> sinks;
    LoggerPtr logger;

    SchedulerPtr scheduler;
    ModuleManagerPtr moduleManager;
    AuthenticationProviderPtr authenticationProvider;
    ContextPtr context{nullptr};

    Bool useStandardProviders{false};
    ListPtr<IConfigProvider> providers;
    DictPtr<IString, IBaseObject> options;
    PropertyObjectPtr rootDeviceConfig{nullptr};
    ListPtr<IString> discoveryServers;
};

END_NAMESPACE_OPENDAQ
