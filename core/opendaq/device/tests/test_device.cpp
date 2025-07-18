#include <gtest/gtest.h>
#include <coreobjects/authentication_provider_factory.h>
#include <coreobjects/user_factory.h>
#include <opendaq/component_exceptions.h>
#include <opendaq/device_ptr.h>
#include <opendaq/device_private_ptr.h>
#include <opendaq/device_impl.h>
#include <opendaq/context_factory.h>
#include <opendaq/device_info_factory.h>
#include <opendaq/io_folder_factory.h>
#include <opendaq/function_block_impl.h>
#include <opendaq/component_deserialize_context_factory.h>
#include <opendaq/channel_impl.h>
#include <opendaq/server_impl.h>
#include <opendaq/io_folder_impl.h>
#include <opendaq/gmock/function_block.h>
#include <opendaq/device_type_factory.h>
#include <opendaq/module_manager_factory.h>
#include <opendaq/scheduler_factory.h>
#include <opendaq/mock/mock_streaming_factory.h>
#include <opendaq/device_network_config_ptr.h>
#include <opendaq/component_private_ptr.h>
#include "testutils/testutils.h"

using DeviceTest = testing::Test;


class TestDevice : public daq::Device
{
public:
    TestDevice(const daq::ContextPtr& ctx = daq::NullContext(), const daq::ComponentPtr& parent = nullptr, const daq::StringPtr& localId = "dev")
        : daq::Device(ctx, parent, localId)
    {
        auto parentFolder = this->addFolder("Folder1");
        this->addFolder("Folder2", parentFolder);
        this->addComponent("Component1");
    }

    daq::DeviceInfoPtr onGetInfo() override
    {
        auto deviceInfo = daq::DeviceInfoWithChanegableFields({"userName", "location"});
        deviceInfo.setConnectionString("conn");
        deviceInfo.setName("test");
        deviceInfo.setManufacturer("test");
        deviceInfo.setSerialNumber("test");
        deviceInfo.setLocation("test");
        deviceInfo.addProperty(daq::StringProperty("CustomChangeableField", "default value"));

        return deviceInfo;
    }
        
    std::set<daq::OperationModeType> onGetAvailableOperationModes() override 
    { 
        return {daq::OperationModeType::Idle, daq::OperationModeType::Operation}; 
    }

protected:
    void onSubmitNetworkConfiguration(const daq::StringPtr& ifaceName, const daq::PropertyObjectPtr& config) override {}
    daq::PropertyObjectPtr onRetrieveNetworkConfiguration(const daq::StringPtr& ifaceName) override
    {
        auto config = daq::PropertyObject();

        config.addProperty(daq::BoolProperty("dhcp4", daq::True));
        config.addProperty(daq::StringProperty("address4", ""));
        config.addProperty(daq::StringProperty("gateway4", ""));
        config.addProperty(daq::BoolProperty("dhcp6", daq::True));
        config.addProperty(daq::StringProperty("address6", ""));
        config.addProperty(daq::StringProperty("gateway6", ""));

        return config;
    }
    daq::Bool onGetNetworkConfigurationEnabled() override { return daq::True; }
    daq::ListPtr<daq::IString> onGetNetworkInterfaceNames() override { return daq::List<daq::IString>("eth0"); }
};

class MockSrvImpl final : public daq::Server
{
public:

    MockSrvImpl(const daq::ContextPtr& ctx, const daq::DevicePtr& rootDev)
        : daq::Server("MockServerId", nullptr, rootDev, ctx)
    {
        createAndAddSignal("sig_srv");
    }
};

class MockFbImpl final : public daq::FunctionBlock
{
public:
    MockFbImpl(const daq::ContextPtr& ctx, const daq::ComponentPtr& parent, const daq::StringPtr& localId)
        : daq::FunctionBlock(daq::FunctionBlockType("test_uid", "test_name", "test_description"), ctx, parent, localId)
    {
        createAndAddSignal("sig_fb");
        createAndAddInputPort("ip_fb", daq::PacketReadyNotification::None);
    }
};

class MockChannel final : public daq::Channel
{
public:
    MockChannel(const daq::ContextPtr& ctx, const daq::ComponentPtr& parent, const daq::StringPtr& localId)
        : daq::Channel(daq::FunctionBlockType("Ch", "", ""), ctx, parent, localId)
    {
        createAndAddSignal("sig_ch");
    }
};

class MockDevice final : public daq::Device
{
public:
    MockDevice(const daq::ContextPtr& ctx, 
               const daq::ComponentPtr& parent, 
               const daq::StringPtr& localId, 
               bool addSubDevice = false)
        : daq::Device(ctx, parent, localId)
    {
        createAndAddSignal("sig_device");

        auto aiIoFolder = this->addIoFolder("AI", ioFolder);
        createAndAddChannel<MockChannel>(aiIoFolder, "Ch");

        const auto fb = daq::createWithImplementation<daq::IFunctionBlock, MockFbImpl>(ctx, this->functionBlocks, "fb");
        addNestedFunctionBlock(fb);

        const auto srv = daq::createWithImplementation<daq::IServer, MockSrvImpl>(ctx, this->template borrowPtr<daq::DevicePtr>());
        servers.addItem(srv);

        if (addSubDevice)
        {
            const auto device = daq::createWithImplementation<daq::IDevice, MockDevice>(ctx, devices, "subDev", false);
            this->addSubDevice(device);
        }
    }

    std::set<daq::OperationModeType> onGetAvailableOperationModes() override 
    { 
        return {daq::OperationModeType::Idle, daq::OperationModeType::Operation, daq::OperationModeType::SafeOperation}; 
    }

    void onOperationModeChanged(daq::OperationModeType modeType) override
    {
        bool active = modeType != daq::OperationModeType::Idle;
        for (const auto& signal : this->signals.getItems(daq::search::InterfaceId(daq::ISignal::Id)))
            signal.setActive(active);
    }
};


TEST_F(DeviceTest, DeviceInfoNameLocationSync)
{
    auto device = daq::createWithImplementation<daq::IDevice, TestDevice>();
    auto info = device.getInfo();

    ASSERT_EQ(info.getLocation(), "test");
    ASSERT_EQ(info.getName(), "dev");

    device.setPropertyValue("location", "new_loc");
    device.setName("new_name");

    ASSERT_EQ(info.getLocation(), "new_loc");
    ASSERT_EQ(info.getName(), "new_name");
}

TEST_F(DeviceTest, DeviceInfoForwardCallbacks)
{
    auto device = daq::createWithImplementation<daq::IDevice, TestDevice>();
    auto info = device.getInfo();

    daq::SizeT readCounter = 0;
    info.getOnPropertyValueRead("CustomChangeableField") += [&readCounter](daq::PropertyObjectPtr& obj, daq::PropertyValueEventArgsPtr& args)
    {
        readCounter++;
    };

    daq::SizeT writeCounter = 0;
    info.getOnPropertyValueWrite("CustomChangeableField") += [&writeCounter](daq::PropertyObjectPtr& obj, daq::PropertyValueEventArgsPtr& args) 
    {
        writeCounter++;
    };

    ASSERT_EQ(info.getPropertyValue("CustomChangeableField"), "default value");
    ASSERT_EQ(writeCounter, 0u);

    info.setPropertyValue("CustomChangeableField", "new_value2");
    ASSERT_EQ(info.getPropertyValue("CustomChangeableField"), "new_value2");
    ASSERT_EQ(writeCounter, 1u);

    // we are reading actualy the owner property
    ASSERT_EQ(readCounter, 2u);
}

TEST_F(DeviceTest, Folders)
{
    auto device = daq::createWithImplementation<daq::IDevice, TestDevice>();

    ASSERT_EQ(device.getSignals().getElementInterfaceId(), daq::ISignal::Id);
    ASSERT_EQ(device.getFunctionBlocks().getElementInterfaceId(), daq::IFunctionBlock::Id);
    ASSERT_EQ(device.getDevices().getElementInterfaceId(), daq::IDevice::Id);
}

TEST_F(DeviceTest, IOFolder)
{
    auto ioFolder = daq::IoFolder(daq::NullContext(), nullptr, "fld");
    ASSERT_TRUE(ioFolder.supportsInterface<daq::IIoFolderConfig>());
}

TEST_F(DeviceTest, IOFolderCreateCustom)
{
    ASSERT_NO_THROW((daq::createWithImplementation<daq::IIoFolderConfig, daq::IoFolderImpl<>>(daq::NullContext(), nullptr, "fld")));
}

TEST_F(DeviceTest, IOFolderSubItems)
{
    auto ioFolder = daq::IoFolder(daq::NullContext(), nullptr, "fld");

    auto subIoFolder = daq::IoFolder(daq::NullContext(), nullptr, "fld1");
    ASSERT_NO_THROW(ioFolder.addItem(subIoFolder));

    auto subFolder = daq::Folder(daq::NullContext(), nullptr, "fld2");
    ASSERT_THROW(ioFolder.addItem(subFolder), daq::InvalidParameterException);

    auto comp = daq::Component(daq::NullContext(), nullptr, "cmp");
    ASSERT_THROW(ioFolder.addItem(comp), daq::InvalidParameterException);

    daq::MockChannel::Strict ch;
    EXPECT_CALL(ch.mock(), getLocalId(testing::_)).WillOnce(daq::Get{daq::String("Ch")});
    ASSERT_NO_THROW(ioFolder.addItem(*ch));
}

TEST_F(DeviceTest, CustomComponentSubItems)
{
    const auto device = daq::createWithImplementation<daq::IDevice, TestDevice>();
    const auto customComponents = device.getCustomComponents();

    ASSERT_EQ(customComponents.getCount(), 2u);

    daq::FolderPtr folder1 = customComponents[0];
    ASSERT_EQ(folder1.getItems().getCount(), 1u);
    ASSERT_FALSE(customComponents[1].asPtrOrNull<daq::IFolder>().assigned());
}

TEST_F(DeviceTest, DefaultProperties)
{
    auto device = daq::createWithImplementation<daq::IDevice, TestDevice>();
    device.getInfo();
    ASSERT_EQ(device.getPropertyValue("location"), "test");
    ASSERT_EQ(device.getPropertyValue("userName"), "");
}

TEST_F(DeviceTest, DeviceTypeStructType)
{
    const auto structType = daq::DeviceTypeStructType();
    const daq::StructPtr structPtr = daq::DeviceType("id", "name", "desc", "prefix");
    ASSERT_EQ(structType, structPtr.getStructType());
}

TEST_F(DeviceTest, DeviceTypeStructFields)
{
    const daq::StructPtr structPtr = daq::DeviceType("id", "name", "desc", "prefix");
    ASSERT_EQ(structPtr.get("Id"), "id");
    ASSERT_EQ(structPtr.get("Name"), "name");
    ASSERT_EQ(structPtr.get("Description"), "desc");
    ASSERT_EQ(structPtr.get("Prefix"), "prefix");
}

TEST_F(DeviceTest, DeviceTypeStructNames)
{
    const auto structType = daq::DeviceTypeStructType();
    const daq::StructPtr structPtr = daq::DeviceType("id", "name", "desc", "prefix");
    ASSERT_EQ(structType.getFieldNames(), structPtr.getFieldNames());
}

TEST_F(DeviceTest, StandardProperties)
{
    const auto name = "foo";
    const auto desc = "bar";
    const auto device = daq::createWithImplementation<daq::IDevice, TestDevice>();

    device.setName(name);
    device.setDescription(desc);

    ASSERT_EQ(device.getName(), name);
    ASSERT_EQ(device.getDescription(), desc);
}

TEST_F(DeviceTest, Remove)
{
    auto device = daq::createWithImplementation<daq::IDevice, TestDevice>();

    ASSERT_NO_THROW(device.remove());
    ASSERT_TRUE(device.isRemoved());

    ASSERT_THROW(device.addDevice("DeviceConnectionString"), daq::ComponentRemovedException);
    ASSERT_THROW(
        device.addDevices(daq::Dict<daq::IString, daq::IPropertyObject>({{"DeviceConnectionString", nullptr}})),
        daq::ComponentRemovedException
    );
    ASSERT_THROW(device.addFunctionBlock("FbTypeId"), daq::ComponentRemovedException);
    ASSERT_THROW(device.addStreaming("StreamingConnectionString"), daq::ComponentRemovedException);

    ASSERT_THROW(device.getAvailableFunctionBlockTypes(), daq::ComponentRemovedException);
    ASSERT_THROW(device.getAvailableDeviceTypes(), daq::ComponentRemovedException);

    ASSERT_THROW(device.getAvailableDevices(), daq::ComponentRemovedException);

    ASSERT_THROW(device.getDevices(), daq::ComponentRemovedException);
    ASSERT_THROW(device.getFunctionBlocks(), daq::ComponentRemovedException);
    ASSERT_THROW(device.getChannels(), daq::ComponentRemovedException);
    ASSERT_THROW(device.getChannelsRecursive(), daq::ComponentRemovedException);
    ASSERT_THROW(device.getSignals(), daq::ComponentRemovedException);
    ASSERT_THROW(device.getSignalsRecursive(), daq::ComponentRemovedException);
    ASSERT_THROW(device.getCustomComponents(), daq::ComponentRemovedException);

    ASSERT_THROW(device.getInfo(), daq::ComponentRemovedException);
    ASSERT_THROW(device.getDomain(), daq::ComponentRemovedException);
    ASSERT_THROW(device.getInputsOutputsFolder(), daq::ComponentRemovedException);
    ASSERT_THROW(device.getTicksSinceOrigin(), daq::ComponentRemovedException);
}

TEST_F(DeviceTest, SerializeAndDeserialize)
{
    const auto dev = daq::createWithImplementation<daq::IDevice, MockDevice>(daq::NullContext(), nullptr, "dev");

    dev.setActive(daq::False);

    const auto serializer = daq::JsonSerializer(daq::True);
    dev.serialize(serializer);
    const auto str1 = serializer.getOutput();

    const auto deserializer = daq::JsonDeserializer();

    const auto deserializeContext = daq::ComponentDeserializeContext(daq::NullContext(), nullptr, nullptr, "dev");

    const daq::DevicePtr newDev = deserializer.deserialize(str1, deserializeContext, nullptr);

    const auto serializer2 = daq::JsonSerializer(daq::True);
    newDev.serialize(serializer2);
    const auto str2 = serializer2.getOutput();

    ASSERT_EQ(str1, str2);

    ASSERT_EQ(newDev.getDevices().getElementInterfaceId(), daq::IDevice::Id);

    ASSERT_FALSE(newDev.getActive());
}

TEST_F(DeviceTest, BeginUpdateEndUpdate)
{
    const auto dev = daq::createWithImplementation<daq::IDevice, MockDevice>(daq::NullContext(), nullptr, "dev");
    dev.addProperty(daq::StringPropertyBuilder("DevProp", "-").build());

    const auto sig = dev.getSignals()[0];
    sig.addProperty(daq::StringPropertyBuilder("SigProp", "-").build());

    dev.beginUpdate();

    dev.setPropertyValue("DevProp", "s");
    ASSERT_EQ(dev.getPropertyValue("DevProp"), "-");

    sig.setPropertyValue("SigProp", "cs");
    ASSERT_EQ(sig.getPropertyValue("SigProp"), "-");

    dev.endUpdate();

    ASSERT_EQ(dev.getPropertyValue("DevProp"), "s");
    ASSERT_EQ(sig.getPropertyValue("SigProp"), "cs");
}

TEST_F(DeviceTest, LockUnlock)
{
    const auto jure = daq::User("jure", "jure");
    const auto tomaz = daq::User("tomaz", "tomaz");

    auto device = daq::createWithImplementation<daq::IDevice, TestDevice>();

    ASSERT_FALSE(device.isLocked());

    device.asPtr<daq::IDevicePrivate>().lock(jure);
    ASSERT_TRUE(device.isLocked());

    ASSERT_THROW(device.asPtr<daq::IDevicePrivate>().lock(tomaz), daq::DeviceLockedException);
    ASSERT_THROW(device.asPtr<daq::IDevicePrivate>().unlock(tomaz), daq::AccessDeniedException);

    device.asPtr<daq::IDevicePrivate>().unlock(jure);
    ASSERT_FALSE(device.isLocked());

    ASSERT_NO_THROW(device.asPtr<daq::IDevicePrivate>().unlock(jure));
    ASSERT_NO_THROW(device.asPtr<daq::IDevicePrivate>().unlock(tomaz));
}

TEST_F(DeviceTest, LockUnlockAnonymous)
{
    const auto jure = daq::User("jure", "jure");
    const auto tomaz = daq::User("tomaz", "tomaz");

    auto device = daq::createWithImplementation<daq::IDevice, TestDevice>();

    ASSERT_FALSE(device.isLocked());

    device.lock();
    ASSERT_TRUE(device.isLocked());

    ASSERT_THROW(device.asPtr<daq::IDevicePrivate>().lock(jure), daq::DeviceLockedException);
    ASSERT_THROW(device.asPtr<daq::IDevicePrivate>().lock(tomaz), daq::DeviceLockedException);

    // unlock anonymous
    device.unlock();
    ASSERT_FALSE(device.isLocked());

    // unlock jure
    device.lock();
    ASSERT_TRUE(device.isLocked());
    device.asPtr<daq::IDevicePrivate>().unlock(jure);
    ASSERT_FALSE(device.isLocked());

    // unlock tomaz
    device.lock();
    ASSERT_TRUE(device.isLocked());
    device.asPtr<daq::IDevicePrivate>().unlock(tomaz);
    ASSERT_FALSE(device.isLocked());
}

TEST_F(DeviceTest, LockUnlockAnonymousInstance)
{
    const auto anonymous = daq::User("", "");
    const auto tomaz = daq::User("tomaz", "tomaz");

    auto device = daq::createWithImplementation<daq::IDevice, TestDevice>();
    ASSERT_FALSE(device.isLocked());

    // unlock tomaz
    device.asPtr<daq::IDevicePrivate>().lock(anonymous);
    ASSERT_TRUE(device.isLocked());
    device.asPtr<daq::IDevicePrivate>().unlock(tomaz);
    ASSERT_FALSE(device.isLocked());

    // unlock anonymous instance
    device.asPtr<daq::IDevicePrivate>().lock(anonymous);
    ASSERT_TRUE(device.isLocked());
    device.asPtr<daq::IDevicePrivate>().unlock(anonymous);
    ASSERT_FALSE(device.isLocked());

    // unlock nullptr
    device.asPtr<daq::IDevicePrivate>().lock(anonymous);
    ASSERT_TRUE(device.isLocked());
    device.unlock();
    ASSERT_FALSE(device.isLocked());
}

TEST_F(DeviceTest, SerializeLocked)
{
    const auto tomaz = daq::User("tomaz", "tomaz");
    auto users = daq::List<daq::IUser>(tomaz);

    auto authenticationProvider = daq::StaticAuthenticationProvider(false, users);

    auto logger = daq::Logger();
    auto context = daq::Context(daq::Scheduler(logger, 1), logger, daq::TypeManager(), nullptr, authenticationProvider);

    auto device = daq::createWithImplementation<daq::IDevice, TestDevice>();
    device.asPtr<daq::IDevicePrivate>().lock(tomaz);
    ASSERT_TRUE(device.isLocked());

    auto serializer = daq::JsonSerializer();
    device.serialize(serializer);
    const auto json = serializer.getOutput();

    auto deserializeContext = daq::ComponentDeserializeContext(context, nullptr, nullptr, "dev");
    auto deserializer = daq::JsonDeserializer();
    const daq::DevicePtr deviceNew = deserializer.deserialize(json, deserializeContext, nullptr);

    ASSERT_TRUE(deviceNew.isLocked());
    ASSERT_THROW(deviceNew.asPtr<daq::IDevicePrivate>().unlock(nullptr), daq::AccessDeniedException);
    deviceNew.asPtr<daq::IDevicePrivate>().unlock(tomaz);
    ASSERT_FALSE(deviceNew.isLocked());
}

TEST_F(DeviceTest, ConnectionStatusContainer)
{
    const auto device = daq::createWithImplementation<daq::IDevice, TestDevice>();
    const auto connectionStatusContainer = device.getConnectionStatusContainer();

    ASSERT_TRUE(connectionStatusContainer.assigned());
}

TEST_F(DeviceTest, SerializeAndDeserializeWithConnectionStatuses)
{
    const auto context = daq::NullContext();
    const auto dev = daq::createWithImplementation<daq::IDevice, MockDevice>(context, nullptr, "dev");

    const auto typeManager = dev.getContext().getTypeManager();
    const auto statusValue = Enumeration("ConnectionStatusType", "Reconnecting", typeManager);

    auto connectionStatusContainer = dev.getConnectionStatusContainer().asPtr<daq::IConnectionStatusContainerPrivate>();
    auto mockStreaming1 = daq::MockStreaming("MockStreaming1", context);
    auto mockStreaming2 = daq::MockStreaming("MockStreaming2", context);
    connectionStatusContainer.addConfigurationConnectionStatus("ConfigConnStr", statusValue);
    connectionStatusContainer.addStreamingConnectionStatus("StreamingConnStr1", statusValue, mockStreaming1);
    connectionStatusContainer.addStreamingConnectionStatus("StreamingConnStr2", statusValue, mockStreaming2);

    connectionStatusContainer.updateConnectionStatusWithMessage("ConfigConnStr", statusValue, nullptr, "Config connection status message");
    connectionStatusContainer.updateConnectionStatusWithMessage("StreamingConnStr1", statusValue, mockStreaming1, "Streaming connection 1 status message");
    connectionStatusContainer.updateConnectionStatusWithMessage("StreamingConnStr2", statusValue, mockStreaming2, "Streaming connection 2 status message");

    const auto serializer = daq::JsonSerializer(daq::True);
    dev.serialize(serializer);
    const auto str1 = serializer.getOutput();

    const auto deserializer = daq::JsonDeserializer();

    const auto deserializeContext = daq::ComponentDeserializeContext(daq::NullContext(), nullptr, nullptr, "dev");

    const daq::DevicePtr deserializedDev = deserializer.deserialize(str1, deserializeContext, nullptr);

    // TODO streaming statuses are not serialized/deserialized
    // ASSERT_EQ(deserializedDev.getConnectionStatusContainer().getStatuses(),
    //           deserializedDev.getConnectionStatusContainer().getStatuses());
    // test config status only
    auto deserializedConnectionStatusContainer = deserializedDev.getConnectionStatusContainer();
    ASSERT_EQ(deserializedConnectionStatusContainer.getStatuses().getCount(), 1u);
    ASSERT_EQ(deserializedConnectionStatusContainer.getStatus("ConfigurationStatus"),
              dev.getConnectionStatusContainer().getStatus("ConfigurationStatus"));
    ASSERT_EQ(deserializedConnectionStatusContainer.getStatusMessage("ConfigurationStatus"),
              "Config connection status message");
    ASSERT_FALSE(deserializedConnectionStatusContainer.getStatuses().hasKey("StreamingStatus_1"));
    ASSERT_FALSE(deserializedConnectionStatusContainer.getStatuses().hasKey("StreamingStatus_2"));
}

TEST_F(DeviceTest, SerializeAndDeserializeManufacturer)
{
    const auto context = daq::NullContext();
    const auto dev = daq::createWithImplementation<daq::IDevice, MockDevice>(context, nullptr, "dev");
    
    const auto serializer = daq::JsonSerializer(daq::True);
    dev.serialize(serializer);
    const std::string str1 = serializer.getOutput();
    ASSERT_EQ(str1.find("manufacturer"), std::string::npos);
    ASSERT_EQ(str1.find("serialNumber"), std::string::npos);
}

TEST_F(DeviceTest, NetworkConfigEnabled)
{
    const auto device = daq::createWithImplementation<daq::IDevice, TestDevice>();
    auto deviceNetworkConfig = device.asPtr<daq::IDeviceNetworkConfig>();
    const auto msg = "Device must be set as root to manage network configuration.";

    ASSERT_TRUE(deviceNetworkConfig.getNetworkConfigurationEnabled());
    ASSERT_THROW_MSG(deviceNetworkConfig.submitNetworkConfiguration("eth0", daq::PropertyObject()), daq::InvalidStateException, msg);
    ASSERT_THROW_MSG(deviceNetworkConfig.retrieveNetworkConfiguration("eth0"), daq::InvalidStateException, msg);
    ASSERT_THROW_MSG(deviceNetworkConfig.getNetworkInterfaceNames(), daq::InvalidStateException, msg);

    device.asPtr<daq::IDevicePrivate>().setAsRoot();

    ASSERT_TRUE(deviceNetworkConfig.getNetworkConfigurationEnabled());
    ASSERT_NO_THROW(deviceNetworkConfig.submitNetworkConfiguration("eth0", daq::PropertyObject()));
    ASSERT_EQ(deviceNetworkConfig.retrieveNetworkConfiguration("eth0").getAllProperties().getCount(), 6u);
    ASSERT_EQ(deviceNetworkConfig.getNetworkInterfaceNames(), daq::List<daq::IString>("eth0"));
}

TEST_F(DeviceTest, NetworkConfigDisabled)
{
    const auto device = daq::createWithImplementation<daq::IDevice, MockDevice>(daq::NullContext(), nullptr, "dev");
    auto deviceNetworkConfig = device.asPtr<daq::IDeviceNetworkConfig>();

    device.asPtr<daq::IDevicePrivate>().setAsRoot();

    ASSERT_FALSE(deviceNetworkConfig.getNetworkConfigurationEnabled());
    ASSERT_THROW(deviceNetworkConfig.submitNetworkConfiguration("eth0", daq::PropertyObject()), daq::NotImplementedException);
    ASSERT_THROW(deviceNetworkConfig.retrieveNetworkConfiguration("eth0"), daq::NotImplementedException);
    ASSERT_THROW(deviceNetworkConfig.getNetworkInterfaceNames(), daq::NotImplementedException);
}

void checkDeviceOperationMode(const daq::DevicePtr& device, daq::OperationModeType expected)
{
    ASSERT_EQ(device.getOperationMode(), expected);
    bool active = expected != daq::OperationModeType::Idle;
    size_t count = 0;

    for (const auto& sig: device.getSignals())
    {
        ASSERT_EQ(sig.getActive(), active) << "Checking device signal " << sig.getGlobalId() << " for mode " << static_cast<daq::EnumType>(expected);
        count++;
    }

    ASSERT_GT(count, 0u) << "No signals found for device " << device.getGlobalId();
}

TEST_F(DeviceTest, DeviceSetOperationModeSanity)
{
    const auto device = daq::createWithImplementation<daq::IDevice, MockDevice>(daq::NullContext(), nullptr, "dev", true);
    const auto subDevice = device.getDevices()[0];
    device.asPtr<daq::IComponentPrivate>(true).updateOperationMode(daq::OperationModeType::Unknown);
    subDevice.asPtr<daq::IComponentPrivate>(true).updateOperationMode(daq::OperationModeType::Unknown);

    auto expectedDeviceModes = daq::List<daq::IInteger>(
        static_cast<daq::Int>(daq::OperationModeType::Idle),
        static_cast<daq::Int>(daq::OperationModeType::Operation),
        static_cast<daq::Int>(daq::OperationModeType::SafeOperation));

    ASSERT_EQ(device.getAvailableOperationModes(), expectedDeviceModes);
    ASSERT_EQ(subDevice.getAvailableOperationModes(), expectedDeviceModes);

    // compare with the default operation mode
    ASSERT_EQ(device.getAvailableOperationModes()[0], daq::OperationModeType::Idle);
    ASSERT_EQ(device.getAvailableOperationModes()[1], daq::OperationModeType::Operation);
    ASSERT_EQ(device.getAvailableOperationModes()[2], daq::OperationModeType::SafeOperation);
    
    // check getting operation mode from the list
    daq::OperationModeType mode = device.getAvailableOperationModes()[0];
    ASSERT_EQ(mode, daq::OperationModeType::Idle);

    auto mode2 = device.getAvailableOperationModes()[1];
    ASSERT_EQ(mode2, daq::OperationModeType::Operation);

    checkDeviceOperationMode(device, daq::OperationModeType::Operation);
    checkDeviceOperationMode(subDevice, daq::OperationModeType::Operation);

    ASSERT_NO_THROW(device.setOperationModeRecursive(daq::OperationModeType::Idle));
    checkDeviceOperationMode(device, daq::OperationModeType::Idle);
    checkDeviceOperationMode(subDevice, daq::OperationModeType::Idle);

    ASSERT_NO_THROW(device.setOperationMode(daq::OperationModeType::Operation));
    checkDeviceOperationMode(device, daq::OperationModeType::Operation);
    checkDeviceOperationMode(subDevice, daq::OperationModeType::Idle);

    ASSERT_NO_THROW(subDevice.setOperationModeRecursive(daq::OperationModeType::SafeOperation));
    checkDeviceOperationMode(device, daq::OperationModeType::Operation);
    checkDeviceOperationMode(subDevice, daq::OperationModeType::SafeOperation);

    ASSERT_NO_THROW(device.setOperationModeRecursive(daq::OperationModeType::Idle));
    checkDeviceOperationMode(device, daq::OperationModeType::Idle);
    checkDeviceOperationMode(subDevice, daq::OperationModeType::Idle);
}

TEST_F(DeviceTest, CheckNotSupportedOpMode)
{
    auto device = daq::createWithImplementation<daq::IDevice, TestDevice>();
    device.asPtr<daq::IComponentPrivate>(true).updateOperationMode(daq::OperationModeType::Unknown);

    auto expectedDeviceModes = daq::List<daq::IInteger>(
        static_cast<daq::Int>(daq::OperationModeType::Idle), 
        static_cast<daq::Int>(daq::OperationModeType::Operation));

    ASSERT_EQ(device.getAvailableOperationModes(), expectedDeviceModes);
    ASSERT_EQ(device.getOperationMode(), daq::OperationModeType::Operation);

    ASSERT_EQ(device->setOperationMode(daq::OperationModeType::SafeOperation), OPENDAQ_IGNORED);
    ASSERT_EQ(device.getOperationMode(), daq::OperationModeType::Operation);
}
