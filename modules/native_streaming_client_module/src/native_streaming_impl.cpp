#include <native_streaming_client_module/native_streaming_impl.h>

#include <opendaq/signal_config_ptr.h>
#include <opendaq/custom_log.h>

#include <opendaq/mirrored_signal_private.h>

BEGIN_NAMESPACE_OPENDAQ_NATIVE_STREAMING_CLIENT_MODULE

using namespace opendaq_native_streaming_protocol;

NativeStreamingImpl::NativeStreamingImpl(const StringPtr& connectionString,
                                         const StringPtr& host,
                                         const StringPtr& port,
                                         const StringPtr& path,
                                         const ContextPtr& context,
                                         const ProcedurePtr& onDeviceSignalAvailableCallback,
                                         const ProcedurePtr& onDeviceSignalUnavailableCallback)
    : Streaming(connectionString, context)
    , onDeviceSignalAvailableCallback(onDeviceSignalAvailableCallback)
    , onDeviceSignalUnavailableCallback(onDeviceSignalUnavailableCallback)
    , ioContextPtr(std::make_shared<boost::asio::io_context>())
    , workGuard(ioContextPtr->get_executor())
    , logger(context.getLogger())
    , loggerComponent(logger.getOrAddComponent("NativeStreamingImpl"))
{
    prepareClientHandler();
    startAsyncOperations();

    if (!this->clientHandler->connect(ioContextPtr, host.toStdString(), port.toStdString(), path.toStdString()))
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
                                                 const StringPtr& domainSignalStringId,
                                                 const DataDescriptorPtr& signalDescriptor,
                                                 const StringPtr& name,
                                                 const StringPtr& description)
{
    addToAvailableSignals(signalStringId);
    if (onDeviceSignalAvailableCallback.assigned())
    {
        ErrCode errCode = wrapHandler(onDeviceSignalAvailableCallback,
                                      signalStringId,
                                      domainSignalStringId,
                                      signalDescriptor,
                                      name,
                                      description);
        checkErrorInfo(errCode);
    }
}

void NativeStreamingImpl::addToAvailableSignals(const StringPtr& signalStringId)
{
    std::string idToAdd = signalStringId.toStdString();

    std::scoped_lock lock(availableSignalsSync);
    if (const auto it = availableSignals.find(idToAdd); it == availableSignals.end())
    {
        availableSignals.insert({idToAdd, 0});
    }
    else
    {
        throw AlreadyExistsException("Signal with id {} already registered in native streaming", signalStringId);
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

void NativeStreamingImpl::removeFromAvailableSignals(const StringPtr& signalStringId)
{
    std::string idToRemove = signalStringId.toStdString();

    std::scoped_lock lock(availableSignalsSync);
    if (const auto it = availableSignals.find(idToRemove); it != availableSignals.end())
    {
        availableSignals.erase(it);
    }
    else
    {
        throw NotFoundException("Signal with id {} is not registered in native streaming", signalStringId);
    }
}

void NativeStreamingImpl::prepareClientHandler()
{
    OnSignalAvailableCallback signalAvailableCb =
        [this](const StringPtr& signalStringId,
               const StringPtr& domainSignalStringId,
               const DataDescriptorPtr& signalDescriptor,
               const StringPtr& name,
               const StringPtr& description)
    {
        signalAvailableHandler(signalStringId, domainSignalStringId, signalDescriptor, name, description);
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
    clientHandler = std::make_shared<NativeStreamingClientHandler>(context,
                                                                   signalAvailableCb,
                                                                   signalUnavailableCb,
                                                                   onPacketCallback);
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

StringPtr NativeStreamingImpl::onAddSignal(const MirroredSignalConfigPtr& signal)
{
    return getSignalStreamingId(signal);
}

void NativeStreamingImpl::onRemoveSignal(const MirroredSignalConfigPtr& signal)
{
}

void NativeStreamingImpl::onSubscribeSignal(const MirroredSignalConfigPtr& signal)
{
    auto domainSignal = signal.getDomainSignal();
    if (domainSignal.assigned())
        checkAndSubscribe(domainSignal.template asPtr<IMirroredSignalConfig>());
    checkAndSubscribe(signal);
}

void NativeStreamingImpl::onUnsubscribeSignal(const MirroredSignalConfigPtr& signal)
{
    auto domainSignal = signal.getDomainSignal();
    if (domainSignal.assigned())
        checkAndUnsubscribe(domainSignal.template asPtr<IMirroredSignalConfig>());
    checkAndUnsubscribe(signal);
}

void NativeStreamingImpl::checkAndSubscribe(const MirroredSignalConfigPtr& signal)
{
    auto signalStreamingId = getSignalStreamingId(signal);

    std::scoped_lock lock(availableSignalsSync);
    if (const auto it = availableSignals.find(signalStreamingId); it != availableSignals.end())
    {
        if (it->second == 0)
            clientHandler->subscribeSignal(signalStreamingId);
        it->second++;
    }
}

void NativeStreamingImpl::checkAndUnsubscribe(const MirroredSignalConfigPtr& signal)
{
    auto signalStreamingId = getSignalStreamingId(signal);

    std::scoped_lock lock(availableSignalsSync);
    if (const auto it = availableSignals.find(signalStreamingId);
        it != availableSignals.end() && it->second > 0)
    {
        it->second--;
        if (it->second == 0)
            clientHandler->unsubscribeSignal(signalStreamingId);
    }
}

EventPacketPtr NativeStreamingImpl::onCreateDataDescriptorChangedEventPacket(const MirroredSignalConfigPtr& signal)
{
    StringPtr signalStreamingId = getSignalStreamingId(signal);
    return clientHandler->getDataDescriptorChangedEventPacket(signalStreamingId);
}

void NativeStreamingImpl::handleEventPacket(const MirroredSignalConfigPtr& signal, const EventPacketPtr& eventPacket)
{
    Bool forwardPacket = signal.template asPtr<IMirroredSignalPrivate>()->triggerEvent(eventPacket);
    if (forwardPacket)
        signal.sendPacket(eventPacket);
}

StringPtr NativeStreamingImpl::getSignalStreamingId(const MirroredSignalConfigPtr& signal)
{
    std::string signalFullId = signal.getRemoteId().toStdString();

    std::scoped_lock lock(availableSignalsSync);
    const auto it = std::find_if(
        availableSignals.begin(),
        availableSignals.end(),
        [signalFullId](std::pair<std::string, SizeT> element)
        {
            std::string idEnding = element.first;
            if (idEnding.size() > signalFullId.size())
                return false;
            return std::equal(idEnding.rbegin(), idEnding.rend(), signalFullId.rbegin());
        }
    );

    if (it != availableSignals.end())
        return String(it->first);
    else
        throw NotFoundException("Signal with id {} is not available in Native streaming", signal.getRemoteId());
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
