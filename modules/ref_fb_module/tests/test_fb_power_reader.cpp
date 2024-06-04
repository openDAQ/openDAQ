#include <opendaq/context_internal_ptr.h>
#include <opendaq/instance_factory.h>
#include <opendaq/module_ptr.h>
#include <opendaq/opendaq.h>
#include <ref_fb_module/module_dll.h>
#include <testutils/memcheck_listener.h>
#include <gmock/gmock.h>

using namespace daq;

static ModulePtr CreateModule()
{
    ModulePtr module;
    auto logger = Logger();
    createModule(&module, Context(Scheduler(logger), logger, nullptr, nullptr, nullptr));
    return module;
}

class PowerReaderTest : public testing::Test
{
protected:
    SignalConfigPtr voltageSignal;
    SignalConfigPtr currentSignal;
    SignalConfigPtr timeSignal;
    ContextPtr ctx;

    void createSignals()
    {
        ctx = NullContext();

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
    const auto module = CreateModule();

    auto fb = module.createFunctionBlock("ref_fb_module_power_reader", nullptr, "id");
    ASSERT_TRUE(fb.assigned());
}

TEST_F(PowerReaderTest, Connect)
{
    const auto module = CreateModule();

    auto fb = module.createFunctionBlock("ref_fb_module_power_reader", nullptr, "id");
    ASSERT_TRUE(fb.assigned());

    createSignals();

    fb.getInputPorts()[0].connect(voltageSignal);
    fb.getInputPorts()[1].connect(currentSignal);
}

TEST_F(PowerReaderTest, Simple)
{
    const auto module = CreateModule();

    auto fb = module.createFunctionBlock("ref_fb_module_power_reader", nullptr, "id");
    ASSERT_TRUE(fb.assigned());

    createSignals();

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

    const auto reader = StreamReader(powerSignal, SampleType::Float64, SampleType::Int64, ReadMode::Scaled, ReadTimeoutType::All);

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
    const auto module = CreateModule();

    auto fb = module.createFunctionBlock("ref_fb_module_power_reader", nullptr, "id");
    ASSERT_TRUE(fb.assigned());

    createSignals();

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

    const auto reader = StreamReader(powerSignal, SampleType::Float64, SampleType::Int64, ReadMode::Scaled, ReadTimeoutType::All);

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


    std::vector<double> data(200);
    std::vector<int64_t> time(200);

    SizeT count = 200;
    reader.readWithDomain(data.data(), time.data(), &count, 10000);
    ASSERT_EQ(count, 200);

    std::vector<double> expectedData(200);
    std::generate(expectedData.begin(),
                  expectedData.end(),
                  [n = 0]() mutable
                  {
                      const auto res = n * n;
                      n++;
                      return res;
                  });
    ASSERT_THAT(data, testing::ElementsAreArray(expectedData));

    std::vector<int64_t> expectedTime(200);
    std::generate(expectedTime.begin(), expectedTime.end(), [n = 0]() mutable { return n++; });
    ASSERT_THAT(time, testing::ElementsAreArray(expectedTime));
}

TEST_F(PowerReaderTest, DisconnectReconnect)
{
    const auto module = CreateModule();

    auto fb = module.createFunctionBlock("ref_fb_module_power_reader", nullptr, "id");
    ASSERT_TRUE(fb.assigned());

    createSignals();

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

    const auto reader = StreamReader(powerSignal, SampleType::Float64, SampleType::Int64, ReadMode::Scaled, ReadTimeoutType::All);

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
}

TEST_F(PowerReaderTest, Gap)
{
    const auto module = CreateModule();

    auto fb = module.createFunctionBlock("ref_fb_module_power_reader", nullptr, "id");
    ASSERT_TRUE(fb.assigned());

    createSignals();

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

    const auto reader = StreamReader(powerSignal, SampleType::Float64, SampleType::Int64, ReadMode::Scaled, ReadTimeoutType::All);

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
