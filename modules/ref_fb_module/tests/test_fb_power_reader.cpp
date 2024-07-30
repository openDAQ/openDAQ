#include <opendaq/context_internal_ptr.h>
#include <opendaq/instance_factory.h>
#include <opendaq/module_ptr.h>
#include <opendaq/opendaq.h>
#include <ref_fb_module/module_dll.h>
#include <testutils/memcheck_listener.h>
#include <gmock/gmock.h>
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

class PowerReaderTest : public testing::Test
{
protected:
    SignalConfigPtr voltageSignal;
    SignalConfigPtr currentSignal;
    SignalConfigPtr timeSignal;
    ContextPtr ctx;

    void createSignals(const ContextPtr& ctx)
    {
        timeSignal = Signal(ctx, nullptr, "time");
        voltageSignal = Signal(ctx, nullptr, "voltage");
        voltageSignal.setDomainSignal(timeSignal);
        currentSignal = Signal(ctx, nullptr, "current");
        currentSignal.setDomainSignal(timeSignal);

        const auto timeDescriptor = DataDescriptorBuilder()
            .setSampleType(SampleType::Int64)
            .setTickResolution(Ratio(1, 1000))
            .setOrigin("1970-01-01T00:00:00")
            .setRule(LinearDataRule(1, 0))
            .setUnit(Unit("s", -1, "second", "time"))
            .build();
        timeSignal.setDescriptor(timeDescriptor);

        const auto voltageDescriptor = DataDescriptorBuilder()
            .setSampleType(SampleType::Float32)
            .build();
        voltageSignal.setDescriptor(voltageDescriptor);

        const auto currentDescriptor = DataDescriptorBuilder().setSampleType(SampleType::Float32).build();
        currentSignal.setDescriptor(currentDescriptor);
    }
};

TEST_F(PowerReaderTest, Create)
{
    const auto module = createModule(createContext());

    auto fb = module.createFunctionBlock("RefFBModulePowerReader", nullptr, "id");
    ASSERT_TRUE(fb.assigned());
}

TEST_F(PowerReaderTest, Connect)
{
    const auto ctx = createContext();
    const auto module = createModule(ctx);

    auto fb = module.createFunctionBlock("RefFBModulePowerReader", nullptr, "id");
    ASSERT_TRUE(fb.assigned());

    createSignals(ctx);

    fb.getInputPorts()[0].connect(voltageSignal);
    fb.getInputPorts()[1].connect(currentSignal);
}

TEST_F(PowerReaderTest, ConnectOneSignal)
{
    const auto ctx = createContext();
    const auto module = createModule(ctx);

    auto fb = module.createFunctionBlock("RefFBModulePowerReader", nullptr, "id");
    ASSERT_TRUE(fb.assigned());

    createSignals(ctx);

    fb.getInputPorts()[0].connect(voltageSignal);

    size_t cnt = 0;
    for (size_t i = 0; i < cnt; i++)
    {
        const auto timePacket = DataPacket(timeSignal.getDescriptor(), 100, 100 * cnt);
        timeSignal.sendPacket(timePacket);

        const auto voltagePacket = DataPacketWithDomain(timePacket, voltageSignal.getDescriptor(), 100);
        voltageSignal.sendPacket(voltagePacket);

        const auto currentPacket = DataPacketWithDomain(timePacket, currentSignal.getDescriptor(), 100);
        currentSignal.sendPacket(currentPacket);
    }
}

TEST_F(PowerReaderTest, Simple)
{
    const auto ctx = createContext();
    const auto module = createModule(ctx);

    auto fb = module.createFunctionBlock("RefFBModulePowerReader", nullptr, "id");
    ASSERT_TRUE(fb.assigned());

    createSignals(ctx);

    fb.getInputPorts()[0].connect(voltageSignal);
    fb.getInputPorts()[1].connect(currentSignal);

    const auto powerSignal = fb.getSignals()[0];

    const auto timePacket = DataPacket(timeSignal.getDescriptor(), 100, 0);
    timeSignal.sendPacket(timePacket);

    const auto voltagePacket = DataPacketWithDomain(timePacket, voltageSignal.getDescriptor(), 100);
    auto voltageData = static_cast<float*>(voltagePacket.getRawData());
    for (size_t i = 0; i < 100; i++)
        *voltageData++ = static_cast<float>(i);
    voltageSignal.sendPacket(voltagePacket);

    const auto currentPacket = DataPacketWithDomain(timePacket, currentSignal.getDescriptor(), 100);
    auto currentData = static_cast<float*>(currentPacket.getRawData());
    for (size_t i = 0; i < 100; i++)
        *currentData++ = static_cast<float>(i);
    currentSignal.sendPacket(currentPacket);

    const auto reader = StreamReaderBuilder()
                            .setSkipEvents(True)
                            .setSignal(powerSignal)
                            .setValueReadType(SampleType::Float64)
                            .setDomainReadType(SampleType::Int64)
                            .setReadMode(ReadMode::Scaled)
                            .setReadTimeoutType(ReadTimeoutType::All)
                            .build();

    std::vector<double> data(100);
    std::vector<int64_t> time(100);

    SizeT count = 100;
    reader.readWithDomain(data.data(), time.data(), &count, 10000);
    ASSERT_EQ(count, 100);

    std::vector<double> expectedData(100);
    std::generate(expectedData.begin(),
                  expectedData.end(),
                  [n = 0]() mutable
                  {
                      const auto res = n * n;
                      n++;
                      return res;
                  });
    ASSERT_THAT(data, testing::ElementsAreArray(expectedData));

    std::vector<int64_t> expectedTime(100);
    std::generate(expectedTime.begin(),
                  expectedTime.end(),
                  [n = 0]() mutable
                  {
                      return n++;
                  });
    ASSERT_THAT(time, testing::ElementsAreArray(expectedTime));
}

TEST_F(PowerReaderTest, MultiplePackets)
{
    const auto ctx = createContext();
    const auto module = createModule(ctx);

    auto fb = module.createFunctionBlock("RefFBModulePowerReader", nullptr, "id");
    ASSERT_TRUE(fb.assigned());

    createSignals(ctx);

    fb.getInputPorts()[0].connect(voltageSignal);
    fb.getInputPorts()[1].connect(currentSignal);

    const auto powerSignal = fb.getSignals()[0];

    const auto timePacket0 = DataPacket(timeSignal.getDescriptor(), 100, 0);
    timeSignal.sendPacket(timePacket0);

    const auto voltagePacket0 = DataPacketWithDomain(timePacket0, voltageSignal.getDescriptor(), 100);
    auto voltageData0 = static_cast<float*>(voltagePacket0.getRawData());
    for (size_t i = 0; i < 100; i++)
        *voltageData0++ = static_cast<float>(i);
    voltageSignal.sendPacket(voltagePacket0);

    const auto currentPacket0 = DataPacketWithDomain(timePacket0, currentSignal.getDescriptor(), 100);
    auto currentData0 = static_cast<float*>(currentPacket0.getRawData());
    for (size_t i = 0; i < 100; i++)
        *currentData0++ = static_cast<float>(i);
    currentSignal.sendPacket(currentPacket0);

    const auto reader = StreamReaderBuilder()
                            .setSkipEvents(True)
                            .setSignal(powerSignal)
                            .setValueReadType(SampleType::Float64)
                            .setDomainReadType(SampleType::Int64)
                            .setReadMode(ReadMode::Scaled)
                            .setReadTimeoutType(ReadTimeoutType::All)
                            .build();

    constexpr size_t samplesToRead0 = 50;

    std::vector<double> data0(samplesToRead0);
    std::vector<int64_t> time0(samplesToRead0);

    SizeT count0 = samplesToRead0;
    reader.readWithDomain(data0.data(), time0.data(), &count0, 100000);
    ASSERT_EQ(count0, samplesToRead0);

    std::vector<double> expectedData0(samplesToRead0);
    std::generate(expectedData0.begin(),
                  expectedData0.end(),
                  [n = 0]() mutable
                  {
                      const auto res = n * n;
                      n++;
                      return res;
                  });
    ASSERT_THAT(data0, testing::ElementsAreArray(expectedData0));

    std::vector<int64_t> expectedTime(samplesToRead0);
    std::generate(expectedTime.begin(), expectedTime.end(), [n = 0]() mutable { return n++; });
    ASSERT_THAT(time0, testing::ElementsAreArray(expectedTime));

    const auto timePacket1 = DataPacket(timeSignal.getDescriptor(), 100, 100);
    timeSignal.sendPacket(timePacket1);

    const auto voltagePacket1 = DataPacketWithDomain(timePacket1, voltageSignal.getDescriptor(), 100);
    auto voltageData1 = static_cast<float*>(voltagePacket1.getRawData());
    for (size_t i = 100; i < 200; i++)
        *voltageData1++ = static_cast<float>(i);
    voltageSignal.sendPacket(voltagePacket1);

    const auto currentPacket1 = DataPacketWithDomain(timePacket1, currentSignal.getDescriptor(), 100);
    auto currentData1 = static_cast<float*>(currentPacket1.getRawData());
    for (size_t i = 100; i < 200; i++)
        *currentData1++ = static_cast<float>(i);
    currentSignal.sendPacket(currentPacket1);

    constexpr size_t samplesToRead1 = 150;
    std::vector<double> data1(samplesToRead1);
    std::vector<int64_t> time1(samplesToRead1);

    SizeT count1 = samplesToRead1;
    reader.readWithDomain(data1.data(), time1.data(), &count1, 100000);
    ASSERT_EQ(count1, samplesToRead1);

    std::vector<double> expectedData1(samplesToRead1);
    std::generate(expectedData1.begin(),
                  expectedData1.end(),
                  [n = samplesToRead0]() mutable
                  {
                      const auto res = n * n;
                      n++;
                      return res;
                  });
    ASSERT_THAT(data1, testing::ElementsAreArray(expectedData1));

    std::vector<int64_t> expectedTime1(samplesToRead1);
    std::generate(expectedTime1.begin(), expectedTime1.end(), [n = samplesToRead0]() mutable { return n++; });
    ASSERT_THAT(time1, testing::ElementsAreArray(expectedTime1));
}

TEST_F(PowerReaderTest, DisconnectReconnect)
{
    const auto ctx = createContext();
    const auto module = createModule(ctx);

    auto fb = module.createFunctionBlock("RefFBModulePowerReader", nullptr, "id");
    ASSERT_TRUE(fb.assigned());

    createSignals(ctx);

    fb.getInputPorts()[0].connect(voltageSignal);
    fb.getInputPorts()[1].connect(currentSignal);

    const auto powerSignal = fb.getSignals()[0];

    auto timePacket = DataPacket(timeSignal.getDescriptor(), 100, 0);
    timeSignal.sendPacket(timePacket);

    auto voltagePacket = DataPacketWithDomain(timePacket, voltageSignal.getDescriptor(), 100);
    auto voltageData = static_cast<float*>(voltagePacket.getRawData());
    for (size_t i = 0; i < 100; i++)
        *voltageData++ = static_cast<float>(i);
    voltageSignal.sendPacket(voltagePacket);

    auto currentPacket = DataPacketWithDomain(timePacket, currentSignal.getDescriptor(), 100);
    auto currentData = static_cast<float*>(currentPacket.getRawData());
    for (size_t i = 0; i < 100; i++)
        *currentData++ = static_cast<float>(i);
    currentSignal.sendPacket(currentPacket);

    const auto reader = StreamReaderBuilder()
                            .setSkipEvents(True)
                            .setSignal(powerSignal)
                            .setValueReadType(SampleType::Float64)
                            .setDomainReadType(SampleType::Int64)
                            .setReadMode(ReadMode::Scaled)
                            .setReadTimeoutType(ReadTimeoutType::All)
                            .build();

    std::vector<double> data(100);
    std::vector<int64_t> time(100);

    SizeT count = 100;
    reader.readWithDomain(data.data(), time.data(), &count, 10000);
    ASSERT_EQ(count, 100);

    std::vector<double> expectedData(100);
    std::generate(expectedData.begin(),
                  expectedData.end(),
                  [n = 0]() mutable
                  {
                      const auto res = n * n;
                      n++;
                      return res;
                  });
    ASSERT_THAT(data, testing::ElementsAreArray(expectedData));

    std::vector<int64_t> expectedTime(100);
    std::generate(expectedTime.begin(), expectedTime.end(), [n = 0]() mutable { return n++; });
    ASSERT_THAT(time, testing::ElementsAreArray(expectedTime));

    fb.getInputPorts()[1].disconnect();
    fb.getInputPorts()[1].connect(currentSignal);

    timePacket = DataPacket(timeSignal.getDescriptor(), 100, 100);
    timeSignal.sendPacket(timePacket);

    voltagePacket = DataPacketWithDomain(timePacket, voltageSignal.getDescriptor(), 100);
    voltageData = static_cast<float*>(voltagePacket.getRawData());
    for (size_t i = 0; i < 100; i++)
        *voltageData++ = static_cast<float>(i + 100);
    voltageSignal.sendPacket(voltagePacket);

    currentPacket = DataPacketWithDomain(timePacket, currentSignal.getDescriptor(), 100);
    currentData = static_cast<float*>(currentPacket.getRawData());
    for (size_t i = 0; i < 100; i++)
        *currentData++ = static_cast<float>(i + 100);
    currentSignal.sendPacket(currentPacket);

    std::vector<double> data1(100);
    std::vector<int64_t> time1(100);

    SizeT count1 = 100;
    reader.readWithDomain(data1.data(), time1.data(), &count1, 10000);
    ASSERT_EQ(count1, 100);

    std::vector<double> expectedData1(100);
    std::generate(expectedData1.begin(),
                  expectedData1.end(),
                  [n = 100]() mutable
                  {
                      const auto res = n * n;
                      n++;
                      return res;
                  });
    ASSERT_THAT(data1, testing::ElementsAreArray(expectedData1));

    std::vector<int64_t> expectedTime1(100);
    std::generate(expectedTime1.begin(), expectedTime1.end(), [n = 100]() mutable { return n++; });
    ASSERT_THAT(time, testing::ElementsAreArray(expectedTime));
}

TEST_F(PowerReaderTest, Gap)
{
    const auto ctx = createContext();
    const auto module = createModule(ctx);

    auto fb = module.createFunctionBlock("RefFBModulePowerReader", nullptr, "id");
    ASSERT_TRUE(fb.assigned());

    createSignals(ctx);

    fb.getInputPorts()[0].connect(voltageSignal);
    fb.getInputPorts()[1].connect(currentSignal);

    const auto powerSignal = fb.getSignals()[0];

    auto timePacket = DataPacket(timeSignal.getDescriptor(), 100, 0);
    timeSignal.sendPacket(timePacket);

    auto voltagePacket = DataPacketWithDomain(timePacket, voltageSignal.getDescriptor(), 100);
    auto voltageData = static_cast<float*>(voltagePacket.getRawData());
    for (size_t i = 0; i < 100; i++)
        *voltageData++ = static_cast<float>(i);
    voltageSignal.sendPacket(voltagePacket);

    auto currentPacket = DataPacketWithDomain(timePacket, currentSignal.getDescriptor(), 100);
    auto currentData = static_cast<float*>(currentPacket.getRawData());
    for (size_t i = 0; i < 100; i++)
        *currentData++ = static_cast<float>(i);
    currentSignal.sendPacket(currentPacket);

    const auto reader = StreamReaderBuilder()
                            .setSkipEvents(True)
                            .setSignal(powerSignal)
                            .setValueReadType(SampleType::Float64)
                            .setDomainReadType(SampleType::Int64)
                            .setReadMode(ReadMode::Scaled)
                            .setReadTimeoutType(ReadTimeoutType::All)
                            .build();

    timePacket = DataPacket(timeSignal.getDescriptor(), 100, 150);
    timeSignal.sendPacket(timePacket);

    voltagePacket = DataPacketWithDomain(timePacket, voltageSignal.getDescriptor(), 100);
    voltageData = static_cast<float*>(voltagePacket.getRawData());
    for (size_t i = 0; i < 100; i++)
        *voltageData++ = static_cast<float>(i + 100);
    voltageSignal.sendPacket(voltagePacket);

    currentPacket = DataPacketWithDomain(timePacket, currentSignal.getDescriptor(), 100);
    currentData = static_cast<float*>(currentPacket.getRawData());
    for (size_t i = 0; i < 100; i++)
        *currentData++ = static_cast<float>(i + 100);
    currentSignal.sendPacket(currentPacket);

    std::vector<double> data(200);
    std::vector<int64_t> time(200);

    SizeT count = 200;
    reader.readWithDomain(data.data(), time.data(), &count, 10000);
    ASSERT_EQ(count, 100);

    // to make comparing 100 samples
    data.resize(100);
    time.resize(100);

    std::vector<double> expectedData(100);
    std::generate(expectedData.begin(),
                  expectedData.end(),
                  [n = 0]() mutable
                  {
                      const auto res = n * n;
                      n++;
                      return res;
                  });
    ASSERT_THAT(data, testing::ElementsAreArray(expectedData.data(), 100));

    std::vector<int64_t> expectedTime(100);
    std::generate(expectedTime.begin(), expectedTime.end(), [n = 0]() mutable { return n++; });
    ASSERT_THAT(time, testing::ElementsAreArray(expectedTime.data(), 100));
}
