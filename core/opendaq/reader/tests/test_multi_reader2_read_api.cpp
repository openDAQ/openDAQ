/*
 * MultiReader2 read surface: argument validation of read, readWithDomain, skipSamples and the getters, the count
 * contract (requested, available, min read count), packet offsets, domain ticks and the status handed back.
 */
#include "test_multi_reader2_common.h"

using namespace daq;
using namespace testing;

class MultiReader2ReadApiTest : public MultiReader2Test
{
public:
    // Two 1 kHz signals on one clock, ten samples per packet, probed and ready to stream
    MultiReader2Ptr twoSignals(SizeT minReadCount = 1, SampleType readType = SampleType::Float64)
    {
        readSignals.reserve(2);
        auto domain = createDomainSignal();
        addSignal(0, 10, domain);
        addSignal(0, 10, domain);
        auto p = params(signalsToList(), readType);
        p.setMinReadCount(minReadCount);
        return createReaderProbed(p);
    }

    std::vector<std::vector<double>> readAll(const MultiReader2Ptr& reader, SizeT inputs, SizeT& count, SizeT* offset = nullptr)
    {
        std::vector<std::vector<double>> values(inputs, std::vector<double>(count, -1.0));
        std::vector<void*> buffers(inputs);
        for (SizeT i = 0; i < inputs; i++)
            buffers[i] = values[i].data();
        SizeT localOffset = 0;
        reader.read(buffers.data(), &count, offset ? *offset : localOffset);
        return values;
    }
};

// ---------------------------------------------------------------- argument validation

TEST_F(MultiReader2ReadApiTest, ReadRejectsNullCount)
{
    auto reader = twoSignals();
    std::array<double, 10> a{}, b{};
    void* buffers[2]{a.data(), b.data()};
    SizeT offset = 0;
    IMultiReader2Status* status = nullptr;
    ASSERT_EQ(reader->read(buffers, nullptr, &offset, &status), OPENDAQ_ERR_ARGUMENT_NULL);
    daqClearErrorInfo();
}

TEST_F(MultiReader2ReadApiTest, ReadRejectsNullStatus)
{
    auto reader = twoSignals();
    std::array<double, 10> a{}, b{};
    void* buffers[2]{a.data(), b.data()};
    SizeT count = 10;
    SizeT offset = 0;
    ASSERT_EQ(reader->read(buffers, &count, &offset, nullptr), OPENDAQ_ERR_ARGUMENT_NULL);
    daqClearErrorInfo();
}

TEST_F(MultiReader2ReadApiTest, ReadAcceptsNullPacketOffset)
{
    auto reader = twoSignals();
    sendPackets(0);
    std::array<double, 10> a{}, b{};
    void* buffers[2]{a.data(), b.data()};
    SizeT count = 10;
    IMultiReader2Status* status = nullptr;
    ASSERT_EQ(reader->read(buffers, &count, nullptr, &status), OPENDAQ_SUCCESS);
    MultiReader2StatusPtr::Adopt(status);
    ASSERT_EQ(count, 10u);
    ASSERT_THAT(a, ElementsAre(0, 1, 2, 3, 4, 5, 6, 7, 8, 9));
}

TEST_F(MultiReader2ReadApiTest, ReadWithDomainRejectsNullCountAndStatus)
{
    auto reader = twoSignals();
    std::array<std::int64_t, 10> ticks{};
    std::array<double, 10> a{}, b{};
    void* buffers[3]{ticks.data(), a.data(), b.data()};
    SizeT count = 10;
    IMultiReader2Status* status = nullptr;
    ASSERT_EQ(reader->readWithDomain(buffers, nullptr, &status), OPENDAQ_ERR_ARGUMENT_NULL);
    daqClearErrorInfo();
    ASSERT_EQ(reader->readWithDomain(buffers, &count, nullptr), OPENDAQ_ERR_ARGUMENT_NULL);
    daqClearErrorInfo();
}

TEST_F(MultiReader2ReadApiTest, SkipSamplesRejectsNullCountAndStatus)
{
    auto reader = twoSignals();
    SizeT count = 10;
    IMultiReader2Status* status = nullptr;
    ASSERT_EQ(reader->skipSamples(nullptr, &status), OPENDAQ_ERR_ARGUMENT_NULL);
    daqClearErrorInfo();
    ASSERT_EQ(reader->skipSamples(&count, nullptr), OPENDAQ_ERR_ARGUMENT_NULL);
    daqClearErrorInfo();
}

TEST_F(MultiReader2ReadApiTest, GettersRejectNullOutput)
{
    auto reader = twoSignals();
    ASSERT_EQ(reader->getAvailableCount(nullptr), OPENDAQ_ERR_ARGUMENT_NULL);
    daqClearErrorInfo();
    ASSERT_EQ(reader->getMainInput(nullptr), OPENDAQ_ERR_ARGUMENT_NULL);
    daqClearErrorInfo();
    ASSERT_EQ(reader->getOnDataAvailable(nullptr), OPENDAQ_ERR_ARGUMENT_NULL);
    daqClearErrorInfo();
}

TEST_F(MultiReader2ReadApiTest, ReadCountBelowMinReadCountIsRejected)
{
    auto reader = twoSignals(4);
    sendPackets(0);
    std::array<double, 10> a{}, b{};
    void* buffers[2]{a.data(), b.data()};
    SizeT count = 3;
    SizeT offset = 0;
    IMultiReader2Status* status = nullptr;
    ASSERT_EQ(reader->read(buffers, &count, &offset, &status), OPENDAQ_ERR_INVALIDPARAMETER);
    daqClearErrorInfo();
    // Nothing was consumed by the rejected call
    ASSERT_EQ(reader.getAvailableCount(), 10u);
}

TEST_F(MultiReader2ReadApiTest, ReadCountAtMinReadCountIsAccepted)
{
    auto reader = twoSignals(4);
    sendPackets(0);
    SizeT count = 4;
    const auto values = readAll(reader, 2, count);
    ASSERT_EQ(count, 4u);
    ASSERT_THAT(values[0], ElementsAre(0, 1, 2, 3));
}

TEST_F(MultiReader2ReadApiTest, ReadWithDomainBelowMinReadCountIsRejected)
{
    auto reader = twoSignals(4);
    sendPackets(0);
    std::array<std::int64_t, 10> ticks{};
    std::array<double, 10> a{}, b{};
    void* buffers[3]{ticks.data(), a.data(), b.data()};
    SizeT count = 2;
    IMultiReader2Status* status = nullptr;
    ASSERT_EQ(reader->readWithDomain(buffers, &count, &status), OPENDAQ_ERR_INVALIDPARAMETER);
    daqClearErrorInfo();
}

TEST_F(MultiReader2ReadApiTest, SkipBelowMinReadCountIsRejected)
{
    auto reader = twoSignals(4);
    sendPackets(0);
    SizeT count = 2;
    IMultiReader2Status* status = nullptr;
    ASSERT_EQ(reader->skipSamples(&count, &status), OPENDAQ_ERR_INVALIDPARAMETER);
    daqClearErrorInfo();
    ASSERT_EQ(reader.getAvailableCount(), 10u);
}

TEST_F(MultiReader2ReadApiTest, NonZeroCountWithoutBuffersIsRejected)
{
    auto reader = twoSignals();
    sendPackets(0);
    SizeT count = 5;
    SizeT offset = 0;
    IMultiReader2Status* status = nullptr;
    ASSERT_EQ(reader->read(nullptr, &count, &offset, &status), OPENDAQ_ERR_INVALIDPARAMETER);
    daqClearErrorInfo();
    ASSERT_EQ(reader->readWithDomain(nullptr, &count, &status), OPENDAQ_ERR_INVALIDPARAMETER);
    daqClearErrorInfo();
    ASSERT_EQ(reader.getAvailableCount(), 10u);
}

TEST_F(MultiReader2ReadApiTest, ZeroCountWithoutBuffersIsTheStatusProbe)
{
    auto reader = twoSignals();
    sendPackets(0);
    SizeT count = 0;
    SizeT offset = 7;
    IMultiReader2Status* status = nullptr;
    ASSERT_EQ(reader->read(nullptr, &count, &offset, &status), OPENDAQ_SUCCESS);
    auto s = MultiReader2StatusPtr::Adopt(status);
    ASSERT_EQ(count, 0u);
    ASSERT_EQ(offset, 0u);
    ASSERT_TRUE(s.getValid());
    ASSERT_EQ(reader.getAvailableCount(), 10u);

    count = 0;
    ASSERT_EQ(reader->readWithDomain(nullptr, &count, &status), OPENDAQ_SUCCESS);
    MultiReader2StatusPtr::Adopt(status);
    count = 0;
    ASSERT_EQ(reader->skipSamples(&count, &status), OPENDAQ_SUCCESS);
    MultiReader2StatusPtr::Adopt(status);
    ASSERT_EQ(reader.getAvailableCount(), 10u);
}

TEST_F(MultiReader2ReadApiTest, NullBufferForAUsedInputIsRejectedAndNothingIsConsumed)
{
    auto reader = twoSignals();
    sendPackets(0);
    std::array<double, 10> a{};
    void* buffers[2]{a.data(), nullptr};
    SizeT count = 10;
    SizeT offset = 0;
    IMultiReader2Status* status = nullptr;
    ASSERT_EQ(reader->read(buffers, &count, &offset, &status), OPENDAQ_ERR_INVALIDPARAMETER);
    daqClearErrorInfo();
    ASSERT_EQ(reader.getAvailableCount(), 10u);

    std::array<double, 10> b{};
    buffers[1] = b.data();
    count = 10;
    ASSERT_EQ(reader->read(buffers, &count, &offset, &status), OPENDAQ_SUCCESS);
    MultiReader2StatusPtr::Adopt(status);
    ASSERT_EQ(count, 10u);
    ASSERT_THAT(b, ElementsAre(0, 1, 2, 3, 4, 5, 6, 7, 8, 9));
}

TEST_F(MultiReader2ReadApiTest, NullBufferForAnUnusedInputIsAccepted)
{
    readSignals.reserve(2);
    auto domain = createDomainSignal();
    addSignal(0, 10, domain);
    addSignal(0, 10, domain);
    auto p = params(signalsToList());
    p.setInputUsed(readSignals[1].signal, false);
    auto reader = createReaderProbed(p);
    sendPackets(0);
    std::array<double, 10> a{};
    void* buffers[2]{a.data(), nullptr};
    SizeT count = 10;
    SizeT offset = 0;
    IMultiReader2Status* status = nullptr;
    ASSERT_EQ(reader->read(buffers, &count, &offset, &status), OPENDAQ_SUCCESS);
    MultiReader2StatusPtr::Adopt(status);
    ASSERT_EQ(count, 10u);
}

TEST_F(MultiReader2ReadApiTest, NullBufferForASetAsideInputIsAccepted)
{
    readSignals.reserve(2);
    auto domain = createDomainSignal();
    addSignal(0, 10, domain);
    addSignal(0, 10, domain);
    auto p = params(portsList());
    p.setErrorPolicy(MultiReader2ErrorPolicy::Exclude);
    auto reader = createReader(p);
    ports[0].connect(readSignals[0].signal);
    scheduler.waitAll();
    probe(reader);
    readSignals[0].createAndSendPacket(0);
    scheduler.waitAll();
    std::array<double, 10> a{};
    void* buffers[2]{a.data(), nullptr};
    SizeT count = 10;
    SizeT offset = 0;
    IMultiReader2Status* status = nullptr;
    ASSERT_EQ(reader->read(buffers, &count, &offset, &status), OPENDAQ_SUCCESS);
    MultiReader2StatusPtr::Adopt(status);
    ASSERT_EQ(count, 10u);
}

TEST_F(MultiReader2ReadApiTest, ReadWithDomainRejectsNullDomainBuffer)
{
    auto reader = twoSignals();
    sendPackets(0);
    std::array<double, 10> a{}, b{};
    void* buffers[3]{nullptr, a.data(), b.data()};
    SizeT count = 10;
    IMultiReader2Status* status = nullptr;
    ASSERT_EQ(reader->readWithDomain(buffers, &count, &status), OPENDAQ_ERR_INVALIDPARAMETER);
    daqClearErrorInfo();
    ASSERT_EQ(reader.getAvailableCount(), 10u);
}

TEST_F(MultiReader2ReadApiTest, ReadWithDomainRejectsNullValueBuffer)
{
    auto reader = twoSignals();
    sendPackets(0);
    std::array<std::int64_t, 10> ticks{};
    std::array<double, 10> a{};
    void* buffers[3]{ticks.data(), a.data(), nullptr};
    SizeT count = 10;
    IMultiReader2Status* status = nullptr;
    ASSERT_EQ(reader->readWithDomain(buffers, &count, &status), OPENDAQ_ERR_INVALIDPARAMETER);
    daqClearErrorInfo();
    ASSERT_EQ(reader.getAvailableCount(), 10u);
}

TEST_F(MultiReader2ReadApiTest, NullBuffersAreNotCheckedWhileNothingIsAvailable)
{
    auto reader = twoSignals();
    SizeT count = 10;
    SizeT offset = 0;
    IMultiReader2Status* status = nullptr;
    void* buffers[2]{nullptr, nullptr};
    ASSERT_EQ(reader->read(buffers, &count, &offset, &status), OPENDAQ_SUCCESS);
    MultiReader2StatusPtr::Adopt(status);
    ASSERT_EQ(count, 0u);
}

// ---------------------------------------------------------------- counts

TEST_F(MultiReader2ReadApiTest, ReadMoreThanAvailableReturnsWhatIsThere)
{
    auto reader = twoSignals();
    sendPackets(0);
    SizeT count = 100;
    const auto values = readAll(reader, 2, count);
    ASSERT_EQ(count, 10u);
    ASSERT_EQ(values[0][9], 9);
    ASSERT_EQ(values[0][10], -1.0);  // beyond the count the buffer is untouched
}

TEST_F(MultiReader2ReadApiTest, ReadLessThanAvailableLeavesTheRest)
{
    auto reader = twoSignals();
    sendPackets(0);
    SizeT count = 3;
    readAll(reader, 2, count);
    ASSERT_EQ(count, 3u);
    ASSERT_EQ(reader.getAvailableCount(), 7u);
    count = 3;
    SizeT offset = 0;
    const auto values = readAll(reader, 2, count, &offset);
    ASSERT_EQ(offset, 3u);
    ASSERT_THAT(values[1], ElementsAre(3, 4, 5));
}

TEST_F(MultiReader2ReadApiTest, ReadExactlyAvailableEmptiesTheReader)
{
    auto reader = twoSignals();
    sendPackets(0);
    SizeT count = 10;
    readAll(reader, 2, count);
    ASSERT_EQ(count, 10u);
    ASSERT_EQ(reader.getAvailableCount(), 0u);
    count = 10;
    const auto values = readAll(reader, 2, count);
    ASSERT_EQ(count, 0u);
    ASSERT_EQ(values[0][0], -1.0);
}

TEST_F(MultiReader2ReadApiTest, ReadInChunksCoversEverySample)
{
    auto reader = twoSignals();
    for (Int i = 0; i < 10; i++)
        sendPackets(i);
    ASSERT_EQ(reader.getAvailableCount(), 100u);
    std::vector<double> all;
    while (true)
    {
        SizeT count = 7;
        const auto values = readAll(reader, 2, count);
        if (count == 0)
            break;
        all.insert(all.end(), values[1].begin(), values[1].begin() + count);
    }
    ASSERT_EQ(all.size(), 100u);
    for (SizeT k = 0; k < 100; k++)
        ASSERT_EQ(all[k], static_cast<double>(k));
}

TEST_F(MultiReader2ReadApiTest, ReadWhileStreamingWithNothingQueuedReturnsZeroQuietly)
{
    auto reader = twoSignals();
    sendPackets(0);
    SizeT count = 10;
    readAll(reader, 2, count);
    std::array<std::array<double, 10>, 2> values{};
    count = 10;
    auto status = read(reader, values, count);
    ASSERT_EQ(count, 0u);
    ASSERT_TRUE(status.getValid());
    ASSERT_FALSE(status.getHasChanges());
}

TEST_F(MultiReader2ReadApiTest, FirstReadAfterConstructionCarriesNoDataEvenIfQueued)
{
    readSignals.reserve(2);
    auto domain = createDomainSignal();
    addSignal(0, 10, domain);
    addSignal(0, 10, domain);
    auto reader = createReader(params(signalsToList()));
    sendPackets(0);
    ASSERT_EQ(reader.getAvailableCount(), 10u);

    std::array<std::array<double, 10>, 2> values{};
    SizeT count = 10;
    auto status = read(reader, values, count);
    ASSERT_EQ(count, 0u);
    ASSERT_TRUE(status.getHasChanges());
    count = 10;
    status = read(reader, values, count);
    ASSERT_EQ(count, 10u);
    ASSERT_FALSE(status.getHasChanges());
}

TEST_F(MultiReader2ReadApiTest, ProbeDoesNotConsume)
{
    auto reader = twoSignals();
    sendPackets(0);
    for (int i = 0; i < 5; i++)
        probe(reader);
    ASSERT_EQ(reader.getAvailableCount(), 10u);
}

TEST_F(MultiReader2ReadApiTest, SkipMoreThanAvailableSkipsWhatIsThere)
{
    auto reader = twoSignals();
    sendPackets(0);
    SizeT count = 100;
    reader.skipSamples(&count);
    ASSERT_EQ(count, 10u);
    ASSERT_EQ(reader.getAvailableCount(), 0u);
}

TEST_F(MultiReader2ReadApiTest, SkipWithinAPacketThenRead)
{
    auto reader = twoSignals();
    sendPackets(0);
    SizeT count = 4;
    reader.skipSamples(&count);
    ASSERT_EQ(count, 4u);
    count = 6;
    SizeT offset = 0;
    const auto values = readAll(reader, 2, count, &offset);
    ASSERT_EQ(count, 6u);
    ASSERT_EQ(offset, 4u);
    ASSERT_THAT(values[0], ElementsAre(4, 5, 6, 7, 8, 9));
}

TEST_F(MultiReader2ReadApiTest, SkipAcrossPackets)
{
    auto reader = twoSignals();
    for (Int i = 0; i < 3; i++)
        sendPackets(i);
    SizeT count = 25;
    reader.skipSamples(&count);
    ASSERT_EQ(count, 25u);
    count = 5;
    const auto values = readAll(reader, 2, count);
    ASSERT_THAT(values[0], ElementsAre(25, 26, 27, 28, 29));
}

// ---------------------------------------------------------------- packet offsets and domain ticks

TEST_F(MultiReader2ReadApiTest, PacketOffsetAdvancesWithEveryRead)
{
    auto reader = twoSignals();
    for (Int i = 0; i < 3; i++)
        sendPackets(i);
    SizeT offset = 99;
    for (SizeT expected = 0; expected < 30; expected += 6)
    {
        SizeT count = 6;
        readAll(reader, 2, count, &offset);
        ASSERT_EQ(count, 6u);
        ASSERT_EQ(offset, expected);
    }
}

TEST_F(MultiReader2ReadApiTest, PacketOffsetIsZeroWhenNothingIsRead)
{
    auto reader = twoSignals();
    SizeT offset = 99;
    SizeT count = 10;
    readAll(reader, 2, count, &offset);
    ASSERT_EQ(count, 0u);
    ASSERT_EQ(offset, 0u);
}

TEST_F(MultiReader2ReadApiTest, PacketOffsetIsInMainTicks)
{
    readSignals.reserve(2);
    // 1 kHz with a 10 kHz tick: ten ticks per sample
    auto domain = createDomainSignal("", Ratio(1, 10000), LinearDataRule(10, 0));
    addSignal(0, 10, domain);
    addSignal(0, 10, domain);
    auto reader = createReaderProbed(params(signalsToList()));
    sendPackets(0);
    sendPackets(1);
    SizeT count = 10;
    SizeT offset = 0;
    readAll(reader, 2, count, &offset);
    ASSERT_EQ(offset, 0u);
    count = 10;
    readAll(reader, 2, count, &offset);
    ASSERT_EQ(offset, 100u);
}

TEST_F(MultiReader2ReadApiTest, PacketOffsetMatchesTheDomainPacketOffsetOfTheMain)
{
    readSignals.reserve(2);
    addSignal(0, 10, createDomainSignal("", Ratio(1, 1000), LinearDataRule(1, 300)));
    addSignal(0, 10, createDomainSignal("", Ratio(1, 1000), LinearDataRule(1, 300)));
    auto reader = createReaderProbed(params(signalsToList()));
    // Domain packets at offset 350 hold ticks 650 and up; the reported offset is the packet's, not the tick
    readSignals[0].sendAt(350, 10);
    readSignals[1].sendAt(350, 10);
    scheduler.waitAll();
    SizeT count = 10;
    SizeT offset = 0;
    readAll(reader, 2, count, &offset);
    ASSERT_EQ(count, 10u);
    ASSERT_EQ(offset, 350u);
    std::array<std::int64_t, 10> ticks{};
    std::array<double, 10> a{}, b{};
    void* buffers[3]{ticks.data(), a.data(), b.data()};
    readSignals[0].sendAt(360, 10);
    readSignals[1].sendAt(360, 10);
    scheduler.waitAll();
    count = 10;
    reader.readWithDomain(buffers, &count);
    ASSERT_EQ(count, 10u);
    ASSERT_EQ(ticks[0], 660);
}

TEST_F(MultiReader2ReadApiTest, DomainTicksFollowTheMainDelta)
{
    readSignals.reserve(2);
    auto domain = createDomainSignal("", Ratio(1, 10000), LinearDataRule(10, 0));
    addSignal(0, 10, domain);
    addSignal(0, 10, domain);
    auto reader = createReaderProbed(params(signalsToList()));
    sendPackets(0);
    std::array<std::int64_t, 5> ticks{};
    std::array<double, 5> a{}, b{};
    void* buffers[3]{ticks.data(), a.data(), b.data()};
    SizeT count = 5;
    reader.readWithDomain(buffers, &count);
    ASSERT_EQ(count, 5u);
    ASSERT_THAT(ticks, ElementsAre(0, 10, 20, 30, 40));
    ASSERT_THAT(a, ElementsAre(0, 10, 20, 30, 40));
}

TEST_F(MultiReader2ReadApiTest, DomainTicksContinueAfterASkip)
{
    auto reader = twoSignals();
    sendPackets(0);
    SizeT count = 3;
    reader.skipSamples(&count);
    std::array<std::int64_t, 4> ticks{};
    std::array<double, 4> a{}, b{};
    void* buffers[3]{ticks.data(), a.data(), b.data()};
    count = 4;
    reader.readWithDomain(buffers, &count);
    ASSERT_THAT(ticks, ElementsAre(3, 4, 5, 6));
}

TEST_F(MultiReader2ReadApiTest, ReadAndReadWithDomainShareOnePosition)
{
    auto reader = twoSignals();
    sendPackets(0);
    SizeT count = 4;
    readAll(reader, 2, count);
    std::array<std::int64_t, 3> ticks{};
    std::array<double, 3> a{}, b{};
    void* buffers[3]{ticks.data(), a.data(), b.data()};
    count = 3;
    reader.readWithDomain(buffers, &count);
    ASSERT_THAT(ticks, ElementsAre(4, 5, 6));
    count = 3;
    SizeT offset = 0;
    const auto values = readAll(reader, 2, count, &offset);
    ASSERT_EQ(offset, 7u);
    ASSERT_THAT(values[0], ElementsAre(7, 8, 9));
}

TEST_F(MultiReader2ReadApiTest, DomainTicksAreMainTicksForScaledInputs)
{
    readSignals.reserve(2);
    addSignal(0, 10, createDomainSignal("", Ratio(1, 1000), LinearDataRule(1, 0)));
    addSignal(0, 10, createDomainSignal("", Ratio(1, 10000), LinearDataRule(10, 0)));
    auto reader = createReaderProbed(params(signalsToList()));
    sendPackets(0);
    std::array<std::int64_t, 3> ticks{};
    std::array<double, 3> a{}, b{};
    void* buffers[3]{ticks.data(), a.data(), b.data()};
    SizeT count = 3;
    reader.readWithDomain(buffers, &count);
    ASSERT_THAT(ticks, ElementsAre(0, 1, 2));
    ASSERT_THAT(b, ElementsAre(0, 10, 20));
}

// ---------------------------------------------------------------- available count

TEST_F(MultiReader2ReadApiTest, AvailableIsZeroUntilEveryInputHasData)
{
    auto reader = twoSignals();
    readSignals[0].createAndSendPacket(0);
    scheduler.waitAll();
    ASSERT_EQ(reader.getAvailableCount(), 0u);
    readSignals[1].createAndSendPacket(0);
    scheduler.waitAll();
    ASSERT_EQ(reader.getAvailableCount(), 10u);
}

TEST_F(MultiReader2ReadApiTest, AvailableIsTheMinimumOverTheInputs)
{
    readSignals.reserve(3);
    auto domain = createDomainSignal();
    addSignal(0, 10, domain);
    addSignal(0, 20, domain);
    addSignal(0, 30, domain);
    auto reader = createReaderProbed(params(signalsToList()));
    sendPackets(0);
    ASSERT_EQ(reader.getAvailableCount(), 10u);
    readSignals[0].createAndSendPacket(1);
    scheduler.waitAll();
    ASSERT_EQ(reader.getAvailableCount(), 20u);
    readSignals[0].createAndSendPacket(2);
    scheduler.waitAll();
    ASSERT_EQ(reader.getAvailableCount(), 20u);
    readSignals[1].createAndSendPacket(1);
    scheduler.waitAll();
    ASSERT_EQ(reader.getAvailableCount(), 30u);
}

TEST_F(MultiReader2ReadApiTest, AvailableIgnoresUnusedInputs)
{
    readSignals.reserve(3);
    auto domain = createDomainSignal();
    addSignal(0, 10, domain);
    addSignal(0, 10, domain);
    addSignal(0, 10, domain);
    auto p = params(signalsToList());
    p.setInputUsed(readSignals[2].signal, false);
    auto reader = createReaderProbed(p);
    readSignals[0].createAndSendPacket(0);
    readSignals[1].createAndSendPacket(0);
    scheduler.waitAll();
    ASSERT_EQ(reader.getAvailableCount(), 10u);
}

TEST_F(MultiReader2ReadApiTest, AvailableIgnoresSetAsideInputs)
{
    readSignals.reserve(3);
    auto domain = createDomainSignal();
    addSignal(0, 10, domain);
    addSignal(0, 10, domain);
    addSignal(0, 10, domain);
    auto p = params(portsList());
    p.setErrorPolicy(MultiReader2ErrorPolicy::Exclude);
    auto reader = createReader(p);
    ports[0].connect(readSignals[0].signal);
    ports[1].connect(readSignals[1].signal);
    scheduler.waitAll();
    probe(reader);
    readSignals[0].createAndSendPacket(0);
    readSignals[1].createAndSendPacket(0);
    scheduler.waitAll();
    ASSERT_EQ(reader.getAvailableCount(), 10u);
}

TEST_F(MultiReader2ReadApiTest, AvailableIsZeroBelowMinReadCount)
{
    auto reader = twoSignals(15);
    sendPackets(0);
    ASSERT_EQ(reader.getAvailableCount(), 0u);
    sendPackets(1);
    ASSERT_EQ(reader.getAvailableCount(), 20u);
}

TEST_F(MultiReader2ReadApiTest, AvailableStopsAtABoundary)
{
    auto reader = twoSignals();
    sendPackets(0);
    readSignals[1].signal.setDescriptor(DataDescriptorBuilder().setSampleType(SampleType::Float64).setUnit(Unit("V")).build());
    sendPackets(1);
    ASSERT_EQ(reader.getAvailableCount(), 10u);
    SizeT count = 20;
    readAll(reader, 2, count);
    ASSERT_EQ(count, 10u);
    ASSERT_EQ(reader.getAvailableCount(), 10u);
}

TEST_F(MultiReader2ReadApiTest, AvailableIsZeroWhileInvalid)
{
    auto reader = twoSignals();
    sendPackets(0);
    readSignals[1].signal.setDescriptor(setupDescriptor(SampleType::ComplexFloat64));
    scheduler.waitAll();
    SizeT count = 20;
    readAll(reader, 2, count);
    ASSERT_EQ(count, 10u);
    sendPackets(1);
    ASSERT_EQ(reader.getAvailableCount(), 0u);
}

TEST_F(MultiReader2ReadApiTest, AvailableIsZeroWhenTheReaderIsUnused)
{
    readSignals.reserve(2);
    auto domain = createDomainSignal();
    addSignal(0, 10, domain);
    addSignal(0, 10, domain);
    auto p = params(signalsToList());
    p.setUsed(false);
    auto reader = createReaderProbed(p);
    sendPackets(0);
    ASSERT_EQ(reader.getAvailableCount(), 0u);
    SizeT count = 10;
    const auto values = readAll(reader, 2, count);
    ASSERT_EQ(count, 0u);
}

// ---------------------------------------------------------------- shapes

TEST_F(MultiReader2ReadApiTest, UnequalPacketSizesAlignSampleBySample)
{
    readSignals.reserve(3);
    auto domain = createDomainSignal();
    addSignal(0, 3, domain);
    addSignal(0, 7, domain);
    addSignal(0, 11, domain);
    auto reader = createReaderProbed(params(signalsToList()));
    for (Int i = 0; i < 77; i++)
        readSignals[0].createAndSendPacket(i);
    for (Int i = 0; i < 33; i++)
        readSignals[1].createAndSendPacket(i);
    for (Int i = 0; i < 21; i++)
        readSignals[2].createAndSendPacket(i);
    scheduler.waitAll();
    ASSERT_EQ(reader.getAvailableCount(), 231u);
    SizeT count = 231;
    const auto values = readAll(reader, 3, count);
    ASSERT_EQ(count, 231u);
    for (SizeT i = 0; i < 3; i++)
        for (SizeT k = 0; k < 231; k++)
            ASSERT_EQ(values[i][k], static_cast<double>(k)) << i << " " << k;
}

TEST_F(MultiReader2ReadApiTest, LargePacketsReadInOneCall)
{
    readSignals.reserve(2);
    auto domain = createDomainSignal();
    addSignal(0, 100000, domain);
    addSignal(0, 100000, domain);
    auto reader = createReaderProbed(params(signalsToList()));
    sendPackets(0);
    SizeT count = 100000;
    const auto values = readAll(reader, 2, count);
    ASSERT_EQ(count, 100000u);
    ASSERT_EQ(values[0][99999], 99999);
    ASSERT_EQ(values[1][50000], 50000);
}

TEST_F(MultiReader2ReadApiTest, SixteenInputsOnDifferentClocksAlign)
{
    readSignals.reserve(16);
    for (Int i = 0; i < 16; i++)
        addSignal(i * 3, 50, createDomainSignal(i % 2 ? "2022-09-27T00:02:04+00:00" : "2022-09-27T00:02:03+00:00"));
    auto reader = createReaderProbed(params(signalsToList()));
    for (Int p = 0; p < 30; p++)
        sendPackets(p);
    // The latest start is input 15: tick 45 of a clock one second behind the main's origin
    SizeT count = 10;
    SizeT offset = 0;
    const auto values = readAll(reader, 16, count, &offset);
    ASSERT_EQ(count, 10u);
    ASSERT_EQ(offset, 1045u);
    for (SizeT i = 0; i < 16; i++)
        ASSERT_EQ(values[i][0], i % 2 ? 45.0 : 1045.0) << i;
}

// ---------------------------------------------------------------- the status

TEST_F(MultiReader2ReadApiTest, StatusListsEveryInputInOrder)
{
    readSignals.reserve(4);
    auto domain = createDomainSignal();
    for (int i = 0; i < 4; i++)
        addSignal(0, 10, domain);
    auto reader = createReader(params(signalsToList()));
    scheduler.waitAll();
    auto status = probe(reader);
    ASSERT_EQ(status.getInputs().getCount(), 4u);
    for (SizeT i = 0; i < 4; i++)
    {
        ASSERT_EQ(input(status, i).getInput(), readSignals[i].signal);
        ASSERT_EQ(status.getInputStatus(readSignals[i].signal).getInput(), readSignals[i].signal);
        ASSERT_EQ(input(status, i).getDescriptor(), readSignals[i].signal.getDescriptor());
    }
}

TEST_F(MultiReader2ReadApiTest, StatusDomainDescriptorIsTheMainInputs)
{
    readSignals.reserve(2);
    addSignal(0, 10, createDomainSignal("2022-09-27T00:02:03+00:00"));
    addSignal(0, 10, createDomainSignal("2022-09-27T00:02:04+00:00"));
    auto p = params(signalsToList());
    p.setMainInput(readSignals[1].signal);
    auto reader = createReader(p);
    scheduler.waitAll();
    auto status = probe(reader);
    ASSERT_EQ(status.getDomainDescriptor(), readSignals[1].domainDescriptor());
    ASSERT_EQ(status.getDomainDescriptor().getOrigin(), "2022-09-27T00:02:04+00:00");
}

TEST_F(MultiReader2ReadApiTest, StatusIsImmutable)
{
    auto reader = twoSignals();
    sendPackets(0);
    readSignals[0].signal.setDescriptor(DataDescriptorBuilder().setSampleType(SampleType::Float64).setUnit(Unit("V")).build());
    scheduler.waitAll();
    SizeT count = 10;
    std::array<std::array<double, 10>, 2> values{};
    auto first = read(reader, values, count);
    ASSERT_TRUE(first.getHasChanges());
    ASSERT_TRUE(input(first, 0).getDescriptorChanged());

    // Later reads do not reach back into an earlier status
    sendPackets(1);
    count = 10;
    auto second = read(reader, values, count);
    ASSERT_FALSE(second.getHasChanges());
    ASSERT_TRUE(first.getHasChanges());
    ASSERT_TRUE(input(first, 0).getDescriptorChanged());
}

TEST_F(MultiReader2ReadApiTest, StatusGettersRejectNullOutput)
{
    auto reader = twoSignals();
    auto status = probe(reader);
    ASSERT_EQ(status->getHasChanges(nullptr), OPENDAQ_ERR_ARGUMENT_NULL);
    daqClearErrorInfo();
    ASSERT_EQ(status->getValid(nullptr), OPENDAQ_ERR_ARGUMENT_NULL);
    daqClearErrorInfo();
    ASSERT_EQ(status->getDomainDescriptorChanged(nullptr), OPENDAQ_ERR_ARGUMENT_NULL);
    daqClearErrorInfo();
    ASSERT_EQ(status->getDomainDescriptor(nullptr), OPENDAQ_ERR_ARGUMENT_NULL);
    daqClearErrorInfo();
    ASSERT_EQ(status->getResynchronized(nullptr), OPENDAQ_ERR_ARGUMENT_NULL);
    daqClearErrorInfo();
    ASSERT_EQ(status->getInputs(nullptr), OPENDAQ_ERR_ARGUMENT_NULL);
    daqClearErrorInfo();
    IMultiReader2InputStatus* raw = nullptr;
    ASSERT_EQ(status->getInputStatus(nullptr, &raw), OPENDAQ_ERR_ARGUMENT_NULL);
    daqClearErrorInfo();
    ASSERT_EQ(status->getInputStatus(readSignals[0].signal, nullptr), OPENDAQ_ERR_ARGUMENT_NULL);
    daqClearErrorInfo();
    auto inputStatus = input(status, 0);
    ASSERT_EQ(inputStatus->getInput(nullptr), OPENDAQ_ERR_ARGUMENT_NULL);
    daqClearErrorInfo();
    ASSERT_EQ(inputStatus->getUsed(nullptr), OPENDAQ_ERR_ARGUMENT_NULL);
    daqClearErrorInfo();
    ASSERT_EQ(inputStatus->getError(nullptr), OPENDAQ_ERR_ARGUMENT_NULL);
    daqClearErrorInfo();
    ASSERT_EQ(inputStatus->getDescriptorChanged(nullptr), OPENDAQ_ERR_ARGUMENT_NULL);
    daqClearErrorInfo();
    ASSERT_EQ(inputStatus->getDescriptor(nullptr), OPENDAQ_ERR_ARGUMENT_NULL);
    daqClearErrorInfo();
}

TEST_F(MultiReader2ReadApiTest, QuietStatusCarriesLevelsOnly)
{
    auto reader = twoSignals();
    sendPackets(0);
    SizeT count = 10;
    readAll(reader, 2, count);
    auto status = probe(reader);
    ASSERT_FALSE(status.getHasChanges());
    ASSERT_TRUE(status.getValid());
    ASSERT_FALSE(status.getDomainDescriptorChanged());
    ASSERT_FALSE(status.getResynchronized());
    ASSERT_TRUE(status.getDomainDescriptor().assigned());
    for (SizeT i = 0; i < 2; i++)
    {
        ASSERT_FALSE(input(status, i).getDescriptorChanged());
        ASSERT_TRUE(input(status, i).getDescriptor().assigned());
        ASSERT_EQ(input(status, i).getError(), MultiReader2InputError::None);
        ASSERT_TRUE(input(status, i).getUsed());
    }
}
