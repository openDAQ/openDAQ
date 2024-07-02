#include <opendaq/reader_status_impl.h>
#include <coretypes/validation.h>
#include <coretypes/common.h>

BEGIN_NAMESPACE_OPENDAQ

template <class MainInterface, class ... Interfaces>
GenericReaderStatusImpl<MainInterface, Interfaces...>::GenericReaderStatusImpl(const EventPacketPtr& eventPacket, Bool valid, const NumberPtr& offset)
    : eventPacket(eventPacket)
    , valid(valid)
    , offset(offset.assigned() ? offset : NumberPtr(0))
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

template <class MainInterface, class ... Interfaces>
ErrCode GenericReaderStatusImpl<MainInterface, Interfaces...>::getOffset(INumber** offset)
{
    OPENDAQ_PARAM_NOT_NULL(offset);
    *offset = this->offset.addRefAndReturn();
    return OPENDAQ_SUCCESS;
}

BlockReaderStatusImpl::BlockReaderStatusImpl(const EventPacketPtr& eventPacket, Bool valid, const NumberPtr& offset, SizeT readSamples)
    : Super(eventPacket, valid, offset)
    , readSamples(readSamples)
{
}

ErrCode BlockReaderStatusImpl::getReadSamples(SizeT* readSamples)
{
    OPENDAQ_PARAM_NOT_NULL(readSamples);
    *readSamples = this->readSamples;
    return OPENDAQ_SUCCESS;
}

TailReaderStatusImpl::TailReaderStatusImpl(const EventPacketPtr& eventPacket, Bool valid, const NumberPtr& offset, Bool sufficientHistory)
    : Super(eventPacket, valid, offset)
    , sufficientHistory(sufficientHistory)
{
}

ErrCode TailReaderStatusImpl::getReadStatus(ReadStatus* status)
{
    OPENDAQ_PARAM_NOT_NULL(status);
    if (!sufficientHistory)
    {
        *status = ReadStatus::Fail;
        return OPENDAQ_SUCCESS;
    }
    return Super::getReadStatus(status);
}

ErrCode TailReaderStatusImpl::getSufficientHistory(Bool* status)
{
    OPENDAQ_PARAM_NOT_NULL(status);
    *status = sufficientHistory;
    return OPENDAQ_SUCCESS;
}

MultiReaderStatusImpl::MultiReaderStatusImpl(const DictPtr<IString, IEventPacket>& eventPackets, Bool valid, const NumberPtr& offset)
    : Super(nullptr, valid, offset)
    , eventPackets(eventPackets.assigned() ? eventPackets : Dict<IString, IEventPacket>())
{
}

ErrCode MultiReaderStatusImpl::getReadStatus(ReadStatus* status)
{
    OPENDAQ_PARAM_NOT_NULL(status);
    Bool valid;
    Super::getValid(&valid);

    if (valid && (eventPackets.getCount() == 0))
        *status = ReadStatus::Ok;
    else if (eventPackets.assigned())
        *status = ReadStatus::Event;
    else
        *status = ReadStatus::Fail;

    return OPENDAQ_SUCCESS;
}

ErrCode MultiReaderStatusImpl::getEventPackets(IDict** events)
{
    OPENDAQ_PARAM_NOT_NULL(events);
    *events = eventPackets.addRefAndReturn();
    return OPENDAQ_SUCCESS;
}

OPENDAQ_DEFINE_CLASS_FACTORY (
    LIBRARY_FACTORY, ReaderStatus,
    IEventPacket*, eventPacket,
    Bool, valid,
    INumber*, offset
)

OPENDAQ_DEFINE_CLASS_FACTORY (
    LIBRARY_FACTORY, BlockReaderStatus,
    IEventPacket*, eventPacket,
    Bool, valid,
    INumber*, offset,
    SizeT, readSamples
)

OPENDAQ_DEFINE_CLASS_FACTORY (
    LIBRARY_FACTORY, TailReaderStatus,
    IEventPacket*, eventPacket,
    Bool, valid,
    INumber*, offset,
    Bool, sufficientHistory
)

OPENDAQ_DEFINE_CLASS_FACTORY (
    LIBRARY_FACTORY, MultiReaderStatus,
    IDict*, eventPackets,
    Bool, valid,
    INumber*, offset
)

END_NAMESPACE_OPENDAQ
