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

#include <functional>

#include <opendaq/opendaq.h>

#include <websocket_streaming_server_module/common.h>
#include <websocket_streaming_server_module/ws_client_signal_provider.h>
#include <websocket_streaming_server_module/ws_streaming_server.h>
#include <websocket_streaming_server_module/ws_client_fb.h>

using namespace std::placeholders;

BEGIN_NAMESPACE_OPENDAQ_NEWER_WEBSOCKET_STREAMING_SERVER_MODULE

static WsStreamingServer *findServer(ComponentPtr component)
{
    if (!component.assigned())
        return nullptr;

    while (true)
    {
        ComponentPtr parent = component.getParent();
        if (!parent.assigned())
            break;
        component = parent;
    }

    auto device = component.asPtrOrNull<IDevice>();
    if (!device.assigned())
        return nullptr;

    for (ServerPtr server : device.getServers())
        if (server.getId() == WsStreamingServer::ID)
            return dynamic_cast<WsStreamingServer *>(server.getObject());

    return nullptr;
}

FunctionBlockTypePtr WsClientSignalProvider::createType()
{
    return FunctionBlockType(
        ID,
        ID,
        "Exposes signals provided by clients connected to a WebSocket Streaming server");
}

WsClientSignalProvider::WsClientSignalProvider(
        const ContextPtr& ctx,
        const ComponentPtr& parent,
        const StringPtr& localId)
    : FunctionBlock(createType(), ctx, parent, localId)
{
    auto *server = findServer(parent);
    if (!server)
        return;

    auto& wsServer = server->getWsServer();

    _onClientConnected = wsServer.on_client_connected.connect(
        [self = thisPtr<FunctionBlockPtr>()]
        (wss::connection_ptr connection)
        {
            reinterpret_cast<WsClientSignalProvider *>(self.getObject())->onClientConnected(connection);
        });

    _onClientDisconnected = wsServer.on_client_disconnected.connect(
        [self = thisPtr<FunctionBlockPtr>()]
        (wss::connection_ptr connection, const boost::system::error_code&)
        {
            reinterpret_cast<WsClientSignalProvider *>(self.getObject())->onClientDisconnected(connection);
        });

    _onServerClosed = wsServer.on_closed.connect(
        [self = thisPtr<FunctionBlockPtr>()]
        (const boost::system::error_code& ec)
        {
            reinterpret_cast<WsClientSignalProvider *>(self.getObject())->onServerClosed(ec);
        });
}

void WsClientSignalProvider::removed()
{
    FunctionBlock::removed();

    _onClientConnected.disconnect();
    _onClientDisconnected.disconnect();
    _onServerClosed.disconnect();
}

void WsClientSignalProvider::onClientConnected(
    wss::connection_ptr connection)
{
    auto clientFb = createWithImplementation<IFunctionBlock, WsClientFb>(
        context,
        functionBlocks,
        connection);

    reinterpret_cast<WsClientFb *>(clientFb.getObject())->attach();

    _clientFbs[connection.get()] = clientFb;
    addNestedFunctionBlock(clientFb);
}

void WsClientSignalProvider::onClientDisconnected(
    wss::connection_ptr connection)
{
    auto clientFbIt = _clientFbs.find(connection.get());
    if (clientFbIt == _clientFbs.end())
        return;

    removeNestedFunctionBlock(clientFbIt->second);
    _clientFbs.erase(clientFbIt);
}

void WsClientSignalProvider::onServerClosed(
    const boost::system::error_code& ec)
{
    _onClientConnected.disconnect();
    _onClientDisconnected.disconnect();
    _onServerClosed.disconnect();
}

END_NAMESPACE_OPENDAQ_NEWER_WEBSOCKET_STREAMING_SERVER_MODULE
