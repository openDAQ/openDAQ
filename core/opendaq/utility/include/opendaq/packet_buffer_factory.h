#pragma once
#include <opendaq/packet_buffer_ptr.h>
#include <opendaq/packet_buffer_builder_ptr.h>

BEGIN_NAMESPACE_OPENDAQ

inline PacketBufferPtr PacketBuffer(const PacketBufferBuilderPtr builder)
{
    return PacketBufferPtr(PacketBuffer_Create(builder));
}

END_NAMESPACE_OPENDAQ
