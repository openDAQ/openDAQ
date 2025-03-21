#include <gtest/gtest.h>
#include <opendaq/device_info_factory.h>
#include <coreobjects/property_object_factory.h>
#include <coreobjects/property_factory.h>

using ConnectedClientInfoTest = testing::Test;

BEGIN_NAMESPACE_OPENDAQ

TEST_F(ConnectedClientInfoTest, FactoryNoParams)
{
    ConnectedClientInfoPtr clientInfo = ConnectedClientInfo();
    ASSERT_EQ(clientInfo.getAddress(), "");
    ASSERT_EQ(clientInfo.getProtocolName(), "");
    ASSERT_EQ(clientInfo.getClientTypeName(), "");
    ASSERT_EQ(clientInfo.getProtocolType(), ProtocolType::Unknown);
    ASSERT_EQ(clientInfo.getHostName(), "");
}

TEST_F(ConnectedClientInfoTest, FactoryWithParams)
{
    ConnectedClientInfoPtr clientInfo =
        ConnectedClientInfo("url",
                            ProtocolType::Configuration,
                            "Protocol name",
                            "ExclusiveControl",
                            "Host name");
    ASSERT_EQ(clientInfo.getAddress(), "url");
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

    ASSERT_ANY_THROW(clientInfo.addProperty(IntProperty("ElapsedTime", 999)));
    ASSERT_ANY_THROW(clientInfo.addProperty(FloatProperty("SomeMetric", 172.4)));
    ASSERT_ANY_THROW(clientInfo.addProperty(BoolProperty("IsActive", true)));

    ASSERT_EQ(clientInfo.getAllProperties().getCount(), 1u + defaultPropertiesCnt);
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

    const auto serializer = JsonSerializer();
    clientInfo.serialize(serializer);
    const auto serializedClientInfo = serializer.getOutput();

    const auto deserializer = JsonDeserializer();

    const ConnectedClientInfoPtr newClientInfo = deserializer.deserialize(serializedClientInfo, nullptr, nullptr);

    ASSERT_EQ(newClientInfo.getAddress(), "url");
    ASSERT_EQ(newClientInfo.getProtocolName(), "Protocol name");
    ASSERT_EQ(newClientInfo.getClientTypeName(), "ExclusiveControl");
    ASSERT_EQ(newClientInfo.getProtocolType(), ProtocolType::Configuration);
    ASSERT_EQ(newClientInfo.getHostName(), "Host name");

    serializer.reset();
    newClientInfo.serialize(serializer);
    const auto newSerializedClientInfo = serializer.getOutput();

    ASSERT_EQ(serializedClientInfo, newSerializedClientInfo);
}

TEST_F(ConnectedClientInfoTest, SerializeForOlderVersion)
{
    ConnectedClientInfoPtr clientInfo = ConnectedClientInfo();

    clientInfo.addProperty(StringProperty("Location", "Office"));

    const auto serializer = JsonSerializerWithVersion(2);
    clientInfo.serialize(serializer);
    const auto serializedClientInfo = serializer.getOutput();

    const auto deserializer = JsonDeserializer();

    const BaseObjectPtr newClientInfo = deserializer.deserialize(serializedClientInfo, nullptr, nullptr);
    ASSERT_FALSE(newClientInfo.supportsInterface<IConnectedClientInfo>());
    ASSERT_TRUE(newClientInfo.supportsInterface<IPropertyObject>());
    ASSERT_EQ(newClientInfo.asPtr<IPropertyObject>().getAllProperties().getCount(), 0u);
}

END_NAMESPACE_OPENDAQ
