#include <opendaq/device_info_factory.h>
#include <coreobjects/unit_factory.h>
#include <opendaq/mock/mock_physical_device.h>
#include <opendaq/device_ptr.h>
#include <opendaq/mock/mock_channel_factory.h>
#include <opendaq/signal_factory.h>
#include <opendaq/data_rule_factory.h>
#include <opendaq/folder_ptr.h>
#include <coreobjects/callable_info_factory.h>

#include "opendaq/device_domain_factory.h"

using namespace daq;

inline MockPhysicalDeviceImpl::MockPhysicalDeviceImpl(const ContextPtr& ctx,
                                                      const ComponentPtr& parent,
                                                      const StringPtr& localId,
                                                      const PropertyObjectPtr& config)
    : GenericDevice<>(ctx, parent, localId)
    , config(config)
    , mockFolderA(IoFolder(ctx, ioFolder, "mockfolderA"))
    , mockFolderB(IoFolder(ctx, ioFolder, "mockfolderB"))
    , mockChannel1(MockChannel(ctx, ioFolder, "mockch1"))
    , mockChannelA1(MockChannel(ctx, mockFolderA, "mockchA1"))
    , mockChannelB1(MockChannel(ctx, mockFolderB, "mockchB1"))
    , mockChannelB2(MockChannel(ctx, mockFolderB, "mockchB2"))
{
    auto mockSignal = Signal(this->context, signals, "devicetimesig");
    auto mockPrivateSignal = Signal(this->context, signals, "devicetimesigprivate");

    const size_t nanosecondsInSecond = 1000000000;
    auto delta = nanosecondsInSecond / 10000;
    time = 0;
    
    this->name = "MockPhysicalDevice";
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

    this->setDeviceDomain(DeviceDomain(Ratio(123, 456), "Origin", Unit("UnitSymbol", 987, "UnitName", "UnitQuantity")));
}

MockPhysicalDeviceImpl::~MockPhysicalDeviceImpl()
{
    stopAcq();
}

DeviceInfoPtr MockPhysicalDeviceImpl::onGetInfo()
{
    auto deviceInfo = DeviceInfoWithChanegableFields(this->getChangeableDeviceInfoDefaultFields());
    deviceInfo.setName("MockPhysicalDevice");
    deviceInfo.setConnectionString("daqmock://phys_device");
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
    deviceInfo.addProperty(StringPropertyBuilder("custom_string", "custom_string").setReadOnly(true).build());
    deviceInfo.addProperty(IntPropertyBuilder("custom_int", 1).setReadOnly(true).build());
    deviceInfo.addProperty(FloatPropertyBuilder("custom_float", 1.123).setReadOnly(true).build());
    deviceInfo.addProperty(StringProperty("TestChangeableField", "Test"));
    return deviceInfo;
}

uint64_t MockPhysicalDeviceImpl::onGetTicksSinceOrigin()
{
    return 789;
}

bool MockPhysicalDeviceImpl::allowAddDevicesFromModules()
{
    return true;
}

void MockPhysicalDeviceImpl::setDeviceDomainHelper(const DeviceDomainPtr& deviceDomain)
{
    this->setDeviceDomain(deviceDomain);
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
    uint64_t tickDelta = 100;

    for (size_t i = 1; i <= packetCount; i++)
    {
        // we want tick values to be 100 % reproducable even if they do not reperesnt the exact real time
        std::this_thread::sleep_for(std::chrono::milliseconds(tickDelta));
        time = time + i * tickDelta;

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

    obj.addProperty(BoolProperty("ChangeDescriptors", 0));
    obj.getOnPropertyValueWrite("ChangeDescriptors") += [this](PropertyObjectPtr& /*obj*/, PropertyValueEventArgsPtr& args)
    {
        for (const SignalConfigPtr& sig : ioFolder.getItems(search::Recursive(search::InterfaceId(ISignal::Id))))
        {
            if (args.getValue() == True)
            {
                const auto builder =
                    DataDescriptorBuilderCopy(sig.getDescriptor()).setMetadata(Dict<IString, IString>({{"new_metadata", "new_value"}}));
                sig.setDescriptor(builder.build());
            }
            else
            {
                const auto builder =
                    DataDescriptorBuilderCopy(sig.getDescriptor()).setMetadata(Dict<IString, IString>());
                sig.setDescriptor(builder.build());
            }

        }
    };

    registerTestConfigProperties();
}

void MockPhysicalDeviceImpl::registerTestConfigProperties()
{
    if (!config.assigned())
        return;

    auto obj = this->borrowPtr<PropertyObjectPtr>();

    if (config.hasProperty("message"))
    {
        const auto prop = config.getProperty("message");
        obj.addProperty(PropertyBuilder(prop.getName()).setValueType(prop.getValueType()).setDefaultValue(prop.getDefaultValue()).build());
        obj.setPropertyValue(prop.getName(), config.getPropertyValue(prop.getName()));
    }
}

OPENDAQ_DEFINE_CLASS_FACTORY_WITH_INTERFACE(
    INTERNAL_FACTORY,
    MockPhysicalDevice, daq::IDevice,
    daq::IContext*, ctx,
    daq::IComponent*, parent,
    daq::IString*, localId,
    daq::IPropertyObject*, config)

