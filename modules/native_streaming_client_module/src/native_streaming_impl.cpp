#include <native_streaming_client_module/native_streaming_impl.h>

#include <opendaq/signal_config_ptr.h>
#include <opendaq/custom_log.h>

#include <opendaq/mirrored_signal_private.h>
#include <opendaq/subscription_event_args_factory.h>

#include <opendaq/ids_parser.h>

BEGIN_NAMESPACE_OPENDAQ_NATIVE_STREAMING_CLIENT_MODULE

using namespace opendaq_native_streaming_protocol;

NativeStreamingImpl::NativeStreamingImpl(
    const StringPtr& connectionString,
    const StringPtr& host,
    const StringPtr& port,
    const StringPtr& path,
    const ContextPtr& context,
    NativeStreamingClientHandlerPtr clientHandler,
    const ProcedurePtr& onDeviceSignalAvailableCallback,
    const ProcedurePtr& onDeviceSignalUnavailableCallback,
    OnReconnectionStatusChangedCallback onReconnectionStatusChangedCb)
    : Streaming(connectionString, context)
    , clientHandler(clientHandler)
    , onDeviceSignalAvailableCallback(onDeviceSignalAvailableCallback)
    , onDeviceSignalUnavailableCallback(onDeviceSignalUnavailableCallback)
    , onDeviceReconnectionStatusChangedCb(onReconnectionStatusChangedCb)
    , reconnectionStatus(ClientReconnectionStatus::Connected)
    , ioContextPtr(std::make_shared<boost::asio::io_context>())
    , workGuard(ioContextPtr->get_executor())
    , loggerComponent(context.getLogger().getOrAddComponent("NativeStreamingImpl"))
{
    prepareClientHandler();
    startAsyncOperations();

    if (!this->clientHandler->connect(host.toStdString(), port.toStdString(), path.toStdString()))
    {
        stopAsyncOperations();
        LOG_E("Failed to connect to native streaming server - host {} port {} path {}", host, port, path);
        throw NotFoundException("Failed to connect to native streaming server, connection string: {}",
                                connectionString);
    }
}

NativeStreamingImpl::~NativeStreamingImpl()
{
    stopAsyncOperations();
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

void NativeStreamingImpl::removeFromAvailableSignals(const StringPtr& signalStringId)
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
            // re-subscribe signals
            if (auto it = availableSignalsReconnection.find(signalId); it != availableSignalsReconnection.end())
            {
                if (signalSubscribersCount > 0)
                {
                    clientHandler->subscribeSignal(signalId);
                    it->second = signalSubscribersCount;
                }
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
    OnSignalAvailableCallback signalAvailableCb =
        [this](const StringPtr& signalStringId,
               const StringPtr& serializedSignal)
    {
        signalAvailableHandler(signalStringId, serializedSignal);
    };
    OnSignalUnavailableCallback signalUnavailableCb =
        [this](const StringPtr& signalStringId)
    {
        signalUnavailableHandler(signalStringId);
    };
    OnPacketCallback onPacketCallback =
        [this](const StringPtr& signalStringId, const PacketPtr& packet)
    {
        onPacket(signalStringId, packet);
    };
    OnSignalSubscriptionAckCallback onSignalSubscriptionAckCallback =
        [this](const StringPtr& signalStringId, bool subscribed)
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
    };
    OnReconnectionStatusChangedCallback onReconnectionStatusChangedCb =
        [this](ClientReconnectionStatus status)
    {
        reconnectionStatusChangedHandler(status);
    };
    clientHandler->setIoContext(ioContextPtr);
    clientHandler->setSignalAvailableHandler(signalAvailableCb);
    clientHandler->setSignalUnavailableHandler(signalUnavailableCb);
    clientHandler->setPacketHandler(onPacketCallback);
    clientHandler->setSignalSubscriptionAckCallback(onSignalSubscriptionAckCallback);
    clientHandler->setReconnectionStatusChangedCb(onReconnectionStatusChangedCb);
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
            clientHandler->subscribeSignal(signalStreamingId);
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
            clientHandler->unsubscribeSignal(signalStreamingId);
    }
}

EventPacketPtr NativeStreamingImpl::onCreateDataDescriptorChangedEventPacket(const StringPtr& signalRemoteId)
{
    StringPtr signalStreamingId = onGetSignalStreamingId(signalRemoteId);
    return clientHandler->getDataDescriptorChangedEventPacket(signalStreamingId);
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

void NativeStreamingImpl::startAsyncOperations()
{
    ioThread = std::thread([this]()
                           {
                               ioContextPtr->run();
                               LOG_I("IO thread finished");
                           });
}

void NativeStreamingImpl::stopAsyncOperations()
{
    ioContextPtr->stop();
    if (ioThread.joinable())
    {
        ioThread.join();
        LOG_I("IO thread joined");
    }
}

END_NAMESPACE_OPENDAQ_NATIVE_STREAMING_CLIENT_MODULE
