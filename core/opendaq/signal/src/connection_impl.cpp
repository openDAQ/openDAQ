#include <coretypes/validation.h>
#include <opendaq/connection_impl.h>
#include <opendaq/data_packet_ptr.h>
#include <opendaq/event_packet_ids.h>
#include <opendaq/event_packet_ptr.h>

BEGIN_NAMESPACE_OPENDAQ
ConnectionImpl::ConnectionImpl(const InputPortPtr& port, const SignalPtr& signal, ContextPtr context)
    : port(port)
    , signalRef(signal)
    , context(std::move(context))
{
}

ErrCode ConnectionImpl::enqueue(IPacket* packet)
{
    OPENDAQ_PARAM_NOT_NULL(packet);

    withLock([&packet, this]()
    {
        packets.emplace_back(packet);
    });

    port.notifyPacketEnqueued();
    return OPENDAQ_SUCCESS;
}

ErrCode INTERFACE_FUNC ConnectionImpl::enqueueOnThisThread(IPacket* packet)
{
    OPENDAQ_PARAM_NOT_NULL(packet);

    withLock([&packet, this]()
    {
        packets.emplace_back(packet);
    });

    port.notifyPacketEnqueuedOnThisThread();
    return OPENDAQ_SUCCESS;
}

ErrCode ConnectionImpl::dequeue(IPacket** packet)
{
    OPENDAQ_PARAM_NOT_NULL(packet);

    return withLock([&packet, this]()
    {
        if (packets.empty())
        {
            *packet = nullptr;
            return OPENDAQ_NO_MORE_ITEMS;
        }

        *packet = packets.front().addRefAndReturn();
        packets.pop_front();

        return OPENDAQ_SUCCESS;
    });
}

ErrCode ConnectionImpl::peek(IPacket** packet)
{
    OPENDAQ_PARAM_NOT_NULL(packet);

    return withLock([&packet, this]()
    {
        if (packets.empty())
        {
            *packet = nullptr;
            return OPENDAQ_NO_MORE_ITEMS;
        }

        *packet = packets.front().addRefAndReturn();
        return OPENDAQ_SUCCESS;
    });
}

ErrCode ConnectionImpl::getPacketCount(SizeT* packetCount)
{
    OPENDAQ_PARAM_NOT_NULL(packetCount);

    return withLock([&packetCount, this]()
    {
        *packetCount = packets.size();
        return OPENDAQ_SUCCESS;
    });
}

ErrCode ConnectionImpl::getAvailableSamples(SizeT* samples)
{
    OPENDAQ_PARAM_NOT_NULL(samples);

    return withLock([samples, this]()
    {
        *samples = 0;
        for (const auto& packet : packets)
        {
            if (packet.getType() == PacketType::Data)
            {
                auto dataPacket = packet.template asPtrOrNull<IDataPacket>(true);
                if (dataPacket.assigned())
                {
                    *samples += dataPacket.getSampleCount();
                }
            }
        }

        return OPENDAQ_SUCCESS;
    });
}

ErrCode ConnectionImpl::getSamplesUntilNextDescriptor(SizeT* samples)
{
    OPENDAQ_PARAM_NOT_NULL(samples);

    return withLock([samples, this]() {
        *samples = 0;
        for (const auto& packet : packets)
        {
            switch (packet.getType())
            {
                case PacketType::Data:
                {
                    auto dataPacket = packet.template asPtrOrNull<IDataPacket>(true);
                    if (dataPacket.assigned())
                    {
                        *samples += dataPacket.getSampleCount();
                    }
                    break;
                }
                case PacketType::Event:
                {
                    auto eventPacket = packet.template asPtrOrNull<IEventPacket>(true);
                    if (eventPacket.getEventId() == event_packet_id::DATA_DESCRIPTOR_CHANGED)
                    {
                        return OPENDAQ_SUCCESS;
                    }
                    break;
                }
                case PacketType::None:
                    break;
            }
        }

        return OPENDAQ_SUCCESS;
    });
}

ErrCode ConnectionImpl::isRemote(Bool* remote)
{
    OPENDAQ_PARAM_NOT_NULL(remote);

    *remote = False;
    return OPENDAQ_SUCCESS;
}

ErrCode ConnectionImpl::getSignal(ISignal** signal)
{
    OPENDAQ_PARAM_NOT_NULL(signal);

    OPENDAQ_TRY({
        *signal = this->signalRef.getRef().detach();
        return OPENDAQ_SUCCESS;
    })
}

ErrCode ConnectionImpl::getInputPort(IInputPort** inputPort)
{
    OPENDAQ_PARAM_NOT_NULL(inputPort);

    *inputPort = this->port.addRefAndReturn();
    return OPENDAQ_SUCCESS;
}

const std::deque<PacketPtr>& ConnectionImpl::getPackets() const noexcept
{
    return packets;
}

OPENDAQ_DEFINE_CLASS_FACTORY(
    LIBRARY_FACTORY,
    Connection,
    IInputPort*,
    inputPort,
    ISignal*,
    signal,
    IContext*,
    context
)

END_NAMESPACE_OPENDAQ
