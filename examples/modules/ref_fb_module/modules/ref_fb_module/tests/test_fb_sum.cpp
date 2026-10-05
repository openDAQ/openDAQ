#include <opendaq/context_internal_ptr.h>
#include <opendaq/instance_factory.h>
#include <opendaq/module_ptr.h>
#include <opendaq/opendaq.h>
#include <ref_fb_module/module_dll.h>
#include <testutils/memcheck_listener.h>

#include <chrono>
#include <thread>

using namespace daq;

static ModulePtr createModule(const ContextPtr& context)
{
    ModulePtr module;
    auto logger = Logger();
    createModule(&module, context);
    return module;
}

static ContextPtr createContext()
{
    const auto logger = Logger();
    return Context(Scheduler(logger), logger, TypeManager(), nullptr, nullptr);
}

// The block's reader evaluates on the scheduler, so statuses, port counts and output data follow asynchronously
template <typename Condition>
static bool waitFor(Condition condition, int timeoutMs = 3000)
{
    const auto deadline = std::chrono::steady_clock::now() + std::chrono::milliseconds(timeoutMs);
    while (!condition())
    {
        if (std::chrono::steady_clock::now() > deadline)
            return false;
        std::this_thread::sleep_for(std::chrono::milliseconds(5));
    }
    return true;
}

static bool waitForStatus(const FunctionBlockPtr& fb, ComponentStatus status)
{
    return waitFor([&] { return fb.getStatusContainer().getStatus("ComponentStatus") == status; });
}

static bool waitForPortCount(const FunctionBlockPtr& fb, SizeT count)
{
    return waitFor([&] { return fb.getInputPorts().getCount() == count; });
}

// After the last connect the block still reconfigures once; data sent into that window is discarded by design
static bool settle(const FunctionBlockPtr& fb, SizeT portCount)
{
    const bool ok = waitForPortCount(fb, portCount) && waitForStatus(fb, ComponentStatus::Ok);
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    return ok;
}

template <typename Reader>
static bool waitForAvailable(const Reader& reader, SizeT count)
{
    return waitFor([&] { return reader.getAvailableCount() >= count; });
}

class SumTest : public testing::Test
{
public:
    ModulePtr module;
    FunctionBlockPtr fb;
    ContextPtr context;

    DataDescriptorPtr validDescriptor;
    DataDescriptorPtr invalidDescriptor;
    DataDescriptorPtr timeDescriptor;

    ListPtr<ISignalConfig> validSignals;
    ListPtr<ISignalConfig> invalidSignals;
    SignalConfigPtr timeSignal;

protected:
    void SetUp() override
    {
        context = createContext();
        module = createModule(context);
        fb = module.createFunctionBlock("RefFBModuleSumReader", nullptr, "fb");

        validDescriptor = DataDescriptorBuilder().setSampleType(SampleType::Float64).build();
        invalidDescriptor = DataDescriptorBuilder().setSampleType(SampleType::ComplexFloat32).build();
        timeDescriptor = DataDescriptorBuilder()
                             .setSampleType(SampleType::Int64)
                             .setTickResolution(Ratio(1, 1000))
                             .setOrigin("1970-01-01T00:00:00")
                             .setRule(LinearDataRule(1, 0))
                             .setUnit(Unit("s", -1, "seconds", "time"))
                             .build();

        validSignals = List<ISignal>();
        invalidSignals = List<ISignal>();
        timeSignal = SignalWithDescriptor(context, timeDescriptor, nullptr, "time_sig");

        for (size_t i = 0; i < 10; ++i)
        {
            validSignals.pushBack(SignalWithDescriptor(context, validDescriptor, nullptr, fmt::format("sig{}", i)));
            invalidSignals.pushBack(SignalWithDescriptor(context, invalidDescriptor, nullptr, fmt::format("sig{}", i)));
            validSignals[i].setDomainSignal(timeSignal);
            invalidSignals[i].setDomainSignal(timeSignal);
        }
    }

    void TearDown() override
    {
        context.getScheduler().stop();
    }

    // Connects signal i to port i; every connect adds the next free port, so the ports are waited for
    void connect(SizeT port, const SignalConfigPtr& signal)
    {
        ASSERT_TRUE(waitForPortCount(fb, port + 1));
        fb.getInputPorts()[port].connect(signal);
    }

    void sendData(SizeT sampleCount, SizeT offset, bool sendInvalid, std::pair<size_t, size_t> signalRange,
                  ListPtr<ISignalConfig> extraSignals = List<ISignalConfig>(),
                  ListPtr<ISignalConfig> extraDomainSignals = List<ISignalConfig>())
    {
        DataPacketPtr domainPacket = DataPacket(timeDescriptor, sampleCount, offset);
        DataPacketPtr valuePacket = DataPacketWithDomain(domainPacket, validDescriptor, sampleCount);

        double* sumValueData = static_cast<double*>(valuePacket.getRawData());
        for (size_t i = 0; i < sampleCount; ++i)
            sumValueData[i] = 1;

        timeSignal.sendPacket(domainPacket);
        for (size_t i = signalRange.first; i < signalRange.second; ++i)
            validSignals[i].sendPacket(valuePacket);

        if (sendInvalid)
        {
            DataPacketPtr invalidValuePacket = DataPacketWithDomain(domainPacket, invalidDescriptor, sampleCount);
            for (size_t i = signalRange.first; i < signalRange.second; ++i)
                invalidSignals[i].sendPacket(invalidValuePacket);
        }

        for (const auto& signal : extraSignals)
            signal.sendPacket(valuePacket);
        for (const auto& signal : extraDomainSignals)
            signal.sendPacket(domainPacket);
    }
};

TEST_F(SumTest, Create)
{
    const auto module = createModule(createContext());
    auto fb = module.createFunctionBlock("RefFBModuleSumReader", nullptr, "id");
    ASSERT_TRUE(fb.assigned());
    ASSERT_EQ(fb.getStatusContainer().getStatus("ComponentStatus"), ComponentStatus::Warning);
}

TEST_F(SumTest, ConnectSignal)
{
    ASSERT_NO_THROW(fb.getInputPorts()[0].connect(validSignals[0]));
    ASSERT_TRUE(waitForStatus(fb, ComponentStatus::Ok));
    ASSERT_TRUE(waitForPortCount(fb, 2));
}

TEST_F(SumTest, ConnectSignals)
{
    for (size_t i = 0; i < validSignals.getCount(); ++i)
        connect(i, validSignals[i]);
    ASSERT_TRUE(waitForStatus(fb, ComponentStatus::Ok));
    ASSERT_TRUE(waitForPortCount(fb, 11));
}

TEST_F(SumTest, DisconnectSignals)
{
    for (size_t i = 0; i < validSignals.getCount(); ++i)
        connect(i, validSignals[i]);
    ASSERT_TRUE(waitForPortCount(fb, 11));

    for (const auto& ip : fb.getInputPorts())
        ip.disconnect();

    // Every disconnected used port is removed; the free port stays
    ASSERT_TRUE(waitForPortCount(fb, 1));
    ASSERT_TRUE(waitForStatus(fb, ComponentStatus::Warning));
}

TEST_F(SumTest, InvalidSignals)
{
    for (size_t i = 0; i < validSignals.getCount(); ++i)
    {
        connect(i * 2, validSignals[i]);
        connect(i * 2 + 1, invalidSignals[i]);
    }

    // The inputs the block cannot read are set unused and reported; the others are summed
    ASSERT_TRUE(waitForPortCount(fb, 21));
    ASSERT_TRUE(waitForStatus(fb, ComponentStatus::Warning));
}

TEST_F(SumTest, InvalidSignalsRecovery)
{
    for (size_t i = 0; i < validSignals.getCount(); ++i)
    {
        connect(i * 2, validSignals[i]);
        connect(i * 2 + 1, invalidSignals[i]);
    }
    ASSERT_TRUE(waitForPortCount(fb, 21));
    ASSERT_TRUE(waitForStatus(fb, ComponentStatus::Warning));

    auto ip = fb.getInputPorts();
    for (int i = static_cast<int>(fb.getInputPorts().getCount()) - 2; i > 0; i -= 2)
        ip[i].disconnect();

    ASSERT_TRUE(waitForPortCount(fb, 11));
    ASSERT_TRUE(waitForStatus(fb, ComponentStatus::Ok));
}

TEST_F(SumTest, SumSignals)
{
    for (size_t i = 0; i < validSignals.getCount(); ++i)
        connect(i, validSignals[i]);
    ASSERT_TRUE(settle(fb, 11));

    auto reader = StreamReaderBuilder().setSkipEvents(true).setValueReadType(SampleType::Float64).setSignal(fb.getSignals()[0]).build();
    sendData(100, 0, false, std::make_pair(0, 10));

    ASSERT_TRUE(waitForAvailable(reader, 100));
    double data[100];
    SizeT count = 100;
    auto status = reader.read(&data, &count);
    ASSERT_EQ(status.getReadStatus(), ReadStatus::Ok);
    ASSERT_EQ(count, 100u);
    for (auto val : data)
        ASSERT_DOUBLE_EQ(val, 10);
}

TEST_F(SumTest, SumSignalsReconnect)
{
    for (size_t i = 0; i < validSignals.getCount(); ++i)
        connect(i, validSignals[i]);
    ASSERT_TRUE(waitForStatus(fb, ComponentStatus::Ok));

    auto reader = StreamReaderBuilder().setSkipEvents(true).setValueReadType(SampleType::Float64).setSignal(fb.getSignals()[0]).build();

    auto ip = fb.getInputPorts();
    for (size_t i = 0; i < 5; ++i)
        ip[i].disconnect();
    ASSERT_TRUE(settle(fb, 6));

    sendData(100, 0, false, std::make_pair(5, 10));
    ASSERT_TRUE(waitForAvailable(reader, 100));
    double data[100];
    SizeT count = 100;
    auto status = reader.read(&data, &count);
    ASSERT_EQ(status.getReadStatus(), ReadStatus::Ok);
    for (auto val : data)
        ASSERT_DOUBLE_EQ(val, 5);

    for (size_t i = 0; i < 5; ++i)
        connect(5 + i, validSignals[i]);
    ASSERT_TRUE(settle(fb, 11));

    sendData(100, 100, false, std::make_pair(0, 10));
    ASSERT_TRUE(waitForAvailable(reader, 100));
    count = 100;
    status = reader.read(&data, &count);
    ASSERT_EQ(status.getReadStatus(), ReadStatus::Ok);
    for (auto val : data)
        ASSERT_DOUBLE_EQ(val, 10);
}

TEST_F(SumTest, SumSignalsInvalidRecovery)
{
    for (size_t i = 0; i < validSignals.getCount(); ++i)
    {
        connect(i * 2, validSignals[i]);
        connect(i * 2 + 1, invalidSignals[i]);
    }
    ASSERT_TRUE(waitForPortCount(fb, 21));
    ASSERT_TRUE(waitForStatus(fb, ComponentStatus::Warning));
    std::this_thread::sleep_for(std::chrono::milliseconds(50));

    auto reader = StreamReaderBuilder().setSkipEvents(true).setValueReadType(SampleType::Float64).setSignal(fb.getSignals()[0]).build();

    // The unreadable inputs are set aside, so the readable ones are summed meanwhile
    sendData(100, 0, true, std::make_pair(0, 10));
    ASSERT_TRUE(waitForAvailable(reader, 100));
    double data[100];
    SizeT count = 100;
    auto status = reader.read(&data, &count);
    ASSERT_EQ(status.getReadStatus(), ReadStatus::Ok);
    for (auto val : data)
        ASSERT_DOUBLE_EQ(val, 10);

    // Disconnecting the invalid signals clears the warning
    auto ip = fb.getInputPorts();
    for (int i = static_cast<int>(fb.getInputPorts().getCount()) - 2; i > 0; i -= 2)
        ip[i].disconnect();
    ASSERT_TRUE(settle(fb, 11));

    sendData(100, 100, false, std::make_pair(0, 10));
    ASSERT_TRUE(waitForAvailable(reader, 100));
    count = 100;
    status = reader.read(&data, &count);
    ASSERT_EQ(status.getReadStatus(), ReadStatus::Ok);
    for (auto val : data)
        ASSERT_DOUBLE_EQ(val, 10);
}

TEST_F(SumTest, ReplaceValidWithInvalid)
{
    for (size_t i = 0; i < validSignals.getCount(); ++i)
        connect(i, validSignals[i]);
    ASSERT_TRUE(waitForStatus(fb, ComponentStatus::Ok));
    ASSERT_NO_THROW(fb.getInputPorts()[0].connect(invalidSignals[0]));
    ASSERT_TRUE(waitForStatus(fb, ComponentStatus::Warning));
}

TEST_F(SumTest, IncompatibleDomainsReconnect)
{
    auto incompatibleDomainDescriptor = DataDescriptorBuilder()
                                            .setSampleType(SampleType::Int64)
                                            .setTickResolution(Ratio(1, 1000))
                                            .setOrigin("1970-01-01T00:00:00")
                                            .setRule(LinearDataRule(2, 0))
                                            .setUnit(Unit("s", -1, "seconds", "time"))
                                            .build();
    auto incompatibleDomainSignal = SignalWithDescriptor(context, incompatibleDomainDescriptor, nullptr, "invalidDomainSig", nullptr);
    auto incompatibleSignal = SignalWithDescriptor(context, validDescriptor, nullptr, "invalidSig", nullptr);
    incompatibleSignal.setDomainSignal(incompatibleDomainSignal);

    connect(0, validSignals[0]);
    connect(1, incompatibleSignal);
    ASSERT_TRUE(waitForPortCount(fb, 3));

    // The input at another rate is set aside and reported; the other one is summed on its own
    ASSERT_TRUE(waitForStatus(fb, ComponentStatus::Warning));
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    auto reader = StreamReaderBuilder().setSkipEvents(true).setValueReadType(SampleType::Float64).setSignal(fb.getSignals()[0]).build();
    sendData(100, 0, false, std::make_pair(0, 1), List<ISignalConfig>(incompatibleSignal), List<ISignalConfig>(incompatibleDomainSignal));
    ASSERT_TRUE(waitForAvailable(reader, 100));

    // A compatible signal on that port joins the sum
    fb.getInputPorts()[1].connect(validSignals[1]);
    ASSERT_TRUE(settle(fb, 3));
    sendData(100, 100, false, std::make_pair(0, 2));
    ASSERT_TRUE(waitForAvailable(reader, 200));
}
