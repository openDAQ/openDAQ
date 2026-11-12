#include <opendaq/device_type_factory.h>
#include <opendaq/component_type_builder_factory.h>
#include <testutils/testutils.h>
#include <coretypes/json_serializer_factory.h>
#include <coretypes/json_deserializer_factory.h>
#include <opendaq/transport_option_impl.h>

using DeviceTypeTest = testing::Test;

BEGIN_NAMESPACE_OPENDAQ

TEST_F(DeviceTypeTest, TransportOptionsDefault)
{
    const DeviceTypePtr factoryType = DeviceType("id", "name", "description", "prefix");
    ASSERT_EQ(factoryType.getTransportOptions().getCount(), 0u);

    const DeviceTypePtr builtType = DeviceTypeBuilder().setId("id").setConnectionStringPrefix("prefix").build();
    ASSERT_EQ(builtType.getTransportOptions().getCount(), 0u);
}

TEST_F(DeviceTypeTest, BuilderSetsTransportOptions)
{
    const DeviceTypePtr type = DeviceTypeBuilder()
                                   .setId("id")
                                   .setConnectionStringPrefix("prefix")
                                   .setTransportOptions(List<ITransportOption>(TransportOption("tls", 7421, TransportProtection::Encrypted),
                                                                               TransportOption("plain", 7420)))
                                   .build();

    const auto options = type.getTransportOptions();
    ASSERT_EQ(TransportOptionNames(options), List<IString>("tls", "plain"));
    ASSERT_EQ(options[0].getPort(), 7421);
    ASSERT_EQ(options[0].getProtection(), TransportProtection::Encrypted);
    ASSERT_EQ(options[1].getPort(), 7420);
    ASSERT_EQ(options[1].getProtection(), TransportProtection::None);
}

TEST_F(DeviceTypeTest, BuilderSetTransportOptionsReplaces)
{
    const auto builder = DeviceTypeBuilder()
                             .setTransportOptions(List<ITransportOption>(TransportOption("plain", 7420), TransportOption("tls", 7421)))
                             .setTransportOptions(List<ITransportOption>(TransportOption("https", 443)));

    ASSERT_EQ(TransportOptionNames(builder.getTransportOptions()), List<IString>("https"));
}

TEST_F(DeviceTypeTest, BuilderSetTransportOptionsInvalid)
{
    const auto builder = DeviceTypeBuilder();

    ASSERT_THROW(builder.setTransportOptions(nullptr), ArgumentNullException);
    ASSERT_THROW(builder.setTransportOptions(List<IBaseObject>(String("plain"))), InvalidTypeException);
    ASSERT_THROW(builder.setTransportOptions(List<ITransportOption>(TransportOption("plain", 7420), TransportOption("plain", 7421))),
                 InvalidValueException);
    ASSERT_EQ(builder.getTransportOptions().getCount(), 0u);
}

TEST_F(DeviceTypeTest, TransportOptionsAreFrozenCopy)
{
    auto options = List<ITransportOption>(TransportOption("plain", 7420));
    const DeviceTypePtr type = DeviceTypeBuilder().setId("id").setTransportOptions(options).build();

    options.pushBack(TransportOption("tls", 7421));

    ASSERT_EQ(TransportOptionNames(type.getTransportOptions()), List<IString>("plain"));
    ASSERT_THROW(type.getTransportOptions().pushBack(TransportOption("wss", 7443)), FrozenException);
}

TEST_F(DeviceTypeTest, SerializeDeserializeTransportOptions)
{
    const DeviceTypePtr type = DeviceTypeBuilder()
                                   .setId("id")
                                   .setName("name")
                                   .setDescription("description")
                                   .setConnectionStringPrefix("prefix")
                                   .setTransportOptions(List<ITransportOption>(TransportOption("wss", 7443, TransportProtection::Encrypted, True),
                                                                               TransportOption("tls", 7421, TransportProtection::Encrypted),
                                                                               TransportOption("plain", 7420)))
                                   .build();

    const auto serializer = JsonSerializer();
    type.serialize(serializer);
    const auto serializedType = serializer.getOutput();

    const auto deserializer = JsonDeserializer();
    const DeviceTypePtr newType = deserializer.deserialize(serializedType, nullptr, nullptr);

    ASSERT_EQ(newType.getId(), "id");
    ASSERT_EQ(newType.getConnectionStringPrefix(), "prefix");

    // read back as plain Property objects and restored to transport options
    const auto options = newType.getTransportOptions();
    ASSERT_EQ(TransportOptionNames(options), List<IString>("wss", "tls", "plain"));
    ASSERT_EQ(options[0].getPort(), 7443);
    ASSERT_EQ(options[0].getProtection(), TransportProtection::Encrypted);
    ASSERT_TRUE(options[0].getCertificateRequired());
    ASSERT_FALSE(options[1].getCertificateRequired());

    serializer.reset();
    newType.serialize(serializer);
    ASSERT_EQ(serializedType, serializer.getOutput());
}

// Types without transport options serialize exactly as an older SDK did, and the older format deserializes to no options
TEST_F(DeviceTypeTest, SerializeWithoutTransportOptions)
{
    const DeviceTypePtr type = DeviceType("id", "name", "description", "prefix");

    const auto serializer = JsonSerializer();
    type.serialize(serializer);
    const auto serializedType = serializer.getOutput();
    ASSERT_EQ(std::string(serializedType).find("transportOptions"), std::string::npos);

    const auto deserializer = JsonDeserializer();
    const DeviceTypePtr newType = deserializer.deserialize(serializedType, nullptr, nullptr);

    ASSERT_EQ(newType.getTransportOptions().getCount(), 0u);
}

END_NAMESPACE_OPENDAQ
