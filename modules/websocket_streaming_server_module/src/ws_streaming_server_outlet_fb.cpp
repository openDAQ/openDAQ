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

#include <websocket_streaming/ws_streaming_server.h>

#include <websocket_streaming_server_module/common.h>
#include <websocket_streaming_server_module/ws_streaming_client_outlet_fb.h>
#include <websocket_streaming_server_module/ws_streaming_server_outlet_fb.h>

using namespace std::placeholders;

BEGIN_NAMESPACE_OPENDAQ_WEBSOCKET_STREAMING_SERVER_MODULE

/*!
 * @brief Searches for a WsStreamingServer instance in the openDAQ instance that owns the
 *     @p component.
 *
 * The parents of the @p component are inspected to find the root component. If that component is
 * a device object, its servers are inspected to find a WsStreamingServer instance. That instance
 * is then returned.
 *
 * @param component Any component in the openDAQ instance to search.
 *
 * @return A pointer to the first located WsStreamingServer instance, or `nullptr` if the root
 *     component is not a device, or if that device does not have a WsStreamingServer instance.
 */
static ServerPtr findServer(ComponentPtr component)
{
    if (!component.assigned())
        return nullptr;

    // Iteratively ascend the chain of parents until we reach the root Component with no parents.
    while (true)
    {
        ComponentPtr parent = component.getParent();
        if (!parent.assigned())
            break;
        component = parent;
    }

    // Check if the root component is a Device.
    auto device = component.asPtrOrNull<IDevice>();
    if (!device.assigned())
        return nullptr;

    // Check each of the Device's Servers to find a WsStreamingServer.
    for (ServerPtr server : device.getServers())
        if (server.getId() == WsStreamingServer::ID)
            return dynamic_cast<WsStreamingServer *>(server.getObject());

    // No WsStreamingServer was found.
    return nullptr;
}

const FunctionBlockTypePtr WsStreamingServerOutletFb::TYPE{
    FunctionBlockType_Create(
        StringPtr{WsStreamingServerOutletFb::ID},
        StringPtr{WsStreamingServerOutletFb::ID},
        StringPtr{"Exposes signals provided by clients connected to a WebSocket Streaming server"},
        PropertyObject())};

WsStreamingServerOutletFb::WsStreamingServerOutletFb(
        const ContextPtr& context,
        const ComponentPtr& parent,
        const StringPtr& localId)
    : FunctionBlock{TYPE, context, parent, localId}
    , _server{findServer(parent)}
{
    // If no WsStreamingServer instance in this openDAQ instance, log a
    // warning and go dead: this function block will then do nothing.
    if (!_server.assigned())
    {
        LOG_W("No WsStreamingServer instance was found in this openDAQ instance. This FB will do nothing.");
        return;
    }

    // Get a pointer to the underlying ws-streaming server object. We are also holding a smart
    // pointer to its parent WsStreamingServer, ensuring the object will not be destroyed.
    _wsServer = &reinterpret_cast<WsStreamingServer *>(_server.getObject())->getWsServer();

    // To avoid race conditions, we must iterate over any already-connected clients and attach
    // our on-connected/on-disconnected event handlers in the same execution context in which the
    // server is running.
    boost::asio::post(
        _wsServer->executor(),
        [selfPtr = thisPtr<FunctionBlockPtr>()]
        {
            auto& self = *reinterpret_cast<WsStreamingServerOutletFb *>(selfPtr.getObject());
            self.attach();
        });
}

void WsStreamingServerOutletFb::removed()
{
    FunctionBlock::removed();

    _onClientConnected.disconnect();
    _onClientDisconnected.disconnect();
    _onServerClosed.disconnect();
}

void WsStreamingServerOutletFb::attach()
{
    // It is possible the server has already been closed, in which
    // case go dead: this function block will then do nothing.
    if (_wsServer->closed())
        return removed();

    // Attach event handlers to process any clients that connect or disconnect in the future.

    _onClientConnected = _wsServer->on_client_connected.connect(
        [selfPtr = thisPtr<FunctionBlockPtr>()]
        (wss::connection_ptr connection)
        {
            auto& self = *reinterpret_cast<WsStreamingServerOutletFb *>(selfPtr.getObject());
            self.onClientConnected(connection);
        });

    _onClientDisconnected = _wsServer->on_client_disconnected.connect(
        [selfPtr = thisPtr<FunctionBlockPtr>()]
        (wss::connection_ptr connection, const boost::system::error_code&)
        {
            auto& self = *reinterpret_cast<WsStreamingServerOutletFb *>(selfPtr.getObject());
            self.onClientDisconnected(connection);
        });

    _onServerClosed = _wsServer->on_closed.connect(
        [selfPtr = thisPtr<FunctionBlockPtr>()]
        (const boost::system::error_code& ec)
        {
            auto& self = *reinterpret_cast<WsStreamingServerOutletFb *>(selfPtr.getObject());
            self.onServerClosed(ec);
        });

    // Process any already-connected clients.
    for (auto& client : *_wsServer)
        onClientConnected(client);
}

void WsStreamingServerOutletFb::onClientConnected(
    wss::connection_ptr connection)
{
    auto lock = getRecursiveConfigLock();

    auto clientFb = createWithImplementation<IFunctionBlock, WsStreamingClientOutletFb>(
        context,
        functionBlocks,
        connection);

    _clientFbs[connection.get()] = clientFb;
    addNestedFunctionBlock(clientFb);
}

void WsStreamingServerOutletFb::onClientDisconnected(
    wss::connection_ptr connection)
{
    auto lock = getRecursiveConfigLock();

    auto clientFbIt = _clientFbs.find(connection.get());
    if (clientFbIt == _clientFbs.end())
        return;

    removeNestedFunctionBlock(clientFbIt->second);
    _clientFbs.erase(clientFbIt);
}

void WsStreamingServerOutletFb::onServerClosed(
    const boost::system::error_code& ec)
{
    auto lock = getRecursiveConfigLock();

    _onClientConnected.disconnect();
    _onClientDisconnected.disconnect();
    _onServerClosed.disconnect();

    _server.release();
    _wsServer = nullptr;
}

END_NAMESPACE_OPENDAQ_WEBSOCKET_STREAMING_SERVER_MODULE
