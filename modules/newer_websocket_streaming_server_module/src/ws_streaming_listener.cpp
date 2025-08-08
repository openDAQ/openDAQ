#include <iostream>

#include <opendaq/opendaq.h>

#include <ws-streaming/local_signal.hpp>

#include <websocket_streaming_server_module/common.h>
#include <websocket_streaming_server_module/descriptor_to_metadata.h>
#include <websocket_streaming_server_module/ws_streaming_listener.h>

BEGIN_NAMESPACE_OPENDAQ_NEWER_WEBSOCKET_STREAMING_SERVER_MODULE

WsStreamingListener::WsStreamingListener(
        IContext *context,
        ISignal *signal,
        wss::local_signal *localSignal)
    : signal(signal)
    , port(
        InputPort(
            context,
            nullptr,
            String("ws-streaming")))
    , lastDescriptor(this->signal.getDescriptor())
    , localSignal(*localSignal)
{
    std::cout << "listener created" << std::endl;

    this->localSignal.set_metadata(
        descriptorToMetadata(
            signal,
            lastDescriptor));

    // XXX TODO - why?
    internalAddRef();
}

void WsStreamingListener::start()
{
    port.setListener(this->template thisPtr<InputPortNotificationsPtr>());
    port.setNotificationMethod(PacketReadyNotification::SameThread);
    port.connect(signal);
}

ErrCode WsStreamingListener::acceptsSignal(
    IInputPort *port,
    ISignal *signal,
    Bool *accept)
{
    *accept = true;

    return OPENDAQ_SUCCESS;
};

ErrCode WsStreamingListener::connected(IInputPort *port)
{
    return OPENDAQ_SUCCESS;
}

ErrCode WsStreamingListener::disconnected(IInputPort *port)
{
    return OPENDAQ_SUCCESS;
}

ErrCode WsStreamingListener::packetReceived(IInputPort *port)
{
    while (true)
    {
        auto packet = this->port.getConnection().dequeue();
        if (!packet.assigned())
            break;

        if (packet.getType() == PacketType::Data)
            onDataPacketReceived(packet);
    }

    return OPENDAQ_SUCCESS;
}

void WsStreamingListener::onDataPacketReceived(DataPacketPtr packet)
{
    std::int64_t offset = 0;

    if (auto domainPacket = packet.getDomainPacket(); domainPacket.assigned())
        if (auto offsetPtr = domainPacket.getOffset(); offsetPtr.assigned())
            offset = offsetPtr;

    auto descriptor = packet.getDataDescriptor();

    if (descriptor != lastDescriptor)
    {
        std::cout << '(' << signal.getGlobalId() << ") setting metadata" << std::endl;
        localSignal.set_metadata(
            descriptorToMetadata(
                descriptor,
                signal));

        lastDescriptor = descriptor;
    }

    if (packet.getRawDataSize())
        localSignal.publish_data(
            offset,
            packet.getSampleCount(),
            packet.getRawData(),
            packet.getRawDataSize());
}

END_NAMESPACE_OPENDAQ_NEWER_WEBSOCKET_STREAMING_SERVER_MODULE
