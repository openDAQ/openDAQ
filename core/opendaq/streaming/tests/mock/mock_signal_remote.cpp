#include "mock_signal_remote.h"

using namespace daq;

MockSignalRemoteImpl::MockSignalRemoteImpl(const ContextPtr &ctx, const ComponentPtr &parent, const StringPtr &localId)
    : SignalRemote<SignalStandardProps::Skip>(ctx, parent, localId)
    , streamingId(localId)
{
}

StringPtr MockSignalRemoteImpl::onGetRemoteId() const
{
    return streamingId;
}

Bool MockSignalRemoteImpl::onTriggerEvent(EventPacketPtr eventPacket)
{
    return False;
}
