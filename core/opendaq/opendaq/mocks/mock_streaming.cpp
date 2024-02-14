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

StringPtr MockStreamingImpl::onGetSignalStreamingId(const StringPtr& signalRemoteId)
{
    return signalRemoteId;
}

void MockStreamingImpl::onAddSignal(const MirroredSignalConfigPtr& /*signal*/)
{

}

void MockStreamingImpl::onRemoveSignal(const MirroredSignalConfigPtr& /*signal*/)
{

}

void MockStreamingImpl::onSubscribeSignal(const StringPtr& /*signalRemoteId*/, const StringPtr& /*domainSignalRemoteId*/)
{

}

void MockStreamingImpl::onUnsubscribeSignal(const StringPtr& /*signalRemoteId*/, const StringPtr& /*domainSignalRemoteId*/)
{

}

EventPacketPtr MockStreamingImpl::onCreateDataDescriptorChangedEventPacket(const StringPtr& /*signalRemoteId*/)
{
    return DataDescriptorChangedEventPacket(nullptr, nullptr);
}

OPENDAQ_DEFINE_CLASS_FACTORY_WITH_INTERFACE(
    INTERNAL_FACTORY,
    MockStreaming, IStreaming,
    IString*, connectionString
)
