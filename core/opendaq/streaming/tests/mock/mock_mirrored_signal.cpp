#include "mock_mirrored_signal.h"

using namespace daq;

MockMirroredSignalImpl::MockMirroredSignalImpl(const ContextPtr& ctx,
                                               const ComponentPtr& parent,
                                               const StringPtr& localId)
    : MirroredSignal(ctx, parent, localId, nullptr)
    , streamingId(localId)
{
}

StringPtr MockMirroredSignalImpl::onGetRemoteId() const
{
    return streamingId;
}

SignalPtr MockMirroredSignalImpl::onGetDomainSignal()
{
    return mirroredDomainSignal;
}

Bool MockMirroredSignalImpl::onTriggerEvent(const EventPacketPtr& eventPacket)
{
    return MirroredSignal::onTriggerEvent(eventPacket);
}
