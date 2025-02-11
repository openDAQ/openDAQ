#include <coretypes/common.h>
#include <gmock/gmock.h>
#include <opendaq/config_provider_factory.h>
#include <opendaq/context_factory.h>
#include <opendaq/data_packet_ptr.h>
#include <opendaq/device_ptr.h>
#include <opendaq/event_packet_ids.h>
#include <opendaq/event_packet_params.h>
#include <opendaq/event_packet_ptr.h>
#include <opendaq/input_port_factory.h>
#include <opendaq/module_ptr.h>
#include <opendaq/range_factory.h>
#include <opendaq/reader_factory.h>
#include <opendaq/removable_ptr.h>
#include <opendaq/search_filter_factory.h>
#include <ref_template_device_module/module_dll.h>
#include <ref_template_device_module/version.h>
#include <testutils/testutils.h>
#include <thread>
#include "../../../core/opendaq/opendaq/tests/test_config_provider.h"
#include <opendaq/instance_factory.h>
#include <coreobjects/property_object_factory.h>
#include <coreobjects/property_factory.h>
#include <chrono>
#include <coretypes/filesystem.h>

using namespace daq;
using namespace test_config_provider_helpers;
using RefTemplateDeviceModuleTestConfig = ConfigProviderTest;
using RefTemplateDeviceTestInstance = testing::Test;

class RefTemplateDeviceModuleTest : public testing::Test
{
public:

    void createDevice(const StringPtr& connectionString, const ComponentPtr& parent, const PropertyObjectPtr& config = nullptr)
    {
        device = module.createDevice(connectionString, parent, config);
    }

    void SetUp() override
    {
        createModule(&module, NullContext());
    }

    void TearDown() override
    {
        if (device.assigned() && !device.isRemoved())
            device.remove();
    }
    
    ModulePtr module;
    DevicePtr device;
};

TEST_F(RefTemplateDeviceModuleTest, CreateModule)
{
    IModule* module = nullptr;
    ErrCode errCode = createModule(&module, NullContext());
    ASSERT_TRUE(OPENDAQ_SUCCEEDED(errCode));

    ASSERT_NE(module, nullptr);
    module->releaseRef();
}

TEST_F(RefTemplateDeviceModuleTest, ModuleName)
{
    ASSERT_EQ(module.getModuleInfo().getName(), "TemplateReferenceDeviceModule");
}

TEST_F(RefTemplateDeviceModuleTest, VersionAvailable)
{
    ASSERT_TRUE(module.getModuleInfo().getVersionInfo().assigned());
}

TEST_F(RefTemplateDeviceModuleTest, VersionCorrect)
{
    auto version = module.getModuleInfo().getVersionInfo();

    ASSERT_EQ(version.getMajor(), REF_TEMPLATE_DEVICE_MODULE_MAJOR_VERSION);
    ASSERT_EQ(version.getMinor(), REF_TEMPLATE_DEVICE_MODULE_MINOR_VERSION);
    ASSERT_EQ(version.getPatch(), REF_TEMPLATE_DEVICE_MODULE_PATCH_VERSION);
}

TEST_F(RefTemplateDeviceModuleTest, EnumerateDevices)
{
    ListPtr<IDeviceInfo> deviceInfo;
    ASSERT_NO_THROW(deviceInfo = module.getAvailableDevices());

    ASSERT_EQ(deviceInfo.getCount(), 2u);
    ASSERT_EQ(deviceInfo[0].getConnectionString(), "daq.template://device0");
    ASSERT_EQ(deviceInfo[1].getConnectionString(), "daq.template://device1");
}

TEST_F(RefTemplateDeviceModuleTest, CreateDeviceConnectionStringNull)
{
    ASSERT_THROW(createDevice(nullptr, nullptr), ArgumentNullException);
}

TEST_F(RefTemplateDeviceModuleTest, CreateDeviceConnectionStringEmpty)
{
    ASSERT_THROW(createDevice("", nullptr), InvalidParameterException);
}

TEST_F(RefTemplateDeviceModuleTest, CreateDeviceConnectionStringInvalid)
{
    ASSERT_THROW(createDevice("fdfdfdfdde", nullptr), NotFoundException);
}

TEST_F(RefTemplateDeviceModuleTest, CreateDeviceConnectionStringInvalidId)
{
    ASSERT_THROW(createDevice("daq.template://devicett3axxr1", nullptr), NotFoundException);
}

TEST_F(RefTemplateDeviceModuleTest, CreateDeviceConnectionStringOutOfRange)
{
    ASSERT_THROW(createDevice("daq.template://device3", nullptr), NotFoundException);
}

TEST_F(RefTemplateDeviceModuleTest, CreateDeviceConnectionStringCorrect)
{
    ASSERT_NO_THROW(createDevice("daq.template://device0", nullptr));
}

TEST_F(RefTemplateDeviceModuleTest, DeviceDomainResolution)
{
    createDevice("daq.template://device0", nullptr);
    auto domain = device.getDomain();

    auto res = domain.getTickResolution();
    ASSERT_EQ(res, Ratio(1, 1000000));
}

TEST_F(RefTemplateDeviceModuleTest, DeviceDomainUnit)
{
    createDevice("daq.template://device0", nullptr);
    auto domain = device.getDomain();

    auto unit = domain.getUnit();
    ASSERT_EQ(unit.getSymbol(), "s");
    ASSERT_EQ(unit.getName(), "second");
    ASSERT_EQ(unit.getQuantity(), "time");
}

TEST_F(RefTemplateDeviceModuleTest, DeviceDomainTicksSinceEpoch)
{
    createDevice("daq.template://device0", nullptr);

    auto res = device.getTicksSinceOrigin();
    ASSERT_GT(res, 0u);
}

TEST_F(RefTemplateDeviceModuleTest, DeviceDomainOrigin)
{
    createDevice("daq.template://device0", nullptr);
    auto domain = device.getDomain();

    auto res = domain.getOrigin();
    ASSERT_FALSE(static_cast<std::string>(res).empty());
}

TEST_F(RefTemplateDeviceModuleTest, DeviceDomainReferenceDomainId)
{
    createDevice("daq.template://device1", nullptr);
    auto domain = device.getDomain();

    auto res = domain.getReferenceDomainInfo().getReferenceDomainId();
    ASSERT_EQ(res, "openDAQ_TempRefDev1");
}

TEST_F(RefTemplateDeviceModuleTest, DeviceDomainReferenceDomainOffset)
{
    createDevice("daq.template://device1", nullptr);
    auto domain = device.getDomain();

    auto res = domain.getReferenceDomainInfo().getReferenceDomainOffset();
    ASSERT_EQ(res, 0);
}

TEST_F(RefTemplateDeviceModuleTest, DeviceDomainReferenceTimeSource)
{
    createDevice("daq.template://device1", nullptr);
    auto domain = device.getDomain();

    auto res = domain.getReferenceDomainInfo().getReferenceTimeSource();
    ASSERT_EQ(res, TimeSource::Unknown);
}

TEST_F(RefTemplateDeviceModuleTest, DeviceDomainUsesOffset)
{
    createDevice("daq.template://device1", nullptr);
    auto domain = device.getDomain();

    auto res = domain.getReferenceDomainInfo().getUsesOffset();
    ASSERT_EQ(res, UsesOffset::Unknown);
}

TEST_F(RefTemplateDeviceModuleTest, GetAvailableComponentTypes)
{
    DictPtr<IString, IFunctionBlockType> functionBlockTypes;
    ASSERT_NO_THROW(functionBlockTypes = module.getAvailableFunctionBlockTypes());
    ASSERT_EQ(functionBlockTypes.getCount(), 0u);

    DictPtr<IString, IDeviceType> deviceTypes;
    ASSERT_NO_THROW(deviceTypes = module.getAvailableDeviceTypes());
    ASSERT_EQ(deviceTypes.getCount(), 1u);
    ASSERT_TRUE(deviceTypes.hasKey("TemplateReferenceDevice"));
    ASSERT_EQ(deviceTypes.get("TemplateReferenceDevice").getId(), "TemplateReferenceDevice");

    DictPtr<IString, IServerType> serverTypes;
    ASSERT_NO_THROW(serverTypes = module.getAvailableServerTypes());
    ASSERT_EQ(serverTypes.getCount(), 0u);

    // Check module info for module
    ModuleInfoPtr moduleInfo;
    ASSERT_NO_THROW(moduleInfo = module.getModuleInfo());
    ASSERT_NE(moduleInfo, nullptr);
    ASSERT_EQ(moduleInfo.getName(), "TemplateReferenceDeviceModule");
    ASSERT_EQ(moduleInfo.getId(), "TemplateReferenceDevice");

    // Check version info for module
    VersionInfoPtr versionInfoModule;
    ASSERT_NO_THROW(versionInfoModule = moduleInfo.getVersionInfo());
    ASSERT_NE(versionInfoModule, nullptr);
    ASSERT_EQ(versionInfoModule.getMajor(), REF_TEMPLATE_DEVICE_MODULE_MAJOR_VERSION);
    ASSERT_EQ(versionInfoModule.getMinor(), REF_TEMPLATE_DEVICE_MODULE_MINOR_VERSION);
    ASSERT_EQ(versionInfoModule.getPatch(), REF_TEMPLATE_DEVICE_MODULE_PATCH_VERSION);

    // Check module and version info for device types
    for (const auto& deviceType : deviceTypes)
    {
        ModuleInfoPtr moduleInfoDeviceType;
        ASSERT_NO_THROW(moduleInfoDeviceType = deviceType.second.getModuleInfo());
        ASSERT_NE(moduleInfoDeviceType, nullptr);
        ASSERT_EQ(moduleInfoDeviceType.getName(), "TemplateReferenceDeviceModule");
        ASSERT_EQ(moduleInfoDeviceType.getId(), "TemplateReferenceDevice");

        VersionInfoPtr versionInfoDeviceType;
        ASSERT_NO_THROW(versionInfoDeviceType = moduleInfoDeviceType.getVersionInfo());
        ASSERT_NE(versionInfoDeviceType, nullptr);
        ASSERT_EQ(versionInfoDeviceType.getMajor(), REF_TEMPLATE_DEVICE_MODULE_MAJOR_VERSION);
        ASSERT_EQ(versionInfoDeviceType.getMinor(), REF_TEMPLATE_DEVICE_MODULE_MINOR_VERSION);
        ASSERT_EQ(versionInfoDeviceType.getPatch(), REF_TEMPLATE_DEVICE_MODULE_PATCH_VERSION);
    }
}

TEST_F(RefTemplateDeviceModuleTest, CreateFunctionBlockIdNull)
{
    FunctionBlockPtr functionBlock;
    ASSERT_THROW(functionBlock = module.createFunctionBlock(nullptr, nullptr, "Id"), ArgumentNullException);
}

TEST_F(RefTemplateDeviceModuleTest, CreateFunctionBlockIdEmpty)
{
    ASSERT_THROW(module.createFunctionBlock("", nullptr, "Id"), NotFoundException);
}

TEST_F(RefTemplateDeviceModuleTest, DeviceNumberOfChannels)
{
    createDevice("daq.template://device0", nullptr);

    Int numChannels = device.getPropertyValue("NumberOfChannels");
    ASSERT_EQ(numChannels, 2);

    ASSERT_EQ(device.getChannels().getCount(), 2u);
}

TEST_F(RefTemplateDeviceModuleTest, DeviceChangeNumberOfChannels)
{
    createDevice("daq.template://device0", nullptr);

    device.setPropertyValue("NumberOfChannels", 5);
    Int numChannels = device.getPropertyValue("NumberOfChannels");
    ASSERT_EQ(numChannels, 5);
    ASSERT_EQ(device.getChannels().getCount(), 5u);

    device.setPropertyValue("NumberOfChannels", 3);
    numChannels = device.getPropertyValue("NumberOfChannels");
    ASSERT_EQ(numChannels, 3);
    ASSERT_EQ(device.getChannels().getCount(), 3u);
}

TEST_F(RefTemplateDeviceModuleTest, DeviceChangeAcqLoopTime)
{
    createDevice("daq.template://device0", nullptr);

    device.setPropertyValue("AcquisitionLoopTime", 100);
    Int acqLoopTime = device.getPropertyValue("AcquisitionLoopTime");
    ASSERT_EQ(acqLoopTime, 100);
}

TEST_F(RefTemplateDeviceModuleTest, DeviceGlobalSampleRate)
{
    createDevice("daq.template://device0", nullptr);

    Float globalSampleRate = device.getPropertyValue("SampleRate");
    ASSERT_DOUBLE_EQ(globalSampleRate, 1000.0);

    device.setPropertyValue("SampleRate", 500.0);
    globalSampleRate = device.getPropertyValue("SampleRate");
    ASSERT_DOUBLE_EQ(globalSampleRate, 500.0);
}

TEST_F(RefTemplateDeviceModuleTest, ChannelWaveform)
{
    createDevice("daq.template://device0", nullptr);
    auto channels = device.getChannels();
    auto channel = channels[0];

    Int waveform = channel.getPropertyValue("Waveform");
    ASSERT_EQ(waveform, 0);
    channel.setPropertyValue("Waveform", 1);
    waveform = channel.getPropertyValue("Waveform");
    ASSERT_EQ(waveform, 1);
}

TEST_F(RefTemplateDeviceModuleTest, ChannelFrequency)
{
    createDevice("daq.template://device0", nullptr);
    auto channels = device.getChannels();
    auto channel = channels[0];

    Int frequency = channel.getPropertyValue("Frequency");
    ASSERT_FLOAT_EQ(frequency, 10.0);
    channel.setPropertyValue("Frequency", 100.0);
    frequency = channel.getPropertyValue("Frequency");
    ASSERT_FLOAT_EQ(frequency, 100.0);
}

TEST_F(RefTemplateDeviceModuleTest, ChannelDC)
{
    createDevice("daq.template://device0", nullptr);
    auto channels = device.getChannels();
    auto channel = channels[0];

    Int dc = channel.getPropertyValue("DC");
    ASSERT_FLOAT_EQ(dc, 0.0);
    channel.setPropertyValue("DC", 1.0);
    dc = channel.getPropertyValue("DC");
    ASSERT_FLOAT_EQ(dc, 1.0);
}

TEST_F(RefTemplateDeviceModuleTest, ChannelAmplitude)
{
    createDevice("daq.template://device0", nullptr);
    auto channels = device.getChannels();
    auto channel = channels[0];

    Int amplitude = channel.getPropertyValue("Amplitude");
    ASSERT_FLOAT_EQ(amplitude, 5.0);
    channel.setPropertyValue("Amplitude", 6.0);
    amplitude = channel.getPropertyValue("Amplitude");
    ASSERT_FLOAT_EQ(amplitude, 6.0);
}

TEST_F(RefTemplateDeviceModuleTest, ChannelName)
{
    createDevice("daq.template://device0", nullptr);
    auto channels = device.getChannels();

    size_t i = 0;
    for (const auto& ch : channels)
    {
        std::string chName = ch.getFunctionBlockType().getName();
        ASSERT_EQ(chName, fmt::format("Reference Channel", ++i));
    }
}

TEST_F(RefTemplateDeviceModuleTest, ChannelNoiseAmplitude)
{
    createDevice("daq.template://device0", nullptr);
    auto channels = device.getChannels();
    auto channel = channels[0];

    Int noiseAmpl = channel.getPropertyValue("NoiseAmplitude");
    ASSERT_FLOAT_EQ(noiseAmpl, 0.0);
    channel.setPropertyValue("NoiseAmplitude", 1.0);
    noiseAmpl = channel.getPropertyValue("NoiseAmplitude");
    ASSERT_FLOAT_EQ(noiseAmpl, 1.0);
}

TEST_F(RefTemplateDeviceModuleTest, ChannelCustomRange)
{
    createDevice("daq.template://device0", nullptr);
    auto channel = device.getChannels()[0];
    auto signal = channel.getSignals()[0];

    auto desc = signal.getDescriptor();
    ASSERT_EQ(desc.getValueRange().getHighValue(), 10.0);
    ASSERT_EQ(desc.getValueRange().getLowValue(), -10.0);

    channel.setPropertyValue("CustomRange", Range(-5.0, 5.0));

    desc = signal.getDescriptor();
    ASSERT_EQ(desc.getValueRange().getHighValue(), 5.0);
    ASSERT_EQ(desc.getValueRange().getLowValue(), -5.0);
}

TEST_F(RefTemplateDeviceModuleTest, ChannelSampleRate)
{
    createDevice("daq.template://device0", nullptr);
    auto channels = device.getChannels();
    auto channel = channels[0];

    channel.setPropertyValue("SampleRate", 10000.0);

    ASSERT_FALSE(channel.getProperty("SampleRate").getVisible());
    channel.setPropertyValue("UseGlobalSampleRate", False);
    auto sampleRate = channel.getPropertyValue("SampleRate");
    ASSERT_DOUBLE_EQ(sampleRate, 10000.0);

    ASSERT_TRUE(channel.getProperty("SampleRate").getVisible());
    device.remove();
}

TEST_F(RefTemplateDeviceModuleTest, CoerceChannelSampleRate)
{
    createDevice("daq.template://device0", nullptr);
    auto channels = device.getChannels();
    auto channel = channels[0];

    device.setPropertyValue("SampleRate", 49999);
    double sampleRate = channel.getPropertyValue("SampleRate");
    ASSERT_DOUBLE_EQ(sampleRate, 50000.0);
}

TEST_F(RefTemplateDeviceModuleTest, Ids)
{
    createDevice("daq.template://device1", nullptr);
    auto channels = device.getChannels();
    auto channel = channels[0];
    auto signals = channel.getSignals(search::Any());
    auto domainSignal = signals[0];
    auto valueSignal = signals[1];

    ASSERT_EQ(channel.getLocalId(), "AI1");
    ASSERT_EQ(channel.getGlobalId(), "/openDAQ_TempRefDev1/IO/AI/AI1");

    ASSERT_EQ(valueSignal.getLocalId(), "AI1");
    ASSERT_EQ(valueSignal.getGlobalId(), "/openDAQ_TempRefDev1/IO/AI/AI1/Sig/AI1");

    ASSERT_EQ(domainSignal.getLocalId(), "AI1Time");
    ASSERT_EQ(domainSignal.getGlobalId(), "/openDAQ_TempRefDev1/IO/AI/AI1/Sig/AI1Time");
}

bool propertyInfoListContainsProperty(const ListPtr<IProperty>& list, const std::string& propName)
{
    auto it = std::find_if(list.begin(), list.end(), [propName](const PropertyPtr& prop) { return prop.getName() == propName; });

    return it != list.end();
}

bool propertyInfoListDoesntContainProperty(const ListPtr<IProperty>& list, const std::string& propName)
{
    return !propertyInfoListContainsProperty(list, propName);
}

TEST_F(RefTemplateDeviceModuleTest, ChannelProperties)
{
    createDevice("daq.template://device0", nullptr);
    auto channels = device.getChannels();
    auto channel = channels[0];

    channel.setPropertyValue("Waveform", 0);
    auto visibleProps = channel.getVisibleProperties();
    ASSERT_PRED2(propertyInfoListContainsProperty, visibleProps, "Amplitude");

    channel.setPropertyValue("Waveform", 3);
    visibleProps = channel.getVisibleProperties();
    ASSERT_PRED2(propertyInfoListDoesntContainProperty, visibleProps, "Amplitude");
}

TEST_F(RefTemplateDeviceModuleTest, SignalCheck)
{
    createDevice("daq.template://device0", nullptr);
    auto channels = device.getChannels();
    auto channel = channels[0];
    auto signals = channel.getSignals(search::Any());
    ASSERT_EQ(signals.getCount(), 2u);
}

TEST_F(RefTemplateDeviceModuleTest, DeviceRemoveDisconnectsInputPort)
{
    createDevice("daq.template://device0", nullptr);
    auto channels = device.getChannels();
    auto channel = channels[0];
    auto signals = channel.getSignals();

    auto inputPort = InputPort(NullContext(), nullptr, "Input");
    inputPort.connect(signals[0]);

    ASSERT_EQ(inputPort.getSignal(), signals[0]);

    device.asPtr<IRemovable>().remove();
    ASSERT_EQ(inputPort.getSignal(), nullptr);
}

TEST_F(RefTemplateDeviceModuleTest, ChannelRemovedDisconnectsInputPort)
{
    createDevice("daq.template://device0", nullptr);
    auto channels = device.getChannels();
    auto channel = channels[1];
    auto signals = channel.getSignals();

    auto inputPort = InputPort(NullContext(), nullptr, "Input");
    inputPort.connect(signals[0]);

    ASSERT_EQ(inputPort.getSignal(), signals[0]);

    device.setPropertyValue("NumberOfChannels", 1);
    ASSERT_EQ(inputPort.getSignal(), nullptr);
}

TEST_F(RefTemplateDeviceModuleTest, CreateDeviceTwice)
{
    ASSERT_NO_THROW(createDevice("daq.template://device0", nullptr));
    ASSERT_THROW(createDevice("daq.template://device0", nullptr), AlreadyExistsException);
}

TEST_F(RefTemplateDeviceModuleTest, CreateReleaseAndCreateDevice)
{
    ASSERT_NO_THROW(createDevice("daq.template://device0", nullptr));
    device.remove();
    device.release();
    ASSERT_NO_THROW(createDevice("daq.template://device0", nullptr));
}

TEST_F(RefTemplateDeviceModuleTest, Folders)
{
    createDevice("daq.template://device0", nullptr);
    FolderPtr ioFolder = device.getItem("IO");
    FolderPtr aiFolder = ioFolder.getItem("AI");
    ChannelPtr chX = aiFolder.getItems()[0];

    auto channels = device.getChannels();
    auto chY = channels[0];

    ASSERT_EQ(chX, chY);
}

TEST_F(RefTemplateDeviceModuleTest, Serialize)
{
    createDevice("daq.template://device0", nullptr);

    device.setPropertyValue("NumberOfChannels", 5);
    device.setPropertyValue("SampleRate", 500.0);

    auto serializer = JsonSerializer(True);

    device.serialize(serializer);

    auto str = serializer.getOutput();
    std::cout << str << std::endl;
}

TEST_F(RefTemplateDeviceModuleTest, DeviceEnableCANChannel)
{
    createDevice("daq.template://device0", nullptr);

    Int numChannels = device.getPropertyValue("NumberOfChannels");
    ASSERT_EQ(numChannels, 2);

    ASSERT_EQ(device.getChannels().getCount(), 2u);

    device.setPropertyValue("EnableCANChannel", True);

    ASSERT_EQ(device.getChannels().getCount(), 3u);
}

TEST_F(RefTemplateDeviceModuleTest, ReadCANChannel)
{
    createDevice("daq.template://device0", nullptr);

    device.setPropertyValue("EnableCANChannel", True);

    const ChannelPtr canCh = device.getInputsOutputsFolder().getItem("CAN").asPtr<IFolder>().getItems()[0];

    const auto canSignal = canCh.getSignals()[0];
    const auto canTimeSignal = canSignal.getDomainSignal();
    const auto packetReader = PacketReader(canSignal);

    while (packetReader.getAvailableCount() < 2u)
        std::this_thread::sleep_for(std::chrono::milliseconds(10));

    EventPacketPtr eventPacket = packetReader.read();
    ASSERT_EQ(eventPacket.getEventId(), event_packet_id::DATA_DESCRIPTOR_CHANGED);

    const DataDescriptorPtr dataDesc = eventPacket.getParameters().get(event_packet_param::DATA_DESCRIPTOR);
    const DataDescriptorPtr domainDesc = eventPacket.getParameters().get(event_packet_param::DOMAIN_DATA_DESCRIPTOR);
    ASSERT_EQ(dataDesc, canSignal.getDescriptor());
    ASSERT_EQ(domainDesc, canTimeSignal.getDescriptor());

    ASSERT_GT(dataDesc.getStructFields().getCount(), 0u);

    DataPacketPtr dataPacket = packetReader.read();
    ASSERT_EQ(dataPacket.getDataDescriptor(), dataDesc);

    const auto sampleCount = dataPacket.getSampleCount();
    ASSERT_GE(sampleCount, 1u);

#pragma pack(push, 1)
    struct CANData
    {
        uint32_t arbId;
        uint8_t length;
        uint8_t data[64];
    };
#pragma pack(pop)

    auto* canData = reinterpret_cast<CANData*>(dataPacket.getData());
    for (size_t i = 0; i < sampleCount; i++)
    {
        ASSERT_EQ(canData->arbId, (uint32_t) 12);
        ASSERT_EQ(canData->length, (uint8_t) 8);
        canData++;
    }
}

TEST_F(RefTemplateDeviceModuleTest, ReadCANChannelWithStreamReader)
{
    createDevice("daq.template://device0", nullptr);

    device.setPropertyValue("EnableCANChannel", True);

    const ChannelPtr canCh = device.getInputsOutputsFolder().getItem("CAN").asPtr<IFolder>().getItems()[0];

    const auto canSignal = canCh.getSignals()[0];
    const auto canTimeSignal = canSignal.getDomainSignal();
    const auto streamReader = StreamReader<void*>(canSignal);

#pragma pack(push, 1)
    struct CANData
    {
        uint32_t arbId;
        uint8_t length;
        uint8_t data[64];
    };
#pragma pack(pop)

    CANData canData[10];
    SizeT count = 0;
    do
    {
        SizeT toRead = 2;
        streamReader.read(&canData[count], &toRead, 10);
        count += toRead;
    } while (count < 2);

    for (size_t i = 0; i < count; i++)
    {
        ASSERT_EQ(canData[i].arbId, (uint32_t) 12);
        ASSERT_EQ(canData[i].length, (uint8_t) 8);
    }
}

TEST_F(RefTemplateDeviceModuleTest, ReadAIChannelWithFixedPacketSize)
{
    constexpr SizeT packetSize = 1000;

    createDevice("daq.template://device0", nullptr);
    device.setPropertyValue("SampleRate", 1000);

    const auto channel = device.getChannels()[0];
    channel.setPropertyValue("FixedPacketSize", True);
    channel.setPropertyValue("PacketSize", packetSize);

    const auto signal = channel.getSignals()[0];
    const auto domainSignal = signal.getDomainSignal();
    const auto packetReader = PacketReader(signal);

    while (packetReader.getAvailableCount() < 1u)
        std::this_thread::sleep_for(std::chrono::milliseconds(10));

    const EventPacketPtr eventPacket = packetReader.read();
    ASSERT_EQ(eventPacket.getEventId(), event_packet_id::DATA_DESCRIPTOR_CHANGED);

    const DataDescriptorPtr dataDesc = eventPacket.getParameters().get(event_packet_param::DATA_DESCRIPTOR);
    const DataDescriptorPtr domainDesc = eventPacket.getParameters().get(event_packet_param::DOMAIN_DATA_DESCRIPTOR);
    ASSERT_EQ(dataDesc, signal.getDescriptor());
    ASSERT_EQ(domainDesc, domainSignal.getDescriptor());

    // there might be old packets in the signal path, so exit when we encounter the first with packet size
    for (;;)
    {
        while (packetReader.getAvailableCount() < 1u)
            std::this_thread::sleep_for(std::chrono::milliseconds(10));

        const DataPacketPtr dataPacket = packetReader.read();
        ASSERT_EQ(dataPacket.getDataDescriptor(), dataDesc);

        const auto sampleCount = dataPacket.getSampleCount();
        if (sampleCount == packetSize)
            break;
    };
}

TEST_F(RefTemplateDeviceModuleTest, ReadConstantRule)
{
    createDevice("daq.template://device0", nullptr);

    const ChannelPtr ch = device.getChannels()[0];
    ch.setPropertyValue("ConstantValue", 4.0);
    ch.setPropertyValue("Waveform", 4);
    const auto signal = ch.getSignals()[0];

    const auto packetReader = PacketReader(signal);
    while (true)
    {
        const auto packet = packetReader.read();
        if (packet.assigned() && packet.getType() == PacketType::Data &&
            packet.asPtr<IDataPacket>(true).getDataDescriptor().getRule().getType() == DataRuleType::Constant)
        {
            DataPacketPtr dataPacket = packet;
            const auto data = reinterpret_cast<double*>(dataPacket.getData());
            ASSERT_EQ(*data, 4.0);
            break;
        }
        else
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
}

TEST_F(RefTemplateDeviceModuleTest, DeviceModuleJsonConfigNoOptions)
{
    ASSERT_NO_THROW(createDevice("daq.template://device1", nullptr));
    ASSERT_EQ(device.getLocalId(), "openDAQ_TempRefDev1");
}

TEST_F(RefTemplateDeviceModuleTestConfig, JsonConfigReadReferenceDeviceLocalId)
{
    std::string filename = "test.json";
    std::string json = "{ \"Modules\": { \"TemplateReferenceDevice\": { \"LocalId\": \"testtest\" } } }";
    createConfigFile(filename, json);

    auto options = GetOptionsWithReferenceDevice();

    auto expectedOptions = GetOptionsWithReferenceDevice();
    getChildren(getChildren(expectedOptions, "Modules"), "TemplateReferenceDevice").set("LocalId", "testtest");

    auto provider = JsonConfigProvider(StringPtr(filename));
    provider.populateOptions(options);

    ASSERT_EQ(options, expectedOptions);
}

TEST_F(RefTemplateDeviceModuleTestConfig, DeviceModuleJsonConfigLocalId)
{
    std::string filename = "test.json";
    std::string json = "{ \"Modules\": { \"TemplateReferenceDevice\": { \"SerialNumber\": \"testtest\" } } }";
    createConfigFile(filename, json);

    auto options = GetDefaultOptions();

    auto provider = JsonConfigProvider(StringPtr(filename));
    provider.populateOptions(options);

    auto context = NullContext(Logger(), TypeManager(), options);

    ModulePtr module;
    createModule(&module, context);

    DevicePtr ptr;
    ASSERT_NO_THROW(ptr = module.createDevice("daq.template://device0", nullptr));
    ASSERT_EQ(ptr.getLocalId(), "openDAQ_testtest");

    ptr.remove();
}

TEST_F(RefTemplateDeviceModuleTestConfig, DeviceModuleJsonConfigName)
{
    std::string filename = "test.json";
    std::string json = "{ \"Modules\": { \"TemplateReferenceDevice\": { \"Name\": \"testname\" } } }";
    createConfigFile(filename, json);

    auto options = GetDefaultOptions();

    auto provider = JsonConfigProvider(StringPtr(filename));
    provider.populateOptions(options);

    auto context = NullContext(Logger(), TypeManager(), options);

    ModulePtr module;
    createModule(&module, context);

    DevicePtr ptr;
    ASSERT_NO_THROW(ptr = module.createDevice("daq.template://device0", nullptr));
    ASSERT_EQ(ptr.getName(), "testname");

    ptr.remove();
}

TEST_F(RefTemplateDeviceModuleTestConfig, DeviceModuleJsonConfigLocalIdAndName)
{
    std::string filename = "test.json";
    std::string json = "{ \"Modules\": { \"TemplateReferenceDevice\": { \"SerialNumber\": \"testtest\", \"Name\": \"testname\" } } }";
    createConfigFile(filename, json);

    auto options = GetDefaultOptions();

    auto provider = JsonConfigProvider(StringPtr(filename));
    provider.populateOptions(options);

    auto context = NullContext(Logger(), TypeManager(), options);

    ModulePtr module;
    createModule(&module, context);

    DevicePtr ptr;
    ASSERT_NO_THROW(ptr = module.createDevice("daq.template://device0", nullptr));
    ASSERT_EQ(ptr.getLocalId(), "openDAQ_testtest");
    ASSERT_EQ(ptr.getName(), "testname");

    ptr.remove();
}

TEST_F(RefTemplateDeviceModuleTestConfig, DeviceModuleJsonConfigMalformed)
{
    std::string filename = "test.json";
    std::string json = "{ \"Modules\": { \"TemplateReferenceDevice\": { \"LocalId\": { \"Error\": \"testtest\" } } } }";
    createConfigFile(filename, json);

    auto options = GetDefaultOptions();

    auto provider = JsonConfigProvider(StringPtr(filename));
    provider.populateOptions(options);

    auto context = NullContext(Logger(), TypeManager(), options);

    ModulePtr module;
    createModule(&module, context);

    DevicePtr device;
    ASSERT_NO_THROW(device = module.createDevice("daq.template://device0", nullptr));

    device.remove();
}

TEST_F(RefTemplateDeviceModuleTestConfig, DeviceModuleJsonConfigDefault)
{
    auto options = GetDefaultOptions();

    auto context = NullContext(Logger(), TypeManager(), options);

    ModulePtr module;
    createModule(&module, context);

    DevicePtr ptr;
    ASSERT_NO_THROW(ptr = module.createDevice("daq.template://device1", nullptr));
    ASSERT_EQ(ptr.getLocalId(), "openDAQ_TempRefDev1");

    ptr.remove();
}

TEST_F(RefTemplateDeviceModuleTestConfig, DeviceModuleJsonConfigEmptyString)
{
    auto options = GetOptionsWithReferenceDevice();

    auto context = NullContext(Logger(), TypeManager(), options);

    ModulePtr module;
    createModule(&module, context);

    DevicePtr ptr;
    ASSERT_NO_THROW(ptr = module.createDevice("daq.template://device1", nullptr));

    ptr.remove();
}

using RefTemplateDeviceInstanceTest = testing::Test;

TEST_F(RefTemplateDeviceInstanceTest, AddRemoveAddDevice)
{
    const auto instance = Instance();

    auto dev0 = instance.addDevice("daq.template://device0");
    auto dev1 = instance.addDevice("daq.template://device1");
    instance.removeDevice(dev0);
    instance.removeDevice(dev1);

    dev0.release();
    dev1.release();

    ASSERT_NO_THROW(dev0 = instance.addDevice("daq.template://device0"));
    ASSERT_NO_THROW(dev1 = instance.addDevice("daq.template://device1"));
    ASSERT_TRUE(dev0.assigned());
    ASSERT_TRUE(dev1.assigned());

    instance.removeDevice(dev0);
    instance.removeDevice(dev1);

    ASSERT_NO_THROW(dev0 = instance.addDevice("daq.template://device0"));
    ASSERT_NO_THROW(dev1 = instance.addDevice("daq.template://device1"));
    ASSERT_TRUE(dev0.assigned());
    ASSERT_TRUE(dev1.assigned());
}

StringPtr getFileLastModifiedTime(const std::string& path)
{
    auto ftime = fs::last_write_time(path);
    auto sctp = std::chrono::time_point_cast<std::chrono::system_clock::duration>(
        ftime - fs::file_time_type::clock::now() + std::chrono::system_clock::now()
    );
    std::time_t cftime = std::chrono::system_clock::to_time_t(sctp);

    std::ostringstream oss;
    oss << std::put_time(std::gmtime(&cftime), "%Y-%m-%dT%H:%M:%SZ");
    return oss.str();
}

TEST_F(RefTemplateDeviceTestInstance, EnableLogging)
{
    StringPtr loggerPath = "ref_template_device_simulator.log";

    PropertyObjectPtr config = PropertyObject();
    config.addProperty(BoolProperty("EnableLogging", true));
    config.addProperty(StringProperty("LoggingPath", loggerPath));

    auto instanceBuilder = InstanceBuilder();
    instanceBuilder.setRootDevice("daq.template://device0", config);
    auto sinks = DefaultSinks(loggerPath);
    for (const auto& sink : sinks)
        instanceBuilder.addLoggerSink(sink);

    const auto instance = instanceBuilder.build();

    {
        auto logFiles = instance.getLogFileInfos();
        auto logFileLastModified = getFileLastModifiedTime(loggerPath);
        ASSERT_EQ(logFiles.getCount(), 1u);
        auto logFile = logFiles[0];
        
        ASSERT_EQ(logFile.getName(), loggerPath);
        ASSERT_NE(logFile.getSize(), 0);
        ASSERT_EQ(logFile.getLastModified(), logFileLastModified);

        StringPtr firstSymb = instance.getLog(loggerPath, 1, 0);
        ASSERT_EQ(firstSymb, "[");

        StringPtr wrongLog = instance.getLog("wrong.log", 1, 0);
        ASSERT_EQ(wrongLog, "");
    }

    {
        instance.getRootDevice().setPropertyValue("LoggingEnabled", false);
        auto logFiles = instance.getLogFileInfos();
        ASSERT_EQ(logFiles.getCount(), 0u);
    }

    {
        instance.getRootDevice().setPropertyValue("LoggingEnabled", true);
        auto logFiles = instance.getLogFileInfos();
        ASSERT_EQ(logFiles.getCount(), 1u);

        StringPtr firstSymb = instance.getLog(loggerPath, 1, 0);
        ASSERT_EQ(firstSymb, "[");
    }
}
