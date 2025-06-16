#pragma once
#include <opendaq/packet_buffer.h>
#include <opendaq/packet_buffer_builder.h>

BEGIN_NAMESPACE_OPENDAQ

class PacketBufferBuilderImpl : public ImplementationOf<IPacketBufferBuilder>
{
    PacketBufferBuilderImpl(SizeT sampleSize, SizeT sampleSizeInMilliseconds, ContextPtr* context);

    ErrCode INTERFACE_FUNC getSampleSize(SizeT* sampleSize) override;
    ErrCode INTERFACE_FUNC setSampleSize(SizeT sampleSize) override;

    ErrCode INTERFACE_FUNC getSampleSizeInMilliseconds(SizeT* sizeInMilliseconds) override;
    ErrCode INTERFACE_FUNC setSampleSizeInMilliseconds(SizeT sizeInMilliseconds) override;

    ErrCode INTERFACE_FUNC getContext(ContextPtr** context) override;
    ErrCode INTERFACE_FUNC setContext(ContextPtr* context) override;

    ErrCode INTERFACE_FUNC getDescription(DataDescriptorPtr** descriptor) override;
    ErrCode INTERFACE_FUNC setDescription(DataDescriptorPtr* descriptor) override;

    ErrCode INTERFACE_FUNC build(IPacketBuffer** buffer) override;

private:
    SizeT sampleSize;
    SizeT sizeInMilliseconds;
    ContextPtr context;
    DataDescriptorPtr description;
};

inline PacketBufferBuilderImpl::PacketBufferBuilderImpl(SizeT sampleSize, SizeT sampleSizeInMilliseconds, ContextPtr* context)
    : sampleSize(sampleSize)
    , sizeInMilliseconds(sampleSizeInMilliseconds)
    , context(*context)
{
}

END_NAMESPACE_OPENDAQ
