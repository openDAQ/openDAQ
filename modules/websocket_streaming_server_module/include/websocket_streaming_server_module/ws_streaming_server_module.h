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

#include <opendaq/module_impl.h>
#include <opendaq/opendaq.h>

#include <websocket_streaming_server_module/common.h>

BEGIN_NAMESPACE_OPENDAQ_WEBSOCKET_STREAMING_SERVER_MODULE

class WsStreamingServerModule final : public Module
{
    public:

        WsStreamingServerModule(ContextPtr context);

        DictPtr<IString, IFunctionBlockType>
        onGetAvailableFunctionBlockTypes() override;

        DictPtr<IString, IServerType>
        onGetAvailableServerTypes() override;

        FunctionBlockPtr onCreateFunctionBlock(
            const StringPtr& typeId,
            const ComponentPtr& parent,
            const StringPtr& localId,
            const PropertyObjectPtr& config) override;

        ServerPtr onCreateServer(
            const StringPtr& typeId,
            const PropertyObjectPtr& config,
            const DevicePtr& rootDevice) override;

    private:

};

END_NAMESPACE_OPENDAQ_WEBSOCKET_STREAMING_SERVER_MODULE
