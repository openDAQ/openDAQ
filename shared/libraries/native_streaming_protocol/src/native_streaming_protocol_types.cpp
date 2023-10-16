#include <native_streaming_protocol/native_streaming_protocol_types.h>

BEGIN_NAMESPACE_OPENDAQ_NATIVE_STREAMING_PROTOCOL

TransportHeader::TransportHeader(PayloadType payloadType, size_t payloadSize)
    : payloadType(payloadType)
    , payloadSize(payloadSize)
    , packedHeader(
        (((payloadSize << PAYLOAD_SIZE_SHIFT) & PAYLOAD_SIZE_MASK)) |
        ((static_cast<PackedHeaderType>(payloadType) << PAYLOAD_TYPE_SHIFT) & PAYLOAD_TYPE_MASK)
    )
{
}

TransportHeader::TransportHeader(const PackedHeaderType* packedHeader)
    : payloadType(static_cast<PayloadType>((*packedHeader & PAYLOAD_TYPE_MASK) >> PAYLOAD_TYPE_SHIFT))
    , payloadSize((*packedHeader & PAYLOAD_SIZE_MASK) >> PAYLOAD_SIZE_SHIFT)
    , packedHeader(*packedHeader)
{
}

const PackedHeaderType* TransportHeader::getPackedHeaderPtr() const
{
    return &packedHeader;
}

PayloadType TransportHeader::getPayloadType() const
{
    return payloadType;
}

size_t TransportHeader::getPayloadSize() const
{
    return payloadSize;
}

NativeStreamingProtocolException::NativeStreamingProtocolException(const std::string& msg)
    : runtime_error(msg)
{
}

END_NAMESPACE_OPENDAQ_NATIVE_STREAMING_PROTOCOL
