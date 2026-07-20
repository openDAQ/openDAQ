/*
 * Copyright 2022-2026 openDAQ d.o.o.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#pragma once
#include <opendaq/connection.h>
#include <opendaq/connection_internal.h>
#include <opendaq/input_port_config_ptr.h>
#include <opendaq/context_ptr.h>
#include <coretypes/intfs.h>
#include <coretypes/weakrefobj.h>
#include <opendaq/event_packet_ptr.h>
#include <opendaq/data_packet_ptr.h>

#include <atomic>
#include <deque>
#include <mutex>

BEGIN_NAMESPACE_OPENDAQ

/*
 * Lock-free connection queue.
 *
 * Threading contract (see IConnection docs): all queue operations are restricted to the
 * signal's single producer thread (the enqueue methods) and the port's single consumer
 * thread (dequeue, peek, counters, scans). Configuration paths (descriptor events,
 * setListener, disconnect) may run on other threads; they use the generic enqueue entry
 * points, the descriptor latch and closeQueue(), all of which tolerate blocking.
 *
 * Structure:
 *  - Producers push heap/pooled nodes onto `inboxTop` (Treiber-style CAS push, LIFO).
 *    A CAS push always publishes an intact chain, so a concurrent consumer grab
 *    (exchange) can never observe a half-linked node.
 *  - The consumer grabs the whole inbox with exchange(nullptr), reverses it (restoring
 *    FIFO), runs gap checking + counters, and appends to the consumer-owned `packets`
 *    deque, which serves every consumer-side query without any lock.
 *  - Consumed nodes are recycled via `freeTop` (consumer CAS-pushes). Only the producer
 *    fast path takes nodes out, and it takes the entire list with exchange(nullptr) into
 *    a producer-private cache; there is no concurrent CAS-pop, hence no ABA hazard.
 *  - `closedFlag` + `accessGate` implement disconnect teardown: closeQueue() sets the
 *    flag (seq_cst), then spins until the gate is 0. Every producer/consumer operation
 *    increments the gate (seq_cst) and re-checks the flag inside it, so once the spin
 *    completes no operation can be in flight and the queue state is owned exclusively.
 *    (Dekker-style argument: if an operation's flag check read `false`, the check
 *    precedes the flag store in the seq_cst total order, hence its gate increment does
 *    too, and closeQueue()'s spin observes the gate until that operation finishes.)
 */
class ConnectionImpl : public ImplementationOfWeak<IConnection, IConnectionInternal>
{
public:
    using Super = ImplementationOfWeak<IConnection, IConnectionInternal>;

    explicit ConnectionImpl(
        const InputPortPtr& port,
        const SignalPtr& signal,
        ContextPtr context
    );
    ~ConnectionImpl() override;

    ErrCode INTERFACE_FUNC enqueue(IPacket* packet) override;
    ErrCode INTERFACE_FUNC enqueueMultiple(IList* packets) override;
    ErrCode INTERFACE_FUNC enqueueAndStealRef(IPacket* packet) override;
    ErrCode INTERFACE_FUNC enqueueMultipleAndStealRef(IList* packets) override;

    ErrCode INTERFACE_FUNC enqueueOnThisThread(IPacket* packet) override;
    ErrCode INTERFACE_FUNC enqueueWithScheduler(IPacket* packet) override;
    ErrCode INTERFACE_FUNC dequeue(IPacket** packet) override;
    ErrCode INTERFACE_FUNC dequeueAll(IList** packets) override;
    ErrCode INTERFACE_FUNC peek(IPacket** packet) override;
    ErrCode INTERFACE_FUNC getPacketCount(SizeT* packetCount) override;
    ErrCode INTERFACE_FUNC getSignal(ISignal** signal) override;
    ErrCode INTERFACE_FUNC getInputPort(IInputPort** inputPort) override;

    ErrCode INTERFACE_FUNC dequeueUpTo(IPacket** packetPtr, SizeT* count) override;

    ErrCode INTERFACE_FUNC getAvailableSamples(SizeT* samples) override;
    ErrCode INTERFACE_FUNC getSamplesUntilNextDescriptor(SizeT* samples) override;
    ErrCode INTERFACE_FUNC getSamplesUntilNextEventPacket(SizeT* samples) override;
    ErrCode INTERFACE_FUNC getSamplesUntilNextGapPacket(SizeT* samples) override;
    ErrCode INTERFACE_FUNC hasEventPacket(Bool* hasEventPacket) override;
    ErrCode INTERFACE_FUNC hasGapPacket(Bool* hasGapPacket) override;

    ErrCode INTERFACE_FUNC isRemote(Bool* remote) override;

    // IConnectionInternal
    ErrCode INTERFACE_FUNC enqueueLastDescriptor() override;
    ErrCode INTERFACE_FUNC enqueueProducer(IPacket* packet) override;
    ErrCode INTERFACE_FUNC enqueueProducerAndStealRef(IPacket* packet) override;
    ErrCode INTERFACE_FUNC enqueueMultipleProducer(IList* packets) override;
    ErrCode INTERFACE_FUNC enqueueMultipleProducerAndStealRef(IList* packets) override;
    ErrCode INTERFACE_FUNC closeQueue() override;

    [[nodiscard]] const std::deque<PacketPtr>& getPackets() const noexcept;

private:
    union DomainValue
    {
        int64_t valueInt64_t;
        double valueDouble;
    };

    enum class GapCheckState { disabled, uninitialized, not_available, initialized, running };

    struct PacketNode
    {
        std::atomic<PacketNode*> next{nullptr};
        IPacket* packet{nullptr};
        bool gapCheck{true};
    };

    // RAII around accessGate; every producer/consumer operation holds it while touching
    // queue state so closeQueue() can wait for quiescence.
    struct GateGuard
    {
        explicit GateGuard(std::atomic<int>& gate)
            : gate(gate)
        {
            gate.fetch_add(1, std::memory_order_seq_cst);
        }

        ~GateGuard()
        {
            gate.fetch_sub(1, std::memory_order_release);
        }

        std::atomic<int>& gate;
    };

    InputPortConfigPtr port;
    WeakRefPtr<ISignal> signalRef;
    ContextPtr context;
    GapCheckState gapCheckState;
    DomainValue nextExpectedPacketOffset;
    DomainValue delta;
    SampleType domainSampleType;
    LoggerComponentPtr loggerComponent;

    // Descriptor latch: written when DATA_DESCRIPTOR_CHANGED events are enqueued, read by
    // enqueueLastDescriptor (setListener). Config paths only -> a plain mutex is fine here.
    std::mutex latchMutex;
    DataDescriptorPtr valueDataDescriptor;
    DataDescriptorPtr domainDataDescriptor;

    // lock-free producer -> consumer inbox
    std::atomic<PacketNode*> inboxTop{nullptr};
    std::atomic<PacketNode*> freeTop{nullptr};
    PacketNode* producerNodeCache{nullptr};          // producer-thread-private
    std::atomic<int> accessGate{0};
    std::atomic<bool> closedFlag{false};
    std::atomic<bool> queueEmptyFlag{true};
    std::atomic<IPacket*> pendingFrontDescriptor{nullptr};
    PacketNode* pendingDrainChain{nullptr};          // consumer-owned; drain resumed after a gap-check throw

    void onPacketEnqueuedCounters(const PacketPtr& packet);
    void onPacketDequeued(const PacketPtr& packet);
    void latchDescriptors(const EventPacketPtr& packet);

    void checkForGaps(const PacketPtr& packet);
    void enqueueGapPacket(const DomainValue& diff);
    void beginGapCheck(const DataPacketPtr& domainPacket);
    bool doGapCheck(const DataPacketPtr& domainPacket, DomainValue& diff);
    void initGapCheck(const EventPacketPtr& packet);
    void countPackets();

    DomainValue numberToDomainValue(const NumberPtr& number);

    PacketNode* acquireNodeProducer();
    static PacketNode* reverseChain(PacketNode* chain);
    void recycleNode(PacketNode* node);
    static void destroyChain(PacketNode* chain);
    // pushes an intact pre-linked chain (first is the newest element); returns false when closed
    bool pushChain(PacketNode* first, PacketNode* last, bool& queueWasEmpty);
    void drainInbox();  // consumer thread, inside the gate

    template <class P, class F>
    ErrCode enqueueInternal(P&& packet, bool useProducerCache, const F& f);

    template <class P, class F>
    ErrCode enqueueMultipleInternal(P&& packets, bool useProducerCache, const F& f);

protected:
    SizeT samplesCnt{};
    SizeT eventPacketsCnt{};
    SizeT gapPacketsCnt{};
    std::deque<PacketPtr> packets;                   // consumer-owned
};

END_NAMESPACE_OPENDAQ
