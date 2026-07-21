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
#include <opendaq/reclamation_gate.h>
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
 * Threading contract (openDAQ usage rule, not enforced by any ownership mechanism):
 * a connection has exactly one producer role and one consumer role. Every enqueue entry
 * point - the signal's sendPacket fan-out as well as configuration-side event packets
 * (setDescriptor, connect) - is serialized externally by the signal's owner, so no two
 * enqueues ever run concurrently. All dequeue/peek/counter/scan operations belong to the
 * port's single consumer. Only disconnect (closeQueue) and setListener
 * (enqueueLastDescriptor) are exempt from the rule and may race everything else.
 *
 * Structure:
 *  - The (externally serialized) enqueuer pushes pooled nodes onto `inboxTop`
 *    (Treiber-style CAS push, LIFO). A CAS push always publishes an intact chain, so a
 *    concurrent consumer grab (exchange) can never observe a half-linked node.
 *  - The consumer grabs the whole inbox with exchange(nullptr), reverses it (restoring
 *    FIFO), runs gap checking + counters, and appends to the consumer-owned `packets`
 *    deque, which serves every consumer-side query without any lock.
 *  - Consumed nodes are recycled via `freeTop` (consumer CAS-pushes). Only the enqueue
 *    path takes nodes out, and it takes the entire list with exchange(nullptr) into a
 *    private cache; there is no concurrent CAS-pop, hence no ABA hazard.
 *  - `closedFlag` + `accessGate` implement disconnect teardown: closeQueue() sets the
 *    flag (seq_cst), then waits until the gate is quiescent. Every producer/consumer
 *    operation holds the gate and re-checks the flag inside it, so once the wait
 *    completes no operation can be in flight and the queue state is owned exclusively
 *    (see the correctness argument in reclamation_gate.h; the "state" here is the
 *    closed flag, the "pin" is the queue access itself).
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
    ErrCode INTERFACE_FUNC closeQueue() override;

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
    PacketNode* nodeCache{nullptr};                  // enqueuer-private (enqueues are externally serialized)
    details::ReclamationGate accessGate;
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

    PacketNode* acquireNode();
    static PacketNode* reverseChain(PacketNode* chain);
    void recycleNode(PacketNode* node);
    static void destroyChain(PacketNode* chain);
    // pushes an intact pre-linked chain (first is the newest element); returns false when closed
    bool pushChain(PacketNode* first, PacketNode* last, bool& queueWasEmpty);
    void drainInbox();  // consumer thread, inside the gate

    template <class P, class F>
    ErrCode enqueueInternal(P&& packet, const F& f);

    template <class P, class F>
    ErrCode enqueueMultipleInternal(P&& packets, const F& f);

    // Scaffold for every consumer-side operation: gate + closed check + inbox drain + body.
    // closedBody runs instead of body when the queue is closed and MUST NOT touch any queue
    // state (deque, counters): closeQueue() only excludes operations that entered the gate
    // before it flipped the flag, so a later operation can run concurrently with its drain.
    template <class ClosedF, class F>
    ErrCode consumerOp(const ClosedF& closedBody, const F& body);

    template <class ShortCircuit, class IsBoundary>
    ErrCode samplesUntil(SizeT* samples, const ShortCircuit& shortCircuit, const IsBoundary& isBoundary);

protected:
    SizeT samplesCnt{};
    SizeT eventPacketsCnt{};
    SizeT gapPacketsCnt{};
    std::deque<PacketPtr> packets;                   // consumer-owned
};

END_NAMESPACE_OPENDAQ
