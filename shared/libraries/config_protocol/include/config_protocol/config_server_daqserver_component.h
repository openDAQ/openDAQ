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
#include <opendaq/server_ptr.h>
#include <opendaq/component_ptr.h>

namespace daq::config_protocol
{

class ConfigServerDaqServerComponent
{
public:
    static BaseObjectPtr enableDiscovery(const RpcContext& context, const ServerPtr& daqServerComponent, const ParamsDictPtr& params);
    static BaseObjectPtr disableDiscovery(const RpcContext& context, const ServerPtr& daqServerComponent, const ParamsDictPtr& params);
};

inline BaseObjectPtr ConfigServerDaqServerComponent::enableDiscovery(const RpcContext& context, const ServerPtr& daqServerComponent, const ParamsDictPtr& /*params*/)
{
    ComponentPtr component = daqServerComponent.asPtr<IComponent>();
    ConfigServerAccessControl::protectViewOnlyConnection(context.connectionType);
    ConfigServerAccessControl::protectLockedComponent(component);
    ConfigServerAccessControl::protectObject(component, context.user, {Permission::Read, Permission::Write});

    daqServerComponent.enableDiscovery();
    return nullptr;
}

inline BaseObjectPtr ConfigServerDaqServerComponent::disableDiscovery(const RpcContext& context, const ServerPtr& daqServerComponent, const ParamsDictPtr& /*params*/)
{
    ComponentPtr component = daqServerComponent.asPtr<IComponent>();
    ConfigServerAccessControl::protectViewOnlyConnection(context.connectionType);
    ConfigServerAccessControl::protectLockedComponent(component);
    ConfigServerAccessControl::protectObject(component, context.user, {Permission::Read, Permission::Write});

    daqServerComponent.disableDiscovery();
    return nullptr;
}

}
