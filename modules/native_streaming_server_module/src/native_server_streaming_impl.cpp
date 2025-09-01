#include <native_streaming_server_module/native_server_streaming_impl.h>

#include <opendaq/signal_config_ptr.h>
#include <opendaq/custom_log.h>

#include <opendaq/mirrored_signal_private_ptr.h>
#include <opendaq/subscription_event_args_factory.h>

#include <boost/asio/dispatch.hpp>
#include <native_streaming_protocol/native_streaming_protocol_types.h>

BEGIN_NAMESPACE_OPENDAQ_NATIVE_STREAMING_SERVER_MODULE

using namespace opendaq_native_streaming_protocol;

NativeServerStreamingImpl::NativeServerStreamingImpl(TransportServerHandlerPtr transportServerHandler,
                                                     std::shared_ptr<boost::asio::io_context> processingIOContextPtr,
                                                     ContextPtr context)
    : Super("OpenDAQNativeStreaming", context, false, "OpenDAQNativeStreaming")
    , transportServerHandler(transportServerHandler)
    , processingIOContextPtr(processingIOContextPtr)
{
    initServerHandlerCallbacks();
}

NativeServerStreamingImpl::~NativeServerStreamingImpl()
{
    transportServerHandler->resetStreamingToDeviceHandlers();
}

void NativeServerStreamingImpl::upgradeToSafeProcessingCallbacks()
{
    upgradeServerHandlerCallbacks();
}

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
    transportServerHandler->doSubscribeSignal(signalStreamingId, true);
}

void NativeServerStreamingImpl::onUnsubscribeSignal(const StringPtr& signalStreamingId)
{
    transportServerHandler->doSubscribeSignal(signalStreamingId, false);
}

void NativeServerStreamingImpl::initServerHandlerCallbacks()
{
    using namespace boost::asio;

    OnSignalAvailableCallback signalAvailableCb =
        [this](const StringPtr& signalStringId,
               const StringPtr& /*serializedSignal*/)
    {
        dispatch(
            *processingIOContextPtr,
            [this, signalStringId]()
            {
                this->addToAvailableSignals(signalStringId);
            }
        );
    };
    OnSignalUnavailableCallback signalUnavailableCb =
        [this](const StringPtr& signalStringId)
    {
        dispatch(
            *processingIOContextPtr,
            [this, signalStringId]()
            {
                this->removeFromAvailableSignals(signalStringId);
            }
        );
    };
    OnPacketCallback onPacketCallback =
        [this](const StringPtr& signalStringId, const PacketPtr& packet)
    {
        dispatch(
            *processingIOContextPtr,
            [this, signalStringId, packet]()
            {
                this->onPacket(signalStringId, packet);
            }
        );
    };
    OnSignalSubscriptionAckCallback onSignalSubscriptionAckCallback =
        [this](const StringPtr& signalStringId, bool subscribed)
    {
        dispatch(
            *processingIOContextPtr,
            [this, signalStringId, subscribed]()
            {
                this->triggerSubscribeAck(signalStringId, subscribed);
            }
        );
    };

    transportServerHandler->setStreamingToDeviceHandlers(signalAvailableCb,
                                                         signalUnavailableCb,
                                                         onPacketCallback,
                                                         onSignalSubscriptionAckCallback);
}

void NativeServerStreamingImpl::upgradeServerHandlerCallbacks()
{
    using namespace boost::asio;
    WeakRefPtr<IStreaming> thisRef = this->template borrowPtr<StreamingPtr>();

    OnSignalAvailableCallback signalAvailableCb =
        [this, thisRef](const StringPtr& signalStringId,
                        const StringPtr& /*serializedSignal*/)
    {
        dispatch(
            *processingIOContextPtr,
            [this, thisRef, signalStringId]()
            {
                if (auto thisPtr = thisRef.getRef(); thisPtr.assigned())
                    this->addToAvailableSignals(signalStringId);
            }
        );
    };
    OnSignalUnavailableCallback signalUnavailableCb =
        [this, thisRef](const StringPtr& signalStringId)
    {
        dispatch(
            *processingIOContextPtr,
            [this, thisRef, signalStringId]()
            {
                if (auto thisPtr = thisRef.getRef(); thisPtr.assigned())
                    this->removeFromAvailableSignals(signalStringId);
            }
        );
    };
    OnPacketCallback onPacketCallback =
        [this, thisRef](const StringPtr& signalStringId, const PacketPtr& packet)
    {
        dispatch(
            *processingIOContextPtr,
            [this, thisRef, signalStringId, packet]()
            {
                if (auto thisPtr = thisRef.getRef(); thisPtr.assigned())
                    this->onPacket(signalStringId, packet);
            }
        );
    };
    OnSignalSubscriptionAckCallback onSignalSubscriptionAckCallback =
        [this, thisRef](const StringPtr& signalStringId, bool subscribed)
    {
        dispatch(
            *processingIOContextPtr,
            [this, thisRef, signalStringId, subscribed]()
            {
                if (auto thisPtr = thisRef.getRef(); thisPtr.assigned())
                    this->triggerSubscribeAck(signalStringId, subscribed);
            }
        );
    };

    transportServerHandler->setStreamingToDeviceHandlers(signalAvailableCb,
                                                         signalUnavailableCb,
                                                         onPacketCallback,
                                                         onSignalSubscriptionAckCallback);
}


END_NAMESPACE_OPENDAQ_NATIVE_STREAMING_SERVER_MODULE


