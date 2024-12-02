#include <opendaq/context_internal_ptr.h>
#include <opendaq/instance_factory.h>
#include <opendaq/module_ptr.h>
#include <opendaq/opendaq.h>
#include <ref_fb_module/module_dll.h>
#include <testutils/memcheck_listener.h>
#include <gmock/gmock.h>
#include <opendaq/search_filter_factory.h>

using namespace daq;

static ModulePtr createModule(const ContextPtr& context)
{
    ModulePtr module;
    auto logger = Logger();
    createModule(&module, context);
    return module;
}

class StructDecoderTest : public testing::Test
{
protected:
    static void prepare(const ContextPtr& ctx, SignalConfigPtr& valueSignal, SignalConfigPtr& timeSignal)
    {
        const auto field1Descriptor = DataDescriptorBuilder().setSampleType(SampleType::Float32).setName("Field1").build();

        const auto field2Descriptor = DataDescriptorBuilder()
                                          .setSampleType(SampleType::Int32)
                                          .setDimensions(List<IDimension>(Dimension(LinearDimensionRule(1, 0, 2))))
                                          .setName("Field2")
                                          .build();

        const auto dataDescriptor = DataDescriptorBuilder()
                                        .setSampleType(SampleType::Struct)
                                        .setName("Struct2")
                                        .setStructFields(List<IDataDescriptor>(field1Descriptor, field2Descriptor))
                                        .build();

        const auto timeDescriptor = DataDescriptorBuilder()
                                        .setSampleType(SampleType::Int64)
                                        .setTickResolution(Ratio(1, 1000))
                                        .setUnit(Unit("s", -1, "second", "time"))
                                        .setRule(LinearDataRule(1, 0))
                                        .build();

        valueSignal = SignalWithDescriptor(ctx, dataDescriptor, nullptr, "valuesig");
        timeSignal = SignalWithDescriptor(ctx, timeDescriptor, nullptr, "timesig");

        valueSignal.setDomainSignal(timeSignal);
    }
};

TEST_F(StructDecoderTest, ConnectAndReadStruct)
{
    const auto ctx = NullContext();

    const auto module = createModule(ctx);

    auto fb = module.createFunctionBlock("RefFBModuleStructDecoder", nullptr, "id");
    ASSERT_TRUE(fb.assigned());

    const auto statusContainer = fb.getStatusContainer();
    ASSERT_EQ(statusContainer.getStatus("InputStatus").getValue(), "Disconnected");

    ASSERT_EQ(fb.getSignals().getCount(), 0);
    ASSERT_EQ(fb.getInputPorts().getCount(), 1);

    SignalConfigPtr valueSignal;
    SignalConfigPtr timeSignal;
    prepare(ctx, valueSignal, timeSignal);

    fb.getInputPorts()[0].connect(valueSignal);

    auto signals = fb.getSignals(search::Any());
    ASSERT_EQ(signals.getCount(), 3);

    ASSERT_EQ(signals[0].getDescriptor(), timeSignal.getDescriptor());
    ASSERT_EQ(signals[1].getDescriptor(), valueSignal.getDescriptor().getStructFields()[0]);
    ASSERT_EQ(signals[2].getDescriptor(), valueSignal.getDescriptor().getStructFields()[1]);

    signals = fb.getSignals();
    ASSERT_EQ(signals.getCount(), 2);

    ASSERT_EQ(signals[0].getDescriptor(), valueSignal.getDescriptor().getStructFields()[0]);
    ASSERT_EQ(signals[1].getDescriptor(), valueSignal.getDescriptor().getStructFields()[1]);

    ASSERT_EQ(statusContainer.getStatus("InputStatus").getValue(), "Connected");

    const auto multiReader = MultiReaderBuilder()
        .setReadTimeoutType(ReadTimeoutType::All)
        .setValueReadType(SampleType::Undefined)
        .addSignal(signals[0])
        .addSignal(signals[1])
        .build();

    const auto timePacket = DataPacket(timeSignal.getDescriptor(), 2, 0);
    const auto valuePacket = DataPacketWithDomain(timePacket, valueSignal.getDescriptor(), 2);

    struct Data
    {
        float field1;
        int32_t field2[2];
    };

    auto valueData = static_cast<Data*>(valuePacket.getRawData());
    valueData->field1 = 2.0;
    valueData->field2[0] = 21;
    valueData++->field2[1] = 22;
    valueData->field1 = 3.0;
    valueData->field2[0] = 31;
    valueData++->field2[1] = 32;

    valueSignal.sendPacket(valuePacket);

    std::array<float, 2> valuesFloat;
    std::array<int32_t[2], 2> valuesIntArray;

    void* valuesPerSignal[2]{valuesFloat.data(), valuesIntArray.data()};

    SizeT count = 0;
    auto status = multiReader.read(nullptr, &count, 0);
    ASSERT_EQ(status.getReadStatus(), ReadStatus::Event);

    count = 2;
    status = multiReader.read(valuesPerSignal, &count, 5000);
    ASSERT_EQ(status.getReadStatus(), ReadStatus::Ok);

    ASSERT_EQ(count, 2);
    ASSERT_THAT(valuesFloat, testing::ElementsAre(2.0, 3.0));
    ASSERT_EQ(valuesIntArray[0][0], 21);
    ASSERT_EQ(valuesIntArray[0][1], 22);
    ASSERT_EQ(valuesIntArray[1][0], 31);
    ASSERT_EQ(valuesIntArray[1][1], 32);
}

TEST_F(StructDecoderTest, ConnectAndDisconnect)
{
    const auto ctx = NullContext();

    const auto module = createModule(ctx);

    auto fb = module.createFunctionBlock("RefFBModuleStructDecoder", nullptr, "id");
    ASSERT_TRUE(fb.assigned());

    const auto statusContainer = fb.getStatusContainer();
    ASSERT_EQ(statusContainer.getStatus("InputStatus").getValue(), "Disconnected");

    ASSERT_EQ(fb.getSignals().getCount(), 0);
    ASSERT_EQ(fb.getInputPorts().getCount(), 1);

    SignalConfigPtr valueSignal;
    SignalConfigPtr timeSignal;
    prepare(ctx, valueSignal, timeSignal);

    fb.getInputPorts()[0].connect(valueSignal);

    auto signals = fb.getSignals(search::Any());
    ASSERT_EQ(signals.getCount(), 3);

    fb.getInputPorts()[0].disconnect();

    signals = fb.getSignals(search::Any());
    ASSERT_EQ(signals.getCount(), 0);
}

TEST_F(StructDecoderTest, ConnectScalar)
{
    const auto ctx = NullContext();

    const auto module = createModule(ctx);

    const auto fb = module.createFunctionBlock("RefFBModuleStructDecoder", nullptr, "id");
    ASSERT_TRUE(fb.assigned());

    const auto statusContainer = fb.getStatusContainer();
    ASSERT_EQ(statusContainer.getStatus("InputStatus").getValue(), "Disconnected");

    ASSERT_EQ(fb.getSignals().getCount(), 0);
    ASSERT_EQ(fb.getInputPorts().getCount(), 1);

    const auto dataDescriptor = DataDescriptorBuilder()
                                    .setSampleType(SampleType::Float32)
                                    .build();

    const auto timeDescriptor = DataDescriptorBuilder()
                                    .setSampleType(SampleType::Int64)
                                    .setTickResolution(Ratio(1, 1000))
                                    .setUnit(Unit("s", -1, "second", "time"))
                                    .setRule(LinearDataRule(1, 0))
                                    .build();

    const auto valueSignal = SignalWithDescriptor(ctx, dataDescriptor, nullptr, "valuesig");
    const auto timeSignal = SignalWithDescriptor(ctx, timeDescriptor, nullptr, "timesig");

    valueSignal.setDomainSignal(timeSignal);

    fb.getInputPorts()[0].connect(valueSignal);

    const auto signals = fb.getSignals(search::Any());
    ASSERT_EQ(signals.getCount(), 0);

    ASSERT_EQ(statusContainer.getStatus("InputStatus").getValue(), "Invalid");
}
