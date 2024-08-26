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
#include <coreobjects/property_object_impl.h>

BEGIN_NAMESPACE_OPENDAQ

class ServerImpl;

using Server = ServerImpl;

class ServerImpl : public GenericPropertyObjectImpl<IServer>
{
public:
    using Super = GenericPropertyObjectImpl<IServer>;
    using Self = ServerImpl;

    explicit ServerImpl(StringPtr id,
                        PropertyObjectPtr serverConfig,
                        DevicePtr rootDevice,
                        ContextPtr context)
        : Super()
        , config(std::move(serverConfig))
        , rootDevice(std::move(rootDevice))
        , context(std::move(context))
    {
        Super::addProperty(StringPropertyBuilder("ServerId", id).setReadOnly(true).build());
    }

    ErrCode INTERFACE_FUNC getId(IString** serverId) override
    {
        return getTypedValue("ServerId", serverId);
    }

    ErrCode INTERFACE_FUNC enableDiscovery() override
    {
        if (context != nullptr)
        {
            DeviceInfoPtr rootDeviceInfo;
            if (this->rootDevice != nullptr)
                rootDeviceInfo = this->rootDevice.getInfo();

            StringPtr id;
            getId(&id);
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
            StringPtr id;
            getId(&id);

            for (const auto& [_, service] : context.getDiscoveryServers())
            {
                service.asPtr<IDiscoveryServer>().unregisterService(id);
            }
        }
        return wrapHandler(this, &Self::onStopServer);
    }

protected:

    template <typename T>
    ErrCode getTypedValue(const StringPtr& name, T** value)
    {
        if (name == nullptr)
            return OPENDAQ_ERR_ARGUMENT_NULL;
        if (value == nullptr)
            return OPENDAQ_ERR_ARGUMENT_NULL;

        BaseObjectPtr prop;
        ErrCode errCode = getPropertyValue(name, &prop);
        if (OPENDAQ_FAILED(errCode))
            return errCode;

        if (auto convertedValue = prop.asPtrOrNull<T>(); convertedValue.assigned())
        {
            *value = convertedValue.detach();
            return OPENDAQ_SUCCESS;
        }
        return OPENDAQ_ERR_INVALIDTYPE;
    }

    virtual PropertyObjectPtr getDiscoveryConfig()
    {
        return PropertyObject();
    }

    virtual void onStopServer()
    {
    }

    PropertyObjectPtr config;
    DevicePtr rootDevice;
    ContextPtr context;
};

END_NAMESPACE_OPENDAQ
