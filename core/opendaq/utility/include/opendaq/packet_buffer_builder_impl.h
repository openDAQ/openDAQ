#pragma once
#include <opendaq/packet_buffer_builder.h>

BEGIN_NAMESPACE_OPENDAQ

class PacketBufferBuilderImpl : public ImplementationOf<IPacketBufferBuilder>
{
public:

    PacketBufferBuilderImpl();

    ErrCode INTERFACE_FUNC getContext(IContext** context) override;
    ErrCode INTERFACE_FUNC setContext(IContext* context) override;

    ErrCode INTERFACE_FUNC getSizeInBytes(SizeT* sampleCount) override;
    ErrCode INTERFACE_FUNC setSizeInBytes(SizeT sampleCount) override;

    ErrCode INTERFACE_FUNC build(IPacketBuffer** buffer) override;

private:

    SizeT sampleCount;
    SizeT sampleSize;
    SizeT sizeInMilliseconds;
    ContextPtr context;
    DataDescriptorPtr descriptor;
};

END_NAMESPACE_OPENDAQ
