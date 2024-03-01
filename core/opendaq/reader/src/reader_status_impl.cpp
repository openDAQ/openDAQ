#include <opendaq/reader_status_impl.h>
#include <coretypes/validation.h>
#include <coretypes/common.h>

BEGIN_NAMESPACE_OPENDAQ

ReaderStatusImpl::ReaderStatusImpl(const EventPacketPtr& eventPacket, Bool valid)
    : eventPacket(eventPacket)
    , valid(valid)
{
}

ErrCode ReaderStatusImpl::getReadStatus(ReadStatus* status)
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

ErrCode ReaderStatusImpl::getEventPacket(IEventPacket** packet)
{
    OPENDAQ_PARAM_NOT_NULL(packet);
    *packet = eventPacket.addRefAndReturn();
    return OPENDAQ_SUCCESS;
}

ErrCode ReaderStatusImpl::getValid(Bool* valid)
{
    OPENDAQ_PARAM_NOT_NULL(valid);
    *valid = this->valid;
    return OPENDAQ_SUCCESS;
}

OPENDAQ_DEFINE_CLASS_FACTORY (
    LIBRARY_FACTORY, ReaderStatus,
    IEventPacket*, eventPacket,
    Bool, valid
)

END_NAMESPACE_OPENDAQ
