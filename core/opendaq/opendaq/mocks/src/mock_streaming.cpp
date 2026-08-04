#include "opendaq/mock/mock_streaming.h"
#include <opendaq/packet_factory.h>

using namespace daq;

MockStreamingImpl::MockStreamingImpl(const StringPtr& connectionString, const ContextPtr& context)
    : Streaming(connectionString, context, false, "MockStreaming", true)
{
}

MockStreamingImpl::MockStreamingImpl(const StringPtr& connectionString,
                                     const ContextPtr& context,
                                     const StringPtr& protocolId,
                                     const StringPtr& protocolGroupId)
    // client-to-device streaming requires a protocol id, so it is only enabled when one is given
    : Streaming(connectionString,
                context,
                false,
                protocolId,
                protocolId.assigned() && protocolId.getLength() > 0,
                protocolGroupId)
{
}

void MockStreamingImpl::onSetActive(bool /*active*/)
{
}

void MockStreamingImpl::onAddSignal(const MirroredSignalConfigPtr& /*signal*/)
{
}

void MockStreamingImpl::onRemoveSignal(const MirroredSignalConfigPtr& /*signal*/)
{
}

void MockStreamingImpl::onSubscribeSignal(const StringPtr& /*signalStreamingId*/)
{
}

void MockStreamingImpl::onUnsubscribeSignal(const StringPtr& /*signalStreamingId*/)
{
}

void MockStreamingImpl::onRegisterStreamedClientSignal(const daq::SignalPtr& /*signal*/)
{
}

void MockStreamingImpl::onUnregisterStreamedClientSignal(const daq::SignalPtr& /*signal*/)
{
}

OPENDAQ_DEFINE_CLASS_FACTORY_WITH_INTERFACE(
    INTERNAL_FACTORY,
    MockStreaming, IStreaming,
    IString*, connectionString,
    IContext*, context
)

OPENDAQ_DEFINE_CLASS_FACTORY_WITH_INTERFACE_AND_CREATEFUNC_OBJ(
    INTERNAL_FACTORY,
    MockStreamingImpl, IStreaming,
    createMockStreamingWithProtocol,
    IString*, connectionString,
    IContext*, context,
    IString*, protocolId,
    IString*, protocolGroupId
)
