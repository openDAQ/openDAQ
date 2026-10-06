/*
 * MultiReader2 over time: boundaries at every input position, the two error policies against one, several and all
 * inputs failing, the automatic and the pinned main input, synchronization limits, the min read count against
 * boundaries, the wake event and a producer thread.
 */
#include "test_multi_reader2_common.h"

#include <opendaq/event_packet_params.h>
#include <opendaq/packet_factory.h>

using namespace daq;
using namespace testing;

class MultiReader2LifecycleTest : public MultiReader2Test
{
public:
    // Three signals on one clock, ten samples per packet, over ports when asked
    MultiReader2Ptr three(MultiReader2ErrorPolicy policy, bool overPorts, SizeT minReadCount = 1)
    {
        readSignals.reserve(3);
        auto domain = createDomainSignal();
        for (int i = 0; i < 3; i++)
            addSignal(0, 10, domain);
        auto p = params(overPorts ? portsList() : signalsToList());
        p.setErrorPolicy(policy);
        p.setMinReadCount(minReadCount);
        auto reader = createReader(p);
        if (overPorts)
            connectAll(reader);
        else
        {
            scheduler.waitAll();
            probe(reader);
        }
        return reader;
    }

    std::vector<std::vector<double>> readAll(const MultiReader2Ptr& reader, SizeT inputs, SizeT& count, MultiReader2StatusPtr& statusOut)
    {
        std::vector<std::vector<double>> values(inputs, std::vector<double>(count, -1.0));
        std::vector<void*> buffers(inputs);
        for (SizeT i = 0; i < inputs; i++)
            buffers[i] = values[i].data();
        SizeT offset = 0;
        statusOut = reader.read(buffers.data(), &count, offset);
        return values;
    }

    std::vector<std::vector<double>> readAll(const MultiReader2Ptr& reader, SizeT inputs, SizeT& count)
    {
        MultiReader2StatusPtr status;
        return readAll(reader, inputs, count, status);
    }

    static DataDescriptorPtr voltDescriptor()
    {
        return DataDescriptorBuilder().setSampleType(SampleType::Float64).setUnit(Unit("V")).build();
    }
};

class MultiReader2PositionTest : public MultiReader2LifecycleTest, public WithParamInterface<int>
{
};

// A value descriptor change on the input at the position: the data before it flows, the change rides with it
TEST_P(MultiReader2PositionTest, ValueChange)
{
    const int at = GetParam();
    auto reader = three(MultiReader2ErrorPolicy::Invalidate, false);
    sendPackets(0);
    readSignals[at].signal.setDescriptor(voltDescriptor());
    sendPackets(1);

    SizeT count = 20;
    MultiReader2StatusPtr status;
    readAll(reader, 3, count, status);
    ASSERT_EQ(count, 10u);
    ASSERT_TRUE(status.getHasChanges());
    ASSERT_TRUE(status.getValid());
    ASSERT_FALSE(status.getResynchronized());
    ASSERT_FALSE(status.getDomainDescriptorChanged());
    for (int i = 0; i < 3; i++)
        ASSERT_EQ(input(status, i).getDescriptorChanged(), i == at) << i;
    ASSERT_EQ(input(status, at).getDescriptor().getUnit().getSymbol(), "V");

    count = 20;
    const auto values = readAll(reader, 3, count, status);
    ASSERT_EQ(count, 10u);
    ASSERT_FALSE(status.getHasChanges());
    for (int i = 0; i < 3; i++)
        ASSERT_EQ(values[i][0], 10.0) << i;
}

// A gap on the input at the position: the others wait, the stream resumes where every input has data
TEST_P(MultiReader2PositionTest, Gap)
{
    const int at = GetParam();
    auto reader = three(MultiReader2ErrorPolicy::Invalidate, false);
    sendPackets(0);
    for (int i = 0; i < 3; i++)
        readSignals[i].sendAt(i == at ? 13 : 10, 10);
    scheduler.waitAll();

    // The run up to the gap comes out, and the read that reaches the gap resynchronizes on the same status
    SizeT count = 20;
    MultiReader2StatusPtr status;
    readAll(reader, 3, count, status);
    ASSERT_EQ(count, 10u);
    ASSERT_TRUE(status.getHasChanges());
    ASSERT_TRUE(status.getResynchronized());
    ASSERT_TRUE(status.getValid());
    ASSERT_EQ(reader.getAvailableCount(), 7u);
    count = 20;
    const auto values = readAll(reader, 3, count, status);
    ASSERT_EQ(count, 7u);
    for (int i = 0; i < 3; i++)
        ASSERT_EQ(values[i][0], 13.0) << i;
}

// A disconnect at the position under Invalidate: the reader is invalid until the port reconnects
TEST_P(MultiReader2PositionTest, DisconnectInvalidates)
{
    const int at = GetParam();
    auto reader = three(MultiReader2ErrorPolicy::Invalidate, true);
    sendPackets(0);
    ports[at].disconnect();
    scheduler.waitAll();
    sendPackets(1);

    SizeT count = 20;
    MultiReader2StatusPtr status;
    readAll(reader, 3, count, status);
    ASSERT_EQ(count, 10u);
    ASSERT_FALSE(status.getValid());
    for (int i = 0; i < 3; i++)
        ASSERT_EQ(input(status, i).getError(), i == at ? MultiReader2InputError::Disconnected : MultiReader2InputError::None) << i;
    ASSERT_EQ(reader.getAvailableCount(), 0u);

    ports[at].connect(readSignals[at].signal);
    scheduler.waitAll();
    status = probe(reader);
    ASSERT_TRUE(status.getValid());
    ASSERT_TRUE(status.getResynchronized());
    ASSERT_EQ(reader.getMainInput(), ports[0].getGlobalId());
    sendPackets(2);
    ASSERT_EQ(reader.getAvailableCount(), 10u);
}

// A disconnect at the position under Exclude: the others go on; at position 0 the main moves
TEST_P(MultiReader2PositionTest, DisconnectExcludes)
{
    const int at = GetParam();
    auto reader = three(MultiReader2ErrorPolicy::Exclude, true);
    sendPackets(0);
    SizeT count = 20;
    MultiReader2StatusPtr status;
    readAll(reader, 3, count, status);
    ASSERT_EQ(count, 10u);

    ports[at].disconnect();
    scheduler.waitAll();
    status = probe(reader);
    ASSERT_TRUE(status.getValid());
    ASSERT_TRUE(status.getHasChanges());
    ASSERT_EQ(input(status, at).getError(), MultiReader2InputError::Disconnected);
    ASSERT_TRUE(input(status, at).getUsed());
    ASSERT_EQ(status.getDomainDescriptorChanged(), at == 0);
    ASSERT_EQ(reader.getMainInput(), ports[at == 0 ? 1 : 0].getGlobalId());

    sendPackets(1);
    count = 20;
    const auto values = readAll(reader, 3, count, status);
    ASSERT_EQ(count, 10u);
    for (int i = 0; i < 3; i++)
        ASSERT_EQ(values[i][0], i == at ? -1.0 : 10.0) << i;

    ports[at].connect(readSignals[at].signal);
    scheduler.waitAll();
    status = probe(reader);
    ASSERT_TRUE(status.getValid());
    ASSERT_EQ(input(status, at).getError(), MultiReader2InputError::None);
    ASSERT_TRUE(input(status, at).getDescriptorChanged());
    ASSERT_EQ(reader.getMainInput(), ports[0].getGlobalId());
}

// An invalid value descriptor at the position under Exclude, then its repair
TEST_P(MultiReader2PositionTest, InvalidValueExcludesAndRejoins)
{
    const int at = GetParam();
    auto reader = three(MultiReader2ErrorPolicy::Exclude, false);
    sendPackets(0);
    readSignals[at].signal.setDescriptor(setupDescriptor(SampleType::ComplexFloat32));
    scheduler.waitAll();

    SizeT count = 20;
    MultiReader2StatusPtr status;
    readAll(reader, 3, count, status);
    ASSERT_EQ(count, 10u);
    ASSERT_TRUE(status.getValid());
    ASSERT_EQ(input(status, at).getError(), MultiReader2InputError::ValueDescriptorInvalid);
    ASSERT_FALSE(input(status, at).getDescriptor().assigned());

    for (int i = 0; i < 3; i++)
        if (i != at)
            readSignals[i].createAndSendPacket(1);
    scheduler.waitAll();
    ASSERT_EQ(reader.getAvailableCount(), 10u);

    readSignals[at].signal.setDescriptor(setupDescriptor(SampleType::Float64));
    scheduler.waitAll();
    count = 20;
    readAll(reader, 3, count, status);
    ASSERT_EQ(count, 10u);  // the others' data in front of the rejoin
    ASSERT_TRUE(status.getHasChanges());
    ASSERT_EQ(input(status, at).getError(), MultiReader2InputError::None);
    ASSERT_TRUE(input(status, at).getDescriptorChanged());
    ASSERT_TRUE(status.getResynchronized());
    sendPackets(2);
    ASSERT_EQ(reader.getAvailableCount(), 10u);
}

INSTANTIATE_TEST_SUITE_P(Positions,
                         MultiReader2PositionTest,
                         Values(0, 1, 2),
                         [](const TestParamInfo<int>& info) { return "At" + std::to_string(info.param); });

// ---------------------------------------------------------------- exclude with several failures

TEST_F(MultiReader2LifecycleTest, ExcludeWithTwoOfFourFailingKeepsTheRestStreaming)
{
    readSignals.reserve(4);
    auto domain = createDomainSignal();
    for (int i = 0; i < 4; i++)
        addSignal(0, 10, domain);
    auto p = params(portsList());
    p.setErrorPolicy(MultiReader2ErrorPolicy::Exclude);
    auto reader = createReader(p);
    ports[1].connect(readSignals[1].signal);
    ports[3].connect(readSignals[3].signal);
    scheduler.waitAll();
    auto status = probe(reader);
    ASSERT_TRUE(status.getValid());
    ASSERT_EQ(reader.getMainInput(), ports[1].getGlobalId());
    ASSERT_EQ(input(status, 0).getError(), MultiReader2InputError::Disconnected);
    ASSERT_EQ(input(status, 2).getError(), MultiReader2InputError::Disconnected);

    readSignals[1].createAndSendPacket(0);
    readSignals[3].createAndSendPacket(0);
    scheduler.waitAll();
    SizeT count = 10;
    const auto values = readAll(reader, 4, count);
    ASSERT_EQ(count, 10u);
    ASSERT_EQ(values[0][0], -1.0);
    ASSERT_EQ(values[1][0], 0.0);
    ASSERT_EQ(values[2][0], -1.0);
    ASSERT_EQ(values[3][0], 0.0);
}

TEST_F(MultiReader2LifecycleTest, ExcludeWithEveryInputFailingIsInvalid)
{
    auto reader = three(MultiReader2ErrorPolicy::Exclude, true);
    for (auto& port : ports)
        port.disconnect();
    scheduler.waitAll();
    auto status = probe(reader);
    ASSERT_FALSE(status.getValid());
    ASSERT_EQ(reader.getMainInput(), "");
    for (int i = 0; i < 3; i++)
        ASSERT_EQ(input(status, i).getError(), MultiReader2InputError::Disconnected);

    // One returning input is enough
    ports[2].connect(readSignals[2].signal);
    scheduler.waitAll();
    status = probe(reader);
    ASSERT_TRUE(status.getValid());
    ASSERT_EQ(reader.getMainInput(), ports[2].getGlobalId());
    readSignals[2].createAndSendPacket(0);
    scheduler.waitAll();
    ASSERT_EQ(reader.getAvailableCount(), 10u);
}

TEST_F(MultiReader2LifecycleTest, ExcludeWithAPinnedMainFailingIsInvalidUntilItReturns)
{
    readSignals.reserve(3);
    auto domain = createDomainSignal();
    for (int i = 0; i < 3; i++)
        addSignal(0, 10, domain);
    auto p = params(portsList());
    p.setErrorPolicy(MultiReader2ErrorPolicy::Exclude);
    p.setMainInput(ports[1]);
    auto reader = createReader(p);
    connectAll(reader);

    ports[1].disconnect();
    scheduler.waitAll();
    auto status = probe(reader);
    ASSERT_FALSE(status.getValid());
    ASSERT_EQ(reader.getMainInput(), ports[1].getGlobalId());  // a pin never moves

    ports[1].connect(readSignals[1].signal);
    scheduler.waitAll();
    status = probe(reader);
    ASSERT_TRUE(status.getValid());
    sendPackets(0);
    ASSERT_EQ(reader.getAvailableCount(), 10u);
}

TEST_F(MultiReader2LifecycleTest, ExcludeWithAPinnedMainHealthyAndTheOthersFailingIsValid)
{
    readSignals.reserve(3);
    auto domain = createDomainSignal();
    for (int i = 0; i < 3; i++)
        addSignal(0, 10, domain);
    auto p = params(portsList());
    p.setErrorPolicy(MultiReader2ErrorPolicy::Exclude);
    p.setMainInput(ports[1]);
    auto reader = createReader(p);
    ports[1].connect(readSignals[1].signal);
    scheduler.waitAll();
    auto status = probe(reader);
    ASSERT_TRUE(status.getValid());
    readSignals[1].createAndSendPacket(0);
    scheduler.waitAll();
    SizeT count = 10;
    const auto values = readAll(reader, 3, count);
    ASSERT_EQ(count, 10u);
    ASSERT_EQ(values[1][9], 9.0);
}

TEST_F(MultiReader2LifecycleTest, AutomaticMainReturnsToTheFirstInputWhenItRecovers)
{
    auto reader = three(MultiReader2ErrorPolicy::Exclude, true);
    ports[0].disconnect();
    scheduler.waitAll();
    probe(reader);
    ASSERT_EQ(reader.getMainInput(), ports[1].getGlobalId());

    ports[0].connect(readSignals[0].signal);
    scheduler.waitAll();
    auto status = probe(reader);
    ASSERT_EQ(reader.getMainInput(), ports[0].getGlobalId());
    ASSERT_TRUE(status.getDomainDescriptorChanged());
    ASSERT_TRUE(status.getResynchronized());
}

TEST_F(MultiReader2LifecycleTest, AutomaticMainSkipsUnusedInputs)
{
    readSignals.reserve(3);
    auto domain = createDomainSignal();
    for (int i = 0; i < 3; i++)
        addSignal(0, 10, domain);
    auto p = params(portsList());
    p.setErrorPolicy(MultiReader2ErrorPolicy::Exclude);
    p.setInputUsed(ports[0], false);
    auto reader = createReader(p);
    connectAll(reader);
    ASSERT_EQ(reader.getMainInput(), ports[1].getGlobalId());
    ports[1].disconnect();
    scheduler.waitAll();
    probe(reader);
    ASSERT_EQ(reader.getMainInput(), ports[2].getGlobalId());
}

TEST_F(MultiReader2LifecycleTest, MainDisconnectUnderInvalidateInvalidatesAndRecovers)
{
    auto reader = three(MultiReader2ErrorPolicy::Invalidate, true);
    sendPackets(0);
    ports[0].disconnect();
    scheduler.waitAll();
    SizeT count = 10;
    MultiReader2StatusPtr status;
    readAll(reader, 3, count, status);
    ASSERT_EQ(count, 10u);
    ASSERT_FALSE(status.getValid());
    ASSERT_EQ(reader.getMainInput(), "");  // an automatic main leads nothing while invalid

    ports[0].connect(readSignals[0].signal);
    scheduler.waitAll();
    status = probe(reader);
    ASSERT_TRUE(status.getValid());
    ASSERT_EQ(reader.getMainInput(), ports[0].getGlobalId());
}

// ---------------------------------------------------------------- used flags over time

TEST_F(MultiReader2LifecycleTest, AllInputsUnusedIsValidWithoutAMain)
{
    readSignals.reserve(2);
    auto domain = createDomainSignal();
    addSignal(0, 10, domain);
    addSignal(0, 10, domain, SampleType::ComplexFloat32);
    auto p = params(signalsToList());
    p.setInputUsed(readSignals[0].signal, false);
    p.setInputUsed(readSignals[1].signal, false);
    auto reader = createReader(p);
    scheduler.waitAll();
    auto status = probe(reader);
    ASSERT_TRUE(status.getValid());
    ASSERT_EQ(reader.getMainInput(), "");
    ASSERT_FALSE(status.getDomainDescriptor().assigned());
    ASSERT_EQ(input(status, 1).getError(), MultiReader2InputError::ValueDescriptorInvalid);
    sendPackets(0);
    ASSERT_EQ(reader.getAvailableCount(), 0u);
}

TEST_F(MultiReader2LifecycleTest, UnusedReaderIgnoresErrorsAndData)
{
    readSignals.reserve(2);
    auto domain = createDomainSignal();
    addSignal(0, 10, domain);
    addSignal(0, 10, domain, SampleType::ComplexFloat32);
    auto p = params(signalsToList());
    p.setUsed(false);
    auto reader = createReader(p);
    scheduler.waitAll();
    auto status = probe(reader);
    ASSERT_TRUE(status.getValid());
    ASSERT_FALSE(input(status, 0).getUsed());
    ASSERT_EQ(input(status, 1).getError(), MultiReader2InputError::ValueDescriptorInvalid);
    sendPackets(0);
    ASSERT_EQ(reader.getAvailableCount(), 0u);

    // Taking the reader into use surfaces the error
    p.setUsed(true);
    reader.configure(p);
    scheduler.waitAll();
    status = probe(reader);
    ASSERT_FALSE(status.getValid());
}

TEST_F(MultiReader2LifecycleTest, UnusedInputDisconnectIsReportedWithoutResynchronization)
{
    readSignals.reserve(3);
    auto domain = createDomainSignal();
    for (int i = 0; i < 3; i++)
        addSignal(0, 10, domain);
    auto p = params(portsList());
    p.setInputUsed(ports[2], false);
    auto reader = createReader(p);
    connectAll(reader);
    sendPackets(0);
    ports[2].disconnect();
    scheduler.waitAll();
    SizeT count = 10;
    MultiReader2StatusPtr status;
    readAll(reader, 3, count, status);
    ASSERT_EQ(count, 10u);
    ASSERT_TRUE(status.getHasChanges());
    ASSERT_TRUE(status.getValid());
    ASSERT_FALSE(status.getResynchronized());
    ASSERT_EQ(input(status, 2).getError(), MultiReader2InputError::Disconnected);
}

TEST_F(MultiReader2LifecycleTest, GapOnAnUnusedInputChangesNothing)
{
    readSignals.reserve(3);
    auto domain = createDomainSignal();
    for (int i = 0; i < 3; i++)
        addSignal(0, 10, domain);
    auto p = params(signalsToList());
    p.setInputUsed(readSignals[2].signal, false);
    auto reader = createReaderProbed(p);
    sendPackets(0);
    readSignals[2].signal.sendPacket(ImplicitDomainGapDetectedEventPacket(5));
    readSignals[2].sendAt(50, 10);
    scheduler.waitAll();
    SizeT count = 10;
    MultiReader2StatusPtr status;
    readAll(reader, 3, count, status);
    ASSERT_EQ(count, 10u);
    ASSERT_FALSE(status.getHasChanges());
}

// ---------------------------------------------------------------- synchronization limits

TEST_F(MultiReader2LifecycleTest, SilentInputUnderExcludeIsSetAsideAfterTheDeadline)
{
    auto reader = three(MultiReader2ErrorPolicy::Exclude, false);
    manager(reader).setSyncLimits(std::chrono::milliseconds(50), std::chrono::milliseconds(2000));
    readSignals[0].createAndSendPacket(0);
    readSignals[1].createAndSendPacket(0);
    scheduler.waitAll();
    std::this_thread::sleep_for(std::chrono::milliseconds(80));
    readSignals[0].createAndSendPacket(1);
    scheduler.waitAll();

    auto status = probe(reader);
    ASSERT_TRUE(status.getValid());
    ASSERT_EQ(input(status, 2).getError(), MultiReader2InputError::SyncFailed);
    ASSERT_EQ(reader.getAvailableCount(), 10u);

    // Its data brings it back
    readSignals[2].createAndSendPacket(1);
    scheduler.waitAll();
    status = probe(reader);
    ASSERT_EQ(input(status, 2).getError(), MultiReader2InputError::None);
}

TEST_F(MultiReader2LifecycleTest, InputTooFarBehindUnderInvalidateInvalidates)
{
    readSignals.reserve(2);
    auto domain = createDomainSignal();
    addSignal(0, 10, domain);
    addSignal(5000, 10, domain);
    auto reader = createReaderProbed(params(signalsToList()));
    manager(reader).setSyncLimits(std::chrono::milliseconds(2000), std::chrono::milliseconds(1000));
    sendPackets(0);
    auto status = probe(reader);
    ASSERT_FALSE(status.getValid());
    ASSERT_EQ(input(status, 0).getError(), MultiReader2InputError::SyncFailed);

    // Data on the failed input is its way back; the recovery restarts from scratch, so both send again
    readSignals[0].sendAt(5000, 10);
    scheduler.waitAll();
    status = probe(reader);
    ASSERT_TRUE(status.getValid());
    ASSERT_TRUE(status.getResynchronized());
    ASSERT_EQ(input(status, 0).getError(), MultiReader2InputError::None);
    readSignals[0].sendAt(5000, 10);
    readSignals[1].sendAt(5000, 10);
    scheduler.waitAll();
    ASSERT_EQ(reader.getAvailableCount(), 10u);
}

TEST_F(MultiReader2LifecycleTest, InputJustWithinTheDistanceCatchesUp)
{
    readSignals.reserve(2);
    auto domain = createDomainSignal();
    addSignal(0, 10, domain);
    addSignal(900, 10, domain);
    auto reader = createReaderProbed(params(signalsToList()));
    manager(reader).setSyncLimits(std::chrono::milliseconds(2000), std::chrono::milliseconds(1000));
    sendPackets(0);
    auto status = probe(reader);
    ASSERT_TRUE(status.getValid());
    ASSERT_EQ(input(status, 0).getError(), MultiReader2InputError::None);
    ASSERT_EQ(reader.getAvailableCount(), 0u);
    for (Int i = 1; i <= 90; i++)
        readSignals[0].createAndSendPacket(i);
    scheduler.waitAll();
    ASSERT_EQ(reader.getAvailableCount(), 10u);
}

TEST_F(MultiReader2LifecycleTest, DisconnectWhileSynchronizingInvalidatesAndReconnectRestarts)
{
    auto reader = three(MultiReader2ErrorPolicy::Invalidate, true);
    readSignals[0].createAndSendPacket(0);
    scheduler.waitAll();
    ports[1].disconnect();
    scheduler.waitAll();
    auto status = probe(reader);
    ASSERT_FALSE(status.getValid());
    ports[1].connect(readSignals[1].signal);
    scheduler.waitAll();
    status = probe(reader);
    ASSERT_TRUE(status.getValid());
    sendPackets(1);
    ASSERT_EQ(reader.getAvailableCount(), 10u);
}

// ---------------------------------------------------------------- min read count against boundaries

TEST_F(MultiReader2LifecycleTest, MinReadCountOfAHundredGatesUntilAHundredAligned)
{
    auto reader = three(MultiReader2ErrorPolicy::Invalidate, false, 100);
    for (Int i = 0; i < 9; i++)
        sendPackets(i);
    ASSERT_EQ(reader.getAvailableCount(), 0u);
    sendPackets(9);
    ASSERT_EQ(reader.getAvailableCount(), 100u);
    SizeT count = 100;
    const auto values = readAll(reader, 3, count);
    ASSERT_EQ(count, 100u);
    ASSERT_EQ(values[2][99], 99.0);
}

TEST_F(MultiReader2LifecycleTest, ShortRunBeforeADisconnectIsDroppedUnderMinReadCount)
{
    auto reader = three(MultiReader2ErrorPolicy::Exclude, true, 8);
    sendPackets(0);
    readSignals[2].sendAt(10, 3);
    scheduler.waitAll();
    ports[2].disconnect();
    scheduler.waitAll();
    readSignals[0].sendAt(10, 20);
    readSignals[1].sendAt(10, 20);
    scheduler.waitAll();

    SizeT count = 10;
    MultiReader2StatusPtr status;
    readAll(reader, 3, count, status);
    ASSERT_EQ(count, 10u);
    count = 10;
    readAll(reader, 3, count, status);
    ASSERT_EQ(count, 0u);  // the three samples are dropped with the disconnect
    ASSERT_EQ(input(status, 2).getError(), MultiReader2InputError::Disconnected);
    count = 10;
    const auto values = readAll(reader, 3, count, status);
    ASSERT_EQ(count, 10u);
    ASSERT_EQ(values[0][0], 13.0);
}

TEST_F(MultiReader2LifecycleTest, ShortOpenRunWaitsForMoreData)
{
    auto reader = three(MultiReader2ErrorPolicy::Invalidate, false, 8);
    sendPackets(0);
    SizeT count = 10;
    readAll(reader, 3, count);
    readSignals[0].sendAt(10, 3);
    readSignals[1].sendAt(10, 10);
    readSignals[2].sendAt(10, 10);
    scheduler.waitAll();
    ASSERT_EQ(reader.getAvailableCount(), 0u);
    count = 10;
    MultiReader2StatusPtr status;
    readAll(reader, 3, count, status);
    ASSERT_EQ(count, 0u);
    ASSERT_FALSE(status.getHasChanges());  // a run that is merely short is not a boundary

    readSignals[0].sendAt(13, 7);
    scheduler.waitAll();
    ASSERT_EQ(reader.getAvailableCount(), 10u);
}

// ---------------------------------------------------------------- the wake event

TEST_F(MultiReader2LifecycleTest, WakeFiresForDataAtOrAboveMinReadCountOnly)
{
    auto reader = three(MultiReader2ErrorPolicy::Invalidate, false, 10);
    std::atomic<int> wakes{0};
    reader.getOnDataAvailable() += [&](InputPortPtr&, EventArgsPtr<>&) { wakes++; };

    for (auto& read : readSignals)
        read.sendAt(0, 10);
    scheduler.waitAll();
    const int afterSync = wakes.load();
    ASSERT_GE(afterSync, 1);

    SizeT count = 10;
    readAll(reader, 3, count);
    ASSERT_EQ(count, 10u);
    for (auto& read : readSignals)
        read.sendAt(10, 4);
    scheduler.waitAll();
    ASSERT_EQ(wakes.load(), afterSync);

    for (auto& read : readSignals)
        read.sendAt(14, 6);
    scheduler.waitAll();
    ASSERT_GT(wakes.load(), afterSync);
}

TEST_F(MultiReader2LifecycleTest, WakeFiresForADisconnectAndAGap)
{
    auto reader = three(MultiReader2ErrorPolicy::Exclude, true);
    std::atomic<int> wakes{0};
    reader.getOnDataAvailable() += [&](InputPortPtr&, EventArgsPtr<>&) { wakes++; };

    ports[2].disconnect();
    scheduler.waitAll();
    ASSERT_GE(wakes.load(), 1);
    const int afterDisconnect = wakes.load();

    readSignals[0].signal.sendPacket(ImplicitDomainGapDetectedEventPacket(3));
    scheduler.waitAll();
    ASSERT_GT(wakes.load(), afterDisconnect);
}

TEST_F(MultiReader2LifecycleTest, WakeFiresForASynchronizationFailure)
{
    auto reader = three(MultiReader2ErrorPolicy::Invalidate, false);
    manager(reader).setSyncLimits(std::chrono::milliseconds(50), std::chrono::milliseconds(2000));
    std::atomic<int> wakes{0};
    reader.getOnDataAvailable() += [&](InputPortPtr&, EventArgsPtr<>&) { wakes++; };
    readSignals[0].createAndSendPacket(0);
    scheduler.waitAll();
    const int before = wakes.load();
    std::this_thread::sleep_for(std::chrono::milliseconds(80));
    readSignals[0].createAndSendPacket(1);
    scheduler.waitAll();
    ASSERT_GT(wakes.load(), before);
    ASSERT_FALSE(probe(reader).getValid());
}

TEST_F(MultiReader2LifecycleTest, HandlerDrainsEverythingTheReaderGets)
{
    auto reader = three(MultiReader2ErrorPolicy::Invalidate, false);
    std::atomic<SizeT> total{0};
    std::mutex handlerMutex;
    reader.getOnDataAvailable() += [&](InputPortPtr&, EventArgsPtr<>&)
    {
        std::scoped_lock lock(handlerMutex);
        while (true)
        {
            SizeT count = 64;
            readAll(reader, 3, count);
            if (count == 0)
                break;
            total += count;
        }
    };
    for (Int i = 0; i < 200; i++)
        sendPackets(i);
    scheduler.waitAll();
    {
        std::scoped_lock lock(handlerMutex);
        SizeT count = 64;
        while (true)
        {
            count = 64;
            readAll(reader, 3, count);
            if (count == 0)
                break;
            total += count;
        }
    }
    ASSERT_EQ(total.load(), 2000u);
}

TEST_F(MultiReader2LifecycleTest, ProducerThreadAgainstTheReader)
{
    auto reader = three(MultiReader2ErrorPolicy::Invalidate, false);
    constexpr Int packets = 500;
    std::thread producer(
        [&]
        {
            for (Int i = 0; i < packets; i++)
                for (const auto& read : readSignals)
                    read.createAndSendPacket(i);
        });

    std::vector<double> seen;
    const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(20);
    while (seen.size() < static_cast<SizeT>(packets) * 10 && std::chrono::steady_clock::now() < deadline)
    {
        SizeT count = 128;
        MultiReader2StatusPtr status;
        const auto values = readAll(reader, 3, count, status);
        ASSERT_TRUE(status.getValid());
        ASSERT_FALSE(status.getResynchronized());
        for (SizeT k = 0; k < count; k++)
        {
            ASSERT_EQ(values[0][k], values[1][k]);
            ASSERT_EQ(values[1][k], values[2][k]);
            seen.push_back(values[0][k]);
        }
        if (count == 0)
            std::this_thread::yield();
    }
    producer.join();
    ASSERT_EQ(seen.size(), static_cast<SizeT>(packets) * 10);
    for (SizeT k = 0; k < seen.size(); k++)
        ASSERT_EQ(seen[k], static_cast<double>(k));
}

// ---------------------------------------------------------------- boundaries while invalid

TEST_F(MultiReader2LifecycleTest, DescriptorChangeWhileInvalidIsReportedOnRecovery)
{
    auto reader = three(MultiReader2ErrorPolicy::Invalidate, true);
    ports[2].disconnect();
    scheduler.waitAll();
    ASSERT_FALSE(probe(reader).getValid());

    readSignals[0].signal.setDescriptor(voltDescriptor());
    scheduler.waitAll();
    auto status = probe(reader);
    ASSERT_FALSE(status.getValid());
    ASSERT_FALSE(input(status, 0).getDescriptor().assigned());

    ports[2].connect(readSignals[2].signal);
    scheduler.waitAll();
    status = probe(reader);
    ASSERT_TRUE(status.getValid());
    ASSERT_EQ(input(status, 0).getDescriptor().getUnit().getSymbol(), "V");
    ASSERT_TRUE(input(status, 0).getDescriptorChanged());
}

TEST_F(MultiReader2LifecycleTest, GapAndDataWhileInvalidAreDropped)
{
    auto reader = three(MultiReader2ErrorPolicy::Invalidate, true);
    ports[2].disconnect();
    scheduler.waitAll();
    ASSERT_FALSE(probe(reader).getValid());
    readSignals[0].signal.sendPacket(ImplicitDomainGapDetectedEventPacket(3));
    readSignals[0].sendAt(100, 10);
    readSignals[1].sendAt(100, 10);
    scheduler.waitAll();
    auto status = probe(reader);
    ASSERT_FALSE(status.getHasChanges());
    ASSERT_FALSE(status.getValid());

    ports[2].connect(readSignals[2].signal);
    scheduler.waitAll();
    probe(reader);
    ASSERT_EQ(reader.getAvailableCount(), 0u);
    for (auto& read : readSignals)
        read.sendAt(200, 10);
    scheduler.waitAll();
    ASSERT_EQ(reader.getAvailableCount(), 10u);
}

TEST_F(MultiReader2LifecycleTest, ReconnectToADifferentSignalReportsItsDescriptor)
{
    auto reader = three(MultiReader2ErrorPolicy::Invalidate, true);
    auto other = Signal(context, nullptr, "other");
    other.setDescriptor(voltDescriptor());
    other.setDomainSignal(readSignals[0].signal.getDomainSignal());
    ports[1].disconnect();
    ports[1].connect(other);
    scheduler.waitAll();
    auto status = probe(reader);
    ASSERT_TRUE(status.getValid());
    ASSERT_TRUE(status.getHasChanges());
    ASSERT_TRUE(input(status, 1).getDescriptorChanged());
    ASSERT_EQ(input(status, 1).getDescriptor().getUnit().getSymbol(), "V");
    ASSERT_EQ(input(status, 1).getInput(), ports[1]);
}

TEST_F(MultiReader2LifecycleTest, ReconnectToASignalOnAnotherRateIsADomainError)
{
    auto reader = three(MultiReader2ErrorPolicy::Invalidate, true);
    auto other = Signal(context, nullptr, "other");
    other.setDescriptor(setupDescriptor(SampleType::Float64));
    other.setDomainSignal(createDomainSignal("", Ratio(1, 1000), LinearDataRule(2, 0)));
    ports[1].disconnect();
    ports[1].connect(other);
    scheduler.waitAll();
    auto status = probe(reader);
    ASSERT_FALSE(status.getValid());
    ASSERT_EQ(input(status, 1).getError(), MultiReader2InputError::DomainDescriptorInvalid);
}

TEST_F(MultiReader2LifecycleTest, ForeignEventPacketsAreIgnored)
{
    auto reader = three(MultiReader2ErrorPolicy::Invalidate, false);
    sendPackets(0);
    readSignals[1].signal.sendPacket(EventPacket("SomethingElse", Dict<IString, IBaseObject>()));
    scheduler.waitAll();
    SizeT count = 10;
    MultiReader2StatusPtr status;
    readAll(reader, 3, count, status);
    ASSERT_EQ(count, 10u);
    ASSERT_FALSE(status.getHasChanges());
}

TEST_F(MultiReader2LifecycleTest, EmptyDataPacketsAreIgnored)
{
    auto reader = three(MultiReader2ErrorPolicy::Invalidate, false);
    for (auto& read : readSignals)
        read.sendAt(0, 0);
    scheduler.waitAll();
    ASSERT_EQ(reader.getAvailableCount(), 0u);
    sendPackets(0);
    ASSERT_EQ(reader.getAvailableCount(), 10u);
    SizeT count = 10;
    MultiReader2StatusPtr status;
    const auto values = readAll(reader, 3, count, status);
    ASSERT_EQ(count, 10u);
    ASSERT_FALSE(status.getHasChanges());
    ASSERT_EQ(values[0][0], 0.0);
}
