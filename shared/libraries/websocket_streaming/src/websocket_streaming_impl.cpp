#include "websocket_streaming/websocket_streaming_impl.h"
#include <opendaq/event_packet.h>
#include <opendaq/mirrored_signal_config.h>
#include <opendaq/streaming_ptr.h>
#include <opendaq/mirrored_signal_private_ptr.h>
#include <opendaq/subscription_event_args_factory.h>

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
    : Streaming(connectionString, context, true)
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

void WebsocketStreamingImpl::onSubscribeSignal(const StringPtr& signalStreamingId)
{
    streamingClient->subscribeSignal(signalStreamingId.toStdString());
}

void WebsocketStreamingImpl::onUnsubscribeSignal(const StringPtr& signalStreamingId)
{
    streamingClient->unsubscribeSignal(signalStreamingId.toStdString());
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
    streamingClient->onStreamingAvailableSignals(availableSignalsCallback);

    auto unavailableSignalsCallback = [this](const std::vector<std::string>& signalIds)
    {
        this->onUnavailableSignals(signalIds);
    };
    streamingClient->onStreamingUnavailableSignals(unavailableSignalsCallback);

    auto hiddenSignalCallback = [this](const StringPtr& signalId, const SubscribedSignalInfo& /*sInfo*/)
    {
        this->onHiddenSignal(signalId);
    };
    streamingClient->onStreamingHiddenSignal(hiddenSignalCallback);

    auto signalSubscriptionAckCallback = [this](const std::string& signalStringId, bool subscribed)
    {
        this->triggerSubscribeAck(signalStringId, subscribed);
    };
    streamingClient->onSubscriptionAck(signalSubscriptionAckCallback);
}

void WebsocketStreamingImpl::onAvailableSignals(const std::vector<std::string>& signalIds)
{
    for (const auto& signalId : signalIds)
    {
        auto signalStringId = String(signalId);
        addToAvailableSignals(signalStringId);
    }
}

void WebsocketStreamingImpl::onUnavailableSignals(const std::vector<std::string>& signalIds)
{
    for (const auto& signalId : signalIds)
    {
        auto signalStringId = String(signalId);
        removeFromAvailableSignals(signalStringId);
    }
}

void WebsocketStreamingImpl::onHiddenSignal(const std::string& signalId)
{
    if (const auto it = hiddenSignals.find(signalId); it == hiddenSignals.end())
    {
        hiddenSignals.insert(signalId);

        auto signalStringId = String(signalId);
        addToAvailableSignals(signalStringId);
    }
}

END_NAMESPACE_OPENDAQ_WEBSOCKET_STREAMING
