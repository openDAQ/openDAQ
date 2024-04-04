#include <opendaq/device_info_factory.h>
#include <opendaq/context_factory.h>
#include <gtest/gtest.h>
#include <coreobjects/property_object_factory.h>
#include <coreobjects/property_factory.h>
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
}

TEST_F(ServerCapabilityTest, SetGet)
{
    ServerCapabilityPtr capability = ServerCapability("protocol_id", "Protocol name", ProtocolType::Streaming).addConnectionString("connection string")
                                                                                                  .setConnectionType("connection type")
                                                                                                  .setCoreEventsEnabled(true);
    ASSERT_EQ(capability.getProtocolName(), "Protocol name");
    ASSERT_EQ(capability.getProtocolType(), ProtocolType::Streaming);
    ASSERT_EQ(capability.getConnectionString(), "connection string");
    ASSERT_EQ(capability.getConnectionType(), "connection type");
    ASSERT_EQ(capability.getCoreEventsEnabled(), true);
}

TEST_F(ServerCapabilityTest, Freezable)
{
    ServerCapabilityConfigPtr capability = ServerCapability("protocol_id", "Protocol name", ProtocolType::Streaming);

    ASSERT_FALSE(capability.isFrozen());
    ASSERT_NO_THROW(capability.freeze());
    ASSERT_TRUE(capability.isFrozen());

    ASSERT_THROW(capability.addConnectionString("string"), FrozenException);
    ASSERT_THROW(capability.setConnectionType("string"), FrozenException);
    ASSERT_THROW(capability.setCoreEventsEnabled(false), FrozenException);
    ASSERT_THROW(capability.setProtocolName("string"), FrozenException);
    
    ASSERT_THROW(capability.addProperty(StringProperty("test_key", "test_value")), FrozenException);
}

TEST_F(ServerCapabilityTest, CustomProperties)
{
   ServerCapabilityConfigPtr capability = ServerCapability("protocol_id", "Protocol name", ProtocolType::Streaming);
   SizeT defaultProportiesCnt = capability.getAllProperties().getCount();

    capability.addProperty(StringProperty("Name", "Chell"));
    ASSERT_EQ(capability.getPropertyValue("Name"), "Chell");
    ASSERT_EQ(capability.getPropertyValue("Name"), "Chell");

    ASSERT_NO_THROW(capability.addProperty(IntProperty("Age", 999)));
    ASSERT_NO_THROW(capability.addProperty(FloatProperty("Height", 172.4)));
    ASSERT_NO_THROW(capability.addProperty(BoolProperty("IsAsleep", true)));

    ASSERT_EQ(capability.getAllProperties().getCount(), 4u + defaultProportiesCnt);
}

END_NAMESPACE_OPENDAQ
