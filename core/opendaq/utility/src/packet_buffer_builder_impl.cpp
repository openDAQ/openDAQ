#include <opendaq/packet_buffer_builder_impl.h>
#include <opendaq/packet_buffer_factory.h>

BEGIN_NAMESPACE_OPENDAQ

PacketBufferBuilderImpl::PacketBufferBuilderImpl()
    : sizeInBytes(0)
{
}

ErrCode PacketBufferBuilderImpl::getContext(IContext** context)
{
    OPENDAQ_PARAM_NOT_NULL(context);

    *context = this->context.addRefAndReturn();
    return OPENDAQ_SUCCESS;
}

ErrCode PacketBufferBuilderImpl::setContext(IContext* context)
{
    this->context = context;
    return OPENDAQ_SUCCESS;
}

ErrCode PacketBufferBuilderImpl::getSizeInBytes(SizeT* sizeInBytes)
{
    OPENDAQ_PARAM_NOT_NULL(sizeInBytes);

    *sizeInBytes = this->sizeInBytes;
    return OPENDAQ_SUCCESS;
}

ErrCode PacketBufferBuilderImpl::setSizeInBytes(SizeT sizeInBytes)
{
    this->sizeInBytes = sizeInBytes;
    return OPENDAQ_SUCCESS;
}

ErrCode PacketBufferBuilderImpl::build(IPacketBuffer** buffer)
{
    OPENDAQ_PARAM_NOT_NULL(buffer);
    const auto builder = this->borrowPtr<PacketBufferBuilderPtr>();

    if (sizeInBytes == 0)
        return DAQ_MAKE_ERROR_INFO(OPENDAQ_ERR_INVALIDSTATE, "Buffer SizeInBytes parameter must be greater than 0.");

    return daqTry(
        [&]()
        {
            *buffer = PacketBuffer(builder).detach();
            return OPENDAQ_SUCCESS;
        });
}

OPENDAQ_DEFINE_CLASS_FACTORY(LIBRARY_FACTORY, PacketBufferBuilder)

END_NAMESPACE_OPENDAQ
