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

#include <opendaq/data_descriptor_ptr.h>

#include <packet_streaming/packet_streaming_client.h>

BEGIN_NAMESPACE_OPENDAQ_NATIVE_STREAMING_PROTOCOL

class ClientSessionHandler : public BaseSessionHandler
{
public:
    ClientSessionHandler(const ContextPtr& daqContext,
                         boost::asio::io_context& ioContext,
                         SessionPtr session,
                         OnSignalCallback signalReceivedHandler,
                         OnPacketReceivedCallback packetReceivedHandler,
                         OnStreamingProtocolInitDoneCallback protocolInitDoneHandler,
                         OnSubscriptionAckCallback subscriptionAckHandler,
                         native_streaming::OnSessionErrorCallback errorHandler);

    void sendSignalSubscribe(const SignalNumericIdType& signalNumericId, const std::string& signalStringId);
    void sendSignalUnsubscribe(const SignalNumericIdType& signalNumericId, const std::string& signalStringId);
    void sendTransportLayerProperties(const PropertyObjectPtr& properties);

    EventPacketPtr getDataDescriptorChangedEventPacket(const SignalNumericIdType& signalNumericId);

private:
    daq::native_streaming::ReadTask readHeader(const void* data, size_t size) override;

    daq::native_streaming::ReadTask readPacket(const void* data, size_t size);
    daq::native_streaming::ReadTask readSignalAvailable(const void* data, size_t size);
    daq::native_streaming::ReadTask readSignalUnavailable(const void* data, size_t size);
    daq::native_streaming::ReadTask readSignalSubscribedAck(const void* data, size_t size);
    daq::native_streaming::ReadTask readSignalUnsubscribedAck(const void* data, size_t size);

    void processReceivedPackets();

    OnSignalCallback signalReceivedHandler;
    OnPacketReceivedCallback packetReceivedHandler;
    OnStreamingProtocolInitDoneCallback protocolInitDoneHandler;
    OnSubscriptionAckCallback subscriptionAckHandler;

    packet_streaming::PacketStreamingClient packetStreamingClient;
};
END_NAMESPACE_OPENDAQ_NATIVE_STREAMING_PROTOCOL
