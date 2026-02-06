#include <opendaq/context_factory.h>
#include <ref_fb_module/module_dll.h>
#include <opendaq/data_descriptor_factory.h>
#include <opendaq/signal_factory.h>
#include <opendaq/packet_factory.h>
#include <opendaq/input_port_factory.h>
#include <thread>

#include <testutils/memcheck_listener.h>

using namespace daq;

using TimeDelayFbTest = testing::Test;

static ModulePtr createModule(const ContextPtr& context)
{
    ModulePtr module;
    auto logger = Logger();
    createModule(&module, context);
    return module;
}

static DataDescriptorPtr CreateDataDescriptor(SampleType sampleType, const DataRulePtr& rule = LinearDataRule(1, 0))
{
    return DataDescriptorBuilder().setSampleType(sampleType)
                                  .setRule(LinearDataRule(1, 0))
                                  .build();
}

static DataDescriptorPtr CreateTimeDescriptor(const DataRulePtr& rule = LinearDataRule(1, 0))
{
    return DataDescriptorBuilder().setSampleType(SampleType::Int64)
                                  .setRule(rule)
                                  .setTickResolution(Ratio(1, 1))
                                  .setUnit(Unit("s", -1, "seconds", "time"))
                                  .setOrigin("1970-01-01T00:00:00")
                                  .build();
}

static void PacketCompare(const DataPacketPtr& lhs, const DataPacketPtr& rhs, bool checkData = true)
{
    ASSERT_EQ(lhs.getRawDataSize(), rhs.getRawDataSize());
    ASSERT_EQ(memcmp(lhs.getRawData(), rhs.getRawData(), lhs.getRawDataSize()), 0);

    ASSERT_EQ(lhs.getDataSize(), rhs.getDataSize());
    if (checkData)
        ASSERT_EQ(memcmp(lhs.getData(), rhs.getData(), lhs.getDataSize()), 0);

    const auto lhsDomain = lhs.getDomainPacket();
    const auto rhsDomain = rhs.getDomainPacket();

    if (lhsDomain.assigned())
        PacketCompare(lhsDomain, rhsDomain, false);
}

TEST_F(TimeDelayFbTest, Create)
{
    const auto ctx = NullContext();
    const auto module = createModule(ctx);
    
    FunctionBlockPtr fb;
    ASSERT_NO_THROW(fb = module.createFunctionBlock("RefFBModuleTimeDelay", nullptr, "id"));
    ASSERT_TRUE(fb.assigned());

    // check default properties
    ASSERT_TRUE(fb.hasProperty("TimeDelay"));
    ASSERT_EQ(fb.getPropertyValue("TimeDelay"), 0);

    // check input and outputs
    ASSERT_EQ(fb.getInputPorts().getCount(), 1u);
    ASSERT_EQ(fb.getSignals().getCount(), 1u);
}

TEST_F(TimeDelayFbTest, CreateWithConfig)
{
    const auto ctx = NullContext();
    const auto module = createModule(ctx);

    auto fbConfig = module.getAvailableFunctionBlockTypes().get("RefFBModuleTimeDelay").createDefaultConfig();
    ASSERT_TRUE(fbConfig.hasProperty("TimeDelay"));
    ASSERT_EQ(fbConfig.getPropertyValue("TimeDelay"), 0);

    fbConfig.setPropertyValue("TimeDelay", 100);

    auto fb = module.createFunctionBlock("RefFBModuleTimeDelay", nullptr, "id", fbConfig);
    ASSERT_EQ(fb.getPropertyValue("TimeDelay"), 100);
}

TEST_F(TimeDelayFbTest, SetTimeDelay)
{
    const auto ctx = NullContext();
    const auto module = createModule(ctx);

    auto fb = module.createFunctionBlock("RefFBModuleTimeDelay", nullptr, "id");

    fb.setPropertyValue("TimeDelay", 100);
    ASSERT_EQ(fb.getPropertyValue("TimeDelay"), 100);

    fb.setPropertyValue("TimeDelay", -100);
    ASSERT_EQ(fb.getPropertyValue("TimeDelay"), -100);
}

TEST_F(TimeDelayFbTest, ConnectingSignalWithoutDomain)
{
    const auto ctx = NullContext();
    const auto module = createModule(ctx);

    auto fb = module.createFunctionBlock("RefFBModuleTimeDelay", nullptr, "id");

    const auto dataSignal = SignalWithDescriptor(ctx, CreateDataDescriptor(SampleType::Int64), nullptr, "signal");
    fb.getInputPorts()[0].connect(dataSignal);

    const auto status = fb.getStatusContainer();
    ASSERT_EQ(status.getStatus("ComponentStatus"), "Error");
}

TEST_F(TimeDelayFbTest, ConnectingSignalWithLinearDomain)
{
    const auto ctx = NullContext();
    const auto module = createModule(ctx);

    auto fb = module.createFunctionBlock("RefFBModuleTimeDelay", nullptr, "id");

    const auto dataSignal = SignalWithDescriptor(ctx, CreateDataDescriptor(SampleType::Int64), nullptr, "signal");
    const auto domainSignal = SignalWithDescriptor(ctx, CreateTimeDescriptor(), nullptr, "timeSignal");
    dataSignal.setDomainSignal(domainSignal);
    fb.getInputPorts()[0].connect(dataSignal);

    const auto status = fb.getStatusContainer();
    ASSERT_EQ(status.getStatus("ComponentStatus"), "Ok");
}

TEST_F(TimeDelayFbTest, ConnectingSignalWithExplicitDomain)
{
    const auto ctx = NullContext();
    const auto module = createModule(ctx);

    auto fb = module.createFunctionBlock("RefFBModuleTimeDelay", nullptr, "id");

    const auto dataSignal = SignalWithDescriptor(ctx, CreateDataDescriptor(SampleType::Int64), nullptr, "signal");
    const auto domainSignal = SignalWithDescriptor(ctx, CreateTimeDescriptor(ExplicitDataRule()), nullptr, "timeSignal");
    dataSignal.setDomainSignal(domainSignal);
    fb.getInputPorts()[0].connect(dataSignal);

    const auto status = fb.getStatusContainer();
    ASSERT_EQ(status.getStatus("ComponentStatus"), "Error");
}

TEST_F(TimeDelayFbTest, SendingLinearData)
{
    const auto ctx = NullContext();
    const auto module = createModule(ctx);

    auto fb = module.createFunctionBlock("RefFBModuleTimeDelay", nullptr, "id");

    const auto dataSignal = SignalWithDescriptor(ctx, CreateDataDescriptor(SampleType::Int64), nullptr, "signal");
    const auto domainSignal = SignalWithDescriptor(ctx, CreateTimeDescriptor(), nullptr, "timeSignal");
    dataSignal.setDomainSignal(domainSignal);
    fb.getInputPorts()[0].connect(dataSignal);

    const auto status = fb.getStatusContainer();
    ASSERT_EQ(status.getStatus("ComponentStatus"), "Ok");

    const auto inputPort = InputPort(ctx, nullptr, "ip");
    inputPort.connect(fb.getSignals()[0]);

    // checking with 0 delay
    {
        constexpr size_t sampleCount = 10;
        const auto domainPacket = DataPacket(domainSignal.getDescriptor(), sampleCount, 0);
        const auto dataPacket = DataPacketWithDomain(domainPacket, dataSignal.getDescriptor(), sampleCount, 0);

        domainSignal.sendPacket(domainPacket);
        dataSignal.sendPacket(dataPacket);

        DataPacketPtr outputPacket;
        const auto connection = inputPort.getConnection();
        int timeout = 100;
        while (timeout-- && !outputPacket.assigned())
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
            outputPacket = connection.dequeue().asPtrOrNull<IDataPacket>();
        }
        ASSERT_TRUE(outputPacket.assigned());
        PacketCompare(outputPacket, dataPacket);
        ASSERT_EQ(outputPacket.getDomainPacket().getOffset(), 0);
    }

    // checking with 100 delay
    {
        fb.setPropertyValue("TimeDelay", 100);
    
        constexpr size_t sampleCount = 10;
        const auto domainPacket = DataPacket(domainSignal.getDescriptor(), sampleCount, 0);
        const auto dataPacket = DataPacketWithDomain(domainPacket, dataSignal.getDescriptor(), sampleCount, 0);

        domainSignal.sendPacket(domainPacket);
        dataSignal.sendPacket(dataPacket);

        DataPacketPtr outputPacket;
        const auto connection = inputPort.getConnection();
        int timeout = 100;
        while (timeout-- && !outputPacket.assigned())
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
            outputPacket = connection.dequeue().asPtrOrNull<IDataPacket>();
        }
        ASSERT_TRUE(outputPacket.assigned());
    
        PacketCompare(outputPacket, dataPacket);
        ASSERT_EQ(outputPacket.getDomainPacket().getOffset(), 100);
    }
}

TEST_F(TimeDelayFbTest, SendingExplicitData)
{
    const auto ctx = NullContext();
    const auto module = createModule(ctx);

    auto fb = module.createFunctionBlock("RefFBModuleTimeDelay", nullptr, "id");

    const auto dataSignal = SignalWithDescriptor(ctx, CreateDataDescriptor(SampleType::Int64, ExplicitDataRule()), nullptr, "signal");
    const auto domainSignal = SignalWithDescriptor(ctx, CreateTimeDescriptor(), nullptr, "timeSignal");
    dataSignal.setDomainSignal(domainSignal);
    fb.getInputPorts()[0].connect(dataSignal);

    const auto status = fb.getStatusContainer();
    ASSERT_EQ(status.getStatus("ComponentStatus"), "Ok");

    const auto inputPort = InputPort(ctx, nullptr, "ip");
    inputPort.connect(fb.getSignals()[0]);

    // checking with 0 delay
    {
        constexpr size_t sampleCount = 10;
        const auto domainPacket = DataPacket(domainSignal.getDescriptor(), sampleCount, 0);
        const auto dataPacket = DataPacketWithDomain(domainPacket, dataSignal.getDescriptor(), sampleCount, 0);

        domainSignal.sendPacket(domainPacket);
        dataSignal.sendPacket(dataPacket);

        DataPacketPtr outputPacket;
        const auto connection = inputPort.getConnection();
        int timeout = 100;
        while (timeout-- && !outputPacket.assigned())
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
            outputPacket = connection.dequeue().asPtrOrNull<IDataPacket>();
        }
        ASSERT_TRUE(outputPacket.assigned());
        PacketCompare(outputPacket, dataPacket);
        ASSERT_EQ(outputPacket.getDomainPacket().getOffset(), 0);
    }

    // checking with 100 delay
    {
        fb.setPropertyValue("TimeDelay", 100);
    
        constexpr size_t sampleCount = 10;
        const auto domainPacket = DataPacket(domainSignal.getDescriptor(), sampleCount, 0);
        const auto dataPacket = DataPacketWithDomain(domainPacket, dataSignal.getDescriptor(), sampleCount, 0);

        domainSignal.sendPacket(domainPacket);
        dataSignal.sendPacket(dataPacket);

        DataPacketPtr outputPacket;
        const auto connection = inputPort.getConnection();
        int timeout = 100;
        while (timeout-- && !outputPacket.assigned())
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
            outputPacket = connection.dequeue().asPtrOrNull<IDataPacket>();
        }
        ASSERT_TRUE(outputPacket.assigned());
    
        PacketCompare(outputPacket, dataPacket);
        ASSERT_EQ(outputPacket.getDomainPacket().getOffset(), 100);
    }
}