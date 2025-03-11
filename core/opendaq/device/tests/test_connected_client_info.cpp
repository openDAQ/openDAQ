#include <gtest/gtest.h>
#include <opendaq/device_info_factory.h>
#include <coreobjects/property_object_factory.h>
#include <coreobjects/property_factory.h>

using ConnectedClientInfoTest = testing::Test;

BEGIN_NAMESPACE_OPENDAQ

TEST_F(ConnectedClientInfoTest, Factory)
{
    ConnectedClientInfoPtr clientInfo =
        ConnectedClientInfo("url", ProtocolType::Configuration, "Protocol name", ClientType::ExclusiveControl, "Host name");
    ASSERT_EQ(clientInfo.getUrl(), "url");
    ASSERT_EQ(clientInfo.getProtocolName(), "Protocol name");
    ASSERT_EQ(clientInfo.getClientType(), ClientType::ExclusiveControl);
    ASSERT_EQ(clientInfo.getProtocolType(), ProtocolType::Configuration);
    ASSERT_EQ(clientInfo.getHostName(), "Host name");
}

TEST_F(ConnectedClientInfoTest, CustomProperties)
{
    ConnectedClientInfoPtr clientInfo =
        ConnectedClientInfo("url", ProtocolType::ConfigurationAndStreaming, "Protocol name", ClientType::Control, "Host name");
    SizeT defaultPropertiesCnt = clientInfo.getAllProperties().getCount();

    clientInfo.addProperty(StringProperty("Location", "Office"));
    ASSERT_EQ(clientInfo.getPropertyValue("Location"), "Office");

    ASSERT_NO_THROW(clientInfo.addProperty(IntProperty("ElapsedTime", 999)));
    ASSERT_NO_THROW(clientInfo.addProperty(FloatProperty("SomeMetric", 172.4)));
    ASSERT_NO_THROW(clientInfo.addProperty(BoolProperty("IsActive", true)));

    ASSERT_EQ(clientInfo.getAllProperties().getCount(), 4u + defaultPropertiesCnt);
}

TEST_F(ConnectedClientInfoTest, Freezable)
{
    ConnectedClientInfoPtr clientInfo =
        ConnectedClientInfo("url", ProtocolType::Streaming, "Protocol name", ClientType(), "Host name");

    ASSERT_FALSE(clientInfo.isFrozen());
    ASSERT_NO_THROW(clientInfo.freeze());
    ASSERT_TRUE(clientInfo.isFrozen());

    ASSERT_THROW(clientInfo.addProperty(StringProperty("test_key", "test_value")), FrozenException);
}

END_NAMESPACE_OPENDAQ
