#include "websocket_streaming/websocket_streaming_impl.h"
#include <opendaq/signal_config_ptr.h>
#include <opendaq/event_packet.h>
#include <opendaq/signal_remote.h>
#include <opendaq/streaming_ptr.h>

BEGIN_NAMESPACE_OPENDAQ_WEBSOCKET_STREAMING

WebsocketStreamingImpl::WebsocketStreamingImpl(const StringPtr& connectionString,
                                               const ContextPtr& context)
    : WebsocketStreamingImpl(std::make_shared<StreamingClient>(context, connectionString),
                             connectionString,
                             context)
{
}

WebsocketStreamingImpl::WebsocketStreamingImpl(StreamingClientPtr streamingClient,
                                               const StringPtr& connectionString,
                                               const ContextPtr& context)
    : Streaming(connectionString, context)
    , streamingClient(streamingClient)
{
    prepareStreamingClient();
    if (!this->streamingClient->connect())
        throw NotFoundException("Failed to connect to websocket server url: {}", connectionString);
}

void WebsocketStreamingImpl::onSetActive(bool active)
{
}

StringPtr WebsocketStreamingImpl::onAddSignal(const SignalRemotePtr& signal)
{
    StringPtr signalStreamingId = this->getSignalStreamingId(signal);
    if ( !signalStreamingId.assigned() )
        throw NotFoundException("Signal with id {} is not available in Websocket streaming", signal.getRemoteId());

    handleCachedEventPackets(signalStreamingId, signal);
    return signalStreamingId;
}

void WebsocketStreamingImpl::onRemoveSignal(const SignalRemotePtr& /*signal*/)
{
}

void WebsocketStreamingImpl::prepareStreamingClient()
{
    auto packetCallback = [this](const StringPtr& signalId, const PacketPtr& packet)
    {
        this->onPacket(signalId, packet);
    };
    streamingClient->onPacket(packetCallback);

    auto availableSignalsCallback = [this](const std::vector<std::string>& signalIds)
    {
        this->onAvailableSignals(signalIds);
    };
    streamingClient->onAvailableStreamingSignals(availableSignalsCallback);
}

void WebsocketStreamingImpl::handleEventPacket(const StringPtr& signalId, const EventPacketPtr& eventPacket)
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

void WebsocketStreamingImpl::handleCachedEventPackets(const StringPtr& signalStreamingId,
                                                      const SignalRemotePtr& signal)
{
    if (auto it = cachedEventPackets.find(signalStreamingId); it != cachedEventPackets.end())
    {
        for (const auto& eventPacket : it->second)
            signal.triggerEvent(eventPacket);
        cachedEventPackets.erase(it);
    }
}

void WebsocketStreamingImpl::handleDataPacket(const StringPtr& signalId, const PacketPtr& dataPacket)
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

void WebsocketStreamingImpl::onPacket(const StringPtr& signalId, const PacketPtr& packet)
{
    if (!packet.assigned())
        return;

    const auto eventPacket = packet.asPtrOrNull<IEventPacket>();
    if (eventPacket.assigned())
    {
        handleEventPacket(signalId, eventPacket);
    }
    else
    {
        handleDataPacket(signalId, packet);
    }
}

void WebsocketStreamingImpl::onAvailableSignals(const std::vector<std::string>& signalIds)
{
    availableSignalIds = signalIds;
}

StringPtr WebsocketStreamingImpl::getSignalStreamingId(const SignalRemotePtr &signal)
{
    std::string signalFullId = signal.getRemoteId().toStdString();
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

END_NAMESPACE_OPENDAQ_WEBSOCKET_STREAMING
