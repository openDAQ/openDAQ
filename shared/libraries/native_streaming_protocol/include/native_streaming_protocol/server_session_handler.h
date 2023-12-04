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

#include <native_streaming_protocol/base_session_handler.h>

#include <opendaq/context_ptr.h>
#include <opendaq/logger_ptr.h>
#include <opendaq/logger_component_ptr.h>
#include <opendaq/signal_ptr.h>

#include <packet_streaming/packet_streaming_server.h>

BEGIN_NAMESPACE_OPENDAQ_NATIVE_STREAMING_PROTOCOL

class ServerSessionHandler : public BaseSessionHandler
{
public:
    ServerSessionHandler(const ContextPtr& context,
                         SessionPtr session,
                         OnSignalSubscriptionCallback signalSubscriptionHandler,
                         OnErrorCallback errorHandler);

    ~ServerSessionHandler();

    void sendSignalAvailable(const SignalNumericIdType& signalNumericId, const SignalPtr &signal);
    void sendSignalUnavailable(const SignalNumericIdType& signalNumericId, const SignalPtr& signal);
    void sendInitializationDone();
    void sendPacket(const SignalNumericIdType signalId, const PacketPtr& packet);
    void sendSubscribingDone(const SignalNumericIdType signalNumericId);
    void sendUnsubscribingDone(const SignalNumericIdType signalNumericId);

private:
    daq::native_streaming::ReadTask readHeader(const void* data, size_t size) override;

    daq::native_streaming::ReadTask readSignalSubscribe(const void* data, size_t size);
    daq::native_streaming::ReadTask readSignalUnsubscribe(const void* data, size_t size);

    void sendPacketBuffer(const packet_streaming::PacketBufferPtr& packetBuffer);

    OnSignalSubscriptionCallback signalSubscriptionHandler;

    LoggerPtr logger;
    LoggerComponentPtr loggerComponent;

    packet_streaming::PacketStreamingServer packetStreamingServer;
    SerializerPtr jsonSerializer;
};
END_NAMESPACE_OPENDAQ_NATIVE_STREAMING_PROTOCOL
