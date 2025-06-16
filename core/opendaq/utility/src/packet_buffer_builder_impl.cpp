#include <opendaq/packet_buffer_builder_impl.h>

BEGIN_NAMESPACE_OPENDAQ

PacketBufferBuilderImpl::PacketBufferBuilderImpl(SizeT sampleSize, SizeT sampleSizeInMilliseconds, ContextPtr* context)
    : sampleSize(sampleSize)
    , sizeInMilliseconds(sampleSizeInMilliseconds)
    , context(*context)
{
}

ErrCode PacketBufferBuilderImpl::getSampleSize(SizeT* sampleSize)
{
    return OPENDAQ_SUCCESS;
}

ErrCode PacketBufferBuilderImpl::setSampleSize(SizeT sampleSize)
{
    return OPENDAQ_SUCCESS;
}

ErrCode PacketBufferBuilderImpl::getSampleSizeInMilliseconds(SizeT* sizeInMilliseconds)
{
    return OPENDAQ_SUCCESS;
}

ErrCode PacketBufferBuilderImpl::setSampleSizeInMilliseconds(SizeT sizeInMilliseconds)
{
    return OPENDAQ_SUCCESS;
}

ErrCode PacketBufferBuilderImpl::getContext(ContextPtr** context)
{
    return OPENDAQ_SUCCESS;
}

ErrCode PacketBufferBuilderImpl::setContext(ContextPtr* context)
{
    return OPENDAQ_SUCCESS;
}

ErrCode PacketBufferBuilderImpl::getDescription(DataDescriptorPtr** descriptor)
{
    return OPENDAQ_SUCCESS;
}

ErrCode PacketBufferBuilderImpl::setDescription(DataDescriptorPtr* descriptor)
{
    return OPENDAQ_SUCCESS;
}

ErrCode PacketBufferBuilderImpl::build(IPacketBuffer** buffer)
{
    return OPENDAQ_SUCCESS;
}

OPENDAQ_DECLARE_CLASS_FACTORY_WITH_INTERFACE(LIBRARY_FACTORY, PacketBufferBuilder, IPacketBufferBuilder*)




END_NAMESPACE_OPENDAQ
