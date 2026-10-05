/*
 * MultiReader2 ports of the MultiReaderTest suite (test_multi_reader.cpp). Test names mirror the originals so the
 * mapping stays obvious; the cases the new reader cannot host are catalogued in test_multi_reader2_unsupported.cpp.
 *
 * Differences from the originals that every port shares:
 * - origins sit on full seconds (the old suite used a .123 s origin on the third signal),
 * - connection edges and descriptor events evaluate in a read, so a status probe follows every connect,
 * - the old event handshake (read(nullptr, &count) returning Event) is the first status with hasChanges.
 */
#include "test_multi_reader2_common.h"

#include <future>

using namespace daq;
using namespace testing;

class MultiReader2MigrationTest : public MultiReader2Test
{
public:
    static constexpr const char* EPOCH_03 = "2022-09-27T00:02:03+00:00";
    static constexpr const char* EPOCH_04 = "2022-09-27T00:02:04+00:00";
    static constexpr const char* EPOCH_05 = "2022-09-27T00:02:05+00:00";

    // The three classic signals: 523, 732 and 843 samples per packet at 1 kHz, origins one second apart
    void addClassicSignals(Int offset0 = 0, Int offset1 = 0, Int offset2 = 0, const char* epoch1 = EPOCH_04, const char* epoch2 = EPOCH_03)
    {
        readSignals.reserve(3);
        addSignal(offset0, 523, createDomainSignal(EPOCH_03));
        addSignal(offset1, 732, createDomainSignal(epoch1));
        addSignal(offset2, 843, createDomainSignal(epoch2));
    }

    template <SizeT Inputs, SizeT Samples>
    MultiReader2StatusPtr readWithDomain(const MultiReader2Ptr& reader,
                                         std::array<std::int64_t, Samples>& ticks,
                                         std::array<std::array<double, Samples>, Inputs>& values,
                                         SizeT& count)
    {
        void* buffers[Inputs + 1];
        buffers[0] = ticks.data();
        for (SizeT i = 0; i < Inputs; i++)
            buffers[i + 1] = values[i].data();
        return reader.readWithDomain(buffers, &count);
    }
};

// --- Alignment on origins and packet offsets -------------------------------------------------------------

TEST_F(MultiReader2MigrationTest, SignalStartDomainFrom0)
{
    addClassicSignals();
    auto reader = createReader(params(signalsToList()));
    probe(reader);
    ASSERT_EQ(reader.getAvailableCount(), 0u);

    for (Int i = 0; i < 3; i++)
        sendPackets(i);

    // Signal 1 starts one second later, at main tick 1000; signal 0 ends at tick 1568
    ASSERT_EQ(reader.getAvailableCount(), 569u);

    std::array<std::int64_t, 5> ticks{};
    std::array<std::array<double, 5>, 3> values{};
    SizeT count = 5;
    readWithDomain(reader, ticks, values, count);
    ASSERT_EQ(count, 5u);
    ASSERT_THAT(ticks, ElementsAre(1000, 1001, 1002, 1003, 1004));
    ASSERT_THAT(values[0], ElementsAre(1000, 1001, 1002, 1003, 1004));
    ASSERT_THAT(values[1], ElementsAre(0, 1, 2, 3, 4));
    ASSERT_THAT(values[2], ElementsAre(1000, 1001, 1002, 1003, 1004));
}

TEST_F(MultiReader2MigrationTest, SignalStartDomainFrom0SkipSamples)
{
    addClassicSignals();
    auto reader = createReader(params(signalsToList()));
    probe(reader);
    for (Int i = 0; i < 3; i++)
        sendPackets(i);
    ASSERT_EQ(reader.getAvailableCount(), 569u);

    SizeT count = 100;
    reader.skipSamples(&count);
    ASSERT_EQ(count, 100u);
    ASSERT_EQ(reader.getAvailableCount(), 469u);

    std::array<std::int64_t, 5> ticks{};
    std::array<std::array<double, 5>, 3> values{};
    count = 5;
    readWithDomain(reader, ticks, values, count);
    ASSERT_THAT(ticks, ElementsAre(1100, 1101, 1102, 1103, 1104));
    ASSERT_THAT(values[1], ElementsAre(100, 101, 102, 103, 104));
}

TEST_F(MultiReader2MigrationTest, SignalStartRelativeOffset0)
{
    readSignals.reserve(3);
    addSignal(0, 523, createDomainSignal(""));
    addSignal(0, 732, createDomainSignal(""));
    addSignal(0, 843, createDomainSignal(""));
    // Blank origins need no conversion; the fixture fills a default one, so clear them
    for (auto& read : readSignals)
        read.signal.getDomainSignal().asPtr<ISignalConfig>().setDescriptor(
            DataDescriptorBuilderCopy(read.domainDescriptor()).setOrigin("").build());
    auto reader = createReader(params(signalsToList()));
    probe(reader);

    for (Int i = 0; i < 3; i++)
        sendPackets(i);
    ASSERT_EQ(reader.getAvailableCount(), 1569u);

    std::array<std::int64_t, 5> ticks{};
    std::array<std::array<double, 5>, 3> values{};
    SizeT count = 5;
    readWithDomain(reader, ticks, values, count);
    ASSERT_EQ(count, 5u);
    ASSERT_THAT(values[0], ElementsAreArray(values[1]));
    ASSERT_THAT(values[0], ElementsAreArray(values[2]));
}

TEST_F(MultiReader2MigrationTest, WithPacketOffsetNot0)
{
    addClassicSignals(123, 134, 111);
    auto reader = createReader(params(signalsToList()));
    probe(reader);
    for (Int i = 0; i < 3; i++)
        sendPackets(i);

    // Firsts at 123, 1134 and 111 main ticks; signal 0 ends at 123 + 1569
    ASSERT_EQ(reader.getAvailableCount(), 558u);

    std::array<std::int64_t, 5> ticks{};
    std::array<std::array<double, 5>, 3> values{};
    SizeT count = 5;
    readWithDomain(reader, ticks, values, count);
    ASSERT_THAT(ticks, ElementsAre(1134, 1135, 1136, 1137, 1138));
    ASSERT_THAT(values[0], ElementsAre(1134, 1135, 1136, 1137, 1138));
    ASSERT_THAT(values[1], ElementsAre(134, 135, 136, 137, 138));
    ASSERT_THAT(values[2], ElementsAre(1134, 1135, 1136, 1137, 1138));
}

TEST_F(MultiReader2MigrationTest, WithPacketOffsetNot0Relative)
{
    addClassicSignals(123, 134, 111, EPOCH_03, EPOCH_03);
    auto reader = createReader(params(signalsToList()));
    probe(reader);
    for (Int i = 0; i < 3; i++)
        sendPackets(i);

    // Samples needed to sync signal 0: 134 - 123 = 11
    ASSERT_EQ(reader.getAvailableCount(), 523u * 3 - 11);
}

TEST_F(MultiReader2MigrationTest, MaxTimeIsNotOnSignalWithMaxEpoch)
{
    addClassicSignals(1240, 134, 111);
    auto reader = createReader(params(signalsToList()));
    probe(reader);
    for (Int i = 0; i < 3; i++)
        sendPackets(i);

    // The latest start is signal 0 at 1240, not the signal with the latest origin; signal 2 ends at 111 + 2529
    ASSERT_EQ(reader.getAvailableCount(), 1400u);
    std::array<std::int64_t, 5> ticks{};
    std::array<std::array<double, 5>, 3> values{};
    SizeT count = 5;
    readWithDomain(reader, ticks, values, count);
    ASSERT_THAT(ticks, ElementsAre(1240, 1241, 1242, 1243, 1244));
    ASSERT_THAT(values[1], ElementsAre(240, 241, 242, 243, 244));
}

// --- Clocks at equal rates in different tick units --------------------------------------------------------

TEST_F(MultiReader2MigrationTest, Clock10kHzDelta10)
{
    readSignals.reserve(3);
    addSignal(0, 10, createDomainSignal(EPOCH_03));
    addSignal(0, 10, createDomainSignal(EPOCH_03, Ratio(1, 10000), LinearDataRule(10, 0)));
    addSignal(0, 10, createDomainSignal(EPOCH_03));
    auto reader = createReader(params(signalsToList()));
    probe(reader);
    sendPackets(0);
    ASSERT_EQ(reader.getAvailableCount(), 10u);

    std::array<std::int64_t, 5> ticks{};
    std::array<std::array<double, 5>, 3> values{};
    SizeT count = 5;
    readWithDomain(reader, ticks, values, count);
    ASSERT_THAT(ticks, ElementsAre(0, 1, 2, 3, 4));
    ASSERT_THAT(values[1], ElementsAre(0, 10, 20, 30, 40));
}

TEST_F(MultiReader2MigrationTest, Clock10kHzDelta10WithAlignedOffset)
{
    readSignals.reserve(3);
    addSignal(0, 10, createDomainSignal(EPOCH_03));
    addSignal(50, 10, createDomainSignal(EPOCH_03, Ratio(1, 10000), LinearDataRule(10, 0)));  // 5 main ticks in
    addSignal(0, 10, createDomainSignal(EPOCH_03));
    auto reader = createReader(params(signalsToList()));
    probe(reader);
    sendPackets(0);
    ASSERT_EQ(reader.getAvailableCount(), 5u);

    std::array<std::int64_t, 5> ticks{};
    std::array<std::array<double, 5>, 3> values{};
    SizeT count = 5;
    readWithDomain(reader, ticks, values, count);
    ASSERT_THAT(ticks, ElementsAre(5, 6, 7, 8, 9));
    ASSERT_THAT(values[0], ElementsAre(5, 6, 7, 8, 9));
    ASSERT_THAT(values[1], ElementsAre(50, 60, 70, 80, 90));
}

TEST_F(MultiReader2MigrationTest, Clock10kHzDelta10WithIntersampleOffsetIsOffGrid)
{
    readSignals.reserve(2);
    addSignal(0, 10, createDomainSignal(EPOCH_03));
    addSignal(55, 10, createDomainSignal(EPOCH_03, Ratio(1, 10000), LinearDataRule(10, 0)));  // half a main tick in
    auto reader = createReader(params(signalsToList()));
    probe(reader);
    sendPackets(0);
    ASSERT_EQ(reader.getAvailableCount(), 0u);
    auto status = probe(reader);
    ASSERT_FALSE(status.getValid());
    ASSERT_EQ(input(status, 1).getError(), MultiReader2InputError::DomainDescriptorInvalid);
}

// --- Descriptor changes -----------------------------------------------------------------------------------

TEST_F(MultiReader2MigrationTest, EpochChanged)
{
    addClassicSignals();
    auto reader = createReader(params(signalsToList()));
    probe(reader);
    sendPackets(0);

    // Signal 1 moves to a clock one second further on; the data before the change is delivered first
    readSignals[1].signal.getDomainSignal().asPtr<ISignalConfig>().setDescriptor(createDomainDescriptor(EPOCH_05));
    sendPackets(1);
    sendPackets(2);

    // Before the change: common start 1000, signal 1's first packet ends at 1000 + 732
    ASSERT_EQ(reader.getAvailableCount(), 569u);
    std::array<std::array<double, 800>, 3> values{};
    SizeT count = 800;
    SizeT offset = 0;
    auto status = read(reader, values, count, &offset);
    ASSERT_EQ(count, 569u);
    ASSERT_EQ(offset, 1000u);
    ASSERT_FALSE(status.getHasChanges());

    // The boundary sits behind 163 undelivered samples of signal 1, so it waits for signal 0 to catch up
    count = 800;
    status = read(reader, values, count, &offset);
    ASSERT_EQ(count, 0u);
    ASSERT_FALSE(status.getHasChanges());

    // The read that reaches it delivers those 163 samples and resynchronizes: signal 1 tick 732 is now main tick 2732
    readSignals[0].sendAt(1569, 200);
    scheduler.waitAll();
    count = 800;
    status = read(reader, values, count, &offset);
    ASSERT_EQ(count, 163u);
    ASSERT_EQ(offset, 1569u);
    ASSERT_TRUE(status.getHasChanges());
    ASSERT_TRUE(status.getResynchronized());
    ASSERT_TRUE(status.getValid());
    ASSERT_EQ(reader.getAvailableCount(), 0u);  // signal 0 has no data at 2732 yet
}

TEST_F(MultiReader2MigrationTest, EpochChangedBeforeFirstData)
{
    addClassicSignals();
    auto reader = createReader(params(signalsToList()));
    probe(reader);
    readSignals[1].signal.getDomainSignal().asPtr<ISignalConfig>().setDescriptor(createDomainDescriptor(EPOCH_05));
    scheduler.waitAll();

    auto status = probe(reader);
    ASSERT_TRUE(status.getHasChanges());
    ASSERT_TRUE(status.getResynchronized());
    ASSERT_FALSE(status.getDomainDescriptorChanged());

    for (Int i = 0; i < 3; i++)
        sendPackets(i);
    // Signal 1 now starts at main tick 2000; signal 0 ends at 1568, so nothing aligns until it catches up
    ASSERT_EQ(reader.getAvailableCount(), 0u);
    readSignals[0].sendAt(1569, 1000);
    scheduler.waitAll();
    ASSERT_EQ(reader.getAvailableCount(), 529u);
}

TEST_F(MultiReader2MigrationTest, Signal2Invalidated)
{
    readSignals.reserve(3);
    auto domain = createDomainSignal(EPOCH_03);
    addSignal(0, 523, domain);
    addSignal(0, 732, domain);
    addSignal(0, 843, domain);
    auto reader = createReader(params(signalsToList()));
    probe(reader);
    sendPackets(0);
    readSignals[2].signal.setDescriptor(setupDescriptor(SampleType::ComplexFloat64));
    readSignals[0].createAndSendPacket(1);
    readSignals[1].createAndSendPacket(1);
    readSignals[2].createAndSendPacket<ComplexFloat64>(1);
    scheduler.waitAll();

    // One packet of signal 2 is readable in front of the change
    ASSERT_EQ(reader.getAvailableCount(), 843u);
    std::array<std::array<double, 844>, 3> values{};
    SizeT count = 844;
    auto status = read(reader, values, count);
    ASSERT_EQ(count, 843u);
    ASSERT_TRUE(status.getHasChanges());
    ASSERT_FALSE(status.getValid());
    ASSERT_EQ(input(status, 2).getError(), MultiReader2InputError::ValueDescriptorInvalid);
    ASSERT_EQ(reader.getAvailableCount(), 0u);
}

TEST_F(MultiReader2MigrationTest, SampleRateChanged)
{
    readSignals.reserve(2);
    addSignal(0, 100, createDomainSignal(EPOCH_03));
    addSignal(0, 100, createDomainSignal(EPOCH_03));
    auto reader = createReader(params(signalsToList()));
    probe(reader);
    sendPackets(0);
    readSignals[1].signal.getDomainSignal().asPtr<ISignalConfig>().setDescriptor(
        createDomainDescriptor(EPOCH_03, Ratio(1, 1000), LinearDataRule(2, 0)));
    scheduler.waitAll();

    std::array<std::array<double, 100>, 2> values{};
    SizeT count = 100;
    auto status = read(reader, values, count);
    ASSERT_EQ(count, 100u);
    ASSERT_FALSE(status.getValid());
    ASSERT_EQ(input(status, 1).getError(), MultiReader2InputError::DomainDescriptorInvalid);

    // Back at the main rate the reader recovers
    readSignals[1].signal.getDomainSignal().asPtr<ISignalConfig>().setDescriptor(createDomainDescriptor(EPOCH_03));
    scheduler.waitAll();
    status = probe(reader);
    ASSERT_TRUE(status.getValid());
}

TEST_F(MultiReader2MigrationTest, SharedDomainDescriptorChangeKeepsTheReaderValid)
{
    auto domainSignal = Signal(context, nullptr, "shared_domain");
    domainSignal.setDescriptor(createDomainDescriptor(EPOCH_03, Ratio(1, 1000000), LinearDataRule(1000, 0)));
    readSignals.reserve(2);
    addSignal(12345000, 50, domainSignal, SampleType::Float32);
    addSignal(12345000, 40, domainSignal, SampleType::Float32);
    auto reader = createReader(params(portsList(), SampleType::Float32));
    connectAll(reader);

    std::atomic<int> wakes{0};
    reader.getOnDataAvailable() += [&](InputPortPtr&, EventArgsPtr<>&) { wakes++; };

    // Both inputs move to the new rate together: the main domain changes, the other input follows, nothing is invalid
    domainSignal.setDescriptor(createDomainDescriptor(EPOCH_03, Ratio(1, 1000000), LinearDataRule(100, 0)));
    scheduler.waitAll();
    ASSERT_GE(wakes.load(), 1);
    auto status = probe(reader);
    ASSERT_TRUE(status.getHasChanges());
    ASSERT_TRUE(status.getValid());
    ASSERT_TRUE(status.getDomainDescriptorChanged());
    ASSERT_EQ(status.getDomainDescriptor().getRule().getParameters().get("delta"), 100);
}

// --- Input ports ------------------------------------------------------------------------------------------

TEST_F(MultiReader2MigrationTest, MultiReaderWithInputPort)
{
    readSignals.reserve(3);
    addSignal(0, 523, createDomainSignal(EPOCH_03));
    addSignal(0, 732, createDomainSignal(EPOCH_04, Ratio(1, 10000), LinearDataRule(10, 0)));
    addSignal(0, 843, createDomainSignal(EPOCH_03));
    auto reader = createReader(params(portsList()));
    connectAll(reader);
    ASSERT_EQ(reader.getAvailableCount(), 0u);

    for (Int i = 0; i < 3; i++)
        sendPackets(i);
    ASSERT_EQ(reader.getAvailableCount(), 569u);
    ASSERT_EQ(reader.getAvailableCount(), 569u);

    std::array<std::int64_t, 5> ticks{};
    std::array<std::array<double, 5>, 3> values{};
    SizeT count = 5;
    readWithDomain(reader, ticks, values, count);
    ASSERT_EQ(count, 5u);
    ASSERT_THAT(ticks, ElementsAre(1000, 1001, 1002, 1003, 1004));
    ASSERT_THAT(values[1], ElementsAre(0, 10, 20, 30, 40));
}

TEST_F(MultiReader2MigrationTest, MultiReaderWithNotConnectedInputPort)
{
    addClassicSignals();
    auto reader = createReader(params(portsList()));
    ASSERT_EQ(reader.getAvailableCount(), 0u);
    auto status = probe(reader);
    ASSERT_FALSE(status.getValid());

    connectAll(reader);
    for (Int i = 0; i < 3; i++)
        sendPackets(i);
    ASSERT_EQ(reader.getAvailableCount(), 569u);
}

TEST_F(MultiReader2MigrationTest, MultiReaderWithDifferentInputs)
{
    addClassicSignals();
    portsList();
    MultiReader2ParamsPtr p = createWithImplementation<IMultiReader2Params, MultiReader2ParamsImpl>();
    ASSERT_EQ(p->setInputs(List<IComponent>(ports[0], ports[1], readSignals[2].signal)), OPENDAQ_ERR_INVALIDPARAMETER);
    daqClearErrorInfo();
}

TEST_F(MultiReader2MigrationTest, MultiReaderReuseInputPort)
{
    readSignals.reserve(1);
    addSignal(0, 523, createDomainSignal(EPOCH_03));
    auto list = portsList();
    {
        auto reader = createReader(params(list));
    }
    auto reader = createReader(params(list));
    connectAll(reader);
    sendPackets(0);
    ASSERT_EQ(reader.getAvailableCount(), 523u);
}

TEST_F(MultiReader2MigrationTest, ReadWhenOnePortIsNotConnected)
{
    readSignals.reserve(3);
    auto domain = createDomainSignal(EPOCH_03);
    addSignal(0, 20, domain);
    addSignal(0, 30, domain);
    addSignal(0, 40, domain);
    auto reader = createReader(params(portsList()));
    ports[0].connect(readSignals[0].signal);
    ports[1].connect(readSignals[1].signal);
    scheduler.waitAll();
    readSignals[0].createAndSendPacket(0);
    readSignals[1].createAndSendPacket(0);
    scheduler.waitAll();

    // Nothing is read while a used port has no signal
    ASSERT_EQ(reader.getAvailableCount(), 0u);
    std::array<std::array<double, 10>, 3> values{};
    SizeT count = 10;
    auto status = read(reader, values, count);
    ASSERT_EQ(count, 0u);
    ASSERT_FALSE(status.getValid());
    ASSERT_EQ(input(status, 2).getError(), MultiReader2InputError::Disconnected);

    ports[2].connect(readSignals[2].signal);
    scheduler.waitAll();
    readSignals[2].createAndSendPacket(0);
    scheduler.waitAll();
    status = probe(reader);
    ASSERT_TRUE(status.getValid());
    ASSERT_TRUE(status.getHasChanges());

    // The data from before the recovery is gone; new data on a common range aligns
    count = 10;
    status = read(reader, values, count);
    ASSERT_EQ(count, 0u);
    for (auto& read : readSignals)
        read.sendAt(100, 10);
    scheduler.waitAll();
    count = 10;
    status = read(reader, values, count);
    ASSERT_EQ(count, 10u);
}

TEST_F(MultiReader2MigrationTest, NotifyPortIsConnected)
{
    readSignals.reserve(3);
    auto domain = createDomainSignal(EPOCH_03);
    addSignal(0, 20, domain);
    addSignal(0, 30, domain);
    addSignal(0, 40, domain);
    auto reader = createReader(params(portsList()));
    ports[0].connect(readSignals[0].signal);
    ports[1].connect(readSignals[1].signal);
    scheduler.waitAll();
    probe(reader);

    std::promise<MultiReader2StatusPtr> promise;
    auto future = promise.get_future();
    reader.getOnDataAvailable() += [&](InputPortPtr&, EventArgsPtr<>&) { promise.set_value(probe(reader)); };

    ports[2].connect(readSignals[2].signal);
    ASSERT_EQ(future.wait_for(std::chrono::seconds(1)), std::future_status::ready);
    const auto status = future.get();
    ASSERT_TRUE(status.getHasChanges());
    ASSERT_TRUE(status.getValid());
    ASSERT_EQ(input(status, 2).getError(), MultiReader2InputError::None);
}

TEST_F(MultiReader2MigrationTest, ReconnectWhileReading)
{
    readSignals.reserve(3);
    auto domain = createDomainSignal(EPOCH_03);
    addSignal(0, 10, domain);
    addSignal(0, 20, domain);
    addSignal(0, 30, domain);
    auto reader = createReader(params(portsList()));
    connectAll(reader);
    sendPackets(0);

    std::array<std::array<double, 20>, 3> values{};
    SizeT count = 20;
    auto status = read(reader, values, count);
    ASSERT_EQ(count, 10u);

    ports[0].disconnect();
    scheduler.waitAll();
    ports[0].connect(readSignals[0].signal);
    scheduler.waitAll();

    // The disconnect and the reconnect were both queued with nothing between them: evaluated together they
    // change nothing, and the stream goes on where it was
    status = probe(reader);
    ASSERT_FALSE(status.getHasChanges());
    ASSERT_TRUE(status.getValid());
    sendPackets(1);
    count = 20;
    status = read(reader, values, count);
    ASSERT_EQ(count, 10u);
}

TEST_F(MultiReader2MigrationTest, TestReaderWithConnectedPortConnectionEmpty)
{
    readSignals.reserve(3);
    auto domain = createDomainSignal(EPOCH_03);
    addSignal(0, 20, domain);
    addSignal(0, 30, domain);
    addSignal(0, 40, domain);
    auto list = portsList();
    for (SizeT i = 0; i < 3; i++)
        ports[i].connect(readSignals[i].signal);
    scheduler.waitAll();

    // The connections hold their initial descriptor event; drain them before the reader takes the ports
    for (const auto& port : ports)
    {
        auto connection = port.getConnection();
        SizeT packets = 0;
        while (connection.dequeue().assigned())
            packets++;
        ASSERT_EQ(packets, 1u);
    }

    auto reader = createReader(params(list));
    auto status = probe(reader);
    ASSERT_TRUE(status.getValid());
    for (SizeT i = 0; i < 3; i++)
        ASSERT_TRUE(input(status, i).getDescriptor().assigned());

    sendPackets(0);
    ASSERT_EQ(reader.getAvailableCount(), 20u);
}

TEST_F(MultiReader2MigrationTest, TestReaderWithConnectedPortConnectionNotEmpty)
{
    readSignals.reserve(3);
    auto domain = createDomainSignal(EPOCH_03);
    addSignal(0, 20, domain);
    addSignal(0, 30, domain);
    addSignal(0, 40, domain);
    auto list = portsList();
    for (SizeT i = 0; i < 3; i++)
        ports[i].connect(readSignals[i].signal);
    sendPackets(0);

    // Data already waiting in the connections is picked up when the reader takes the ports
    auto reader = createReader(params(list));
    probe(reader);
    ASSERT_EQ(reader.getAvailableCount(), 20u);
}

// --- Active and used --------------------------------------------------------------------------------------

TEST_F(MultiReader2MigrationTest, MultiReaderActive)
{
    readSignals.reserve(3);
    addSignal(0, 10, createDomainSignal());
    addSignal(0, 10, createDomainSignal());
    addSignal(0, 10, createDomainSignal());
    auto p = params(portsList());
    auto reader = createReader(p);
    connectAll(reader);
    Int packetIndex = 0;

    sendPackets(packetIndex++);
    std::array<std::int64_t, 10> ticks{};
    std::array<std::array<double, 10>, 3> values{};
    SizeT count = 10;
    auto status = readWithDomain(reader, ticks, values, count);
    ASSERT_EQ(count, 10u);

    // Unused: data is dropped, the reader stays valid
    p.setUsed(false);
    reader.configure(p);
    sendPackets(packetIndex++);
    count = 10;
    status = readWithDomain(reader, ticks, values, count);
    ASSERT_EQ(count, 0u);
    ASSERT_TRUE(status.getValid());

    // A value descriptor change while unused is still reported
    readSignals[0].signal.setDescriptor(DataDescriptorBuilder().setSampleType(SampleType::Float64).setUnit(Unit("A")).build());
    sendPackets(packetIndex++);
    count = 10;
    status = readWithDomain(reader, ticks, values, count);
    ASSERT_EQ(count, 0u);
    ASSERT_TRUE(status.getHasChanges());
    ASSERT_TRUE(input(status, 0).getDescriptorChanged());

    // Used again: data flows
    p.setUsed(true);
    reader.configure(p);
    probe(reader);
    sendPackets(packetIndex++);
    count = 10;
    status = readWithDomain(reader, ticks, values, count);
    ASSERT_EQ(count, 10u);
}

TEST_F(MultiReader2MigrationTest, MultiReaderActiveGapPacket)
{
    readSignals.reserve(3);
    addSignal(0, 10, createDomainSignal());
    addSignal(0, 10, createDomainSignal());
    addSignal(0, 10, createDomainSignal());
    // Ports with gap checking: the connection turns a skipped packet into a gap event
    ports.clear();
    auto list = List<IComponent>();
    for (SizeT i = 0; i < 3; i++)
    {
        ports.push_back(InputPort(context, nullptr, fmt::format("gapport{}", i), true));
        list.pushBack(ports.back());
    }
    auto reader = createReader(params(list));
    connectAll(reader);

    readSignals[0].createAndSendPacket(0);
    readSignals[1].createAndSendPacket(0);
    readSignals[2].createAndSendPacket(0);
    readSignals[1].createAndSendPacket(1);
    readSignals[2].createAndSendPacket(1);
    for (auto& read : readSignals)
        read.createAndSendPacket(2);  // signal 0 skipped packet 1
    scheduler.waitAll();

    // Ten samples until the gap
    ASSERT_EQ(reader.getAvailableCount(), 10u);
    std::array<std::array<double, 10>, 3> values{};
    SizeT count = 10;
    auto status = read(reader, values, count);
    ASSERT_EQ(count, 10u);
    ASSERT_TRUE(status.getHasChanges());
    ASSERT_TRUE(status.getResynchronized());

    // Synchronization lands on the third packet
    ASSERT_EQ(reader.getAvailableCount(), 10u);
    count = 10;
    SizeT offset = 0;
    status = read(reader, values, count, &offset);
    ASSERT_EQ(count, 10u);
    ASSERT_EQ(offset, 20u);
}

TEST_F(MultiReader2MigrationTest, ReadSignalWithoutDomainDoesNotCrash)
{
    // A domain-type signal has no domain signal of its own
    auto timeDesc = DataDescriptorBuilder()
                        .setSampleType(SampleType::Int64)
                        .setRule(LinearDataRule(1, 0))
                        .setTickResolution(Ratio(1, 1000))
                        .setUnit(Unit("s", -1, "seconds", "time"))
                        .build();
    const auto timeSignal = SignalWithDescriptor(context, timeDesc, nullptr, "time");
    auto reader = createReader(params(List<IComponent>(timeSignal)));
    timeSignal.sendPacket(DataPacket(timeDesc, 100, 0));
    scheduler.waitAll();

    auto status = probe(reader);
    ASSERT_FALSE(status.getValid());
    ASSERT_EQ(input(status, 0).getError(), MultiReader2InputError::DomainDescriptorInvalid);
    ASSERT_EQ(reader.getAvailableCount(), 0u);
}

TEST_F(MultiReader2MigrationTest, MinReadCount)
{
    readSignals.reserve(3);
    auto domain = createDomainSignal(EPOCH_03);
    addSignal(0, 10, domain);
    addSignal(0, 10, domain);
    addSignal(0, 10, domain);
    auto p = params(signalsToList());
    p.setMinReadCount(20);
    auto reader = createReader(p);
    probe(reader);
    sendPackets(0);

    std::array<std::array<double, 20>, 3> values{};
    void* buffers[3]{values[0].data(), values[1].data(), values[2].data()};
    ASSERT_EQ(reader.getAvailableCount(), 0u);

    SizeT count = 10;
    SizeT offset = 0;
    ASSERT_THROW(reader.read(buffers, &count, offset), InvalidParameterException);
    count = 10;
    ASSERT_THROW(reader.skipSamples(&count), InvalidParameterException);

    sendPackets(1);
    ASSERT_EQ(reader.getAvailableCount(), 20u);
    count = 20;
    reader.read(buffers, &count, offset);
    ASSERT_EQ(count, 20u);

    sendPackets(2);
    ASSERT_EQ(reader.getAvailableCount(), 0u);

    // Ten samples sit in front of the descriptor change; they are dropped so the boundary can be consumed
    readSignals[0].signal.setDescriptor(setupDescriptor(SampleType::Int32));
    scheduler.waitAll();
    ASSERT_EQ(reader.getAvailableCount(), 0u);
    count = 20;
    auto status = reader.read(buffers, &count, offset);
    ASSERT_EQ(count, 0u);
    ASSERT_TRUE(status.getHasChanges());
    ASSERT_TRUE(input(status, 0).getDescriptorChanged());
    ASSERT_EQ(reader.getAvailableCount(), 0u);
}

TEST_F(MultiReader2MigrationTest, UndefinedValueType)
{
    readSignals.reserve(2);
    auto domain = createDomainSignal(EPOCH_03);
    addSignal(0, 10, domain, SampleType::Int64);
    addSignal(0, 10, domain, SampleType::Int64);
    auto reader = createReader(params(signalsToList(), SampleType::Undefined));
    probe(reader);
    readSignals[0].createAndSendPacket<int64_t>(0);
    readSignals[1].createAndSendPacket<int64_t>(0);
    readSignals[0].createAndSendPacket<int64_t>(1);
    readSignals[1].createAndSendPacket<int64_t>(1);
    scheduler.waitAll();
    ASSERT_EQ(reader.getAvailableCount(), 20u);

    std::array<int64_t, 10> ticks{};
    std::array<int64_t, 10> a{};
    std::array<int64_t, 10> b{};
    void* buffers[3]{ticks.data(), a.data(), b.data()};
    SizeT count = 10;
    reader.readWithDomain(buffers, &count);
    ASSERT_EQ(count, 10u);
    ASSERT_THAT(a, ElementsAreArray(ticks));
}

TEST_F(MultiReader2MigrationTest, AddRemoveInput)
{
    readSignals.reserve(3);
    auto resolution = Ratio(1, 1000);
    auto rule = LinearDataRule(4, 0);
    addSignal(0, 20, createDomainSignal(EPOCH_04, resolution, rule));
    addSignal(0, 20, createDomainSignal(EPOCH_04, resolution, rule));
    addSignal(0, 10, createDomainSignal(EPOCH_04, resolution, rule));
    portsList();
    auto p = params(List<IComponent>(ports[0], ports[1]));
    auto reader = createReader(p);
    ports[0].connect(readSignals[0].signal);
    ports[1].connect(readSignals[1].signal);
    scheduler.waitAll();
    auto status = probe(reader);
    ASSERT_TRUE(status.getValid());

    readSignals[0].createAndSendPacket(0);
    readSignals[1].createAndSendPacket(0);
    readSignals[2].createAndSendPacket(0);
    readSignals[2].createAndSendPacket(1);
    scheduler.waitAll();
    ASSERT_EQ(reader.getAvailableCount(), 20u);

    // Adding a port discards the queues; the unconnected port is an error until its signal arrives
    p.setInputs(List<IComponent>(ports[0], ports[1], ports[2]));
    reader.configure(p);
    status = probe(reader);
    ASSERT_FALSE(status.getValid());
    ports[2].connect(readSignals[2].signal);
    scheduler.waitAll();
    status = probe(reader);
    ASSERT_TRUE(status.getValid());
    ASSERT_EQ(reader.getAvailableCount(), 0u);

    readSignals[0].createAndSendPacket(1);
    readSignals[1].createAndSendPacket(1);
    readSignals[2].createAndSendPacket(2);
    scheduler.waitAll();
    ASSERT_EQ(reader.getAvailableCount(), 10u);  // ticks 80 to 120 are on every input

    std::array<std::array<double, 10>, 3> values{};
    SizeT count = 10;
    SizeT offset = 0;
    status = read(reader, values, count, &offset);
    ASSERT_EQ(count, 10u);
    ASSERT_EQ(offset, 80u);

    // Removing the short input: the other two go on alone
    p.setInputs(List<IComponent>(ports[0], ports[1]));
    reader.configure(p);
    probe(reader);
    readSignals[0].createAndSendPacket(2);
    readSignals[1].createAndSendPacket(2);
    scheduler.waitAll();
    ASSERT_EQ(reader.getAvailableCount(), 20u);
}

TEST_F(MultiReader2MigrationTest, UsedUnusedInput)
{
    readSignals.reserve(3);
    auto resolution = Ratio(1, 1000);
    auto rule = LinearDataRule(4, 0);
    addSignal(0, 20, createDomainSignal(EPOCH_04, resolution, rule));
    addSignal(0, 20, createDomainSignal(EPOCH_04, resolution, rule));
    addSignal(0, 10, createDomainSignal(EPOCH_04, resolution, rule));
    auto p = params(portsList());
    p.setInputUsed(ports[2], false);
    auto reader = createReader(p);
    ports[0].connect(readSignals[0].signal);
    ports[1].connect(readSignals[1].signal);
    scheduler.waitAll();
    auto status = probe(reader);
    ASSERT_TRUE(status.getValid());
    ASSERT_EQ(input(status, 2).getError(), MultiReader2InputError::Disconnected);

    readSignals[0].createAndSendPacket(0);
    readSignals[1].createAndSendPacket(0);
    readSignals[2].createAndSendPacket(0);
    scheduler.waitAll();
    ASSERT_EQ(reader.getAvailableCount(), 20u);  // the unused input gates nothing

    ports[2].connect(readSignals[2].signal);
    readSignals[2].createAndSendPacket(1);
    scheduler.waitAll();
    p.setInputUsed(ports[2], true);
    reader.configure(p);
    status = probe(reader);
    ASSERT_TRUE(status.getValid());
    ASSERT_TRUE(input(status, 2).getUsed());
    ASSERT_EQ(reader.getAvailableCount(), 0u);

    readSignals[0].createAndSendPacket(1);
    readSignals[1].createAndSendPacket(1);
    readSignals[2].createAndSendPacket(2);
    scheduler.waitAll();
    ASSERT_EQ(reader.getAvailableCount(), 10u);
}

TEST_F(MultiReader2MigrationTest, DisposeDisconnectsInternalPorts)
{
    readSignals.reserve(2);
    auto domain = createDomainSignal(EPOCH_03);
    addSignal(0, 10, domain);
    addSignal(0, 10, domain);
    {
        auto reader = createReader(params(signalsToList()));
        ASSERT_EQ(readSignals[0].signal.getConnections().getCount(), 1u);
    }
    ASSERT_EQ(readSignals[0].signal.getConnections().getCount(), 0u);
}

TEST_F(MultiReader2MigrationTest, MultiReaderGapDetection)
{
    readSignals.reserve(2);
    addSignal(0, 10, createDomainSignal(EPOCH_03, nullptr, LinearDataRule(1, 0)));
    addSignal(0, 20, createDomainSignal(EPOCH_03, nullptr, LinearDataRule(1, 0)));
    ports.clear();
    auto list = List<IComponent>();
    for (SizeT i = 0; i < 2; i++)
    {
        ports.push_back(InputPort(context, nullptr, fmt::format("gapport{}", i), true));
        list.pushBack(ports.back());
    }
    auto reader = createReader(params(list));
    connectAll(reader);

    // Signal 0 sends ten samples, skips five, sends ten more; signal 1 sends twenty
    readSignals[0].sendAt(0, 10);
    readSignals[0].sendAt(15, 10);
    readSignals[1].sendAt(0, 20);
    scheduler.waitAll();

    ASSERT_EQ(reader.getAvailableCount(), 10u);
    std::array<std::array<double, 10>, 2> values{};
    SizeT count = 10;
    auto status = read(reader, values, count);
    ASSERT_EQ(count, 10u);
    ASSERT_TRUE(status.getResynchronized());

    ASSERT_EQ(reader.getAvailableCount(), 5u);
    count = 10;
    SizeT offset = 0;
    status = read(reader, values, count, &offset);
    ASSERT_EQ(count, 5u);
    ASSERT_EQ(offset, 15u);
}
