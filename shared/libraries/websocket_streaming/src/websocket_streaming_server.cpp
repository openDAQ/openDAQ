#include <opendaq/packet.h>
#include <opendaq/reader_factory.h>
#include <iostream>
#include "websocket_streaming/websocket_streaming_server.h"
#include <opendaq/device_private.h>
#include <coreobjects/property_factory.h>
#include <opendaq/search_filter_factory.h>
#include <opendaq/device_info_factory.h>
#include <opendaq/device_info_internal_ptr.h>

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
    , packetReader(device, context)
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

    streamingServer.onAccept([this](const daq::streaming_protocol::StreamWriterPtr& writer) { return device.getSignals(search::Recursive(search::Any())); });
    streamingServer.onSubscribe([this](const daq::SignalPtr& signal) { packetReader.startReadSignal(signal); } );
    streamingServer.onUnsubscribe([this](const daq::SignalPtr& signal) { packetReader.stopReadSignal(signal); } );
    streamingServer.start(streamingPort, controlPort);

    packetReader.setLoopFrequency(50);
    packetReader.onPacket([this](const SignalPtr& signal, const ListPtr<IPacket>& packets) {
        const auto signalId = signal.getGlobalId();
        for (const auto& packet : packets)
            streamingServer.sendPacketToSubscribers(signalId, packet);
    });
    packetReader.start();

    ServerCapabilityPtr serverCapability = ServerCapability("openDAQ WebsocketTcp Streaming", ProtocolType::Streaming);
    serverCapability.setPropertyValue("protocolId", "daq.ws");
    serverCapability.addProperty(IntProperty("Port", streamingPort));
    this->device.getInfo().asPtr<IDeviceInfoInternal>().addServerCapability(serverCapability);
}

void WebsocketStreamingServer::stop()
{
    this->device.getInfo().asPtr<IDeviceInfoInternal>().removeServerCapability(String("daq.ws"));
    stopInternal();
}

void WebsocketStreamingServer::stopInternal()
{
    packetReader.stop();
    streamingServer.stop();
}

END_NAMESPACE_OPENDAQ_WEBSOCKET_STREAMING
