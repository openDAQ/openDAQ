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

    ctx.getScheduler().stop();
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

    const auto voltagePacket = DataPacketWithDomain(timePacket, voltageSignal.getDescriptor(), 100);
    auto voltageData = static_cast<float*>(voltagePacket.getRawData());
    for (size_t i = 0; i < 100; i++)
        *voltageData++ = static_cast<float>(i);

    const auto currentPacket = DataPacketWithDomain(timePacket, currentSignal.getDescriptor(), 100);
    auto currentData = static_cast<float*>(currentPacket.getRawData());
    for (size_t i = 0; i < 100; i++)
        *currentData++ = static_cast<float>(i);

    const auto reader = StreamReaderBuilder()
                            .setSkipEvents(True)
                            .setSignal(powerSignal)
                            .setValueReadType(SampleType::Float64)
                            .setDomainReadType(SampleType::Int64)
                            .setReadMode(ReadMode::Scaled)
                            .setReadTimeoutType(ReadTimeoutType::All)
                            .build();

    timeSignal.sendPacket(timePacket);
    voltageSignal.sendPacket(voltagePacket);
    currentSignal.sendPacket(currentPacket);

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

    ctx.getScheduler().stop();
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

    const auto voltagePacket0 = DataPacketWithDomain(timePacket0, voltageSignal.getDescriptor(), 100);
    auto voltageData0 = static_cast<float*>(voltagePacket0.getRawData());
    for (size_t i = 0; i < 100; i++)
        *voltageData0++ = static_cast<float>(i);

    const auto currentPacket0 = DataPacketWithDomain(timePacket0, currentSignal.getDescriptor(), 100);
    auto currentData0 = static_cast<float*>(currentPacket0.getRawData());
    for (size_t i = 0; i < 100; i++)
        *currentData0++ = static_cast<float>(i);

    const auto reader = StreamReaderBuilder()
                            .setSkipEvents(True)
                            .setSignal(powerSignal)
                            .setValueReadType(SampleType::Float64)
                            .setDomainReadType(SampleType::Int64)
                            .setReadMode(ReadMode::Scaled)
                            .setReadTimeoutType(ReadTimeoutType::All)
                            .build();

    timeSignal.sendPacket(timePacket0);
    voltageSignal.sendPacket(voltagePacket0);
    currentSignal.sendPacket(currentPacket0);

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

    ctx.getScheduler().stop();
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

    auto voltagePacket = DataPacketWithDomain(timePacket, voltageSignal.getDescriptor(), 100);
    auto voltageData = static_cast<float*>(voltagePacket.getRawData());
    for (size_t i = 0; i < 100; i++)
        *voltageData++ = static_cast<float>(i);

    auto currentPacket = DataPacketWithDomain(timePacket, currentSignal.getDescriptor(), 100);
    auto currentData = static_cast<float*>(currentPacket.getRawData());
    for (size_t i = 0; i < 100; i++)
        *currentData++ = static_cast<float>(i);

    const auto reader = StreamReaderBuilder()
                            .setSkipEvents(True)
                            .setSignal(powerSignal)
                            .setValueReadType(SampleType::Float64)
                            .setDomainReadType(SampleType::Int64)
                            .setReadMode(ReadMode::Scaled)
                            .setReadTimeoutType(ReadTimeoutType::All)
                            .build();

    timeSignal.sendPacket(timePacket);
    voltageSignal.sendPacket(voltagePacket);
    currentSignal.sendPacket(currentPacket);

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

    ctx.getScheduler().stop();
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

    auto voltagePacket = DataPacketWithDomain(timePacket, voltageSignal.getDescriptor(), 100);
    auto voltageData = static_cast<float*>(voltagePacket.getRawData());
    for (size_t i = 0; i < 100; i++)
        *voltageData++ = static_cast<float>(i);

    auto currentPacket = DataPacketWithDomain(timePacket, currentSignal.getDescriptor(), 100);
    auto currentData = static_cast<float*>(currentPacket.getRawData());
    for (size_t i = 0; i < 100; i++)
        *currentData++ = static_cast<float>(i);

    const auto reader = StreamReaderBuilder()
                            .setSkipEvents(True)
                            .setSignal(powerSignal)
                            .setValueReadType(SampleType::Float64)
                            .setDomainReadType(SampleType::Int64)
                            .setReadMode(ReadMode::Scaled)
                            .setReadTimeoutType(ReadTimeoutType::All)
                            .build();

    timeSignal.sendPacket(timePacket);
    voltageSignal.sendPacket(voltagePacket);
    currentSignal.sendPacket(currentPacket);

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

    ctx.getScheduler().stop();
}

TEST_F(PowerReaderTest, InvalidateVoltageSignal)
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

    const auto voltagePacket0 = DataPacketWithDomain(timePacket0, voltageSignal.getDescriptor(), 100);
    auto voltageData0 = static_cast<float*>(voltagePacket0.getRawData());
    for (size_t i = 0; i < 100; i++)
        *voltageData0++ = static_cast<float>(i);

    const auto currentPacket0 = DataPacketWithDomain(timePacket0, currentSignal.getDescriptor(), 100);
    auto currentData0 = static_cast<float*>(currentPacket0.getRawData());
    for (size_t i = 0; i < 100; i++)
        *currentData0++ = static_cast<float>(i);

    const auto reader = StreamReaderBuilder()
                            .setSkipEvents(False)
                            .setSignal(powerSignal)
                            .setValueReadType(SampleType::Float64)
                            .setDomainReadType(SampleType::Int64)
                            .setReadMode(ReadMode::Scaled)
                            .setReadTimeoutType(ReadTimeoutType::All)
                            .build();

    {
        SizeT count{0};
        auto status = reader.read(nullptr, &count, 1000);
        ASSERT_EQ(status.getReadStatus(), ReadStatus::Event);
    }

    timeSignal.sendPacket(timePacket0);
    voltageSignal.sendPacket(voltagePacket0);
    currentSignal.sendPacket(currentPacket0);

    constexpr size_t samplesToRead0 = 100;

    std::vector<double> data0(samplesToRead0);
    std::vector<int64_t> time0(samplesToRead0);

    SizeT count0 = samplesToRead0;
    auto status0 = reader.readWithDomain(data0.data(), time0.data(), &count0, 1000);
    ASSERT_EQ(status0.getReadStatus(), ReadStatus::Ok);
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

    // Set incompatible Unit
    voltageSignal.setDescriptor(
        DataDescriptorBuilderCopy(voltageSignal.getDescriptor()).setUnit(Unit("W", -1, "watt", "power")).build());

    const auto timePacket1 = DataPacket(timeSignal.getDescriptor(), 100, 100);
    timeSignal.sendPacket(timePacket1);

    const auto voltagePacket1 = DataPacketWithDomain(timePacket1, voltageSignal.getDescriptor(), 100);
    voltageSignal.sendPacket(voltagePacket1);

    const auto currentPacket1 = DataPacketWithDomain(timePacket1, currentSignal.getDescriptor(), 100);
    currentSignal.sendPacket(currentPacket1);

    constexpr size_t samplesToRead1 = 100;
    ASSERT_EQ(reader.getAvailableCount(), 0u); // no data available when FB internal multireader is inactive

    // Set correct Unit
    voltageSignal.setDescriptor(
        DataDescriptorBuilderCopy(voltageSignal.getDescriptor()).setUnit(Unit("V", -1, "volts", "voltage")).build());

    // Catch event packet from value desc change
    {
        constexpr size_t samplesToRead = 1;
        std::vector<double> data(samplesToRead);
        std::vector<int64_t> time(samplesToRead);

        SizeT count = samplesToRead;
        auto status = reader.readWithDomain(data.data(), time.data(), &count, 1000);
        ASSERT_EQ(status.getReadStatus(), ReadStatus::Event);
    }

    // Catch event packet from domain desc change
    {
        constexpr size_t samplesToRead = 1;
        std::vector<double> data(samplesToRead);
        std::vector<int64_t> time(samplesToRead);

        SizeT count = samplesToRead;
        auto status = reader.readWithDomain(data.data(), time.data(), &count, 1000);
        ASSERT_EQ(status.getReadStatus(), ReadStatus::Event);
    }

    const auto timePacket2 = DataPacket(timeSignal.getDescriptor(), 50, 200);
    timeSignal.sendPacket(timePacket2);

    const auto voltagePacket2 = DataPacketWithDomain(timePacket2, voltageSignal.getDescriptor(), 50);
    auto voltageData2 = static_cast<float*>(voltagePacket2.getRawData());
    for (size_t i = 200; i < 250; i++)
        *voltageData2++ = static_cast<float>(i);
    voltageSignal.sendPacket(voltagePacket2);

    const auto currentPacket2 = DataPacketWithDomain(timePacket2, currentSignal.getDescriptor(), 50);
    auto currentData2 = static_cast<float*>(currentPacket2.getRawData());
    for (size_t i = 200; i < 250; i++)
        *currentData2++ = static_cast<float>(i);
    currentSignal.sendPacket(currentPacket2);

    constexpr size_t samplesToRead2 = 50;
    std::vector<double> data2(samplesToRead2);
    std::vector<int64_t> time2(samplesToRead2);

    SizeT count2 = samplesToRead2;
    auto status2 = reader.readWithDomain(data2.data(), time2.data(), &count2, 1000);
    ASSERT_EQ(status2.getReadStatus(), ReadStatus::Ok);
    ASSERT_TRUE(status2.getValid());
    ASSERT_EQ(count2, samplesToRead2);

    std::vector<double> expectedData2(samplesToRead2);
    std::generate(expectedData2.begin(),
                  expectedData2.end(),
                  [n = samplesToRead0 + samplesToRead1]() mutable
                  {
                      const auto res = n * n;
                      n++;
                      return res;
                  });
    ASSERT_THAT(data2, testing::ElementsAreArray(expectedData2));

    std::vector<int64_t> expectedTime2(samplesToRead2);
    std::generate(expectedTime2.begin(), expectedTime2.end(), [n = samplesToRead0 + samplesToRead1]() mutable { return n++; });
    ASSERT_THAT(time2, testing::ElementsAreArray(expectedTime2));

    ctx.getScheduler().stop();
}
