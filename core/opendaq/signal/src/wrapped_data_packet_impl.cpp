#include <opendaq/wrapped_data_packet_impl.h>

BEGIN_NAMESPACE_OPENDAQ

WrappedDataPacketImpl::WrappedDataPacketImpl(IDataPacket* sourcePacket, IDataDescriptor* descriptor)
    : Super(PacketDetails::CreatePacketNoMemoryTag{},
            nullptr,
            descriptor,
            DataPacketPtr::Borrow(sourcePacket).getSampleCount(),
            nullptr)
    , sourcePacket(sourcePacket)
{
#ifdef OPENDAQ_ENABLE_PARAMETER_VALIDATION
    if (sourcePacket == nullptr)
        DAQ_THROW_EXCEPTION(ArgumentNullException, "Source packet is null.");

    const DataDescriptorPtr sourceDataDescriptor = this->sourcePacket.getDataDescriptor();

    if (sourceDataDescriptor.getRawSampleSize() != this->rawSampleSize)
        DAQ_THROW_EXCEPTION(ArgumentNullException, "Raw data does not match.");
#endif

    checkErrorInfo(sourcePacket->getRawData(&data));
}

ErrCode WrappedDataPacketImpl::getDomainPacket(IDataPacket** packet)
{
    assert(sourcePacket.assigned());
    return sourcePacket->getDomainPacket(packet);
}

ErrCode WrappedDataPacketImpl::getOffset(INumber** offset)
{
    assert(sourcePacket.assigned());
    return sourcePacket->getOffset(offset);
}

ErrCode WrappedDataPacketImpl::getSourcePacket(IDataPacket** sourcePacket)
{
    OPENDAQ_PARAM_NOT_NULL(sourcePacket);

    *sourcePacket = this->sourcePacket.addRefAndReturn();
    return OPENDAQ_SUCCESS;
}

void WrappedDataPacketImpl::internalDispose(bool disposing)
{
    Super::internalDispose(disposing);

    sourcePacket.release();
}

OPENDAQ_DEFINE_CLASS_FACTORY_WITH_INTERFACE_AND_CREATEFUNC_OBJ(LIBRARY_FACTORY,
                                                               WrappedDataPacketImpl,
                                                               IDataPacket,
                                                               createWrappedDataPacket,
                                                               IDataPacket*,
                                                               sourcePacket,
                                                               IDataDescriptor*,
                                                               descriptor)

END_NAMESPACE_OPENDAQ
