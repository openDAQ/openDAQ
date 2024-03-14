#include <opendaq/device_info_factory.h>
#include <coreobjects/unit_factory.h>
#include <opendaq/mock/mock_physical_device.h>
#include <opendaq/device_ptr.h>
#include <opendaq/mock/mock_channel_factory.h>
#include <opendaq/signal_factory.h>
#include <opendaq/data_rule_factory.h>
#include <opendaq/folder_ptr.h>
#include <coreobjects/callable_info_factory.h>

using namespace daq;

inline MockPhysicalDeviceImpl::MockPhysicalDeviceImpl(const ContextPtr& ctx, const ComponentPtr& parent, const StringPtr& localId)
    : GenericDevice<>(ctx, parent, localId)
    , mockFolderA(IoFolder(ctx, ioFolder, "mockfolderA"))
    , mockFolderB(IoFolder(ctx, ioFolder, "mockfolderB"))
    , mockChannel1(MockChannel(ctx, ioFolder, "mockch1"))
    , mockChannelA1(MockChannel(ctx, ioFolder, "mockchA1"))
    , mockChannelB1(MockChannel(ctx, ioFolder, "mockchB1"))
    , mockChannelB2(MockChannel(ctx, ioFolder, "mockchB2"))
{
    auto mockSignal = Signal(this->context, signals, "devicetimesig");
    auto mockPrivateSignal = Signal(this->context, signals, "devicetimesigprivate");

    const size_t nanosecondsInSecond = 1000000000;
    auto delta = nanosecondsInSecond / 10000;

    auto valueDescriptor = DataDescriptorBuilder()
                               .setSampleType(SampleType::UInt64)
                               .setRule(LinearDataRule(delta, 100))
                               .setTickResolution(Ratio(1, nanosecondsInSecond))
                               .setOrigin("1970-01-01T00:00:00")
                               .setName("devicetimesig")
                               .build();

    mockSignal.setDescriptor(valueDescriptor);
    signals.addItem(mockSignal);

    mockPrivateSignal.setDescriptor(valueDescriptor);
    mockPrivateSignal.setPublic(false);
    signals.addItem(mockPrivateSignal);

    ioFolder.addItem(mockChannel1);

    ioFolder.addItem(mockFolderA);
    mockFolderA.addItem(mockChannelA1);

    ioFolder.addItem(mockFolderB);
    mockFolderB.addItem(mockChannelB1);
    mockFolderB.addItem(mockChannelB2);
    
    componentA = addFolder("componentA");
    componentA1 = addComponent("componentA1", componentA);
    componentA1.addProperty(StringProperty("StringProp", "foo"));
    componentB = addComponent("componentB");
    componentB.addProperty(IntProperty("IntProp", 5));
    registerProperties();

    const PropertyObjectPtr thisPtr = this->borrowPtr<PropertyObjectPtr>();
    thisPtr.addProperty(StringProperty("TestProperty", "Test").detach());
    this->tags.add("phys_device");
}

MockPhysicalDeviceImpl::~MockPhysicalDeviceImpl()
{
    stopAcq();
}

DeviceInfoPtr MockPhysicalDeviceImpl::onGetInfo()
{
    if (deviceInfo != nullptr)
        return deviceInfo;

    deviceInfo = DeviceInfo("");
    deviceInfo.setName("MockPhysicalDevice");
    deviceInfo.setConnectionString("connection_string");
    deviceInfo.setManufacturer("manufacturer");
    deviceInfo.setManufacturerUri("manufacturer_uri");
    deviceInfo.setModel("model");
    deviceInfo.setProductCode("product_code");
    deviceInfo.setHardwareRevision("hardware_revision");
    deviceInfo.setSoftwareRevision("software_revision");
    deviceInfo.setDeviceManual("device_manual");
    deviceInfo.setDeviceClass("device_class");
    deviceInfo.setSerialNumber("serial_number");
    deviceInfo.setProductInstanceUri("product_instance_uri");
    deviceInfo.setRevisionCounter(123);
    deviceInfo.addProperty(StringProperty("custom_string", "custom_string"));
    deviceInfo.addProperty(IntProperty("custom_int", 1));
    deviceInfo.addProperty(FloatProperty("custom_float", 1.123));
    deviceInfo.freeze();
    return deviceInfo;
}

RatioPtr MockPhysicalDeviceImpl::onGetResolution()
{
    return Ratio(123, 456);
}

uint64_t MockPhysicalDeviceImpl::onGetTicksSinceOrigin()
{
    return 789;
}

std::string MockPhysicalDeviceImpl::onGetOrigin()
{
    return "origin";
}

UnitPtr MockPhysicalDeviceImpl::onGetDomainUnit()
{
    return Unit("unit_symbol", 987, "unit_name", "unit_quantity");
}

void MockPhysicalDeviceImpl::startAcq()
{
    stopAcq();

    auto obj = this->borrowPtr<PropertyObjectPtr>();
    Int generatePacketCount = obj.getPropertyValue("GeneratePackets");
    if (generatePacketCount <= 0)
        return;

    generateThread = std::thread([this, generatePacketCount]() { generatePackets(generatePacketCount); });
}

void MockPhysicalDeviceImpl::stopAcq()
{
    if (generateThread.joinable())
        generateThread.join();
}

void MockPhysicalDeviceImpl::generatePackets(size_t packetCount)
{
    uint64_t time = 0;
    uint64_t tickDelta = 100;

    for (size_t i = 1; i <= packetCount; i++)
    {
        // we want tick values to be 100 % reproducable even if they do not reperesnt the exact real time
        std::this_thread::sleep_for(std::chrono::milliseconds(tickDelta));
        time = i * tickDelta;

        for (const auto& channel : ioFolder.getItems())
        {
            auto mockCh = channel.asPtrOrNull<IMockChannel>();
            if (mockCh.assigned())
                mockCh->generateSamplesUntil(std::chrono::milliseconds(time));
        }
    }
}

void MockPhysicalDeviceImpl::registerProperties()
{
    auto obj = this->borrowPtr<PropertyObjectPtr>();

    auto generateUntil = IntProperty("GeneratePackets", 0);
    obj.addProperty(generateUntil);
    obj.getOnPropertyValueWrite("GeneratePackets") += [this](PropertyObjectPtr& /*obj*/, PropertyValueEventArgsPtr& /*args*/)
    {
        startAcq();
    };

    obj.addProperty(FunctionProperty("stop", ProcedureInfo()));
    obj.setPropertyValue("stop", Procedure(
        [this]()
        {
            stopAcq();
        }
    ));
}

OPENDAQ_DEFINE_CLASS_FACTORY_WITH_INTERFACE(
    INTERNAL_FACTORY,
    MockPhysicalDevice, daq::IDevice,
    daq::IContext*, ctx,
    daq::IComponent*, parent,
    daq::IString*, localId)

