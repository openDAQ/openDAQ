#include <gtest/gtest.h>
#include <opendaq/device_info_factory.h>
#include <coreobjects/property_object_factory.h>
#include <coreobjects/property_factory.h>

using ConnectedClientInfoTest = testing::Test;

BEGIN_NAMESPACE_OPENDAQ

TEST_F(ConnectedClientInfoTest, Factory)
{
    ConnectedClientInfoPtr clientInfo =
        ConnectedClientInfo("url",
                            ProtocolType::Configuration,
                            "Protocol name",
                            "ExclusiveControl",
                            "Host name");
    ASSERT_EQ(clientInfo.getUrl(), "url");
    ASSERT_EQ(clientInfo.getProtocolName(), "Protocol name");
    ASSERT_EQ(clientInfo.getClientTypeName(), "ExclusiveControl");
    ASSERT_EQ(clientInfo.getProtocolType(), ProtocolType::Configuration);
    ASSERT_EQ(clientInfo.getHostName(), "Host name");
}

TEST_F(ConnectedClientInfoTest, CustomProperties)
{
    ConnectedClientInfoPtr clientInfo =
        ConnectedClientInfo("url",
                            ProtocolType::ConfigurationAndStreaming,
                            "Protocol name",
                            "",
                            "Host name");
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
        ConnectedClientInfo("url", ProtocolType::Streaming, "Protocol name", "", "Host name");

    ASSERT_FALSE(clientInfo.isFrozen());
    ASSERT_NO_THROW(clientInfo.freeze());
    ASSERT_TRUE(clientInfo.isFrozen());

    ASSERT_THROW(clientInfo.addProperty(StringProperty("test_key", "test_value")), FrozenException);
}

TEST_F(ConnectedClientInfoTest, SerializeDeserialize)
{
    ConnectedClientInfoPtr clientInfo =
        ConnectedClientInfo("url",
                            ProtocolType::Configuration,
                            "Protocol name",
                            "ExclusiveControl",
                            "Host name");

    clientInfo.addProperty(StringProperty("Location", "Office"));
    clientInfo.addProperty(IntProperty("ElapsedTime", 999));
    clientInfo.addProperty(FloatProperty("SomeMetric", 172.4));
    clientInfo.addProperty(BoolProperty("IsActive", true));

    const auto serializer = JsonSerializer();
    clientInfo.serialize(serializer);
    const auto serializedClientInfo = serializer.getOutput();

    const auto deserializer = JsonDeserializer();

    const ConnectedClientInfoPtr newClientInfo = deserializer.deserialize(serializedClientInfo, nullptr, nullptr);

    ASSERT_EQ(newClientInfo.getUrl(), "url");
    ASSERT_EQ(newClientInfo.getProtocolName(), "Protocol name");
    ASSERT_EQ(newClientInfo.getClientTypeName(), "ExclusiveControl");
    ASSERT_EQ(newClientInfo.getProtocolType(), ProtocolType::Configuration);
    ASSERT_EQ(newClientInfo.getHostName(), "Host name");

    serializer.reset();
    newClientInfo.serialize(serializer);
    const auto newSerializedClientInfo = serializer.getOutput();

    ASSERT_EQ(serializedClientInfo, newSerializedClientInfo);
}

END_NAMESPACE_OPENDAQ
