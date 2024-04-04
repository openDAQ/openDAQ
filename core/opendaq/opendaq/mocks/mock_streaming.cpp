#include "opendaq/mock/mock_streaming.h"
#include <opendaq/packet_factory.h>

using namespace daq;

MockStreamingImpl::MockStreamingImpl(const StringPtr& connectionString, const ContextPtr& context)
    : Streaming(connectionString, context, false)
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

EventPacketPtr MockStreamingImpl::onCreateDataDescriptorChangedEventPacket(const StringPtr& /*signalStreamingId*/)
{
    return DataDescriptorChangedEventPacket(nullptr, nullptr);
}

OPENDAQ_DEFINE_CLASS_FACTORY_WITH_INTERFACE(
    INTERNAL_FACTORY,
    MockStreaming, IStreaming,
    IString*, connectionString,
    IContext*, context
)
