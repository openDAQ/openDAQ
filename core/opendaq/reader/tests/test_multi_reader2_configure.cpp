/*
 * MultiReader2 construction and configure: input kinds and counts, the main input, reconfiguration between any two
 * parameter sets, and what happens to the input ports the reader uses.
 */
#include "test_multi_reader2_common.h"

#include <opendaq/folder_factory.h>
#include <opendaq/multi_reader2_factory.h>

using namespace daq;
using namespace testing;

class MultiReader2ConfigureTest : public MultiReader2Test
{
public:
    void addSignals(SizeT count, const SignalPtr& domain = nullptr)
    {
        readSignals.reserve(count);
        for (SizeT i = 0; i < count; i++)
            addSignal(0, 10, domain.assigned() ? domain : createDomainSignal());
    }

    // Reads `count` doubles per input into heap buffers; inputs the reader does not fill stay at -1
    std::vector<std::vector<double>> readAll(const MultiReader2Ptr& reader, SizeT inputs, SizeT& count)
    {
        std::vector<std::vector<double>> values(inputs, std::vector<double>(count, -1.0));
        std::vector<void*> buffers(inputs);
        for (SizeT i = 0; i < inputs; i++)
            buffers[i] = values[i].data();
        SizeT offset = 0;
        reader.read(buffers.data(), &count, offset);
        return values;
    }
};

// ---------------------------------------------------------------- construction

TEST_F(MultiReader2ConfigureTest, FactoryCreatesAReaderOverSignals)
{
    addSignals(2);
    auto p = MultiReader2Params();
    p.setInputs(signalsToList());
    auto reader = MultiReader2(p);
    ASSERT_EQ(reader.getMainInput(), readSignals[0].signal.getGlobalId());
    scheduler.waitAll();
    probe(reader);
    sendPackets(0);
    ASSERT_EQ(reader.getAvailableCount(), 10u);
}

TEST_F(MultiReader2ConfigureTest, NullParamsAreRejected)
{
    ASSERT_THROW(createReader(MultiReader2ParamsPtr()), ArgumentNullException);
}

TEST_F(MultiReader2ConfigureTest, ConfigureRejectsNullParams)
{
    addSignals(1);
    auto reader = createReader(params(signalsToList()));
    ASSERT_EQ(reader->configure(nullptr), OPENDAQ_ERR_ARGUMENT_NULL);
    daqClearErrorInfo();
}

TEST_F(MultiReader2ConfigureTest, SingleInputReads)
{
    addSignals(1);
    auto reader = createReaderProbed(params(signalsToList()));
    sendPackets(0);
    SizeT count = 10;
    const auto values = readAll(reader, 1, count);
    ASSERT_EQ(count, 10u);
    ASSERT_THAT(values[0], ElementsAre(0, 1, 2, 3, 4, 5, 6, 7, 8, 9));
}

TEST_F(MultiReader2ConfigureTest, SixtyFourInputsRead)
{
    addSignals(64, createDomainSignal());
    auto reader = createReaderProbed(params(signalsToList()));
    sendPackets(0);
    ASSERT_EQ(reader.getAvailableCount(), 10u);
    SizeT count = 10;
    const auto values = readAll(reader, 64, count);
    ASSERT_EQ(count, 10u);
    for (SizeT i = 0; i < 64; i++)
        ASSERT_THAT(values[i], ElementsAre(0, 1, 2, 3, 4, 5, 6, 7, 8, 9)) << i;
}

TEST_F(MultiReader2ConfigureTest, PinnedMainAtEveryPosition)
{
    addSignals(3, createDomainSignal());
    for (SizeT main = 0; main < 3; main++)
    {
        auto p = params(signalsToList());
        p.setMainInput(readSignals[main].signal);
        auto reader = createReader(p);
        ASSERT_EQ(reader.getMainInput(), readSignals[main].signal.getGlobalId());
        scheduler.waitAll();
        ASSERT_TRUE(probe(reader).getValid());
    }
}

TEST_F(MultiReader2ConfigureTest, AutomaticMainIsTheFirstUsedInput)
{
    addSignals(3, createDomainSignal());
    auto p = params(signalsToList());
    p.setInputUsed(readSignals[0].signal, false);
    auto reader = createReader(p);
    ASSERT_EQ(reader.getMainInput(), readSignals[1].signal.getGlobalId());
}

TEST_F(MultiReader2ConfigureTest, ReaderOverSignalsAddsOneConnectionPerReader)
{
    addSignals(2, createDomainSignal());
    auto first = createReader(params(signalsToList()));
    ASSERT_EQ(readSignals[0].signal.getConnections().getCount(), 1u);
    auto second = createReader(params(signalsToList()));
    ASSERT_EQ(readSignals[0].signal.getConnections().getCount(), 2u);
    second.release();
    ASSERT_EQ(readSignals[0].signal.getConnections().getCount(), 1u);
}

TEST_F(MultiReader2ConfigureTest, TwoReadersOverTheSameSignalsReadIndependently)
{
    addSignals(2, createDomainSignal());
    auto first = createReaderProbed(params(signalsToList()));
    auto second = createReaderProbed(params(signalsToList()));
    sendPackets(0);

    SizeT count = 10;
    const auto a = readAll(first, 2, count);
    ASSERT_EQ(count, 10u);
    ASSERT_EQ(second.getAvailableCount(), 10u);
    count = 4;
    const auto b = readAll(second, 2, count);
    ASSERT_EQ(count, 4u);
    ASSERT_EQ(first.getAvailableCount(), 0u);
    ASSERT_EQ(second.getAvailableCount(), 6u);
}

// ---------------------------------------------------------------- reconfiguration

TEST_F(MultiReader2ConfigureTest, ReconfigureAddsAnInput)
{
    addSignals(3, createDomainSignal());
    auto reader = createReaderProbed(params(List<IComponent>(readSignals[0].signal, readSignals[1].signal)));
    sendPackets(0);
    SizeT count = 10;
    readAll(reader, 2, count);
    ASSERT_EQ(count, 10u);

    reader.configure(params(signalsToList()));
    scheduler.waitAll();
    auto status = probe(reader);
    ASSERT_TRUE(status.getHasChanges());
    ASSERT_TRUE(status.getValid());
    ASSERT_EQ(status.getInputs().getCount(), 3u);
    ASSERT_EQ(input(status, 2).getInput(), readSignals[2].signal);
    ASSERT_TRUE(input(status, 2).getDescriptorChanged());

    sendPackets(1);
    count = 10;
    const auto values = readAll(reader, 3, count);
    ASSERT_EQ(count, 10u);
    ASSERT_THAT(values[2], ElementsAre(10, 11, 12, 13, 14, 15, 16, 17, 18, 19));
}

TEST_F(MultiReader2ConfigureTest, ReconfigureRemovesAnInputAndDisconnectsItsPort)
{
    addSignals(3, createDomainSignal());
    auto reader = createReaderProbed(params(signalsToList()));
    ASSERT_EQ(readSignals[2].signal.getConnections().getCount(), 1u);

    reader.configure(params(List<IComponent>(readSignals[0].signal, readSignals[1].signal)));
    scheduler.waitAll();
    ASSERT_EQ(readSignals[2].signal.getConnections().getCount(), 0u);
    auto status = probe(reader);
    ASSERT_EQ(status.getInputs().getCount(), 2u);

    sendPackets(0);
    SizeT count = 10;
    const auto values = readAll(reader, 2, count);
    ASSERT_EQ(count, 10u);
    ASSERT_THAT(values[1], ElementsAre(0, 1, 2, 3, 4, 5, 6, 7, 8, 9));
}

TEST_F(MultiReader2ConfigureTest, ReconfigureReordersInputsAndMovesTheAutomaticMain)
{
    addSignals(2);
    auto reader = createReaderProbed(params(signalsToList()));
    ASSERT_EQ(reader.getMainInput(), readSignals[0].signal.getGlobalId());

    reader.configure(params(List<IComponent>(readSignals[1].signal, readSignals[0].signal)));
    scheduler.waitAll();
    auto status = probe(reader);
    ASSERT_EQ(reader.getMainInput(), readSignals[1].signal.getGlobalId());
    ASSERT_EQ(input(status, 0).getInput(), readSignals[1].signal);
    ASSERT_EQ(input(status, 1).getInput(), readSignals[0].signal);
    ASSERT_EQ(status.getDomainDescriptor(), readSignals[1].domainDescriptor());

    // Buffers follow the new order
    readSignals[0].sendAt(0, 10);
    readSignals[1].sendAt(100, 10);
    scheduler.waitAll();
    readSignals[0].sendAt(10, 100);
    scheduler.waitAll();
    SizeT count = 10;
    const auto values = readAll(reader, 2, count);
    ASSERT_EQ(count, 10u);
    ASSERT_EQ(values[0][0], 100);
    ASSERT_EQ(values[1][0], 100);
}

TEST_F(MultiReader2ConfigureTest, ReconfigureChangesTheReadType)
{
    addSignals(2, createDomainSignal());
    auto p = params(signalsToList());
    auto reader = createReaderProbed(p);
    sendPackets(0);
    SizeT count = 10;
    readAll(reader, 2, count);
    ASSERT_EQ(count, 10u);

    p.setValueReadType(SampleType::Int32);
    reader.configure(p);
    scheduler.waitAll();
    probe(reader);
    sendPackets(1);
    std::array<int32_t, 10> a{};
    std::array<int32_t, 10> b{};
    void* buffers[2]{a.data(), b.data()};
    count = 10;
    SizeT offset = 0;
    reader.read(buffers, &count, offset);
    ASSERT_EQ(count, 10u);
    ASSERT_THAT(a, ElementsAre(10, 11, 12, 13, 14, 15, 16, 17, 18, 19));
    ASSERT_THAT(b, ElementsAreArray(a));
}

TEST_F(MultiReader2ConfigureTest, ReconfigureChangesTheMinReadCount)
{
    addSignals(2, createDomainSignal());
    auto p = params(signalsToList());
    auto reader = createReaderProbed(p);
    sendPackets(0);
    SizeT count = 3;
    readAll(reader, 2, count);
    ASSERT_EQ(count, 3u);

    p.setMinReadCount(5);
    reader.configure(p);
    scheduler.waitAll();
    probe(reader);
    sendPackets(1);
    std::array<std::array<double, 10>, 2> values{};
    count = 3;
    SizeT offset = 0;
    void* buffers[2]{values[0].data(), values[1].data()};
    IMultiReader2Status* raw = nullptr;
    ASSERT_EQ(reader->read(buffers, &count, &offset, &raw), OPENDAQ_ERR_INVALIDPARAMETER);
    daqClearErrorInfo();
    count = 5;
    read(reader, values, count);
    ASSERT_EQ(count, 5u);
}

TEST_F(MultiReader2ConfigureTest, ReconfigureChangesTheErrorPolicy)
{
    addSignals(3, createDomainSignal());
    auto p = params(portsList());
    auto reader = createReader(p);
    ports[0].connect(readSignals[0].signal);
    ports[1].connect(readSignals[1].signal);
    scheduler.waitAll();
    ASSERT_FALSE(probe(reader).getValid());

    p.setErrorPolicy(MultiReader2ErrorPolicy::Exclude);
    reader.configure(p);
    scheduler.waitAll();
    auto status = probe(reader);
    ASSERT_TRUE(status.getValid());
    ASSERT_EQ(input(status, 2).getError(), MultiReader2InputError::Disconnected);

    p.setErrorPolicy(MultiReader2ErrorPolicy::Invalidate);
    reader.configure(p);
    scheduler.waitAll();
    ASSERT_FALSE(probe(reader).getValid());
}

TEST_F(MultiReader2ConfigureTest, ReconfigureWithTheSameParamsObjectIsANoOp)
{
    addSignals(2, createDomainSignal());
    auto p = params(signalsToList());
    auto reader = createReaderProbed(p);
    sendPackets(0);
    ASSERT_EQ(reader->configure(p), OPENDAQ_SUCCESS);
    scheduler.waitAll();
    ASSERT_EQ(reader.getAvailableCount(), 10u);
    ASSERT_FALSE(probe(reader).getHasChanges());
}

TEST_F(MultiReader2ConfigureTest, ReconfigureFromSignalsToPorts)
{
    addSignals(2, createDomainSignal());
    auto reader = createReaderProbed(params(signalsToList()));
    reader.configure(params(portsList()));
    connectAll(reader);
    // The internal ports are gone; only the external ones remain connected
    ASSERT_EQ(readSignals[0].signal.getConnections().getCount(), 1u);
    sendPackets(0);
    ASSERT_EQ(reader.getAvailableCount(), 10u);
    ASSERT_EQ(reader.getMainInput(), ports[0].getGlobalId());
}

TEST_F(MultiReader2ConfigureTest, ReconfigureFromPortsToSignals)
{
    addSignals(2, createDomainSignal());
    auto reader = createReader(params(portsList()));
    connectAll(reader);
    reader.configure(params(signalsToList()));
    scheduler.waitAll();
    probe(reader);
    // The external ports were handed back and keep their signals
    ASSERT_FALSE(ports[0].getListener().assigned());
    ASSERT_TRUE(ports[0].getSignal().assigned());
    sendPackets(0);
    ASSERT_EQ(reader.getAvailableCount(), 10u);
    ASSERT_EQ(reader.getMainInput(), readSignals[0].signal.getGlobalId());
}

TEST_F(MultiReader2ConfigureTest, FailedConfigureKeepsQueuedData)
{
    addSignals(2, createDomainSignal());
    auto reader = createReaderProbed(params(signalsToList()));
    sendPackets(0);
    auto bad = params(List<IComponent>(readSignals[0].signal));
    bad.setMainInput(readSignals[1].signal);
    ASSERT_EQ(reader->configure(bad), OPENDAQ_ERR_INVALIDPARAMETER);
    daqClearErrorInfo();
    ASSERT_EQ(reader.getAvailableCount(), 10u);
}

TEST_F(MultiReader2ConfigureTest, ConfigureReportsTheLatestDescriptorOfARetainedInput)
{
    addSignals(2, createDomainSignal());
    auto p = params(signalsToList());
    auto reader = createReaderProbed(p);
    sendPackets(0);
    const auto changed = DataDescriptorBuilder().setSampleType(SampleType::Float64).setUnit(Unit("V")).build();
    readSignals[1].signal.setDescriptor(changed);
    scheduler.waitAll();

    // The boundary was never read; the reconfigured reader starts from the descriptor the signal announced last
    p.setMinReadCount(2);
    reader.configure(p);
    scheduler.waitAll();
    auto status = probe(reader);
    ASSERT_TRUE(status.getValid());
    ASSERT_EQ(input(status, 1).getDescriptor(), changed);
    ASSERT_TRUE(input(status, 1).getDescriptorChanged());
    ASSERT_EQ(reader.getAvailableCount(), 0u);
}

TEST_F(MultiReader2ConfigureTest, ConfigureTogglesTheReaderUsedFlag)
{
    addSignals(2, createDomainSignal());
    auto p = params(signalsToList());
    auto reader = createReaderProbed(p);

    p.setUsed(false);
    reader.configure(p);
    scheduler.waitAll();
    auto status = probe(reader);
    ASSERT_TRUE(status.getHasChanges());
    ASSERT_TRUE(status.getValid());
    ASSERT_FALSE(input(status, 0).getUsed());
    ASSERT_FALSE(input(status, 1).getUsed());
    ASSERT_EQ(reader.getMainInput(), "");
    sendPackets(0);
    ASSERT_EQ(reader.getAvailableCount(), 0u);

    p.setUsed(true);
    reader.configure(p);
    scheduler.waitAll();
    status = probe(reader);
    ASSERT_TRUE(input(status, 0).getUsed());
    ASSERT_EQ(reader.getMainInput(), readSignals[0].signal.getGlobalId());
    sendPackets(1);
    ASSERT_EQ(reader.getAvailableCount(), 10u);
}

TEST_F(MultiReader2ConfigureTest, ConfigureSetsAnInputUnusedAndBack)
{
    addSignals(2, createDomainSignal());
    auto p = params(signalsToList());
    auto reader = createReaderProbed(p);
    sendPackets(0);

    p.setInputUsed(readSignals[1].signal, false);
    reader.configure(p);
    scheduler.waitAll();
    auto status = probe(reader);
    ASSERT_TRUE(status.getHasChanges());
    ASSERT_FALSE(input(status, 1).getUsed());
    ASSERT_TRUE(input(status, 0).getUsed());

    // Only signal 0 gates; signal 1 may stay silent
    readSignals[0].createAndSendPacket(1);
    scheduler.waitAll();
    ASSERT_EQ(reader.getAvailableCount(), 10u);
    SizeT count = 10;
    const auto values = readAll(reader, 2, count);
    ASSERT_EQ(count, 10u);
    ASSERT_EQ(values[1][0], -1.0);  // untouched

    p.setInputUsed(readSignals[1].signal, true);
    reader.configure(p);
    scheduler.waitAll();
    status = probe(reader);
    ASSERT_TRUE(input(status, 1).getUsed());
    sendPackets(2);
    ASSERT_EQ(reader.getAvailableCount(), 10u);
}

TEST_F(MultiReader2ConfigureTest, ConfigureToAPinnedMainChangesTheDomainReported)
{
    addSignals(2);
    auto p = params(signalsToList());
    auto reader = createReaderProbed(p);

    p.setMainInput(readSignals[1].signal);
    reader.configure(p);
    scheduler.waitAll();
    auto status = probe(reader);
    ASSERT_TRUE(status.getDomainDescriptorChanged());
    ASSERT_EQ(status.getDomainDescriptor(), readSignals[1].domainDescriptor());
    ASSERT_EQ(reader.getMainInput(), readSignals[1].signal.getGlobalId());
}

TEST_F(MultiReader2ConfigureTest, ReconfigureManyTimesInARow)
{
    addSignals(4, createDomainSignal());
    auto reader = createReaderProbed(params(signalsToList()));
    for (int round = 0; round < 20; round++)
    {
        auto p = params(signalsToList());
        p.setMinReadCount(1 + round % 3);
        p.setMainInput(readSignals[round % 4].signal);
        ASSERT_EQ(reader->configure(p), OPENDAQ_SUCCESS);
    }
    scheduler.waitAll();
    auto status = probe(reader);
    ASSERT_TRUE(status.getValid());
    ASSERT_EQ(reader.getMainInput(), readSignals[3].signal.getGlobalId());
    sendPackets(0);
    ASSERT_EQ(reader.getAvailableCount(), 10u);
}

// ---------------------------------------------------------------- input ports

TEST_F(MultiReader2ConfigureTest, ExternalPortsStayConnectedAfterTheReaderIsGone)
{
    addSignals(2, createDomainSignal());
    auto list = portsList();
    {
        auto reader = createReader(params(list));
        connectAll(reader);
    }
    ASSERT_TRUE(ports[0].getSignal().assigned());
    ASSERT_TRUE(ports[1].getSignal().assigned());
    ASSERT_FALSE(ports[0].getListener().assigned());
    ASSERT_EQ(ports[0].getNotificationMethod(), PacketReadyNotification::None);
}

TEST_F(MultiReader2ConfigureTest, PortWithAParentKeepsIt)
{
    addSignals(1);
    auto folder = Folder(context, nullptr, "folder");
    auto port = InputPort(context, folder, "port");
    {
        auto reader = createReader(params(List<IComponent>(port)));
        ASSERT_EQ(port.getParent(), folder);
    }
    ASSERT_EQ(port.getParent(), folder);
}

TEST_F(MultiReader2ConfigureTest, FailedTakeoverAtConstructionLeavesEveryPortAsItWas)
{
    addSignals(2, createDomainSignal());
    auto folder = Folder(context, nullptr, "owner");
    auto parented = InputPort(context, folder, "parented");
    auto owned = InputPort(context, nullptr, "owned");
    auto first = createReader(params(List<IComponent>(owned)));
    owned.connect(readSignals[1].signal);
    scheduler.waitAll();
    probe(first);

    // The parented port was taken before the owned one failed; it is handed back on the way out
    ASSERT_THROW(createReader(params(List<IComponent>(parented, owned))), AlreadyExistsException);
    ASSERT_FALSE(parented.getListener().assigned());
    ASSERT_EQ(parented.getNotificationMethod(), PacketReadyNotification::None);
    ASSERT_EQ(owned.getListener(), first.asPtr<IInputPortNotifications>());
    readSignals[1].createAndSendPacket(0);
    scheduler.waitAll();
    ASSERT_EQ(first.getAvailableCount(), 10u);
}

TEST_F(MultiReader2ConfigureTest, FailedTakeoverOnReconfigureKeepsTheOldConfiguration)
{
    addSignals(2, createDomainSignal());
    auto folder = Folder(context, nullptr, "owner");
    auto parented = InputPort(context, folder, "parented");
    auto owned = InputPort(context, nullptr, "owned");
    auto first = createReader(params(List<IComponent>(owned)));
    auto second = createReader(params(List<IComponent>(parented)));
    parented.connect(readSignals[0].signal);
    scheduler.waitAll();
    probe(second);

    ASSERT_EQ(second->configure(params(List<IComponent>(parented, owned))), OPENDAQ_ERR_ALREADYEXISTS);
    daqClearErrorInfo();
    ASSERT_EQ(parented.getListener(), second.asPtr<IInputPortNotifications>());
    ASSERT_EQ(owned.getListener(), first.asPtr<IInputPortNotifications>());
    readSignals[0].createAndSendPacket(0);
    scheduler.waitAll();
    ASSERT_EQ(second.getAvailableCount(), 10u);
    auto status = probe(second);
    ASSERT_EQ(status.getInputs().getCount(), 1u);
}

TEST_F(MultiReader2ConfigureTest, PortWithoutAParentIsOwnedByTheReader)
{
    addSignals(1);
    auto port = InputPort(context, nullptr, "port");
    auto reader = createReader(params(List<IComponent>(port)));
    port.connect(readSignals[0].signal);
    scheduler.waitAll();
    probe(reader);
    sendPackets(0);
    ASSERT_EQ(reader.getAvailableCount(), 10u);
    // Nobody else can take it while the reader lives; it is free again afterwards
    ASSERT_THROW(createReader(params(List<IComponent>(port))), AlreadyExistsException);
    reader.release();
    ASSERT_NO_THROW(createReader(params(List<IComponent>(port))));
}

TEST_F(MultiReader2ConfigureTest, SecondReaderOverTheSamePortsTakesOverAndHandsBack)
{
    addSignals(2, createDomainSignal());
    // Ports with a parent are not owned by the reader, so a second reader may take them over
    auto folder = Folder(context, nullptr, "owner");
    auto list = List<IComponent>();
    for (int i = 0; i < 2; i++)
    {
        ports.push_back(InputPort(context, folder, fmt::format("port{}", i)));
        list.pushBack(ports.back());
    }
    auto first = createReader(params(list));
    connectAll(first);
    sendPackets(0);
    SizeT count = 10;
    readAll(first, 2, count);
    ASSERT_EQ(count, 10u);

    {
        auto second = createReaderProbed(params(list));
        ASSERT_EQ(ports[0].getListener(), second.asPtr<IInputPortNotifications>());
        sendPackets(1);
        ASSERT_EQ(second.getAvailableCount(), 10u);
        ASSERT_EQ(first.getAvailableCount(), 0u);
    }

    // The ports are the first reader's again; the stream resumes with a resynchronization over the missed packets
    ASSERT_EQ(ports[0].getListener(), first.asPtr<IInputPortNotifications>());
    ASSERT_EQ(ports[0].getNotificationMethod(), PacketReadyNotification::Scheduler);
    sendPackets(2);
    count = 10;
    readAll(first, 2, count);
    auto status = probe(first);
    sendPackets(3);
    count = 10;
    const auto values = readAll(first, 2, count);
    ASSERT_GT(count, 0u);
}

TEST_F(MultiReader2ConfigureTest, ReaderDestroyedWhilePacketsAreInFlight)
{
    addSignals(4, createDomainSignal());
    std::atomic<int> wakes{0};
    {
        auto reader = createReaderProbed(params(signalsToList()));
        reader.getOnDataAvailable() += [&wakes](InputPortPtr&, EventArgsPtr<>&) { wakes++; };
        for (Int i = 0; i < 50; i++)
            for (const auto& read : readSignals)
                read.createAndSendPacket(i);
    }
    scheduler.waitAll();
    ASSERT_GE(wakes.load(), 0);
}

TEST_F(MultiReader2ConfigureTest, ReaderDestroyedInsideItsOwnHandlerIsSafe)
{
    addSignals(2, createDomainSignal());
    auto reader = createReaderProbed(params(signalsToList()));
    std::atomic<bool> released{false};
    reader.getOnDataAvailable() += [&](InputPortPtr&, EventArgsPtr<>&)
    {
        if (!released.exchange(true))
            reader.release();
    };
    sendPackets(0);
    scheduler.waitAll();
    ASSERT_TRUE(released.load());
    ASSERT_FALSE(reader.assigned());
}
