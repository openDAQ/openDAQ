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

#include <utility>

#include <opendaq/module_impl.h>
#include <opendaq/opendaq.h>

#include <websocket_streaming/ws_streaming_server.h>

#include <websocket_streaming_server_module/common.h>
#include <websocket_streaming_server_module/ws_stream_test_fb.h>
#include <websocket_streaming_server_module/ws_streaming_server_module.h>
#include <websocket_streaming_server_module/ws_streaming_server_outlet_fb.h>

#include <websocket_streaming_server_module/version.h>

using namespace daq::websocket_streaming;

BEGIN_NAMESPACE_OPENDAQ_WEBSOCKET_STREAMING_SERVER_MODULE

WsStreamingServerModule::WsStreamingServerModule(ContextPtr context)
    : Module(
        "OpenDAQWebSocketStreamingServerModule",
        daq::VersionInfo(
            WS_STREAM_SRV_MODULE_MAJOR_VERSION,
            WS_STREAM_SRV_MODULE_MINOR_VERSION,
            WS_STREAM_SRV_MODULE_PATCH_VERSION),
        std::move(context),
        "OpenDAQWebSocketStreamingServerModule")
{
}

DictPtr<IString, IFunctionBlockType>
WsStreamingServerModule::onGetAvailableFunctionBlockTypes()
{
    auto result = Dict<IString, IFunctionBlockType>();

    result.set(WsStreamingServerOutletFb::ID, WsStreamingServerOutletFb::TYPE);
    result.set(WsStreamTestFb::ID, WsStreamTestFb::createType());

    return result;
}

DictPtr<IString, IServerType>
WsStreamingServerModule::onGetAvailableServerTypes()
{
    auto result = Dict<IString, IServerType>();

    auto type = WsStreamingServer::createType(context);
    result.set(type.getId(), type);

    return result;
}

FunctionBlockPtr WsStreamingServerModule::onCreateFunctionBlock(
    const StringPtr& typeId,
    const ComponentPtr& parent,
    const StringPtr& localId,
    const PropertyObjectPtr& config)
{
    if (typeId == WsStreamingServerOutletFb::ID)
        return createWithImplementation<IFunctionBlock, WsStreamingServerOutletFb>(
            context,
            parent,
            localId);

    else if (typeId == WsStreamTestFb::ID)
        return createWithImplementation<IFunctionBlock, WsStreamTestFb>(
            context,
            parent,
            localId);

    return nullptr;
}

ServerPtr WsStreamingServerModule::onCreateServer(
    const StringPtr& typeId,
    const PropertyObjectPtr& config,
    const DevicePtr& rootDevice)
{
    auto wsConfig = config;
    if (!wsConfig.assigned())
        wsConfig = WsStreamingServer::createDefaultConfig(context);
    else
        wsConfig = WsStreamingServer::populateDefaultConfig(wsConfig, context);

    return createWithImplementation<IServer, WsStreamingServer>(
        rootDevice,
        wsConfig,
        context);
}

END_NAMESPACE_OPENDAQ_WEBSOCKET_STREAMING_SERVER_MODULE
