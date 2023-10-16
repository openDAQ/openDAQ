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
    {
    }

    ErrCode INTERFACE_FUNC stop() override
    {
        return wrapHandler(this, &Self::onStopServer);
    }

protected:
    virtual void onStopServer()
    {

    }

    PropertyObjectPtr serverConfig;
    DevicePtr rootDevice;
    ContextPtr context;
    ModuleManagerPtr moduleManager;
};

END_NAMESPACE_OPENDAQ
