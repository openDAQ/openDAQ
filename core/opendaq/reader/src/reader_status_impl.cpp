#include <opendaq/reader_status_impl.h>
#include <coretypes/validation.h>
#include <coretypes/common.h>

BEGIN_NAMESPACE_OPENDAQ

template <class MainInterface, class ... Interfaces>
GenericReaderStatusImpl<MainInterface, Interfaces...>::GenericReaderStatusImpl(const EventPacketPtr& eventPacket, Bool valid)
    : eventPacket(eventPacket)
    , valid(valid)
{
}

template <class MainInterface, class ... Interfaces>
ErrCode GenericReaderStatusImpl<MainInterface, Interfaces...>::getReadStatus(ReadStatus* status)
{
    OPENDAQ_PARAM_NOT_NULL(status);
    if (valid && !eventPacket.assigned())
        *status = ReadStatus::Ok;
    else if (eventPacket.assigned())
        *status = ReadStatus::Event;
    else
        *status = ReadStatus::Fail;

    return OPENDAQ_SUCCESS;
}

template <class MainInterface, class ... Interfaces>
ErrCode GenericReaderStatusImpl<MainInterface, Interfaces...>::getEventPacket(IEventPacket** packet)
{
    OPENDAQ_PARAM_NOT_NULL(packet);
    *packet = eventPacket.addRefAndReturn();
    return OPENDAQ_SUCCESS;
}

template <class MainInterface, class ... Interfaces>
ErrCode GenericReaderStatusImpl<MainInterface, Interfaces...>::getValid(Bool* valid)
{
    OPENDAQ_PARAM_NOT_NULL(valid);
    *valid = this->valid;
    return OPENDAQ_SUCCESS;
}

BlockReaderStatusImpl::BlockReaderStatusImpl(const EventPacketPtr& eventPacket, Bool valid, SizeT readSamples)
    : Super(eventPacket, valid)
    , readSamples(readSamples)
{
}

ErrCode BlockReaderStatusImpl::getReadSamples(SizeT* readSamples)
{
    OPENDAQ_PARAM_NOT_NULL(readSamples);
    *readSamples = this->readSamples;
    return OPENDAQ_SUCCESS;
}

OPENDAQ_DEFINE_CLASS_FACTORY (
    LIBRARY_FACTORY, ReaderStatus,
    IEventPacket*, eventPacket,
    Bool, valid
)

OPENDAQ_DEFINE_CLASS_FACTORY (
    LIBRARY_FACTORY, BlockReaderStatus,
    IEventPacket*, eventPacket,
    Bool, valid,
    SizeT, readSamples
)

END_NAMESPACE_OPENDAQ