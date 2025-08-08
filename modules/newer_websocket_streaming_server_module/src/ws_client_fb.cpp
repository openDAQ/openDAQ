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

#include <algorithm>
#include <functional>
#include <memory>
#include <string>
#include <utility>

#include <boost/system/error_code.hpp>

#include <opendaq/opendaq.h>

#include <websocket_streaming_server_module/common.h>
#include <websocket_streaming_server_module/metadata_to_descriptor.h>
#include <websocket_streaming_server_module/ws_client_fb.h>

#include <ws-streaming/connection.hpp>

using namespace std::placeholders;

BEGIN_NAMESPACE_OPENDAQ_NEWER_WEBSOCKET_STREAMING_SERVER_MODULE

FunctionBlockTypePtr WsClientFb::createType()
{
    return FunctionBlockType(
        ID,
        ID,
        "Exposes signals provided by a WebSocket Streaming client");
}

WsClientFb::WsClientFb(
        const ContextPtr& ctx,
        const ComponentPtr& parent,
        wss::connection_ptr connection)
    : FunctionBlock(createType(), ctx, parent, calculateLocalId(connection))
    , _connection(connection)
{
}

void WsClientFb::attach()
{
    _onConnectionDisconnected = _connection->on_disconnected.connect(
        [self = thisPtr<FunctionBlockPtr>()]
        (const boost::system::error_code& ec)
        {
            reinterpret_cast<WsClientFb *>(self.getObject())->onConnectionDisconnected(ec);
        });

    _onSignalAvailable = _connection->on_available.connect(
        [self = thisPtr<FunctionBlockPtr>()]
        (wss::remote_signal_ptr signal)
        {
            reinterpret_cast<WsClientFb *>(self.getObject())->onSignalAvailable(signal);
        });

    _onSignalUnavailable = _connection->on_unavailable.connect(
        [self = thisPtr<FunctionBlockPtr>()]
        (wss::remote_signal_ptr signal)
        {
            reinterpret_cast<WsClientFb *>(self.getObject())->onSignalUnavailable(signal);
        });
}

StringPtr WsClientFb::calculateLocalId(const wss::connection_ptr& connection)
{
    auto endpoint = connection->socket().remote_endpoint();
    return endpoint.address().to_string() + ':' + std::to_string(endpoint.port());
}

void WsClientFb::onSignalAvailable(wss::remote_signal_ptr signal)
{
    auto& entry = _handlers[signal.get()];
    entry.handler = std::make_shared<RemoteSignalHandler>(signal);
    entry.handler->attach();

    entry.onSignalReady = entry.handler->onSignalReady.connect(
        [self = thisPtr<FunctionBlockPtr>(), handler = entry.handler]()
        {
            return reinterpret_cast<WsClientFb *>(self.getObject())->onSignalReady(handler);
        });
}

void WsClientFb::removed()
{
    FunctionBlock::removed();

    _onSignalAvailable.disconnect();
    _onSignalUnavailable.disconnect();
    _onConnectionDisconnected.disconnect();
}

std::pair<SignalConfigPtr, SignalConfigPtr>
WsClientFb::onSignalReady(std::shared_ptr<RemoteSignalHandler> handler)
{
    auto handlerIt = _handlers.find(handler->signal().get());
    if (handlerIt == _handlers.end())
        return {};

    auto& entry = handlerIt->second;

    if (entry.signal.assigned())
    {
        // XXX TODO - update signal descriptor
        return std::make_pair(entry.signal, entry.domainSignal);
    }

    else
    {
        const auto& signalId = handler->signal()->id();
        std::string safeId = signalId;
        std::replace_if(safeId.begin(), safeId.end(), [](char ch) { return ch == '/'; }, '#');

        const auto& metadata = handler->signal()->metadata();
        auto descriptor = metadataToDescriptor(metadata);
        if (!descriptor.assigned())
            return {};

        auto domainSignalId = metadata.table_id();

        if (domainSignalId != handler->signal()->id())
        {
            auto domainHandlerIt = std::find_if(
                _handlers.begin(),
                _handlers.end(),
                [&](const decltype(_handlers)::value_type& pair)
                {
                    return pair.second.handler->signal()->id() == domainSignalId;
                });

            if (domainHandlerIt == _handlers.end())
                return {};

            entry.domainSignal = domainHandlerIt->second.signal;
            if (!entry.domainSignal.assigned())
                return {};
        }

        entry.signal = createAndAddSignal(safeId, descriptor);
        if (entry.domainSignal.assigned())
            entry.signal.setDomainSignal(entry.domainSignal);

        return std::make_pair(entry.signal, entry.domainSignal);
    }
}

void WsClientFb::onSignalUnavailable(wss::remote_signal_ptr signal)
{
    _handlers.erase(signal.get());
}

void WsClientFb::onConnectionDisconnected(const boost::system::error_code& ec)
{
    _onSignalAvailable.disconnect();
    _onSignalUnavailable.disconnect();
    _onConnectionDisconnected.disconnect();
}

END_NAMESPACE_OPENDAQ_NEWER_WEBSOCKET_STREAMING_SERVER_MODULE
