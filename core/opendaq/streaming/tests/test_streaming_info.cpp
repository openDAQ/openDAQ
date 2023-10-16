#include <opendaq/streaming_info_factory.h>
#include <gtest/gtest.h>
#include <coreobjects/property_factory.h>
#include <opendaq/context_factory.h>

using StreamingInfoTest = testing::Test;

BEGIN_NAMESPACE_OPENDAQ

TEST_F(StreamingInfoTest, Factory)
{
    StreamingInfoPtr streamingInfo;
    ASSERT_NO_THROW(streamingInfo = StreamingInfo("protocol"));
    ASSERT_EQ(streamingInfo.getProtocolId(), "protocol");
    ASSERT_EQ(streamingInfo.getPrimaryAddress(), "");
}

TEST_F(StreamingInfoTest, DefaultValues)
{
    StreamingInfoPtr streamingInfo = StreamingInfo("protocol");

    ASSERT_EQ(streamingInfo.getProtocolId(), "protocol");
    ASSERT_EQ(streamingInfo.getPrimaryAddress(), "");

    ASSERT_EQ(streamingInfo.getAllProperties().getCount(), 2u);
}

TEST_F(StreamingInfoTest, SetGetProperties)
{
    StreamingInfoConfigPtr streamingInfoConfig = StreamingInfo("protocol");
    StreamingInfoPtr streamingInfo = streamingInfoConfig;

    ASSERT_EQ(streamingInfo.getProtocolId(), "protocol");
    ASSERT_EQ(streamingInfo.getPropertyValue("protocolId"), "protocol");
    ASSERT_ANY_THROW(streamingInfoConfig.setPropertyValue("protocolId", "value"));

    ASSERT_NO_THROW(streamingInfoConfig.setPrimaryAddress("address"));
    ASSERT_EQ(streamingInfo.getPrimaryAddress(), "address");
    ASSERT_EQ(streamingInfo.getPropertyValue("address"), "address");

    streamingInfoConfig.addProperty(StringProperty("stringKey", ""));
    ASSERT_TRUE(streamingInfo.hasProperty("stringKey"));
    streamingInfoConfig.setPropertyValue("stringKey", "testValue");
    ASSERT_EQ(streamingInfo.getPropertyValue("stringKey"), "testValue");

    streamingInfoConfig.addProperty(IntProperty("intKey", 0));
    ASSERT_TRUE(streamingInfo.hasProperty("intKey"));
    streamingInfoConfig.setPropertyValue("intKey", 123);
    ASSERT_EQ(streamingInfo.getPropertyValue("intKey"), 123);
}

END_NAMESPACE_OPENDAQ
