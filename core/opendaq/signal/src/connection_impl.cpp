#include <coretypes/validation.h>
#include <opendaq/connection_impl.h>
#include <opendaq/data_packet_ptr.h>
#include <opendaq/event_packet_ids.h>
#include <opendaq/event_packet_ptr.h>
#include <opendaq/event_packet_params.h>
#include <opendaq/event_packet_utils.h>
#include <opendaq/custom_log.h>

#include "opendaq/data_descriptor_factory.h"
#include "opendaq/packet_factory.h"

BEGIN_NAMESPACE_OPENDAQ

ConnectionImpl::ConnectionImpl(const InputPortPtr& port, const SignalPtr& signal, ContextPtr context)
    : port(port)
    , signalRef(signal)
    , context(std::move(context))
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

ConnectionImpl::~ConnectionImpl()
{
    // refcount reached zero: no producer or consumer can be inside the gate anymore
    if (IPacket* front = pendingFrontDescriptor.exchange(nullptr, std::memory_order_acquire))
        front->releaseRef();
    destroyChain(pendingDrainChain);
    destroyChain(inboxTop.exchange(nullptr, std::memory_order_acquire));
    destroyChain(freeTop.exchange(nullptr, std::memory_order_acquire));
    destroyChain(nodeCache);
}

void ConnectionImpl::destroyChain(PacketNode* chain)
{
    while (chain)
    {
        PacketNode* next = chain->next.load(std::memory_order_relaxed);
        if (chain->packet)
            chain->packet->releaseRef();
        delete chain;
        chain = next;
    }
}

ConnectionImpl::PacketNode* ConnectionImpl::reverseChain(PacketNode* chain)
{
    PacketNode* reversed = nullptr;
    while (chain)
    {
        PacketNode* next = chain->next.load(std::memory_order_relaxed);
        chain->next.store(reversed, std::memory_order_relaxed);
        reversed = chain;
        chain = next;
    }
    return reversed;
}

ConnectionImpl::PacketNode* ConnectionImpl::acquireNode()
{
    // Enqueuer-private cache (all enqueues are externally serialized by the signal owner),
    // refilled wholesale from the shared free list. freeTop is only ever taken with
    // exchange (all at once), never CAS-popped, so the classic Treiber-pop ABA problem
    // cannot occur.
    if (!nodeCache)
        nodeCache = freeTop.exchange(nullptr, std::memory_order_acq_rel);

    if (nodeCache)
    {
        PacketNode* node = nodeCache;
        nodeCache = node->next.load(std::memory_order_relaxed);
        node->next.store(nullptr, std::memory_order_relaxed);
        node->gapCheck = true;
        return node;
    }
    return new PacketNode();
}

void ConnectionImpl::recycleNode(PacketNode* node)
{
    node->packet = nullptr;
    PacketNode* cur = freeTop.load(std::memory_order_relaxed);
    do
    {
        node->next.store(cur, std::memory_order_relaxed);
    } while (!freeTop.compare_exchange_weak(cur, node, std::memory_order_release, std::memory_order_relaxed));
}

bool ConnectionImpl::pushChain(PacketNode* first, PacketNode* last, bool& queueWasEmpty)
{
    details::ActivityCounter::Scope op(activeOps);

    // pairs with closeQueue(): a false read here precedes the closed store in the seq_cst
    // total order, so closeQueue()'s idle wait covers this whole push
    if (closedFlag.load(std::memory_order_seq_cst))
    {
        destroyChain(first);
        queueWasEmpty = false;
        return false;
    }

    PacketNode* cur = inboxTop.load(std::memory_order_relaxed);
    do
    {
        last->next.store(cur, std::memory_order_relaxed);
    } while (!inboxTop.compare_exchange_weak(cur, first, std::memory_order_release, std::memory_order_relaxed));

    queueWasEmpty = queueEmptyFlag.exchange(false, std::memory_order_acq_rel);
    return true;
}

void ConnectionImpl::latchDescriptors(const EventPacketPtr& packet)
{
    if (packet.getEventId() != event_packet_id::DATA_DESCRIPTOR_CHANGED)
        return;

    const auto params = packet.getParameters();
    const DataDescriptorPtr valueDescriptorParam = params[event_packet_param::DATA_DESCRIPTOR];
    const DataDescriptorPtr domainDescriptorParam = params[event_packet_param::DOMAIN_DATA_DESCRIPTOR];

    std::lock_guard lock(latchMutex);
    if (valueDescriptorParam.assigned())
        valueDataDescriptor = valueDescriptorParam;
    if (domainDescriptorParam.assigned())
        domainDataDescriptor = domainDescriptorParam;
}

template <class P, class F>
ErrCode ConnectionImpl::enqueueInternal(P&& packet, const F& f)
{
    const ErrCode errCode = daqTry([this, &packet, &f]
    {
        const auto type = packet.getType();

        if (!port.getActive())
        {
            if (type != PacketType::Event)
                return OPENDAQ_IGNORED;
            LOGP_T("Port not active, data packet dropped.")
        }

        // descriptor latch feeds enqueueLastDescriptor for listeners attached later
        if (type == PacketType::Event)
        {
            const auto eventPacket = packet.template asPtrOrNull<IEventPacket>(true);
            if (eventPacket.assigned())
                latchDescriptors(eventPacket);
        }

        PacketNode* node = acquireNode();
        if constexpr (std::is_rvalue_reference_v<P&&>)
            node->packet = packet.detach();
        else
            node->packet = packet.addRefAndReturn();

        bool queueWasEmpty;
        if (!pushChain(node, node, queueWasEmpty))
        {
            LOGP_T("Connection closed, packet dropped.")
            return OPENDAQ_IGNORED;
        }

        f(queueWasEmpty);
        LOGP_T("Packet enqueued.")
        return OPENDAQ_SUCCESS;
    });
    OPENDAQ_RETURN_IF_FAILED(errCode);
    return errCode;
}

template <class P, class F>
ErrCode ConnectionImpl::enqueueMultipleInternal(P&& packets, const F& f)
{
    const ErrCode errCode = daqTry([this, &packets, &f]
    {
        if (!port.getActive())
            return OPENDAQ_IGNORED;

        const size_t cnt = packets.getCount();
        if (cnt == 0)
            return OPENDAQ_IGNORED;

        // Build the chain newest-first so that pushing it as one unit is equivalent to
        // pushing the packets one by one (the consumer restores FIFO by reversing).
        PacketNode* first = nullptr;  // newest
        PacketNode* last = nullptr;   // oldest
        for (size_t i = 0; i < cnt; ++i)
        {
            PacketPtr packet;
            if constexpr (std::is_rvalue_reference_v<P&&>)
                packet = packets.popFront();
            else
                packet = packets.getItemAt(i);

            if (const auto eventPacket = packet.template asPtrOrNull<IEventPacket>(true); eventPacket.assigned())
                latchDescriptors(eventPacket);

            PacketNode* node = acquireNode();
            node->packet = packet.detach();
            node->gapCheck = false;  // multi-packet enqueue skipped gap checks historically
            node->next.store(first, std::memory_order_relaxed);
            first = node;
            if (!last)
                last = node;
        }

        bool queueWasEmpty;
        if (!pushChain(first, last, queueWasEmpty))
        {
            LOGP_T("Connection closed, packets dropped.")
            return OPENDAQ_IGNORED;
        }

        f(queueWasEmpty);
        return OPENDAQ_SUCCESS;
    });
    OPENDAQ_RETURN_IF_FAILED(errCode);
    return errCode;
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

ErrCode ConnectionImpl::enqueueWithScheduler(IPacket* packet)
{
    OPENDAQ_PARAM_NOT_NULL(packet);

    const auto packetPtr = PacketPtr::Borrow(packet);

    return enqueueInternal(packetPtr, [this](bool) { port.notifyPacketEnqueuedWithScheduler(); });
}

ErrCode INTERFACE_FUNC ConnectionImpl::enqueueMultiple(IList* packets)
{
    OPENDAQ_PARAM_NOT_NULL(packets);

    const auto packetsPtr = ListPtr<IPacket>::Borrow(packets);

    return enqueueMultipleInternal(packetsPtr, [this](bool queueWasEmpty) { port.notifyPacketEnqueued(queueWasEmpty); });
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

    return enqueueMultipleInternal(std::move(packetsPtr), [this](bool queueWasEmpty) { port.notifyPacketEnqueued(queueWasEmpty); });
}

void ConnectionImpl::drainInbox()
{
    // consumer thread, called inside the gate (via consumerOp)

    if (IPacket* front = pendingFrontDescriptor.exchange(nullptr, std::memory_order_acq_rel))
    {
        packets.emplace_front(PacketPtr::Adopt(front));
        eventPacketsCnt++;
    }

    PacketNode* chain = pendingDrainChain;
    pendingDrainChain = nullptr;
    PacketNode* fresh = reverseChain(inboxTop.exchange(nullptr, std::memory_order_acq_rel));
    if (!chain)
    {
        chain = fresh;
    }
    else if (fresh)
    {
        PacketNode* tail = chain;
        while (PacketNode* next = tail->next.load(std::memory_order_relaxed))
            tail = next;
        tail->next.store(fresh, std::memory_order_relaxed);
    }

    while (chain)
    {
        PacketNode* node = chain;
        chain = node->next.load(std::memory_order_relaxed);

        PacketPtr packet = PacketPtr::Adopt(node->packet);
        const bool doGapCheck = node->gapCheck;
        recycleNode(node);

        if (gapCheckState != GapCheckState::disabled && doGapCheck)
        {
            try
            {
                checkForGaps(packet);
            }
            catch (...)
            {
                // the offending packet is dropped (it was never part of the queue, matching
                // the historical enqueue-side behavior); the rest is drained on the next call
                pendingDrainChain = chain;
                throw;
            }
        }

        onPacketEnqueuedCounters(packet);
        packets.emplace_back(std::move(packet));
        LOGP_T("Packet enqueued.")
    }
}

template <class ClosedF, class F>
ErrCode ConnectionImpl::consumerOp(const ClosedF& closedBody, const F& body)
{
    return daqTry([this, &closedBody, &body]
    {
        details::ActivityCounter::Scope op(activeOps);
        // A closed queue may still be mid-drain in closeQueue() (it only waits out
        // operations that started before the flag flipped), so the closed path must
        // not touch any queue state.
        if (closedFlag.load(std::memory_order_seq_cst))
            return closedBody();
        drainInbox();
        return body();
    });
}

ErrCode ConnectionImpl::dequeue(IPacket** packet)
{
    OPENDAQ_PARAM_NOT_NULL(packet);

    return consumerOp(
        [&packet]()
        {
            *packet = nullptr;
            return OPENDAQ_NO_MORE_ITEMS;
        },
        [&packet, this]()
        {
            if (packets.empty())
            {
                queueEmptyFlag.store(true, std::memory_order_release);
                LOGP_T("No packet to dequeue.")
                *packet = nullptr;
                return OPENDAQ_NO_MORE_ITEMS;
            }

            *packet = packets.front().detach();
            packets.pop_front();
            onPacketDequeued(*packet);
            LOGP_T("Packet dequeued.")

            return OPENDAQ_SUCCESS;
        });
}

ErrCode INTERFACE_FUNC ConnectionImpl::dequeueAll(IList** packets)
{
    OPENDAQ_PARAM_NOT_NULL(packets);

    return consumerOp(
        [packets]()
        {
            *packets = List<IPacket>().detach();
            return OPENDAQ_NO_MORE_ITEMS;
        },
        [packets, this]()
        {
            auto packetsPtr = List<IPacket>();
            for (const auto& packet : this->packets)
            {
                packetsPtr.pushBack(packet);
            }
            samplesCnt = 0;
            eventPacketsCnt = 0;
            this->packets.clear();

            *packets = packetsPtr.detach();
            return OPENDAQ_NO_MORE_ITEMS;
        });
}

ErrCode ConnectionImpl::peek(IPacket** packet)
{
    OPENDAQ_PARAM_NOT_NULL(packet);

    return consumerOp(
        [&packet]()
        {
            *packet = nullptr;
            return OPENDAQ_NO_MORE_ITEMS;
        },
        [&packet, this]()
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

    return consumerOp(
        [&packetCount]()
        {
            *packetCount = 0;
            return OPENDAQ_SUCCESS;
        },
        [&packetCount, this]()
        {
            *packetCount = packets.size();
            LOG_T("Packet count = {}.", *packetCount)
            return OPENDAQ_SUCCESS;
        });
}

ErrCode ConnectionImpl::getAvailableSamples(SizeT* samples)
{
    OPENDAQ_PARAM_NOT_NULL(samples);

    return consumerOp(
        [samples]()
        {
            *samples = 0;
            return OPENDAQ_SUCCESS;
        },
        [samples, this]()
        {
            *samples = samplesCnt;

            LOG_T("Available samples = {}.", *samples)
            return OPENDAQ_SUCCESS;
        });
}

template <class ShortCircuit, class IsBoundary>
ErrCode ConnectionImpl::samplesUntil(SizeT* samples, const ShortCircuit& shortCircuit, const IsBoundary& isBoundary)
{
    OPENDAQ_PARAM_NOT_NULL(samples);

    return consumerOp(
        [samples]()
        {
            *samples = 0;
            return OPENDAQ_SUCCESS;
        },
        [&]()
        {
        if (shortCircuit())
        {
            *samples = samplesCnt;
            return OPENDAQ_SUCCESS;
        }
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
                    if (isBoundary(packet.template asPtrOrNull<IEventPacket>(true)))
                        return OPENDAQ_SUCCESS;
                    break;
                }
                case PacketType::None:
                    break;
            }
        }

        return OPENDAQ_SUCCESS;
    });
}

ErrCode ConnectionImpl::getSamplesUntilNextEventPacket(SizeT* samples)
{
    return samplesUntil(
        samples,
        [this] { return eventPacketsCnt == 0 && gapPacketsCnt == 0; },
        [](const EventPacketPtr&) { return true; });
}

ErrCode ConnectionImpl::getSamplesUntilNextDescriptor(SizeT* samples)
{
    return samplesUntil(
        samples,
        [this] { return eventPacketsCnt == 0; },
        [](const EventPacketPtr& eventPacket)
        { return eventPacket.assigned() && eventPacket.getEventId() == event_packet_id::DATA_DESCRIPTOR_CHANGED; });
}

ErrCode ConnectionImpl::getSamplesUntilNextGapPacket(SizeT* samples)
{
    return samplesUntil(
        samples,
        [this] { return gapPacketsCnt == 0; },
        [](const EventPacketPtr& eventPacket)
        { return eventPacket.assigned() && eventPacket.getEventId() == event_packet_id::IMPLICIT_DOMAIN_GAP_DETECTED; });
}

ErrCode ConnectionImpl::hasEventPacket(Bool* hasEventPacket)
{
    OPENDAQ_PARAM_NOT_NULL(hasEventPacket);

    return consumerOp(
        [hasEventPacket]()
        {
            *hasEventPacket = False;
            return OPENDAQ_SUCCESS;
        },
        [hasEventPacket, this]()
        {
            *hasEventPacket = eventPacketsCnt != 0 || gapPacketsCnt != 0;
            LOG_T("Has event packet = {}.", *hasEventPacket)
            return OPENDAQ_SUCCESS;
        });
}

ErrCode ConnectionImpl::hasGapPacket(Bool* hasGapPacket)
{
    OPENDAQ_PARAM_NOT_NULL(hasGapPacket);

    return consumerOp(
        [hasGapPacket]()
        {
            *hasGapPacket = False;
            return OPENDAQ_SUCCESS;
        },
        [hasGapPacket, this]()
        {
            *hasGapPacket = gapPacketsCnt != 0;
            LOG_T("Has gap packet = {}.", *hasGapPacket)
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

ErrCode ConnectionImpl::getSignal(ISignal** signal)
{
    OPENDAQ_PARAM_NOT_NULL(signal);

    const ErrCode errCode = daqTry([this, &signal]
    {
        auto sig = this->signalRef.getRef();
        *signal = sig.detach();
        LOG_T("Signal = {}.", sig.assigned() ? sig.getGlobalId().toStdString() : "null")
        return OPENDAQ_SUCCESS;
    });
    OPENDAQ_RETURN_IF_FAILED(errCode);
    return errCode;
}

ErrCode ConnectionImpl::getInputPort(IInputPort** inputPort)
{
    OPENDAQ_PARAM_NOT_NULL(inputPort);

    *inputPort = this->port.addRefAndReturn();
    LOG_T("InputPort = {}.", this->port.getGlobalId().toStdString());
    return OPENDAQ_SUCCESS;
}

ErrCode ConnectionImpl::dequeueUpTo(IPacket** packetPtr, SizeT* count)
{
    OPENDAQ_PARAM_NOT_NULL(packetPtr);
    OPENDAQ_PARAM_NOT_NULL(count);

    return consumerOp(
        [&count]()
        {
            *count = 0;
            return OPENDAQ_SUCCESS;
        },
        [&packetPtr, &count, this]()
        {
            auto ptr = packetPtr;
            *count = std::min(*count, packets.size());
            for (size_t i = 0; i < *count; ++i)
            {
                *ptr = packets.front().detach();
                ptr++;
                packets.pop_front();
            }

            countPackets();
            return OPENDAQ_SUCCESS;
        });
}

ErrCode ConnectionImpl::closeQueue()
{
    // config path: block until every in-flight producer/consumer operation has finished
    // (see activity_counter.h for the seq_cst argument), then drop everything so no
    // packet stays pinned in an orphaned queue.
    closedFlag.store(true, std::memory_order_seq_cst);
    activeOps.waitUntilIdle();

    if (IPacket* front = pendingFrontDescriptor.exchange(nullptr, std::memory_order_acq_rel))
        front->releaseRef();
    destroyChain(pendingDrainChain);
    pendingDrainChain = nullptr;
    destroyChain(inboxTop.exchange(nullptr, std::memory_order_acq_rel));
    packets.clear();
    samplesCnt = 0;
    eventPacketsCnt = 0;
    gapPacketsCnt = 0;

    LOGP_T("Connection queue closed.")
    return OPENDAQ_SUCCESS;
}

void ConnectionImpl::checkForGaps(const PacketPtr& packet)
{
    assert(gapCheckState != GapCheckState::disabled);

    switch (packet.getType())
    {
        case PacketType::Data:
        {
            if (gapCheckState == GapCheckState::uninitialized)
                DAQ_THROW_EXCEPTION(InvalidStateException, "No event packet received.");

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
    gapPacketsCnt += 1;
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
        bool domainDescriptorChanged;
        DataDescriptorPtr newDomainDescriptor;
        std::tie(std::ignore, domainDescriptorChanged, std::ignore, newDomainDescriptor) =
            parseDataDescriptorEventPacket(packet);

        if (!domainDescriptorChanged)
        {
            if (gapCheckState == GapCheckState::uninitialized)
            {
                LOGP_T("Gap check not available, no domain signal.")
                gapCheckState = GapCheckState::not_available;
            }

            // domain not changed, keep state as it is
            return;
        }

        if (!newDomainDescriptor.assigned())
        {
            LOGP_T("Gap check not available, domain descriptor is not assigned.")
            gapCheckState = GapCheckState::not_available;
            return;
        }

        const auto rule = newDomainDescriptor.getRule();
        if (rule.getType() != DataRuleType::Linear)
        {
            LOGP_T("Gap check not available, no linear rule.")
            gapCheckState = GapCheckState::not_available;
            return;
        }

        domainSampleType = newDomainDescriptor.getSampleType();
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
        DAQ_THROW_EXCEPTION(InvalidOperationException, "Gap packets should not be inserted into connection queue from outside.");
    }
}

void ConnectionImpl::countPackets()
{
    eventPacketsCnt = 0;
    samplesCnt = 0;
    for (const auto& packet : packets)
    {
        const auto packetType = packet.getType();
        if (packetType == PacketType::Data)
        {
            auto dataPacket = packet.asPtr<IDataPacket>(true);
            samplesCnt += dataPacket.getSampleCount();
        }
        else if (packetType == PacketType::Event)
        {
            eventPacketsCnt++;
        }
    }
}

void ConnectionImpl::onPacketEnqueuedCounters(const PacketPtr& packet)
{
    if (packet.getType() == PacketType::Data)
    {
        auto dataPacket = packet.asPtr<IDataPacket>(true);
        samplesCnt += dataPacket.getSampleCount();
    }
    else if (packet.getType() == PacketType::Event)
    {
        eventPacketsCnt++;
    }
}

void ConnectionImpl::onPacketDequeued(const PacketPtr& packet)
{
    if (packet.getType() == PacketType::Data)
    {
        auto dataPacket = packet.asPtrOrNull<IDataPacket>(true);
        if (dataPacket.assigned())
        {
            samplesCnt -= dataPacket.getSampleCount();
        }
    }
    else if (packet.getType() == PacketType::Event)
    {
        auto eventPacket = packet.asPtr<IEventPacket>(true);
        if (eventPacket.getEventId() == event_packet_id::DATA_DESCRIPTOR_CHANGED)
        {
            eventPacketsCnt--;
        }
        else if (eventPacket.getEventId() == event_packet_id::IMPLICIT_DOMAIN_GAP_DETECTED)
        {
            gapPacketsCnt--;
        }
    }
}

ErrCode ConnectionImpl::enqueueLastDescriptor()
{
    return daqTry([this]
    {
        DataDescriptorPtr valueDescriptor;
        DataDescriptorPtr domainDescriptor;
        {
            std::lock_guard lock(latchMutex);
            valueDescriptor = valueDataDescriptor;
            domainDescriptor = domainDataDescriptor;
        }

        if (valueDescriptor.assigned() || domainDescriptor.assigned())
        {
            details::ActivityCounter::Scope op(activeOps);
            if (closedFlag.load(std::memory_order_seq_cst))
                return OPENDAQ_IGNORED;

            auto dataDescriptorEventPacket = DataDescriptorChangedEventPacket(valueDescriptor, domainDescriptor);
            // merged to the queue front by the consumer's next drain; latest request wins
            if (IPacket* old = pendingFrontDescriptor.exchange(dataDescriptorEventPacket.detach(), std::memory_order_acq_rel))
                old->releaseRef();
        }
        return OPENDAQ_SUCCESS;
    });
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
            DAQ_THROW_EXCEPTION(InvalidParameterException, "Cannot convert number.");
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
