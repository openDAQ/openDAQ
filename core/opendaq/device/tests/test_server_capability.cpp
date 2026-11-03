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

TEST_F(ServerCapabilityTest, AvailableTransportsDefault)
{
    ServerCapabilityPtr capability = ServerCapability("protocol_id", "Protocol name", ProtocolType::Streaming);

    ASSERT_EQ(capability.getAvailableTransports().getCount(), 0u);
}

TEST_F(ServerCapabilityTest, SetGetAvailableTransports)
{
    ServerCapabilityConfigPtr capability = ServerCapability("protocol_id", "Protocol name", ProtocolType::Streaming)
                                               .setAvailableTransports(Dict<IString, IInteger>({{"wss", 7443}, {"tls", 7421}, {"plain", 7420}}));

    const auto transports = capability.getAvailableTransports();
    ASSERT_EQ(transports.getKeyList(), List<IString>("wss", "tls", "plain"));
    ASSERT_EQ(transports.get("wss"), 7443);
    ASSERT_EQ(transports.get("tls"), 7421);
    ASSERT_EQ(transports.get("plain"), 7420);
}

TEST_F(ServerCapabilityTest, SetAvailableTransportsReplaces)
{
    ServerCapabilityConfigPtr capability = ServerCapability("protocol_id", "Protocol name", ProtocolType::Streaming);

    capability.setAvailableTransports(Dict<IString, IInteger>({{"plain", 7420}, {"tls", 7421}}));
    capability.setAvailableTransports(Dict<IString, IInteger>({{"https", 443}}));

    ASSERT_EQ(capability.getAvailableTransports().getKeyList(), List<IString>("https"));
}

TEST_F(ServerCapabilityTest, SetAvailableTransportsInvalid)
{
    ServerCapabilityConfigPtr capability = ServerCapability("protocol_id", "Protocol name", ProtocolType::Streaming);

    ASSERT_THROW(capability.setAvailableTransports(nullptr), ArgumentNullException);
    ASSERT_THROW(capability.setAvailableTransports(Dict<IString, IString>({{"plain", "7420"}})), InvalidTypeException);
    ASSERT_THROW(capability.setAvailableTransports(Dict<IInteger, IInteger>({{1, 7420}})), InvalidTypeException);
    ASSERT_EQ(capability.getAvailableTransports().getCount(), 0u);
}

TEST_F(ServerCapabilityTest, SetAvailableTransportsFrozen)
{
    ServerCapabilityConfigPtr capability = ServerCapability("protocol_id", "Protocol name", ProtocolType::Streaming);
    capability.freeze();

    ASSERT_THROW(capability.setAvailableTransports(Dict<IString, IInteger>({{"plain", 7420}})), FrozenException);
}

TEST_F(ServerCapabilityTest, AvailableTransportsAreCopied)
{
    auto transports = Dict<IString, IInteger>({{"plain", 7420}});
    ServerCapabilityConfigPtr capability = ServerCapability("protocol_id", "Protocol name", ProtocolType::Streaming)
                                               .setAvailableTransports(transports);

    transports.set("tls", 7421);
    capability.getAvailableTransports().set("wss", 7443);

    ASSERT_EQ(capability.getAvailableTransports().getKeyList(), List<IString>("plain"));
}

TEST_F(ServerCapabilityTest, SerializeDeserializeAvailableTransports)
{
    ServerCapabilityConfigPtr capability = ServerCapability("protocol_id", "Protocol name", ProtocolType::Streaming)
                                               .setAvailableTransports(Dict<IString, IInteger>({{"wss", 7443}, {"tls", 7421}, {"plain", 7420}}));

    const auto serializer = JsonSerializer();
    capability.serialize(serializer);
    const auto serializedCapability = serializer.getOutput();

    const auto deserializer = JsonDeserializer();
    const ServerCapabilityPtr newCapability = deserializer.deserialize(serializedCapability, nullptr, nullptr);

    const auto transports = newCapability.getAvailableTransports();
    ASSERT_EQ(transports.getKeyList(), List<IString>("wss", "tls", "plain"));
    ASSERT_EQ(transports.get("tls"), 7421);
    ASSERT_EQ(newCapability.getPort(), 7443);
    // What an older client reads
    ASSERT_EQ(newCapability.getPropertyValue("Port"), 7443);

    serializer.reset();
    newCapability.serialize(serializer);
    ASSERT_EQ(serializedCapability, serializer.getOutput());
}

// A capability serialized by an older SDK has no available transports property
TEST_F(ServerCapabilityTest, DeserializeWithoutAvailableTransports)
{
    ServerCapabilityConfigPtr capability = ServerCapability("protocol_id", "Protocol name", ProtocolType::Streaming).setPort(7420);
    capability.removeProperty("AvailableTransports");

    const auto serializer = JsonSerializer();
    capability.serialize(serializer);
    const auto serializedCapability = serializer.getOutput();
    ASSERT_EQ(std::string(serializedCapability).find("AvailableTransports"), std::string::npos);

    const auto deserializer = JsonDeserializer();
    const ServerCapabilityPtr newCapability = deserializer.deserialize(serializedCapability, nullptr, nullptr);

    ASSERT_EQ(newCapability.getAvailableTransports().getCount(), 0u);
    ASSERT_EQ(newCapability.getPort(), 7420);
}

TEST_F(ServerCapabilityTest, CloneAvailableTransports)
{
    ServerCapabilityConfigPtr capability = ServerCapability("protocol_id", "Protocol name", ProtocolType::Streaming)
                                               .setAvailableTransports(Dict<IString, IInteger>({{"tls", 7421}, {"plain", 7420}}));

    const ServerCapabilityPtr cloned = capability.asPtr<IPropertyObjectInternal>().clone();

    ASSERT_EQ(cloned.getAvailableTransports().getKeyList(), List<IString>("tls", "plain"));
}

TEST_F(ServerCapabilityTest, PortWithoutTransports)
{
    ServerCapabilityConfigPtr capability = ServerCapability("protocol_id", "Protocol name", ProtocolType::Streaming).setPort(7420);

    ASSERT_EQ(capability.getPort(), 7420);
    ASSERT_EQ(capability.getAvailableTransports().getCount(), 0u);
}

TEST_F(ServerCapabilityTest, SetAvailableTransportsMirrorsPort)
{
    ServerCapabilityConfigPtr capability = ServerCapability("protocol_id", "Protocol name", ProtocolType::Streaming)
                                               .setPort(1234)
                                               .setAvailableTransports(Dict<IString, IInteger>({{"tls", 7421}, {"plain", 7420}}));

    ASSERT_EQ(capability.getPort(), 7421);
    ASSERT_EQ(capability.getPropertyValue("Port"), 7421);
}

TEST_F(ServerCapabilityTest, SetEmptyAvailableTransportsKeepsPort)
{
    ServerCapabilityConfigPtr capability = ServerCapability("protocol_id", "Protocol name", ProtocolType::Streaming)
                                               .setPort(7420)
                                               .setAvailableTransports(Dict<IString, IInteger>());

    ASSERT_EQ(capability.getPort(), 7420);
    ASSERT_EQ(capability.getPropertyValue("Port"), 7420);
}

TEST_F(ServerCapabilityTest, SetPortUpdatesDefaultTransport)
{
    ServerCapabilityConfigPtr capability = ServerCapability("protocol_id", "Protocol name", ProtocolType::Streaming)
                                               .setAvailableTransports(Dict<IString, IInteger>({{"tls", 7421}, {"plain", 7420}}))
                                               .setPort(8000);

    const auto transports = capability.getAvailableTransports();
    ASSERT_EQ(transports.getKeyList(), List<IString>("tls", "plain"));
    ASSERT_EQ(transports.get("tls"), 8000);
    ASSERT_EQ(transports.get("plain"), 7420);
    ASSERT_EQ(capability.getPort(), 8000);
    ASSERT_EQ(capability.getPropertyValue("Port"), 8000);
}

// Dict equality ignores order, so a reordering write is dropped and the port must follow the stored order
TEST_F(ServerCapabilityTest, ReorderedTransportsKeepPortInStep)
{
    ServerCapabilityConfigPtr capability = ServerCapability("protocol_id", "Protocol name", ProtocolType::Streaming)
                                               .setAvailableTransports(Dict<IString, IInteger>({{"tls", 7421}, {"plain", 7420}}))
                                               .setAvailableTransports(Dict<IString, IInteger>({{"plain", 7420}, {"tls", 7421}}));

    const auto defaultTransport = capability.getAvailableTransports().getKeyList().getItemAt(0);
    ASSERT_EQ(capability.getPort(), capability.getAvailableTransports().get(defaultTransport));
    ASSERT_EQ(capability.getPropertyValue("Port"), capability.getPort());
}

END_NAMESPACE_OPENDAQ
