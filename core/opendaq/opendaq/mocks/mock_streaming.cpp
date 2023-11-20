#include "opendaq/mock/mock_streaming.h"
#include <opendaq/packet_factory.h>

using namespace daq;

MockStreamingImpl::MockStreamingImpl(const StringPtr& connectionString)
    : Streaming(connectionString, nullptr)
{
}

void MockStreamingImpl::onSetActive(bool /*active*/)
{

}

StringPtr MockStreamingImpl::onAddSignal(const MirroredSignalConfigPtr& signal)
{
    return signal.getRemoteId();
}

void MockStreamingImpl::onRemoveSignal(const MirroredSignalConfigPtr& /*signal*/)
{

}

void MockStreamingImpl::onSubscribeSignal(const MirroredSignalConfigPtr& /*signal*/)
{

}

void MockStreamingImpl::onUnsubscribeSignal(const MirroredSignalConfigPtr& /*signal*/)
{

}

EventPacketPtr MockStreamingImpl::onCreateDataDescriptorChangedEventPacket(const daq::MirroredSignalConfigPtr& /*signal*/)
{
    return DataDescriptorChangedEventPacket(nullptr, nullptr);
}

OPENDAQ_DEFINE_CLASS_FACTORY_WITH_INTERFACE(
    INTERNAL_FACTORY,
    MockStreaming, IStreaming,
    IString*, connectionString
)
