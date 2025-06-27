#include <opendaq/packet_buffer_builder_impl.h>
#include <opendaq/packet_buffer_factory.h>

BEGIN_NAMESPACE_OPENDAQ

PacketBufferBuilderImpl::PacketBufferBuilderImpl()
    : sampleCount(100)
    , sampleSize(8)
    , sizeInMilliseconds(10)
{
}

ErrCode PacketBufferBuilderImpl::getContext(IContext** context)
{
    *context = this->context.addRefAndReturn();
    return OPENDAQ_SUCCESS;
}

ErrCode PacketBufferBuilderImpl::setContext(IContext* context)
{
    OPENDAQ_PARAM_NOT_NULL(context);
    this->context = context;
    return OPENDAQ_SUCCESS;
}

ErrCode PacketBufferBuilderImpl::getSizeInBytes(SizeT* sampleCount)
{
    *sampleCount = this->sampleCount * this->sampleSize;
    return OPENDAQ_SUCCESS;
}

ErrCode PacketBufferBuilderImpl::setSizeInBytes(SizeT sampleCount)
{
    this->sampleCount = sampleCount;
    return OPENDAQ_SUCCESS;
}

ErrCode PacketBufferBuilderImpl::getRawSampleSize(SizeT* rawSampleSize)
{
    *rawSampleSize = this->sampleSize;
    return OPENDAQ_SUCCESS;
}

ErrCode PacketBufferBuilderImpl::setRawSampleSize(SizeT rawSampleSize)
{
    this->sampleSize = rawSampleSize;
    return OPENDAQ_SUCCESS;
}

ErrCode PacketBufferBuilderImpl::build(IPacketBuffer** buffer)
{
    OPENDAQ_PARAM_NOT_NULL(buffer);
    const auto builder = this->borrowPtr<PacketBufferBuilderPtr>();

    return daqTry(
        [&]()
        {
            *buffer = PacketBuffer(builder).detach();
            return OPENDAQ_SUCCESS;
        });
}

OPENDAQ_DEFINE_CLASS_FACTORY(LIBRARY_FACTORY, PacketBufferBuilder)

END_NAMESPACE_OPENDAQ
