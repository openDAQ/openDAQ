#include <iostream>

#include <opendaq/opendaq.h>

#include <ws-streaming/ws-streaming.hpp>

#include <newer_websocket_streaming_server_module/common.h>
#include <newer_websocket_streaming_server_module/newer_websocket_streaming_listener.h>

BEGIN_NAMESPACE_OPENDAQ_NEWER_WEBSOCKET_STREAMING_SERVER_MODULE

NewerWebsocketStreamingListenerImpl::NewerWebsocketStreamingListenerImpl(
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
        buildMetadata(lastDescriptor)
            .build());

    // XXX TODO - why?
    internalAddRef();
}

NewerWebsocketStreamingListenerImpl::~NewerWebsocketStreamingListenerImpl()
{
    std::cout << "listener destroyed" << std::endl;
}

void NewerWebsocketStreamingListenerImpl::start()
{
    port.setListener(this->template thisPtr<InputPortNotificationsPtr>());
    port.setNotificationMethod(PacketReadyNotification::SameThread);
    port.connect(signal);
}

ErrCode NewerWebsocketStreamingListenerImpl::acceptsSignal(
    IInputPort *port,
    ISignal *signal,
    Bool *accept)
{
    *accept = true;

    return OPENDAQ_SUCCESS;
};

ErrCode NewerWebsocketStreamingListenerImpl::connected(IInputPort *port)
{
    return OPENDAQ_SUCCESS;
}

ErrCode NewerWebsocketStreamingListenerImpl::disconnected(IInputPort *port)
{
    return OPENDAQ_SUCCESS;
}

ErrCode NewerWebsocketStreamingListenerImpl::packetReceived(IInputPort *port)
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

wss::metadata_builder NewerWebsocketStreamingListenerImpl::buildMetadata(
    const DataDescriptorPtr& descriptor)
{
    auto builder = wss::metadata_builder{descriptor.getName()};

    switch (descriptor.getSampleType())
    {
        case SampleType::Float32:
            builder.data_type(wss::data_types::real32_t);
            break;
        case SampleType::Float64:
            builder.data_type(wss::data_types::real64_t);
            break;
        case SampleType::Int8:
            builder.data_type(wss::data_types::int8_t);
            break;
        case SampleType::Int16:
            builder.data_type(wss::data_types::int16_t);
            break;
        case SampleType::Int32:
            builder.data_type(wss::data_types::int32_t);
            break;
        case SampleType::Int64:
            builder.data_type(wss::data_types::int64_t);
            break;
        case SampleType::UInt8:
            builder.data_type(wss::data_types::uint8_t);
            break;
        case SampleType::UInt16:
            builder.data_type(wss::data_types::uint16_t);
            break;
        case SampleType::UInt32:
            builder.data_type(wss::data_types::uint32_t);
            break;
        case SampleType::UInt64:
            builder.data_type(wss::data_types::uint64_t);
            break;
        default:
            break;
    }

    if (auto originPtr = descriptor.getOrigin(); originPtr.assigned())
        builder.origin(originPtr);

    if (auto resolutionPtr = descriptor.getTickResolution(); resolutionPtr.assigned())
        builder.tick_resolution(
            resolutionPtr.getNumerator(),
            resolutionPtr.getDenominator());

    if (auto rangePtr = descriptor.getValueRange(); rangePtr.assigned())                
        builder.range(
            rangePtr.getLowValue(),
            rangePtr.getHighValue());

    if (auto unitPtr = descriptor.getUnit(); unitPtr.assigned())
        builder.unit(
            unitPtr.getId(),
            unitPtr.getName(),
            unitPtr.getQuantity(),
            unitPtr.getSymbol());

    if (auto domainSignalPtr = signal.getDomainSignal(); domainSignalPtr.assigned())
        builder.table(domainSignalPtr.getGlobalId());

    if (auto rulePtr = descriptor.getRule(); rulePtr.assigned())
    {
        if (rulePtr.getType() == DataRuleType::Linear)
        {
            std::int64_t start = 0, delta = 1;

            if (auto paramsPtr = rulePtr.getParameters(); paramsPtr.assigned())
            {
                if (auto startPtr = paramsPtr.get("start"); startPtr.assigned())
                    start = startPtr;
                if (auto deltaPtr = paramsPtr.get("delta"); deltaPtr.assigned())
                    delta = deltaPtr;
            }

            builder.linear_rule(start, delta);
        }
    }

    return builder;
}

void NewerWebsocketStreamingListenerImpl::onDataPacketReceived(DataPacketPtr packet)
{
    std::int64_t offset = 0;

    if (auto domainPacket = packet.getDomainPacket(); domainPacket.assigned())
        if (auto offsetPtr = domainPacket.getOffset(); offsetPtr.assigned())
            offset = offsetPtr;

    auto descriptor = packet.getDataDescriptor();

    if (descriptor != lastDescriptor)
    {
        std::cout << '(' << signal.getGlobalId() << ") setting metadata" << std::endl;
        localSignal.set_metadata(buildMetadata(descriptor).build());

        lastDescriptor = descriptor;
    }

    if (packet.getRawDataSize())
        localSignal.publish_data(
            offset,
            packet.getSampleCount(),
            packet.getRawData(),
            packet.getRawDataSize());
}

OPENDAQ_DEFINE_CLASS_FACTORY_WITH_INTERFACE(
    INTERNAL_FACTORY, NewerWebsocketStreamingListener, IInputPortNotifications,
    IContext *, context,
    ISignal *, signal,
    wss::local_signal *, localSignal
)

END_NAMESPACE_OPENDAQ_NEWER_WEBSOCKET_STREAMING_SERVER_MODULE
