#include <opendaq/device_type_factory.h>
#include <opendaq/component_type_builder_factory.h>
#include <testutils/testutils.h>
#include <testutils/transport_options.h>
#include <coretypes/json_serializer_factory.h>
#include <coretypes/json_deserializer_factory.h>
#include <opendaq/transport_option_impl.h>
#include <coreobjects/property_object_factory.h>
#include <coreobjects/property_factory.h>
#include <coreobjects/property_object_internal_ptr.h>

using DeviceTypeTest = testing::Test;

BEGIN_NAMESPACE_OPENDAQ

TEST_F(DeviceTypeTest, TransportOptionsDefault)
{
    const DeviceTypePtr factoryType = DeviceType("id", "name", "description", "prefix");
    ASSERT_EQ(factoryType.getAvailableTransportOptions().getCount(), 0u);

    const DeviceTypePtr builtType = DeviceTypeBuilder().setId("id").setConnectionStringPrefix("prefix").build();
    ASSERT_EQ(builtType.getAvailableTransportOptions().getCount(), 0u);
}

TEST_F(DeviceTypeTest, BuilderSetsTransportOptions)
{
    const DeviceTypePtr type = DeviceTypeBuilder()
                                   .setId("id")
                                   .setConnectionStringPrefix("prefix")
                                   .setAvailableTransportOptions(TransportOptionsById(
                                       {TransportOption("tls", 7421, SecurityMode::Encrypted), TransportOption("plain", 7420)}))
                                   .build();

    const auto options = type.getAvailableTransportOptions();
    ASSERT_EQ(options.getKeyList(), List<IString>("tls", "plain"));
    ASSERT_EQ(options.getValueList()[0].getPort(), 7421);
    ASSERT_EQ(options.getValueList()[0].getSecurityMode(), SecurityMode::Encrypted);
    ASSERT_EQ(options.getValueList()[1].getPort(), 7420);
    ASSERT_EQ(options.getValueList()[1].getSecurityMode(), SecurityMode::None);
}

TEST_F(DeviceTypeTest, BuilderSetTransportOptionsReplaces)
{
    const auto builder =
        DeviceTypeBuilder()
            .setAvailableTransportOptions(TransportOptionsById({TransportOption("plain", 7420), TransportOption("tls", 7421)}))
            .setAvailableTransportOptions(TransportOptionsById({TransportOption("https", 443)}));

    ASSERT_EQ(builder.getAvailableTransportOptions().getKeyList(), List<IString>("https"));
}

TEST_F(DeviceTypeTest, BuilderSetTransportOptionsInvalid)
{
    const auto builder = DeviceTypeBuilder();

    ASSERT_THROW(builder.setAvailableTransportOptions(nullptr), ArgumentNullException);
    ASSERT_THROW(builder.setAvailableTransportOptions(Dict<IString, IBaseObject>({{"plain", String("plain")}})), InvalidTypeException);
    ASSERT_THROW(builder.setAvailableTransportOptions(Dict<IBaseObject, IBaseObject>({{Integer(1), TransportOption("plain", 7420)}})),
                 InvalidTypeException);
    // the key must be the id of its option
    ASSERT_THROW(builder.setAvailableTransportOptions(Dict<IString, ITransportOption>({{"tls", TransportOption("plain", 7420)}})),
                 InvalidValueException);
    ASSERT_EQ(builder.getAvailableTransportOptions().getCount(), 0u);
}

TEST_F(DeviceTypeTest, TransportOptionsAreFrozenCopy)
{
    auto options = TransportOptionsById({TransportOption("plain", 7420)});
    const DeviceTypePtr type = DeviceTypeBuilder().setId("id").setAvailableTransportOptions(options).build();

    options.set("tls", TransportOption("tls", 7421));

    ASSERT_EQ(type.getAvailableTransportOptions().getKeyList(), List<IString>("plain"));
    ASSERT_THROW(type.getAvailableTransportOptions().set("wss", TransportOption("wss", 7443)), FrozenException);
}

// The builder keeps a copy of the options it checked: the caller changing its dictionary before build changes nothing
TEST_F(DeviceTypeTest, BuilderKeepsCopyOfTransportOptions)
{
    auto options = TransportOptionsById({TransportOption("plain", 7420)});
    const auto builder = DeviceTypeBuilder().setId("id").setAvailableTransportOptions(options);

    options.set("wrong", TransportOption("tls", 7421));

    ASSERT_EQ(builder.getAvailableTransportOptions().getKeyList(), List<IString>("plain"));
    ASSERT_EQ(builder.build().getAvailableTransportOptions().getKeyList(), List<IString>("plain"));
}

TEST_F(DeviceTypeTest, SerializeDeserializeTransportOptions)
{
    const DeviceTypePtr type =
        DeviceTypeBuilder()
            .setId("id")
            .setName("name")
            .setDescription("description")
            .setConnectionStringPrefix("prefix")
            .setAvailableTransportOptions(TransportOptionsById({TransportOption("wss", 7443, SecurityMode::Encrypted, True),
                                                                TransportOption("tls", 7421, SecurityMode::Encrypted),
                                                                TransportOption("plain", 7420)}))
            .build();

    const auto serializer = JsonSerializer();
    type.serialize(serializer);
    const auto serializedType = serializer.getOutput();

    const auto deserializer = JsonDeserializer();
    const DeviceTypePtr newType = deserializer.deserialize(serializedType, nullptr, nullptr);

    ASSERT_EQ(newType.getId(), "id");
    ASSERT_EQ(newType.getConnectionStringPrefix(), "prefix");

    // read back as plain Property objects and restored to transport options
    const auto options = newType.getAvailableTransportOptions();
    ASSERT_EQ(options.getKeyList(), List<IString>("wss", "tls", "plain"));
    ASSERT_EQ(options.getValueList()[0].getPort(), 7443);
    ASSERT_EQ(options.getValueList()[0].getSecurityMode(), SecurityMode::Encrypted);
    ASSERT_TRUE(options.getValueList()[0].getCertificateRequired());
    ASSERT_FALSE(options.getValueList()[1].getCertificateRequired());

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
    ASSERT_EQ(std::string(serializedType).find("availableTransportOptions"), std::string::npos);

    const auto deserializer = JsonDeserializer();
    const DeviceTypePtr newType = deserializer.deserialize(serializedType, nullptr, nullptr);

    ASSERT_EQ(newType.getAvailableTransportOptions().getCount(), 0u);
}

// The module manager adds the selection properties right before a device is created, not to the default config
TEST_F(DeviceTypeTest, DefaultConfigHasNoTransportSelection)
{
    auto moduleConfig = PropertyObject();
    moduleConfig.addProperty(IntProperty("Port", 7000));

    const DeviceTypePtr type =
        DeviceTypeBuilder()
            .setId("id")
            .setConnectionStringPrefix("prefix")
            .setDefaultConfig(moduleConfig)
            .setAvailableTransportOptions(TransportOptionsById({TransportOption("tls", 7421), TransportOption("plain", 7420)}))
            .build();

    // The module's own port is kept as it is
    const PropertyObjectPtr config = type.createDefaultConfig();
    ASSERT_FALSE(config.hasProperty(TransportOptionConfigProperty));
    ASSERT_EQ(config.getPropertyValue(PortConfigProperty), 7000);

    const DeviceTypePtr withoutModuleConfig = DeviceTypeBuilder()
                                                  .setId("id")
                                                  .setConnectionStringPrefix("prefix")
                                                  .setAvailableTransportOptions(TransportOptionsById({TransportOption("tls", 7421)}))
                                                  .build();
    ASSERT_FALSE(withoutModuleConfig.createDefaultConfig().hasProperty(TransportOptionConfigProperty));
    ASSERT_FALSE(withoutModuleConfig.createDefaultConfig().hasProperty(PortConfigProperty));
}

END_NAMESPACE_OPENDAQ
