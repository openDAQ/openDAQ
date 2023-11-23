#include <opendaq/packet.h>
#include <opendaq/reader_factory.h>
#include <iostream>
#include "websocket_streaming/websocket_streaming_server.h"
#include <opendaq/device_private.h>
#include <opendaq/streaming_info_config_ptr.h>
#include <coreobjects/property_factory.h>
#include <opendaq/streaming_info_factory.h>

BEGIN_NAMESPACE_OPENDAQ_WEBSOCKET_STREAMING

WebsocketStreamingServer::~WebsocketStreamingServer()
{
    stopInternal();
}

WebsocketStreamingServer::WebsocketStreamingServer(const InstancePtr& instance)
    : WebsocketStreamingServer(instance.getRootDevice(), instance.getContext())
{
}

WebsocketStreamingServer::WebsocketStreamingServer(const DevicePtr& device, const ContextPtr& context)
    : device(device)
    , context(context)
    , streamingServer(context)
{
}

void WebsocketStreamingServer::setStreamingPort(uint16_t port)
{
    this->streamingPort = port;
}

void WebsocketStreamingServer::setControlPort(uint16_t port)
{
    this->controlPort = port;
}

void WebsocketStreamingServer::start()
{
    if (!device.assigned())
        throw InvalidStateException("Device is not set.");
    if (!context.assigned())
        throw InvalidStateException("Context is not set.");
    if (streamingPort == 0 || controlPort == 0)
        return;

    streamingServer.onAccept([this](const daq::streaming_protocol::StreamWriterPtr& writer) { return device.getSignalsRecursive(); });
    // TODO implement subscribe/unsubscribe callbacks
    streamingServer.onSubscribe([](const std::string& signalId) {} );
    streamingServer.onUnsubscribe([](const std::string& signalId) {} );
    streamingServer.start(streamingPort, controlPort);

    packetReader.setLoopFrequency(50);
    packetReader.onPacket([this](const SignalPtr& signal, const ListPtr<IPacket>& packets) {
        const auto signalId = signal.getGlobalId();
        for (const auto& packet : packets)
            streamingServer.sendPacketToSubscribers(signalId, packet);
    });
    packetReader.startReading(device, context);

    // The control port is published thru the streaming protocol itself
    // so here the streaming port only is added to the StreamingInfo object
    StreamingInfoConfigPtr streamingInfo = StreamingInfo("daq.wss");
    streamingInfo.addProperty(IntProperty("Port", streamingPort));
    ErrCode errCode = this->device.asPtr<IDevicePrivate>()->addStreamingOption(streamingInfo);
    checkErrorInfo(errCode);
}

void WebsocketStreamingServer::stop()
{
    ErrCode errCode =
        this->device.asPtr<IDevicePrivate>()->removeStreamingOption(String("daq.wss"));
    checkErrorInfo(errCode);
    stopInternal();
}

void WebsocketStreamingServer::stopInternal()
{
    packetReader.stopReading();
    streamingServer.stop();
}

END_NAMESPACE_OPENDAQ_WEBSOCKET_STREAMING
