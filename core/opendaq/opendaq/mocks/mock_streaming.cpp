#include "opendaq/mock/mock_streaming.h"

using namespace daq;

MockStreamingImpl::MockStreamingImpl(const StringPtr& connectionString)
    : Streaming(connectionString, nullptr)
{
}

void MockStreamingImpl::onSetActive(bool /*active*/)
{

}

StringPtr MockStreamingImpl::onAddSignal(const SignalRemotePtr& signal)
{
    return signal.getRemoteId();
}

void MockStreamingImpl::onRemoveSignal(const SignalRemotePtr& /*signal*/)
{

}

OPENDAQ_DEFINE_CLASS_FACTORY_WITH_INTERFACE(
    INTERNAL_FACTORY,
    MockStreaming, IStreaming,
    IString*, connectionString
)
