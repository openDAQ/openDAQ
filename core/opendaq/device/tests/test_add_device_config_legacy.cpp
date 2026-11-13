#include <gtest/gtest.h>
#include <opendaq/add_device_config_legacy.h>
#include <opendaq/add_component_config_factory.h>
#include <opendaq/component_type_builder_factory.h>
#include <opendaq/logger_factory.h>
#include <opendaq/logger_sink_last_message_private_ptr.h>
#include <coreobjects/property_object_factory.h>
#include <coreobjects/property_factory.h>
#include <coretypes/json_serializer_factory.h>
#include <vector>

BEGIN_NAMESPACE_OPENDAQ

class AddDeviceConfigLegacyTest : public testing::Test
{
protected:
    static constexpr const char* NativeId = "OpenDAQNativeStreaming";
    static constexpr const char* LtId = "OpenDAQLTStreaming";
    static constexpr const char* OtherId = "other";

    static PropertyObjectPtr createTypeConfig()
    {
        auto config = PropertyObject();
        config.addProperty(IntProperty("Port", 7420));
        config.addProperty(StringProperty("Username", ""));
        config.addProperty(StringProperty("Password", ""));
        config.addProperty(IntProperty("ClientType", 0));
        config.addProperty(BoolProperty("ExclusiveControlDropOthers", false));

        auto nested = PropertyObject();
        nested.addProperty(IntProperty("Value", 0));
        nested.addProperty(IntProperty("Other", 0));
        config.addProperty(ObjectProperty("Nested", nested));
        return config;
    }

    static DeviceTypePtr createDeviceType(DeviceKind kind = DeviceKind::Mirrored)
    {
        auto builder =
            DeviceTypeBuilder().setId("dev").setName("dev").setConnectionStringPrefix("dev").setDefaultConfig(createTypeConfig());
        if (kind != DeviceKind::Unspecified)
            builder.setDeviceKind(kind);
        return builder.build();
    }

    static StreamingTypePtr createStreamingType(const StringPtr& id)
    {
        return StreamingTypeBuilder().setId(id).setName(id).setConnectionStringPrefix(id).setDefaultConfig(createTypeConfig()).build();
    }

    PropertyObjectPtr createLegacyConfig() const
    {
        auto devices = PropertyObject();
        devices.addProperty(ObjectProperty("dev", createTypeConfig()));

        auto streamings = PropertyObject();
        for (const auto& [id, type] : streamingTypes)
            streamings.addProperty(ObjectProperty(id, type.createDefaultConfig()));

        auto legacy = PropertyObject();
        legacy.addProperty(ObjectProperty("Device", devices));
        legacy.addProperty(ObjectProperty("Streaming", streamings));
        legacy.addProperty(ObjectProperty("General", createLegacyGeneralConfig(True)));
        return legacy;
    }

    static PropertyObjectPtr section(const PropertyObjectPtr& legacy, const StringPtr& name)
    {
        return legacy.getPropertyValue(name);
    }

    static PropertyObjectPtr typeSection(const PropertyObjectPtr& legacy, const StringPtr& name, const StringPtr& typeId)
    {
        return section(legacy, name).getPropertyValue(typeId);
    }

    AddDeviceConfigPtr createConfig(DeviceKind kind = DeviceKind::Mirrored) const
    {
        return AddDeviceConfig(createDeviceType(kind), streamingTypes, True);
    }

    AddDeviceConfigPtr fromLegacy(const PropertyObjectPtr& legacy, DeviceKind kind = DeviceKind::Mirrored) const
    {
        return fromLegacyAddDeviceConfig(legacy, createDeviceType(kind), streamingTypes, True, loggerComponent);
    }

    static ListPtr<IString> toList(const std::vector<std::string>& ids)
    {
        auto list = List<IString>();
        for (const auto& id : ids)
            list.pushBack(id);
        return list;
    }

    static StringPtr selectedStreamingType(const AddDeviceConfigPtr& config)
    {
        return config.getStreamingOptions().asPtr<IPropertyObject>().getPropertyValue("StreamingType");
    }

    static std::string serialize(const PropertyObjectPtr& obj)
    {
        const auto serializer = JsonSerializer();
        obj.serialize(serializer);
        return serializer.getOutput();
    }

    bool logged(const std::string& part) const
    {
        const LastMessageLoggerSinkPrivatePtr lastMessage = sink;
        while (lastMessage.waitForMessage(1000))
            if (lastMessage.getLastMessage().toStdString().find(part) != std::string::npos)
                return true;
        return false;
    }

    const DictPtr<IString, IStreamingType> streamingTypes = Dict<IString, IStreamingType>(
        {{NativeId, createStreamingType(NativeId)}, {LtId, createStreamingType(LtId)}, {OtherId, createStreamingType(OtherId)}});
    LoggerSinkPtr sink = LastMessageLoggerSink();
    LoggerPtr logger = LoggerWithSinks(List<ILoggerSink>(sink));
    LoggerComponentPtr loggerComponent = logger.getOrAddComponent("AddDeviceConfigLegacy");
};

struct IdentityCase
{
    std::string name;
    DeviceKind kind;
    std::string selected;
    StreamingMode mode;
    AddressType addressType;
    Bool scanOnAdd;
    Int port;
};

void PrintTo(const IdentityCase& identityCase, std::ostream* os)
{
    *os << identityCase.name;
}

class AddDeviceConfigLegacyIdentityTest : public AddDeviceConfigLegacyTest, public testing::WithParamInterface<IdentityCase>
{
};

TEST_P(AddDeviceConfigLegacyIdentityTest, NewToOldToNew)
{
    const auto& param = GetParam();
    const auto config = createConfig(param.kind);
    config.setPrimaryAddressType(param.addressType);
    config.setScanOnAdd(param.scanOnAdd);
    config.getTypeConfig().setPropertyValue("Port", param.port);
    if (const auto options = config.getStreamingOptions(); options.assigned())
    {
        options.setStreamingType(param.selected.empty() ? nullptr : String(param.selected));
        options.setStreamingMode(param.mode);
        options.getStreamingConfig(LtId).getTypeConfig().setPropertyValue("Port", param.port);
    }

    ASSERT_EQ(serialize(fromLegacy(toLegacyAddDeviceConfig(config), param.kind)), serialize(config));
}

INSTANTIATE_TEST_SUITE_P(
    AddDeviceConfigLegacy,
    AddDeviceConfigLegacyIdentityTest,
    testing::Values(
        IdentityCase{"NoSelection", DeviceKind::Mirrored, "", StreamingMode::MinHops, AddressType::IPv4, True, 7420},
        IdentityCase{"Selected", DeviceKind::Mirrored, "OpenDAQLTStreaming", StreamingMode::MinHops, AddressType::IPv4, True, 7420},
        IdentityCase{"MinConnections", DeviceKind::Mirrored, "", StreamingMode::MinConnections, AddressType::IPv4, True, 7420},
        IdentityCase{"IPv6", DeviceKind::Mirrored, "", StreamingMode::MinHops, AddressType::IPv6, True, 7420},
        IdentityCase{"ScanOnAddOff", DeviceKind::Mirrored, "", StreamingMode::MinHops, AddressType::IPv4, False, 7420},
        IdentityCase{"ChangedTypeConfigs", DeviceKind::Mirrored, "", StreamingMode::MinHops, AddressType::IPv4, True, 1},
        IdentityCase{"Unspecified", DeviceKind::Unspecified, "OpenDAQLTStreaming", StreamingMode::MinHops, AddressType::IPv4, True, 1},
        IdentityCase{"Adapter", DeviceKind::Adapter, "", StreamingMode::MinHops, AddressType::IPv4, True, 1}),
    [](const testing::TestParamInfo<IdentityCase>& info) { return info.param.name; });

TEST_F(AddDeviceConfigLegacyTest, OldToNewToOldIsStableAfterOnePass)
{
    const auto legacy = createLegacyConfig();
    const auto general = section(legacy, "General");
    general.setPropertyValue("PrioritizedStreamingProtocols", List<IString>(LtId, NativeId));
    general.setPropertyValue("StreamingConnectionHeuristic", 2);
    general.setPropertyValue("Username", "user");

    const auto once = toLegacyAddDeviceConfig(fromLegacy(legacy));
    const auto twice = toLegacyAddDeviceConfig(fromLegacy(once));

    ASSERT_EQ(serialize(twice), serialize(once));
}

struct StreamingCase
{
    std::string name;
    std::vector<std::string> prioritized;
    std::vector<std::string> allowed;
    Int heuristic;
    bool connect;
    std::string selected;
    StreamingMode mode;
};

void PrintTo(const StreamingCase& streamingCase, std::ostream* os)
{
    *os << streamingCase.name;
}

class AddDeviceConfigLegacyStreamingTest : public AddDeviceConfigLegacyTest, public testing::WithParamInterface<StreamingCase>
{
};

TEST_P(AddDeviceConfigLegacyStreamingTest, OldToNew)
{
    const auto& param = GetParam();
    const auto legacy = createLegacyConfig();
    const auto general = section(legacy, "General");
    general.setPropertyValue("PrioritizedStreamingProtocols", toList(param.prioritized));
    general.setPropertyValue("AllowedStreamingProtocols", toList(param.allowed));
    general.setPropertyValue("StreamingConnectionHeuristic", param.heuristic);
    general.setPropertyValue("AutomaticallyConnectStreaming", param.connect);

    const auto config = fromLegacy(legacy);

    ASSERT_EQ(selectedStreamingType(config), param.selected);
    ASSERT_EQ(config.getStreamingOptions().getStreamingMode(), param.mode);
}

INSTANTIATE_TEST_SUITE_P(
    AddDeviceConfigLegacy,
    AddDeviceConfigLegacyStreamingTest,
    testing::Values(
        StreamingCase{"DefaultList",
                      {"OpenDAQNativeStreaming", "OpenDAQLTStreaming"},
                      {},
                      0,
                      true,
                      "OpenDAQNativeStreaming",
                      StreamingMode::MinConnections},
        StreamingCase{"DefaultListWithAllowedList",
                      {"OpenDAQNativeStreaming", "OpenDAQLTStreaming"},
                      {"OpenDAQLTStreaming"},
                      0,
                      true,
                      "OpenDAQLTStreaming",
                      StreamingMode::MinConnections},
        StreamingCase{"ExplicitList", {"other", "OpenDAQLTStreaming"}, {}, 0, true, "other", StreamingMode::MinConnections},
        StreamingCase{"EmptyList", {}, {}, 0, true, "None", StreamingMode::MinConnections},
        StreamingCase{"MinHops",
                      {"OpenDAQNativeStreaming", "OpenDAQLTStreaming"},
                      {},
                      1,
                      true,
                      "OpenDAQNativeStreaming",
                      StreamingMode::MinHops},
        StreamingCase{"NotConnected", {"OpenDAQLTStreaming"}, {}, 2, true, "None", StreamingMode::MinConnections},
        StreamingCase{"AutoConnectOff", {"OpenDAQLTStreaming"}, {}, 0, false, "None", StreamingMode::MinConnections}),
    [](const testing::TestParamInfo<StreamingCase>& info) { return info.param.name; });

TEST_F(AddDeviceConfigLegacyTest, UnlistedPickSelectsNoneAndWarns)
{
    const auto legacy = createLegacyConfig();
    section(legacy, "General").setPropertyValue("PrioritizedStreamingProtocols", List<IString>("unloaded", LtId));

    const auto config = fromLegacy(legacy);

    ASSERT_EQ(selectedStreamingType(config), "None");
    ASSERT_TRUE(logged(R"("unloaded")"));
}

TEST_F(AddDeviceConfigLegacyTest, UnknownPrimaryAddressTypeIsIPv4AndWarns)
{
    const auto legacy = createLegacyConfig();
    section(legacy, "General").setPropertyValue("PrimaryAddressType", "IPv5");

    ASSERT_EQ(fromLegacy(legacy).getPrimaryAddressType(), AddressType::IPv4);
    ASSERT_TRUE(logged("IPv5"));
}

TEST_F(AddDeviceConfigLegacyTest, GeneralValuesFillTypeConfigDefaults)
{
    const auto legacy = createLegacyConfig();
    const auto general = section(legacy, "General");
    general.setPropertyValue("Username", "user");
    general.setPropertyValue("Password", "pw");
    general.setPropertyValue("ClientType", 1);
    general.setPropertyValue("ExclusiveControlDropOthers", true);
    general.setPropertyValue("ScanOnAdd", false);
    typeSection(legacy, "Device", "dev").setPropertyValue("Password", "devpw");
    typeSection(legacy, "Streaming", LtId).setPropertyValue("Username", "ltuser");

    const auto config = fromLegacy(legacy);

    const auto typeConfig = config.getTypeConfig();
    ASSERT_EQ(typeConfig.getPropertyValue("Username"), "user");
    ASSERT_EQ(typeConfig.getPropertyValue("Password"), "devpw");
    ASSERT_EQ(typeConfig.getPropertyValue("ClientType"), 1);
    ASSERT_EQ(typeConfig.getPropertyValue("ExclusiveControlDropOthers"), true);

    const auto streamingConfig = config.getStreamingOptions().getStreamingConfig(LtId).getTypeConfig();
    ASSERT_EQ(streamingConfig.getPropertyValue("Username"), "ltuser");
    ASSERT_EQ(streamingConfig.getPropertyValue("Password"), "pw");

    ASSERT_FALSE(config.getScanOnAdd());
}

TEST_F(AddDeviceConfigLegacyTest, MissingDeviceSection)
{
    const auto legacy = createLegacyConfig();
    legacy.removeProperty("Device");

    ASSERT_EQ(serialize(fromLegacy(legacy).getTypeConfig()), serialize(createDeviceType().createDefaultConfig()));
}

TEST_F(AddDeviceConfigLegacyTest, MissingStreamingSectionListsLoadedTypes)
{
    const auto legacy = createLegacyConfig();
    legacy.removeProperty("Streaming");

    ASSERT_EQ(fromLegacy(legacy).getStreamingOptions().getStreamingConfigs().getKeyList(), streamingTypes.getKeyList());
}

TEST_F(AddDeviceConfigLegacyTest, MissingGeneralSection)
{
    const auto legacy = createLegacyConfig();
    legacy.removeProperty("General");

    const auto config = fromLegacyAddDeviceConfig(legacy, createDeviceType(), streamingTypes, False, loggerComponent);

    ASSERT_FALSE(config.getScanOnAdd());
    ASSERT_EQ(config.getPrimaryAddressType(), AddressType::IPv4);
    ASSERT_EQ(selectedStreamingType(config), NativeId);
    ASSERT_EQ(config.getStreamingOptions().getStreamingMode(), StreamingMode::MinConnections);
}

TEST_F(AddDeviceConfigLegacyTest, OnlyPresentStreamingSectionsAreListed)
{
    const auto legacy = createLegacyConfig();
    const auto streamings = section(legacy, "Streaming");
    streamings.removeProperty(NativeId);
    streamings.removeProperty(OtherId);

    ASSERT_EQ(fromLegacy(legacy).getStreamingOptions().getStreamingConfigs().getKeyList(), List<IString>(LtId));
}

TEST_F(AddDeviceConfigLegacyTest, ReservedStreamingTypeIdIsNotListedAndWarns)
{
    const auto legacy = createLegacyConfig();
    legacy.removeProperty("Streaming");
    section(legacy, "General").setPropertyValue("PrioritizedStreamingProtocols", List<IString>(LtId));
    const auto loadedTypes = Dict<IString, IStreamingType>({{LtId, createStreamingType(LtId)}, {"a.b", createStreamingType("a.b")}});

    const auto config = fromLegacyAddDeviceConfig(legacy, createDeviceType(), loadedTypes, True, loggerComponent);

    ASSERT_EQ(config.getStreamingOptions().getStreamingConfigs().getKeyList(), List<IString>(LtId));
    ASSERT_TRUE(logged(R"(Streaming type ID "a.b" is reserved)"));
}

TEST_F(AddDeviceConfigLegacyTest, PropertyOnlyOldSideHasIsDropped)
{
    const auto legacy = createLegacyConfig();
    typeSection(legacy, "Device", "dev").addProperty(IntProperty("Extra", 1));

    ASSERT_FALSE(fromLegacy(legacy).getTypeConfig().hasProperty("Extra"));
}

TEST_F(AddDeviceConfigLegacyTest, NestedValuesAreCopied)
{
    const auto legacy = createLegacyConfig();
    PropertyObjectPtr nested = typeSection(legacy, "Device", "dev").getPropertyValue("Nested");
    nested.setPropertyValue("Value", 1);
    nested.removeProperty("Other");

    const PropertyObjectPtr translated = fromLegacy(legacy).getTypeConfig().getPropertyValue("Nested");

    ASSERT_EQ(translated.getPropertyValue("Value"), 1);
    ASSERT_EQ(translated.getPropertyValue("Other"), 0);
}

TEST_F(AddDeviceConfigLegacyTest, RejectedValue)
{
    const auto legacy = createLegacyConfig();
    const auto deviceSection = typeSection(legacy, "Device", "dev");
    deviceSection.removeProperty("Port");
    deviceSection.addProperty(StringProperty("Port", "not a port"));

    ASSERT_THROW(fromLegacy(legacy), InvalidValueException);
}

TEST_F(AddDeviceConfigLegacyTest, NullLoggerComponent)
{
    ASSERT_THROW(fromLegacyAddDeviceConfig(createLegacyConfig(), createDeviceType(), streamingTypes, True, nullptr), ArgumentNullException);
}

TEST_F(AddDeviceConfigLegacyTest, AdapterDropsStreamingSection)
{
    const auto legacy = createLegacyConfig();
    typeSection(legacy, "Device", "dev").setPropertyValue("Port", 1);

    const auto config = fromLegacy(legacy, DeviceKind::Adapter);

    ASSERT_FALSE(config.getStreamingOptions().assigned());
    ASSERT_EQ(config.getTypeConfig().getPropertyValue("Port"), 1);
}

TEST_F(AddDeviceConfigLegacyTest, NoStreamingOptionsDoesNotConnectStreaming)
{
    const auto connects = [](DeviceKind kind)
    {
        const auto legacy = toLegacyAddDeviceConfig(AddDeviceConfig(createDeviceType(kind), nullptr, True));
        return static_cast<Bool>(section(legacy, "General").getPropertyValue("AutomaticallyConnectStreaming"));
    };

    ASSERT_FALSE(connects(DeviceKind::Mirrored));
    ASSERT_FALSE(connects(DeviceKind::Unspecified));
    ASSERT_TRUE(connects(DeviceKind::Adapter));
}

END_NAMESPACE_OPENDAQ
