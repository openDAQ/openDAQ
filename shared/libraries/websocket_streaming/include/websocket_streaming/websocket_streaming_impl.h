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
#include <opendaq/streaming_impl.h>
#include "websocket_streaming/streaming_client.h"
#include <opendaq/event_packet_ptr.h>

#include <map>

BEGIN_NAMESPACE_OPENDAQ_WEBSOCKET_STREAMING

class WebsocketStreamingImpl : public Streaming
{
public:
    explicit WebsocketStreamingImpl(const StringPtr& connectionString,
                                    const ContextPtr& context);

    explicit WebsocketStreamingImpl(StreamingClientPtr streamingClient,
                                    const StringPtr& connectionString,
                                    const ContextPtr& context);
protected:
    void onSetActive(bool active) override;
    StringPtr onAddSignal(const MirroredSignalConfigPtr& signal) override;
    void onRemoveSignal(const MirroredSignalConfigPtr& signal) override;
    void onSubscribeSignal(const MirroredSignalConfigPtr& signal) override;
    void onUnsubscribeSignal(const MirroredSignalConfigPtr& signal) override;
    EventPacketPtr onCreateDataDescriptorChangedEventPacket(const MirroredSignalConfigPtr& signal) override;

    void prepareStreamingClient();
    void handleEventPacket(const MirroredSignalConfigPtr& signal, const EventPacketPtr& eventPacket);
    void onPacket(const StringPtr& signalId, const PacketPtr& packet);
    void onAvailableSignals(const std::vector<std::string>& signalIds);
    StringPtr getSignalStreamingId(const MirroredSignalConfigPtr& signal);

    daq::websocket_streaming::StreamingClientPtr streamingClient;
    std::vector<StringPtr> availableSignalIds;
};

END_NAMESPACE_OPENDAQ_WEBSOCKET_STREAMING
