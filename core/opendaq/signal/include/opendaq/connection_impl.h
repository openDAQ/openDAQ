/*
 * Copyright 2022-2024 openDAQ d.o.o.
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
#include <opendaq/input_port_config_ptr.h>
#include <opendaq/context_ptr.h>
#include <coretypes/intfs.h>
#include <coretypes/weakrefobj.h>
#include <opendaq/event_packet_ptr.h>
#include <opendaq/data_packet_ptr.h>

#ifdef OPENDAQ_THREAD_SAFE
    #include <mutex>
#endif

#include <queue>

BEGIN_NAMESPACE_OPENDAQ
class ConnectionImpl : public ImplementationOfWeak<IConnection>
{
public:
    using Super = ImplementationOfWeak<IConnection>;

    explicit ConnectionImpl(
        const InputPortPtr& port,
        const SignalPtr& signal,
        ContextPtr context
    );
    ErrCode INTERFACE_FUNC enqueue(IPacket* packet) override;
    ErrCode INTERFACE_FUNC enqueueMultiple(IList* packets) override;
    ErrCode INTERFACE_FUNC enqueueAndStealRef(IPacket* packet) override;
    ErrCode INTERFACE_FUNC enqueueMultipleAndStealRef(IList* packets) override;

    ErrCode INTERFACE_FUNC enqueueOnThisThread(IPacket* packet) override;
    ErrCode INTERFACE_FUNC dequeue(IPacket** packet) override;
    ErrCode INTERFACE_FUNC dequeueAll(IList** packets) override;
    ErrCode INTERFACE_FUNC peek(IPacket** packet) override;
    ErrCode INTERFACE_FUNC getPacketCount(SizeT* packetCount) override;
    ErrCode INTERFACE_FUNC getSignal(ISignal** signal) override;
    ErrCode INTERFACE_FUNC getInputPort(IInputPort** inputPort) override;

    ErrCode INTERFACE_FUNC getAvailableSamples(SizeT* samples) override;
    ErrCode INTERFACE_FUNC getSamplesUntilNextDescriptor(SizeT* samples) override;
    ErrCode INTERFACE_FUNC getSamplesUntilNextEventPacket(SizeT* samples) override;
    ErrCode INTERFACE_FUNC getSamplesUntilNextGapPacket(SizeT* samples) override;
    ErrCode INTERFACE_FUNC hasEventPacket(Bool* hasEventPacket) override;
    ErrCode INTERFACE_FUNC hasGapPacket(Bool* hasGapPacket) override;

    ErrCode INTERFACE_FUNC isRemote(Bool* remote) override;

    // IBaseObject
    ErrCode INTERFACE_FUNC queryInterface(const IntfID& id, void** intf) override;
    ErrCode INTERFACE_FUNC borrowInterface(const IntfID& id, void** intf) const override;

    [[nodiscard]] const std::deque<PacketPtr>& getPackets() const noexcept;

#ifdef OPENDAQ_THREAD_SAFE
    template <typename Func>
    auto withLock(Func&& func) const
    {
        std::lock_guard guard(mutex);
        return func();
    }
#else
    template <typename Func>
    auto withLock(Func&& func) const
    {
        return func();
    }
#endif

private:
    union DomainValue
    {
        int64_t valueInt64_t;
        double valueDouble;
    };

    enum class GapCheckState { disabled, uninitialized, not_available, initialized, running };

    InputPortConfigPtr port;
    WeakRefPtr<ISignal> signalRef;
    ContextPtr context;
    bool queueEmpty;
    GapCheckState gapCheckState;
    DomainValue nextExpectedPacketOffset;
    DomainValue delta;
    SampleType domainSampleType;
    LoggerComponentPtr loggerComponent;

#ifdef OPENDAQ_THREAD_SAFE
    mutable std::mutex mutex;
#endif

    void onPacketEnqueued(const PacketPtr& packet);
    void onPacketDequeued(const PacketPtr& packet);

    void checkForGaps(const PacketPtr& packet);
    void enqueueGapPacket(const DomainValue& diff);
    void beginGapCheck(const DataPacketPtr& domainPacket);
    bool doGapCheck(const DataPacketPtr& domainPacket, DomainValue& diff);
    void initGapCheck(const EventPacketPtr& packet);

    DomainValue numberToDomainValue(const NumberPtr& number);

    template <class P, class F>
    ErrCode enqueueInternal(P&& packet, const F& f);

#if _MSC_VER < 1920
    ErrCode enqueueMultipleInternal(const ListPtr<IPacket>& packets);
    ErrCode enqueueMultipleInternal(ListPtr<IPacket>&& packets);
#else
    template <class P>
    ErrCode enqueueMultipleInternal(P&& packets);
#endif

protected:
    SizeT samplesCnt{};
    SizeT eventPacketsCnt{};
    SizeT gapPacketsCnt{};
    std::deque<PacketPtr> packets;
};

END_NAMESPACE_OPENDAQ
