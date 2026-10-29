#include <opendaq/opendaq.h>
#include <ref_fb_module/module_dll.h>
#include <gtest/gtest.h>

#include <cmath>

using namespace daq;

class BridgeReaderTest : public testing::Test
{
protected:
    ContextPtr context;
    ModulePtr module;
    FunctionBlockPtr fb;
    DataDescriptorPtr timeDescriptor;
    SignalConfigPtr time;
    SignalConfigPtr bridge;
    SignalConfigPtr excitation;
    Int nextOffset = 0;

    void SetUp() override
    {
        const auto logger = Logger();
        context = Context(Scheduler(logger), logger, TypeManager(), nullptr, nullptr);
        createModule(&module, context);
        fb = module.createFunctionBlock("RefFBModuleBridgeReader", nullptr, "fb");

        timeDescriptor = DataDescriptorBuilder()
                             .setSampleType(SampleType::Int64)
                             .setTickResolution(Ratio(1, 1000000))
                             .setOrigin("1970-01-01T00:00:00Z")
                             .setRule(LinearDataRule(1000, 7))
                             .setUnit(Unit("s", -1, "seconds", "time"))
                             .build();
        time = SignalWithDescriptor(context, timeDescriptor, nullptr, "time");
        bridge = SignalWithDescriptor(context, voltage("mV", -10, 10), nullptr, "bridge");
        bridge.setDomainSignal(time);
        excitation = SignalWithDescriptor(context, voltage("V", 4, 6), nullptr, "excitation");
        excitation.setDomainSignal(time);
    }

    static DataDescriptorPtr voltage(const std::string& symbol, double low, double high)
    {
        return DataDescriptorBuilder().setSampleType(SampleType::Float64).setUnit(Unit(symbol)).setValueRange(Range(low, high)).build();
    }

    void connect()
    {
        fb.getInputPorts()[0].connect(bridge);
        fb.getInputPorts()[1].connect(excitation);
        context.getScheduler().waitAll();  // the reader reports a connect from its handler
    }

    // `count` samples of a constant bridge output and excitation; returns the block's output
    std::vector<double> run(double bridgeValue, double excitationValue, SizeT count = 10)
    {
        const auto reader = PacketReader(fb.getSignals()[0]);
        const auto domainPacket = DataPacket(timeDescriptor, count, nextOffset);
        nextOffset += static_cast<Int>(count) * 1000;
        time.sendPacket(domainPacket);
        for (const auto& [signal, value] : {std::make_pair(bridge, bridgeValue), std::make_pair(excitation, excitationValue)})
        {
            const auto packet = DataPacketWithDomain(domainPacket, signal.getDescriptor(), count);
            std::fill_n(static_cast<double*>(packet.getRawData()), count, value);
            signal.sendPacket(packet);
        }
        context.getScheduler().waitAll();

        std::vector<double> out;
        while (reader.getAvailableCount() > 0)
            if (const auto data = reader.read().asPtrOrNull<IDataPacket>(); data.assigned())
                out.insert(out.end(), static_cast<const double*>(data.getData()), static_cast<const double*>(data.getData()) + data.getSampleCount());
        return out;
    }

    ComponentStatus status() const
    {
        return static_cast<ComponentStatus>(static_cast<Int>(fb.getStatusContainer().getStatus("ComponentStatus").getIntValue()));
    }
};

TEST_F(BridgeReaderTest, CreateWaitsForBothPorts)
{
    ASSERT_EQ(fb.getFunctionBlockType().getId(), "RefFBModuleBridgeReader");
    ASSERT_EQ(fb.getInputPorts()[0].getLocalId(), "Signal");
    ASSERT_EQ(fb.getInputPorts()[1].getLocalId(), "Excitation");
    ASSERT_EQ(status(), ComponentStatus::Warning);
    ASSERT_EQ(fb.getStatusContainer().getStatusMessage("ComponentStatus"), "Signal: not connected; Excitation: not connected");
}

// 1 mV over 5 V is 0.2 mV/V; a quarter bridge with gauge factor 2 reads -4 * 2e-4 / (2 * (1 + 4e-4)) = -399.84 um/m
TEST_F(BridgeReaderTest, QuarterBridgeStrain)
{
    connect();
    ASSERT_EQ(status(), ComponentStatus::Ok);
    const auto out = run(1, 5);
    ASSERT_EQ(out.size(), 10u);
    for (const double strain : out)
        ASSERT_NEAR(strain, -4 * 2e-4 / (2 * (1 + 4e-4)) * 1e6, 1e-9);

    const auto descriptor = fb.getSignals()[0].getDescriptor();
    ASSERT_EQ(descriptor.getUnit().getSymbol().toStdString(), "\xC2\xB5m/m");
    ASSERT_EQ(descriptor.getUnit().getQuantity(), "strain");
    ASSERT_EQ(fb.getSignals()[0].getDomainSignal().getDescriptor(), timeDescriptor);
}

TEST_F(BridgeReaderTest, BridgeTypesZeroOffsetAndRatio)
{
    connect();
    fb.setPropertyValue("BridgeType", 1);
    ASSERT_NEAR(run(1, 5)[0], -200, 1e-9);
    fb.setPropertyValue("BridgeType", 2);
    ASSERT_NEAR(run(1, 5)[0], -100, 1e-9);
    fb.setPropertyValue("ZeroOffset", 0.1);
    ASSERT_NEAR(run(1, 5)[0], -50, 1e-9);
    fb.setPropertyValue("OutputQuantity", 1);
    ASSERT_NEAR(run(1, 5)[0], 0.1, 1e-12);
    ASSERT_EQ(fb.getSignals()[0].getDescriptor().getUnit().getSymbol(), "mV/V");
}

TEST_F(BridgeReaderTest, LowExcitationYieldsNaN)
{
    connect();
    const auto out = run(1, 0.1);
    ASSERT_EQ(out.size(), 10u);
    ASSERT_TRUE(std::isnan(out[0]));
}

// +/-10 mV over at least 4 V is +/-2.5 mV/V: the quarter bridge maps it to [-4975.1, 5025.1] um/m
TEST_F(BridgeReaderTest, RangeFollowsTheLowestExcitation)
{
    connect();
    const auto range = fb.getSignals()[0].getDescriptor().getValueRange();
    const auto strain = [](double ratio) { return -4 * ratio / (2 * (1 + 2 * ratio)) * 1e6; };
    ASSERT_NEAR(range.getLowValue(), strain(2.5e-3), 1e-6);
    ASSERT_NEAR(range.getHighValue(), strain(-2.5e-3), 1e-6);
}

TEST_F(BridgeReaderTest, ANonVoltageExcitationInvalidatesTheBlock)
{
    excitation.setDescriptor(DataDescriptorBuilder().setSampleType(SampleType::Float64).setUnit(Unit("A")).build());
    connect();
    ASSERT_EQ(status(), ComponentStatus::Error);
    ASSERT_EQ(fb.getStatusContainer().getStatusMessage("ComponentStatus"), "Excitation: value descriptor rejected by the owner");
}

TEST_F(BridgeReaderTest, AVoltageExcitationRecoversTheBlock)
{
    excitation.setDescriptor(DataDescriptorBuilder().setSampleType(SampleType::Float64).setUnit(Unit("A")).build());
    connect();
    excitation.setDescriptor(voltage("V", 4, 6));
    context.getScheduler().waitAll();  // the recovery drops the data queued behind its event
    ASSERT_NEAR(run(1, 5)[0], -4 * 2e-4 / (2 * (1 + 4e-4)) * 1e6, 1e-9);
    ASSERT_EQ(status(), ComponentStatus::Ok);
}
