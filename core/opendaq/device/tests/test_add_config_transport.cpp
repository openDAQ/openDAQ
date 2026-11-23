#include <gtest/gtest.h>
#include <opendaq/add_component_config_factory.h>
#include <opendaq/add_device_config_private_ptr.h>
#include <opendaq/add_streaming_options_ptr.h>
#include <opendaq/component_type_builder_factory.h>
#include <opendaq/device_info_factory.h>
#include <opendaq/transport_option_factory.h>
#include <opendaq/transport_option_impl.h>
#include <testutils/testutils.h>
#include <testutils/transport_options.h>
#include <coreobjects/property_object_factory.h>
#include <coreobjects/property_factory.h>
#include <coreobjects/property_object_internal_ptr.h>
#include <coretypes/json_serializer_factory.h>
#include <coretypes/updatable_ptr.h>
#include <coretypes/json_deserializer_factory.h>
#include <opendaq/add_device_config_legacy.h>
#include <opendaq/logger_factory.h>

BEGIN_NAMESPACE_OPENDAQ

// The transport selection of the add-device and add-streaming configs
class AddConfigTransportTest : public testing::Test
{
protected:
    static DictPtr<IString, ITransportOption> TypeOptions()
    {
        return TransportOptionsById({TransportOption("tls", 7421, SecurityMode::Encrypted), TransportOption("plain", 7420)});
    }

    // A Mirrored Device type whose module declares a port of its own in the default config
    static DeviceTypePtr CreateDeviceType(const DictPtr<IString, ITransportOption>& options = TypeOptions())
    {
        auto defaultConfig = PropertyObject();
        defaultConfig.addProperty(IntProperty(PortConfigProperty, 7000));
        return DeviceTypeBuilder()
            .setId("dev")
            .setName("dev")
            .setConnectionStringPrefix("dev")
            .setDeviceKind(DeviceKind::Mirrored)
            .setDefaultConfig(defaultConfig)
            .setAvailableTransportOptions(options)
            .build();
    }

    static StreamingTypePtr CreateStreamingType(const DictPtr<IString, ITransportOption>& options = TypeOptions())
    {
        return StreamingTypeBuilder()
            .setId("str")
            .setName("str")
            .setConnectionStringPrefix("str")
            .setAvailableTransportOptions(options)
            .build();
    }

    static AddDeviceConfigPtr CreateConfig(const DeviceTypePtr& deviceType = CreateDeviceType())
    {
        return AddDeviceConfig(deviceType, Dict<IString, IStreamingType>({{"str", CreateStreamingType()}}), nullptr, True);
    }

    // Whether the saved form of the config holds a value of its Port property: a port not set is not saved
    static bool SavesPort(const BaseObjectPtr& config)
    {
        const auto serializer = JsonSerializer();
        config.asPtr<IUpdatable>().serializeForUpdate(serializer);

        bool saves = false;
        JsonDeserializer().callCustomProc(
            [&saves](const SerializedObjectPtr& saved)
            { saves = saved.hasKey("propValues") && saved.readSerializedObject("propValues").hasKey("Port"); },
            serializer.getOutput());
        return saves;
    }

    static PropertyObjectPtr Properties(const BaseObjectPtr& config)
    {
        return config.asPtr<IPropertyObject>();
    }

    static AddDeviceConfigPtr Bind(const AddDeviceConfigPtr& config, const DeviceTypePtr& deviceType = CreateDeviceType())
    {
        const auto deviceTypes = Dict<IString, IDeviceType>({{"dev", deviceType}});
        const auto streamingTypes = Dict<IString, IStreamingType>({{"str", CreateStreamingType()}});
        return config.asPtr<IAddDeviceConfigPrivate>().bind(deviceTypes, streamingTypes);
    }
};

TEST_F(AddConfigTransportTest, ListsTypeOptions)
{
    const auto config = CreateConfig();

    ASSERT_EQ(config.getAvailableTransportOptions().getKeyList(), List<IString>("tls", "plain"));
    ASSERT_EQ(config.getSelectedTransportOption().getId(), "tls");

    // The first option is selected with its port, not the one the module declares
    ASSERT_EQ(config.getPort(), 7421);
    ASSERT_FALSE(SavesPort(config));

    // The selection is in properties of the config; the type config stays as the module declares it
    const auto properties = Properties(config);
    ASSERT_EQ(properties.getPropertyValue("TransportOption"), "tls");
    ASSERT_EQ(properties.getProperty("TransportOption").getSelectionValues(), List<IString>("tls", "plain"));
    // -1 while no port is set: the port of the selected option applies
    ASSERT_EQ(properties.getPropertyValue("Port"), -1);
    ASSERT_EQ(config.getPort(), 7421);
    ASSERT_FALSE(config.getTypeConfig().hasProperty(TransportOptionConfigProperty));
    ASSERT_EQ(config.getTypeConfig().getPropertyValue(PortConfigProperty), 7000);
}

// The properties are the selection: the methods read and write them
TEST_F(AddConfigTransportTest, SelectionProperties)
{
    const auto config = CreateConfig();
    const auto properties = Properties(config);

    properties.setPropertyValue("TransportOption", "plain");
    ASSERT_EQ(config.getSelectedTransportOption().getId(), "plain");
    ASSERT_EQ(config.getPort(), 7420);

    properties.setPropertyValue("Port", 9000);
    ASSERT_EQ(config.getPort(), 9000);
    ASSERT_TRUE(SavesPort(config));

    // Like setTransportOption, writing the property keeps the port set
    properties.setPropertyValue("TransportOption", "tls");
    ASSERT_EQ(config.getPort(), 9000);

    config.setTransportOption("plain");
    ASSERT_EQ(properties.getPropertyValue("TransportOption"), "plain");
    ASSERT_EQ(properties.getPropertyValue("Port"), 9000);

    ASSERT_ANY_THROW(properties.setPropertyValue("TransportOption", "wss"));
    ASSERT_EQ(config.getSelectedTransportOption().getId(), "plain");
}

// The options give the default port; a port set overrides it whichever option is selected
TEST_F(AddConfigTransportTest, SetTransportOptionKeepsPortSetBefore)
{
    const auto config = CreateConfig();

    config.setPort(9000);
    ASSERT_EQ(config.getPort(), 9000);
    ASSERT_TRUE(SavesPort(config));

    config.setTransportOption("plain");
    ASSERT_EQ(config.getSelectedTransportOption().getId(), "plain");
    ASSERT_EQ(config.getSelectedTransportOption().getPort(), 7420);
    ASSERT_EQ(config.getPort(), 9000);
    ASSERT_TRUE(SavesPort(config));

    // The type config is not touched
    ASSERT_FALSE(config.getTypeConfig().hasProperty(TransportOptionConfigProperty));
    ASSERT_EQ(config.getTypeConfig().getPropertyValue(PortConfigProperty), 7000);
}

// A port equal to the one of the selected option is set all the same: it stays when another option is selected
TEST_F(AddConfigTransportTest, SetPortEqualToOptionPortIsKept)
{
    const auto config = CreateConfig();
    ASSERT_EQ(config.getSelectedTransportOption().getPort(), 7421);

    config.setPort(7421);
    ASSERT_TRUE(SavesPort(config));

    config.setTransportOption("plain");
    ASSERT_EQ(config.getPort(), 7421);
}

// -1 drops the port set, so the port of the selected option applies again
TEST_F(AddConfigTransportTest, SetPortMinusOneUsesTheOptionPortAgain)
{
    const auto config = CreateConfig();
    config.setPort(9000);
    config.setPort(-1);
    ASSERT_EQ(config.getPort(), 7421);
    ASSERT_FALSE(SavesPort(config));

    config.setTransportOption("plain");
    ASSERT_EQ(config.getPort(), 7420);

    const auto streamingConfig = AddStreamingConfig(CreateStreamingType());
    streamingConfig.setPort(9000);
    streamingConfig.setPort(-1);
    ASSERT_EQ(streamingConfig.getPort(), 7421);
    ASSERT_FALSE(SavesPort(streamingConfig));
}

TEST_F(AddConfigTransportTest, InvalidSelection)
{
    const auto config = CreateConfig();

    ASSERT_THROW(config.setTransportOption("wss"), NotFoundException);
    ASSERT_EQ(config.getSelectedTransportOption().getId(), "tls");
    ASSERT_THROW(config.setPort(-2), InvalidParameterException);
    ASSERT_EQ(config.getPort(), 7421);
    ASSERT_FALSE(SavesPort(config));
}

TEST_F(AddConfigTransportTest, WithoutTransportOptions)
{
    const auto config = CreateConfig(CreateDeviceType(TransportOptionsById({})));

    ASSERT_EQ(config.getAvailableTransportOptions().getCount(), 0u);
    ASSERT_THROW(config.setTransportOption("plain"), NotFoundException);
    ASSERT_FALSE(config.getSelectedTransportOption().assigned());
    ASSERT_FALSE(Properties(config).hasProperty("TransportOption"));

    // The port works without options: a module that does not use them gets it in the connection string
    ASSERT_EQ(config.getPort(), -1);
    config.setPort(7420);
    ASSERT_EQ(config.getPort(), 7420);
    ASSERT_TRUE(SavesPort(config));
    config.setPort(-1);
    ASSERT_EQ(config.getPort(), -1);
    ASSERT_FALSE(config.getTypeConfig().hasProperty(TransportOptionConfigProperty));
    ASSERT_EQ(config.getTypeConfig().getPropertyValue(PortConfigProperty), 7000);
}

TEST_F(AddConfigTransportTest, Clone)
{
    const auto config = CreateConfig();
    config.setTransportOption("plain");
    config.setPort(9000);
    config.getStreamingOptions().getAvailableStreamingConfigs().get("str").setTransportOption("plain");

    const PropertyObjectPtr cloned = config.asPtr<IPropertyObjectInternal>().clone();
    const auto clonedConfig = cloned.asPtrOrNull<IAddDeviceConfig, AddDeviceConfigPtr>();
    ASSERT_TRUE(clonedConfig.assigned());
    ASSERT_EQ(clonedConfig.getAvailableTransportOptions().getKeyList(), List<IString>("tls", "plain"));
    ASSERT_EQ(clonedConfig.getSelectedTransportOption().getId(), "plain");
    ASSERT_EQ(clonedConfig.getPort(), 9000);

    const auto clonedStreamingConfig = clonedConfig.getStreamingOptions().getAvailableStreamingConfigs().get("str");
    ASSERT_EQ(clonedStreamingConfig.getSelectedTransportOption().getId(), "plain");
    ASSERT_EQ(clonedStreamingConfig.getPort(), 7420);
    ASSERT_FALSE(SavesPort(clonedStreamingConfig));

    // The clone is a copy
    clonedConfig.setPort(9001);
    ASSERT_EQ(config.getPort(), 9000);
}

// The saved form keeps the selection and the port set; a bound config selects among the options of the loaded type
TEST_F(AddConfigTransportTest, BindKeepsSelection)
{
    const auto config = CreateConfig();
    config.setTransportOption("plain");
    config.getStreamingOptions().setStreamingType("str");
    config.getStreamingOptions().getStreamingConfig().setTransportOption("plain");
    config.getStreamingOptions().getStreamingConfig().setPort(9000);

    const auto bound = Bind(config);
    ASSERT_EQ(bound.getAvailableTransportOptions().getKeyList(), List<IString>("tls", "plain"));
    ASSERT_EQ(bound.getSelectedTransportOption().getId(), "plain");
    ASSERT_EQ(bound.getPort(), 7420);

    const auto boundStreaming = bound.getStreamingOptions().getStreamingConfig();
    ASSERT_EQ(boundStreaming.getSelectedTransportOption().getId(), "plain");
    ASSERT_EQ(boundStreaming.getPort(), 9000);
}

TEST_F(AddConfigTransportTest, BindKeepsPortSet)
{
    const auto config = CreateConfig();
    config.setPort(9000);

    const auto bound = Bind(config);
    ASSERT_EQ(bound.getSelectedTransportOption().getId(), "tls");
    ASSERT_EQ(bound.getPort(), 9000);
    ASSERT_TRUE(SavesPort(bound));
}

// Like any selection value, a saved option the loaded type no longer lists fails the bind
TEST_F(AddConfigTransportTest, BindRejectsUnknownSelection)
{
    const auto config = CreateConfig();
    config.setTransportOption("plain");

    ASSERT_THROW(Bind(config, CreateDeviceType(TransportOptionsById({TransportOption("wss", 7414), TransportOption("tls", 7421)}))),
                 InvalidValueException);
}

// An option not selected is not saved: the bound config selects the loaded type's first option
TEST_F(AddConfigTransportTest, BindWithoutSavedSelection)
{
    const auto config =
        CreateConfig(CreateDeviceType(TransportOptionsById({TransportOption("plain", 7420), TransportOption("tls", 7421)})));
    ASSERT_EQ(config.getSelectedTransportOption().getId(), "plain");

    const auto bound = Bind(config);
    ASSERT_EQ(bound.getSelectedTransportOption().getId(), "tls");
    ASSERT_EQ(bound.getPort(), 7421);
}

TEST_F(AddConfigTransportTest, SerializeDeserialize)
{
    const auto config = CreateConfig();
    config.setTransportOption("plain");
    config.setPort(9000);
    config.getStreamingOptions().setStreamingType("str");
    config.getStreamingOptions().getStreamingConfig().setTransportOption("plain");

    const auto serializer = JsonSerializer();
    config.serialize(serializer);
    const AddDeviceConfigPtr deserialized = JsonDeserializer().deserialize(serializer.getOutput(), nullptr, nullptr);

    ASSERT_EQ(deserialized.getAvailableTransportOptions().getKeyList(), List<IString>("tls", "plain"));
    ASSERT_EQ(deserialized.getSelectedTransportOption().getId(), "plain");
    ASSERT_EQ(deserialized.getPort(), 9000);

    const auto streamingConfig = deserialized.getStreamingOptions().getAvailableStreamingConfigs().get("str");
    ASSERT_EQ(streamingConfig.getSelectedTransportOption().getId(), "plain");
    ASSERT_EQ(streamingConfig.getPort(), 7420);
    ASSERT_FALSE(SavesPort(streamingConfig));
}

TEST_F(AddConfigTransportTest, AddStreamingConfig)
{
    const auto config = AddStreamingConfig(CreateStreamingType());

    ASSERT_EQ(config.getSelectedTransportOption().getId(), "tls");
    ASSERT_EQ(config.getPort(), 7421);

    config.setTransportOption("plain");
    config.setPort(9000);
    ASSERT_EQ(config.getSelectedTransportOption().getId(), "plain");
    ASSERT_EQ(config.getPort(), 9000);
    ASSERT_FALSE(config.getTypeConfig().hasProperty(TransportOptionConfigProperty));
    ASSERT_THROW(config.setTransportOption("wss"), NotFoundException);

    const auto withoutOptions = AddStreamingConfig(CreateStreamingType(TransportOptionsById({})));
    ASSERT_THROW(withoutOptions.setTransportOption("plain"), NotFoundException);
    ASSERT_FALSE(withoutOptions.getSelectedTransportOption().assigned());
    ASSERT_EQ(withoutOptions.getPort(), -1);
    withoutOptions.setPort(9000);
    ASSERT_EQ(withoutOptions.getPort(), 9000);
}

// The legacy translation keeps a set port in the Port of the Device and Streaming entries
class AddConfigTransportLegacyTest : public AddConfigTransportTest
{
protected:
    // A Mirrored Device type whose module has no Port
    static DeviceTypePtr DeviceTypeWithoutOwnPort()
    {
        return DeviceTypeBuilder()
            .setId("dev")
            .setName("dev")
            .setConnectionStringPrefix("dev")
            .setDeviceKind(DeviceKind::Mirrored)
            .setAvailableTransportOptions(TypeOptions())
            .build();
    }

    static PropertyObjectPtr Entry(const PropertyObjectPtr& legacy, const char* section, const char* id)
    {
        const PropertyObjectPtr sectionObj = legacy.getPropertyValue(section);
        return sectionObj.getPropertyValue(id);
    }

    // Sets the device port to 7500 and the streaming port to 7600
    static AddDeviceConfigPtr ConfigWithPortsSet(const DeviceTypePtr& deviceType)
    {
        const auto config = CreateConfig(deviceType);
        config.setPort(7500);
        config.getStreamingOptions().getAvailableStreamingConfigs().get("str").setPort(7600);
        return config;
    }

    static AddStreamingConfigPtr StreamingConfig(const AddDeviceConfigPtr& config)
    {
        return config.getStreamingOptions().getAvailableStreamingConfigs().get("str");
    }

    static PropertyObjectPtr LegacyConfig(const DeviceTypePtr& deviceType)
    {
        return createLegacyAddDeviceConfig(
            Dict<IString, IDeviceType>({{"dev", deviceType}}), Dict<IString, IStreamingType>({{"str", CreateStreamingType()}}), True);
    }

    static AddDeviceConfigPtr FromLegacy(const PropertyObjectPtr& legacy,
                                         const DeviceTypePtr& deviceType,
                                         const StreamingTypePtr& streamingType = CreateStreamingType())
    {
        return fromLegacyAddDeviceConfig(legacy,
                                         deviceType,
                                         Dict<IString, IStreamingType>({{"str", streamingType}}),
                                         True,
                                         Logger().getOrAddComponent("AddConfigTransportLegacyTest"));
    }
};

TEST_F(AddConfigTransportLegacyTest, ToLegacyCarriesPortSet)
{
    const PropertyObjectPtr legacy = toLegacyAddDeviceConfig(ConfigWithPortsSet(DeviceTypeWithoutOwnPort()));

    ASSERT_EQ(Entry(legacy, "Device", "dev").getPropertyValue(PortConfigProperty), 7500);
    ASSERT_EQ(Entry(legacy, "Streaming", "str").getPropertyValue(PortConfigProperty), 7600);
}

// The module has its own Port (7000). The legacy entry gets the set port instead.
TEST_F(AddConfigTransportLegacyTest, ToLegacyCarriesPortSetOverModulePort)
{
    const PropertyObjectPtr legacy = toLegacyAddDeviceConfig(ConfigWithPortsSet(CreateDeviceType()));

    ASSERT_EQ(Entry(legacy, "Device", "dev").getPropertyValue(PortConfigProperty), 7500);
}

// Without a set port, the entries keep the module's values.
// Writing the option port there would override the port of the connection string.
TEST_F(AddConfigTransportLegacyTest, ToLegacyWithoutPortSetKeepsTypeConfigs)
{
    const PropertyObjectPtr legacy = toLegacyAddDeviceConfig(CreateConfig(CreateDeviceType()));

    ASSERT_EQ(Entry(legacy, "Device", "dev").getPropertyValue(PortConfigProperty), 7000);
    ASSERT_FALSE(Entry(legacy, "Streaming", "str").hasProperty(PortConfigProperty));
}

// A config serialized for an older peer (serializer version 3) goes through the translation
TEST_F(AddConfigTransportLegacyTest, SerializedForOlderPeerCarriesPortSet)
{
    const auto serializer = JsonSerializerWithVersion(3);
    ConfigWithPortsSet(CreateDeviceType()).serialize(serializer);
    const PropertyObjectPtr legacy = JsonDeserializer().deserialize(serializer.getOutput());

    ASSERT_EQ(Entry(legacy, "Device", "dev").getPropertyValue(PortConfigProperty), 7500);
}

TEST_F(AddConfigTransportLegacyTest, FromLegacyTakesPortSet)
{
    const auto deviceType = CreateDeviceType();
    const auto legacy = LegacyConfig(deviceType);
    Entry(legacy, "Device", "dev").setPropertyValue(PortConfigProperty, 7500);
    Entry(legacy, "Streaming", "str").addProperty(IntProperty(PortConfigProperty, 7600));

    const auto config = FromLegacy(legacy, deviceType);

    ASSERT_EQ(config.getPort(), 7500);
    ASSERT_EQ(config.getTypeConfig().getPropertyValue(PortConfigProperty), 7500);
    ASSERT_EQ(StreamingConfig(config).getPort(), 7600);
}

// The default of the module's Port is not a set port, so the option port is used
TEST_F(AddConfigTransportLegacyTest, FromLegacyPortAtModuleDefaultIsNoPortSet)
{
    const auto deviceType = CreateDeviceType();
    const auto config = FromLegacy(LegacyConfig(deviceType), deviceType);

    ASSERT_EQ(Properties(config).getPropertyValue("Port"), -1);
    ASSERT_EQ(config.getPort(), 7421);
    ASSERT_EQ(Properties(StreamingConfig(config)).getPropertyValue("Port"), -1);
}

TEST_F(AddConfigTransportLegacyTest, LegacyRoundTripKeepsPortSet)
{
    for (const auto& deviceType : {DeviceTypeWithoutOwnPort(), CreateDeviceType()})
    {
        const auto config = FromLegacy(toLegacyAddDeviceConfig(ConfigWithPortsSet(deviceType)), deviceType);

        ASSERT_EQ(config.getPort(), 7500);
        ASSERT_EQ(StreamingConfig(config).getPort(), 7600);
    }
}

// A type without transport options keeps the legacy Port in the module's Port
TEST_F(AddConfigTransportLegacyTest, FromLegacyTypeWithoutOptionsKeepsModulePort)
{
    const auto deviceType = CreateDeviceType(Dict<IString, ITransportOption>());
    const auto legacy = LegacyConfig(deviceType);
    Entry(legacy, "Device", "dev").setPropertyValue(PortConfigProperty, 7500);

    const auto config = FromLegacy(legacy, deviceType);

    ASSERT_EQ(Properties(config).getPropertyValue("Port"), -1);
    ASSERT_EQ(config.getTypeConfig().getPropertyValue(PortConfigProperty), 7500);
}

// Without options and without a module Port, the legacy Port can only be a set port
TEST_F(AddConfigTransportLegacyTest, LegacyRoundTripKeepsPortSetWithoutOptions)
{
    const auto deviceType = DeviceTypeBuilder()
                                .setId("dev")
                                .setName("dev")
                                .setConnectionStringPrefix("dev")
                                .setDeviceKind(DeviceKind::Mirrored)
                                .build();
    const auto streamingType = CreateStreamingType(Dict<IString, ITransportOption>());
    const auto config = AddDeviceConfig(deviceType, Dict<IString, IStreamingType>({{"str", streamingType}}), nullptr, True);
    config.setPort(7500);
    StreamingConfig(config).setPort(7600);

    const auto copy = FromLegacy(toLegacyAddDeviceConfig(config), deviceType, streamingType);

    ASSERT_EQ(copy.getPort(), 7500);
    ASSERT_EQ(StreamingConfig(copy).getPort(), 7600);
}

END_NAMESPACE_OPENDAQ
