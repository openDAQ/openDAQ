#include <opendaq/device_info_factory.h>
#include <opendaq/context_factory.h>
#include <testutils/testutils.h>
#include <coreobjects/property_object_factory.h>
#include <coreobjects/property_factory.h>
#include <coreobjects/property_object_internal_ptr.h>
#include <coretypes/json_serializer_factory.h>
#include <coretypes/json_deserializer_factory.h>
#include <opendaq/device_type_factory.h>

using ServerCapabilityTest = testing::Test;

BEGIN_NAMESPACE_OPENDAQ

TEST_F(ServerCapabilityTest, Factory)
{
    ServerCapabilityPtr capability = ServerCapability("protocol_id", "Protocol name", ProtocolType::Streaming);
    ASSERT_EQ(capability.getProtocolName(), "Protocol name");
    ASSERT_EQ(capability.getProtocolType(), ProtocolType::Streaming);
    ASSERT_EQ(capability.getConnectionString(), "");
    ASSERT_EQ(capability.getConnectionType(), "Unknown");
    ASSERT_EQ(capability.getCoreEventsEnabled(), false);
    ASSERT_EQ(capability.getProtocolGroupId(), "");
    ASSERT_EQ(capability.getProtocolSecurityLevel(), -1);
}

TEST_F(ServerCapabilityTest, SetGet)
{
    ServerCapabilityPtr capability = ServerCapability("protocol_id", "Protocol name", ProtocolType::Streaming).addConnectionString("connection string")
                                                                                                  .setConnectionType("connection type")
                                                                                                  .setCoreEventsEnabled(true)
                                                                                                  .setProtocolGroupId("protocol_group_id")
                                                                                                  .setProtocolSecurityLevel(2);
    ASSERT_EQ(capability.getProtocolName(), "Protocol name");
    ASSERT_EQ(capability.getProtocolType(), ProtocolType::Streaming);
    ASSERT_EQ(capability.getConnectionString(), "connection string");
    ASSERT_EQ(capability.getConnectionType(), "connection type");
    ASSERT_EQ(capability.getCoreEventsEnabled(), true);
    ASSERT_EQ(capability.getProtocolGroupId(), "protocol_group_id");
    ASSERT_EQ(capability.getProtocolSecurityLevel(), 2);
}

TEST_F(ServerCapabilityTest, SetGetEmptyProtocolGroupId)
{
    // an empty group id means the protocol is not part of any group
    ServerCapabilityConfigPtr capability = ServerCapability("protocol_id", "Protocol name", ProtocolType::Streaming);

    ASSERT_NO_THROW(capability.setProtocolGroupId(""));
    ASSERT_EQ(capability.getProtocolGroupId(), "");
}

TEST_F(ServerCapabilityTest, SetGetNegativeSecurityLevel)
{
    ServerCapabilityConfigPtr capability = ServerCapability("protocol_id", "Protocol name", ProtocolType::Streaming);

    ASSERT_NO_THROW(capability.setProtocolSecurityLevel(0));
    ASSERT_EQ(capability.getProtocolSecurityLevel(), 0);

    ASSERT_NO_THROW(capability.setProtocolSecurityLevel(-1));
    ASSERT_EQ(capability.getProtocolSecurityLevel(), -1);
}

TEST_F(ServerCapabilityTest, Freezable)
{
    ServerCapabilityConfigPtr capability = ServerCapability("protocol_id", "Protocol name", ProtocolType::Streaming);

    ASSERT_FALSE(capability.isFrozen());
    ASSERT_NO_THROW(capability.freeze());
    ASSERT_TRUE(capability.isFrozen());

    ASSERT_THROW(capability.addConnectionString("String"), FrozenException);
    ASSERT_THROW(capability.setConnectionType("String"), FrozenException);
    ASSERT_THROW(capability.setCoreEventsEnabled(false), FrozenException);
    ASSERT_THROW(capability.setProtocolName("String"), FrozenException);
    ASSERT_THROW(capability.setProtocolGroupId("String"), FrozenException);
    ASSERT_THROW(capability.setProtocolSecurityLevel(1), FrozenException);

    ASSERT_THROW(capability.addProperty(StringProperty("test_key", "test_value")), FrozenException);
}

TEST_F(ServerCapabilityTest, CustomProperties)
{
   ServerCapabilityConfigPtr capability = ServerCapability("protocol_id", "Protocol name", ProtocolType::Streaming);
   SizeT defaultPropertiesCnt = capability.getAllProperties().getCount();

    capability.addProperty(StringProperty("Name", "Chell"));
    ASSERT_EQ(capability.getPropertyValue("Name"), "Chell");

    ASSERT_NO_THROW(capability.addProperty(IntProperty("Age", 999)));
    ASSERT_NO_THROW(capability.addProperty(FloatProperty("Height", 172.4)));
    ASSERT_NO_THROW(capability.addProperty(BoolProperty("IsAsleep", true)));

    ASSERT_EQ(capability.getAllProperties().getCount(), 4u + defaultPropertiesCnt);
}

TEST_F(ServerCapabilityTest, SerializeDeserialize)
{
    ServerCapabilityConfigPtr capability = ServerCapability("protocol_id", "Protocol name", ProtocolType::Streaming)
                                               .addConnectionString("connection string")
                                               .setConnectionType("connection type")
                                               .setProtocolGroupId("protocol_group_id")
                                               .setProtocolSecurityLevel(3);

    const auto serializer = JsonSerializer();
    capability.serialize(serializer);
    const auto serializedCapability = serializer.getOutput();

    const auto deserializer = JsonDeserializer();
    const ServerCapabilityPtr newCapability = deserializer.deserialize(serializedCapability, nullptr, nullptr);

    ASSERT_EQ(newCapability.getProtocolId(), "protocol_id");
    ASSERT_EQ(newCapability.getProtocolName(), "Protocol name");
    ASSERT_EQ(newCapability.getProtocolType(), ProtocolType::Streaming);
    ASSERT_EQ(newCapability.getProtocolGroupId(), "protocol_group_id");
    ASSERT_EQ(newCapability.getProtocolSecurityLevel(), 3);

    serializer.reset();
    newCapability.serialize(serializer);
    ASSERT_EQ(serializedCapability, serializer.getOutput());
}

TEST_F(ServerCapabilityTest, Clone)
{
    ServerCapabilityConfigPtr capability = ServerCapability("protocol_id", "Protocol name", ProtocolType::Streaming)
                                               .addConnectionString("connection string")
                                               .setProtocolGroupId("protocol_group_id")
                                               .setProtocolSecurityLevel(3);

    const ServerCapabilityPtr cloned = capability.asPtr<IPropertyObjectInternal>().clone();

    ASSERT_EQ(cloned.getProtocolId(), "protocol_id");
    ASSERT_EQ(cloned.getProtocolName(), "Protocol name");
    ASSERT_EQ(cloned.getProtocolType(), ProtocolType::Streaming);
    ASSERT_EQ(cloned.getConnectionString(), "connection string");
    ASSERT_EQ(cloned.getProtocolGroupId(), "protocol_group_id");
    ASSERT_EQ(cloned.getProtocolSecurityLevel(), 3);
}

END_NAMESPACE_OPENDAQ
