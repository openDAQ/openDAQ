#pragma once
#include <opendaq/packet_buffer.h>

BEGIN_NAMESPACE_OPENDAQ

DECLARE_OPENDAQ_INTERFACE(IPacketBufferBuilder, IBaseObject)
{
    // Getters and setters for init params

    virtual ErrCode INTERFACE_FUNC getContext(IContext** context) = 0;
    virtual ErrCode INTERFACE_FUNC setContext(IContext* context) = 0;

    virtual ErrCode INTERFACE_FUNC getSizeInBytes(SizeT* sampleCount) = 0;
    virtual ErrCode INTERFACE_FUNC setSizeInBytes(SizeT sampleCount) = 0;

    virtual ErrCode INTERFACE_FUNC build(IPacketBuffer** buffer) = 0;
};

// Factories for init
OPENDAQ_DECLARE_CLASS_FACTORY(LIBRARY_FACTORY, PacketBufferBuilder)


END_NAMESPACE_OPENDAQ
