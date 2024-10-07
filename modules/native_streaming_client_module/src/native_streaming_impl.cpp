#include <native_streaming_client_module/native_streaming_impl.h>

#include <opendaq/signal_config_ptr.h>
#include <opendaq/custom_log.h>

#include <opendaq/mirrored_signal_private_ptr.h>
#include <opendaq/subscription_event_args_factory.h>

#include <boost/asio/dispatch.hpp>

BEGIN_NAMESPACE_OPENDAQ_NATIVE_STREAMING_CLIENT_MODULE

using namespace opendaq_native_streaming_protocol;

NativeStreamingImpl::NativeStreamingImpl(
    const StringPtr& connectionString,
    const ContextPtr& context,
    NativeStreamingClientHandlerPtr transportClientHandler,
    std::shared_ptr<boost::asio::io_context> processingIOContextPtr,
    Int streamingInitTimeout,
    const ProcedurePtr& onDeviceSignalAvailableCallback,
    const ProcedurePtr& onDeviceSignalUnavailableCallback,
    OnConnectionStatusChangedCallback onDeviceConnectionStatusChangedCb)
    : Super(connectionString, context, false)
    , transportClientHandler(transportClientHandler)
    , onDeviceSignalAvailableCallback(onDeviceSignalAvailableCallback)
    , onDeviceSignalUnavailableCallback(onDeviceSignalUnavailableCallback)
    , onDeviceConnectionStatusChangedCb(onDeviceConnectionStatusChangedCb)
    , connectionStatus(ClientConnectionStatus::Connected)
    , processingIOContextPtr(processingIOContextPtr)
    , protocolInitFuture(protocolInitPromise.get_future())
    , streamingInitTimeout(std::chrono::milliseconds(streamingInitTimeout))
    , timerContextPtr(transportClientHandler->getIoContext())
    , protocolInitTimer(
          std::make_shared<boost::asio::steady_timer>(*timerContextPtr)
    )
{
    initClientHandlerCallbacks();
    this->transportClientHandler->sendStreamingRequest();

    if (protocolInitFuture.wait_for(this->streamingInitTimeout) != std::future_status::ready)
    {
        stopProcessingOperations();
        throw GeneralErrorException("Streaming protocol intialization timed out; connection string: {}",
                                    connectionString);
    }
}

NativeStreamingImpl::~NativeStreamingImpl()
{
    protocolInitTimer->cancel();

    transportClientHandler->resetStreamingHandlers();
    stopProcessingOperations();
}

void NativeStreamingImpl::upgradeToSafeProcessingCallbacks()
{
    upgradeClientHandlerCallbacks();
}

void NativeStreamingImpl::stopProcessingOperations()
{
    if (!processingIOContextPtr->stopped())
    {
        processingIOContextPtr->stop();
    }
}

void NativeStreamingImpl::signalAvailableHandler(const StringPtr& signalStringId, const StringPtr& serializedSignal)
{
    addToAvailableSignals(signalStringId);
    if (onDeviceSignalAvailableCallback.assigned())
    {
        ErrCode errCode = wrapHandler(onDeviceSignalAvailableCallback, signalStringId, serializedSignal);
        checkErrorInfo(errCode);
    }
}

void NativeStreamingImpl::signalUnavailableHandler(const StringPtr& signalStringId)
{
    removeFromAvailableSignals(signalStringId);
    if (onDeviceSignalUnavailableCallback.assigned())
    {
        ErrCode errCode = wrapHandler(onDeviceSignalUnavailableCallback, signalStringId);
        checkErrorInfo(errCode);
    }
}

void NativeStreamingImpl::updateConnectionStatus(opendaq_native_streaming_protocol::ClientConnectionStatus status)
{
    if (status == ClientConnectionStatus::Connected)
    {
        completeReconnection();
    }
    else if (status == ClientConnectionStatus::Reconnecting)
    {
        startReconnection();
    }

    connectionStatus = status;

    if (onDeviceConnectionStatusChangedCb)
    {
        onDeviceConnectionStatusChangedCb(status);
    }
}

void NativeStreamingImpl::processConnectionStatus(opendaq_native_streaming_protocol::ClientConnectionStatus status)
{
    if (status == ClientConnectionStatus::Connected)
    {
        this->transportClientHandler->sendStreamingRequest();
        protocolInitPromise = std::promise<void>();
        protocolInitFuture = protocolInitPromise.get_future();
        protocolInitTimer->expires_from_now(streamingInitTimeout);
        protocolInitTimer->async_wait(
            [this](const boost::system::error_code& ec)
            {
                if (ec)
                    return;

                if (protocolInitFuture.wait_for(std::chrono::milliseconds(0)) == std::future_status::ready)
                    updateConnectionStatus(ClientConnectionStatus::Connected);
                else
                    updateConnectionStatus(ClientConnectionStatus::Unrecoverable);
            }
        );
    }
    else
    {
        updateConnectionStatus(status);
    }
}

void NativeStreamingImpl::upgradeClientHandlerCallbacks()
{
    using namespace boost::asio;
    WeakRefPtr<IStreaming> thisRef = this->template borrowPtr<StreamingPtr>();

    OnSignalAvailableCallback signalAvailableCb =
        [this, thisRef](const StringPtr& signalStringId,
               const StringPtr& serializedSignal)
    {
        dispatch(
            *processingIOContextPtr,
            [this, thisRef, signalStringId, serializedSignal]()
            {
                if (auto thisPtr = thisRef.getRef(); thisPtr.assigned())
                    signalAvailableHandler(signalStringId, serializedSignal);
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
                    signalUnavailableHandler(signalStringId);
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
    OnConnectionStatusChangedCallback onConnectionStatusChangedCb =
        [this, thisRef](ClientConnectionStatus status)
    {
        dispatch(
            *processingIOContextPtr,
            [this, thisRef, status]()
            {
                if (auto thisPtr = thisRef.getRef(); thisPtr.assigned())
                    processConnectionStatus(status);
            }
        );
    };
    OnStreamingInitDoneCallback onStreamingInitDoneCb =
        [this, thisRef]()
    {
        dispatch(
            *processingIOContextPtr,
            [this, thisRef]()
            {
                if (auto thisPtr = thisRef.getRef(); thisPtr.assigned())
                    protocolInitPromise.set_value();
            }
        );
    };

    transportClientHandler->setStreamingHandlers(signalAvailableCb,
                                                 signalUnavailableCb,
                                                 onPacketCallback,
                                                 onSignalSubscriptionAckCallback,
                                                 onConnectionStatusChangedCb,
                                                 onStreamingInitDoneCb);
}

void NativeStreamingImpl::initClientHandlerCallbacks()
{
    using namespace boost::asio;

    OnSignalAvailableCallback signalAvailableCb =
        [this](const StringPtr& signalStringId,
               const StringPtr& serializedSignal)
    {
        dispatch(
            *processingIOContextPtr,
            [this, signalStringId, serializedSignal]()
            {
                signalAvailableHandler(signalStringId, serializedSignal);
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
                signalUnavailableHandler(signalStringId);
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
    OnConnectionStatusChangedCallback onConnectionStatusChangedCb =
        [this](ClientConnectionStatus status)
    {
        dispatch(
            *processingIOContextPtr,
            [this, status]()
            {
                processConnectionStatus(status);
            }
        );
    };
    OnStreamingInitDoneCallback onStreamingInitDoneCb =
        [this]()
    {
        dispatch(
            *processingIOContextPtr,
            [this]()
            {
                protocolInitPromise.set_value();
            }
        );
    };

    transportClientHandler->setStreamingHandlers(signalAvailableCb,
                                                 signalUnavailableCb,
                                                 onPacketCallback,
                                                 onSignalSubscriptionAckCallback,
                                                 onConnectionStatusChangedCb,
                                                 onStreamingInitDoneCb);
}

void NativeStreamingImpl::onSetActive(bool active)
{
}

void NativeStreamingImpl::onAddSignal(const MirroredSignalConfigPtr& signal)
{
}

void NativeStreamingImpl::onRemoveSignal(const MirroredSignalConfigPtr& signal)
{
}

void NativeStreamingImpl::onSubscribeSignal(const StringPtr& signalStreamingId)
{
    transportClientHandler->subscribeSignal(signalStreamingId);
}

void NativeStreamingImpl::onUnsubscribeSignal(const StringPtr& signalStreamingId)
{
    transportClientHandler->unsubscribeSignal(signalStreamingId);
}

END_NAMESPACE_OPENDAQ_NATIVE_STREAMING_CLIENT_MODULE
