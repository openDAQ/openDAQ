#pragma once
#include <opendaq/opendaq.h>
#include <opendaq/packet_buffer.h>

BEGIN_NAMESPACE_OPENDAQ

DECLARE_OPENDAQ_INTERFACE(IPacketBufferBuilder, IBaseObject)
{
    // Getters and setters for init params

    virtual ErrCode INTERFACE_FUNC getSampleSize(SizeT* sampleSize) = 0;
    virtual ErrCode INTERFACE_FUNC setSampleSize(SizeT sampleSize) = 0;

    virtual ErrCode INTERFACE_FUNC getSampleSizeInMilliseconds(SizeT* sizeInMilliseconds) = 0;
    virtual ErrCode INTERFACE_FUNC setSampleSizeInMilliseconds(SizeT sizeInMilliseconds) = 0;

    virtual ErrCode INTERFACE_FUNC getContext(ContextPtr** context) = 0;
    virtual ErrCode INTERFACE_FUNC setContext(ContextPtr* context) = 0;

    virtual ErrCode INTERFACE_FUNC getDescription(DataDescriptorPtr** context) = 0;
    virtual ErrCode INTERFACE_FUNC setDescription(DataDescriptorPtr* context) = 0;

    virtual ErrCode INTERFACE_FUNC build(IPacketBuffer** buffer) = 0;
};

// Factories for init
OPENDAQ_DECLARE_CLASS_FACTORY_WITH_INTERFACE(LIBRARY_FACTORY, PacketBufferBuilder, IPacketBufferBuilder*)


END_NAMESPACE_OPENDAQ
