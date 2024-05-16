#include <coretypes/validation.h>
#include <opendaq/connection_impl.h>
#include <opendaq/data_packet_ptr.h>
#include <opendaq/event_packet_ids.h>
#include <opendaq/event_packet_ptr.h>
#include <opendaq/event_packet_params.h>
#include <opendaq/custom_log.h>

#include "opendaq/packet_factory.h"

BEGIN_NAMESPACE_OPENDAQ

ConnectionImpl::ConnectionImpl(const InputPortPtr& port, const SignalPtr& signal, ContextPtr context)
    : port(port)
    , signalRef(signal)
    , context(std::move(context))
    , queueEmpty(true)
    , loggerComponent(this->context.getLogger().getOrAddComponent("daq_connection"))
{
    const auto portConfig = port.asPtrOrNull<IInputPortConfig>(true);
    if (portConfig.assigned() && portConfig.getGapCheckingEnabled())
    {
        gapCheckState = GapCheckState::uninitialized;
        LOGP_D("Gap checking enabled.")
    }
    else
    {
        gapCheckState = GapCheckState::disabled;
        LOGP_T("Gap checking disabled.")
    }
}

template <class P, class F>
ErrCode ConnectionImpl::enqueueInternal(P&& packet, const F& f)
{
    return daqTry(
        [this, &packet, &f]
        {
            if (!port.getActive())
            {
                const auto type = packet.getType();
                if (type != PacketType::Event)
                    return OPENDAQ_IGNORED;
                LOGP_T("Port not active, data packet dropped.")
            }

			bool queueWasEmpty;

            withLock(
                [&packet, &queueWasEmpty, this]()
                {
                    queueWasEmpty = queueEmpty;
                    if (gapCheckState != GapCheckState::disabled)
                        checkForGaps(packet);

                    packets.emplace_back(std::forward<P>(packet));
                    queueEmpty = false;
                    LOGP_T("Packet enqueued.")
                });

            f(queueWasEmpty);
            return OPENDAQ_SUCCESS;
        });
}

ErrCode ConnectionImpl::enqueue(IPacket* packet)
{
    OPENDAQ_PARAM_NOT_NULL(packet);

    const auto packetPtr = PacketPtr::Borrow(packet);

    return enqueueInternal(packetPtr, [this](bool queueWasEmpty) { port.notifyPacketEnqueued(queueWasEmpty); });
}

ErrCode INTERFACE_FUNC ConnectionImpl::enqueueOnThisThread(IPacket* packet)
{
    OPENDAQ_PARAM_NOT_NULL(packet);

    const auto packetPtr = PacketPtr::Borrow(packet);

    return enqueueInternal(packetPtr, [this](bool) { port.notifyPacketEnqueuedOnThisThread(); });
}

#if _MSC_VER < 1920

ErrCode ConnectionImpl::enqueueMultipleInternal(const ListPtr<IPacket>& packets)
{
    return daqTry([this, &packets] {
        if (!port.getActive())
            return OPENDAQ_IGNORED;

        bool queueWasEmpty;

        withLock([&packets, &queueWasEmpty, this]() {
            queueWasEmpty = queueEmpty;
            const size_t cnt = packets.getCount();
            for (size_t i = 0; i < cnt; ++i)
            {
                this->packets.push_back(packets.getItemAt(i));
            }
            queueEmpty = false;
        });

        port.notifyPacketEnqueued(queueWasEmpty);
        return OPENDAQ_SUCCESS;
    });
}

ErrCode ConnectionImpl::enqueueMultipleInternal(ListPtr<IPacket>&& packets)
{
    return daqTry([this, &packets] {
        if (!port.getActive())
            return OPENDAQ_IGNORED;

        bool queueWasEmpty;

        withLock([&packets, &queueWasEmpty, this]() {
            queueWasEmpty = queueEmpty;
            const size_t cnt = packets.getCount();
            for (size_t i = 0; i < cnt; ++i)
                this->packets.push_back(packets.popBack());
            queueEmpty = false;
        });

        port.notifyPacketEnqueued(queueWasEmpty);
        return OPENDAQ_SUCCESS;
    });
}

#else

template <class P>
ErrCode ConnectionImpl::enqueueMultipleInternal(P&& packets)
{
    return daqTry(
        [this, &packets]
        {
            if (!port.getActive())
                return OPENDAQ_IGNORED;

            bool queueWasEmpty;

            withLock(
                [&packets, &queueWasEmpty, this]()
                {
                    queueWasEmpty = queueEmpty;
                    const size_t cnt = packets.getCount();
                    for (size_t i = 0; i < cnt; ++i)
                    {
                        if constexpr (std::is_rvalue_reference_v<P&&>)
                        {
                            this->packets.push_back(packets.popBack());
                        }
                        else
                        {
                            this->packets.push_back(packets.getItemAt(i));
                        }
                    }
                    queueEmpty = false;
                });

            port.notifyPacketEnqueued(queueWasEmpty);
            return OPENDAQ_SUCCESS;
        });
}

#endif
 
ErrCode INTERFACE_FUNC ConnectionImpl::enqueueMultiple(IList* packets)
{
    OPENDAQ_PARAM_NOT_NULL(packets);

    const auto packetsPtr = ListPtr<IPacket>::Borrow(packets);

    return enqueueMultipleInternal(packetsPtr);
}

ErrCode INTERFACE_FUNC ConnectionImpl::enqueueAndStealRef(IPacket* packet)
{
    OPENDAQ_PARAM_NOT_NULL(packet);

    auto packetPtr = PacketPtr::Adopt(packet);

    return enqueueInternal(std::move(packetPtr), [this](bool queueWasEmpty) { port.notifyPacketEnqueued(queueWasEmpty); });
}

ErrCode INTERFACE_FUNC ConnectionImpl::enqueueMultipleAndStealRef(IList* packets)
{
    OPENDAQ_PARAM_NOT_NULL(packets);

    auto packetsPtr = ListPtr<IPacket>::Adopt(packets);

    return enqueueMultipleInternal(std::move(packetsPtr));
}

ErrCode ConnectionImpl::dequeue(IPacket** packet)
{
    OPENDAQ_PARAM_NOT_NULL(packet);

    return withLock([&packet, this]()
    {
        if (packets.empty())
        {
            queueEmpty = true;
            LOGP_T("No packet to dequeue.")
            *packet = nullptr;
            return OPENDAQ_NO_MORE_ITEMS;
        }

        *packet = packets.front().detach();
        packets.pop_front();
        LOGP_T("Packet dequeued.")

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
            LOGP_T("No packet to peek.")
            *packet = nullptr;
            return OPENDAQ_NO_MORE_ITEMS;
        }

        *packet = packets.front().addRefAndReturn();
        LOGP_T("Packet peeked.")
        return OPENDAQ_SUCCESS;
    });
}

ErrCode ConnectionImpl::getPacketCount(SizeT* packetCount)
{
    OPENDAQ_PARAM_NOT_NULL(packetCount);

    return withLock([&packetCount, this]()
    {
        *packetCount = packets.size();
        LOG_T("Packet count = {}.", *packetCount)
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

        LOG_T("Available samples = {}.", *samples)
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
                        LOG_T("Samples until next descriptor = {}.", *samples)
                        return OPENDAQ_SUCCESS;
                    }
                    break;
                }
                case PacketType::None:
                    break;
            }
        }

        LOG_T("Samples until next descriptor = {}.", *samples)
        return OPENDAQ_SUCCESS;
    });
}

ErrCode ConnectionImpl::isRemote(Bool* remote)
{
    OPENDAQ_PARAM_NOT_NULL(remote);

    *remote = False;
    LOG_T("Remote = {}.", *remote)
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

    return daqTry(
        [this, &signal]
        {
            auto sig = this->signalRef.getRef();
            *signal = sig.detach();
            LOG_T("Signal = {}.", sig.assigned() ? sig.getGlobalId().toStdString() : "null")
            return OPENDAQ_SUCCESS;
        });
}

ErrCode ConnectionImpl::getInputPort(IInputPort** inputPort)
{
    OPENDAQ_PARAM_NOT_NULL(inputPort);

    *inputPort = this->port.addRefAndReturn();
    LOG_T("InputPort = {}.", this->port.getGlobalId().toStdString());
    return OPENDAQ_SUCCESS;
}

const std::deque<PacketPtr>& ConnectionImpl::getPackets() const noexcept
{
    return packets;
}

void ConnectionImpl::checkForGaps(const PacketPtr& packet)
{
    assert(gapCheckState != GapCheckState::disabled);

    switch (packet.getType())
    {
        case PacketType::Data:
        {
            if (gapCheckState == GapCheckState::uninitialized)
                throw InvalidStateException("No event packet received.");

            if (gapCheckState != GapCheckState::not_available)
            {
                const auto dataPacket = packet.asPtr<IDataPacket>(true);
                const auto domainPacket = dataPacket.getDomainPacket();
                assert(domainPacket.assigned());

                if (gapCheckState == GapCheckState::initialized)
                    beginGapCheck(domainPacket);
                else
                {
                    DomainValue diff;
                    if (doGapCheck(domainPacket, diff))
                        enqueueGapPacket(diff);
                }
            }

            break;
        }
        case PacketType::Event:
            initGapCheck(packet.asPtr<IEventPacket>(true));
            break;
        default:
            break;
    }
}

void ConnectionImpl::enqueueGapPacket(const DomainValue& diff)
{
    NumberPtr diffNumber;
    if (domainSampleType == SampleType::Float64)
        diffNumber = diff.valueDouble;
    else
        diffNumber = diff.valueInt64_t;

    const auto gapPacket = ImplicitDomainGapDetectedEventPacket(diffNumber);
    packets.emplace_back(gapPacket);
    LOGP_T("Gap packet enqueued.")
}

void ConnectionImpl::beginGapCheck(const DataPacketPtr& domainPacket)
{
    nextExpectedPacketOffset = numberToDomainValue(domainPacket.getOffset());
    if (domainSampleType == SampleType::Float64)
        nextExpectedPacketOffset.valueDouble += static_cast<double>(domainPacket.getSampleCount()) * delta.valueDouble;
    else
        nextExpectedPacketOffset.valueInt64_t += static_cast<int64_t>(domainPacket.getSampleCount()) * delta.valueInt64_t;

    gapCheckState = GapCheckState::running;
    LOGP_T("Gap check started.")
}

bool ConnectionImpl::doGapCheck(const DataPacketPtr& domainPacket, DomainValue& diff)
{
    const auto currPacketOffset = numberToDomainValue(domainPacket.getOffset());
    bool gapDetected;

    if (domainSampleType == SampleType::Float64)
    {
        diff.valueDouble = currPacketOffset.valueDouble - nextExpectedPacketOffset.valueDouble;

        // floating point comparison is not exact, we have to use some epsilon
        // here we choose empiric value 1/10 of delta between two samples
        gapDetected = std::abs(diff.valueDouble) > delta.valueDouble / 10.0;
    }
    else
    {
        diff.valueInt64_t = currPacketOffset.valueInt64_t - nextExpectedPacketOffset.valueInt64_t;
        gapDetected = diff.valueInt64_t != 0;
    }

    if (domainSampleType == SampleType::Float64)
        nextExpectedPacketOffset.valueDouble = currPacketOffset.valueDouble + static_cast<double>(domainPacket.getSampleCount()) * delta.valueDouble;
    else
        nextExpectedPacketOffset.valueInt64_t = currPacketOffset.valueInt64_t + static_cast<int64_t>(domainPacket.getSampleCount()) * delta.valueInt64_t;

#if (OPENDAQ_LOG_LEVEL <= OPENDAQ_LOG_LEVEL_DEBUG)
    if (gapDetected)
    {
        if (domainSampleType == SampleType::Float64)
        {
            LOG_D("Gap detected, diff = {}.", diff.valueDouble)
        }
        else
        {
            LOG_D("Gap detected, diff = {}.", diff.valueInt64_t)
        }
    }
#endif

    return gapDetected;
}

void ConnectionImpl::initGapCheck(const EventPacketPtr& packet)
{
    if (packet.getEventId() == event_packet_id::DATA_DESCRIPTOR_CHANGED)
    {
        const auto params = packet.getParameters();
        const DataDescriptorPtr valueDescriptor = params.get(event_packet_param::DATA_DESCRIPTOR);
        const DataDescriptorPtr domainDescriptor = params.get(event_packet_param::DOMAIN_DATA_DESCRIPTOR);
        if (!domainDescriptor.assigned())
        {
            if (gapCheckState == GapCheckState::uninitialized)
            {
                LOGP_T("Gap check not available, no domain signal.")
                gapCheckState = GapCheckState::not_available;
            }

            // domain not changed, keep state as it is
            return;
        }

        const auto rule = domainDescriptor.getRule();
        if (rule.getType() != DataRuleType::Linear)
        {
            LOGP_T("Gap check not available, no linear rule.")
            gapCheckState = GapCheckState::not_available;
            return;
        }

        domainSampleType = domainDescriptor.getSampleType();
        if (domainSampleType == SampleType::Float64)
            delta.valueDouble = rule.getParameters()["delta"];
        else if (domainSampleType == SampleType::Int64 || domainSampleType == SampleType::UInt64)
            delta.valueInt64_t = rule.getParameters()["delta"];
        else
        {
            LOGP_T("Gap check not available, invalid domain sample type.")
            gapCheckState = GapCheckState::not_available;
            return;
        }

        LOGP_T("Gap check initiaized")
        gapCheckState = GapCheckState::initialized;

    }
    else if (packet.getEventId() == event_packet_id::IMPLICIT_DOMAIN_GAP_DETECTED)
    {
        throw InvalidOperationException("Gap packets should not be inserted into connection queue from outside.");
    }
}

ConnectionImpl::DomainValue ConnectionImpl::numberToDomainValue(const NumberPtr& number)
{
    DomainValue dv;
    switch (domainSampleType)
    {
        case SampleType::Int64:
        case SampleType::UInt64:
            dv.valueInt64_t = number;
            break;
        case SampleType::Float64:
            dv.valueDouble = number;
            break;
        default:
            throw InvalidParameterException("Cannot convert number.");
    }
    return dv;
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
