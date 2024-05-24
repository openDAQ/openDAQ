/*
 * Copyright 2022-2024 Blueberry d.o.o.
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
#include <opendaq/context_ptr.h>
#include <opendaq/device_ptr.h>
#include <opendaq/module_manager_ptr.h>
#include <opendaq/server.h>
#include <coretypes/impl.h>
#include <coretypes/intfs.h>
#include <coretypes/string_ptr.h>
#include <coretypes/validation.h>
#include <coreobjects/property_factory.h>
#include <opendaq/discovery_server_ptr.h>
#include <coreobjects/property_object_factory.h>

BEGIN_NAMESPACE_OPENDAQ

class ServerImpl;

using Server = ServerImpl;

class ServerImpl : public ImplementationOf<IServer>
{
public:
    using Super = ImplementationOf<IServer>;
    using Self = ServerImpl;

    explicit ServerImpl(StringPtr id,
                        PropertyObjectPtr serverConfig,
                        DevicePtr rootDevice,
                        ContextPtr context,
                        ModuleManagerPtr moduleManager)
        : id(std::move(id))
        , config(std::move(serverConfig))
        , rootDevice(std::move(rootDevice))
        , context(std::move(context))
        , moduleManager(std::move(moduleManager))
    {
    }

    ErrCode INTERFACE_FUNC getId(IString** serverId) override
    {
        if (serverId == nullptr)
            return OPENDAQ_ERR_INVALIDPARAMETER;
        *serverId = id.addRefAndReturn();
        return OPENDAQ_SUCCESS;
    }

    ErrCode INTERFACE_FUNC enableDiscovery() override
    {
        if (context != nullptr)
        {
            DeviceInfoPtr rootDeviceInfo;
            if (this->rootDevice != nullptr)
                rootDeviceInfo = this->rootDevice.getInfo();
            for (const auto& [_, service] : context.getDiscoveryServers())
            {
                service.asPtr<IDiscoveryServer>().registerService(id, getDiscoveryConfig(), rootDeviceInfo);
            }
        }
        return OPENDAQ_SUCCESS;
    }

    ErrCode INTERFACE_FUNC stop() override
    {
        if (context != nullptr)
        {
            for (const auto& [_, service] : context.getDiscoveryServers())
            {
                service.asPtr<IDiscoveryServer>().unregisterService(id);
            }
        }
        return wrapHandler(this, &Self::onStopServer);
    }

protected:

    virtual PropertyObjectPtr getDiscoveryConfig()
    {
        return PropertyObject();
    }

    virtual void onStopServer()
    {
    }

    StringPtr id;
    PropertyObjectPtr config;
    DevicePtr rootDevice;
    ContextPtr context;
    ModuleManagerPtr moduleManager;
};

END_NAMESPACE_OPENDAQ
