#include <cstdint>

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
    : _signal(signal)
    , _port(
        InputPort(
            context,
            nullptr,
            String("ws-streaming")))
    , _lastDescriptor(_signal.getDescriptor())
    , _localSignal(*localSignal)
{
    _localSignal.set_metadata(
        descriptorToMetadata(
            signal,
            _lastDescriptor));
}

void WsStreamingListener::start()
{
    _port.setListener(this->template thisPtr<InputPortNotificationsPtr>());
    _port.setNotificationMethod(PacketReadyNotification::SameThread);
    _port.connect(_signal);
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
        auto packet = _port.getConnection().dequeue();
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

    if (descriptor != _lastDescriptor)
    {
        _localSignal.set_metadata(
            descriptorToMetadata(
                _signal,
                descriptor));

        _lastDescriptor = descriptor;
    }

    if (packet.getRawDataSize())
        _localSignal.publish_data(
            offset,
            packet.getSampleCount(),
            packet.getRawData(),
            packet.getRawDataSize());
}

END_NAMESPACE_OPENDAQ_NEWER_WEBSOCKET_STREAMING_SERVER_MODULE
