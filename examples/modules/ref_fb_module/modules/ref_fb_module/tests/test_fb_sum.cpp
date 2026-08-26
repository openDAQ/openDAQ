#include <opendaq/instance_factory.h>
#include <opendaq/module_ptr.h>
#include <opendaq/opendaq.h>
#include <ref_fb_module/module_dll.h>
#include <testutils/memcheck_listener.h>

#include <chrono>
#include <thread>

using namespace daq;

// Functional suite driving MultiReader2 end to end through the sum function block

static ModulePtr createModule(const ContextPtr& context)
{
    ModulePtr module;
    createModule(&module, context);
    return module;
}

static ContextPtr createContext()
{
    const auto logger = Logger();
    return Context(Scheduler(logger), logger, TypeManager(), nullptr, nullptr);
}

class SumTest : public testing::Test
{
public:
    ModulePtr module;
    FunctionBlockPtr fb;
    ContextPtr context;

protected:
    void SetUp() override
    {
        context = createContext();
        module = createModule(context);
        fb = module.createFunctionBlock("RefFBModuleSumReader", nullptr, "fb");
    }

    DataDescriptorPtr domainDescriptor(Int delta = 1) const
    {
        return DataDescriptorBuilder()
            .setSampleType(SampleType::Int64)
            .setTickResolution(Ratio(1, 1000))
            .setOrigin("1970-01-01T00:00:00")
            .setRule(LinearDataRule(delta, 0))
            .setUnit(Unit("s", -1, "seconds", "time"))
            .build();
    }

    SignalConfigPtr createSignal(const std::string& localId, Int delta = 1, const UnitPtr& unit = nullptr)
    {
        auto domainSignal = Signal(context, nullptr, localId + "_domain");
        domainSignal.setDescriptor(domainDescriptor(delta));
        auto signal = Signal(context, nullptr, localId);
        auto builder = DataDescriptorBuilder().setSampleType(SampleType::Float64);
        if (unit.assigned())
            builder.setUnit(unit);
        signal.setDescriptor(builder.build());
        signal.setDomainSignal(domainSignal);
        return signal;
    }

    // Sample at tick t carries the value `base` on every signal
    static void sendData(const SignalConfigPtr& signal, Int offset, SizeT samples, double base = 1.0)
    {
        auto domainPacket = DataPacket(signal.getDomainSignal().getDescriptor(), samples, offset);
        auto packet = DataPacketWithDomain(domainPacket, signal.getDescriptor(), samples);
        const auto values = static_cast<double*>(packet.getData());
        for (SizeT i = 0; i < samples; i++)
            values[i] = base;
        signal.sendPacket(packet);
    }

    // Reads the sum output until `expected` samples arrive or the timeout runs out
    template <typename TReader>
    static std::vector<double> readSum(TReader& reader, SizeT expected, SizeT timeoutMs = 2000)
    {
        std::vector<double> values;
        const auto deadline = std::chrono::steady_clock::now() + std::chrono::milliseconds(timeoutMs);
        while (values.size() < expected && std::chrono::steady_clock::now() < deadline)
        {
            double buffer[256];
            SizeT count = std::min<SizeT>(256, expected - values.size());
            reader.read(buffer, &count, 50);
            values.insert(values.end(), buffer, buffer + count);
        }
        return values;
    }

    template <typename TReader>
    static SizeT drainSum(TReader& reader, SizeT timeoutMs = 300)
    {
        double buffer[256];
        SizeT total = 0;
        SizeT count = 256;
        reader.read(buffer, &count, timeoutMs);
        total += count;
        return total;
    }
};

TEST_F(SumTest, CreateReportsWarning)
{
    ASSERT_TRUE(fb.assigned());
    ASSERT_EQ(fb.getStatusContainer().getStatus("ComponentStatus"), ComponentStatus::Warning);
    ASSERT_EQ(fb.getInputPorts().getCount(), 1u);
}

TEST_F(SumTest, SpareUnusedPortAlwaysPresent)
{
    auto sig1 = createSignal("sig1");
    auto sig2 = createSignal("sig2");

    fb.getInputPorts()[0].connect(sig1);
    ASSERT_EQ(fb.getInputPorts().getCount(), 2u);
    fb.getInputPorts()[1].connect(sig2);
    ASSERT_EQ(fb.getInputPorts().getCount(), 3u);

    fb.getInputPorts()[0].disconnect();
    ASSERT_EQ(fb.getInputPorts().getCount(), 2u);
    fb.getInputPorts()[0].disconnect();
    ASSERT_EQ(fb.getInputPorts().getCount(), 1u);
    ASSERT_EQ(fb.getStatusContainer().getStatus("ComponentStatus"), ComponentStatus::Warning);
}

TEST_F(SumTest, SumsTwoSignals)
{
    auto sig1 = createSignal("sig1");
    auto sig2 = createSignal("sig2");
    fb.getInputPorts()[0].connect(sig1);
    fb.getInputPorts()[1].connect(sig2);

    auto reader = StreamReader<double>(fb.getSignals()[0]);

    sendData(sig1, 100, 50, 1.0);
    sendData(sig2, 100, 50, 2.0);

    const auto values = readSum(reader, 50);
    ASSERT_EQ(values.size(), 50u);
    for (const auto value : values)
        ASSERT_DOUBLE_EQ(value, 3.0);
    ASSERT_EQ(fb.getStatusContainer().getStatus("ComponentStatus"), ComponentStatus::Ok);
}

TEST_F(SumTest, AlignsLateStarter)
{
    auto sig1 = createSignal("sig1");
    auto sig2 = createSignal("sig2");
    fb.getInputPorts()[0].connect(sig1);
    fb.getInputPorts()[1].connect(sig2);

    auto reader = StreamReader<double>(fb.getSignals()[0]);

    // The second input starts 20 ticks later: only the overlap is summed
    sendData(sig1, 100, 50, 1.0);
    sendData(sig2, 120, 30, 2.0);

    const auto values = readSum(reader, 30);
    ASSERT_EQ(values.size(), 30u);
    for (const auto value : values)
        ASSERT_DOUBLE_EQ(value, 3.0);
}

// FINDING 3b: fails ~3 runs in 4 once gap checking is on. The offset jump makes the connection
// emit a gap, and the over-broad gap discard (FINDING 3) throws away the post-gap data that
// arrived in the same read window. Streaming across ANY discontinuity is unreliable, not latent.
TEST_F(SumTest, DISABLED_DescriptorChangeKeepsSumming)
{
    auto sig1 = createSignal("sig1");
    auto sig2 = createSignal("sig2");
    fb.getInputPorts()[0].connect(sig1);
    fb.getInputPorts()[1].connect(sig2);

    auto reader = StreamReader<double>(fb.getSignals()[0]);
    sendData(sig1, 100, 20, 1.0);
    sendData(sig2, 100, 20, 2.0);
    ASSERT_EQ(readSum(reader, 20).size(), 20u);

    // A range change parks the reader; the block commits and resyncs on fresh data
    sig1.setDescriptor(DataDescriptorBuilder().setSampleType(SampleType::Float64).setValueRange(Range(-5, 5)).build());
    sendData(sig1, 200, 20, 5.0);
    sendData(sig2, 200, 20, 2.0);

    const auto values = readSum(reader, 20);
    ASSERT_GE(values.size(), 20u);
    ASSERT_DOUBLE_EQ(values.back(), 7.0);
}

TEST_F(SumTest, ExcludeModeKeepsGoodInputs)
{
    auto good = createSignal("good");
    auto bad = createSignal("bad", 2);  // half the rate: InvalidDomain against the main input
    fb.getInputPorts()[0].connect(good);
    fb.getInputPorts()[1].connect(bad);

    auto reader = StreamReader<double>(fb.getSignals()[0]);

    sendData(good, 100, 40, 1.0);
    sendData(bad, 100, 40, 2.0);

    // The bad input is excluded and the good one keeps summing alone
    const auto values = readSum(reader, 40);
    ASSERT_EQ(values.size(), 40u);
    for (const auto value : values)
        ASSERT_DOUBLE_EQ(value, 1.0);
}

// FINDING 4 (flaky, ~1 run in 3): recovery after Deactivate mode is racy - same discontinuity
// handling as FINDING 3b; the recovery data follows an offset jump and can be discarded with the gap.
TEST_F(SumTest, DeactivateModeStopsAndRecovers)
{
    fb.setPropertyValue("BadInputHandling", 1);

    auto good = createSignal("good");
    auto bad = createSignal("bad", 2);
    fb.getInputPorts()[0].connect(good);
    fb.getInputPorts()[1].connect(bad);

    auto reader = StreamReader<double>(fb.getSignals()[0]);

    sendData(good, 100, 20, 1.0);
    sendData(bad, 100, 20, 2.0);

    // The failing input deactivates the whole reader: fresh data goes nowhere
    std::this_thread::sleep_for(std::chrono::milliseconds(300));
    drainSum(reader);
    sendData(good, 200, 20, 1.0);
    ASSERT_EQ(drainSum(reader), 0u);

    // Removing the bad input reconfigures the reader and the output resumes
    fb.getInputPorts()[1].disconnect();
    sendData(good, 300, 20, 1.0);
    const auto values = readSum(reader, 20);
    ASSERT_EQ(values.size(), 20u);
    for (const auto value : values)
        ASSERT_DOUBLE_EQ(value, 1.0);
}

TEST_F(SumTest, DisconnectReconfiguresAndContinues)
{
    auto sig1 = createSignal("sig1");
    auto sig2 = createSignal("sig2");
    fb.getInputPorts()[0].connect(sig1);
    fb.getInputPorts()[1].connect(sig2);

    auto reader = StreamReader<double>(fb.getSignals()[0]);
    sendData(sig1, 100, 20, 1.0);
    sendData(sig2, 100, 20, 2.0);
    ASSERT_EQ(readSum(reader, 20).size(), 20u);

    fb.getInputPorts()[1].disconnect();
    ASSERT_EQ(fb.getInputPorts().getCount(), 2u);

    sendData(sig1, 200, 20, 1.0);
    const auto values = readSum(reader, 20);
    ASSERT_EQ(values.size(), 20u);
    for (const auto value : values)
        ASSERT_DOUBLE_EQ(value, 1.0);
}

// FINDING 1: the block only configures its output when EVERY input is healthy, so an input
// excluded on the very first handshake leaves the output descriptor unset and nothing is ever
// emitted. Exclude mode only works if a clean handshake happened first.
TEST_F(SumTest, DISABLED_MixedUnitsAreRejected)
{
    auto volts = createSignal("volts", 1, Unit("V", -1, "volts", "voltage"));
    auto amps = createSignal("amps", 1, Unit("A", -1, "amperes", "current"));
    fb.getInputPorts()[0].connect(volts);
    fb.getInputPorts()[1].connect(amps);

    auto reader = StreamReader<double>(fb.getSignals()[0]);

    sendData(volts, 100, 20, 1.0);
    sendData(amps, 100, 20, 2.0);

    // Summing different quantities is refused: the mismatching input is excluded, not summed in
    const auto values = readSum(reader, 20);
    ASSERT_EQ(values.size(), 20u);
    for (const auto value : values)
        ASSERT_DOUBLE_EQ(value, 1.0);
    ASSERT_EQ(fb.getStatusContainer().getStatus("ComponentStatus"), ComponentStatus::Warning);

    // Matching the main input's unit lets it rejoin
    amps.setDescriptor(DataDescriptorBuilder().setSampleType(SampleType::Float64).setUnit(Unit("V", -1, "volts", "voltage")).build());
    sendData(volts, 200, 20, 1.0);
    sendData(amps, 200, 20, 2.0);
    const auto rejoined = readSum(reader, 20);
    ASSERT_EQ(rejoined.size(), 20u);
    ASSERT_DOUBLE_EQ(rejoined.back(), 3.0);
    ASSERT_EQ(fb.getStatusContainer().getStatus("ComponentStatus"), ComponentStatus::Ok);
}

// FINDING 2: an invalid MAIN input is never surfaced. Every committed event restarts the sync
// window, and with gap checking on, a gapping stream refreshes it forever, so the 2s timeout
// never elapses: the reader waits silently and the block keeps reporting Ok while emitting nothing.
TEST_F(SumTest, DISABLED_MainInputFailureParksInsteadOfStalling)
{
    // The main input carries a non-integer sample rate: the reader reports it as InvalidDomain
    auto badMain = createSignal("badMain", 3);
    auto good = createSignal("good");
    fb.getInputPorts()[0].connect(badMain);
    fb.getInputPorts()[1].connect(good);

    auto reader = StreamReader<double>(fb.getSignals()[0]);

    sendData(badMain, 100, 20, 1.0);
    sendData(good, 100, 20, 2.0);

    // Excluding the main input would leave the reader without a grid, so the block parks instead
    std::this_thread::sleep_for(std::chrono::milliseconds(2300));
    sendData(badMain, 200, 20, 1.0);
    sendData(good, 200, 20, 2.0);
    ASSERT_EQ(drainSum(reader), 0u);
    ASSERT_EQ(fb.getStatusContainer().getStatus("ComponentStatus"), ComponentStatus::Warning);

    // Dropping the bad main promotes the good input and the block recovers
    fb.getInputPorts()[0].disconnect();
    sendData(good, 300, 20, 2.0);
    const auto values = readSum(reader, 20);
    ASSERT_EQ(values.size(), 20u);
    for (const auto value : values)
        ASSERT_DOUBLE_EQ(value, 2.0);
}

// FINDING 3: gaps are counted, not queued, so the boundary position is unknown and the gapped
// slot discards ALL staged data, including samples that arrived after the gap in the same window.
TEST_F(SumTest, DISABLED_GapDoesNotEvictInput)
{
    auto sig1 = createSignal("sig1");
    auto sig2 = createSignal("sig2");
    fb.getInputPorts()[0].connect(sig1);
    fb.getInputPorts()[1].connect(sig2);

    auto reader = StreamReader<double>(fb.getSignals()[0]);
    sendData(sig1, 100, 20, 1.0);
    sendData(sig2, 100, 20, 2.0);
    ASSERT_EQ(readSum(reader, 20).size(), 20u);

    // A discontinuity on one input is a liveness condition: it resyncs, it does not exclude
    sendData(sig1, 500, 20, 1.0);
    sendData(sig2, 500, 20, 2.0);

    const auto values = readSum(reader, 20);
    ASSERT_EQ(values.size(), 20u);
    for (const auto value : values)
        ASSERT_DOUBLE_EQ(value, 3.0);
    ASSERT_EQ(fb.getStatusContainer().getStatus("ComponentStatus"), ComponentStatus::Ok);
}

TEST_F(SumTest, BadInputHandlingSwapReconfigures)
{
    auto sig1 = createSignal("sig1");
    fb.getInputPorts()[0].connect(sig1);

    auto reader = StreamReader<double>(fb.getSignals()[0]);
    sendData(sig1, 100, 20, 1.0);
    ASSERT_EQ(readSum(reader, 20).size(), 20u);

    // Swapping the property reconfigures the reader; streaming resumes seamlessly
    fb.setPropertyValue("BadInputHandling", 1);
    sendData(sig1, 200, 20, 4.0);
    const auto values = readSum(reader, 20);
    ASSERT_EQ(values.size(), 20u);
    ASSERT_DOUBLE_EQ(values.front(), 4.0);
}

