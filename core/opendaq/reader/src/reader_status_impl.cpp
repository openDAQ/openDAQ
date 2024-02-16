#include <opendaq/reader_status_impl.h>
#include <coretypes/validation.h>
#include <coretypes/common.h>

BEGIN_NAMESPACE_OPENDAQ

ReaderStatusImpl::ReaderStatusImpl(const EventPacketPtr& eventPacket, Bool convertable)
    : eventPacket(eventPacket)
    , convertable(convertable)
{
}

ErrCode ReaderStatusImpl::isOk(Bool* status)
{
    OPENDAQ_PARAM_NOT_NULL(status);
    *status = !eventPacket.assigned() && convertable;
    return OPENDAQ_SUCCESS;
}

ErrCode ReaderStatusImpl::getEventPacket(IEventPacket** packet)
{
    OPENDAQ_PARAM_NOT_NULL(packet);
    *packet = eventPacket.addRefAndReturn();
    return OPENDAQ_SUCCESS;
}

ErrCode ReaderStatusImpl::isEventEncountered(Bool* status)
{
    OPENDAQ_PARAM_NOT_NULL(status);
    *status = eventPacket.assigned();
    return OPENDAQ_SUCCESS;
}

ErrCode ReaderStatusImpl::isConvertable(Bool* status)
{
    OPENDAQ_PARAM_NOT_NULL(status);
    *status = convertable;
    return OPENDAQ_SUCCESS;
}


OPENDAQ_DEFINE_CLASS_FACTORY (
    LIBRARY_FACTORY, ReaderStatus,
    IEventPacket*, eventPacket,
    Bool, convertable
)

END_NAMESPACE_OPENDAQ
