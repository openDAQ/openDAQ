#include <opendaq/event_packet_params.h>
#include <opendaq/opendaq.h>
#include <ref_fb_module/module_dll.h>
#include <gtest/gtest.h>

#include <cmath>

using namespace daq;

namespace
{

struct Output
{
    std::vector<double> values;
    std::vector<Int> ticks;  // the domain value of every sample
    DataDescriptorPtr descriptor;
    DataDescriptorPtr domain;
};

// Drains a packet reader: the data, each sample's domain tick, and the last descriptors announced
Output drainOutput(const PacketReaderPtr& reader)
{
    Output out;
    while (reader.getAvailableCount() > 0)
    {
        const auto packet = reader.read();
        if (const auto event = packet.asPtrOrNull<IEventPacket>(); event.assigned())
        {
            const auto params = event.getParameters();
            if (const DataDescriptorPtr value = params.get(event_packet_param::DATA_DESCRIPTOR); value.assigned())
                out.descriptor = value;
            if (const DataDescriptorPtr domain = params.get(event_packet_param::DOMAIN_DATA_DESCRIPTOR); domain.assigned())
                out.domain = domain;
            continue;
        }
        const auto data = packet.asPtr<IDataPacket>();
        const auto* values = static_cast<const double*>(data.getData());
        const auto* ticks = static_cast<const Int*>(data.getDomainPacket().getData());
        for (SizeT i = 0; i < data.getSampleCount(); ++i)
        {
            out.values.push_back(values[i]);
            out.ticks.push_back(ticks[i]);
        }
    }
    return out;
}

void expectRange(const DataDescriptorPtr& descriptor, double low, double high)
{
    const auto range = descriptor.getValueRange();
    ASSERT_TRUE(range.assigned());
    ASSERT_DOUBLE_EQ(range.getLowValue(), low);
    ASSERT_DOUBLE_EQ(range.getHighValue(), high);
}

}

class BlockStatisticsReaderTest : public testing::Test
{
protected:
    ContextPtr context;
    ModulePtr module;
    FunctionBlockPtr fb;
    DataDescriptorPtr timeDescriptor;
    SignalConfigPtr time;
    std::vector<SignalConfigPtr> inputs;

    void SetUp() override
    {
        const auto logger = Logger();
        context = Context(Scheduler(logger), logger, TypeManager(), nullptr, nullptr);
        createModule(&module, context);
        fb = module.createFunctionBlock("RefFBModuleBlockStatisticsReader", nullptr, "fb");
        setTime(1, 0);
    }

    void setTime(Int delta, Int start)
    {
        timeDescriptor = DataDescriptorBuilder()
                             .setSampleType(SampleType::Int64)
                             .setTickResolution(Ratio(1, 1000))
                             .setOrigin("1970-01-01T00:00:00Z")
                             .setRule(LinearDataRule(delta, start))
                             .setUnit(Unit("s", -1, "seconds", "time"))
                             .build();
        if (!time.assigned())
            time = SignalWithDescriptor(context, timeDescriptor, nullptr, "time");
        else
            time.setDescriptor(timeDescriptor);
    }

    // A volt input with a +/-10 V range, connected to the block's free port
    SignalConfigPtr connectInput()
    {
        const auto descriptor = DataDescriptorBuilder()
                                    .setSampleType(SampleType::Float64)
                                    .setUnit(Unit("V", -1, "volts", "voltage"))
                                    .setValueRange(Range(-10, 10))
                                    .build();
        auto signal = SignalWithDescriptor(context, descriptor, nullptr, fmt::format("in{}", inputs.size()));
        signal.setDomainSignal(time);
        fb.getInputPorts()[fb.getInputPorts().getCount() - 1].connect(signal);
        inputs.push_back(signal);
        return signal;
    }

    // One packet per input; sample i of input k is values(k, i)
    template <typename F>
    void send(SizeT count, Int offset, F values)
    {
        const auto domainPacket = DataPacket(timeDescriptor, count, offset);
        time.sendPacket(domainPacket);
        for (SizeT k = 0; k < inputs.size(); ++k)
        {
            const auto packet = DataPacketWithDomain(domainPacket, inputs[k].getDescriptor(), count);
            auto* data = static_cast<double*>(packet.getRawData());
            for (SizeT i = 0; i < count; ++i)
                data[i] = values(k, i);
            inputs[k].sendPacket(packet);
        }
        context.getScheduler().waitAll();
    }

    SignalPtr output(SizeT input) const
    {
        return fb.getSignals().getItemAt(input);  // the domain signal is hidden
    }

    ComponentStatus status() const
    {
        return static_cast<ComponentStatus>(static_cast<Int>(fb.getStatusContainer().getStatus("ComponentStatus").getIntValue()));
    }
};

TEST_F(BlockStatisticsReaderTest, CreateOffersOneFreePortAndAHiddenDomain)
{
    ASSERT_EQ(fb.getFunctionBlockType().getId(), "RefFBModuleBlockStatisticsReader");
    ASSERT_EQ(status(), ComponentStatus::Ok);
    ASSERT_EQ(fb.getInputPorts().getCount(), 1u);
    ASSERT_EQ(fb.getInputPorts()[0].getLocalId(), "Input_1");
    ASSERT_EQ(fb.getSignals(search::Any()).getCount(), 1u);
    ASSERT_EQ(fb.getPropertyValue("BlockSize"), 100);
    ASSERT_EQ(fb.getPropertySelectionValue("Statistic"), "RMS");
}

TEST_F(BlockStatisticsReaderTest, ConnectPublishesOneStatisticPerInput)
{
    connectInput();
    connectInput();
    ASSERT_EQ(status(), ComponentStatus::Ok);
    ASSERT_EQ(fb.getInputPorts().getCount(), 3u);
    ASSERT_EQ(output(0).getLocalId(), "Statistic_1");
    ASSERT_EQ(output(1).getLocalId(), "Statistic_2");
    ASSERT_EQ(output(0).getDomainSignal().getLocalId(), "StatisticsDomain");
    ASSERT_EQ(output(0).getDescriptor().getUnit().getSymbol(), "V");
    expectRange(output(0).getDescriptor(), 0, 10);
}

// Blocks start on the samples whose index is a multiple of BlockSize; the partial blocks at either end yield nothing
TEST_F(BlockStatisticsReaderTest, BlocksAlignToTheSampleIndexFromTheOrigin)
{
    fb.setPropertyValue("BlockSize", 10);
    connectInput();
    connectInput();
    const auto reader0 = PacketReader(output(0));
    const auto reader1 = PacketReader(output(1));

    send(35, 3, [](SizeT k, SizeT i) { return k == 0 ? 3.0 : (i % 2 ? -2.0 : 2.0); });

    const auto out0 = drainOutput(reader0);
    const auto out1 = drainOutput(reader1);
    ASSERT_EQ(out0.ticks, (std::vector<Int>{10, 20}));
    ASSERT_EQ(out0.values, (std::vector<double>{3, 3}));
    ASSERT_EQ(out1.values, (std::vector<double>{2, 2}));

    const auto rule = out0.domain.getRule().getParameters();
    ASSERT_EQ(rule.get("delta"), 10);
    ASSERT_EQ(rule.get("start"), 0);
    ASSERT_EQ(out0.domain.getTickResolution(), Ratio(1, 1000));
    ASSERT_EQ(out0.domain.getOrigin(), "1970-01-01T00:00:00Z");
    ASSERT_EQ(out0.domain.getUnit().getSymbol(), "s");
}

// With delta 2 and start 1, the ticks 5, 7, 9 are the samples 2, 3, 4: the first block of 4 starts on tick 9
TEST_F(BlockStatisticsReaderTest, AlignmentFollowsTheRuleDeltaAndStart)
{
    setTime(2, 1);
    fb.setPropertyValue("BlockSize", 4);
    connectInput();
    const auto reader = PacketReader(output(0));

    send(14, 4, [](SizeT, SizeT) { return 1.0; });

    const auto out = drainOutput(reader);
    ASSERT_EQ(out.ticks, (std::vector<Int>{9, 17, 25}));
    ASSERT_EQ(out.domain.getRule().getParameters().get("delta"), 8);
    ASSERT_EQ(out.domain.getRule().getParameters().get("start"), 1);
}

TEST_F(BlockStatisticsReaderTest, EveryStatisticOnAKnownBlock)
{
    // 1, 2, 3, 4: mean 2.5, RMS sqrt(7.5), AC RMS sqrt(1.25), peak-to-peak 3, crest factor 4 / sqrt(7.5)
    const std::vector<std::pair<std::string, double>> expected = {
        {"Mean", 2.5}, {"RMS", std::sqrt(7.5)}, {"AC RMS", std::sqrt(1.25)}, {"Peak-to-peak", 3}, {"Crest factor", 4 / std::sqrt(7.5)}};
    for (SizeT s = 0; s < expected.size(); ++s)
    {
        SCOPED_TRACE(expected[s].first);
        fb = module.createFunctionBlock("RefFBModuleBlockStatisticsReader", nullptr, fmt::format("fb{}", s));
        inputs.clear();
        fb.setPropertyValue("BlockSize", 4);
        fb.setPropertyValue("Statistic", static_cast<Int>(s));
        connectInput();
        const auto reader = PacketReader(output(0));

        send(8, 0, [](SizeT, SizeT i) { return static_cast<double>(i % 4 + 1); });

        const auto out = drainOutput(reader);
        ASSERT_EQ(out.values.size(), 2u);
        ASSERT_NEAR(out.values[0], expected[s].second, 1e-12);
        ASSERT_NEAR(out.values[1], expected[s].second, 1e-12);
        ASSERT_EQ(out.descriptor.getName(), expected[s].first);
    }
}

TEST_F(BlockStatisticsReaderTest, UnitsAndRangesFollowTheStatistic)
{
    fb.setPropertyValue("BlockSize", 16);
    connectInput();
    const auto descriptor = [this] { return output(0).getDescriptor(); };

    fb.setPropertyValue("Statistic", 0);
    expectRange(descriptor(), -10, 10);
    fb.setPropertyValue("Statistic", 2);
    expectRange(descriptor(), 0, 10);
    ASSERT_EQ(descriptor().getUnit().getSymbol(), "V");
    fb.setPropertyValue("Statistic", 3);
    expectRange(descriptor(), 0, 20);
    fb.setPropertyValue("Statistic", 4);
    ASSERT_FALSE(descriptor().getUnit().assigned());
    expectRange(descriptor(), 1, 4);
}

// A gap ends the block it falls into: the samples before it are dropped and the next block starts on a boundary
TEST_F(BlockStatisticsReaderTest, AGapDropsThePartialBlock)
{
    fb.setPropertyValue("BlockSize", 10);
    connectInput();
    const auto reader = PacketReader(output(0));

    send(15, 0, [](SizeT, SizeT) { return 1.0; });
    send(25, 105, [](SizeT, SizeT) { return 5.0; });

    const auto out = drainOutput(reader);
    ASSERT_EQ(out.ticks, (std::vector<Int>{0, 110, 120}));
    ASSERT_EQ(out.values, (std::vector<double>{1, 5, 5}));
}

TEST_F(BlockStatisticsReaderTest, BlockSizeSetsTheOutputRate)
{
    connectInput();
    const auto reader = PacketReader(output(0));
    send(200, 0, [](SizeT, SizeT) { return 1.0; });
    fb.setPropertyValue("BlockSize", 50);
    send(200, 200, [](SizeT, SizeT) { return 2.0; });

    const auto out = drainOutput(reader);
    ASSERT_EQ(out.ticks, (std::vector<Int>{0, 100, 200, 250, 300, 350}));
    ASSERT_EQ(out.values, (std::vector<double>{1, 1, 2, 2, 2, 2}));
    ASSERT_EQ(out.domain.getRule().getParameters().get("delta"), 50);
}

// An input set aside by the reader yields NaN while the others go on, starting with the block its error fell into
TEST_F(BlockStatisticsReaderTest, AnExcludedInputYieldsNaN)
{
    fb.setPropertyValue("BlockSize", 10);
    connectInput();
    const auto rejected = connectInput();
    const auto reader0 = PacketReader(output(0));
    const auto reader1 = PacketReader(output(1));

    send(15, 0, [](SizeT, SizeT) { return 1.0; });
    rejected.setDescriptor(DataDescriptorBuilder().setSampleType(SampleType::ComplexFloat32).build());
    inputs.pop_back();
    send(25, 15, [](SizeT, SizeT) { return 1.0; });

    ASSERT_EQ(status(), ComponentStatus::Warning);
    ASSERT_EQ(fb.getStatusContainer().getStatusMessage("ComponentStatus").toStdString().rfind("Input_2: ", 0), 0u);
    const auto out0 = drainOutput(reader0);
    const auto out1 = drainOutput(reader1);
    ASSERT_EQ(out0.values, (std::vector<double>{1, 1, 1, 1}));
    ASSERT_EQ(out1.values.size(), 4u);
    ASSERT_EQ(out1.values[0], 1);
    for (SizeT i = 1; i < 4; ++i)
        ASSERT_TRUE(std::isnan(out1.values[i]));
}

TEST_F(BlockStatisticsReaderTest, DisconnectRemovesThePortAndItsOutput)
{
    connectInput();
    connectInput();
    ASSERT_EQ(fb.getSignals(search::Any()).getCount(), 3u);

    fb.getInputPorts()[0].disconnect();
    context.getScheduler().waitAll();

    ASSERT_EQ(fb.getInputPorts().getCount(), 2u);
    ASSERT_EQ(fb.getSignals(search::Any()).getCount(), 2u);
    ASSERT_EQ(output(0).getLocalId(), "Statistic_2");
    ASSERT_EQ(status(), ComponentStatus::Ok);
}

// With reference domain offset 5, the local ticks 5 and 15 are the reference ticks 10 and 20, where the blocks of 10 start
TEST_F(BlockStatisticsReaderTest, BlocksAlignOnTheReferenceDomain)
{
    timeDescriptor = DataDescriptorBuilderCopy(timeDescriptor)
                         .setReferenceDomainInfo(ReferenceDomainInfoBuilder().setReferenceDomainOffset(5).build())
                         .build();
    time.setDescriptor(timeDescriptor);
    fb.setPropertyValue("BlockSize", 10);
    connectInput();
    const auto reader = PacketReader(output(0));

    send(30, 0, [](SizeT, SizeT) { return 1.0; });

    const auto out = drainOutput(reader);
    ASSERT_EQ(out.ticks, (std::vector<Int>{10, 20}));
}

// A contributing input's new descriptor at sample 15 drops the block 10..19 for every input
TEST_F(BlockStatisticsReaderTest, ADescriptorChangeDropsThePartialBlock)
{
    fb.setPropertyValue("BlockSize", 10);
    connectInput();
    const auto changed = connectInput();
    const auto reader0 = PacketReader(output(0));
    const auto reader1 = PacketReader(output(1));

    send(15, 0, [](SizeT, SizeT) { return 1.0; });
    changed.setDescriptor(DataDescriptorBuilderCopy(changed.getDescriptor()).setUnit(Unit("mV", -1, "millivolts", "voltage")).build());
    send(25, 15, [](SizeT, SizeT) { return 1.0; });

    ASSERT_EQ(status(), ComponentStatus::Ok);
    ASSERT_EQ(drainOutput(reader0).ticks, (std::vector<Int>{0, 20, 30}));
    ASSERT_EQ(drainOutput(reader1).ticks, (std::vector<Int>{0, 20, 30}));
}
