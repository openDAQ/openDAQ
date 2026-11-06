#include <gtest/gtest.h>
#include <opendaq/add_component_config_factory.h>
#include <opendaq/add_device_config_private_ptr.h>
#include <opendaq/component_type_builder_factory.h>
#include <coreobjects/property_object_factory.h>
#include <coreobjects/property_factory.h>
#include <coreobjects/property_object_internal_ptr.h>
#include <coretypes/json_serializer_factory.h>
#include <coretypes/json_deserializer_factory.h>
#include <coretypes/type_manager_factory.h>

BEGIN_NAMESPACE_OPENDAQ

class AddDeviceConfigTest : public testing::Test
{
protected:
    static DeviceTypePtr createDeviceType(DeviceKind kind, Int port = 7420)
    {
        auto defaultConfig = PropertyObject();
        defaultConfig.addProperty(IntProperty("Port", port));
        auto builder = DeviceTypeBuilder().setId("dev").setName("dev").setConnectionStringPrefix("dev").setDefaultConfig(defaultConfig);
        if (kind != DeviceKind::Unspecified)
            builder.setDeviceKind(kind);
        return builder.build();
    }

    static StreamingTypePtr createStreamingType(const StringPtr& id,
                                                const StringPtr& linkedDeviceTypeId = "",
                                                const StringPtr& defaultId = nullptr)
    {
        auto defaultConfig = PropertyObject();
        defaultConfig.addProperty(StringProperty("Id", defaultId.assigned() ? defaultId : id));

        auto builder = StreamingTypeBuilder().setId(id).setName(id).setConnectionStringPrefix(id);
        if (linkedDeviceTypeId.getLength() == 0)
            return builder.setDefaultConfig(defaultConfig).build();

        defaultConfig.addProperty(IntProperty("Port", 7420));
        const auto linkedDeviceType = DeviceTypeBuilder()
                                          .setId(linkedDeviceTypeId)
                                          .setConnectionStringPrefix(linkedDeviceTypeId)
                                          .setDefaultConfig(defaultConfig)
                                          .build();
        return builder.setLinkedDeviceType(linkedDeviceType).build();
    }

    static AddDeviceConfigPtr createLinkedConfig()
    {
        auto streamingTypes = Dict<IString, IStreamingType>();
        streamingTypes.set("other", createStreamingType("other", "otherDev"));
        streamingTypes.set("linked", createStreamingType("linked", "dev"));
        streamingTypes.set("linked2", createStreamingType("linked2", "dev"));
        return AddDeviceConfig(createDeviceType(DeviceKind::Mirrored), streamingTypes, true);
    }

    static AddDeviceConfigPtr createMirroredConfig()
    {
        auto streamingTypes = Dict<IString, IStreamingType>();
        streamingTypes.set("b", createStreamingType("b"));
        streamingTypes.set("a", createStreamingType("a"));
        return AddDeviceConfig(createDeviceType(DeviceKind::Mirrored), streamingTypes, true);
    }

    static std::string serialize(const PropertyObjectPtr& obj)
    {
        const auto serializer = JsonSerializer();
        obj.serialize(serializer);
        return serializer.getOutput();
    }

    static std::string serializeForUpdate(const PropertyObjectPtr& obj)
    {
        const auto serializer = JsonSerializer();
        obj.asPtr<IUpdatable>().serializeForUpdate(serializer);
        return serializer.getOutput();
    }

    static AddDeviceConfigPtr bindSaved(const std::string& json,
                                        const DictPtr<IString, IDeviceType>& deviceTypes,
                                        const DictPtr<IString, IStreamingType>& streamingTypes)
    {
        AddDeviceConfigPtr bound;
        JsonDeserializer().callCustomProc(
            [&](const SerializedObjectPtr& saved) { bound = BoundAddDeviceConfig(saved, deviceTypes, streamingTypes); }, json);
        return bound;
    }

    static BaseObjectPtr deserialize(const std::string& json, const BaseObjectPtr& context = nullptr)
    {
        return JsonDeserializer().deserialize(json, context, nullptr);
    }

    static BaseObjectPtr roundTrip(const PropertyObjectPtr& obj)
    {
        const auto json = serialize(obj);
        const auto deserialized = deserialize(json, TypeManager());
        EXPECT_EQ(serialize(deserialized), json);
        return deserialized;
    }
};

TEST_F(AddDeviceConfigTest, Defaults)
{
    const auto deviceType = createDeviceType(DeviceKind::Adapter);
    const auto streamingTypes = Dict<IString, IStreamingType>({{"a", createStreamingType("a")}});
    const auto config = AddDeviceConfig(deviceType, streamingTypes, false);

    ASSERT_EQ(config.getDeviceType(), deviceType);
    ASSERT_EQ(config.asPtr<IPropertyObject>().getPropertyValue("DeviceType"), "dev");
    ASSERT_TRUE(config.asPtr<IPropertyObject>().getProperty("DeviceType").getReadOnly());
    ASSERT_EQ(config.getTypeConfig().getPropertyValue("Port"), 7420);
    ASSERT_EQ(config.getPrimaryAddressType(), AddressType::IPv4);
    ASSERT_FALSE(config.getScanOnAdd());
    ASSERT_EQ(config.asPtr<IPropertyObject>().getProperty("ScanOnAdd").getDefaultValue(), false);

    ASSERT_FALSE(config.asPtr<IPropertyObject>().hasProperty("Streaming"));
    ASSERT_FALSE(config.getStreamingOptions().assigned());
}

TEST_F(AddDeviceConfigTest, StreamingDefaults)
{
    const auto config = createMirroredConfig();
    const auto options = config.getStreamingOptions();

    ASSERT_EQ(options.getStreamingMode(), StreamingMode::MinHops);
    ASSERT_FALSE(options.getStreamingConfig().assigned());
    ASSERT_EQ(options.asPtr<IPropertyObject>().getPropertyValue("StreamingType"), "");
    ASSERT_EQ(options.asPtr<IPropertyObject>().getProperty("StreamingType").getSelectionValues(), List<IString>("", "b", "a"));

    const auto configs = options.getStreamingConfigs();
    ASSERT_EQ(configs.getKeyList(), List<IString>("b", "a"));
    ASSERT_EQ(configs.get("a").getStreamingType().getId(), "a");
    ASSERT_EQ(configs.get("a").getTypeConfig().getPropertyValue("Id"), "a");
}

TEST_F(AddDeviceConfigTest, UnspecifiedAndUnknownKindsHaveStreamingOptions)
{
    const DeviceTypePtr unknownType = deserialize(R"({"__type":"DeviceType","id":"dev","name":"dev","prefix":"dev","deviceKind":99})");
    ASSERT_EQ(static_cast<Int>(unknownType.getDeviceKind()), 99);

    const auto streamingTypes = Dict<IString, IStreamingType>({{"a", createStreamingType("a")}});
    for (const auto& deviceType : {createDeviceType(DeviceKind::Unspecified), unknownType})
    {
        const auto config = AddDeviceConfig(deviceType, streamingTypes, true);
        ASSERT_EQ(config.getStreamingOptions().getStreamingConfigs().getKeyList(), List<IString>("a"));
    }
}

TEST_F(AddDeviceConfigTest, MirroredWithoutStreamingTypes)
{
    const auto options = AddDeviceConfig(createDeviceType(DeviceKind::Mirrored), nullptr, true).getStreamingOptions();

    ASSERT_TRUE(options.assigned());
    ASSERT_EQ(options.getStreamingConfigs().getCount(), 0u);
    ASSERT_EQ(options.asPtr<IPropertyObject>().getProperty("StreamingType").getSelectionValues(), List<IString>(""));
    ASSERT_THROW(options.setStreamingType("a"), NotFoundException);
}

TEST_F(AddDeviceConfigTest, PropertyViewMatchesTypedGetters)
{
    const auto config = createMirroredConfig();
    const auto options = config.getStreamingOptions();
    const auto streamingConfig = options.getStreamingConfigs().get("a");
    const auto configObject = config.asPtr<IPropertyObject>();
    const auto optionsObject = options.asPtr<IPropertyObject>();

    ASSERT_EQ(configObject.getPropertyValue("TypeConfig").getObject(), config.getTypeConfig().getObject());
    ASSERT_EQ(configObject.getPropertyValue("Streaming").getObject(), optionsObject.getObject());
    ASSERT_EQ(optionsObject.getPropertyValue("a").getObject(), streamingConfig.asPtr<IPropertyObject>().getObject());
    ASSERT_EQ(streamingConfig.asPtr<IPropertyObject>().getPropertyValue("TypeConfig").getObject(),
              streamingConfig.getTypeConfig().getObject());

    config.setPrimaryAddressType(AddressType::IPv6);
    ASSERT_EQ(configObject.getPropertyValue("PrimaryAddressType"), static_cast<Int>(AddressType::IPv6));
    configObject.setPropertyValue("ScanOnAdd", false);
    ASSERT_FALSE(config.getScanOnAdd());
    optionsObject.setPropertyValue("StreamingMode", static_cast<Int>(StreamingMode::Disabled));
    ASSERT_EQ(options.getStreamingMode(), StreamingMode::Disabled);
}

TEST_F(AddDeviceConfigTest, EnumSelectionValues)
{
    const auto config = createMirroredConfig();
    const auto options = config.getStreamingOptions();
    ASSERT_EQ(config.asPtr<IPropertyObject>().getProperty("PrimaryAddressType").getSelectionValues(), List<IString>("IPv4", "IPv6"));
    ASSERT_EQ(options.asPtr<IPropertyObject>().getProperty("StreamingMode").getSelectionValues(),
              List<IString>("Disabled", "MinHops", "MinConnections"));

    for (const auto type : {AddressType::IPv4, AddressType::IPv6})
    {
        config.setPrimaryAddressType(type);
        ASSERT_EQ(config.getPrimaryAddressType(), type);
    }

    for (const auto mode : {StreamingMode::Disabled, StreamingMode::MinHops, StreamingMode::MinConnections})
    {
        options.setStreamingMode(mode);
        ASSERT_EQ(options.getStreamingMode(), mode);
    }
}

TEST_F(AddDeviceConfigTest, UnlistableStreamingTypeIdsAreNotListed)
{
    auto streamingTypes = Dict<IString, IStreamingType>();
    for (const auto& id : {"", "a.b", "a'b", "StreamingType", "StreamingMode", "a"})
        streamingTypes.set(id, createStreamingType("a"));

    const auto options = AddDeviceConfig(createDeviceType(DeviceKind::Mirrored), streamingTypes, true).getStreamingOptions();
    ASSERT_EQ(options.getStreamingConfigs().getKeyList(), List<IString>("a"));
    ASSERT_EQ(options.asPtr<IPropertyObject>().getProperty("StreamingType").getSelectionValues(), List<IString>("", "a"));
    ASSERT_EQ(options.getStreamingMode(), StreamingMode::MinHops);
}

TEST_F(AddDeviceConfigTest, OnlySelectedStreamingConfigIsVisible)
{
    const auto options = createMirroredConfig().getStreamingOptions();
    const auto optionsObject = options.asPtr<IPropertyObject>();
    const auto visibleNames = [&]
    {
        std::vector<std::string> names;
        for (const auto& property : optionsObject.getVisibleProperties())
            names.push_back(property.getName());
        return names;
    };

    ASSERT_EQ(visibleNames(), (std::vector<std::string>{"StreamingType", "StreamingMode"}));

    options.setStreamingType("a");
    ASSERT_EQ(visibleNames(), (std::vector<std::string>{"StreamingType", "StreamingMode", "a"}));
}

TEST_F(AddDeviceConfigTest, StreamingSelection)
{
    const auto options = createMirroredConfig().getStreamingOptions();
    const auto configs = options.getStreamingConfigs();
    ASSERT_EQ(options.getStreamingConfig("a").getObject(), configs.get("a").getObject());
    ASSERT_THROW(options.getStreamingConfig("unknown"), NotFoundException);

    options.setStreamingType("a");
    ASSERT_EQ(options.getStreamingConfig().getObject(), configs.get("a").getObject());
    ASSERT_EQ(options.asPtr<IPropertyObject>().getPropertyValue("StreamingType"), "a");

    options.asPtr<IPropertyObject>().setPropertyValue("StreamingType", "b");
    ASSERT_EQ(options.getStreamingConfig().getObject(), configs.get("b").getObject());

    ASSERT_THROW(options.setStreamingType("unknown"), NotFoundException);
    ASSERT_EQ(options.asPtr<IPropertyObject>().getPropertyValue("StreamingType"), "b");

    options.setStreamingType(nullptr);
    ASSERT_FALSE(options.getStreamingConfig().assigned());
    ASSERT_EQ(options.asPtr<IPropertyObject>().getPropertyValue("StreamingType"), "");
}

TEST_F(AddDeviceConfigTest, Clone)
{
    const auto config = createLinkedConfig();
    config.getStreamingOptions().setStreamingType("other");
    config.getStreamingOptions().getStreamingConfigs().get("linked").getTypeConfig().setPropertyValue("Id", "changed");

    const AddDeviceConfigPtr cloned = config.asPtr<IPropertyObjectInternal>().clone().asPtr<IAddDeviceConfig>();
    ASSERT_EQ(cloned.getDeviceType(), config.getDeviceType());
    ASSERT_EQ(cloned.getStreamingOptions().getStreamingConfigs().get("linked").getTypeConfig().getPropertyValue("Id"), "changed");

    cloned.getTypeConfig().setPropertyValue("Port", 1);
    ASSERT_EQ(config.getTypeConfig().getPropertyValue("Port"), 7420);

    const auto options = cloned.getStreamingOptions();
    ASSERT_NE(options.getObject(), config.getStreamingOptions().getObject());
    ASSERT_EQ(options.getStreamingConfigs().getKeyList(), List<IString>("other", "linked", "linked2"));

    const auto streamingConfig = options.getStreamingConfig();
    ASSERT_EQ(streamingConfig.getStreamingType().getId(), "other");
    ASSERT_NE(streamingConfig.getObject(), config.getStreamingOptions().getStreamingConfig().getObject());

    streamingConfig.getTypeConfig().setPropertyValue("Id", "changed");
    ASSERT_EQ(config.getStreamingOptions().getStreamingConfig().getTypeConfig().getPropertyValue("Id"), "other");
}

TEST_F(AddDeviceConfigTest, LinkedStreamingConfigHoldsWholeDefaultConfig)
{
    const auto config = createLinkedConfig();
    config.getTypeConfig().setPropertyValue("Port", 1);

    const auto typeConfig = config.getStreamingOptions().getStreamingConfigs().get("linked").getTypeConfig();
    ASSERT_EQ(typeConfig.getAllProperties().getCount(), 2u);
    ASSERT_EQ(typeConfig.getPropertyValue("Id"), "linked");
    ASSERT_EQ(typeConfig.getPropertyValue("Port"), 7420);
}

TEST_F(AddDeviceConfigTest, WireFormRoundTrip)
{
    const auto config = createLinkedConfig();
    config.getTypeConfig().setPropertyValue("Port", 1);
    config.setPrimaryAddressType(AddressType::IPv6);
    config.setScanOnAdd(false);
    const auto options = config.getStreamingOptions();
    options.setStreamingType("other");
    options.setStreamingMode(StreamingMode::MinConnections);
    options.getStreamingConfig().getTypeConfig().setPropertyValue("Id", "changed");

    const AddDeviceConfigPtr deserialized = roundTrip(config).asPtr<IAddDeviceConfig>();
    ASSERT_EQ(deserialized.getTypeConfig().getPropertyValue("Port"), 1);
    const auto deserializedOptions = deserialized.getStreamingOptions();
    ASSERT_EQ(deserializedOptions.getStreamingConfigs().getKeyList(), options.getStreamingConfigs().getKeyList());
    ASSERT_EQ(deserializedOptions.getStreamingConfig().getTypeConfig().getPropertyValue("Id"), "changed");

    deserialized.asPtr<IPropertyObject>().clearPropertyValue("TypeConfig");
    deserialized.asPtr<IPropertyObject>().clearPropertyValue("ScanOnAdd");
    ASSERT_EQ(deserialized.getTypeConfig().getPropertyValue("Port"), 7420);
    ASSERT_TRUE(deserialized.getScanOnAdd());
}

TEST_F(AddDeviceConfigTest, WireFormWithoutDeviceType)
{
    ASSERT_THROW(deserialize(R"({"__type":"AddDeviceConfig"})"), NotFoundException);
}

TEST_F(AddDeviceConfigTest, WireFormRoundTripNotMirrored)
{
    const auto config = AddDeviceConfig(createDeviceType(DeviceKind::Adapter), nullptr, false);

    const AddDeviceConfigPtr deserialized = roundTrip(config).asPtr<IAddDeviceConfig>();
    ASSERT_FALSE(deserialized.getStreamingOptions().assigned());
}

TEST_F(AddDeviceConfigTest, StandaloneStreamingConfig)
{
    const auto streamingType = createStreamingType("a", "dev");
    const auto standalone = AddStreamingConfig(streamingType);
    ASSERT_EQ(standalone.getStreamingType(), streamingType);
    ASSERT_EQ(standalone.getTypeConfig().getPropertyValue("Id"), "a");
}

TEST_F(AddDeviceConfigTest, StreamingObjectsWireFormRoundTrip)
{
    const auto standalone = AddStreamingConfig(createStreamingType("a", "dev"));
    standalone.getTypeConfig().setPropertyValue("Id", "changed");
    const auto config = createLinkedConfig();
    const auto options = config.getStreamingOptions();
    options.setStreamingType("linked");
    const auto linked = options.getStreamingConfig();
    linked.getTypeConfig().setPropertyValue("Id", "changed");

    const AddStreamingOptionsPtr deserializedOptions = roundTrip(options).asPtr<IAddStreamingOptions>();
    ASSERT_EQ(deserializedOptions.getStreamingConfigs().getKeyList(), List<IString>("other", "linked", "linked2"));
    ASSERT_EQ(deserializedOptions.getStreamingConfig().getTypeConfig().getPropertyValue("Id"), "changed");

    for (const auto& streamingConfig : {standalone, linked})
    {
        const AddStreamingConfigPtr deserialized = roundTrip(streamingConfig).asPtr<IAddStreamingConfig>();
        ASSERT_EQ(deserialized.getTypeConfig().getPropertyValue("Id"), "changed");
    }
}

TEST_F(AddDeviceConfigTest, SaveForm)
{
    const auto config = createMirroredConfig();
    config.getTypeConfig().setPropertyValue("Port", 1);
    config.getStreamingOptions().getStreamingConfigs().get("b").getTypeConfig().setPropertyValue("Id", "changed");

    ASSERT_EQ(serializeForUpdate(config),
              R"({"__type":"AddDeviceConfig","deviceTypeId":"dev","scanOnAdd":true,"propValues":{)"
              R"("Streaming":{"__type":"AddStreamingOptions","propValues":{)"
              R"("a":{"__type":"AddStreamingConfig","propValues":{"TypeConfig":{"__type":"PropertyObject"}}},)"
              R"("b":{"__type":"AddStreamingConfig","propValues":{"TypeConfig":)"
              R"({"__type":"PropertyObject","propValues":{"Id":"changed"}}}}}},)"
              R"("TypeConfig":{"__type":"PropertyObject","propValues":{"Port":1}}}})");
}

TEST_F(AddDeviceConfigTest, SaveFormIsNotReadable)
{
    ASSERT_THROW(deserialize(serializeForUpdate(createMirroredConfig())), NotFoundException);
}

TEST_F(AddDeviceConfigTest, BindSaveForm)
{
    const auto config = createMirroredConfig();
    config.setPrimaryAddressType(AddressType::IPv6);
    config.setScanOnAdd(false);
    const auto options = config.getStreamingOptions();
    options.setStreamingType("a");
    options.setStreamingMode(StreamingMode::MinConnections);
    options.getStreamingConfig().getTypeConfig().setPropertyValue("Id", "changed");

    const auto deviceType = createDeviceType(DeviceKind::Mirrored, 7000);
    auto streamingTypes = Dict<IString, IStreamingType>();
    streamingTypes.set("a", createStreamingType("a"));
    streamingTypes.set("b", createStreamingType("b", "", "newDefault"));

    const auto bound = bindSaved(serializeForUpdate(config), Dict<IString, IDeviceType>({{"dev", deviceType}}), streamingTypes);
    ASSERT_EQ(bound.getDeviceType().getObject(), deviceType.getObject());
    ASSERT_EQ(bound.getTypeConfig().getPropertyValue("Port"), 7000);
    ASSERT_EQ(bound.getPrimaryAddressType(), AddressType::IPv6);
    ASSERT_FALSE(bound.getScanOnAdd());
    ASSERT_EQ(bound.asPtr<IPropertyObject>().getProperty("ScanOnAdd").getDefaultValue(), false);

    const auto boundOptions = bound.getStreamingOptions();
    ASSERT_EQ(boundOptions.getStreamingMode(), StreamingMode::MinConnections);
    ASSERT_EQ(boundOptions.getStreamingConfigs().getKeyList(), List<IString>("a", "b"));
    ASSERT_EQ(boundOptions.getStreamingConfig().getStreamingType().getObject(), streamingTypes.get("a").getObject());
    ASSERT_EQ(boundOptions.getStreamingConfig().getTypeConfig().getPropertyValue("Id"), "changed");
    ASSERT_EQ(boundOptions.getStreamingConfigs().get("b").getTypeConfig().getPropertyValue("Id"), "newDefault");
}

TEST_F(AddDeviceConfigTest, BindDeserializedWireForm)
{
    const auto config = createMirroredConfig();
    config.getStreamingOptions().setStreamingType("a");
    config.getStreamingOptions().getStreamingConfig().getTypeConfig().setPropertyValue("Id", "changed");

    const auto deviceType = createDeviceType(DeviceKind::Mirrored);
    const auto streamingTypes = Dict<IString, IStreamingType>({{"a", createStreamingType("a")}, {"b", createStreamingType("b")}});

    const AddDeviceConfigPtr deserialized = deserialize(serialize(config)).asPtr<IAddDeviceConfig>();
    const auto bound =
        deserialized.asPtr<IAddDeviceConfigPrivate>().bind(Dict<IString, IDeviceType>({{"dev", deviceType}}), streamingTypes);
    ASSERT_EQ(bound.getDeviceType().getObject(), deviceType.getObject());

    const auto boundOptions = bound.getStreamingOptions();
    ASSERT_EQ(boundOptions.getStreamingConfig().getStreamingType().getObject(), streamingTypes.get("a").getObject());
    ASSERT_EQ(boundOptions.getStreamingConfig().getTypeConfig().getPropertyValue("Id"), "changed");
}

TEST_F(AddDeviceConfigTest, BindUnknownDeviceType)
{
    const auto config = createMirroredConfig();
    const auto deviceTypes = Dict<IString, IDeviceType>({{"otherDev", createDeviceType(DeviceKind::Mirrored)}});

    ASSERT_THROW(config.asPtr<IAddDeviceConfigPrivate>().bind(deviceTypes, nullptr), NotFoundException);
}

TEST_F(AddDeviceConfigTest, BindDeviceKindChange)
{
    const auto types = [](DeviceKind kind)
    {
        return Dict<IString, IDeviceType>({{"dev", createDeviceType(kind)}});
    };
    const auto mirroredSave = serializeForUpdate(createMirroredConfig());
    const auto adapterSave = serializeForUpdate(AddDeviceConfig(createDeviceType(DeviceKind::Adapter), nullptr, true));

    ASSERT_FALSE(bindSaved(mirroredSave, types(DeviceKind::Adapter), nullptr).getStreamingOptions().assigned());
    ASSERT_FALSE(bindSaved(mirroredSave, types(DeviceKind::Root), nullptr).getStreamingOptions().assigned());
    ASSERT_TRUE(bindSaved(mirroredSave, types(DeviceKind::Unspecified), nullptr).getStreamingOptions().assigned());
    ASSERT_THROW(bindSaved(adapterSave, types(DeviceKind::Mirrored), nullptr), InvalidValueException);
    ASSERT_THROW(bindSaved(adapterSave, types(DeviceKind::Unspecified), nullptr), InvalidValueException);
}

TEST_F(AddDeviceConfigTest, BindMalformedSaveForm)
{
    const auto deviceTypes = Dict<IString, IDeviceType>({{"dev", createDeviceType(DeviceKind::Mirrored)}});
    const auto config = createMirroredConfig();
    config.getStreamingOptions().setStreamingMode(StreamingMode::MinConnections);
    const auto valid = serializeForUpdate(config);

    const auto replaced = [&valid](const std::string& from, const std::string& to)
    {
        auto json = valid;
        const auto pos = json.find(from);
        EXPECT_NE(pos, std::string::npos);
        return json.replace(pos, from.size(), to);
    };

    ASSERT_NO_THROW(bindSaved(valid, deviceTypes, nullptr));
    ASSERT_THROW(bindSaved(replaced(R"("scanOnAdd":true)", R"("scanOnAdd":"yes")"), deviceTypes, nullptr), InvalidValueException);
    ASSERT_THROW(bindSaved(replaced(R"("scanOnAdd":true,)", ""), deviceTypes, nullptr), InvalidValueException);
    ASSERT_THROW(bindSaved(replaced(R"("StreamingMode":2)", R"("StreamingMode":99)"), deviceTypes, nullptr), InvalidValueException);
}

TEST_F(AddDeviceConfigTest, BindDropsUnknownStreamingType)
{
    const auto config = createMirroredConfig();
    config.getStreamingOptions().setStreamingType("a");

    const auto deviceTypes = Dict<IString, IDeviceType>({{"dev", createDeviceType(DeviceKind::Mirrored)}});
    const auto streamingTypes = Dict<IString, IStreamingType>({{"b", createStreamingType("b")}, {"c", createStreamingType("c")}});

    const auto options = config.asPtr<IAddDeviceConfigPrivate>().bind(deviceTypes, streamingTypes).getStreamingOptions();
    ASSERT_EQ(options.getStreamingConfigs().getKeyList(), List<IString>("b"));
    ASSERT_EQ(options.asPtr<IPropertyObject>().getProperty("StreamingType").getSelectionValues(), List<IString>("", "b"));
    ASSERT_FALSE(options.getStreamingConfig().assigned());
}

END_NAMESPACE_OPENDAQ
