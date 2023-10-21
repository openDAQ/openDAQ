#include "websocket_streaming/websocket_streaming_impl.h"
#include <opendaq/event_packet.h>
#include <opendaq/mirrored_signal_config.h>
#include <opendaq/streaming_ptr.h>
#include <opendaq/mirrored_signal_private.h>

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

StringPtr WebsocketStreamingImpl::onAddSignal(const MirroredSignalConfigPtr& signal)
{
    return getSignalStreamingId(signal);
}

void WebsocketStreamingImpl::onRemoveSignal(const MirroredSignalConfigPtr& /*signal*/)
{
}

void WebsocketStreamingImpl::onSubscribeSignal(const MirroredSignalConfigPtr& /*signal*/)
{
}

void WebsocketStreamingImpl::onUnsubscribeSignal(const MirroredSignalConfigPtr& /*signal*/)
{
}

EventPacketPtr WebsocketStreamingImpl::onCreateDataDescriptorChangedEventPacket(const MirroredSignalConfigPtr& signal)
{
    StringPtr signalStreamingId = getSignalStreamingId(signal);
    return streamingClient->getDataDescriptorChangedEventPacket(signalStreamingId);
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

void WebsocketStreamingImpl::handleEventPacket(const MirroredSignalConfigPtr& signal, const EventPacketPtr& eventPacket)
{
    Bool forwardPacket = signal.template asPtr<IMirroredSignalPrivate>()->triggerEvent(eventPacket);
    if (forwardPacket)
        signal.sendPacket(eventPacket);
}

void WebsocketStreamingImpl::onPacket(const StringPtr& signalId, const PacketPtr& packet)
{
    if (!packet.assigned() || !this->isActive)
        return;

    if (auto it = streamingSignalsRefs.find(signalId); it != streamingSignalsRefs.end())
    {
        auto signalRef = it->second;
        MirroredSignalConfigPtr signal = signalRef.assigned() ? signalRef.getRef() : nullptr;
        if (signal.assigned() && signal.getActiveStreamingSource() == connectionString)
        {
            const auto eventPacket = packet.asPtrOrNull<IEventPacket>();
            if (eventPacket.assigned())
                handleEventPacket(signal, eventPacket);
            else
                signal.sendPacket(packet);
        }
    }
}

void WebsocketStreamingImpl::onAvailableSignals(const std::vector<std::string>& signalIds)
{
    availableSignalIds = signalIds;
}

StringPtr WebsocketStreamingImpl::getSignalStreamingId(const MirroredSignalConfigPtr &signal)
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
        throw NotFoundException("Signal with id {} is not available in Websocket streaming", signal.getRemoteId());
}

END_NAMESPACE_OPENDAQ_WEBSOCKET_STREAMING
