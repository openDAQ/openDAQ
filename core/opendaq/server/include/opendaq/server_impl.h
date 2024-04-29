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

BEGIN_NAMESPACE_OPENDAQ

class ServerImpl;

using Server = ServerImpl;

class ServerImpl : public ImplementationOf<IServer>
{
public:
    using Super = ImplementationOf<IServer>;
    using Self = ServerImpl;

    explicit ServerImpl(PropertyObjectPtr serverConfig,
                        DevicePtr rootDevice,
                        ContextPtr context,
                        ModuleManagerPtr moduleManager)
        : serverConfig(std::move(serverConfig))
        , rootDevice(std::move(rootDevice))
        , context(std::move(context))
        , moduleManager(std::move(moduleManager))
        , serverId (createServerId(this->serverConfig))
    {
    }

    ErrCode INTERFACE_FUNC stop() override
    {
        return wrapHandler(this, &Self::onStopServer);
    }

    ErrCode INTERFACE_FUNC getServerId(IString** serverId) override
    {
        if (serverId == nullptr)
            return OPENDAQ_ERR_ARGUMENT_NULL;

        *serverId = this->serverId.addRefAndReturn();
        return OPENDAQ_SUCCESS;
    }

protected:
    virtual void onStopServer()
    {

    }

    PropertyObjectPtr serverConfig;
    DevicePtr rootDevice;
    ContextPtr context;
    ModuleManagerPtr moduleManager;

private: 
    StringPtr createServerId(const PropertyObjectPtr& serverConfig)
    {
        std::string result;
        if (StringPtr manufacturer = serverConfig.getPropertyValue("Manufacturer"); manufacturer != nullptr)
            result = manufacturer.toStdString();
        result += "_";
        if (StringPtr serialNumber = serverConfig.getPropertyValue("SerialNumber"); serialNumber != nullptr)
            result += serialNumber.toStdString();
        return result;
    }

    StringPtr serverId;
};

END_NAMESPACE_OPENDAQ
