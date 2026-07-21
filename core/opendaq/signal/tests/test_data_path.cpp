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
#include <coreobjects/unit_factory.h>
#include <atomic>
#include <chrono>
#include <thread>

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
