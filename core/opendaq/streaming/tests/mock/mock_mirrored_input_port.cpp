#include "mock_mirrored_input_port.h"

using namespace daq;

MockMirroredInputPortImpl::MockMirroredInputPortImpl(const ContextPtr& ctx,
                                               const ComponentPtr& parent,
                                               const StringPtr& localId)
    : MirroredInputPort(ctx, parent, localId)
    , remoteId(localId)
{
}

StringPtr MockMirroredInputPortImpl::onGetRemoteId() const
{
    return remoteId;
}
