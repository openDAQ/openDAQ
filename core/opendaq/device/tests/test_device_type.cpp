#include <opendaq/device_type_factory.h>
#include <opendaq/component_type_builder_factory.h>
#include <testutils/testutils.h>
#include <coretypes/json_serializer_factory.h>
#include <coretypes/json_deserializer_factory.h>
#include <opendaq/add_device_config_factory.h>
#include <coreobjects/property_object_factory.h>
#include <coreobjects/property_factory.h>
#include <coreobjects/property_object_internal_ptr.h>

using DeviceTypeTest = testing::Test;

BEGIN_NAMESPACE_OPENDAQ

// Dict equality ignores order, so the order is checked through the key list
TEST_F(DeviceTypeTest, AvailableTransportsDefault)
{
    const DeviceTypePtr factoryType = DeviceType("id", "name", "description", "prefix");
    ASSERT_EQ(factoryType.getAvailableTransports().getCount(), 0u);

    const DeviceTypePtr builtType = DeviceTypeBuilder().setId("id").setConnectionStringPrefix("prefix").build();
    ASSERT_EQ(builtType.getAvailableTransports().getCount(), 0u);
}

TEST_F(DeviceTypeTest, BuilderSetsAvailableTransports)
{
    const DeviceTypePtr type = DeviceTypeBuilder()
                                   .setId("id")
                                   .setConnectionStringPrefix("prefix")
                                   .setAvailableTransports(Dict<IString, IInteger>({{"tls", 7421}, {"plain", 7420}}))
                                   .build();

    const auto transports = type.getAvailableTransports();
    ASSERT_EQ(transports.getKeyList(), List<IString>("tls", "plain"));
    ASSERT_EQ(transports.get("tls"), 7421);
    ASSERT_EQ(transports.get("plain"), 7420);
}

TEST_F(DeviceTypeTest, BuilderSetAvailableTransportsReplaces)
{
    const auto builder = DeviceTypeBuilder()
                             .setAvailableTransports(Dict<IString, IInteger>({{"plain", 7420}, {"tls", 7421}}))
                             .setAvailableTransports(Dict<IString, IInteger>({{"https", 443}}));

    ASSERT_EQ(builder.getAvailableTransports().getKeyList(), List<IString>("https"));
}

TEST_F(DeviceTypeTest, BuilderSetAvailableTransportsInvalid)
{
    const auto builder = DeviceTypeBuilder();

    ASSERT_THROW(builder.setAvailableTransports(nullptr), ArgumentNullException);
    ASSERT_THROW(builder.setAvailableTransports(Dict<IString, IString>({{"plain", "7420"}})), InvalidTypeException);
    ASSERT_THROW(builder.setAvailableTransports(Dict<IInteger, IInteger>({{1, 7420}})), InvalidTypeException);
    ASSERT_EQ(builder.getAvailableTransports().getCount(), 0u);
}

TEST_F(DeviceTypeTest, AvailableTransportsAreFrozenCopy)
{
    auto transports = Dict<IString, IInteger>({{"plain", 7420}});
    const DeviceTypePtr type = DeviceTypeBuilder().setId("id").setAvailableTransports(transports).build();

    transports.set("tls", 7421);

    ASSERT_EQ(type.getAvailableTransports().getKeyList(), List<IString>("plain"));
    ASSERT_THROW(type.getAvailableTransports().set("wss", 7443), FrozenException);
}

TEST_F(DeviceTypeTest, SerializeDeserializeAvailableTransports)
{
    const DeviceTypePtr type = DeviceTypeBuilder()
                                   .setId("id")
                                   .setName("name")
                                   .setDescription("description")
                                   .setConnectionStringPrefix("prefix")
                                   .setAvailableTransports(Dict<IString, IInteger>({{"wss", 7443}, {"tls", 7421}, {"plain", 7420}}))
                                   .build();

    const auto serializer = JsonSerializer();
    type.serialize(serializer);
    const auto serializedType = serializer.getOutput();

    const auto deserializer = JsonDeserializer();
    const DeviceTypePtr newType = deserializer.deserialize(serializedType, nullptr, nullptr);

    ASSERT_EQ(newType.getId(), "id");
    ASSERT_EQ(newType.getConnectionStringPrefix(), "prefix");

    const auto transports = newType.getAvailableTransports();
    ASSERT_EQ(transports.getKeyList(), List<IString>("wss", "tls", "plain"));
    ASSERT_EQ(transports.get("wss"), 7443);

    serializer.reset();
    newType.serialize(serializer);
    ASSERT_EQ(serializedType, serializer.getOutput());
}

// Types without transports serialize exactly as an older SDK did, and the older format deserializes to an empty dictionary
TEST_F(DeviceTypeTest, SerializeWithoutAvailableTransports)
{
    const DeviceTypePtr type = DeviceType("id", "name", "description", "prefix");

    const auto serializer = JsonSerializer();
    type.serialize(serializer);
    const auto serializedType = serializer.getOutput();
    ASSERT_EQ(std::string(serializedType).find("availableTransports"), std::string::npos);

    const auto deserializer = JsonDeserializer();
    const DeviceTypePtr newType = deserializer.deserialize(serializedType, nullptr, nullptr);

    ASSERT_EQ(newType.getAvailableTransports().getCount(), 0u);
}

TEST_F(DeviceTypeTest, AddDeviceConfigSelectsFirstTransport)
{
    const AddDeviceConfigPtr config = AddDeviceConfig(List<IString>("tls", "plain"));

    ASSERT_EQ(config.getAvailableTransports(), List<IString>("tls", "plain"));
    ASSERT_EQ(config.getTransport(), "tls");
    ASSERT_EQ(config.getPropertySelectionValue("Transport"), "tls");
}

TEST_F(DeviceTypeTest, AddDeviceConfigSetTransport)
{
    const AddDeviceConfigPtr config = AddDeviceConfig(List<IString>("tls", "plain"));

    config.setTransport("plain");
    ASSERT_EQ(config.getTransport(), "plain");
    ASSERT_EQ(config.getPropertySelectionValue("Transport"), "plain");

    // The property is an equal alternative to the typed setter
    config.setPropertyValue("Transport", 0);
    ASSERT_EQ(config.getTransport(), "tls");
}

TEST_F(DeviceTypeTest, AddDeviceConfigSetUnknownTransport)
{
    const AddDeviceConfigPtr config = AddDeviceConfig(List<IString>("tls", "plain"));

    ASSERT_THROW(config.setTransport("wss"), NotFoundException);
    ASSERT_EQ(config.getTransport(), "tls");
}

TEST_F(DeviceTypeTest, AddDeviceConfigWithoutTransports)
{
    const AddDeviceConfigPtr config = AddDeviceConfig();

    ASSERT_EQ(config.getAvailableTransports().getCount(), 0u);
    ASSERT_FALSE(config.getTransport().assigned());
    ASSERT_FALSE(config.hasProperty("Transport"));
    ASSERT_THROW(config.setTransport("plain"), NotFoundException);
}

TEST_F(DeviceTypeTest, AddDeviceConfigClone)
{
    const AddDeviceConfigPtr config = AddDeviceConfig(List<IString>("tls", "plain"));
    config.setTransport("plain");

    const PropertyObjectPtr cloned = config.asPtr<IPropertyObjectInternal>().clone();
    const auto clonedConfig = cloned.asPtrOrNull<IAddDeviceConfig>();
    ASSERT_TRUE(clonedConfig.assigned());
    ASSERT_EQ(clonedConfig.getTransport(), "plain");
    ASSERT_EQ(clonedConfig.getAvailableTransports(), List<IString>("tls", "plain"));
}

TEST_F(DeviceTypeTest, AddDeviceConfigSerializeDeserialize)
{
    const AddDeviceConfigPtr config = AddDeviceConfig(List<IString>("tls", "plain"));
    config.setTransport("plain");

    const auto serializer = JsonSerializer();
    config.serialize(serializer);
    const auto serialized = serializer.getOutput();
    ASSERT_NE(std::string(serialized).find(R"("AddDeviceConfig")"), std::string::npos);

    const BaseObjectPtr deserialized = JsonDeserializer().deserialize(serialized, nullptr, nullptr);
    const auto newConfig = deserialized.asPtrOrNull<IAddDeviceConfig>();
    ASSERT_TRUE(newConfig.assigned());
    ASSERT_EQ(newConfig.getAvailableTransports(), List<IString>("tls", "plain"));
    ASSERT_EQ(newConfig.getTransport(), "plain");
}

TEST_F(DeviceTypeTest, DefaultConfigCarriesAddDeviceConfig)
{
    auto moduleConfig = PropertyObject();
    moduleConfig.addProperty(IntProperty("Port", 7420));

    const DeviceTypePtr type = DeviceTypeBuilder()
                                   .setId("id")
                                   .setConnectionStringPrefix("prefix")
                                   .setDefaultConfig(moduleConfig)
                                   .setAvailableTransports(Dict<IString, IInteger>({{"tls", 7421}, {"plain", 7420}}))
                                   .build();

    const PropertyObjectPtr config = type.createDefaultConfig();
    ASSERT_EQ(config.getPropertyValue("Port"), 7420);

    const auto addDeviceConfig = config.getPropertyValue("AddDeviceConfig").asPtrOrNull<IAddDeviceConfig>();
    ASSERT_TRUE(addDeviceConfig.assigned());
    ASSERT_EQ(addDeviceConfig.getAvailableTransports(), List<IString>("tls", "plain"));
    ASSERT_EQ(addDeviceConfig.getTransport(), "tls");

    // Every default config gets its own object
    addDeviceConfig.setTransport("plain");
    const AddDeviceConfigPtr freshConfig = type.createDefaultConfig().getPropertyValue("AddDeviceConfig");
    ASSERT_EQ(freshConfig.getTransport(), "tls");

    // The module's own default config is left as it was
    ASSERT_FALSE(moduleConfig.hasProperty("AddDeviceConfig"));
}

TEST_F(DeviceTypeTest, DefaultConfigWithoutTransports)
{
    const DeviceTypePtr type = DeviceTypeBuilder().setId("id").setConnectionStringPrefix("prefix").build();
    ASSERT_FALSE(type.createDefaultConfig().hasProperty("AddDeviceConfig"));

    const DeviceTypePtr factoryType = DeviceType("id", "name", "description", "prefix");
    ASSERT_FALSE(factoryType.createDefaultConfig().hasProperty("AddDeviceConfig"));
}

// The module manager nests each type's default config into the add-device config, which clones it
TEST_F(DeviceTypeTest, NestedAddDeviceConfigKeepsInterface)
{
    const DeviceTypePtr type = DeviceTypeBuilder()
                                   .setId("id")
                                   .setConnectionStringPrefix("prefix")
                                   .setAvailableTransports(Dict<IString, IInteger>({{"tls", 7421}, {"plain", 7420}}))
                                   .build();

    auto deviceConfigs = PropertyObject();
    deviceConfigs.addProperty(ObjectProperty("id", type.createDefaultConfig()));

    const PropertyObjectPtr typeConfig = deviceConfigs.getPropertyValue("id");
    AddDeviceConfigPtr addDeviceConfig = typeConfig.getPropertyValue("AddDeviceConfig");
    addDeviceConfig.setTransport("plain");

    const PropertyObjectPtr cloned = deviceConfigs.asPtr<IPropertyObjectInternal>().clone();
    const PropertyObjectPtr clonedTypeConfig = cloned.getPropertyValue("id");
    const auto clonedAddDeviceConfig = clonedTypeConfig.getPropertyValue("AddDeviceConfig").asPtrOrNull<IAddDeviceConfig>();
    ASSERT_TRUE(clonedAddDeviceConfig.assigned());
    ASSERT_EQ(clonedAddDeviceConfig.getTransport(), "plain");
}

TEST_F(DeviceTypeTest, DefaultConfigSerializeDeserialize)
{
    const DeviceTypePtr type = DeviceTypeBuilder()
                                   .setId("id")
                                   .setConnectionStringPrefix("prefix")
                                   .setAvailableTransports(Dict<IString, IInteger>({{"tls", 7421}, {"plain", 7420}}))
                                   .build();

    const PropertyObjectPtr config = type.createDefaultConfig();
    AddDeviceConfigPtr addDeviceConfig = config.getPropertyValue("AddDeviceConfig");
    addDeviceConfig.setTransport("plain");

    const auto serializer = JsonSerializer();
    config.serialize(serializer);

    const PropertyObjectPtr newConfig = JsonDeserializer().deserialize(serializer.getOutput(), nullptr, nullptr);
    const auto newAddDeviceConfig = newConfig.getPropertyValue("AddDeviceConfig").asPtrOrNull<IAddDeviceConfig>();
    ASSERT_TRUE(newAddDeviceConfig.assigned());
    ASSERT_EQ(newAddDeviceConfig.getAvailableTransports(), List<IString>("tls", "plain"));
    ASSERT_EQ(newAddDeviceConfig.getTransport(), "plain");
}

END_NAMESPACE_OPENDAQ
