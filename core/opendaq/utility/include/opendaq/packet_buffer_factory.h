#pragma once
#include <opendaq/packet_buffer_ptr.h>

BEGIN_NAMESPACE_OPENDAQ

inline PacketBufferPtr PacketBuffer()
{
    return PacketBufferPtr(PacketBuffer_Create(PacketBufferBuilderPtr()));
}

END_NAMESPACE_OPENDAQ
