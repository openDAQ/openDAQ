#include <opendaq/input_port_factory.h>
#include <gtest/gtest.h>
#include <opendaq/gmock/context.h>
#include <opendaq/gmock/input_port.h>
#include <opendaq/gmock/input_port_notifications.h>
#include <opendaq/gmock/signal.h>
#include <opendaq/context_factory.h>
#include <opendaq/signal_factory.h>
#include <opendaq/signal_private_ptr.h>
#include <opendaq/event_packet.h>
#include <opendaq/packet_factory.h>
#include <opendaq/data_rule_factory.h>
#include <opendaq/range_factory.h>
#include <opendaq/connection_factory.h>
#include <opendaq/connection_internal.h>
#include <coretypes/listobject_factory.h>
#include <coretypes/number_ptr.h>
#include <coreobjects/unit_factory.h>
#include <atomic>
#include <chrono>
#include <condition_variable>
#include <mutex>
#include <thread>
#include <vector>

#include "opendaq/reader_factory.h"

using namespace daq;
using namespace testing;

struct CustomInputPortNotifications : ImplementationOfWeak<IInputPortNotifications>
{
    typedef MockPtr<IInputPortNotifications, InputPortNotificationsPtr, CustomInputPortNotifications> Strict;

    MOCK_METHOD(daq::ErrCode, acceptsSignal, (IInputPort* port, daq::ISignal* signal, daq::Bool* accept), (override MOCK_CALL));
    MOCK_METHOD(daq::ErrCode, connected, (IInputPort* port), (override MOCK_CALL));
    MOCK_METHOD(daq::ErrCode, disconnected, (IInputPort* port), (override MOCK_CALL));
    MOCK_METHOD(daq::ErrCode, packetReceived, (IInputPort* port), (override MOCK_CALL));

    void setInputPort(const InputPortPtr& inputPort)
    {
        this->inputPort = inputPort;
    }

    ~CustomInputPortNotifications() override
    {
        inputPort.remove();   
    }

    InputPortPtr inputPort;
};

using DataPathTest = Test;

TEST_F(DataPathTest, DestroyListenerInNotification)
{
    const auto ctx = NullContext();

    const auto sigDesc = DataDescriptorBuilder().setSampleType(SampleType::Int32).build();

    const auto signal = Signal(ctx, nullptr, "sig");
    signal.setDescriptor(sigDesc);

    const auto inputPort = InputPort(ctx, nullptr, "ip");

    CustomInputPortNotifications::Strict notifications;
    inputPort.setNotificationMethod(PacketReadyNotification::SameThread);
    inputPort.setListener(notifications);
    notifications.mock().setInputPort(inputPort);

    EXPECT_CALL(notifications.mock(), connected).WillOnce(Return(OPENDAQ_SUCCESS));

    bool eventPacketReceived;
    EXPECT_CALL(notifications.mock(), packetReceived)
        .WillOnce(
            [&eventPacketReceived](IInputPort* port)
            {
                const auto portPtr = InputPortPtr::Borrow(port);

                const auto conn = portPtr.getConnection();
                const auto packet = conn.dequeue();

                eventPacketReceived = packet.supportsInterface<IEventPacket>();

                return OPENDAQ_SUCCESS;
            });

    inputPort.connect(signal);
    ASSERT_TRUE(eventPacketReceived);

    std::mutex mtx;
    std::condition_variable cv0;
    bool doReleaseFromThread = false;

    std::condition_variable cv1;
    bool releasedFromThread = false;

    bool dataPacketReceived = false;
    EXPECT_CALL(notifications.mock(), packetReceived)
        .WillOnce(
            [&mtx, &cv0, &cv1, &doReleaseFromThread, &releasedFromThread, &dataPacketReceived](IInputPort* port)
            {
                const auto portPtr = InputPortPtr::Borrow(port);

                {
                    std::unique_lock lock(mtx);
                    doReleaseFromThread = true;
                    cv0.notify_one();
                }

                const auto conn = portPtr.getConnection();
                const auto packet = conn.dequeue();

                dataPacketReceived = packet.supportsInterface<IDataPacket>();

                {
                    std::unique_lock lock(mtx);
                    while (!releasedFromThread)
                        cv1.wait(lock);
                }

                return OPENDAQ_SUCCESS;
            });

    std::thread thr(
        [&cv0, &cv1, &mtx, &doReleaseFromThread, &releasedFromThread, inputPortPtr = std::move(inputPort), notificationsPtr = std::move(notifications.ptr)] () mutable
        {
            {
                std::unique_lock lock(mtx);
                while (!doReleaseFromThread)
                    cv0.wait(lock);
            }

            notificationsPtr.release();

            {
                std::unique_lock lock(mtx);
                releasedFromThread = true;
                cv1.notify_one();
            }
        });

    const auto packet = DataPacket(sigDesc, 1, nullptr);
    signal.sendPacket(packet);

    thr.join();

    ASSERT_TRUE(dataPacketReceived);
}

// Lock-free data path stress tests. These encode the concurrency invariants of the
// snapshot/tombstone design and are meant to run under TSAN on platforms that have it.

TEST_F(DataPathTest, StressSendVsConnectDisconnect)
{
    const auto ctx = NullContext();

    const auto domainDesc = DataDescriptorBuilder()
                                .setSampleType(SampleType::Int64)
                                .setRule(LinearDataRule(10, 0))
                                .setTickResolution(Ratio(1, 1000))
                                .setUnit(Unit("s", -1, "second", "time"))
                                .setOrigin("1970-01-01T00:00:00Z")
                                .build();
    const auto sigDesc = DataDescriptorBuilder().setSampleType(SampleType::Float64).build();

    const auto domainSignal = Signal(ctx, nullptr, "domain");
    domainSignal.setDescriptor(domainDesc);
    const auto signal = Signal(ctx, nullptr, "sig");
    signal.setDescriptor(sigDesc);
    signal.setDomainSignal(domainSignal);

    std::atomic<bool> stop{false};
    std::atomic<int64_t> sent{0};

    // the signal's single producer
    std::thread producer(
        [&]
        {
            int64_t offset = 0;
            while (!stop.load(std::memory_order_relaxed))
            {
                const auto domainPacket = DataPacket(domainDesc, 16, offset);
                const auto packet = DataPacketWithDomain(domainPacket, sigDesc, 16);
                signal.sendPacket(packet);
                offset += 160;
                sent.fetch_add(1, std::memory_order_relaxed);
            }
        });

    // wait for the producer to be live so the roles genuinely overlap (thread startup can
    // exceed the whole test duration otherwise)
    while (sent.load(std::memory_order_relaxed) == 0)
        std::this_thread::yield();

    // this thread plays both the config role (connect/disconnect) and, per connection,
    // the consumer role. Gap checking makes the consumer validate the
    // descriptor-event-before-data invariant on every reconnect: a data packet arriving
    // before the initial event packet would throw InvalidStateException from dequeue.
    for (int round = 0; round < 200; ++round)
    {
        auto port = InputPort(ctx, nullptr, "ip", true /*gap checking*/);
        port.connect(signal);
        const auto connection = port.getConnection();

        bool first = true;
        for (int reads = 0; reads < 50; ++reads)
        {
            PacketPtr packet;
            ASSERT_NO_THROW(packet = connection.dequeue()) << "data-before-event or torn queue on round " << round;
            if (!packet.assigned())
                continue;
            if (first)
            {
                ASSERT_TRUE(packet.supportsInterface<IEventPacket>()) << "first packet on a new connection must be the descriptor event";
                first = false;
            }
        }

        port.disconnect();
    }

    stop.store(true);
    producer.join();
    ASSERT_GT(sent.load(), 0);
}

// closeQueue vs an actively draining consumer and a live producer: exercises the
// access-gate teardown protocol (closed flag + gate spin) from all three roles at once
TEST_F(DataPathTest, StressDisconnectUnderFire)
{
    const auto ctx = NullContext();

    const auto sigDesc = DataDescriptorBuilder().setSampleType(SampleType::Float64).build();
    const auto signal = Signal(ctx, nullptr, "sig");
    signal.setDescriptor(sigDesc);

    std::atomic<bool> stopProducer{false};
    std::thread producer(
        [&]
        {
            const auto packet = DataPacket(sigDesc, 16);
            while (!stopProducer.load(std::memory_order_relaxed))
                signal.sendPacket(packet);
        });

    for (int round = 0; round < 100; ++round)
    {
        auto port = InputPort(ctx, nullptr, "ip");
        port.connect(signal);
        const auto connection = port.getConnection();

        std::atomic<bool> stopConsumer{false};
        std::thread consumer(
            [&]
            {
                // keeps draining right through the disconnect: dequeue/getPacketCount race
                // the closeQueue drain and must never crash, hang, or corrupt the queue
                while (!stopConsumer.load(std::memory_order_relaxed))
                {
                    connection.dequeue();
                    SizeT count;
                    connection->getPacketCount(&count);
                }
            });

        std::this_thread::yield();
        port.disconnect();  // tombstone + drain while both other roles are active

        stopConsumer.store(true);
        consumer.join();

        // closed queue stays empty and rejects everything
        SizeT count = 12345;
        connection->getPacketCount(&count);
        ASSERT_EQ(count, 0u);
        ASSERT_FALSE(connection.dequeue().assigned());
    }

    stopProducer.store(true);
    producer.join();
}

// descriptor events interleaved with data packets from a second thread; per the openDAQ
// usage rule the signal owner serializes setDescriptor against sendPacket (emulated here
// with ownerMutex), while the consumer drains the mixed stream fully concurrently
TEST_F(DataPathTest, StressDescriptorEventsVsProducer)
{
    const auto ctx = NullContext();

    const auto descA = DataDescriptorBuilder().setSampleType(SampleType::Float64).build();
    const auto descB = DataDescriptorBuilder().setSampleType(SampleType::Float64).setValueRange(daq::Range(0, 10)).build();

    const auto signal = Signal(ctx, nullptr, "sig");
    signal.setDescriptor(descA);

    auto port = InputPort(ctx, nullptr, "ip");
    port.connect(signal);
    const auto connection = port.getConnection();

    std::mutex ownerMutex;  // the owner's external serialization of production vs configuration

    std::atomic<bool> stop{false};
    std::thread producer(
        [&]
        {
            const auto packetA = DataPacket(descA, 16);
            while (!stop.load(std::memory_order_relaxed))
            {
                std::lock_guard lock(ownerMutex);
                signal.sendPacket(packetA);
            }
        });

    std::thread config(
        [&]
        {
            // setDescriptor fans events into the queue, serialized with sendPacket by the owner
            for (int i = 0; i < 200; ++i)
            {
                {
                    std::lock_guard lock(ownerMutex);
                    signal.setDescriptor(i % 2 ? descB : descA);
                }
                std::this_thread::yield();
            }
        });

    // consumer: drain the mixed stream; counters must stay consistent with the contents
    size_t eventPackets = 0;
    const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(30);
    while (eventPackets < 100)
    {
        ASSERT_LT(std::chrono::steady_clock::now(), deadline) << "descriptor events lost in the queue";
        const auto packet = connection.dequeue();
        if (packet.assigned() && packet.supportsInterface<IEventPacket>())
            ++eventPackets;
    }

    config.join();
    stop.store(true);
    producer.join();

    // full drain leaves consistent counters
    connection.dequeueAll();
    SizeT count = 999, samples = 999;
    connection->getPacketCount(&count);
    connection->getAvailableSamples(&samples);
    ASSERT_EQ(count, 0u);
    ASSERT_EQ(samples, 0u);
}

// snapshot publish churn with queues that are never consumed: producers pinning old
// snapshots must keep disconnected connections alive exactly until close, and closeQueue
// must release every packet (the suite's leak listener verifies the lifetime half)
TEST_F(DataPathTest, StressSnapshotChurnUnconsumedQueues)
{
    const auto ctx = NullContext();

    const auto sigDesc = DataDescriptorBuilder().setSampleType(SampleType::Float64).build();
    const auto signal = Signal(ctx, nullptr, "sig");
    signal.setDescriptor(sigDesc);

    std::atomic<bool> stop{false};
    std::thread producer(
        [&]
        {
            const auto packet = DataPacket(sigDesc, 16);
            while (!stop.load(std::memory_order_relaxed))
                signal.sendPacket(packet);
        });

    for (int round = 0; round < 100; ++round)
    {
        std::vector<InputPortConfigPtr> ports;
        for (int i = 0; i < 4; ++i)
        {
            auto port = InputPort(ctx, nullptr, "ip" + std::to_string(i));
            port.connect(signal);
            ports.push_back(port);
        }
        // queues fill up unconsumed; disconnect closes + drains them under producer fire
        for (auto& port : ports)
            port.disconnect();
    }

    stop.store(true);
    producer.join();
}

TEST_F(DataPathTest, StressSendVsGetLastValueAndActive)
{
    const auto ctx = NullContext();

    const auto sigDesc = DataDescriptorBuilder().setSampleType(SampleType::Int64).build();
    const auto signal = Signal(ctx, nullptr, "sig");
    signal.setDescriptor(sigDesc);

    std::atomic<bool> stop{false};

    std::thread producer(
        [&]
        {
            int64_t counter = 0;
            while (!stop.load(std::memory_order_relaxed))
            {
                const auto packet = DataPacket(sigDesc, 4);
                auto* data = static_cast<int64_t*>(packet.getRawData());
                for (int i = 0; i < 4; ++i)
                    data[i] = counter;
                signal.sendPacket(packet);
                ++counter;
            }
        });

    // config-path hammering: last-value pin/retire, keep-last toggles, active toggles
    for (int round = 0; round < 2000; ++round)
    {
        BaseObjectPtr value;
        ASSERT_NO_THROW(value = signal.getLastValue());
        if (value.assigned())
            ASSERT_TRUE(value.asPtrOrNull<IInteger>().assigned());

        if (round % 64 == 0)
        {
            signal.setActive(false);
            signal.setActive(true);
        }
        if (round % 128 == 0)
        {
            signal.asPtr<ISignalPrivate>(true).enableKeepLastValue(false);
            signal.asPtr<ISignalPrivate>(true).enableKeepLastValue(true);
        }
    }

    stop.store(true);
    producer.join();
}

// Contract: the signal must not retain any reference to a sent packet after sendPacket
// returns, even with keep-last-value enabled. Producers backed by circular buffers reclaim
// a packet's memory as soon as the send completes; the last value is copied out at send time.
TEST_F(DataPathTest, SendPacketDoesNotRetainPacket)
{
    const auto ctx = NullContext();

    const auto domainDesc = DataDescriptorBuilder()
                                .setSampleType(SampleType::Int64)
                                .setUnit(Unit("s", -1, "seconds", "time"))
                                .setTickResolution(Ratio(1, 1000))
                                .setOrigin("1970-01-01T00:00:00Z")
                                .setRule(ExplicitDataRule())
                                .build();
    const auto sigDesc = DataDescriptorBuilder().setSampleType(SampleType::Int64).build();

    const auto signal = Signal(ctx, nullptr, "sig");
    signal.setDescriptor(sigDesc);

    auto domainPacket = DataPacket(domainDesc, 2);
    static_cast<int64_t*>(domainPacket.getRawData())[0] = 1000;
    static_cast<int64_t*>(domainPacket.getRawData())[1] = 2000;
    auto packet = DataPacketWithDomain(domainPacket, sigDesc, 2);
    static_cast<int64_t*>(packet.getRawData())[0] = 11;
    static_cast<int64_t*>(packet.getRawData())[1] = 42;

    signal.sendPacket(packet);

    // refcount probe: releaseRef reports the count produced by our own references alone.
    // packet: this test's ptr; domainPacket: this test's ptr + the value packet's reference.
    packet->addRef();
    ASSERT_EQ(packet->releaseRef(), 1);
    domainPacket->addRef();
    ASSERT_EQ(domainPacket->releaseRef(), 2);

    // the last value must survive the packets being destroyed (it was copied at send time)
    domainPacket = nullptr;
    packet = nullptr;
    ASSERT_EQ(signal.getLastValue(), 42);

    BaseObjectPtr value;
    BaseObjectPtr ts;
    ASSERT_NO_THROW(ts = signal.getLastValueWithTimestamp(value));
    ASSERT_EQ(value, 42);
    // tick 2000 at resolution 1/1000 s from the 1970 epoch = 2'000'000 us
    ASSERT_EQ(ts, 2000000);
}

// setLastValue is a producer-role call (owner-serialized with sendPacket, lock-free): it
// publishes through the same staged slot as sent packets. A config thread hammering
// getLastValue concurrently must always observe a well-formed value.
TEST_F(DataPathTest, StressSetLastValueVsGetLastValue)
{
    const auto ctx = NullContext();

    const auto sigDesc = DataDescriptorBuilder().setSampleType(SampleType::Int64).build();
    const auto signal = Signal(ctx, nullptr, "sig");
    signal.setDescriptor(sigDesc);
    signal.asPtr<ISignalPrivate>(true).enableKeepLastValue(false);

    std::atomic<bool> stop{false};
    int64_t counter = 0;

    std::thread producer(
        [&]
        {
            while (!stop.load(std::memory_order_relaxed))
            {
                const auto packet = DataPacket(sigDesc, 4);
                auto* data = static_cast<int64_t*>(packet.getRawData());
                for (int i = 0; i < 4; ++i)
                    data[i] = counter;
                signal.sendPacket(packet);
                signal.setLastValue(++counter);
            }
        });

    for (int round = 0; round < 4000; ++round)
    {
        BaseObjectPtr value;
        ASSERT_NO_THROW(value = signal.getLastValue());
        if (value.assigned())
            ASSERT_TRUE(value.asPtrOrNull<IInteger>().assigned());
    }

    stop.store(true);
    producer.join();

    // producer stopped: the last explicit value must be the visible one (keep-last is off,
    // so sendPacket never publishes to the slot)
    if (counter > 0)
        ASSERT_EQ(static_cast<Int>(signal.getLastValue()), counter);
}

// Same contract for calculated (implicit-rule) packets: getRawLastValue computes the last
// sample from the rule into the staged buffer at send time, so the value survives the
// packet without the packet ever being retained.
TEST_F(DataPathTest, LastValueFromCalculatedPacket)
{
    const auto ctx = NullContext();

    const auto sigDesc = DataDescriptorBuilder()
                             .setSampleType(SampleType::Int64)
                             .setRule(LinearDataRule(2, 10))
                             .build();

    const auto signal = Signal(ctx, nullptr, "sig");
    signal.setDescriptor(sigDesc);

    auto packet = DataPacket(sigDesc, 3, 5);
    const auto expected = packet.getLastValue();

    signal.sendPacket(packet);

    packet->addRef();
    ASSERT_EQ(packet->releaseRef(), 1);

    packet = nullptr;
    ASSERT_EQ(signal.getLastValue(), expected);
}

// ============================================================================
// Tier 1 - direct coverage of the consumer-API surface and send variants
// ============================================================================

// Every consumer method shares the consumerOp scaffold but has its own hand-written
// closed-path result. This pins each of those results deterministically: after closeQueue,
// every consumer entry point must return a synthesized empty answer without touching state.
TEST_F(DataPathTest, ClosedQueueConsumerApiMatrix)
{
    const auto ctx = NullContext();
    const auto sigDesc = DataDescriptorBuilder().setSampleType(SampleType::Int64).build();
    const auto signal = Signal(ctx, nullptr, "sig");
    signal.setDescriptor(sigDesc);

    auto port = InputPort(ctx, nullptr, "ip");
    port.connect(signal);
    const auto connection = port.getConnection();

    // put a mix of packets in the queue so "closed returns empty" is a real transition
    for (int i = 0; i < 3; ++i)
        signal.sendPacket(DataPacket(sigDesc, 8));

    auto internal = connection.asPtr<IConnectionInternal>(true);
    internal->closeQueue();

    // scalar/bool queries
    ASSERT_EQ(connection.getPacketCount(), 0u);
    ASSERT_EQ(connection.getAvailableSamples(), 0u);
    ASSERT_EQ(connection.getSamplesUntilNextDescriptor(), 0u);
    ASSERT_EQ(connection.getSamplesUntilNextEventPacket(), 0u);
    ASSERT_EQ(connection.getSamplesUntilNextGapPacket(), 0u);
    ASSERT_FALSE(connection.hasEventPacket());
    ASSERT_FALSE(connection.hasGapPacket());
    // dequeue family
    ASSERT_FALSE(connection.dequeue().assigned());
    ASSERT_FALSE(connection.peek().assigned());
    ASSERT_EQ(connection.dequeueAll().getCount(), 0u);
    IPacket* batch[8] = {};
    SizeT count = 8;
    internal->dequeueUpTo(batch, &count);
    ASSERT_EQ(count, 0u);
}

// Generalizes StressDisconnectUnderFire: the consumer rotates through *every* consumer
// operation while closeQueue tears the queue down under a live producer. Exercises each
// op's gate-enter / closed-check / drain path against the teardown, not just dequeue.
TEST_F(DataPathTest, StressAllConsumerOpsUnderFire)
{
    const auto ctx = NullContext();
    const auto sigDesc = DataDescriptorBuilder().setSampleType(SampleType::Float64).build();
    const auto signal = Signal(ctx, nullptr, "sig");
    signal.setDescriptor(sigDesc);

    std::atomic<bool> stopProducer{false};
    std::thread producer(
        [&]
        {
            const auto packet = DataPacket(sigDesc, 16);
            while (!stopProducer.load(std::memory_order_relaxed))
                signal.sendPacket(packet);
        });

    for (int round = 0; round < 100; ++round)
    {
        auto port = InputPort(ctx, nullptr, "ip");
        port.connect(signal);
        const auto connection = port.getConnection();
        auto internal = connection.asPtr<IConnectionInternal>(true);

        std::atomic<bool> stopConsumer{false};
        std::thread consumer(
            [&]
            {
                int op = 0;
                while (!stopConsumer.load(std::memory_order_relaxed))
                {
                    switch (op++ % 10)
                    {
                        case 0: connection.dequeue(); break;
                        case 1: connection.peek(); break;
                        case 2: connection.getPacketCount(); break;
                        case 3: connection.getAvailableSamples(); break;
                        case 4: connection.dequeueAll(); break;
                        case 5: connection.getSamplesUntilNextDescriptor(); break;
                        case 6: connection.getSamplesUntilNextEventPacket(); break;
                        case 7: connection.hasEventPacket(); break;
                        case 8: connection.hasGapPacket(); break;
                        case 9:
                        {
                            IPacket* batch[4] = {};
                            SizeT n = 4;
                            internal->dequeueUpTo(batch, &n);
                            for (SizeT i = 0; i < n; ++i)
                                if (batch[i])
                                    batch[i]->releaseRef();
                            break;
                        }
                    }
                }
            });

        std::this_thread::yield();
        port.disconnect();

        stopConsumer.store(true);
        consumer.join();

        ASSERT_EQ(connection.getPacketCount(), 0u);
        ASSERT_FALSE(connection.dequeue().assigned());
    }

    stopProducer.store(true);
    producer.join();
}

// dequeueUpTo is the hot reader path (Readers use it) and had no concurrent coverage.
// Sequence-numbered payloads make loss/duplication an exact oracle: a single producer +
// single consumer + FIFO queue must yield a strictly contiguous 0,1,2,... with no gaps.
TEST_F(DataPathTest, DequeueUpToNoLossNoDuplication)
{
    const auto ctx = NullContext();
    const auto sigDesc = DataDescriptorBuilder().setSampleType(SampleType::Int64).build();
    const auto signal = Signal(ctx, nullptr, "sig");
    signal.setDescriptor(sigDesc);

    auto port = InputPort(ctx, nullptr, "ip");
    port.connect(signal);
    const auto connection = port.getConnection();
    auto internal = connection.asPtr<IConnectionInternal>(true);

    constexpr int64_t target = 20000;
    std::atomic<bool> stop{false};
    std::thread producer(
        [&]
        {
            for (int64_t i = 0; i < target; ++i)
            {
                const auto packet = DataPacket(sigDesc, 1);
                *static_cast<int64_t*>(packet.getRawData()) = i;
                signal.sendPacket(packet);
            }
            stop.store(true, std::memory_order_release);
        });

    int64_t expected = 0;
    const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(30);
    while (expected < target)
    {
        ASSERT_LT(std::chrono::steady_clock::now(), deadline) << "dequeueUpTo stalled at " << expected;
        IPacket* batch[16] = {};
        SizeT n = 16;
        internal->dequeueUpTo(batch, &n);
        for (SizeT i = 0; i < n; ++i)
        {
            const auto pkt = PacketPtr::Adopt(batch[i]);
            const auto dp = pkt.asPtrOrNull<IDataPacket>(true);
            if (!dp.assigned())  // the leading descriptor event
                continue;
            const int64_t v = *static_cast<const int64_t*>(dp.getRawData());
            ASSERT_EQ(v, expected) << "loss or duplication in dequeueUpTo";
            ++expected;
        }
        if (n == 0)
            std::this_thread::yield();
    }

    producer.join();
}

// Multiple config threads reading the last value concurrently while the producer publishes.
// They serialize on the config lock, but underneath that lock several threads enter the
// reader window and drain the retire list from different threads, racing the producer's
// one-sided recycle decision. Every observed value must be well-formed.
TEST_F(DataPathTest, MultiThreadedGetLastValueVsProducer)
{
    const auto ctx = NullContext();
    const auto sigDesc = DataDescriptorBuilder().setSampleType(SampleType::Int64).build();
    const auto signal = Signal(ctx, nullptr, "sig");
    signal.setDescriptor(sigDesc);

    std::atomic<bool> stop{false};
    std::thread producer(
        [&]
        {
            int64_t counter = 0;
            while (!stop.load(std::memory_order_relaxed))
            {
                const auto packet = DataPacket(sigDesc, 4);
                auto* data = static_cast<int64_t*>(packet.getRawData());
                for (int i = 0; i < 4; ++i)
                    data[i] = counter;
                signal.sendPacket(packet);
                ++counter;
            }
        });

    std::atomic<bool> malformed{false};
    std::vector<std::thread> readers;
    for (int t = 0; t < 4; ++t)
        readers.emplace_back(
            [&]
            {
                for (int i = 0; i < 3000; ++i)
                {
                    const auto v = signal.getLastValue();
                    if (v.assigned() && !v.asPtrOrNull<IInteger>().assigned())
                        malformed.store(true);
                    BaseObjectPtr value;
                    signal.getLastValueWithTimestamp(value);
                    if (value.assigned() && !value.asPtrOrNull<IInteger>().assigned())
                        malformed.store(true);
                }
            });

    for (auto& r : readers)
        r.join();
    stop.store(true);
    producer.join();

    ASSERT_FALSE(malformed.load()) << "getLastValue returned a malformed value under concurrency";
}

// The two operations exempt from the owner-serialization rule - enqueueLastDescriptor
// (setListener) and closeQueue (disconnect) - may race everything, including each other.
// Nothing else covered enqueueLastDescriptor's pendingFrontDescriptor exchange/merge.
TEST_F(DataPathTest, StressSetListenerVsDisconnect)
{
    const auto ctx = NullContext();
    const auto sigDesc = DataDescriptorBuilder().setSampleType(SampleType::Float64).build();
    const auto signal = Signal(ctx, nullptr, "sig");
    signal.setDescriptor(sigDesc);

    std::atomic<bool> stopProducer{false};
    std::thread producer(
        [&]
        {
            const auto packet = DataPacket(sigDesc, 16);
            while (!stopProducer.load(std::memory_order_relaxed))
                signal.sendPacket(packet);
        });

    for (int round = 0; round < 60; ++round)
    {
        auto port = InputPort(ctx, nullptr, "ip");
        port.connect(signal);  // seeds the descriptor event -> latch is set
        const auto connection = port.getConnection();
        auto internal = connection.asPtr<IConnectionInternal>(true);

        std::atomic<bool> stopWorkers{false};
        std::thread descriptorHammer(
            [&]
            {
                while (!stopWorkers.load(std::memory_order_relaxed))
                    internal->enqueueLastDescriptor();  // exempt op racing the teardown
            });
        std::thread consumer(
            [&]
            {
                while (!stopWorkers.load(std::memory_order_relaxed))
                {
                    connection.dequeue();
                    connection.getPacketCount();
                }
            });

        std::this_thread::yield();
        port.disconnect();  // closeQueue racing enqueueLastDescriptor + consumer + producer

        stopWorkers.store(true);
        descriptorHammer.join();
        consumer.join();

        ASSERT_EQ(connection.getPacketCount(), 0u);
    }

    stopProducer.store(true);
    producer.join();
}

// The multi-packet and steal-ref send forms build a pre-linked node chain and skip
// per-node gap checks - a distinct enqueue path from single sendPacket. The steal-ref
// forms are also where a refcount error becomes a use-after-free rather than a leak.
TEST_F(DataPathTest, StressSendVariantsUnderChurn)
{
    const auto ctx = NullContext();
    const auto sigDesc = DataDescriptorBuilder().setSampleType(SampleType::Int64).build();
    const auto signal = Signal(ctx, nullptr, "sig");
    signal.setDescriptor(sigDesc);

    std::atomic<bool> stop{false};
    std::atomic<int64_t> sent{0};
    std::thread producer(
        [&]
        {
            int i = 0;
            while (!stop.load(std::memory_order_relaxed))
            {
                switch (i % 4)
                {
                    case 0:  // single, by const ref
                    {
                        auto p = DataPacket(sigDesc, 4);
                        signal.sendPacket(p);
                        break;
                    }
                    case 1:  // multiple, by const ref
                    {
                        auto list = List<IPacket>();
                        list.pushBack(DataPacket(sigDesc, 4));
                        list.pushBack(DataPacket(sigDesc, 4));
                        signal.sendPackets(list);
                        break;
                    }
                    case 2:  // single, steal ref (rvalue overload)
                        signal.sendPacket(DataPacket(sigDesc, 4));
                        break;
                    case 3:  // multiple, steal ref
                    {
                        auto list = List<IPacket>();
                        list.pushBack(DataPacket(sigDesc, 4));
                        list.pushBack(DataPacket(sigDesc, 4));
                        signal.sendPackets(std::move(list));
                        break;
                    }
                }
                ++i;
                sent.fetch_add(1, std::memory_order_relaxed);
            }
        });

    while (sent.load(std::memory_order_relaxed) == 0)
        std::this_thread::yield();

    for (int round = 0; round < 150; ++round)
    {
        auto port = InputPort(ctx, nullptr, "ip");
        port.connect(signal);
        const auto connection = port.getConnection();
        for (int d = 0; d < 20; ++d)
            connection.dequeueAll();
        port.disconnect();
    }

    stop.store(true);
    producer.join();
    ASSERT_GT(sent.load(), 0);
}

// ============================================================================
// Tier 2 - semantic edges of the new designs
// ============================================================================

// Lost-wakeup guard. The consumer drains ONLY when notified (SameThread packetReceived
// fires on the producer's thread when the queue goes empty->non-empty). If the
// queueEmptyFlag handoff ever drops a notification, a packet strands and the consumer
// never wakes -> the deadline fires. There is deliberately no timeout-driven re-drain.
namespace
{
// Notifies an external condition variable on packetReceived; sync state is injected via
// the constructor so the test owns it directly (no getObject cast needed).
struct NotifyDrainListener : ImplementationOfWeak<IInputPortNotifications>
{
    std::mutex& m;
    std::condition_variable& cv;
    bool& pending;

    NotifyDrainListener(std::mutex& m, std::condition_variable& cv, bool& pending)
        : m(m)
        , cv(cv)
        , pending(pending)
    {
    }

    ErrCode INTERFACE_FUNC acceptsSignal(IInputPort*, ISignal*, daq::Bool* accept) override
    {
        *accept = daq::True;
        return OPENDAQ_SUCCESS;
    }
    ErrCode INTERFACE_FUNC connected(IInputPort*) override { return OPENDAQ_SUCCESS; }
    ErrCode INTERFACE_FUNC disconnected(IInputPort*) override { return OPENDAQ_SUCCESS; }
    ErrCode INTERFACE_FUNC packetReceived(IInputPort*) override
    {
        {
            std::lock_guard<std::mutex> lock(m);
            pending = true;
        }
        cv.notify_one();
        return OPENDAQ_SUCCESS;
    }
};
}

TEST_F(DataPathTest, LostWakeupLiveness)
{
    const auto ctx = NullContext();
    const auto sigDesc = DataDescriptorBuilder().setSampleType(SampleType::Int64).build();
    const auto signal = Signal(ctx, nullptr, "sig");
    signal.setDescriptor(sigDesc);

    std::mutex m;
    std::condition_variable cv;
    bool pending = false;

    auto port = InputPort(ctx, nullptr, "ip");
    port.setNotificationMethod(PacketReadyNotification::SameThread);
    auto listener = createWithImplementation<IInputPortNotifications, NotifyDrainListener>(m, cv, pending);
    port.setListener(listener);
    port.connect(signal);
    const auto connection = port.getConnection();

    constexpr int64_t target = 10000;
    std::atomic<int64_t> received{0};
    std::atomic<bool> stop{false};

    std::thread consumer(
        [&]
        {
            while (!stop.load(std::memory_order_relaxed))
            {
                std::unique_lock<std::mutex> lock(m);
                cv.wait_for(lock, std::chrono::milliseconds(50),
                            [&] { return pending || stop.load(std::memory_order_relaxed); });
                if (!pending)
                    continue;  // spurious/timeout: do NOT drain, so a real lost wakeup is not masked
                pending = false;
                lock.unlock();

                // notified: drain everything currently queued
                for (;;)
                {
                    const auto pkt = connection.dequeue();
                    if (!pkt.assigned())
                        break;
                    if (pkt.supportsInterface<IDataPacket>())
                        received.fetch_add(1, std::memory_order_relaxed);
                }
            }
        });

    for (int64_t i = 0; i < target; ++i)
    {
        signal.sendPacket(DataPacket(sigDesc, 1));
        if ((i & 0x3f) == 0)
            std::this_thread::yield();  // open empty<->non-empty windows
    }

    const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(30);
    while (received.load(std::memory_order_relaxed) < target)
    {
        ASSERT_LT(std::chrono::steady_clock::now(), deadline)
            << "lost wakeup: stranded at " << received.load() << " / " << target;
        std::this_thread::sleep_for(std::chrono::milliseconds(5));
    }

    stop.store(true);
    cv.notify_all();
    consumer.join();
    ASSERT_EQ(received.load(), target);
}

// When gap-checking throws mid-drain, the drain stashes the rest of the inbox in
// pendingDrainChain, drops the offending packet, and resumes on the next consumer call.
// Sequence [bad-data-before-event, event, good-data]: the first dequeue throws, then the
// event and the good packet must still come through intact on subsequent dequeues.
TEST_F(DataPathTest, GapThrowResumesRemainingPackets)
{
    const auto ctx = NullContext();
    MockInputPort::Strict inputPort;
    MockSignal::Strict signal;
    EXPECT_CALL(inputPort.mock(), getGapCheckingEnabled(testing::_)).WillOnce(GetBool(True));
    const auto connection = Connection(inputPort.ptr, signal.ptr, ctx);

    const auto valueDesc = DataDescriptorBuilder().setSampleType(SampleType::Float64).build();
    const auto domainDesc = DataDescriptorBuilder()
                                .setSampleType(SampleType::Int64)
                                .setRule(LinearDataRule(10, 0))
                                .build();

    // all three land in the inbox before any dequeue, so one drain sees the whole chain
    const auto bad = DataPacketWithDomain(DataPacket(domainDesc, 10, 0), valueDesc, 10);   // data before event
    const auto event = DataDescriptorChangedEventPacket(valueDesc, domainDesc);
    const auto good = DataPacketWithDomain(DataPacket(domainDesc, 10, 100), valueDesc, 10);
    connection.enqueue(bad);
    connection.enqueue(event);
    connection.enqueue(good);

    // drain hits `bad` first (state uninitialized) and throws; event + good are stashed
    ASSERT_THROW(connection.dequeue(), InvalidStateException);
    // resume: the descriptor event initializes gap state, then the good data packet follows
    const auto p1 = connection.dequeue();
    ASSERT_TRUE(p1.assigned());
    ASSERT_TRUE(p1.supportsInterface<IEventPacket>());
    const auto p2 = connection.dequeue();
    ASSERT_TRUE(p2.assigned());
    ASSERT_EQ(p2, good);
}

// closeQueue must free a parked pendingDrainChain (the stash from a gap-throw) without
// leaking or crashing, and leave the queue reporting closed/empty.
TEST_F(DataPathTest, CloseQueueWithParkedDrainChain)
{
    const auto ctx = NullContext();
    MockInputPort::Strict inputPort;
    MockSignal::Strict signal;
    EXPECT_CALL(inputPort.mock(), getGapCheckingEnabled(testing::_)).WillOnce(GetBool(True));
    const auto connection = Connection(inputPort.ptr, signal.ptr, ctx);

    const auto valueDesc = DataDescriptorBuilder().setSampleType(SampleType::Float64).build();
    const auto domainDesc = DataDescriptorBuilder()
                                .setSampleType(SampleType::Int64)
                                .setRule(LinearDataRule(10, 0))
                                .build();

    connection.enqueue(DataPacketWithDomain(DataPacket(domainDesc, 10, 0), valueDesc, 10));   // bad
    connection.enqueue(DataDescriptorChangedEventPacket(valueDesc, domainDesc));              // stashed
    connection.enqueue(DataPacketWithDomain(DataPacket(domainDesc, 10, 100), valueDesc, 10)); // stashed

    ASSERT_THROW(connection.dequeue(), InvalidStateException);  // parks event + good

    auto internal = connection.asPtr<IConnectionInternal>(true);
    ASSERT_NO_THROW(internal->closeQueue());  // must release the parked chain
    ASSERT_EQ(connection.getPacketCount(), 0u);
    ASSERT_FALSE(connection.dequeue().assigned());
}

// Descriptor churn across sample types of different sizes while readers hammer getLastValue.
// Drives the staged node's descriptor cache (pointer-identity skip), the per-packet buffer
// resize on descriptor change, and cacheRaw for varying sample sizes.
TEST_F(DataPathTest, LastValueUnderDescriptorChurn)
{
    const auto ctx = NullContext();
    const auto signal = Signal(ctx, nullptr, "sig");

    const SampleType types[] = {SampleType::Int16, SampleType::Int32, SampleType::Int64, SampleType::Float64};
    std::vector<DataDescriptorPtr> descs;
    for (auto t : types)
        descs.push_back(DataDescriptorBuilder().setSampleType(t).build());
    signal.setDescriptor(descs[0]);

    std::atomic<bool> stop{false};
    std::atomic<bool> failed{false};
    std::thread producer(
        [&]
        {
            int i = 0;
            while (!stop.load(std::memory_order_relaxed))
            {
                const auto& d = descs[i % descs.size()];
                signal.setDescriptor(d);
                signal.sendPacket(DataPacket(d, 8));
                ++i;
            }
        });

    for (int i = 0; i < 4000; ++i)
    {
        try
        {
            const auto v = signal.getLastValue();
            if (v.assigned() && !v.asPtrOrNull<INumber>().assigned())
                failed.store(true);
        }
        catch (...)
        {
            failed.store(true);
        }
    }

    stop.store(true);
    producer.join();
    ASSERT_FALSE(failed.load()) << "getLastValue misbehaved under descriptor churn";
}

// setLastValue is producer-role; enableKeepLastValue is config-role - they may race, and
// the design tolerates it. Also covers setLastValue(nullptr) and the timestamp path.
TEST_F(DataPathTest, SetLastValueEdgeCasesAndRace)
{
    const auto ctx = NullContext();
    const auto sigDesc = DataDescriptorBuilder().setSampleType(SampleType::Int64).build();
    const auto signal = Signal(ctx, nullptr, "sig");
    signal.setDescriptor(sigDesc);
    signal.asPtr<ISignalPrivate>(true).enableKeepLastValue(false);

    // nullptr explicit value -> nothing cached
    signal.setLastValue(nullptr);
    ASSERT_FALSE(signal.getLastValue().assigned());

    // explicit value + timestamp path (no domain -> timestamp unassigned)
    signal.setLastValue(7);
    ASSERT_EQ(signal.getLastValue(), 7);
    BaseObjectPtr value;
    BaseObjectPtr ts;
    ASSERT_NO_THROW(ts = signal.getLastValueWithTimestamp(value));
    ASSERT_EQ(value, 7);
    ASSERT_FALSE(ts.assigned());

    // race: producer role (setLastValue + sendPacket) vs config role (enableKeepLastValue)
    std::atomic<bool> stop{false};
    std::atomic<bool> crashed{false};
    std::thread producerRole(
        [&]
        {
            int64_t c = 0;
            while (!stop.load(std::memory_order_relaxed))
            {
                try
                {
                    signal.setLastValue(++c);  // throws INVALIDSTATE if caching got enabled - tolerated
                }
                catch (...)
                {
                }
                const auto pk = DataPacket(sigDesc, 4);
                *static_cast<int64_t*>(pk.getRawData()) = c;
                signal.sendPacket(pk);
            }
        });
    std::thread configRole(
        [&]
        {
            for (int i = 0; i < 4000; ++i)
                signal.asPtr<ISignalPrivate>(true).enableKeepLastValue(i % 2 == 0);
        });
    std::thread reader(
        [&]
        {
            for (int i = 0; i < 4000; ++i)
            {
                try
                {
                    signal.getLastValue();
                }
                catch (...)
                {
                    crashed.store(true);
                }
            }
        });

    configRole.join();
    reader.join();
    stop.store(true);
    producerRole.join();
    ASSERT_FALSE(crashed.load());

    // settled: caching off, last explicit value wins
    signal.asPtr<ISignalPrivate>(true).enableKeepLastValue(false);
    signal.setLastValue(12345);
    ASSERT_EQ(static_cast<Int>(signal.getLastValue()), 12345);
}

// Zero-sample data packets (staging must skip them but the queue must still deliver) and
// event packets sent through sendPacket (which bypass staging entirely), plain and under
// connect/disconnect churn.
TEST_F(DataPathTest, EmptyAndEventPacketsThroughSendPath)
{
    const auto ctx = NullContext();
    const auto sigDesc = DataDescriptorBuilder().setSampleType(SampleType::Int64).build();
    const auto signal = Signal(ctx, nullptr, "sig");
    signal.setDescriptor(sigDesc);

    {
        auto port = InputPort(ctx, nullptr, "ip");
        port.connect(signal);
        const auto connection = port.getConnection();
        connection.dequeueAll();  // clear the connect descriptor event

        // zero-sample data packet: last-value staging skips it, the queue still carries it
        signal.sendPacket(DataPacket(sigDesc, 0));
        // event packet via sendPacket bypasses staging
        signal.sendPacket(DataDescriptorChangedEventPacket(sigDesc, nullptr));

        const auto all = connection.dequeueAll();
        ASSERT_EQ(all.getCount(), 2u);
        ASSERT_TRUE(all[0].supportsInterface<IDataPacket>());
        ASSERT_EQ(all[0].asPtr<IDataPacket>(true).getSampleCount(), 0u);
        ASSERT_TRUE(all[1].supportsInterface<IEventPacket>());
        // getLastValue unaffected by the zero-sample packet (nothing was staged)
        ASSERT_FALSE(signal.getLastValue().assigned());
        port.disconnect();
    }

    // churn variant: interleave empty / normal / event packets under connect-disconnect
    std::atomic<bool> stop{false};
    std::thread producer(
        [&]
        {
            int i = 0;
            while (!stop.load(std::memory_order_relaxed))
            {
                switch (i % 3)
                {
                    case 0: signal.sendPacket(DataPacket(sigDesc, 0)); break;
                    case 1: signal.sendPacket(DataPacket(sigDesc, 8)); break;
                    case 2: signal.sendPacket(DataDescriptorChangedEventPacket(sigDesc, nullptr)); break;
                }
                ++i;
            }
        });

    for (int round = 0; round < 100; ++round)
    {
        auto port = InputPort(ctx, nullptr, "ip");
        port.connect(signal);
        const auto connection = port.getConnection();
        connection.dequeueAll();
        port.disconnect();
    }

    stop.store(true);
    producer.join();
}
