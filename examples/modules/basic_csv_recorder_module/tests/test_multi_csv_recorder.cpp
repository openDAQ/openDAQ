#include <basic_csv_recorder_module/module_dll.h>
#include <opendaq/context_internal_ptr.h>
#include <opendaq/instance_factory.h>
#include <opendaq/module_ptr.h>
#include <opendaq/opendaq.h>
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

class MultiCsvTest : public testing::Test
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

        auto config = module.getAvailableFunctionBlockTypes().get("MultiCsvRecorder").createDefaultConfig();
        config.setPropertyValue("ReaderNotificationMode", 1);

        // Create function block
        fb = module.createFunctionBlock("MultiCsvRecorder", nullptr, "fb", config);

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

TEST_F(MultiCsvTest, Create)
{
    ASSERT_TRUE(fb.assigned());
    ASSERT_EQ(fb.getStatusContainer().getStatus("ComponentStatus"), ComponentStatus::Warning);
}

TEST_F(MultiCsvTest, ConnectSignal)
{
    ASSERT_NO_THROW(fb.getInputPorts()[0].connect(validSignals[0]));
    ASSERT_EQ(fb.getStatusContainer().getStatus("ComponentStatus"), ComponentStatus::Ok);
}

TEST_F(MultiCsvTest, ConnectSignals)
{
    for (size_t i = 0; i < validSignals.getCount(); ++i)
    {
        ASSERT_NO_THROW(fb.getInputPorts()[0].connect(validSignals[i]));
    }

    ASSERT_EQ(fb.getStatusContainer().getStatus("ComponentStatus"), ComponentStatus::Ok);
}

TEST_F(MultiCsvTest, DisconnectSignals)
{
    for (size_t i = 0; i < validSignals.getCount(); ++i)
        fb.getInputPorts()[i].connect(validSignals[i]);

    ASSERT_EQ(fb.getInputPorts().getCount(), 11u);

    for (const auto& ip : fb.getInputPorts())
        ip.disconnect();

    ASSERT_EQ(fb.getStatusContainer().getStatus("ComponentStatus"), ComponentStatus::Warning);
    ASSERT_EQ(fb.getInputPorts().getCount(), 1u);
}
