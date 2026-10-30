#include "multi_reader2_harness.h"

#include <gtest/gtest-spi.h>

#include <chrono>
#include <cstring>
#include <thread>

namespace daq::mr2test
{

// Checks of the harness itself: what it writes is what the matrix says it writes
class MultiReader2Harness : public MultiReader2Test
{
protected:
    // A packet reader on the signal an input carries, past its initial descriptor event
    PacketReaderPtr probe(Input in)
    {
        auto probeReader = PacketReader(signalOf(in));
        const auto first = probeReader.read();
        EXPECT_EQ(first.getType(), PacketType::Event);
        return probeReader;
    }

    static std::vector<int64_t> ticksOf(const DataPacketPtr& packet)
    {
        std::vector<int64_t> ticks;
        const auto* data = static_cast<int64_t*>(packet.getDomainPacket().getData());
        for (SizeT i = 0; i < packet.getSampleCount(); ++i)
            ticks.push_back(data[i]);
        return ticks;
    }
};

TEST_F(MultiReader2Harness, FactoriesCreateParamsWithSpecDefaultsAndReader)
{
    const auto params = MultiReader2Params();
    EXPECT_EQ(params.getInputs().getCount(), 0u);
    EXPECT_FALSE(params.getMainInput().assigned());
    EXPECT_FALSE(params.getAcceptsDescriptor().assigned());
    EXPECT_EQ(params.getValueReadType(), SampleType::Float64);
    EXPECT_EQ(params.getMinReadCount(), 1u);
    EXPECT_EQ(params.getErrorPolicy(), MultiReader2ErrorPolicy::Invalidate);
    EXPECT_TRUE(MultiReader2(params).assigned());
}

TEST_F(MultiReader2Harness, WriteSplitsMainTicksIntoPacketsOfTenCarryingTheTick)
{
    reader();
    auto p = probe(in1);
    write(in1, 0, 24);
    std::vector<SizeT> counts;
    for (DataPacketPtr packet = p.read(); packet.assigned(); packet = p.read())
    {
        counts.push_back(packet.getSampleCount());
        const auto ticks = ticksOf(packet);
        const auto* values = static_cast<double*>(packet.getData());
        for (SizeT i = 0; i < ticks.size(); ++i)
            EXPECT_EQ(values[i], static_cast<double>(ticks[i]));
    }
    EXPECT_EQ(counts, (std::vector<SizeT>{10, 10, 5}));
}

TEST_F(MultiReader2Harness, WriteConvertsMainTicksIntoTheInputsOwnDomain)
{
    given().signal(in2, sig::fine()).signal(in3, sig::epochNs()).inputs(3);
    reader();
    auto fine = probe(in2);
    auto epoch = probe(in3);
    write(in2, 100, 104);
    write(in3, 0, 2);
    const DataPacketPtr finePacket = fine.read();
    EXPECT_EQ(ticksOf(finePacket), (std::vector<int64_t>{200, 202, 204, 206, 208}));
    const DataPacketPtr epochPacket = epoch.read();
    EXPECT_EQ(ticksOf(epochPacket), (std::vector<int64_t>{1767225600000000000, 1767225600001000000, 1767225600002000000}));
}

TEST_F(MultiReader2Harness, OnePerPacketWritesEverySampleInItsOwnPacket)
{
    given().onePerPacket();
    reader();
    auto p = probe(in2);
    write(in2, 0, 2);
    std::vector<SizeT> counts;
    for (DataPacketPtr packet = p.read(); packet.assigned(); packet = p.read())
        counts.push_back(packet.getSampleCount());
    EXPECT_EQ(counts, (std::vector<SizeT>{1, 1, 1}));
}

TEST_F(MultiReader2Harness, DifferingPacketsWriteTenOnTheMainInputAndSevenOnTheOthers)
{
    given().differingPackets();
    reader();
    auto main = probe(in1);
    auto other = probe(in2);
    write(in1, 0, 13);
    write(in2, 0, 13);
    EXPECT_EQ(DataPacketPtr(main.read()).getSampleCount(), 10u);
    EXPECT_EQ(DataPacketPtr(other.read()).getSampleCount(), 7u);
}

TEST_F(MultiReader2Harness, AnnounceSendsOneEventWithTheNamedDescriptors)
{
    reader();
    auto p = probe(in2);
    announce(in2, Value(SampleType::Int32), Domain().start(3));
    const EventPacketPtr event = p.read();
    ASSERT_TRUE(event.assigned());
    EXPECT_EQ(event.getEventId(), event_packet_id::DATA_DESCRIPTOR_CHANGED);
    const DataDescriptorPtr value = event.getParameters().get(event_packet_param::DATA_DESCRIPTOR);
    const DataDescriptorPtr domain = event.getParameters().get(event_packet_param::DOMAIN_DATA_DESCRIPTOR);
    EXPECT_EQ(value.getSampleType(), SampleType::Int32);
    EXPECT_EQ(static_cast<int64_t>(domain.getRule().getParameters().get("start")), 3);

    announce(in2, Value::null());
    const EventPacketPtr nullEvent = p.read();
    const DataDescriptorPtr nullValue = nullEvent.getParameters().get(event_packet_param::DATA_DESCRIPTOR);
    EXPECT_EQ(nullValue.getSampleType(), SampleType::Null);
    EXPECT_FALSE(nullEvent.getParameters().get(event_packet_param::DOMAIN_DATA_DESCRIPTOR).assigned());
}

TEST_F(MultiReader2Harness, ScaledValuesCarryTheRawTick)
{
    given().signal(in2, sig::scaled());
    reader();
    auto p = probe(in2);
    write(in2, 5, 6);
    const DataPacketPtr packet = p.read();
    const auto* raw = static_cast<int32_t*>(packet.getRawData());
    EXPECT_EQ(raw[0], 5);
    EXPECT_EQ(raw[1], 6);
}

TEST_F(MultiReader2Harness, PortsHeldByAnotherListenerReportIt)
{
    given().listenerBlock(in3, PacketReadyNotification::SameThread).heldByStreamReader(in4);
    reader();
    expectPort(in3, Listener::Block, PacketReadyNotification::SameThread);
    expectPort(in4, Listener::Other);
}

TEST_F(MultiReader2Harness, ErrorSinkCountsErrorEntriesOfTheReaderOnly)
{
    reader();
    // Logged first, so the sink has seen it once the reader's two errors are counted
    context.getLogger().getOrAddComponent("MR2Harness").logMessage(SourceLocation{}, "another component's error", LogLevel::Error);
    auto component = context.getLogger().getOrAddComponent("MultiReader2");
    component.logMessage(SourceLocation{}, "an error", LogLevel::Error);
    component.logMessage(SourceLocation{}, "a warning", LogLevel::Warn);
    component.logMessage(SourceLocation{}, "another error", LogLevel::Error);
    idle();
    expectLoggedErrors(2);
}

TEST_F(MultiReader2Harness, ConcurrentScriptWritesEveryInputOnItsOwnThread)
{
    given().inputs(3);
    reader();
    std::vector<PacketReaderPtr> probes{probe(in1), probe(in2), probe(in3)};
    std::mutex mutex;
    std::set<std::thread::id> threads;
    Script s = script(Timeline::Base);
    for (auto& step : s)
        step.send = [&, send = step.send]
        {
            {
                std::scoped_lock lock(mutex);
                threads.insert(std::this_thread::get_id());
            }
            send();
        };
    runScript(s, true);
    EXPECT_EQ(threads.size(), 3u);
    EXPECT_EQ(threads.count(std::this_thread::get_id()), 0u);
    for (auto& p : probes)
    {
        SizeT samples = 0;
        for (DataPacketPtr packet = p.read(); packet.assigned(); packet = p.read())
            samples += packet.getSampleCount();
        EXPECT_EQ(samples, 100u);
    }
}

TEST_F(MultiReader2Harness, BlockedWorkerQueuesWorkUntilUnblocked)
{
    reader();
    std::atomic<int> runs{0};
    blockWorker();
    scheduler.scheduleFunction(Function(
        [&]() -> BaseObjectPtr
        {
            ++runs;
            return nullptr;
        }));
    std::this_thread::sleep_for(std::chrono::milliseconds(50));  // time a worker that is not held would take to run it
    EXPECT_EQ(runs, 0);
    unblockWorker();
    EXPECT_EQ(runs, 1);
}

// The openDAQ scheduler would run work scheduled from the worker (3) before older queued work (2)
TEST_F(MultiReader2Harness, WorkerRunsWorkInTheOrderItWasScheduled)
{
    reader();
    std::vector<int> order;  // only the one worker touches it until waitAll returns
    blockWorker();
    scheduler.scheduleFunction(Function(
        [&]() -> BaseObjectPtr
        {
            order.push_back(1);
            scheduler.scheduleFunction(Function(
                [&]() -> BaseObjectPtr
                {
                    order.push_back(3);
                    return nullptr;
                }));
            return nullptr;
        }));
    scheduler.scheduleFunction(Function(
        [&]() -> BaseObjectPtr
        {
            order.push_back(2);
            return nullptr;
        }));
    unblockWorker();
    EXPECT_EQ(order, (std::vector<int>{1, 2, 3}));
}

// Each check must be able to fail: a wrong result passed to expect raises exactly the failure it is built for
TEST_F(MultiReader2Harness, BufferAndStatusChecksFailOnAWrongResult)
{
    setupBase();
    const Result r = read(100);
    expect(r, count(100).at(0).noChanges());

    Result overrun = r;
    overrun.buffers[1][100 * sizeof(double)] = 0;  // the first byte past in2's samples
    EXPECT_NONFATAL_FAILURE(expect(overrun, count(100).at(0).noChanges()), "R-BUF-1");

    Result wrong = r;
    const double seven = 7;
    std::memcpy(wrong.buffers[0].data(), &seven, sizeof seven);
    EXPECT_NONFATAL_FAILURE(expect(wrong, count(100).at(0).noChanges()), "in1 sample 0");

    EXPECT_NONFATAL_FAILURE(expect(r, count(100).at(0).resync().noChanges()), "resynchronized");
}

}
