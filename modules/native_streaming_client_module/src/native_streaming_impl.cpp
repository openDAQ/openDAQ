#include <native_streaming_client_module/native_streaming_impl.h>

#include <opendaq/signal_config_ptr.h>
#include <opendaq/custom_log.h>

#include <opendaq/mirrored_signal_private.h>
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
    OnReconnectionStatusChangedCallback onReconnectionStatusChangedCb)
    : Streaming(connectionString, context)
    , transportClientHandler(transportClientHandler)
    , onDeviceSignalAvailableCallback(onDeviceSignalAvailableCallback)
    , onDeviceSignalUnavailableCallback(onDeviceSignalUnavailableCallback)
    , onDeviceReconnectionStatusChangedCb(onReconnectionStatusChangedCb)
    , reconnectionStatus(ClientReconnectionStatus::Connected)
    , processingIOContextPtr(processingIOContextPtr)
    , processingStrand(*(this->processingIOContextPtr))
    , protocolInitFuture(protocolInitPromise.get_future())
    , loggerComponent(context.getLogger().getOrAddComponent("NativeStreamingImpl"))
    , streamingInitTimeout(std::chrono::milliseconds(streamingInitTimeout))
    , timerContextPtr(transportClientHandler->getIoContext())
    , protocolInitTimer(
          std::make_shared<boost::asio::steady_timer>(*timerContextPtr)
    )
{
    prepareClientHandler();
    this->transportClientHandler->sendStreamingRequest();

    if (protocolInitFuture.wait_for(this->streamingInitTimeout) != std::future_status::ready)
    {
        this->processingIOContextPtr->stop();
        throw GeneralErrorException("Streaming protocol intialization timed out; connection string: {}",
                                    connectionString);
    }
}

NativeStreamingImpl::~NativeStreamingImpl()
{
    protocolInitTimer->cancel();
    transportClientHandler->resetStreamingHandlers();
    processingIOContextPtr->stop();
}

void NativeStreamingImpl::signalAvailableHandler(const StringPtr& signalStringId,
                                                 const StringPtr& serializedSignal)
{
    if (reconnectionStatus == ClientReconnectionStatus::Reconnecting)
    {
        addToAvailableSignalsOnReconnection(signalStringId);
    }
    else
    {
        addToAvailableSignals(signalStringId);
    }
    if (onDeviceSignalAvailableCallback.assigned())
    {
        ErrCode errCode = wrapHandler(onDeviceSignalAvailableCallback,
                                      signalStringId,
                                      serializedSignal);
        checkErrorInfo(errCode);
    }
}

void NativeStreamingImpl::addToAvailableSignals(const StringPtr& signalStringId)
{
    std::scoped_lock lock(availableSignalsSync);
    if (const auto it = availableSignals.find(signalStringId); it == availableSignals.end())
    {
        availableSignals.insert({signalStringId, 0});
    }
    else
    {
        throw AlreadyExistsException("Signal with id {} already registered in native streaming", signalStringId);
    }
}

void NativeStreamingImpl::addToAvailableSignalsOnReconnection(const StringPtr& signalStringId)
{
    if (const auto it = availableSignalsReconnection.find(signalStringId); it == availableSignalsReconnection.end())
    {
        availableSignalsReconnection.insert({signalStringId, 0});
    }
    else
    {
        throw AlreadyExistsException("Signal with id {} already registered in native streaming", signalStringId);
    }
}

void NativeStreamingImpl::subscribeAckHandler(const StringPtr& signalStringId, bool subscribed)
{
    if (auto it = streamingSignalsRefs.find(signalStringId); it != streamingSignalsRefs.end())
    {
        auto signalRef = it->second;
        MirroredSignalConfigPtr signal = signalRef.assigned() ? signalRef.getRef() : nullptr;
        if (signal.assigned())
        {
            if (subscribed)
                signal.template asPtr<daq::IMirroredSignalPrivate>()->subscribeCompleted(connectionString);
            else
                signal.template asPtr<daq::IMirroredSignalPrivate>()->unsubscribeCompleted(connectionString);
        }
    }
}

void NativeStreamingImpl::signalUnavailableHandler(const StringPtr& signalStringId)
{
    if (reconnectionStatus != ClientReconnectionStatus::Connected &&
        reconnectionStatus != ClientReconnectionStatus::Restored)
    {
        throw GeneralErrorException("Signal unavailable command received during reconnection");
    }
    removeFromAvailableSignals(signalStringId);
    if (onDeviceSignalUnavailableCallback.assigned())
    {
        ErrCode errCode = wrapHandler(onDeviceSignalUnavailableCallback, signalStringId);
        checkErrorInfo(errCode);
    }
}

void NativeStreamingImpl::removeFromAddedSignals(const StringPtr& signalStringId)
{
    try
    {
        LOG_I("Signal with id {} is not available in streaming {} anymore, removing.", signalStringId, this->connectionString);
        this->removeSignalById(signalStringId);
    }
    catch (const NotFoundException&)
    {
        LOG_I("Streaming {} was not used as source for signal.", this->connectionString);
    }
    catch (const std::exception& e)
    {
        LOG_W("Fail to remove signal: {}", e.what());
    }
}

void NativeStreamingImpl::removeFromAvailableSignals(const StringPtr& signalStringId)
{
    {
        std::scoped_lock lock(availableSignalsSync);
        if (const auto it = availableSignals.find(signalStringId); it != availableSignals.end())
        {
            availableSignals.erase(it);
        }
        else
        {
            throw NotFoundException("Signal with id {} is not registered in native streaming", signalStringId);
        }
    }

    removeFromAddedSignals(signalStringId);
}

void NativeStreamingImpl::reconnectionStatusChangedHandler(opendaq_native_streaming_protocol::ClientReconnectionStatus status)
{
    if (status == ClientReconnectionStatus::Restored)
    {
        // replace available signals with new ones
        std::scoped_lock lock(availableSignalsSync);
        for (const auto& item : availableSignals)
        {
            const auto signalId = item.first;
            const auto signalSubscribersCount = item.second;

            if (auto it = availableSignalsReconnection.find(signalId); it != availableSignalsReconnection.end())
            {
                // signal kept in available re-subscribe it
                if (signalSubscribersCount > 0)
                {
                    transportClientHandler->subscribeSignal(signalId);
                    it->second = signalSubscribersCount;
                }
            }
            else
            {
                // signal no longer exists in available, also remove it from added
                removeFromAddedSignals(signalId);
            }
        }
        availableSignals = availableSignalsReconnection;
        availableSignalsReconnection.clear();
    }
    else if (status == ClientReconnectionStatus::Reconnecting)
    {
        availableSignalsReconnection.clear();
    }

    reconnectionStatus = status;

    if (onDeviceReconnectionStatusChangedCb)
    {
        onDeviceReconnectionStatusChangedCb(status);
    }
}

void NativeStreamingImpl::prepareClientHandler()
{
    using namespace boost::asio;

    OnSignalAvailableCallback signalAvailableCb =
        [this](const StringPtr& signalStringId,
               const StringPtr& serializedSignal)
    {
        dispatch(
            *processingIOContextPtr,
            processingStrand.wrap(
                [this, signalStringId = signalStringId, serializedSignal = serializedSignal]()
                {
                    signalAvailableHandler(signalStringId, serializedSignal);
                }
            )
        );
    };
    OnSignalUnavailableCallback signalUnavailableCb =
        [this](const StringPtr& signalStringId)
    {
        dispatch(
            *processingIOContextPtr,
            processingStrand.wrap(
                [this, signalStringId = signalStringId]()
                {
                    signalUnavailableHandler(signalStringId);
                }
            )
        );
    };
    OnPacketCallback onPacketCallback =
        [this](const StringPtr& signalStringId, const PacketPtr& packet)
    {
        dispatch(
            *processingIOContextPtr,
            processingStrand.wrap(
                [this, signalStringId = signalStringId, packet = packet]()
                {
                    onPacket(signalStringId, packet);
                }
            )
        );
    };
    OnSignalSubscriptionAckCallback onSignalSubscriptionAckCallback =
        [this](const StringPtr& signalStringId, bool subscribed)
    {
        dispatch(
            *processingIOContextPtr,
            processingStrand.wrap(
                [this, signalStringId = signalStringId, subscribed]()
                {
                    subscribeAckHandler(signalStringId, subscribed);
                }
            )
        );
    };
    OnReconnectionStatusChangedCallback onReconnectionStatusChangedCb =
        [this](ClientReconnectionStatus status)
    {
        dispatch(
            *processingIOContextPtr,
            processingStrand.wrap(
                [this, status]()
                {
                    if (status == ClientReconnectionStatus::Restored)
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
                                    reconnectionStatusChangedHandler(ClientReconnectionStatus::Restored);
                                else
                                    reconnectionStatusChangedHandler(ClientReconnectionStatus::Unrecoverable);
                            }
                        );
                    }
                    else
                    {
                        reconnectionStatusChangedHandler(status);
                    }
                }
            )
        );
    };
    OnStreamingInitDoneCallback onStreamingInitDoneCb =
        [this]()
    {
        dispatch(
            *processingIOContextPtr,
            processingStrand.wrap(
                [this]()
                {
                    protocolInitPromise.set_value();
                }
            )
        );
    };

    transportClientHandler->setStreamingHandlers(signalAvailableCb,
                                                 signalUnavailableCb,
                                                 onPacketCallback,
                                                 onSignalSubscriptionAckCallback,
                                                 onReconnectionStatusChangedCb,
                                                 onStreamingInitDoneCb);
}

void NativeStreamingImpl::onPacket(const StringPtr& signalStringId, const PacketPtr& packet)
{
    if (!this->isActive)
        return;

    if (auto it = streamingSignalsRefs.find(signalStringId); it != streamingSignalsRefs.end())
    {
        auto signalRef = it->second;
        MirroredSignalConfigPtr signal = signalRef.assigned() ? signalRef.getRef() : nullptr;
        if (signal.assigned() &&
            signal.getStreamed() &&
            signal.getActiveStreamingSource() == connectionString)
        {
            const auto eventPacket = packet.asPtrOrNull<IEventPacket>();
            if (eventPacket.assigned())
                handleEventPacket(signal, eventPacket);
            else
                signal.sendPacket(packet);
        }
    }
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

void NativeStreamingImpl::onSubscribeSignal(const StringPtr& signalRemoteId, const StringPtr& domainSignalRemoteId)
{
    if (domainSignalRemoteId.assigned())
        checkAndSubscribe(domainSignalRemoteId);
    checkAndSubscribe(signalRemoteId);
}

void NativeStreamingImpl::onUnsubscribeSignal(const StringPtr& signalRemoteId, const StringPtr& domainSignalRemoteId)
{
    if (domainSignalRemoteId.assigned())
        checkAndUnsubscribe(domainSignalRemoteId);
    checkAndUnsubscribe(signalRemoteId);
}

void NativeStreamingImpl::checkAndSubscribe(const StringPtr& signalRemoteId)
{
    auto signalStreamingId = onGetSignalStreamingId(signalRemoteId);
    if (auto it = streamingSignalsRefs.find(signalStreamingId); it == streamingSignalsRefs.end())
        throw NotFoundException("Signal with id {} is not added to Native streaming", signalRemoteId);

    std::scoped_lock lock(availableSignalsSync);
    if (const auto it = availableSignals.find(signalStreamingId); it != availableSignals.end())
    {
        if (it->second == 0)
            transportClientHandler->subscribeSignal(signalStreamingId);
        it->second++;
    }
}

void NativeStreamingImpl::checkAndUnsubscribe(const StringPtr& signalRemoteId)
{
    auto signalStreamingId = onGetSignalStreamingId(signalRemoteId);
    if (auto it = streamingSignalsRefs.find(signalStreamingId); it == streamingSignalsRefs.end())
        throw NotFoundException("Signal with id {} is not added to Native streaming", signalRemoteId);

    std::scoped_lock lock(availableSignalsSync);
    if (const auto it = availableSignals.find(signalStreamingId);
        it != availableSignals.end() && it->second > 0)
    {
        it->second--;
        if (it->second == 0)
            transportClientHandler->unsubscribeSignal(signalStreamingId);
    }
}

EventPacketPtr NativeStreamingImpl::onCreateDataDescriptorChangedEventPacket(const StringPtr& signalRemoteId)
{
    StringPtr signalStreamingId = onGetSignalStreamingId(signalRemoteId);
    return transportClientHandler->getDataDescriptorChangedEventPacket(signalStreamingId);
}

void NativeStreamingImpl::handleEventPacket(const MirroredSignalConfigPtr& signal, const EventPacketPtr& eventPacket)
{
    Bool forwardPacket = signal.template asPtr<IMirroredSignalPrivate>()->triggerEvent(eventPacket);
    if (forwardPacket)
        signal.sendPacket(eventPacket);
}

StringPtr NativeStreamingImpl::onGetSignalStreamingId(const StringPtr& signalRemoteId)
{
    std::scoped_lock lock(availableSignalsSync);
    const auto it = std::find_if(
        availableSignals.begin(),
        availableSignals.end(),
        [&signalRemoteId](std::pair<StringPtr, SizeT> item)
        {
            return IdsParser::idEndsWith(signalRemoteId.toStdString(), item.first.toStdString());
        }
    );

    if (it != availableSignals.end())
        return it->first;
    else
        throw NotFoundException("Signal with id {} is not available in Native streaming", signalRemoteId);
}

END_NAMESPACE_OPENDAQ_NATIVE_STREAMING_CLIENT_MODULE
