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
#include <opendaq/context.h>
#include <opendaq/context_internal.h>
#include <opendaq/logger_ptr.h>
#include <opendaq/scheduler_ptr.h>
#include <opendaq/module_manager_ptr.h>
#include <coretypes/type_manager_ptr.h>
#include <coreobjects/authentication_provider_ptr.h>
#include <opendaq/discovery_server_ptr.h>

BEGIN_NAMESPACE_OPENDAQ

template <typename MainInterface, typename ... Interfaces>
class GenericContextImpl : public ImplementationOf<MainInterface, IContextInternal, Interfaces...>
{
public:
    using Self = GenericContextImpl<MainInterface, Interfaces...>;

    explicit GenericContextImpl(SchedulerPtr scheduler,
                         LoggerPtr logger,
                         TypeManagerPtr typeManager,
                         ModuleManagerPtr moduleManager,
                         AuthenticationProviderPtr authenticationProvider,
                         DictPtr<IString, IBaseObject> options,
                         DictPtr<IString, IDiscoveryServer> discoveryServices);
    ~GenericContextImpl();

    ErrCode INTERFACE_FUNC getScheduler(IScheduler** scheduler) override;
    ErrCode INTERFACE_FUNC getLogger(ILogger** logger) override;
    ErrCode INTERFACE_FUNC getModuleManager(IBaseObject** manager) override;
    ErrCode INTERFACE_FUNC getTypeManager(ITypeManager** manager) override;
    ErrCode INTERFACE_FUNC getAuthenticationProvider(IAuthenticationProvider** authenticationProvider) override;
    ErrCode INTERFACE_FUNC getOnCoreEvent(IEvent** event) override;
    ErrCode INTERFACE_FUNC moveModuleManager(IModuleManager** manager) override;
    ErrCode INTERFACE_FUNC getOptions(IDict** options) override;
    ErrCode INTERFACE_FUNC getModuleOptions(IString* moduleId, IDict** options) override;
    ErrCode INTERFACE_FUNC getDiscoveryServers(IDict** servers) override;

private:
    void componentCoreEventCallback(ComponentPtr& component, CoreEventArgsPtr& eventArgs);
    void registerOpenDaqTypes();

    LoggerPtr logger;
    SchedulerPtr scheduler;
    WeakRefPtr<IModuleManager> moduleManagerWeakRef;
    ModuleManagerPtr moduleManager;
    TypeManagerPtr typeManager;
    AuthenticationProviderPtr authenticationProvider;
    EventEmitter<ComponentPtr, CoreEventArgsPtr> coreEvent;
    DictPtr<IString, IBaseObject> options;
    DictPtr<IString, IDiscoveryServer> discoveryServers;
};

using ContextImpl = GenericContextImpl<IContext>;

END_NAMESPACE_OPENDAQ
