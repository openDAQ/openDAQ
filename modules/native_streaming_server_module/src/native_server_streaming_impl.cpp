#include <native_streaming_server_module/native_server_streaming_impl.h>

#include <opendaq/signal_config_ptr.h>
#include <opendaq/custom_log.h>

#include <opendaq/mirrored_signal_private_ptr.h>
#include <opendaq/subscription_event_args_factory.h>

#include <boost/asio/dispatch.hpp>

BEGIN_NAMESPACE_OPENDAQ_NATIVE_STREAMING_SERVER_MODULE

NativeServerStreamingImpl::NativeServerStreamingImpl(TransportLevelHandlerPtr transportServerHandler, ContextPtr context)
    : daq::Streaming("", context, false)
{}

NativeServerStreamingImpl::~NativeServerStreamingImpl()
{}

void NativeServerStreamingImpl::onSetActive(bool active)
{

}

void NativeServerStreamingImpl::onAddSignal(const MirroredSignalConfigPtr& signal)
{

}

void NativeServerStreamingImpl::onRemoveSignal(const MirroredSignalConfigPtr& signal)
{

}

void NativeServerStreamingImpl::onSubscribeSignal(const StringPtr& signalStreamingId)
{

}

void NativeServerStreamingImpl::onUnsubscribeSignal(const StringPtr& signalStreamingId)
{

}


END_NAMESPACE_OPENDAQ_NATIVE_STREAMING_SERVER_MODULE


