#include <opendaq/device_impl.h>
#include <opendaq/device_ptr.h>
#include <opendaq/device_info_factory.h>
#include <opendaq/context_factory.h>
#include <opendaq/folder_factory.h>
#include <opendaq/io_folder_factory.h>
#include <opendaq/io_folder_impl.h>
#include <opendaq/gmock/function_block.h>
#include <gtest/gtest.h>
#include <opendaq/device_private.h>
#include <opendaq/device_type_factory.h>
#include <opendaq/function_block_impl.h>
#include <opendaq/component_deserialize_context_factory.h>
#include <opendaq/channel_impl.h>
#include <opendaq/component_exceptions.h>

using DeviceTest = testing::Test;

class TestDevice : public daq::Device
{
public:
    TestDevice()
        : daq::Device(daq::NullContext(), nullptr, "dev")
    {
        auto parentFolder = this->addFolder("Folder1");
        this->addFolder("Folder2", parentFolder);
        this->addComponent("Component1");
    }

    daq::DeviceInfoPtr onGetInfo() override
    {
        auto deviceInfo = daq::DeviceInfo("conn");
        deviceInfo.setName("test");
        deviceInfo.setLocation("test");
        deviceInfo.freeze();
        return deviceInfo;
    }
};

TEST_F(DeviceTest, DeviceInfoNameLocationSync)
{
    auto device = daq::createWithImplementation<daq::IDevice, TestDevice>();
    auto info = device.getInfo();

    ASSERT_EQ(info.getLocation(), "");
    ASSERT_EQ(info.getName(), "dev");

    device.setPropertyValue("location", "new_loc");
    device.setName("new_name");

    ASSERT_EQ(info.getLocation(), "new_loc");
    ASSERT_EQ(info.getName(), "new_name");
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
    ASSERT_EQ(device.getPropertyValue("location"), "");
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
    MockDevice(const daq::ContextPtr& ctx, const daq::ComponentPtr& parent, const daq::StringPtr& localId)
        : daq::Device(ctx, parent, localId)
    {
        createAndAddSignal("sig_device");

        auto aiIoFolder = this->addIoFolder("AI", ioFolder);
        createAndAddChannel<MockChannel>(aiIoFolder, "Ch");

        const auto fb = daq::createWithImplementation<daq::IFunctionBlock, MockFbImpl>(ctx, this->functionBlocks, "fb");
        addNestedFunctionBlock(fb);
    }
};

TEST_F(DeviceTest, SerializeAndDeserialize)
{
    const auto dev = daq::createWithImplementation<daq::IDevice, MockDevice>(daq::NullContext(), nullptr, "dev");

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
