#include "websocket_streaming/websocket_streaming_impl.h"
#include <opendaq/event_packet.h>
#include <opendaq/mirrored_signal_config.h>
#include <opendaq/streaming_ptr.h>
#include <opendaq/mirrored_signal_private.h>
#include <opendaq/subscription_event_args_factory.h>
#include <opendaq/ids_parser.h>

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
        throw NotFoundException("Failed to connect to streaming server url: {}", connectionString);
}

void WebsocketStreamingImpl::onSetActive(bool active)
{
}

void WebsocketStreamingImpl::onAddSignal(const MirroredSignalConfigPtr& signal)
{
}

void WebsocketStreamingImpl::onRemoveSignal(const MirroredSignalConfigPtr& /*signal*/)
{
}

void WebsocketStreamingImpl::onSubscribeSignal(const StringPtr& signalRemoteId, const StringPtr& /*domainSignalRemoteId*/)
{
    auto signalStreamingId = onGetSignalStreamingId(signalRemoteId);
    if (auto it = streamingSignalsRefs.find(signalStreamingId); it == streamingSignalsRefs.end())
        throw NotFoundException("Signal with id {} is not added to Websocket streaming", signalRemoteId);

    streamingClient->subscribeSignals({signalStreamingId.toStdString()});
}

void WebsocketStreamingImpl::onUnsubscribeSignal(const StringPtr& signalRemoteId, const StringPtr& /*domainSignalRemoteId*/)
{
    auto signalStreamingId = onGetSignalStreamingId(signalRemoteId);
    if (auto it = streamingSignalsRefs.find(signalStreamingId); it == streamingSignalsRefs.end())
        throw NotFoundException("Signal with id {} is not added to Websocket streaming", signalRemoteId);

    streamingClient->unsubscribeSignals({signalStreamingId.toStdString()});
}

EventPacketPtr WebsocketStreamingImpl::onCreateDataDescriptorChangedEventPacket(const StringPtr& signalRemoteId)
{
    StringPtr signalStreamingId = onGetSignalStreamingId(signalRemoteId);
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

    auto signalSubscriptionAckCallback = [this](const std::string& signalStringId, bool subscribed)
    {
        auto signalKey = String(signalStringId);
        if (auto it = streamingSignalsRefs.find(signalKey); it != streamingSignalsRefs.end())
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
    streamingClient->onSubscriptionAck(signalSubscriptionAckCallback);
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

void WebsocketStreamingImpl::onAvailableSignals(const std::vector<std::string>& signalIds)
{
    for (const auto& signalId : signalIds)
        availableSignalIds.push_back(String(signalId));
}

StringPtr WebsocketStreamingImpl::onGetSignalStreamingId(const StringPtr& signalRemoteId)
{
    const auto it = std::find_if(
        availableSignalIds.begin(),
        availableSignalIds.end(),
        [&signalRemoteId](const StringPtr& signalStreamingId)
        {
            return IdsParser::idEndsWith(signalRemoteId.toStdString(), signalStreamingId.toStdString());
        }
    );

    if (it != availableSignalIds.end())
        return *it;
    else
        throw NotFoundException("Signal with id {} is not available in Websocket streaming", signalRemoteId);
}

END_NAMESPACE_OPENDAQ_WEBSOCKET_STREAMING
