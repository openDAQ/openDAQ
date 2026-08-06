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
#include <memory>
#include <mutex>
#include <thread>
#include <vector>

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

namespace
{
// Shared helpers: every data-path test stands up the same descriptor/signal/port/worker
// scaffolding, so that lives here and each test keeps only what is specific to it.

// Plain single-field descriptor of the given sample type.
DataDescriptorPtr plainDescriptor(SampleType sampleType)
{
    return DataDescriptorBuilder().setSampleType(sampleType).build();
}

// Int64 time-domain descriptor (tick 1/1000 s from the 1970 epoch) with the given rule.
DataDescriptorPtr domainDescriptor(const DataRulePtr& rule)
{
    return DataDescriptorBuilder()
        .setSampleType(SampleType::Int64)
        .setRule(rule)
        .setTickResolution(Ratio(1, 1000))
        .setUnit(Unit("s", -1, "second", "time"))
        .setOrigin("1970-01-01T00:00:00Z")
        .build();
}

// Send one data packet whose every sample equals `value` (the counter/sequence oracle).
void sendFilledInt64(const SignalConfigPtr& signal, const DataDescriptorPtr& desc, SizeT sampleCount, int64_t value)
{
    const auto packet = DataPacket(desc, sampleCount);
    auto* data = static_cast<int64_t*>(packet.getRawData());
    for (SizeT i = 0; i < sampleCount; ++i)
        data[i] = value;
    signal.sendPacket(packet);
}

// RAII background worker: runs `body()` in a tight loop on its own thread until join()/destruction.
class BackgroundLoop
{
public:
    template <typename Body>
    explicit BackgroundLoop(Body body)
        : stopFlag(std::make_shared<std::atomic<bool>>(false))
    {
        auto flag = stopFlag;
        worker = std::thread(
            [flag, body = std::move(body)]() mutable
            {
                while (!flag->load(std::memory_order_relaxed))
                    body();
            });
    }
    BackgroundLoop(BackgroundLoop&&) = default;
    BackgroundLoop& operator=(BackgroundLoop&&) = default;
    ~BackgroundLoop() { join(); }

    // Stop the loop and wait for the thread. Idempotent.
    void join()
    {
        if (stopFlag)
            stopFlag->store(true, std::memory_order_relaxed);
        if (worker.joinable())
            worker.join();
    }

private:
    std::shared_ptr<std::atomic<bool>> stopFlag;
    std::thread worker;
};

// Background producer that repeatedly sends one fixed data packet at `signal` until stopped.
BackgroundLoop spamPackets(const SignalConfigPtr& signal, const DataDescriptorPtr& desc, SizeT sampleCount = 16)
{
    return BackgroundLoop([signal, packet = DataPacket(desc, sampleCount)] { signal.sendPacket(packet); });
}

// True iff `rounds` getLastValue calls (plus, optionally, the timestamp variant) observe only
// unassigned or well-formed `Expected` values without throwing. No ASSERTs, so it is usable
// from worker threads.
template <typename Expected = INumber>
bool lastValuesWellFormed(const SignalConfigPtr& signal, int rounds, bool withTimestamp = false)
{
    try
    {
        for (int i = 0; i < rounds; ++i)
        {
            const auto v = signal.getLastValue();
            if (v.assigned() && !v.asPtrOrNull<Expected>().assigned())
                return false;
            if (withTimestamp)
            {
                BaseObjectPtr value;
                signal.getLastValueWithTimestamp(value);
                if (value.assigned() && !value.asPtrOrNull<Expected>().assigned())
                    return false;
            }
        }
    }
    catch (...)
    {
        return false;
    }
    return true;
}
}  // namespace

class DataPathTest : public Test
{
protected:
    struct Endpoint
    {
        InputPortConfigPtr port;
        ConnectionPtr connection;
        ObjectPtr<IConnectionInternal> internal;
    };

    // A signal wired up with `desc`, ready to send or connect.
    SignalConfigPtr makeSignal(const DataDescriptorPtr& desc, const char* id = "sig")
    {
        const auto signal = Signal(ctx, nullptr, id);
        signal.setDescriptor(desc);
        return signal;
    }

    // A fresh input port connected to `signal`, plus its connection and internal interface.
    Endpoint attach(const SignalConfigPtr& signal, bool gapChecking = false)
    {
        auto port = InputPort(ctx, nullptr, "ip", gapChecking);
        port.connect(signal);
        auto connection = port.getConnection();
        auto internal = connection.asPtr<IConnectionInternal>(true);
        return {port, connection, internal};
    }

    // Gap-checking connection whose inbox already held [bad data, event, good data]: the first
    // dequeue throws (data-before-event), parking event + good (kept in `gapGood`) in
    // pendingDrainChain.
    ConnectionPtr parkedGapConnection()
    {
        EXPECT_CALL(gapPort.mock(), getGapCheckingEnabled(testing::_)).WillOnce(GetBool(True));
        const auto connection = Connection(gapPort.ptr, gapSignal.ptr, ctx);

        const auto valueDesc = plainDescriptor(SampleType::Float64);
        const auto domainDesc = domainDescriptor(LinearDataRule(10, 0));
        connection.enqueue(DataPacketWithDomain(DataPacket(domainDesc, 10, 0), valueDesc, 10));  // data before event
        connection.enqueue(DataDescriptorChangedEventPacket(valueDesc, domainDesc));
        gapGood = DataPacketWithDomain(DataPacket(domainDesc, 10, 100), valueDesc, 10);
        connection.enqueue(gapGood);

        EXPECT_THROW(connection.dequeue(), InvalidStateException);
        return connection;
    }

    ContextPtr ctx = NullContext();
    MockInputPort::Strict gapPort;
    MockSignal::Strict gapSignal;
    PacketPtr gapGood;
};

TEST_F(DataPathTest, DestroyListenerInNotification)
{
    const auto sigDesc = plainDescriptor(SampleType::Int32);
    const auto signal = makeSignal(sigDesc);

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

// Lock-free data path stress tests: they encode the concurrency invariants of the
// snapshot/tombstone design and are meant to run under TSAN on platforms that have it.

// Producer sends continuously while connect/disconnect churns; gap checking makes the consumer
// verify the descriptor-event-before-data invariant on every reconnect.
TEST_F(DataPathTest, StressSendVsConnectDisconnect)
{
    const auto domainDesc = domainDescriptor(LinearDataRule(10, 0));
    const auto sigDesc = plainDescriptor(SampleType::Float64);

    const auto domainSignal = makeSignal(domainDesc, "domain");
    const auto signal = makeSignal(sigDesc);
    signal.setDomainSignal(domainSignal);

    std::atomic<int64_t> sent{0};
    BackgroundLoop producer(
        [&, offset = int64_t{0}]() mutable
        {
            const auto domainPacket = DataPacket(domainDesc, 16, offset);
            signal.sendPacket(DataPacketWithDomain(domainPacket, sigDesc, 16));
            offset += 160;
            sent.fetch_add(1, std::memory_order_relaxed);
        });

    // ensure the roles genuinely overlap (thread startup can exceed the test duration)
    while (sent.load(std::memory_order_relaxed) == 0)
        std::this_thread::yield();

    for (int round = 0; round < 200; ++round)
    {
        auto ep = attach(signal, true /*gap checking*/);

        bool first = true;
        for (int reads = 0; reads < 50; ++reads)
        {
            PacketPtr packet;
            ASSERT_NO_THROW(packet = ep.connection.dequeue()) << "data-before-event or torn queue on round " << round;
            if (!packet.assigned())
                continue;
            if (first)
            {
                ASSERT_TRUE(packet.supportsInterface<IEventPacket>()) << "first packet on a new connection must be the descriptor event";
                first = false;
            }
        }

        ep.port.disconnect();
    }

    producer.join();
    ASSERT_GT(sent.load(), 0);
}

// closeQueue vs an actively draining consumer and a live producer: teardown must never crash,
// hang, or corrupt the queue.
TEST_F(DataPathTest, StressDisconnectUnderFire)
{
    const auto sigDesc = plainDescriptor(SampleType::Float64);
    const auto signal = makeSignal(sigDesc);
    auto producer = spamPackets(signal, sigDesc);

    for (int round = 0; round < 100; ++round)
    {
        auto ep = attach(signal);
        BackgroundLoop consumer(
            [&]
            {
                ep.connection.dequeue();
                ep.connection.getPacketCount();
            });

        std::this_thread::yield();
        ep.port.disconnect();  // tombstone + drain while both other roles are active
        consumer.join();

        // closed queue stays empty and rejects everything
        ASSERT_EQ(ep.connection.getPacketCount(), 0u);
        ASSERT_FALSE(ep.connection.dequeue().assigned());
    }

    producer.join();
}

// setDescriptor events interleaved with data packets (owner-serialized via ownerMutex, per the
// openDAQ usage rule) while the consumer drains the mixed stream concurrently; no event may be lost.
TEST_F(DataPathTest, StressDescriptorEventsVsProducer)
{
    const auto descA = plainDescriptor(SampleType::Float64);
    const auto descB = DataDescriptorBuilder().setSampleType(SampleType::Float64).setValueRange(daq::Range(0, 10)).build();
    const auto signal = makeSignal(descA);
    auto ep = attach(signal);

    std::mutex ownerMutex;  // the owner's external serialization of production vs configuration

    BackgroundLoop producer(
        [&, packetA = DataPacket(descA, 16)]
        {
            std::lock_guard lock(ownerMutex);
            signal.sendPacket(packetA);
        });

    std::thread config(
        [&]
        {
            for (int i = 0; i < 200; ++i)
            {
                {
                    std::lock_guard lock(ownerMutex);
                    signal.setDescriptor(i % 2 ? descB : descA);
                }
                std::this_thread::yield();
            }
        });

    size_t eventPackets = 0;
    const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(30);
    while (eventPackets < 100)
    {
        ASSERT_LT(std::chrono::steady_clock::now(), deadline) << "descriptor events lost in the queue";
        const auto packet = ep.connection.dequeue();
        if (packet.assigned() && packet.supportsInterface<IEventPacket>())
            ++eventPackets;
    }

    config.join();
    producer.join();

    // full drain leaves consistent counters
    ep.connection.dequeueAll();
    ASSERT_EQ(ep.connection.getPacketCount(), 0u);
    ASSERT_EQ(ep.connection.getAvailableSamples(), 0u);
}

// Snapshot publish churn with never-consumed queues: producers pinning old snapshots must keep
// disconnected connections alive exactly until close, and closeQueue must release every packet
// (the suite's leak listener verifies the lifetime half).
TEST_F(DataPathTest, StressSnapshotChurnUnconsumedQueues)
{
    const auto sigDesc = plainDescriptor(SampleType::Float64);
    const auto signal = makeSignal(sigDesc);
    auto producer = spamPackets(signal, sigDesc);

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

    producer.join();
}

// Config-path hammering (last-value reads, keep-last and active toggles) against a live producer.
TEST_F(DataPathTest, StressSendVsGetLastValueAndActive)
{
    const auto sigDesc = plainDescriptor(SampleType::Int64);
    const auto signal = makeSignal(sigDesc);

    BackgroundLoop producer(
        [&, counter = int64_t{0}]() mutable
        {
            sendFilledInt64(signal, sigDesc, 4, counter);
            ++counter;
        });

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

    producer.join();
}

// Contract: the signal must not retain any reference to a sent packet after sendPacket returns,
// even with keep-last-value enabled - the last value is copied out at send time.
TEST_F(DataPathTest, SendPacketDoesNotRetainPacket)
{
    const auto domainDesc = domainDescriptor(ExplicitDataRule());
    const auto sigDesc = plainDescriptor(SampleType::Int64);
    const auto signal = makeSignal(sigDesc);

    auto domainPacket = DataPacket(domainDesc, 2);
    static_cast<int64_t*>(domainPacket.getRawData())[0] = 1000;
    static_cast<int64_t*>(domainPacket.getRawData())[1] = 2000;
    auto packet = DataPacketWithDomain(domainPacket, sigDesc, 2);
    static_cast<int64_t*>(packet.getRawData())[0] = 11;
    static_cast<int64_t*>(packet.getRawData())[1] = 42;

    signal.sendPacket(packet);

    // refcount probe: the counts must stem from this test's own references alone
    packet->addRef();
    ASSERT_EQ(packet->releaseRef(), 1);
    domainPacket->addRef();
    ASSERT_EQ(domainPacket->releaseRef(), 2);  // ours + the value packet's reference

    // the last value must survive the packets being destroyed (copied at send time)
    domainPacket = nullptr;
    packet = nullptr;
    ASSERT_EQ(signal.getLastValue(), 42);

    BaseObjectPtr value;
    BaseObjectPtr ts;
    ASSERT_NO_THROW(ts = signal.getLastValueWithTimestamp(value));
    ASSERT_EQ(value, 42);
    ASSERT_EQ(ts, 2000000);  // tick 2000 at resolution 1/1000 s from the 1970 epoch, in us
}

// setLastValue (producer-role, lock-free, owner-serialized with sendPacket) vs a config thread
// hammering getLastValue: every observed value must be well-formed.
TEST_F(DataPathTest, StressSetLastValueVsGetLastValue)
{
    const auto sigDesc = plainDescriptor(SampleType::Int64);
    const auto signal = makeSignal(sigDesc);
    signal.asPtr<ISignalPrivate>(true).enableKeepLastValue(false);

    int64_t counter = 0;
    BackgroundLoop producer(
        [&]
        {
            sendFilledInt64(signal, sigDesc, 4, counter);
            signal.setLastValue(++counter);
        });

    ASSERT_TRUE(lastValuesWellFormed<IInteger>(signal, 4000));
    producer.join();

    // keep-last is off, so sendPacket never publishes: the last explicit value must be visible
    if (counter > 0)
        ASSERT_EQ(static_cast<Int>(signal.getLastValue()), counter);
}

// Implicit-rule packets: getRawLastValue computes the last sample into the staged buffer at
// send time, so the value survives without the packet ever being retained.
TEST_F(DataPathTest, LastValueFromCalculatedPacket)
{
    const auto sigDesc = DataDescriptorBuilder()
                             .setSampleType(SampleType::Int64)
                             .setRule(LinearDataRule(2, 10))
                             .build();
    const auto signal = makeSignal(sigDesc);

    auto packet = DataPacket(sigDesc, 3, 5);
    const auto expected = packet.getLastValue();
    signal.sendPacket(packet);

    packet->addRef();
    ASSERT_EQ(packet->releaseRef(), 1);

    packet = nullptr;
    ASSERT_EQ(signal.getLastValue(), expected);
}

// ===== Tier 1: consumer-API surface and send variants =====

// After closeQueue, every consumer entry point must return a synthesized empty answer.
TEST_F(DataPathTest, ClosedQueueConsumerApiMatrix)
{
    const auto sigDesc = plainDescriptor(SampleType::Int64);
    const auto signal = makeSignal(sigDesc);
    auto ep = attach(signal);

    // a mix of queued packets makes "closed returns empty" a real transition
    for (int i = 0; i < 3; ++i)
        signal.sendPacket(DataPacket(sigDesc, 8));
    ep.internal->closeQueue();

    ASSERT_EQ(ep.connection.getPacketCount(), 0u);
    ASSERT_EQ(ep.connection.getAvailableSamples(), 0u);
    ASSERT_EQ(ep.connection.getSamplesUntilNextDescriptor(), 0u);
    ASSERT_EQ(ep.connection.getSamplesUntilNextEventPacket(), 0u);
    ASSERT_EQ(ep.connection.getSamplesUntilNextGapPacket(), 0u);
    ASSERT_FALSE(ep.connection.hasEventPacket());
    ASSERT_FALSE(ep.connection.hasGapPacket());
    ASSERT_FALSE(ep.connection.dequeue().assigned());
    ASSERT_FALSE(ep.connection.peek().assigned());
    ASSERT_EQ(ep.connection.dequeueAll().getCount(), 0u);
    IPacket* batch[8] = {};
    SizeT count = 8;
    ep.internal->dequeueUpTo(batch, &count);
    ASSERT_EQ(count, 0u);
}

// Every consumer op rotates against closeQueue teardown under a live producer, exercising each
// op's gate-enter / closed-check / drain path (generalizes StressDisconnectUnderFire).
TEST_F(DataPathTest, StressAllConsumerOpsUnderFire)
{
    const auto sigDesc = plainDescriptor(SampleType::Float64);
    const auto signal = makeSignal(sigDesc);
    auto producer = spamPackets(signal, sigDesc);

    for (int round = 0; round < 100; ++round)
    {
        auto ep = attach(signal);
        BackgroundLoop consumer(
            [&, op = 0]() mutable
            {
                switch (op++ % 10)
                {
                    case 0: ep.connection.dequeue(); break;
                    case 1: ep.connection.peek(); break;
                    case 2: ep.connection.getPacketCount(); break;
                    case 3: ep.connection.getAvailableSamples(); break;
                    case 4: ep.connection.dequeueAll(); break;
                    case 5: ep.connection.getSamplesUntilNextDescriptor(); break;
                    case 6: ep.connection.getSamplesUntilNextEventPacket(); break;
                    case 7: ep.connection.hasEventPacket(); break;
                    case 8: ep.connection.hasGapPacket(); break;
                    case 9:
                    {
                        IPacket* batch[4] = {};
                        SizeT n = 4;
                        ep.internal->dequeueUpTo(batch, &n);
                        for (SizeT i = 0; i < n; ++i)
                            if (batch[i])
                                batch[i]->releaseRef();
                        break;
                    }
                }
            });

        std::this_thread::yield();
        ep.port.disconnect();
        consumer.join();

        ASSERT_EQ(ep.connection.getPacketCount(), 0u);
        ASSERT_FALSE(ep.connection.dequeue().assigned());
    }

    producer.join();
}

// dequeueUpTo (the hot Reader path): sequence-numbered payloads through a single producer /
// single consumer / FIFO queue must arrive strictly contiguous - no loss, no duplication.
TEST_F(DataPathTest, DequeueUpToNoLossNoDuplication)
{
    const auto sigDesc = plainDescriptor(SampleType::Int64);
    const auto signal = makeSignal(sigDesc);
    auto ep = attach(signal);

    constexpr int64_t target = 20000;
    std::thread producer(
        [&]
        {
            for (int64_t i = 0; i < target; ++i)
                sendFilledInt64(signal, sigDesc, 1, i);
        });

    int64_t expected = 0;
    const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(30);
    while (expected < target)
    {
        ASSERT_LT(std::chrono::steady_clock::now(), deadline) << "dequeueUpTo stalled at " << expected;
        IPacket* batch[16] = {};
        SizeT n = 16;
        ep.internal->dequeueUpTo(batch, &n);
        for (SizeT i = 0; i < n; ++i)
        {
            const auto pkt = PacketPtr::Adopt(batch[i]);
            const auto dp = pkt.asPtrOrNull<IDataPacket>(true);
            if (!dp.assigned())  // the leading descriptor event
                continue;
            ASSERT_EQ(*static_cast<const int64_t*>(dp.getRawData()), expected) << "loss or duplication in dequeueUpTo";
            ++expected;
        }
        if (n == 0)
            std::this_thread::yield();
    }

    producer.join();
}

// Several threads reading the last value while the producer publishes: concurrent retire-list
// draining races the producer's one-sided recycle decision and must never surface a malformed value.
TEST_F(DataPathTest, MultiThreadedGetLastValueVsProducer)
{
    const auto sigDesc = plainDescriptor(SampleType::Int64);
    const auto signal = makeSignal(sigDesc);

    BackgroundLoop producer(
        [&, counter = int64_t{0}]() mutable
        {
            sendFilledInt64(signal, sigDesc, 4, counter);
            ++counter;
        });

    std::atomic<bool> malformed{false};
    std::vector<std::thread> readers;
    for (int t = 0; t < 4; ++t)
        readers.emplace_back(
            [&]
            {
                if (!lastValuesWellFormed<IInteger>(signal, 3000, true /*withTimestamp*/))
                    malformed.store(true);
            });

    for (auto& r : readers)
        r.join();
    producer.join();

    ASSERT_FALSE(malformed.load()) << "getLastValue returned a malformed value under concurrency";
}

// The two owner-serialization-exempt ops - enqueueLastDescriptor (setListener) and closeQueue
// (disconnect) - may race everything, including each other, a consumer, and the producer.
TEST_F(DataPathTest, StressSetListenerVsDisconnect)
{
    const auto sigDesc = plainDescriptor(SampleType::Float64);
    const auto signal = makeSignal(sigDesc);
    auto producer = spamPackets(signal, sigDesc);

    for (int round = 0; round < 60; ++round)
    {
        auto ep = attach(signal);  // connect seeds the descriptor event -> latch is set
        BackgroundLoop descriptorHammer([&] { ep.internal->enqueueLastDescriptor(); });
        BackgroundLoop consumer(
            [&]
            {
                ep.connection.dequeue();
                ep.connection.getPacketCount();
            });

        std::this_thread::yield();
        ep.port.disconnect();  // closeQueue racing enqueueLastDescriptor + consumer + producer

        descriptorHammer.join();
        consumer.join();
        ASSERT_EQ(ep.connection.getPacketCount(), 0u);
    }

    producer.join();
}

// The multi-packet and steal-ref send forms build a pre-linked node chain (a distinct enqueue
// path from single sendPacket); a refcount error in the steal-ref forms is a use-after-free.
TEST_F(DataPathTest, StressSendVariantsUnderChurn)
{
    const auto sigDesc = plainDescriptor(SampleType::Int64);
    const auto signal = makeSignal(sigDesc);

    std::atomic<int64_t> sent{0};
    BackgroundLoop producer(
        [&, i = 0]() mutable
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
        });

    while (sent.load(std::memory_order_relaxed) == 0)
        std::this_thread::yield();

    for (int round = 0; round < 150; ++round)
    {
        auto ep = attach(signal);
        for (int d = 0; d < 20; ++d)
            ep.connection.dequeueAll();
        ep.port.disconnect();
    }

    producer.join();
    ASSERT_GT(sent.load(), 0);
}

// ===== Tier 2: semantic edges of the new designs =====

namespace
{
// Notifies an external condition variable on packetReceived; sync state is injected via the ctor.
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

// Lost-wakeup guard: the consumer drains ONLY when notified (no timeout-driven re-drain). If the
// queueEmptyFlag handoff ever drops an empty->non-empty notification, a packet strands and the
// deadline fires.
TEST_F(DataPathTest, LostWakeupLiveness)
{
    const auto sigDesc = plainDescriptor(SampleType::Int64);
    const auto signal = makeSignal(sigDesc);

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

// A gap-check throw mid-drain parks the rest of the inbox in pendingDrainChain; the event and
// the good data packet must still come through intact on subsequent dequeues.
TEST_F(DataPathTest, GapThrowResumesRemainingPackets)
{
    const auto connection = parkedGapConnection();

    const auto p1 = connection.dequeue();  // the descriptor event initializes gap state
    ASSERT_TRUE(p1.assigned());
    ASSERT_TRUE(p1.supportsInterface<IEventPacket>());
    const auto p2 = connection.dequeue();
    ASSERT_TRUE(p2.assigned());
    ASSERT_EQ(p2, gapGood);
}

// closeQueue must free a parked pendingDrainChain without leaking or crashing, and leave the
// queue reporting closed/empty.
TEST_F(DataPathTest, CloseQueueWithParkedDrainChain)
{
    const auto connection = parkedGapConnection();

    auto internal = connection.asPtr<IConnectionInternal>(true);
    ASSERT_NO_THROW(internal->closeQueue());
    ASSERT_EQ(connection.getPacketCount(), 0u);
    ASSERT_FALSE(connection.dequeue().assigned());
}

// Descriptor churn across sample types of different sizes while readers hammer getLastValue:
// drives the staged node's descriptor cache, the per-packet buffer resize, and cacheRaw.
TEST_F(DataPathTest, LastValueUnderDescriptorChurn)
{
    const SampleType types[] = {SampleType::Int16, SampleType::Int32, SampleType::Int64, SampleType::Float64};
    std::vector<DataDescriptorPtr> descs;
    for (auto t : types)
        descs.push_back(plainDescriptor(t));
    const auto signal = makeSignal(descs[0]);

    BackgroundLoop producer(
        [&, i = 0]() mutable
        {
            const auto& d = descs[i % descs.size()];
            signal.setDescriptor(d);
            signal.sendPacket(DataPacket(d, 8));
            ++i;
        });

    const bool wellFormed = lastValuesWellFormed(signal, 4000);
    producer.join();
    ASSERT_TRUE(wellFormed) << "getLastValue misbehaved under descriptor churn";
}

// setLastValue (producer-role) may race enableKeepLastValue (config-role) - the design tolerates
// it. Also covers setLastValue(nullptr) and the timestamp path.
TEST_F(DataPathTest, SetLastValueEdgeCasesAndRace)
{
    const auto sigDesc = plainDescriptor(SampleType::Int64);
    const auto signal = makeSignal(sigDesc);
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
    std::atomic<bool> malformed{false};
    BackgroundLoop producerRole(
        [&, c = int64_t{0}]() mutable
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
            if (!lastValuesWellFormed(signal, 4000))
                malformed.store(true);
        });

    configRole.join();
    reader.join();
    producerRole.join();
    ASSERT_FALSE(malformed.load());

    // settled: caching off, last explicit value wins
    signal.asPtr<ISignalPrivate>(true).enableKeepLastValue(false);
    signal.setLastValue(12345);
    ASSERT_EQ(static_cast<Int>(signal.getLastValue()), 12345);
}

// Zero-sample data packets (staging skips them, the queue still delivers) and event packets sent
// through sendPacket (which bypass staging entirely), plain and under connect/disconnect churn.
TEST_F(DataPathTest, EmptyAndEventPacketsThroughSendPath)
{
    const auto sigDesc = plainDescriptor(SampleType::Int64);
    const auto signal = makeSignal(sigDesc);

    {
        auto ep = attach(signal);
        ep.connection.dequeueAll();  // clear the connect descriptor event

        signal.sendPacket(DataPacket(sigDesc, 0));                              // zero-sample: not staged
        signal.sendPacket(DataDescriptorChangedEventPacket(sigDesc, nullptr));  // event: bypasses staging

        const auto all = ep.connection.dequeueAll();
        ASSERT_EQ(all.getCount(), 2u);
        ASSERT_TRUE(all[0].supportsInterface<IDataPacket>());
        ASSERT_EQ(all[0].asPtr<IDataPacket>(true).getSampleCount(), 0u);
        ASSERT_TRUE(all[1].supportsInterface<IEventPacket>());
        ASSERT_FALSE(signal.getLastValue().assigned());  // nothing was staged
        ep.port.disconnect();
    }

    // churn variant: interleave empty / normal / event packets under connect-disconnect
    BackgroundLoop producer(
        [&, i = 0]() mutable
        {
            switch (i % 3)
            {
                case 0: signal.sendPacket(DataPacket(sigDesc, 0)); break;
                case 1: signal.sendPacket(DataPacket(sigDesc, 8)); break;
                case 2: signal.sendPacket(DataDescriptorChangedEventPacket(sigDesc, nullptr)); break;
            }
            ++i;
        });

    for (int round = 0; round < 100; ++round)
    {
        auto ep = attach(signal);
        ep.connection.dequeueAll();
        ep.port.disconnect();
    }

    producer.join();
}
