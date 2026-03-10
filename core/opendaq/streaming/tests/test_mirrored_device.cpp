#include <gtest/gtest.h>
#include <opendaq/streaming_ptr.h>
#include <opendaq/mock/mock_streaming_factory.h>
#include <opendaq/context_factory.h>
#include <opendaq/mirrored_device_config_ptr.h>
#include <opendaq/mirrored_device_ptr.h>
#include <opendaq/mirrored_device_impl.h>
#include <opendaq/gmock/streaming.h>
#include <opendaq/device_type_factory.h>
#include <opendaq/device_info_factory.h>
#include <opendaq/device_info_config_ptr.h>
#include <opendaq/component_deserialize_context_factory.h>
#include <opendaq/deserialize_component_ptr.h>
#include <coretypes/json_serializer_factory.h>
#include <coretypes/json_deserializer_factory.h>
#include <coretypes/function_factory.h>

BEGIN_NAMESPACE_OPENDAQ

using namespace testing;

class TestDeviceImpl : public MirroredDevice
{
public:
    TestDeviceImpl(const ContextPtr& ctx,
                   const ComponentPtr& parent,
                   const StringPtr& localId)
        : MirroredDevice(ctx, parent, localId)
    {}

protected:
    StringPtr onGetRemoteId() const override
    {
        return String("TestId");
    }
};

class TestDeviceWithInfoImpl : public MirroredDevice
{
public:
    TestDeviceWithInfoImpl(const ContextPtr& ctx,
                           const ComponentPtr& parent,
                           const StringPtr& localId)
        : MirroredDevice(ctx, parent, localId)
    {}

protected:
    StringPtr onGetRemoteId() const override
    {
        return String("TestId");
    }

    DeviceInfoPtr onGetInfo() override
    {
        return DeviceInfo("daq.test://TestDevice", "TestDevice");
    }
};

class MirroredDeviceTest : public testing::Test
{
public:
    MirroredDeviceTest()
        : context(NullContext())
        , device(createWithImplementation<IMirroredDeviceConfig, TestDeviceImpl>(context, nullptr, "TestDevice"))
    {}

    ContextPtr context;
    MirroredDeviceConfigPtr device;
};

TEST_F(MirroredDeviceTest, GetRemoteId)
{
    ASSERT_EQ(device.getRemoteId(), "TestId");
}

TEST_F(MirroredDeviceTest, GetStreamingSources)
{
    ASSERT_EQ(device.getStreamingSources().getCount(), 0u);
    ASSERT_EQ(device.getConnectionStatusContainer().getStatuses().getCount(), 0u);
}

TEST_F(MirroredDeviceTest, AddStreamingSource)
{
    auto streaming = MockStreaming("StreamingConnectionString", context);

    ASSERT_NO_THROW(device.addStreamingSource(streaming));

    const auto streamingSources = device.getStreamingSources();
    ASSERT_EQ(streamingSources.getCount(), 1u);

    ASSERT_EQ(streamingSources[0].getConnectionString(), "StreamingConnectionString");
    ASSERT_EQ(streamingSources[0], streaming);
    ASSERT_EQ(device.getConnectionStatusContainer().getStatuses().getCount(), 1u);
}

TEST_F(MirroredDeviceTest, DuplicateStreamingSource)
{
    auto streaming = MockStreaming("StreamingConnectionString", context);
    auto streamingDuplicate = MockStreaming("StreamingConnectionString", context);

    device.addStreamingSource(streaming);
    ASSERT_THROW(device.addStreamingSource(streaming), DuplicateItemException);
    ASSERT_THROW(device.addStreamingSource(streamingDuplicate), DuplicateItemException);
    ASSERT_THROW(device.addStreaming("StreamingConnectionString"), DuplicateItemException);

    const auto streamingSources = device.getStreamingSources();
    ASSERT_EQ(streamingSources.getCount(), 1u);
    ASSERT_EQ(device.getConnectionStatusContainer().getStatuses().getCount(), 1u);
}

TEST_F(MirroredDeviceTest, RemoveStreamingSourceNotFound)
{
    ASSERT_THROW(device.removeStreamingSource("NotFound"), NotFoundException);
}

TEST_F(MirroredDeviceTest, RemoveAddedStreamingSource)
{
    auto streaming = MockStreaming("StreamingConnectionString", context);
    device.addStreamingSource(streaming);

    ASSERT_NO_THROW(device.removeStreamingSource("StreamingConnectionString"));
    ASSERT_EQ(device.getStreamingSources().getCount(), 0u);
    ASSERT_EQ(device.getConnectionStatusContainer().getStatuses().getCount(), 0u);
}

TEST_F(MirroredDeviceTest, StreamingConnectionStatus)
{
    ASSERT_EQ(device.getConnectionStatusContainer().getStatuses().getCount(), 0u);

    auto streaming1 = MockStreaming::Strict("MockStreaming1", context);
    device.addStreamingSource(streaming1);
    ASSERT_EQ(device.getConnectionStatusContainer().getStatuses().getCount(), 1u);
    ASSERT_EQ(device.getConnectionStatusContainer().getStatus("StreamingStatus_1"), "Connected");

    streaming1.mock().triggerReconnectionStart();
    ASSERT_EQ(device.getConnectionStatusContainer().getStatus("StreamingStatus_1"), "Reconnecting");

    streaming1.mock().triggerReconnectionCompletion();
    ASSERT_EQ(device.getConnectionStatusContainer().getStatus("StreamingStatus_1"), "Connected");

    auto streaming2 = MockStreaming::Strict("MockStreaming2", context);
    streaming2.mock().triggerReconnectionStart();
    device.addStreamingSource(streaming2);
    ASSERT_EQ(device.getConnectionStatusContainer().getStatus("StreamingStatus_2"), "Reconnecting");

    device.removeStreamingSource("MockStreaming1");
    ASSERT_EQ(device.getConnectionStatusContainer().getStatuses().getCount(), 1u);
}

TEST_F(MirroredDeviceTest, GetMirroredDeviceTypeDefault)
{
    ASSERT_FALSE(device.getMirroredDeviceType().assigned());
}

TEST_F(MirroredDeviceTest, SetGetMirroredDeviceType)
{
    const auto deviceType = DeviceType("typeId", "typeName", "typeDescription", "daq.test");
    device.setMirroredDeviceType(deviceType);

    const auto retrieved = device.getMirroredDeviceType();
    ASSERT_TRUE(retrieved.assigned());
    ASSERT_EQ(retrieved.getId(), "typeId");
    ASSERT_EQ(retrieved.getName(), "typeName");
    ASSERT_EQ(retrieved.getDescription(), "typeDescription");
}

TEST_F(MirroredDeviceTest, SetMirroredDeviceTypePropagatesDeviceType)
{
    const auto ctx = NullContext();
    const MirroredDeviceConfigPtr deviceWithInfo =
        createWithImplementation<IMirroredDeviceConfig, TestDeviceWithInfoImpl>(ctx, nullptr, "dev");

    // Force deviceInfo initialization
    deviceWithInfo.asPtr<IDevice>().getInfo();

    const auto deviceType = DeviceType("typeId", "typeName", "typeDescription", "daq.test");
    deviceWithInfo.setMirroredDeviceType(deviceType);

    const auto info = deviceWithInfo.asPtr<IDevice>().getInfo();
    ASSERT_TRUE(info.assigned());
    const auto deviceTypeFromInfo = info.getDeviceType();
    ASSERT_TRUE(deviceTypeFromInfo.assigned());
    ASSERT_EQ(deviceTypeFromInfo.getId(), "typeId");
    ASSERT_EQ(deviceTypeFromInfo.getName(), "typeName");
}

END_NAMESPACE_OPENDAQ
