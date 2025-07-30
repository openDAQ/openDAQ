#include "opendaq/mock/mock_streaming.h"
#include <opendaq/packet_factory.h>

using namespace daq;

MockStreamingImpl::MockStreamingImpl(const StringPtr& connectionString, const ContextPtr& context)
    : Streaming(connectionString, context, false, "MockStreaming")
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

void MockStreamingImpl::onRegisterStreamedSignal(const daq::SignalPtr& /*signal*/) {}
void MockStreamingImpl::onUnregisterStreamedSignal(const daq::SignalPtr& /*signal*/) {}

OPENDAQ_DEFINE_CLASS_FACTORY_WITH_INTERFACE(
    INTERNAL_FACTORY,
    MockStreaming, IStreaming,
    IString*, connectionString,
    IContext*, context
)
