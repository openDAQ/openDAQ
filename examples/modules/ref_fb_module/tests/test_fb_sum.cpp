#include <opendaq/context_internal_ptr.h>
#include <opendaq/instance_factory.h>
#include <opendaq/module_ptr.h>
#include <opendaq/opendaq.h>
#include <ref_fb_module/module_dll.h>
#include <testutils/memcheck_listener.h>

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

class SumTest: public testing::Test
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
        // Create module
        
        context = createContext();
        module = createModule(context);

        auto config = module.getAvailableFunctionBlockTypes().get("RefFBModuleSumReader").createDefaultConfig();
        config.setPropertyValue("ReaderNotificationMode", 1);

        // Create function block
        fb = module.createFunctionBlock("RefFBModuleSumReader", nullptr, "fb", config);

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


    void sendData(SizeT sampleCount,
                  SizeT offset,
                  bool sendInvalid,
                  std::pair<size_t, size_t> signalRange,
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
                invalidSignals[i].sendPacket(valuePacket);
        }

        for (const auto& signal : extraSignals)
        {
            signal.sendPacket(valuePacket);
        }

        for (const auto& signal : extraDomainSignals)
        {
            signal.sendPacket(domainPacket);
        }
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
    ASSERT_EQ(fb.getStatusContainer().getStatus("ComponentStatus"), ComponentStatus::Ok);
}

TEST_F(SumTest, ConnectSignals)
{
    for (size_t i = 0; i < validSignals.getCount(); ++i)
    {
        ASSERT_NO_THROW(fb.getInputPorts()[0].connect(validSignals[i]));
    }

    ASSERT_EQ(fb.getStatusContainer().getStatus("ComponentStatus"), ComponentStatus::Ok);
}

TEST_F(SumTest, DisconnectSignals)
{
    for (size_t i = 0; i < validSignals.getCount(); ++i)
        fb.getInputPorts()[i].connect(validSignals[i]);

    ASSERT_EQ(fb.getInputPorts().getCount(), 11u);

    for (const auto& ip : fb.getInputPorts())
        ip.disconnect();
    
    ASSERT_EQ(fb.getStatusContainer().getStatus("ComponentStatus"), ComponentStatus::Warning);
    ASSERT_EQ(fb.getInputPorts().getCount(), 1u);
}

TEST_F(SumTest, InvalidSignals)
{
    for (size_t i = 0; i < validSignals.getCount(); ++i)
    {
        fb.getInputPorts()[i * 2].connect(validSignals[i]);
        fb.getInputPorts()[i * 2 + 1].connect(invalidSignals[i]);
    }
    
    ASSERT_EQ(fb.getInputPorts().getCount(), 21u);
    ASSERT_EQ(fb.getStatusContainer().getStatus("ComponentStatus"), ComponentStatus::Warning);
}

TEST_F(SumTest, InvalidSignalsRecovery)
{
    for (size_t i = 0; i < validSignals.getCount(); ++i)
    {
        fb.getInputPorts()[i * 2].connect(validSignals[i]);
        fb.getInputPorts()[i * 2 + 1].connect(invalidSignals[i]);
    }

    ASSERT_EQ(fb.getInputPorts().getCount(), 21u);
    ASSERT_EQ(fb.getStatusContainer().getStatus("ComponentStatus"), ComponentStatus::Warning);

    auto ip = fb.getInputPorts();
    for (int i = static_cast<int>(fb.getInputPorts().getCount()) - 2; i > 0; i-=2)
        ip[i].disconnect();
    
    ASSERT_EQ(fb.getInputPorts().getCount(), 11u);
    ASSERT_EQ(fb.getStatusContainer().getStatus("ComponentStatus"), ComponentStatus::Ok);
}

TEST_F(SumTest, SumSignals)
{
    for (size_t i = 0; i < validSignals.getCount(); ++i)
        fb.getInputPorts()[i].connect(validSignals[i]);

    auto reader = StreamReader<double>(fb.getSignals()[0]);

    sendData(100, 0, false, std::make_pair(0, 10));

    SizeT count = reader.getAvailableCount();
    ASSERT_EQ(count, 0u);

    auto status = reader.read(nullptr, &count);
    ASSERT_EQ(status.getReadStatus(), ReadStatus::Event);
    
    count = reader.getAvailableCount();
    ASSERT_EQ(count, 100u);
    
    double data[100];
    status = reader.read(&data, &count);
    ASSERT_EQ(status.getReadStatus(), ReadStatus::Ok);

    for (auto val : data)
    {
        ASSERT_DOUBLE_EQ(val, 10);
    }
}

TEST_F(SumTest, SumSignalsReconnect)
{
    for (size_t i = 0; i < validSignals.getCount(); ++i)
        fb.getInputPorts()[i].connect(validSignals[i]);
    
    auto reader = StreamReaderBuilder().setSkipEvents(true).setValueReadType(SampleType::Float64).setSignal(fb.getSignals()[0]).build();

    auto ip = fb.getInputPorts();
    for (size_t i = 0; i < 5; ++i)
        ip[i].disconnect();
    
    sendData(100, 0, false, std::make_pair(5, 10));

    SizeT count = reader.getAvailableCount();
    ASSERT_EQ(count, 100u);
    double data[100];
    auto status = reader.read(&data, &count);
    ASSERT_EQ(status.getReadStatus(), ReadStatus::Ok);

    for (auto val : data)
    {
        ASSERT_DOUBLE_EQ(val, 5);
    }

    for (size_t i = 5; i < 10; ++i)
        fb.getInputPorts()[i].connect(validSignals[i - 5]);
    
    sendData(100, 100, false, std::make_pair(0, 10));

    count = reader.getAvailableCount();
    ASSERT_EQ(count, 100u);
    status = reader.read(&data, &count);
    ASSERT_EQ(status.getReadStatus(), ReadStatus::Ok);

    for (auto val : data)
    {
        ASSERT_DOUBLE_EQ(val, 10);
    }
}

TEST_F(SumTest, SumSignalsInvalidRecovery)
{
    for (size_t i = 0; i < validSignals.getCount(); ++i)
    {
        fb.getInputPorts()[i * 2].connect(validSignals[i]);
        fb.getInputPorts()[i * 2 + 1].connect(invalidSignals[i]);
    }

    auto reader = StreamReaderBuilder().setSkipEvents(true).setValueReadType(SampleType::Float64).setSignal(fb.getSignals()[0]).build();

    sendData(100, 0, true, std::make_pair(0, 10));

    auto count = reader.getAvailableCount();
    ASSERT_EQ(count, 0u);

    // Disconnect invalid signals
    auto ip = fb.getInputPorts();
    for (int i = static_cast<int>(fb.getInputPorts().getCount()) - 2; i > 0; i-=2)
        ip[i].disconnect();

    sendData(100, 100, false, std::make_pair(0, 10));

    count = reader.getAvailableCount();
    ASSERT_EQ(count, 100u);

    double data[100];
    auto status = reader.read(&data, &count);
    ASSERT_EQ(status.getReadStatus(), ReadStatus::Ok);

    for (auto val : data)
    {
        ASSERT_DOUBLE_EQ(val, 10);
    }
}

TEST_F(SumTest, ReplaceValidWithInvalid)
{
    for (size_t i = 0; i < validSignals.getCount(); ++i)
        fb.getInputPorts()[i].connect(validSignals[i]);

    ASSERT_NO_THROW(fb.getInputPorts()[0].connect(invalidSignals[0]));
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

    fb.getInputPorts()[0].connect(validSignals[0]);
    fb.getInputPorts()[1].connect(incompatibleSignal);

    auto reader = StreamReaderBuilder().setSkipEvents(true).setValueReadType(SampleType::Float64).setSignal(fb.getSignals()[0]).build();
    sendData(100,
             0,
             false,
             std::make_pair(0, 1),
             List<ISignalConfig>(incompatibleSignal),
             List<ISignalConfig>(incompatibleDomainSignal));

    auto count = reader.getAvailableCount();
    ASSERT_EQ(count, 0u);

    fb.getInputPorts()[1].connect(validSignals[1]);
    sendData(100, 100, false, std::make_pair(0, 2));
    
    count = reader.getAvailableCount();
    ASSERT_EQ(count, 100u);
}
