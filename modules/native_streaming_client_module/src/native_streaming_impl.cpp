#include <native_streaming_client_module/native_streaming_impl.h>

#include <opendaq/signal_config_ptr.h>
#include <opendaq/custom_log.h>

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
    const auto it = std::find(availableSignalIds.begin(), availableSignalIds.end(), idToAdd);
    if (it == availableSignalIds.end())
    {
        availableSignalIds.push_back(idToAdd);
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
    const auto it = std::find(availableSignalIds.begin(), availableSignalIds.end(), idToRemove);
    if (it != availableSignalIds.end())
    {
        availableSignalIds.erase(it);
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
    const auto eventPacket = packet.asPtrOrNull<IEventPacket>();
    if (eventPacket.assigned())
    {
        handleEventPacket(signalStringId, eventPacket);
    }
    else
    {
        handleDataPacket(signalStringId, packet);
    }
}

void NativeStreamingImpl::onSetActive(bool active)
{
    for (const auto& [signalStreamingId, _] : streamingSignals)
    {
        if (active)
        {
            // TODO subscribe when listener connected
            //clientHandler->subscribeSignal(signalStreamingId);
        }
        else
        {
            // TODO unsubscribe when all listeners disconnected
            //clientHandler->unsubscribeSignal(signalStreamingId);
        }
    }
}

StringPtr NativeStreamingImpl::onAddSignal(const SignalRemotePtr& signal)
{
    StringPtr signalStreamingId = this->getSignalStreamingId(signal);
    if ( !signalStreamingId.assigned() )
        throw NotFoundException("Signal with id {} is not available in Native streaming", signal.getRemoteId());

    handleCachedEventPackets(signalStreamingId, signal);
    if (this->isActive)
    {
        // TODO subscribe when listener connected
        //clientHandler->subscribeSignal(signalStreamingId);
    }
    return signalStreamingId;
}

void NativeStreamingImpl::onRemoveSignal(const SignalRemotePtr& signal)
{
    StringPtr signalStreamingId = this->getSignalStreamingId(signal);
    if ( !signalStreamingId.assigned() )
        throw NotFoundException("Signal with id {} is not available in Native streaming", signal.getRemoteId());
    if (this->isActive)
    {
        // TODO unsubscribe when all listeners disconnected
        //clientHandler->unsubscribeSignal(signalStreamingId);
    }
}

void NativeStreamingImpl::handleEventPacket(const StringPtr& signalId, const EventPacketPtr& eventPacket)
{
    if (auto it = streamingSignals.find(signalId); it != streamingSignals.end())
    {
        SignalRemotePtr signal = it->second;
        Bool forwardPacket = signal.triggerEvent(eventPacket);
        auto signalConfig = signal.asPtr<ISignalConfig>();
        auto sourceStreamingConnectionString = signalConfig.getActiveStreamingSource();
        if (sourceStreamingConnectionString == connectionString && isActive && forwardPacket)
        {
            signalConfig.sendPacket(eventPacket);
        }
    }
    else
    {
        cachedEventPackets[signalId].push_back(eventPacket);
    }
}

void NativeStreamingImpl::handleDataPacket(const StringPtr& signalId, const PacketPtr& dataPacket)
{
    if (auto it = streamingSignals.find(signalId); it != streamingSignals.end() && isActive)
    {
        auto signal = (it->second).asPtr<ISignalConfig>();
        auto sourceStreamingConnectionString = signal.getActiveStreamingSource();
        if (sourceStreamingConnectionString == connectionString)
        {
            signal.sendPacket(dataPacket);
        }
    }
}

void NativeStreamingImpl::handleCachedEventPackets(const StringPtr& signalStreamingId,
                                                   const SignalRemotePtr& signal)
{
    if (auto it = cachedEventPackets.find(signalStreamingId); it != cachedEventPackets.end())
    {
        for (const auto& eventPacket : it->second)
            signal.triggerEvent(eventPacket);
        cachedEventPackets.erase(it);
    }
}

StringPtr NativeStreamingImpl::getSignalStreamingId(const SignalRemotePtr &signal)
{
    std::string signalFullId = signal.getRemoteId().toStdString();

    std::scoped_lock lock(availableSignalsSync);
    const auto it = std::find_if(
        availableSignalIds.begin(),
        availableSignalIds.end(),
        [signalFullId](std::string idEnding)
        {
            if (idEnding.size() > signalFullId.size())
                return false;
            return std::equal(idEnding.rbegin(), idEnding.rend(), signalFullId.rbegin());
        }
    );

    if (it != availableSignalIds.end())
        return String(*it);
    else
        return nullptr;
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
