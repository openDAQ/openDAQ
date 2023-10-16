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

void WebsocketStreamingServer::start()
{
    if (!device.assigned())
        throw InvalidStateException("Device is not set.");
    if (!context.assigned())
        throw InvalidStateException("Context is not set.");
    if (streamingPort == 0)
        return;

    streamingServer.onAccept([this](const daq::streaming_protocol::StreamWriterPtr& writer) { return device.getSignalsRecursive(); });
    streamingServer.start(streamingPort);

    packetReader.setLoopFrequency(50);
    packetReader.onPacket([this](const SignalPtr& signal, const ListPtr<IPacket>& packets) {
        const auto signalId = signal.getGlobalId();
        for (const auto& packet : packets)
            streamingServer.broadcastPacket(signalId, packet);
    });
    packetReader.startReading(device, context);

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
