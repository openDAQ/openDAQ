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

#include <coretypes/common.h>
#include <coretypes/string_ptr.h>

#include <opendaq/data_descriptor_ptr.h>
#include <opendaq/packet_ptr.h>
#include <coreobjects/property_object_ptr.h>

#include <native_streaming_protocol/native_streaming_protocol.h>

#include <native_streaming/session.hpp>
#include <config_protocol/config_protocol.h>

#include <functional>

BEGIN_NAMESPACE_OPENDAQ_NATIVE_STREAMING_PROTOCOL

using SessionPtr = std::shared_ptr<daq::native_streaming::Session>;
using SignalNumericIdType = uint32_t;

using OnSignalCallback = std::function<void(const SignalNumericIdType& signalNumericId,
                                            const StringPtr& signalStringId,
                                            const StringPtr& serializedSignal,
                                            bool available)>;

using OnSignalSubscriptionCallback = std::function<bool(const SignalNumericIdType& signalNumericId,
                                                        const std::string& signalStringId,
                                                        bool subscribed,
                                                        SessionPtr session)>;

using OnProtocolInitDoneCallback = std::function<void()>;

using OnSubscriptionAckCallback = std::function<void(const SignalNumericIdType& signalNumericId,
                                                     bool subscribed)>;

using OnPacketReceivedCallback = std::function<void(const SignalNumericIdType& signalNumericId,
                                                    const PacketPtr& packet)>;

using ConfigProtocolPacketCb = std::function<void(const config_protocol::PacketBuffer& packetBuffer)>;

using OnTrasportLayerPropertiesCallback = std::function<void(const PropertyObjectPtr& propertyObject)>;

enum class PayloadType
{
    PAYLOAD_TYPE_STREAMING_PACKET = 1,
    PAYLOAD_TYPE_STREAMING_SIGNAL_AVAILABLE = 2,
    PAYLOAD_TYPE_STREAMING_SIGNAL_UNAVAILABLE = 3,
    PAYLOAD_TYPE_STREAMING_SIGNAL_SUBSCRIBE_COMMAND = 4,
    PAYLOAD_TYPE_STREAMING_SIGNAL_UNSUBSCRIBE_COMMAND = 5,
    PAYLOAD_TYPE_STREAMING_PROTOCOL_INIT_DONE = 6,
    PAYLOAD_TYPE_STREAMING_SIGNAL_SUBSCRIBE_ACK = 7,
    PAYLOAD_TYPE_STREAMING_SIGNAL_UNSUBSCRIBE_ACK = 8,
    PAYLOAD_TYPE_CONFIGURATION_PACKET = 9,
    PAYLOAD_TYPE_TRANSPORT_LAYER_PROPERTIES = 10
};

constexpr std::initializer_list<PayloadType> allPayloadTypes =
    {
        PayloadType::PAYLOAD_TYPE_STREAMING_PACKET,
        PayloadType::PAYLOAD_TYPE_STREAMING_SIGNAL_AVAILABLE,
        PayloadType::PAYLOAD_TYPE_STREAMING_SIGNAL_UNAVAILABLE,
        PayloadType::PAYLOAD_TYPE_STREAMING_SIGNAL_SUBSCRIBE_COMMAND,
        PayloadType::PAYLOAD_TYPE_STREAMING_SIGNAL_UNSUBSCRIBE_COMMAND,
        PayloadType::PAYLOAD_TYPE_STREAMING_PROTOCOL_INIT_DONE,
        PayloadType::PAYLOAD_TYPE_STREAMING_SIGNAL_SUBSCRIBE_ACK,
        PayloadType::PAYLOAD_TYPE_STREAMING_SIGNAL_UNSUBSCRIBE_ACK,
        PayloadType::PAYLOAD_TYPE_CONFIGURATION_PACKET,
        PayloadType::PAYLOAD_TYPE_TRANSPORT_LAYER_PROPERTIES
    };

inline std::string convertPayloadTypeToString(PayloadType type)
{
    switch (type)
    {
        case PayloadType::PAYLOAD_TYPE_STREAMING_PACKET:
            return "PAYLOAD_TYPE_STREAMING_PACKET";
        case PayloadType::PAYLOAD_TYPE_STREAMING_SIGNAL_AVAILABLE:
            return "PAYLOAD_TYPE_STREAMING_SIGNAL_AVAILABLE";
        case PayloadType::PAYLOAD_TYPE_STREAMING_SIGNAL_UNAVAILABLE:
            return "PAYLOAD_TYPE_STREAMING_SIGNAL_UNAVAILABLE";
        case PayloadType::PAYLOAD_TYPE_STREAMING_SIGNAL_SUBSCRIBE_COMMAND:
            return "PAYLOAD_TYPE_STREAMING_SIGNAL_SUBSCRIBE_COMMAND";
        case PayloadType::PAYLOAD_TYPE_STREAMING_SIGNAL_UNSUBSCRIBE_COMMAND:
            return "PAYLOAD_TYPE_STREAMING_SIGNAL_UNSUBSCRIBE_COMMAND";
        case PayloadType::PAYLOAD_TYPE_STREAMING_PROTOCOL_INIT_DONE:
            return "PAYLOAD_TYPE_STREAMING_PROTOCOL_INIT_DONE";
        case PayloadType::PAYLOAD_TYPE_STREAMING_SIGNAL_SUBSCRIBE_ACK:
            return "PAYLOAD_TYPE_STREAMING_SIGNAL_SUBSCRIBE_ACK";
        case PayloadType::PAYLOAD_TYPE_STREAMING_SIGNAL_UNSUBSCRIBE_ACK:
            return "PAYLOAD_TYPE_STREAMING_SIGNAL_UNSUBSCRIBE_ACK";
        case PayloadType::PAYLOAD_TYPE_CONFIGURATION_PACKET:
            return "PAYLOAD_TYPE_CONFIGURATION_PACKET";
        case PayloadType::PAYLOAD_TYPE_TRANSPORT_LAYER_PROPERTIES:
            return "PAYLOAD_TYPE_TRANSPORT_LAYER_PROPERTIES";
    }

    return "PAYLOAD_TYPE_INVALID";
}

using PackedHeaderType = uint32_t;

class TransportHeader
{
    static constexpr PackedHeaderType PAYLOAD_TYPE_MASK = 0xF0000000;
    static constexpr PackedHeaderType PAYLOAD_SIZE_MASK = 0x0FFFFFFF;
    static constexpr uint8_t PAYLOAD_TYPE_SHIFT = 28;
    static constexpr uint8_t PAYLOAD_SIZE_SHIFT = 0;

public:
    static constexpr size_t PACKED_HEADER_SIZE = sizeof(PackedHeaderType);
    static constexpr size_t MAX_PAYLOAD_SIZE = PAYLOAD_SIZE_MASK;

    explicit TransportHeader(PayloadType payloadType, size_t payloadSize);
    explicit TransportHeader(const PackedHeaderType* packedHeader);

    const PackedHeaderType* getPackedHeaderPtr() const;
    PayloadType getPayloadType() const;
    size_t getPayloadSize() const;

private:
    PayloadType payloadType;
    size_t payloadSize;
    PackedHeaderType packedHeader;
};

class NativeStreamingProtocolException : public std::runtime_error
{
public:
    NativeStreamingProtocolException(const std::string& msg);
};

END_NAMESPACE_OPENDAQ_NATIVE_STREAMING_PROTOCOL
