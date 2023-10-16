#include <opendaq/device_impl.h>
#include <opendaq/device_ptr.h>
#include <opendaq/device_info_factory.h>
#include <opendaq/context_factory.h>
#include <opendaq/folder_factory.h>
#include <opendaq/io_folder_factory.h>
#include <opendaq/gmock/function_block.h>
#include <gtest/gtest.h>
#include <opendaq/device_private.h>
#include <opendaq/device_type_factory.h>

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
        auto devInfo = daq::DeviceInfo("conn");
        return devInfo;
    }
};

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
    EXPECT_CALL(ch.mock(), getLocalId(testing::_)).WillOnce(daq::Get{daq::String("ch")});
    ASSERT_NO_THROW(ioFolder.addItem(*ch));
}

TEST_F(DeviceTest, CustomComponentSubItems)
{
    const auto device = daq::createWithImplementation<daq::IDevice, TestDevice>();
    const auto customComponents = device.getCustomComponents();

    ASSERT_EQ(customComponents.getCount(), 2);

    daq::FolderPtr folder1 = customComponents[0];
    ASSERT_EQ(folder1.getItems().getCount(), 1);
    ASSERT_FALSE(customComponents[1].asPtrOrNull<daq::IFolder>().assigned());
}

TEST_F(DeviceTest, StreamingOptions)
{
    auto device = daq::createWithImplementation<daq::IDevice, TestDevice>();
    auto devicePrivatePtr = device.asPtr<daq::IDevicePrivate>();
    daq::ListPtr<daq::IStreamingInfo> streamingOptions;

    ASSERT_EQ(devicePrivatePtr->removeStreamingOption(nullptr), OPENDAQ_ERR_ARGUMENT_NULL);
    ASSERT_EQ(devicePrivatePtr->addStreamingOption(nullptr), OPENDAQ_ERR_ARGUMENT_NULL);
    ASSERT_EQ(devicePrivatePtr->removeStreamingOption(daq::String("protocol")), OPENDAQ_ERR_NOTFOUND);
    ASSERT_EQ(devicePrivatePtr->getStreamingOptions(nullptr), OPENDAQ_ERR_ARGUMENT_NULL);

    ASSERT_EQ(devicePrivatePtr->getStreamingOptions(&streamingOptions), OPENDAQ_SUCCESS);
    ASSERT_EQ(streamingOptions.getCount(), 0u);


    auto streamingInfo = daq::StreamingInfo("protocol");
    ASSERT_EQ(devicePrivatePtr->addStreamingOption(streamingInfo), OPENDAQ_SUCCESS);
    ASSERT_EQ(devicePrivatePtr->addStreamingOption(streamingInfo), OPENDAQ_ERR_DUPLICATEITEM);

    ASSERT_EQ(devicePrivatePtr->getStreamingOptions(&streamingOptions), OPENDAQ_SUCCESS);
    ASSERT_EQ(streamingOptions.getCount(), 1u);

    auto streamingInfo2 = daq::StreamingInfo("protocol2");
    ASSERT_EQ(devicePrivatePtr->addStreamingOption(streamingInfo2), OPENDAQ_SUCCESS);
    ASSERT_EQ(devicePrivatePtr->getStreamingOptions(&streamingOptions), OPENDAQ_SUCCESS);
    ASSERT_EQ(streamingOptions.getCount(), 2u);

    ASSERT_EQ(devicePrivatePtr->removeStreamingOption(daq::String("protocol")), OPENDAQ_SUCCESS);
    ASSERT_EQ(devicePrivatePtr->getStreamingOptions(&streamingOptions), OPENDAQ_SUCCESS);
    ASSERT_EQ(streamingOptions.getCount(), 1u);
}

TEST_F(DeviceTest, DefaultProperties)
{
    auto device = daq::createWithImplementation<daq::IDevice, TestDevice>();
    ASSERT_EQ(device.getPropertyValue("Location"), "");
    ASSERT_EQ(device.getPropertyValue("UserName"), "");
}

TEST_F(DeviceTest, DeviceTypeStructType)
{
    const auto structType = daq::DeviceTypeStructType();
    const daq::StructPtr structPtr = daq::DeviceType("id", "name", "desc");
    ASSERT_EQ(structType, structPtr.getStructType());
}

TEST_F(DeviceTest, DeviceTypeStructFields)
{
    const daq::StructPtr structPtr = daq::DeviceType("id", "name", "desc");
    ASSERT_EQ(structPtr.get("id"), "id");
    ASSERT_EQ(structPtr.get("name"), "name");
    ASSERT_EQ(structPtr.get("description"), "desc");
}

TEST_F(DeviceTest, DeviceTypeStructNames)
{
    const auto structType = daq::DeviceTypeStructType();
    const daq::StructPtr structPtr = daq::DeviceType("id", "name", "desc");
    ASSERT_EQ(structType.getFieldNames(), structPtr.getFieldNames());
}
