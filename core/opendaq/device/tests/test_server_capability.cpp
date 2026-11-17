#include <coreobjects/property_factory.h>
#include <coreobjects/property_object_factory.h>
#include <coreobjects/property_object_internal_ptr.h>
#include <coretypes/json_deserializer_factory.h>
#include <coretypes/json_serializer_factory.h>
#include <opendaq/context_factory.h>
#include <opendaq/device_info_factory.h>
#include <opendaq/device_type_factory.h>
#include <opendaq/transport_option_factory.h>
#include <opendaq/transport_option_impl.h>
#include <testutils/testutils.h>

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

TEST_F(ServerCapabilityTest, TransportOptionsDefault)
{
    ServerCapabilityConfigPtr capability = ServerCapability("protocol_id", "Protocol name", ProtocolType::Streaming);
    ASSERT_EQ(capability.getAvailableTransportOptions().getCount(), 0u);
}

TEST_F(ServerCapabilityTest, SetGetTransportOptions)
{
    ServerCapabilityConfigPtr capability =
        ServerCapability("protocol_id", "Protocol name", ProtocolType::Streaming)
            .setAvailableTransportOptions(TransportOptionsById({TransportOption("wss", 7443, SecurityMode::Encrypted, True),
                                                                TransportOption("tls", 7421, SecurityMode::Encrypted),
                                                                TransportOption("plain", 7420)}));

    const auto options = capability.getAvailableTransportOptions();
    ASSERT_EQ(options.getKeyList(), List<IString>("wss", "tls", "plain"));
    ASSERT_EQ(options.getValueList()[1].getPort(), 7421);
    ASSERT_EQ(options.getValueList()[0].getSecurityMode(), SecurityMode::Encrypted);
    ASSERT_TRUE(options.getValueList()[0].getCertificateRequired());
    ASSERT_EQ(options.getValueList()[2].getSecurityMode(), SecurityMode::None);
}

TEST_F(ServerCapabilityTest, SetTransportOptionsReplaces)
{
    ServerCapabilityConfigPtr capability = ServerCapability("protocol_id", "Protocol name", ProtocolType::Streaming);

    capability.setAvailableTransportOptions(TransportOptionsById({TransportOption("plain", 7420), TransportOption("tls", 7421)}));
    capability.setAvailableTransportOptions(TransportOptionsById({TransportOption("https", 443)}));

    ASSERT_EQ(capability.getAvailableTransportOptions().getKeyList(), List<IString>("https"));
}

TEST_F(ServerCapabilityTest, SetTransportOptionsInvalid)
{
    ServerCapabilityConfigPtr capability = ServerCapability("protocol_id", "Protocol name", ProtocolType::Streaming);

    ASSERT_THROW(capability.setAvailableTransportOptions(nullptr), ArgumentNullException);
    ASSERT_THROW(capability.setAvailableTransportOptions(Dict<IString, IBaseObject>({{"plain", String("plain")}})), InvalidTypeException);
    ASSERT_THROW(capability.setAvailableTransportOptions(Dict<IBaseObject, IBaseObject>({{Integer(1), TransportOption("plain", 7420)}})),
                 InvalidTypeException);
    // the key must be the id of its option
    ASSERT_THROW(capability.setAvailableTransportOptions(Dict<IString, ITransportOption>({{"tls", TransportOption("plain", 7420)}})),
                 InvalidValueException);
    ASSERT_EQ(capability.getAvailableTransportOptions().getCount(), 0u);
}

TEST_F(ServerCapabilityTest, AddTransportOption)
{
    ServerCapabilityConfigPtr capability = ServerCapability("protocol_id", "Protocol name", ProtocolType::Streaming)
                                               .addAvailableTransportOption(TransportOption("plain", 7420))
                                               .addAvailableTransportOption(TransportOption("tls", 7422, SecurityMode::Encrypted));

    ASSERT_EQ(capability.getAvailableTransportOptions().getKeyList(), List<IString>("plain", "tls"));
    // the first option added is the default one
    ASSERT_EQ(capability.getPort(), 7420);
    ASSERT_EQ(capability.getPropertyValue("Port"), 7420);

    ASSERT_THROW(capability.addAvailableTransportOption(TransportOption("tls", 9000)), AlreadyExistsException);
    ASSERT_THROW(capability.addAvailableTransportOption(nullptr), ArgumentNullException);
    ASSERT_EQ(capability.getAvailableTransportOptions().getValueList()[1].getPort(), 7422);
}

TEST_F(ServerCapabilityTest, TransportOptionsFrozen)
{
    ServerCapabilityConfigPtr capability = ServerCapability("protocol_id", "Protocol name", ProtocolType::Streaming);
    capability.freeze();

    ASSERT_THROW(capability.setAvailableTransportOptions(TransportOptionsById({TransportOption("plain", 7420)})), FrozenException);
    ASSERT_THROW(capability.addAvailableTransportOption(TransportOption("plain", 7420)), FrozenException);
}

TEST_F(ServerCapabilityTest, TransportOptionsAreCopied)
{
    auto options = TransportOptionsById({TransportOption("plain", 7420)});
    ServerCapabilityConfigPtr capability =
        ServerCapability("protocol_id", "Protocol name", ProtocolType::Streaming).setAvailableTransportOptions(options);

    options.set("tls", TransportOption("tls", 7421));
    capability.getAvailableTransportOptions().set("wss", TransportOption("wss", 7443));

    ASSERT_EQ(capability.getAvailableTransportOptions().getKeyList(), List<IString>("plain"));
}

// The options are written as plain Property objects, which a client without transport options reads too
// The capability keeps each option as a plain Property object, which peers that do not know transport options read
TEST_F(ServerCapabilityTest, TransportOptionsKeptAsPlainPropertyObjects)
{
    ServerCapabilityConfigPtr capability =
        ServerCapability("protocol_id", "Protocol name", ProtocolType::Streaming)
            .setAvailableTransportOptions(TransportOptionsById({TransportOption("tls", 7421, SecurityMode::Encrypted, True)}));

    const PropertyObjectPtr container = capability.getPropertyValue("AvailableTransportOptions");
    const PropertyObjectPtr tls = container.getPropertyValue("tls");

    ASSERT_FALSE(tls.supportsInterface<ITransportOption>());
    ASSERT_EQ(tls.getPropertyValue("Id"), "tls");
    ASSERT_EQ(tls.getPropertyValue("Port"), 7421);
    ASSERT_EQ(tls.getPropertySelectionValue("SecurityMode"), "Encrypted");
    ASSERT_EQ(tls.getPropertyValue("CertificateRequired"), true);
}

TEST_F(ServerCapabilityTest, SerializeDeserializeTransportOptions)
{
    ServerCapabilityConfigPtr capability =
        ServerCapability("protocol_id", "Protocol name", ProtocolType::Streaming)
            .setAvailableTransportOptions(TransportOptionsById({TransportOption("wss", 7443, SecurityMode::Encrypted, True),
                                                                TransportOption("tls", 7421, SecurityMode::Encrypted),
                                                                TransportOption("plain", 7420)}));

    const auto serializer = JsonSerializer();
    capability.serialize(serializer);
    const auto serializedCapability = serializer.getOutput();
    ASSERT_EQ(std::string(serializedCapability).find(R"("__type":"TransportOption")"), std::string::npos);

    const auto deserializer = JsonDeserializer();
    const ServerCapabilityPtr newCapability = deserializer.deserialize(serializedCapability, nullptr, nullptr);

    const auto options = newCapability.getAvailableTransportOptions();
    ASSERT_EQ(options.getKeyList(), List<IString>("wss", "tls", "plain"));
    ASSERT_EQ(options.getValueList()[1].getPort(), 7421);
    ASSERT_EQ(options.getValueList()[0].getSecurityMode(), SecurityMode::Encrypted);
    ASSERT_TRUE(options.getValueList()[0].getCertificateRequired());
    ASSERT_EQ(newCapability.getPort(), 7443);
    // What an older client reads
    ASSERT_EQ(newCapability.getPropertyValue("Port"), 7443);

    serializer.reset();
    newCapability.serialize(serializer);
    ASSERT_EQ(serializedCapability, serializer.getOutput());
}

// A capability serialized by an older SDK has no transport options property
TEST_F(ServerCapabilityTest, DeserializeWithoutTransportOptions)
{
    ServerCapabilityConfigPtr capability = ServerCapability("protocol_id", "Protocol name", ProtocolType::Streaming).setPort(7420);
    capability.removeProperty("AvailableTransportOptions");

    const auto serializer = JsonSerializer();
    capability.serialize(serializer);
    const auto serializedCapability = serializer.getOutput();
    ASSERT_EQ(std::string(serializedCapability).find("AvailableTransportOptions"), std::string::npos);

    const auto deserializer = JsonDeserializer();
    const ServerCapabilityPtr newCapability = deserializer.deserialize(serializedCapability, nullptr, nullptr);

    ASSERT_EQ(newCapability.getAvailableTransportOptions().getCount(), 0u);
    ASSERT_EQ(newCapability.getPort(), 7420);
}

TEST_F(ServerCapabilityTest, CloneTransportOptions)
{
    ServerCapabilityConfigPtr capability =
        ServerCapability("protocol_id", "Protocol name", ProtocolType::Streaming)
            .setAvailableTransportOptions(
                TransportOptionsById({TransportOption("tls", 7421, SecurityMode::Encrypted), TransportOption("plain", 7420)}));

    const ServerCapabilityPtr cloned = capability.asPtr<IPropertyObjectInternal>().clone();

    ASSERT_EQ(cloned.getAvailableTransportOptions().getKeyList(), List<IString>("tls", "plain"));
    ASSERT_EQ(cloned.getAvailableTransportOptions().getValueList()[0].getSecurityMode(), SecurityMode::Encrypted);
}

TEST_F(ServerCapabilityTest, PortWithoutTransportOptions)
{
    ServerCapabilityConfigPtr capability = ServerCapability("protocol_id", "Protocol name", ProtocolType::Streaming).setPort(7420);

    ASSERT_EQ(capability.getPort(), 7420);
    ASSERT_EQ(capability.getAvailableTransportOptions().getCount(), 0u);
}

TEST_F(ServerCapabilityTest, SetTransportOptionsMirrorsPort)
{
    ServerCapabilityConfigPtr capability =
        ServerCapability("protocol_id", "Protocol name", ProtocolType::Streaming)
            .setPort(1234)
            .setAvailableTransportOptions(TransportOptionsById({TransportOption("tls", 7421), TransportOption("plain", 7420)}));

    ASSERT_EQ(capability.getPort(), 7421);
    ASSERT_EQ(capability.getPropertyValue("Port"), 7421);
}

TEST_F(ServerCapabilityTest, SetEmptyTransportOptionsKeepsPort)
{
    ServerCapabilityConfigPtr capability = ServerCapability("protocol_id", "Protocol name", ProtocolType::Streaming)
                                               .setPort(7420)
                                               .setAvailableTransportOptions(TransportOptionsById({}));

    ASSERT_EQ(capability.getPort(), 7420);
    ASSERT_EQ(capability.getPropertyValue("Port"), 7420);
}

TEST_F(ServerCapabilityTest, SetPortUpdatesDefaultOption)
{
    ServerCapabilityConfigPtr capability = ServerCapability("protocol_id", "Protocol name", ProtocolType::Streaming)
                                               .setAvailableTransportOptions(TransportOptionsById(
                                                   {TransportOption("tls", 7421, SecurityMode::Encrypted), TransportOption("plain", 7420)}))
                                               .setPort(8000);

    const auto options = capability.getAvailableTransportOptions();
    ASSERT_EQ(options.getKeyList(), List<IString>("tls", "plain"));
    ASSERT_EQ(options.getValueList()[0].getPort(), 8000);
    ASSERT_EQ(options.getValueList()[0].getSecurityMode(), SecurityMode::Encrypted);
    ASSERT_EQ(options.getValueList()[1].getPort(), 7420);
    ASSERT_EQ(capability.getPort(), 8000);
    ASSERT_EQ(capability.getPropertyValue("Port"), 8000);
}

// Unlike a dictionary, the list keeps a reordering, so the default option and the port follow it
TEST_F(ServerCapabilityTest, ReorderingChangesTheDefaultOption)
{
    ServerCapabilityConfigPtr capability =
        ServerCapability("protocol_id", "Protocol name", ProtocolType::Streaming)
            .setAvailableTransportOptions(TransportOptionsById({TransportOption("tls", 7421), TransportOption("plain", 7420)}))
            .setAvailableTransportOptions(TransportOptionsById({TransportOption("plain", 7420), TransportOption("tls", 7421)}));

    ASSERT_EQ(capability.getAvailableTransportOptions().getValueList()[0].getId(), "plain");
    ASSERT_EQ(capability.getPort(), 7420);
    ASSERT_EQ(capability.getPropertyValue("Port"), 7420);
}

END_NAMESPACE_OPENDAQ
