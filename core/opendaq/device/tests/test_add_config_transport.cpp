#include <gtest/gtest.h>
#include <opendaq/add_component_config_factory.h>
#include <opendaq/add_device_config_private_ptr.h>
#include <opendaq/add_streaming_options_ptr.h>
#include <opendaq/component_type_builder_factory.h>
#include <opendaq/device_info_factory.h>
#include <opendaq/transport_option_factory.h>
#include <opendaq/transport_option_impl.h>
#include <testutils/testutils.h>
#include <coreobjects/property_object_factory.h>
#include <coreobjects/property_factory.h>
#include <coreobjects/property_object_internal_ptr.h>
#include <coretypes/json_serializer_factory.h>
#include <coretypes/updatable_ptr.h>
#include <coretypes/json_deserializer_factory.h>

BEGIN_NAMESPACE_OPENDAQ

// The transport selection of the add-device and add-streaming configs
class AddConfigTransportTest : public testing::Test
{
protected:
    static DictPtr<IString, ITransportOption> TypeOptions()
    {
        return TransportOptionsById({TransportOption("tls", 7421, TransportProtection::Encrypted), TransportOption("plain", 7420)});
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
        return AddDeviceConfig(deviceType, Dict<IString, IStreamingType>({{"str", CreateStreamingType()}}), True);
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
    ASSERT_EQ(properties.getPropertyValue("Port"), 7421);
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

    // Unlike setTransportOption, writing the property keeps the port set
    properties.setPropertyValue("TransportOption", "tls");
    ASSERT_EQ(config.getPort(), 9000);

    config.setTransportOption("plain");
    ASSERT_EQ(properties.getPropertyValue("TransportOption"), "plain");
    ASSERT_EQ(properties.getPropertyValue("Port"), 7420);

    ASSERT_ANY_THROW(properties.setPropertyValue("TransportOption", "wss"));
    ASSERT_EQ(config.getSelectedTransportOption().getId(), "plain");
}

TEST_F(AddConfigTransportTest, SetTransportOptionDropsPortSetBefore)
{
    const auto config = CreateConfig();

    config.setPort(9000);
    ASSERT_EQ(config.getPort(), 9000);
    ASSERT_TRUE(SavesPort(config));

    config.setTransportOption("plain");
    ASSERT_EQ(config.getSelectedTransportOption().getId(), "plain");
    ASSERT_EQ(config.getPort(), 7420);
    ASSERT_FALSE(SavesPort(config));

    config.setPort(9000);
    ASSERT_EQ(config.getPort(), 9000);
    ASSERT_EQ(config.getSelectedTransportOption().getPort(), 7420);

    // The type config is not touched
    ASSERT_FALSE(config.getTypeConfig().hasProperty(TransportOptionConfigProperty));
    ASSERT_EQ(config.getTypeConfig().getPropertyValue(PortConfigProperty), 7000);
}

TEST_F(AddConfigTransportTest, InvalidSelection)
{
    const auto config = CreateConfig();

    ASSERT_THROW(config.setTransportOption("wss"), NotFoundException);
    ASSERT_EQ(config.getSelectedTransportOption().getId(), "tls");
    ASSERT_THROW(config.setPort(-1), InvalidParameterException);
    ASSERT_EQ(config.getPort(), 7421);
    ASSERT_FALSE(SavesPort(config));
}

TEST_F(AddConfigTransportTest, WithoutTransportOptions)
{
    const auto config = CreateConfig(CreateDeviceType(TransportOptionsById({})));

    ASSERT_EQ(config.getAvailableTransportOptions().getCount(), 0u);
    ASSERT_THROW(config.setTransportOption("plain"), NotSupportedException);
    ASSERT_THROW(config.getSelectedTransportOption(), NotSupportedException);
    ASSERT_THROW(config.setPort(7420), NotSupportedException);
    ASSERT_THROW(config.getPort(), NotSupportedException);
    ASSERT_FALSE(Properties(config).hasProperty("TransportOption"));
    ASSERT_FALSE(Properties(config).hasProperty("Port"));
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
    ASSERT_THROW(withoutOptions.setTransportOption("plain"), NotSupportedException);
    ASSERT_THROW(withoutOptions.getPort(), NotSupportedException);
}

END_NAMESPACE_OPENDAQ
