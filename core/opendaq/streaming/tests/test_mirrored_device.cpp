#include <gtest/gtest.h>
#include <opendaq/streaming_ptr.h>
#include <opendaq/mock/mock_streaming_factory.h>
#include <opendaq/context_factory.h>
#include <opendaq/mirrored_device_config_ptr.h>
#include <opendaq/mirrored_device_ptr.h>
#include <opendaq/mirrored_device_impl.h>

BEGIN_NAMESPACE_OPENDAQ

using namespace testing;

class MirroredDeviceTest : public testing::Test
{
public:
    MirroredDeviceTest()
        : context(NullContext())
        , device(createWithImplementation<IMirroredDeviceConfig, MirroredDevice>(context, nullptr, "TestDevice"))
    {}

    ContextPtr context;
    MirroredDeviceConfigPtr device;
};

TEST_F(MirroredDeviceTest, GetStreamingSources)
{
    ASSERT_EQ(device.getStreamingSources().getCount(), 0u);
}

TEST_F(MirroredDeviceTest, AddStreamingSource)
{
    auto streaming = MockStreaming("StreamingConnectionString", context);

    ASSERT_NO_THROW(device.addStreamingSource(streaming));

    const auto streamingSources = device.getStreamingSources();
    ASSERT_EQ(streamingSources.getCount(), 1u);

    ASSERT_EQ(streamingSources[0].getConnectionString(), "StreamingConnectionString");
    ASSERT_EQ(streamingSources[0], streaming);
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
}

END_NAMESPACE_OPENDAQ
