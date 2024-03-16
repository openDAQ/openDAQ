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
    , queueEmpty(true)
{
}

ErrCode ConnectionImpl::enqueue(IPacket* packet)
{
    OPENDAQ_PARAM_NOT_NULL(packet);

    bool queueWasEmpty;

    withLock([&packet, &queueWasEmpty, this]()
    {
        queueWasEmpty = queueEmpty;
        packets.emplace_back(packet);
        queueEmpty = false;
    });

    port.notifyPacketEnqueued(queueWasEmpty);
    return OPENDAQ_SUCCESS;
}

ErrCode INTERFACE_FUNC ConnectionImpl::enqueueMultiple(IList* packets)
{
    OPENDAQ_PARAM_NOT_NULL(packets);

    bool queueWasEmpty;

    withLock([&packets, &queueWasEmpty, this]()
        {
            queueWasEmpty = queueEmpty;
            const auto packetsPtr = ListPtr<IPacket>::Borrow(packets);
            for (size_t i = 0; i < packetsPtr.getCount(); ++i)
                this->packets.push_back(packetsPtr.getItemAt(i).detach());
            queueEmpty = false;
        });

    port.notifyPacketEnqueued(queueWasEmpty);
    return OPENDAQ_SUCCESS;
}

ErrCode INTERFACE_FUNC ConnectionImpl::enqueueAndSteal(IPacket* packet)
{
    OPENDAQ_PARAM_NOT_NULL(packet);

    auto packetPtr = PacketPtr::Adopt(packet);
    bool queueWasEmpty;

    withLock([&packetPtr, &queueWasEmpty, this]()
    {
        queueWasEmpty = queueEmpty;
        packets.push_back(std::move(packetPtr));
        queueEmpty = false;
    });

    port.notifyPacketEnqueued(queueWasEmpty);
    return OPENDAQ_SUCCESS;
}

ErrCode INTERFACE_FUNC ConnectionImpl::enqueueAndStealMultiple(IList* packets)
{
    OPENDAQ_PARAM_NOT_NULL(packets);

    bool queueWasEmpty;
    auto packetsPtr = ListPtr<IPacket>::Adopt(packets);

    withLock(
        [&packetsPtr, &queueWasEmpty, this]()
        {
            queueWasEmpty = queueEmpty;
            for (size_t i = 0; i < packetsPtr.getCount(); ++i)
                this->packets.push_back(packetsPtr.getItemAt(i).detach());
            packetsPtr.clear();
            queueEmpty = false;
        });

    port.notifyPacketEnqueued(queueWasEmpty);
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
            queueEmpty = true;
            *packet = nullptr;
            return OPENDAQ_NO_MORE_ITEMS;
        }

        *packet = packets.front().detach();
        packets.pop_front();

        return OPENDAQ_SUCCESS;
    });
}

ErrCode INTERFACE_FUNC ConnectionImpl::dequeueAll(IList** packets)
{
    OPENDAQ_PARAM_NOT_NULL(packets);

    auto packetsPtr = List<IPacket>();

    return withLock(
        [&packetsPtr, packets, this]()
        {
            for (auto& packet : this->packets)
                packetsPtr.pushBack(std::move(packet));
            this->packets.clear();

            *packets = packetsPtr.detach();
            return OPENDAQ_NO_MORE_ITEMS;
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

ErrCode ConnectionImpl::queryInterface(const IntfID& id, void** intf)
{
    OPENDAQ_PARAM_NOT_NULL(intf);

    if (id == IConnection::Id)
    {
        *intf = static_cast<IConnection*>(this);
        this->addRef();

        return OPENDAQ_SUCCESS;
    }

    return Super::queryInterface(id, intf);
}

ErrCode ConnectionImpl::borrowInterface(const IntfID& id, void** intf) const
{
    OPENDAQ_PARAM_NOT_NULL(intf);

    if (id == IConnection::Id)
    {
        *intf = const_cast<IConnection*>(static_cast<const IConnection*>(this));

        return OPENDAQ_SUCCESS;
    }

    return Super::borrowInterface(id, intf);
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
