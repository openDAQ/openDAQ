/*
 * MultiReader2 public API suite; the fixture is in test_multi_reader2_common.h.
 */
#include "test_multi_reader2_common.h"

using namespace daq;
using namespace testing;

// ---------------------------------------------------------------- params

TEST_F(MultiReader2Test, ParamsDefaults)
{
    readSignals.reserve(2);
    addSignal(0, 10, createDomainSignal());
    addSignal(0, 10, createDomainSignal());
    MultiReader2ParamsPtr p = createWithImplementation<IMultiReader2Params, MultiReader2ParamsImpl>();
    p.setInputs(signalsToList());

    ASSERT_EQ(p.getValueReadType(), SampleType::Float64);
    ASSERT_EQ(p.getMinReadCount(), 1u);
    ASSERT_EQ(p.getErrorPolicy(), MultiReader2ErrorPolicy::Invalidate);
    ASSERT_TRUE(p.getUsed());
    ASSERT_FALSE(p.getMainInput().assigned());
    ASSERT_TRUE(p.getInputUsed(readSignals[0].signal));
    ASSERT_TRUE(p.getInputUsed(readSignals[1].signal));
}

TEST_F(MultiReader2Test, ParamsRejectMixedInputs)
{
    readSignals.reserve(1);
    addSignal(0, 10, createDomainSignal());
    auto port = InputPort(context, nullptr, "port");
    MultiReader2ParamsPtr p = createWithImplementation<IMultiReader2Params, MultiReader2ParamsImpl>();
    ASSERT_EQ(p->setInputs(List<IComponent>(readSignals[0].signal, port)), OPENDAQ_ERR_INVALIDPARAMETER);
    daqClearErrorInfo();
}

TEST_F(MultiReader2Test, ParamsRejectEmptyDuplicateAndBadValues)
{
    readSignals.reserve(1);
    addSignal(0, 10, createDomainSignal());
    MultiReader2ParamsPtr p = createWithImplementation<IMultiReader2Params, MultiReader2ParamsImpl>();
    ASSERT_EQ(p->setInputs(List<IComponent>()), OPENDAQ_ERR_INVALIDPARAMETER);
    daqClearErrorInfo();
    ASSERT_EQ(p->setInputs(List<IComponent>(readSignals[0].signal, readSignals[0].signal)), OPENDAQ_ERR_DUPLICATEITEM);
    daqClearErrorInfo();
    ASSERT_EQ(p->setMinReadCount(0), OPENDAQ_ERR_INVALIDPARAMETER);
    daqClearErrorInfo();
    ASSERT_EQ(p->setValueReadType(SampleType::ComplexFloat32), OPENDAQ_ERR_INVALIDPARAMETER);
    daqClearErrorInfo();
    ASSERT_EQ(p->setValueReadType(SampleType::Struct), OPENDAQ_ERR_INVALIDPARAMETER);
    daqClearErrorInfo();
}

TEST_F(MultiReader2Test, ParamsUsedFlagsFollowTheList)
{
    readSignals.reserve(3);
    addSignal(0, 10, createDomainSignal());
    addSignal(0, 10, createDomainSignal());
    addSignal(0, 10, createDomainSignal());
    auto p = params(List<IComponent>(readSignals[0].signal, readSignals[1].signal));
    p.setInputUsed(readSignals[1].signal, false);
    ASSERT_FALSE(p.getInputUsed(readSignals[1].signal));

    // An input not in the list is rejected
    ASSERT_EQ(p->setInputUsed(readSignals[2].signal, false), OPENDAQ_ERR_INVALIDPARAMETER);
    daqClearErrorInfo();
    daq::Bool used;
    ASSERT_EQ(p->getInputUsed(readSignals[2].signal, &used), OPENDAQ_ERR_INVALIDPARAMETER);
    daqClearErrorInfo();

    // Flags of retained inputs survive a new list
    p.setInputs(List<IComponent>(readSignals[1].signal, readSignals[2].signal));
    ASSERT_FALSE(p.getInputUsed(readSignals[1].signal));
    ASSERT_TRUE(p.getInputUsed(readSignals[2].signal));

    // A pinned main input cannot be unused
    p.setMainInput(readSignals[2].signal);
    ASSERT_EQ(p->setInputUsed(readSignals[2].signal, false), OPENDAQ_ERR_INVALIDPARAMETER);
    daqClearErrorInfo();
}

// ---------------------------------------------------------------- construction and configure

TEST_F(MultiReader2Test, CreateFromSignalsChoosesFirstMain)
{
    readSignals.reserve(2);
    addSignal(0, 10, createDomainSignal());
    addSignal(0, 10, createDomainSignal());
    auto reader = createReader(params(signalsToList()));
    ASSERT_EQ(reader.getMainInput(), readSignals[0].signal.getGlobalId());
}

TEST_F(MultiReader2Test, PinnedMainInput)
{
    readSignals.reserve(2);
    addSignal(0, 10, createDomainSignal());
    addSignal(0, 10, createDomainSignal());
    auto p = params(signalsToList());
    p.setMainInput(readSignals[1].signal);
    auto reader = createReader(p);
    ASSERT_EQ(reader.getMainInput(), readSignals[1].signal.getGlobalId());
}

TEST_F(MultiReader2Test, CreateRejectsEmptyListAndForeignMain)
{
    readSignals.reserve(2);
    addSignal(0, 10, createDomainSignal());
    addSignal(0, 10, createDomainSignal());
    MultiReader2ParamsPtr empty = createWithImplementation<IMultiReader2Params, MultiReader2ParamsImpl>();
    ASSERT_THROW(createReader(empty), InvalidParameterException);

    auto p = params(List<IComponent>(readSignals[0].signal));
    p.setMainInput(readSignals[1].signal);
    ASSERT_THROW(createReader(p), InvalidParameterException);
}

TEST_F(MultiReader2Test, FailedConfigureKeepsReader)
{
    readSignals.reserve(2);
    addSignal(0, 10, createDomainSignal());
    addSignal(0, 10, createDomainSignal());
    auto reader = createReader(params(List<IComponent>(readSignals[0].signal)));

    auto bad = params(List<IComponent>(readSignals[1].signal));
    bad.setMainInput(readSignals[0].signal);
    ASSERT_EQ(reader->configure(bad), OPENDAQ_ERR_INVALIDPARAMETER);
    daqClearErrorInfo();
    ASSERT_EQ(reader.getMainInput(), readSignals[0].signal.getGlobalId());
}

TEST_F(MultiReader2Test, FirstStatusMarksEverythingNew)
{
    readSignals.reserve(2);
    addSignal(0, 10, createDomainSignal());
    addSignal(0, 10, createDomainSignal());
    auto reader = createReader(params(signalsToList()));
    scheduler.waitAll();

    auto status = probe(reader);
    ASSERT_TRUE(status.getHasChanges());
    ASSERT_TRUE(status.getValid());
    ASSERT_TRUE(status.getDomainDescriptorChanged());
    ASSERT_TRUE(status.getResynchronized());
    ASSERT_EQ(status.getDomainDescriptor(), readSignals[0].domainDescriptor());
    ASSERT_EQ(status.getInputs().getCount(), 2u);
    for (SizeT i = 0; i < 2; i++)
    {
        ASSERT_TRUE(input(status, i).getDescriptorChanged());
        ASSERT_EQ(input(status, i).getDescriptor(), readSignals[i].signal.getDescriptor());
        ASSERT_EQ(input(status, i).getError(), MultiReader2InputError::None);
        ASSERT_TRUE(input(status, i).getUsed());
        ASSERT_EQ(input(status, i).getInput(), readSignals[i].signal);
    }

    // Edges are gone on the next status, levels stay
    status = probe(reader);
    ASSERT_FALSE(status.getHasChanges());
    ASSERT_TRUE(status.getValid());
    ASSERT_FALSE(status.getDomainDescriptorChanged());
    ASSERT_FALSE(status.getResynchronized());
    ASSERT_FALSE(input(status, 0).getDescriptorChanged());
    ASSERT_TRUE(status.getDomainDescriptor().assigned());
    ASSERT_TRUE(input(status, 0).getDescriptor().assigned());
}

TEST_F(MultiReader2Test, GetInputStatusByComponent)
{
    readSignals.reserve(2);
    addSignal(0, 10, createDomainSignal());
    addSignal(0, 10, createDomainSignal());
    auto other = Signal(context, nullptr, "other");
    auto reader = createReader(params(signalsToList()));
    scheduler.waitAll();

    auto status = probe(reader);
    ASSERT_EQ(status.getInputStatus(readSignals[1].signal).getInput(), readSignals[1].signal);
    IMultiReader2InputStatus* raw;
    ASSERT_EQ(status->getInputStatus(other, &raw), OPENDAQ_ERR_INVALIDPARAMETER);
    daqClearErrorInfo();
}

TEST_F(MultiReader2Test, NoOpConfigureKeepsQueuedData)
{
    readSignals.reserve(2);
    addSignal(0, 10, createDomainSignal());
    addSignal(0, 10, createDomainSignal());
    auto p = params(signalsToList());
    auto reader = createReader(p);
    sendPackets(0);
    ASSERT_EQ(reader.getAvailableCount(), 10u);

    auto same = params(signalsToList());
    ASSERT_EQ(reader->configure(same), OPENDAQ_SUCCESS);
    ASSERT_EQ(reader.getAvailableCount(), 10u);

    // A different configuration discards the queues
    same.setMinReadCount(2);
    ASSERT_EQ(reader->configure(same), OPENDAQ_SUCCESS);
    scheduler.waitAll();
    ASSERT_EQ(reader.getAvailableCount(), 0u);
}

// ---------------------------------------------------------------- reading

TEST_F(MultiReader2Test, ReadsAlignedData)
{
    readSignals.reserve(3);
    auto domain = createDomainSignal();
    addSignal(0, 5, domain);
    addSignal(0, 7, domain);
    addSignal(0, 9, domain);
    auto reader = createReaderProbed(params(signalsToList()));

    sendPackets(0);
    sendPackets(1);
    ASSERT_EQ(reader.getAvailableCount(), 10u);

    std::array<std::array<double, 10>, 3> values{};
    SizeT count = 10;
    SizeT offset = 99;
    auto status = read(reader, values, count, &offset);
    ASSERT_EQ(count, 10u);
    ASSERT_EQ(offset, 0u);
    ASSERT_FALSE(status.getHasChanges());  // the first status was taken by createReaderProbed
    for (SizeT i = 0; i < 3; i++)
        for (SizeT k = 0; k < 10; k++)
            ASSERT_DOUBLE_EQ(values[i][k], static_cast<double>(k));

    ASSERT_EQ(reader.getAvailableCount(), 0u);
    sendPackets(2);
    ASSERT_EQ(reader.getAvailableCount(), 5u);
    count = 10;
    status = read(reader, values, count, &offset);
    ASSERT_EQ(count, 5u);
    ASSERT_EQ(offset, 10u);
    ASSERT_FALSE(status.getHasChanges());
    for (SizeT i = 0; i < 3; i++)
        for (SizeT k = 0; k < 5; k++)
            ASSERT_DOUBLE_EQ(values[i][k], static_cast<double>(10 + k));
}

TEST_F(MultiReader2Test, AlignsDifferentOriginsAndStarts)
{
    readSignals.reserve(3);
    // 1 kHz each: the second origin is one second later, the third starts two hundred ticks in
    addSignal(0, 500, createDomainSignal("2022-09-27T00:02:03+00:00"));
    addSignal(0, 500, createDomainSignal("2022-09-27T00:02:04+00:00"));
    addSignal(200, 500, createDomainSignal("2022-09-27T00:02:03+00:00"));
    auto reader = createReaderProbed(params(signalsToList()));

    for (Int i = 0; i < 4; i++)
        sendPackets(i);

    // Signal 1 tick 0 is signal 0 tick 1000; signal 2 starts at tick 200 of the same clock
    // Common start is main tick 1000; signal 0 has 2000 ticks, signal 1 reaches 1000 + 2000, signal 2 reaches 200 + 2000
    ASSERT_EQ(reader.getAvailableCount(), 1000u);

    std::array<std::array<double, 5>, 3> values{};
    SizeT count = 5;
    SizeT offset = 0;
    auto status = read(reader, values, count, &offset);
    ASSERT_EQ(count, 5u);
    ASSERT_EQ(offset, 1000u);
    ASSERT_THAT(values[0], ElementsAre(1000, 1001, 1002, 1003, 1004));
    ASSERT_THAT(values[1], ElementsAre(0, 1, 2, 3, 4));
    ASSERT_THAT(values[2], ElementsAre(1000, 1001, 1002, 1003, 1004));
}

TEST_F(MultiReader2Test, AlignsDifferentResolutionsAtEqualRate)
{
    readSignals.reserve(2);
    addSignal(0, 100, createDomainSignal("", Ratio(1, 1000), LinearDataRule(1, 0)));
    addSignal(0, 100, createDomainSignal("", Ratio(1, 2000), LinearDataRule(2, 0)));
    auto reader = createReaderProbed(params(signalsToList()));
    sendPackets(0);
    ASSERT_EQ(reader.getAvailableCount(), 100u);

    std::array<std::array<double, 4>, 2> values{};
    SizeT count = 4;
    auto status = read(reader, values, count);
    ASSERT_EQ(count, 4u);
    ASSERT_THAT(values[0], ElementsAre(0, 1, 2, 3));
    ASSERT_THAT(values[1], ElementsAre(0, 2, 4, 6));  // its own ticks run twice as fast
}

TEST_F(MultiReader2Test, DifferentRateIsDomainInvalid)
{
    readSignals.reserve(2);
    addSignal(0, 100, createDomainSignal("", Ratio(1, 1000), LinearDataRule(1, 0)));
    addSignal(0, 100, createDomainSignal("", Ratio(1, 1000), LinearDataRule(2, 0)));
    auto reader = createReader(params(signalsToList()));
    scheduler.waitAll();

    auto status = probe(reader);
    ASSERT_FALSE(status.getValid());
    ASSERT_EQ(input(status, 0).getError(), MultiReader2InputError::None);
    ASSERT_EQ(input(status, 1).getError(), MultiReader2InputError::DomainDescriptorInvalid);
    ASSERT_FALSE(input(status, 1).getDescriptor().assigned());
    ASSERT_FALSE(status.getDomainDescriptor().assigned());
}

TEST_F(MultiReader2Test, FractionalOriginAndMissingUnitAreDomainInvalid)
{
    readSignals.reserve(2);
    addSignal(0, 100, createDomainSignal("2022-09-27T00:02:04.123+00:00"));
    auto noUnit = Signal(context, nullptr, "nounit");
    noUnit.setDescriptor(DataDescriptorBuilder().setSampleType(SampleType::Int64).setRule(LinearDataRule(1, 0)).setTickResolution(Ratio(1, 1000)).build());
    auto sig = Signal(context, nullptr, "sig_nounit");
    sig.setDescriptor(setupDescriptor(SampleType::Float64));
    sig.setDomainSignal(noUnit);
    readSignals.emplace_back(sig, 0, 100);
    auto reader = createReader(params(signalsToList()));
    scheduler.waitAll();

    auto status = probe(reader);
    ASSERT_FALSE(status.getValid());
    ASSERT_EQ(input(status, 0).getError(), MultiReader2InputError::DomainDescriptorInvalid);
    ASSERT_EQ(input(status, 1).getError(), MultiReader2InputError::DomainDescriptorInvalid);
}

TEST_F(MultiReader2Test, PacketOffsetIsRelativeToRuleStart)
{
    readSignals.reserve(2);
    auto domain = createDomainSignal("", Ratio(1, 1000), LinearDataRule(1, 500));
    addSignal(0, 10, domain);
    addSignal(0, 10, domain);
    auto reader = createReaderProbed(params(signalsToList()));
    sendPackets(0);

    std::array<std::array<double, 10>, 2> values{};
    SizeT count = 10;
    SizeT offset = 0;
    read(reader, values, count, &offset);
    ASSERT_EQ(count, 10u);
    ASSERT_EQ(offset, 0u);
    sendPackets(1);
    count = 10;
    read(reader, values, count, &offset);
    ASSERT_EQ(offset, 10u);
}

TEST_F(MultiReader2Test, ReadWithDomainGivesMainTicks)
{
    readSignals.reserve(2);
    addSignal(0, 10, createDomainSignal("2022-09-27T00:02:03+00:00", Ratio(1, 1000), LinearDataRule(1, 7)));
    addSignal(0, 10, createDomainSignal("2022-09-27T00:02:03+00:00", Ratio(1, 1000), LinearDataRule(1, 7)));
    auto reader = createReaderProbed(params(signalsToList()));
    sendPackets(0);

    std::array<std::int64_t, 10> ticks{};
    std::array<double, 10> a{};
    std::array<double, 10> b{};
    void* buffers[3]{ticks.data(), a.data(), b.data()};
    SizeT count = 10;
    auto status = reader.readWithDomain(buffers, &count);
    ASSERT_EQ(count, 10u);
    ASSERT_THAT(ticks, ElementsAre(7, 8, 9, 10, 11, 12, 13, 14, 15, 16));
    ASSERT_THAT(a, ElementsAre(0, 1, 2, 3, 4, 5, 6, 7, 8, 9));
}

TEST_F(MultiReader2Test, SkipSamplesAndStatusProbe)
{
    readSignals.reserve(2);
    auto domain = createDomainSignal();
    addSignal(0, 10, domain);
    addSignal(0, 10, domain);
    auto reader = createReader(params(signalsToList()));
    sendPackets(0);
    sendPackets(1);
    ASSERT_EQ(reader.getAvailableCount(), 20u);

    probe(reader);
    ASSERT_EQ(reader.getAvailableCount(), 20u);

    SizeT count = 15;
    auto status = reader.skipSamples(&count);
    ASSERT_EQ(count, 15u);
    ASSERT_EQ(reader.getAvailableCount(), 5u);

    std::array<std::array<double, 5>, 2> values{};
    count = 5;
    SizeT offset = 0;
    read(reader, values, count, &offset);
    ASSERT_EQ(offset, 15u);
    ASSERT_THAT(values[0], ElementsAre(15, 16, 17, 18, 19));
}

TEST_F(MultiReader2Test, MinReadCountGatesReadsAndDropsShortRunsAtBoundaries)
{
    readSignals.reserve(2);
    auto domain = createDomainSignal();
    addSignal(0, 10, domain);
    addSignal(0, 10, domain);
    auto p = params(signalsToList());
    p.setMinReadCount(8);
    auto reader = createReader(p);

    readSignals[0].sendAt(0, 10);
    readSignals[1].sendAt(0, 5);
    scheduler.waitAll();
    ASSERT_EQ(reader.getAvailableCount(), 0u);  // 5 aligned, below the minimum

    std::array<std::array<double, 10>, 2> values{};
    SizeT count = 3;
    SizeT offset = 0;
    void* buffers[2]{values[0].data(), values[1].data()};
    IMultiReader2Status* rawStatus = nullptr;
    ASSERT_EQ(reader->read(buffers, &count, &offset, &rawStatus), OPENDAQ_ERR_INVALIDPARAMETER);
    daqClearErrorInfo();

    count = 10;
    auto status = read(reader, values, count, &offset);
    ASSERT_EQ(count, 0u);  // waiting for data, nothing dropped

    readSignals[1].sendAt(5, 5);
    scheduler.waitAll();
    ASSERT_EQ(reader.getAvailableCount(), 10u);

    // Now a descriptor change on signal 1 after 3 more samples: the 3 are dropped so the boundary can be consumed
    readSignals[1].sendAt(10, 3);
    readSignals[0].sendAt(10, 10);
    count = 10;
    read(reader, values, count, &offset);
    ASSERT_EQ(count, 10u);
    readSignals[1].signal.setDescriptor(setupDescriptor(SampleType::Float32));
    readSignals[1].sendAt<float>(13, 10);
    scheduler.waitAll();

    count = 10;
    status = read(reader, values, count, &offset);
    ASSERT_EQ(count, 0u);
    ASSERT_TRUE(status.getHasChanges());
    ASSERT_TRUE(input(status, 1).getDescriptorChanged());
    ASSERT_EQ(reader.getAvailableCount(), 7u > 8u ? 7u : 0u);  // 7 aligned ticks left (13..19 on signal 0 is 10 .. 19 minus the 3 dropped)
}

TEST_F(MultiReader2Test, DataAndChangesComeInOneRead)
{
    readSignals.reserve(2);
    auto domain = createDomainSignal();
    addSignal(0, 10, domain);
    addSignal(0, 10, domain);
    auto reader = createReader(params(signalsToList()));
    probe(reader);
    sendPackets(0);
    readSignals[1].signal.setDescriptor(DataDescriptorBuilder().setSampleType(SampleType::Float64).setUnit(Unit("V")).build());
    sendPackets(1);

    // The first read delivers the data in front of the boundary and reports the change on the same status
    std::array<std::array<double, 20>, 2> values{};
    SizeT count = 20;
    auto status = read(reader, values, count);
    ASSERT_EQ(count, 10u);
    ASSERT_TRUE(status.getHasChanges());
    ASSERT_TRUE(status.getValid());
    ASSERT_TRUE(input(status, 1).getDescriptorChanged());
    ASSERT_EQ(input(status, 1).getDescriptor().getUnit().getSymbol(), "V");
    ASSERT_FALSE(status.getResynchronized());

    // The data behind it flows in the next read under the new descriptor
    count = 20;
    status = read(reader, values, count);
    ASSERT_EQ(count, 10u);
    ASSERT_FALSE(status.getHasChanges());
    ASSERT_THAT(values[1], Contains(10).Times(1));
}

TEST_F(MultiReader2Test, ConvertsSampleTypes)
{
    readSignals.reserve(2);
    auto domain = createDomainSignal();
    addSignal(0, 4, domain, SampleType::Int32);
    addSignal(0, 4, domain, SampleType::Float64);
    auto p = params(signalsToList(), SampleType::Int16);
    auto reader = createReaderProbed(p);
    readSignals[0].createAndSendPacket<int32_t>(0);
    readSignals[1].createAndSendPacket<double>(0);
    scheduler.waitAll();

    std::array<int16_t, 4> a{};
    std::array<int16_t, 4> b{};
    void* buffers[2]{a.data(), b.data()};
    SizeT count = 4;
    SizeT offset = 0;
    reader.read(buffers, &count, offset);
    ASSERT_EQ(count, 4u);
    ASSERT_THAT(a, ElementsAre(0, 1, 2, 3));
    ASSERT_THAT(b, ElementsAre(0, 1, 2, 3));
}

TEST_F(MultiReader2Test, UndefinedReadTypeCopiesAsIs)
{
    readSignals.reserve(2);
    auto domain = createDomainSignal();
    addSignal(0, 4, domain, SampleType::Int32);
    addSignal(0, 4, domain, SampleType::Float32);
    auto reader = createReaderProbed(params(signalsToList(), SampleType::Undefined));
    readSignals[0].createAndSendPacket<int32_t>(0);
    readSignals[1].createAndSendPacket<float>(0);
    scheduler.waitAll();

    std::array<int32_t, 4> a{};
    std::array<float, 4> b{};
    void* buffers[2]{a.data(), b.data()};
    SizeT count = 4;
    SizeT offset = 0;
    reader.read(buffers, &count, offset);
    ASSERT_EQ(count, 4u);
    ASSERT_THAT(a, ElementsAre(0, 1, 2, 3));
    ASSERT_THAT(b, ElementsAre(0.0f, 1.0f, 2.0f, 3.0f));
}

TEST_F(MultiReader2Test, ComplexValuesAreInvalidUnlessUndefined)
{
    readSignals.reserve(2);
    auto domain = createDomainSignal();
    addSignal(0, 4, domain, SampleType::ComplexFloat32);
    addSignal(0, 4, domain, SampleType::Float64);
    auto reader = createReader(params(signalsToList()));
    scheduler.waitAll();
    auto status = probe(reader);
    ASSERT_FALSE(status.getValid());
    ASSERT_EQ(input(status, 0).getError(), MultiReader2InputError::ValueDescriptorInvalid);

    reader.configure(params(signalsToList(), SampleType::Undefined));
    scheduler.waitAll();
    status = probe(reader);
    ASSERT_TRUE(status.getValid());
}

TEST_F(MultiReader2Test, DimensionedValuesConvertElementWise)
{
    readSignals.reserve(2);
    auto domain = createDomainSignal();
    auto vec = Signal(context, nullptr, "vec");
    vec.setDescriptor(DataDescriptorBuilder().setSampleType(SampleType::Int32).setDimensions(List<IDimension>(Dimension(LinearDimensionRule(1, 0, 2)))).build());
    vec.setDomainSignal(domain);
    readSignals.emplace_back(vec, 0, 3);
    addSignal(0, 3, domain);
    auto reader = createReaderProbed(params(signalsToList()));

    {
        auto domainPacket = DataPacket(domain.getDescriptor(), 3, 0);
        auto packet = DataPacketWithDomain(domainPacket, vec.getDescriptor(), 3);
        auto data = static_cast<int32_t*>(packet.getRawData());
        for (int i = 0; i < 6; i++)
            data[i] = i;
        vec.sendPacket(packet);
    }
    readSignals[1].createAndSendPacket(0);
    scheduler.waitAll();

    std::array<double, 6> a{};
    std::array<double, 3> b{};
    void* buffers[2]{a.data(), b.data()};
    SizeT count = 3;
    SizeT offset = 0;
    reader.read(buffers, &count, offset);
    ASSERT_EQ(count, 3u);
    ASSERT_THAT(a, ElementsAre(0, 1, 2, 3, 4, 5));
}

// ---------------------------------------------------------------- boundaries

TEST_F(MultiReader2Test, MainDomainChangeReconfiguresAndResynchronizes)
{
    readSignals.reserve(2);
    addSignal(0, 10, createDomainSignal());
    addSignal(0, 10, createDomainSignal());
    auto reader = createReaderProbed(params(signalsToList()));
    sendPackets(0);
    std::array<std::array<double, 10>, 2> values{};
    SizeT count = 10;
    read(reader, values, count);
    ASSERT_EQ(count, 10u);

    // The main input moves to a clock one second ahead, at the same rate
    readSignals[0].signal.getDomainSignal().asPtr<ISignalConfig>().setDescriptor(createDomainDescriptor("2022-09-27T00:02:04+00:00"));
    scheduler.waitAll();
    auto status = probe(reader);
    ASSERT_TRUE(status.getHasChanges());
    ASSERT_TRUE(status.getValid());
    ASSERT_TRUE(status.getDomainDescriptorChanged());
    ASSERT_TRUE(status.getResynchronized());
    ASSERT_TRUE(input(status, 0).getDescriptorChanged());
    ASSERT_TRUE(input(status, 1).getDescriptorChanged());
    ASSERT_EQ(status.getDomainDescriptor().getOrigin(), "2022-09-27T00:02:04+00:00");

    // Signal 0 tick 0 is now signal 1 tick 1000
    readSignals[0].sendAt(0, 10);
    readSignals[1].sendAt(1000, 10);
    scheduler.waitAll();
    count = 10;
    SizeT offset = 0;
    read(reader, values, count, &offset);
    ASSERT_EQ(count, 10u);
    ASSERT_EQ(offset, 0u);
    ASSERT_THAT(values[1], ElementsAre(1000, 1001, 1002, 1003, 1004, 1005, 1006, 1007, 1008, 1009));
}

TEST_F(MultiReader2Test, NonMainDomainChangeOnlyResynchronizes)
{
    readSignals.reserve(2);
    addSignal(0, 10, createDomainSignal());
    addSignal(0, 10, createDomainSignal());
    auto reader = createReaderProbed(params(signalsToList()));
    sendPackets(0);
    std::array<std::array<double, 10>, 2> values{};
    SizeT count = 10;
    read(reader, values, count);

    readSignals[1].signal.getDomainSignal().asPtr<ISignalConfig>().setDescriptor(createDomainDescriptor("2022-09-27T00:02:04+00:00"));
    scheduler.waitAll();
    auto status = probe(reader);
    ASSERT_TRUE(status.getHasChanges());
    ASSERT_TRUE(status.getResynchronized());
    ASSERT_FALSE(status.getDomainDescriptorChanged());
    ASSERT_FALSE(input(status, 0).getDescriptorChanged());
    ASSERT_FALSE(input(status, 1).getDescriptorChanged());

    // Signal 1 tick 0 is now signal 0 tick 1000
    readSignals[0].sendAt(1000, 10);
    readSignals[1].sendAt(0, 10);
    scheduler.waitAll();
    count = 10;
    SizeT offset = 0;
    read(reader, values, count, &offset);
    ASSERT_EQ(count, 10u);
    ASSERT_EQ(offset, 1000u);
}

TEST_F(MultiReader2Test, GapResynchronizesAndOffsetJumps)
{
    readSignals.reserve(2);
    auto domain = createDomainSignal();
    addSignal(0, 10, domain);
    addSignal(0, 10, domain);
    auto reader = createReaderProbed(params(signalsToList()));
    sendPackets(0);
    std::array<std::array<double, 10>, 2> values{};
    SizeT count = 10;
    SizeT offset = 0;
    read(reader, values, count, &offset);
    ASSERT_EQ(count, 10u);

    // Signal 1 skips 5 ticks; signal 0 does not
    readSignals[0].sendAt(10, 10);
    readSignals[1].sendAt(15, 10);
    scheduler.waitAll();

    count = 10;
    auto status = read(reader, values, count, &offset);
    ASSERT_EQ(count, 0u);
    ASSERT_TRUE(status.getHasChanges());
    ASSERT_TRUE(status.getResynchronized());
    ASSERT_TRUE(status.getValid());

    // Synchronization lands on tick 15 and the rest flows
    ASSERT_EQ(reader.getAvailableCount(), 5u);
    count = 10;
    status = read(reader, values, count, &offset);
    ASSERT_EQ(count, 5u);
    ASSERT_EQ(offset, 15u);
    ASSERT_FALSE(status.getHasChanges());
    ASSERT_THAT(std::vector<double>(values[0].begin(), values[0].begin() + 5), ElementsAre(15, 16, 17, 18, 19));
}

TEST_F(MultiReader2Test, GapEventPacketResynchronizes)
{
    readSignals.reserve(2);
    auto domain = createDomainSignal();
    addSignal(0, 10, domain);
    addSignal(0, 10, domain);
    auto reader = createReaderProbed(params(signalsToList()));
    sendPackets(0);
    std::array<std::array<double, 10>, 2> values{};
    SizeT count = 10;
    read(reader, values, count);

    readSignals[0].signal.sendPacket(ImplicitDomainGapDetectedEventPacket(5));
    scheduler.waitAll();
    auto status = probe(reader);
    ASSERT_TRUE(status.getHasChanges());
    ASSERT_TRUE(status.getResynchronized());
}

TEST_F(MultiReader2Test, InvalidValueDescriptorInvalidatesAndRecovers)
{
    readSignals.reserve(2);
    auto domain = createDomainSignal();
    addSignal(0, 10, domain);
    addSignal(0, 10, domain);
    auto reader = createReaderProbed(params(signalsToList()));
    sendPackets(0);

    readSignals[1].signal.setDescriptor(setupDescriptor(SampleType::ComplexFloat32));
    scheduler.waitAll();

    // The data in front of the boundary is delivered, the error is reported on the same status
    std::array<std::array<double, 10>, 2> values{};
    SizeT count = 10;
    auto status = read(reader, values, count);
    ASSERT_EQ(count, 10u);
    ASSERT_TRUE(status.getHasChanges());
    ASSERT_FALSE(status.getValid());
    ASSERT_EQ(input(status, 1).getError(), MultiReader2InputError::ValueDescriptorInvalid);
    ASSERT_EQ(input(status, 0).getError(), MultiReader2InputError::None);
    ASSERT_FALSE(input(status, 0).getDescriptor().assigned());  // no descriptors while invalid

    // Data while invalid is dropped
    sendPackets(1);
    ASSERT_EQ(reader.getAvailableCount(), 0u);
    status = probe(reader);
    ASSERT_FALSE(status.getHasChanges());
    ASSERT_FALSE(status.getValid());

    // Recovery: everything is new again and synchronization restarts
    readSignals[1].signal.setDescriptor(setupDescriptor(SampleType::Float64));
    scheduler.waitAll();
    status = probe(reader);
    ASSERT_TRUE(status.getHasChanges());
    ASSERT_TRUE(status.getValid());
    ASSERT_TRUE(status.getResynchronized());
    ASSERT_TRUE(status.getDomainDescriptorChanged());
    ASSERT_TRUE(input(status, 0).getDescriptorChanged());
    ASSERT_TRUE(input(status, 1).getDescriptorChanged());

    sendPackets(2);
    ASSERT_EQ(reader.getAvailableCount(), 10u);
    count = 10;
    SizeT offset = 0;
    read(reader, values, count, &offset);
    ASSERT_EQ(offset, 20u);
}

TEST_F(MultiReader2Test, UnusedInputDoesNotGateButIsReported)
{
    readSignals.reserve(3);
    auto domain = createDomainSignal();
    addSignal(0, 10, domain);
    addSignal(0, 10, domain);
    addSignal(0, 10, domain, SampleType::ComplexFloat32);  // would be invalid if used
    auto p = params(signalsToList());
    p.setInputUsed(readSignals[2].signal, false);
    auto reader = createReaderProbed(p);

    readSignals[0].createAndSendPacket(0);
    readSignals[1].createAndSendPacket(0);
    scheduler.waitAll();
    ASSERT_EQ(reader.getAvailableCount(), 10u);

    std::array<std::array<double, 10>, 3> values{};
    SizeT count = 10;
    auto status = read(reader, values, count);
    ASSERT_EQ(count, 10u);
    ASSERT_TRUE(status.getValid());
    ASSERT_FALSE(input(status, 2).getUsed());
    ASSERT_EQ(input(status, 2).getError(), MultiReader2InputError::ValueDescriptorInvalid);

    // Its descriptor change is reported with the others' data
    readSignals[2].signal.setDescriptor(setupDescriptor(SampleType::Float64));
    sendPackets(1);
    count = 10;
    status = read(reader, values, count);
    ASSERT_EQ(count, 10u);
    ASSERT_TRUE(status.getHasChanges());
    ASSERT_EQ(input(status, 2).getError(), MultiReader2InputError::None);
    ASSERT_TRUE(input(status, 2).getDescriptorChanged());
    ASSERT_EQ(input(status, 2).getDescriptor().getSampleType(), SampleType::Float64);

    // Taking it into use reconfigures; it then delivers
    p.setInputUsed(readSignals[2].signal, true);
    reader.configure(p);
    probe(reader);
    sendPackets(2);
    count = 10;
    status = read(reader, values, count);
    ASSERT_EQ(count, 10u);
    ASSERT_TRUE(input(status, 2).getUsed());
    ASSERT_THAT(values[2], ElementsAre(20, 21, 22, 23, 24, 25, 26, 27, 28, 29));
}

TEST_F(MultiReader2Test, SetUsedFalseReadsNothingButStaysValid)
{
    readSignals.reserve(2);
    auto domain = createDomainSignal();
    addSignal(0, 10, domain);
    addSignal(0, 10, domain);
    auto p = params(signalsToList());
    p.setUsed(false);
    auto reader = createReader(p);
    sendPackets(0);
    ASSERT_EQ(reader.getAvailableCount(), 0u);
    auto status = probe(reader);
    ASSERT_TRUE(status.getValid());
    ASSERT_FALSE(input(status, 0).getUsed());
    ASSERT_EQ(reader.getMainInput(), "");
}

// ---------------------------------------------------------------- input ports

TEST_F(MultiReader2Test, DisconnectedPortIsAnError)
{
    readSignals.reserve(2);
    auto domain = createDomainSignal();
    addSignal(0, 10, domain);
    addSignal(0, 10, domain);
    auto reader = createReader(params(portsList()));
    scheduler.waitAll();

    auto status = probe(reader);
    ASSERT_FALSE(status.getValid());
    ASSERT_EQ(input(status, 0).getError(), MultiReader2InputError::Disconnected);
    ASSERT_EQ(input(status, 1).getError(), MultiReader2InputError::Disconnected);
    ASSERT_EQ(reader.getMainInput(), "");

    // Connecting clears the errors and the reader recovers with everything new
    for (SizeT i = 0; i < ports.size(); i++)
        ports[i].connect(readSignals[i].signal);
    scheduler.waitAll();
    status = probe(reader);
    ASSERT_TRUE(status.getHasChanges());
    ASSERT_TRUE(status.getValid());
    ASSERT_EQ(input(status, 0).getError(), MultiReader2InputError::None);
    ASSERT_TRUE(input(status, 0).getDescriptorChanged());
    ASSERT_EQ(reader.getMainInput(), ports[0].getGlobalId());

    sendPackets(0);
    ASSERT_EQ(reader.getAvailableCount(), 10u);
}

TEST_F(MultiReader2Test, UnusedFreePortDoesNotGate)
{
    readSignals.reserve(2);
    auto domain = createDomainSignal();
    addSignal(0, 10, domain);
    addSignal(0, 10, domain);
    portsList();
    auto freePort = InputPort(context, nullptr, "free");
    auto p = params(List<IComponent>(ports[0], ports[1], freePort));
    p.setInputUsed(freePort, false);
    auto reader = createReader(p);
    connectAll(reader);
    sendPackets(0);

    std::array<std::array<double, 10>, 3> values{};
    SizeT count = 10;
    auto status = read(reader, values, count);
    ASSERT_EQ(count, 10u);
    ASSERT_TRUE(status.getValid());
    ASSERT_EQ(status.getInputStatus(freePort).getError(), MultiReader2InputError::Disconnected);
    ASSERT_FALSE(status.getInputStatus(freePort).getUsed());

    // A signal connects to the free port: the error clears, the descriptor is reported, nothing else moves
    auto third = Signal(context, nullptr, "third");
    third.setDescriptor(setupDescriptor(SampleType::Float64));
    third.setDomainSignal(domain);
    freePort.connect(third);
    sendPackets(1);
    count = 10;
    status = read(reader, values, count);
    ASSERT_EQ(count, 10u);
    ASSERT_TRUE(status.getHasChanges());
    ASSERT_EQ(status.getInputStatus(freePort).getError(), MultiReader2InputError::None);
    ASSERT_TRUE(status.getInputStatus(freePort).getDescriptorChanged());
    ASSERT_FALSE(status.getResynchronized());
}

TEST_F(MultiReader2Test, DisconnectDeliversQueuedDataThenInvalidates)
{
    readSignals.reserve(2);
    auto domain = createDomainSignal();
    addSignal(0, 10, domain);
    addSignal(0, 10, domain);
    auto reader = createReader(params(portsList()));
    connectAll(reader);
    sendPackets(0);
    ports[1].disconnect();
    scheduler.waitAll();

    std::array<std::array<double, 10>, 2> values{};
    SizeT count = 10;
    auto status = read(reader, values, count);
    ASSERT_EQ(count, 10u);
    ASSERT_FALSE(status.getValid());
    ASSERT_EQ(input(status, 1).getError(), MultiReader2InputError::Disconnected);

    ports[1].connect(readSignals[1].signal);
    scheduler.waitAll();
    status = probe(reader);
    ASSERT_TRUE(status.getValid());
    ASSERT_TRUE(status.getResynchronized());
    sendPackets(1);
    ASSERT_EQ(reader.getAvailableCount(), 10u);
}

TEST_F(MultiReader2Test, ExcludeSetsAsideAndTakesBack)
{
    readSignals.reserve(3);
    auto domain = createDomainSignal();
    addSignal(0, 10, domain);
    addSignal(0, 10, domain);
    addSignal(0, 10, domain);
    auto p = params(portsList());
    p.setErrorPolicy(MultiReader2ErrorPolicy::Exclude);
    auto reader = createReader(p);
    connectAll(reader);
    sendPackets(0);

    std::array<std::array<double, 10>, 3> values{};
    SizeT count = 10;
    auto status = read(reader, values, count);
    ASSERT_EQ(count, 10u);

    // Port 2 loses its signal: the read that reaches the disconnect reports it and sets the input aside; the data
    // the others hold beyond that point flows from the next read on, as in the drain loop
    ports[2].disconnect();
    scheduler.waitAll();
    sendPackets(1);
    count = 10;
    status = read(reader, values, count);
    ASSERT_EQ(count, 0u);
    ASSERT_TRUE(status.getHasChanges());
    ASSERT_TRUE(status.getValid());
    ASSERT_EQ(input(status, 2).getError(), MultiReader2InputError::Disconnected);
    ASSERT_TRUE(input(status, 2).getUsed());
    ASSERT_FALSE(status.getResynchronized());

    count = 10;
    status = read(reader, values, count);
    ASSERT_EQ(count, 10u);
    ASSERT_FALSE(status.getHasChanges());
    ASSERT_THAT(values[0], ElementsAre(10, 11, 12, 13, 14, 15, 16, 17, 18, 19));

    sendPackets(2);
    count = 10;
    status = read(reader, values, count);
    ASSERT_EQ(count, 10u);
    ASSERT_FALSE(status.getHasChanges());

    // It comes back healthy: a resynchronization and its descriptor marked new
    ports[2].connect(readSignals[2].signal);
    scheduler.waitAll();
    status = probe(reader);
    ASSERT_TRUE(status.getHasChanges());
    ASSERT_TRUE(status.getValid());
    ASSERT_EQ(input(status, 2).getError(), MultiReader2InputError::None);
    ASSERT_TRUE(input(status, 2).getDescriptorChanged());
    ASSERT_TRUE(status.getResynchronized());

    sendPackets(3);
    count = 10;
    SizeT offset = 0;
    status = read(reader, values, count, &offset);
    ASSERT_EQ(count, 10u);
    ASSERT_EQ(offset, 30u);
    ASSERT_THAT(values[2], ElementsAre(30, 31, 32, 33, 34, 35, 36, 37, 38, 39));
}

TEST_F(MultiReader2Test, ExcludeIsInvalidWhenNoUsedInputIsHealthyOrPinnedMainErrors)
{
    readSignals.reserve(2);
    auto domain = createDomainSignal();
    addSignal(0, 10, domain);
    addSignal(0, 10, domain);
    auto p = params(portsList());
    p.setErrorPolicy(MultiReader2ErrorPolicy::Exclude);
    auto reader = createReader(p);
    scheduler.waitAll();
    auto status = probe(reader);
    ASSERT_FALSE(status.getValid());

    ports[1].connect(readSignals[1].signal);
    scheduler.waitAll();
    status = probe(reader);
    ASSERT_TRUE(status.getValid());
    ASSERT_EQ(reader.getMainInput(), ports[1].getGlobalId());

    // Pin the disconnected port as main: its error invalidates
    p.setMainInput(ports[0]);
    reader.configure(p);
    scheduler.waitAll();
    status = probe(reader);
    ASSERT_FALSE(status.getValid());
    ASSERT_EQ(reader.getMainInput(), ports[0].getGlobalId());
}

TEST_F(MultiReader2Test, AutomaticMainMovesWhenItErrors)
{
    readSignals.reserve(2);
    auto domain = createDomainSignal();
    addSignal(0, 10, domain);
    addSignal(0, 10, domain);
    auto p = params(portsList());
    p.setErrorPolicy(MultiReader2ErrorPolicy::Exclude);
    auto reader = createReader(p);
    connectAll(reader);
    sendPackets(0);
    std::array<std::array<double, 10>, 2> values{};
    SizeT count = 10;
    read(reader, values, count);
    ASSERT_EQ(reader.getMainInput(), ports[0].getGlobalId());

    ports[0].disconnect();
    scheduler.waitAll();
    auto status = probe(reader);
    ASSERT_TRUE(status.getValid());
    ASSERT_TRUE(status.getDomainDescriptorChanged());
    ASSERT_TRUE(status.getResynchronized());
    ASSERT_EQ(reader.getMainInput(), ports[1].getGlobalId());

    readSignals[1].createAndSendPacket(1);
    scheduler.waitAll();
    ASSERT_EQ(reader.getAvailableCount(), 10u);
}

TEST_F(MultiReader2Test, PortsKeepTheirOwnerAndListenerChanges)
{
    readSignals.reserve(2);
    auto domain = createDomainSignal();
    addSignal(0, 10, domain);
    addSignal(0, 10, domain);
    auto list = portsList();
    {
        auto reader = createReader(params(list));
        ASSERT_EQ(ports[0].getListener(), reader.asPtr<IInputPortNotifications>());
        ASSERT_EQ(ports[0].getNotificationMethod(), PacketReadyNotification::Scheduler);
    }
    ASSERT_FALSE(ports[0].getListener().assigned());
}

// ---------------------------------------------------------------- wake

TEST_F(MultiReader2Test, OnDataAvailableFiresForDataAndChanges)
{
    readSignals.reserve(2);
    auto domain = createDomainSignal();
    addSignal(0, 10, domain);
    addSignal(0, 10, domain);
    auto reader = createReader(params(signalsToList()));
    scheduler.waitAll();

    std::atomic<int> wakes{0};
    reader.getOnDataAvailable() += [&wakes](InputPortPtr&, EventArgsPtr<>&) { wakes++; };

    // The first status after configure is pending, but the wake for it fired before the subscription
    probe(reader);
    sendPackets(0);
    ASSERT_GE(wakes.load(), 1);
    const int afterData = wakes.load();

    std::array<std::array<double, 10>, 2> values{};
    SizeT count = 10;
    read(reader, values, count);
    ASSERT_EQ(count, 10u);

    // A descriptor change wakes even without data
    readSignals[0].signal.setDescriptor(DataDescriptorBuilder().setSampleType(SampleType::Float64).setUnit(Unit("A")).build());
    scheduler.waitAll();
    ASSERT_GT(wakes.load(), afterData);
}

TEST_F(MultiReader2Test, ConfigureFromInsideTheHandler)
{
    readSignals.reserve(3);
    auto domain = createDomainSignal();
    addSignal(0, 10, domain);
    addSignal(0, 10, domain);
    addSignal(0, 10, domain, SampleType::ComplexFloat32);
    auto p = params(signalsToList());
    auto reader = createReader(p);
    scheduler.waitAll();

    std::atomic<int> reconfigured{0};
    reader.getOnDataAvailable() += [&](InputPortPtr&, EventArgsPtr<>&)
    {
        auto status = probe(reader);
        if (!status.getValid() && reconfigured == 0)
        {
            // Set the offending input unused from within the handler
            p.setInputUsed(readSignals[2].signal, false);
            reader.configure(p);
            reconfigured++;
        }
    };

    // Nothing wakes while invalid but a boundary; a harmless descriptor change on signal 0 is one
    readSignals[0].signal.setDescriptor(DataDescriptorBuilder().setSampleType(SampleType::Float64).setUnit(Unit("V")).build());
    scheduler.waitAll();
    ASSERT_EQ(reconfigured.load(), 1);
    sendPackets(1);
    auto status = probe(reader);
    ASSERT_TRUE(status.getValid());
    ASSERT_FALSE(input(status, 2).getUsed());
}

// ---------------------------------------------------------------- synchronization limits

TEST_F(MultiReader2Test, SilentInputFailsSynchronizationAfterTheDeadlineAndRecovers)
{
    readSignals.reserve(2);
    auto domain = createDomainSignal();
    addSignal(0, 10, domain);
    addSignal(0, 10, domain);
    auto reader = createReader(params(signalsToList()));
    manager(reader).setSyncLimits(std::chrono::milliseconds(50), std::chrono::milliseconds(2000));
    scheduler.waitAll();
    probe(reader);

    readSignals[0].createAndSendPacket(0);
    scheduler.waitAll();
    std::this_thread::sleep_for(std::chrono::milliseconds(80));
    readSignals[0].createAndSendPacket(1);
    scheduler.waitAll();

    auto status = probe(reader);
    ASSERT_TRUE(status.getHasChanges());
    ASSERT_FALSE(status.getValid());
    ASSERT_EQ(input(status, 1).getError(), MultiReader2InputError::SyncFailed);
    ASSERT_EQ(input(status, 0).getError(), MultiReader2InputError::None);

    // Data on the silent input is its way back
    readSignals[1].createAndSendPacket(1);
    scheduler.waitAll();
    status = probe(reader);
    ASSERT_TRUE(status.getValid());
    ASSERT_EQ(input(status, 1).getError(), MultiReader2InputError::None);
    sendPackets(2);
    ASSERT_EQ(reader.getAvailableCount(), 10u);
}

TEST_F(MultiReader2Test, InputTooFarBehindFailsSynchronization)
{
    readSignals.reserve(2);
    auto domain = createDomainSignal();
    addSignal(0, 10, domain);
    addSignal(5000, 10, domain);  // five seconds ahead at 1 kHz
    auto p = params(signalsToList());
    p.setErrorPolicy(MultiReader2ErrorPolicy::Exclude);
    auto reader = createReader(p);
    manager(reader).setSyncLimits(std::chrono::milliseconds(2000), std::chrono::milliseconds(1000));
    sendPackets(0);

    auto status = probe(reader);
    ASSERT_TRUE(status.getValid());
    ASSERT_EQ(input(status, 0).getError(), MultiReader2InputError::SyncFailed);
    ASSERT_EQ(input(status, 1).getError(), MultiReader2InputError::None);
    ASSERT_EQ(reader.getMainInput(), readSignals[1].signal.getGlobalId());
}

// The run behind a boundary is counted whole once the boundary is consumed, however long the queue got
TEST_F(MultiReader2Test, RunBehindBoundaryIsCountedWhole)
{
    readSignals.reserve(2);
    auto domain = createDomainSignal();
    addSignal(0, 10, domain);
    addSignal(0, 10, domain);
    auto reader = createReaderProbed(params(signalsToList()));
    for (Int i = 0; i < 100; i++)
        sendPackets(i);

    readSignals[0].signal.setDescriptor(setupDescriptor(SampleType::Float32));
    for (Int i = 100; i < 200; i++)
    {
        readSignals[0].createAndSendPacket<float>(i);
        readSignals[1].createAndSendPacket(i);
    }
    scheduler.waitAll();

    ASSERT_EQ(reader.getAvailableCount(), 1000u);
    std::array<std::array<double, 1000>, 2> values{};
    SizeT count = 1000;
    auto status = read(reader, values, count);
    ASSERT_EQ(count, 1000u);
    ASSERT_EQ(values[0][999], 999);
    // The run ended exactly at the boundary, so the change rides along with the data that precedes it
    ASSERT_TRUE(status.getHasChanges());
    ASSERT_TRUE(input(status, 0).getDescriptorChanged());

    ASSERT_EQ(reader.getAvailableCount(), 1000u);
    count = 1000;
    status = read(reader, values, count);
    ASSERT_EQ(count, 1000u);
    ASSERT_FALSE(status.getHasChanges());
    ASSERT_EQ(values[0][0], 1000);
    ASSERT_EQ(values[1][999], 1999);
}
