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
#include <cstring>
#include <functional>
#include <memory>
#include <string>

#include <boost/algorithm/string/replace.hpp>
#include <boost/endian/conversion.hpp>
#include <boost/system/error_code.hpp>

#include <opendaq/opendaq.h>
#include <opendaq/streaming_impl.h>

#include <ws-streaming/connection.hpp>
#include <ws-streaming/remote_signal.hpp>

#include <websocket_streaming/common.h>
#include <websocket_streaming/metadata_to_descriptor.h>
#include <websocket_streaming/ws_streaming.h>

using namespace std::placeholders;

BEGIN_NAMESPACE_OPENDAQ_WEBSOCKET_STREAMING

StreamingTypePtr WsStreaming::createType()
{
    return StreamingTypeBuilder()
        .setId("OpenDAQLTStreaming")
        .setName("openDAQ WebSocket Streaming")
        .setDescription("Streaming from devices using the WebSocket Streaming Protocol")
        .setConnectionStringPrefix("daq.lt")
        .build();
}

WsStreaming::WsStreaming(
        const StringPtr& connectionString,
        const ContextPtr& context)
    : Streaming(connectionString, context, true)
    , ioContext{1}
    , wsClient(ioContext.get_executor())
{
    // The ws-streaming library wants a URL like ws://1.2.3.4:7418/foo.
    // So we simply need to replace the daq.lt:// prefix with ws://.
    auto wsConnectionString = connectionString.toStdString();
    boost::replace_all(wsConnectionString, createType().getConnectionStringPrefix().toStdString() + "://", "ws://");

        // Start the ws-streaming connection attempt.
    LOG_I("Connecting to {}", wsConnectionString);
    wsClient.async_connect(wsConnectionString,
        std::bind(&WsStreaming::onConnected, this, _1, _2));

    // Start a background thread to pump the Boost.Asio I/O context. The run() function will
    // return when there is no more work or when ioContext.stop() is called in the destructor.
    thread = std::thread{[this] { ioContext.run(); }};
}

WsStreaming::~WsStreaming()
{
    // Stop the Boost.Asio I/O context (which may already have stopped naturally if the connection
    // failed and there is no more scheduled work) so we can join and destroy the thread.
    LOG_I("Stopping Boost.Asio I/O context thread");
    ioContext.stop();
    thread.join();
}

void WsStreaming::onSetActive(bool active)
{
}

void WsStreaming::onAddSignal(const MirroredSignalConfigPtr& signal)
{
}

void WsStreaming::onRemoveSignal(const MirroredSignalConfigPtr& signal)
{
}

void WsStreaming::onSubscribeSignal(const StringPtr& signalId)
{
    LOG_I("Asked to subscribe signal {}", signalId);

    if (auto signalIt = signals.find(signalId); signalIt != signals.end())
    {
        // Don't subscribe a signal if we haven't actually registered it yet (because we're
        // waiting for its initial metadata). This should not happen.
        if (!signalIt->second->isPublished)
        {
            LOG_I("Found signal, but refusing to subscribe it because it's not published yet");
            return;
        }

        // Don't subscribe a signal if it's already subscribed.
        if (signalIt->second->isSubscribed)
        {
            LOG_I("Found signal, but refusing to subscribe it because it's already subscribed");
            return;
        }

        LOG_I("Found signal, subscribing");
        signalIt->second->ptr->subscribe();
        signalIt->second->isSubscribed = true;
    }

    else
    {
        LOG_I("No such signal");
    }
}

void WsStreaming::onUnsubscribeSignal(const StringPtr& signalId)
{
    LOG_I("Asked to unsubscribe signal {}", signalId);

    if (auto signalIt = signals.find(signalId); signalIt != signals.end())
    {   
        // Don't unsubscribe a signal if it's not actually subscribed.
        if (!signalIt->second->isSubscribed)
        {
            LOG_I("Found signal, but refusing to unsubscribe it because it's not subscribed");
            return;
        }

        LOG_I("Found signal, unsubscribing");
        signalIt->second->ptr->unsubscribe();
        signalIt->second->isSubscribed = false;
    }

    else
    {
        LOG_I("No such signal");
    }
}

void WsStreaming::onConnected(
    const boost::system::error_code& ec,
    wss::connection_ptr connection)
{
    if (ec)
        return;

    LOG_I("Connected to remote peer");
    wsConnection = connection;

    wsConnection->on_available.connect(
        std::bind(&WsStreaming::onRemoteSignalAvailable, this, _1));
    wsConnection->on_unavailable.connect(
        std::bind(&WsStreaming::onRemoteSignalUnavailable, this, _1));
}

void WsStreaming::onRemoteSignalAvailable(wss::remote_signal_ptr signal)
{
    LOG_I("Signal available: {}", signal->id());

    auto entry = std::make_shared<WsStreamingRemoteSignalEntry>();
    entry->ptr = signal;

    entry->onSubscribed         = signal->on_subscribed         .connect(std::bind(&WsStreaming::onRemoteSignalSubscribed,      this, entry));
    entry->onMetadataChanged    = signal->on_metadata_changed   .connect(std::bind(&WsStreaming::onRemoteSignalMetadataChanged, this, entry));
    entry->onDataReceived       = signal->on_data_received      .connect(std::bind(&WsStreaming::onRemoteSignalDataReceived,    this, entry, _1, _2, _3, _4));
    entry->onUnsubscribed       = signal->on_unsubscribed       .connect(std::bind(&WsStreaming::onRemoteSignalUnsubscribed,    this, entry));

    signals[signal->id()] = std::move(entry);

    // Do not immediately register the new signal with openDAQ. We need its metadata first so
    // we can make an openDAQ descriptor. Do an initial subscribe to get that metadata.
    signal->subscribe();
}

void WsStreaming::onRemoteSignalSubscribed(std::shared_ptr<WsStreamingRemoteSignalEntry> entry)
{
    LOG_I("Signal subscribed: {}", entry->ptr->id());

    if (entry->isPublished)
    {
        entry->isSubscribed = true;
        triggerSubscribeAck(entry->ptr->id(), true);
    }
}

void WsStreaming::onRemoteSignalMetadataChanged(std::shared_ptr<WsStreamingRemoteSignalEntry> entry)
{
    LOG_I("Signal metadata changed: {}", entry->ptr->id());

    try
    {
        entry->lastPacket = nullptr;
        entry->descriptor = metadataToDescriptor(entry->ptr->metadata());

        std::string tableId = entry->ptr->metadata().table_id();

        if (auto it = signals.find(tableId); it != signals.end()
                && it->second != entry)
        {
            entry->domainEntry = it->second;
            LOG_I("Signal {} domain now points to {}", entry->ptr->id(), entry->domainEntry->ptr->id());
        }
        else
        {
            LOG_I("Signal {} domain now points to nullptr", entry->ptr->id());
            entry->domainEntry = nullptr;
        }
    }

    catch (const std::exception& ex)
    {
        LOG_I("Cannot understand new metadata for signal {}: {}: {}",
            entry->ptr->id(), ex.what(), entry->ptr->metadata().json().dump());
        entry->descriptor = nullptr;
        entry->domainEntry = nullptr;
    }

    if (entry->descriptor.assigned() && !entry->isPublished)
    {
        LOG_I("Signal {} is now ready, publishing it", entry->ptr->id());
        entry->isPublished = true;
        addToAvailableSignals(entry->ptr->id());
        onSignalAvailable(entry->ptr, entry->descriptor,
            entry->domainEntry ? entry->domainEntry->descriptor : nullptr);
        entry->ptr->unsubscribe();
    }
}

void WsStreaming::onRemoteSignalDataReceived(
    std::shared_ptr<WsStreamingRemoteSignalEntry> entry,
    std::int64_t domainValue,
    std::size_t sampleCount,
    const void *data,
    std::size_t size)
{
    if (!entry->isSubscribed)
        return;

    DataPacketPtr domainPacket;
    DataPacketPtr packet;

    if (entry->domainEntry
        && entry->domainEntry->descriptor.assigned())
    {
        if (entry->domainEntry->descriptor.getRule().assigned()
            && entry->domainEntry->descriptor.getRule().getType() == DataRuleType::Linear
            && (
                !entry->domainEntry->lastPacket.assigned()
                || !entry->domainEntry->lastPacket.getOffset().assigned()
                || entry->domainEntry->lastPacket.getOffset() != domainValue))
        {
            domainPacket = DataPacket(
                entry->domainEntry->descriptor,
                sampleCount,
                domainValue);

            entry->domainEntry->lastPacket = domainPacket;
            onPacket(entry->domainEntry->ptr->id(), domainPacket);
        }

        else
        {
            domainPacket = entry->domainEntry->lastPacket;
        }
    }

    if (domainPacket.assigned())
        packet = DataPacketWithDomain(
            domainPacket,
            entry->descriptor,
            sampleCount);
    else
        packet = DataPacket(entry->descriptor, sampleCount);

    std::memcpy(
        packet.getRawData(),
        data,
        std::min(
            size,
            packet.getRawDataSize()));

    entry->lastPacket = packet;
    onPacket(entry->ptr->id(), packet);
}

void WsStreaming::onRemoteSignalUnsubscribed(std::shared_ptr<WsStreamingRemoteSignalEntry> entry)
{
    if (entry->isSubscribed)
    {
        entry->isSubscribed = false;
        triggerSubscribeAck(entry->ptr->id(), false);
    }
}

void WsStreaming::onRemoteSignalUnavailable(wss::remote_signal_ptr signal)
{
    if (auto signalIt = signals.find(signal->id()); signalIt != signals.end())
    {
        signals.erase(signalIt);
        removeFromAvailableSignals(signal->id());
    }

    onSignalUnavailable(signal);
}

END_NAMESPACE_OPENDAQ_WEBSOCKET_STREAMING
