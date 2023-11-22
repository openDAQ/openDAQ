#include "mock_mirrored_signal.h"

using namespace daq;

MockMirroredSignalImpl::MockMirroredSignalImpl(const ContextPtr& ctx,
                                               const ComponentPtr& parent,
                                               const StringPtr& localId,
                                               const ComponentStandardProps propsMode)
    : MirroredSignal(ctx, parent, localId, nullptr, propsMode)
    , streamingId(localId)
{
}

StringPtr MockMirroredSignalImpl::onGetRemoteId() const
{
    return streamingId;
}

Bool MockMirroredSignalImpl::onTriggerEvent(EventPacketPtr eventPacket)
{
    return False;
}
