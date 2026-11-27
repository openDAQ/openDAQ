#include <gtest/gtest.h>
#include <opendaq/add_device_config_legacy.h>
#include <opendaq/add_device_config_private_ptr.h>
#include <opendaq/add_component_config_factory.h>
#include <opendaq/component_type_builder_factory.h>
#include <opendaq/authentication_method_factory.h>
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

    static StreamingTypePtr createAuthenticatedStreamingType(const StringPtr& id, bool userNamePassword = true)
    {
        auto builder = StreamingTypeBuilder()
                           .setId(id)
                           .setName(id)
                           .setConnectionStringPrefix(id)
                           .setDefaultConfig(createTypeConfig())
                           .addAuthenticationMethod(StandardAnonymousAuthenticationMethod());
        if (userNamePassword)
            builder.addAuthenticationMethod(StandardUserNamePasswordAuthenticationMethod());
        return builder.build();
    }

    static DeviceTypePtr createAuthenticatedDeviceType()
    {
        return DeviceTypeBuilder()
            .setId("dev")
            .setName("dev")
            .setConnectionStringPrefix("dev")
            .setDeviceKind(DeviceKind::Mirrored)
            .setDefaultConfig(createTypeConfig())
            .addAuthenticationMethod(StandardAnonymousAuthenticationMethod())
            .addAuthenticationMethod(StandardUserNamePasswordAuthenticationMethod())
            .build();
    }

    static DictPtr<IString, IString> credential(const StringPtr& username, const StringPtr& password)
    {
        return Dict<IString, IString>({{"UserName", username}, {"Password", password}});
    }

    static void assertCredential(const AuthenticationConfigPtr& authenticationConfig, const DictPtr<IString, IString>& expected)
    {
        ASSERT_EQ(authenticationConfig.getSelectedAuthenticationMethod().getId(), StandardUserNamePasswordId);
        ASSERT_EQ(authenticationConfig.getSuppliedCredential(), expected);
    }

    static void assertNoCredential(const AuthenticationConfigPtr& authenticationConfig)
    {
        ASSERT_EQ(authenticationConfig.getSelectedAuthenticationMethod().getId(), StandardAnonymousId);
        ASSERT_EQ(authenticationConfig.getSuppliedCredential().getCount(), 0u);
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
        return AddDeviceConfig(createDeviceType(kind), streamingTypes, nullptr, True);
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
        options.getAvailableStreamingConfigs().get(LtId).getTypeConfig().setPropertyValue("Port", param.port);
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
        StreamingCase{"DefaultList", {"OpenDAQNativeStreaming"}, {}, 0, true, "OpenDAQNativeStreaming", StreamingMode::MinConnections},
        StreamingCase{"DefaultListWithAllowedList",
                      {"OpenDAQNativeStreaming"},
                      {"OpenDAQLTStreaming"},
                      0,
                      true,
                      "OpenDAQLTStreaming",
                      StreamingMode::MinConnections},
        StreamingCase{"ExplicitList", {"other", "OpenDAQLTStreaming"}, {}, 0, true, "other", StreamingMode::MinConnections},
        StreamingCase{"EmptyList", {}, {}, 0, true, "OpenDAQNativeStreaming", StreamingMode::MinConnections},
        StreamingCase{"UnlistedProtocolIsSkipped",
                      {"unloaded", "OpenDAQLTStreaming"},
                      {},
                      0,
                      true,
                      "OpenDAQLTStreaming",
                      StreamingMode::MinConnections},
        StreamingCase{"NothingAllowedIsLoaded", {"OpenDAQNativeStreaming"}, {"unloaded"}, 0, true, "None", StreamingMode::MinConnections},
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

    const auto streamingConfig = config.getStreamingOptions().getAvailableStreamingConfigs().get(LtId).getTypeConfig();
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

    ASSERT_EQ(fromLegacy(legacy).getStreamingOptions().getAvailableStreamingConfigs().getKeyList(), streamingTypes.getKeyList());
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

TEST_F(AddDeviceConfigLegacyTest, FlatConfig)
{
    const auto flat = createLegacyGeneralConfig(True);
    flat.setPropertyValue("PrioritizedStreamingProtocols", List<IString>(LtId));
    flat.addProperty(IntProperty("Port", 1));

    const auto config = fromLegacy(flat);

    ASSERT_EQ(config.getTypeConfig().getPropertyValue("Port"), 1);
    ASSERT_EQ(selectedStreamingType(config), LtId);
    ASSERT_EQ(config.getStreamingOptions().getAvailableStreamingConfigs().getKeyList(), streamingTypes.getKeyList());
}

TEST_F(AddDeviceConfigLegacyTest, BareTypeConfig)
{
    const auto bare = createTypeConfig();
    bare.setPropertyValue("Port", 1);
    bare.setPropertyValue("Username", "user");

    const auto config = fromLegacy(bare);

    ASSERT_EQ(config.getTypeConfig().getPropertyValue("Port"), 1);
    ASSERT_EQ(config.getStreamingOptions().getAvailableStreamingConfigs().get(LtId).getTypeConfig().getPropertyValue("Username"), "user");
}

TEST_F(AddDeviceConfigLegacyTest, DefaultPrioritizedProtocolIsNative)
{
    const ListPtr<IString> prioritized = createLegacyGeneralConfig(True).getPropertyValue("PrioritizedStreamingProtocols");
    ASSERT_EQ(prioritized, List<IString>(NativeId));
}

TEST_F(AddDeviceConfigLegacyTest, NullConfigIsTheDefaultLegacyConfig)
{
    ASSERT_EQ(serialize(fromLegacy(nullptr)), serialize(fromLegacy(createLegacyConfig())));
}

TEST_F(AddDeviceConfigLegacyTest, OnlyPresentStreamingSectionsAreListed)
{
    const auto legacy = createLegacyConfig();
    const auto streamings = section(legacy, "Streaming");
    streamings.removeProperty(NativeId);
    streamings.removeProperty(OtherId);

    ASSERT_EQ(fromLegacy(legacy).getStreamingOptions().getAvailableStreamingConfigs().getKeyList(), List<IString>(LtId));
}

TEST_F(AddDeviceConfigLegacyTest, ReservedStreamingTypeIdIsNotListedAndWarns)
{
    const auto legacy = createLegacyConfig();
    legacy.removeProperty("Streaming");
    section(legacy, "General").setPropertyValue("PrioritizedStreamingProtocols", List<IString>(LtId));
    const auto loadedTypes = Dict<IString, IStreamingType>({{LtId, createStreamingType(LtId)}, {"a.b", createStreamingType("a.b")}});

    const auto config = fromLegacyAddDeviceConfig(legacy, createDeviceType(), loadedTypes, True, loggerComponent);

    ASSERT_EQ(config.getStreamingOptions().getAvailableStreamingConfigs().getKeyList(), List<IString>(LtId));
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

TEST_F(AddDeviceConfigLegacyTest, AdapterDropsStreamingSection)
{
    const auto legacy = createLegacyConfig();
    typeSection(legacy, "Device", "dev").setPropertyValue("Port", 1);

    const auto config = fromLegacy(legacy, DeviceKind::Adapter);

    ASSERT_FALSE(config.getStreamingOptions().assigned());
    ASSERT_EQ(config.getTypeConfig().getPropertyValue("Port"), 1);
}

// The Add streaming config translated from a legacy config, a plain type config or null is created for the given
// Streaming type.
TEST_F(AddDeviceConfigLegacyTest, StreamingConfigIsCreatedForTheStreamingType)
{
    const auto streamingType = streamingTypes.get(OtherId);

    ASSERT_EQ(fromLegacyAddStreamingConfig(createLegacyConfig(), streamingType).getStreamingType(), streamingType);
    ASSERT_EQ(fromLegacyAddStreamingConfig(createTypeConfig(), streamingType).getStreamingType(), streamingType);
    ASSERT_EQ(fromLegacyAddStreamingConfig(nullptr, streamingType).getStreamingType(), streamingType);
}

// A null config translates into an Add streaming config with the default type config of the Streaming type.
TEST_F(AddDeviceConfigLegacyTest, StreamingConfigOfNullHasDefaultTypeConfig)
{
    const auto config = fromLegacyAddStreamingConfig(nullptr, streamingTypes.get(OtherId));

    ASSERT_EQ(serialize(config.getTypeConfig()), serialize(createTypeConfig()));
}

// A config that isn't a legacy one - missing any of the Device, Streaming or General sections, or a plain type config
// with changed values and extra properties - becomes the type config of the Add streaming config as it is.
TEST_F(AddDeviceConfigLegacyTest, StreamingConfigTakesOtherConfigsAsTypeConfig)
{
    const auto streamingType = streamingTypes.get(OtherId);
    for (const auto name : {"Device", "Streaming", "General"})
    {
        const auto config = createLegacyConfig();
        config.removeProperty(name);
        ASSERT_EQ(serialize(fromLegacyAddStreamingConfig(config, streamingType).getTypeConfig()), serialize(config));
    }

    const auto typeConfig = createTypeConfig();
    typeConfig.setPropertyValue("Port", 1);
    typeConfig.addProperty(StringProperty("Extra", "value"));
    ASSERT_EQ(serialize(fromLegacyAddStreamingConfig(typeConfig, streamingType).getTypeConfig()), serialize(typeConfig));
}

TEST_F(AddDeviceConfigLegacyTest, StreamingConfigTakesItsEntryAndGeneralCredentials)
{
    const auto legacy = createLegacyConfig();
    const auto general = section(legacy, "General");
    general.setPropertyValue("Username", "user");
    general.setPropertyValue("Password", "pw");
    const auto entry = typeSection(legacy, "Streaming", LtId);
    entry.setPropertyValue("Port", 1);
    entry.setPropertyValue("Username", "ltuser");

    const auto typeConfig = fromLegacyAddStreamingConfig(legacy, streamingTypes.get(LtId)).getTypeConfig();

    ASSERT_EQ(typeConfig.getPropertyValue("Port"), 1);
    ASSERT_EQ(typeConfig.getPropertyValue("Username"), "ltuser");
    ASSERT_EQ(typeConfig.getPropertyValue("Password"), "pw");
}

TEST_F(AddDeviceConfigLegacyTest, StreamingConfigWithoutEntryTakesGeneralCredentials)
{
    const auto legacy = createLegacyConfig();
    section(legacy, "Streaming").removeProperty(LtId);
    section(legacy, "General").setPropertyValue("Username", "user");

    const auto expected = createTypeConfig();
    expected.setPropertyValue("Username", "user");

    ASSERT_EQ(serialize(fromLegacyAddStreamingConfig(legacy, streamingTypes.get(LtId)).getTypeConfig()), serialize(expected));
}

// A Streaming type without authentication methods gives an Add streaming config without an authentication config,
// even when the legacy config has credential properties.
TEST_F(AddDeviceConfigLegacyTest, StreamingConfigWithoutAuthenticationMethodsHasNoAuthenticationConfig)
{
    const auto legacy = createLegacyConfig();
    const auto general = section(legacy, "General");
    general.setPropertyValue("Username", "user");
    general.setPropertyValue("Password", "pw");

    ASSERT_FALSE(fromLegacyAddStreamingConfig(legacy, streamingTypes.get(OtherId)).getAuthenticationConfig().assigned());
}

// The General Username/Password of a legacy config are supplied as the UserNamePassword credential of the Add streaming
// config's authentication config, for a Streaming type with authentication methods.
TEST_F(AddDeviceConfigLegacyTest, StreamingConfigSuppliesLegacyCredentials)
{
    const auto legacy = createLegacyConfig();
    const auto general = section(legacy, "General");
    general.setPropertyValue("Username", "user");
    general.setPropertyValue("Password", "pw");

    const auto config = fromLegacyAddStreamingConfig(legacy, createAuthenticatedStreamingType(OtherId));

    assertCredential(config.getAuthenticationConfig(), credential("user", "pw"));
}

// The Username/Password of the type's Streaming entry (legacy) take precedence over the General ones when translated to the
// supplied UserNamePassword credential.
TEST_F(AddDeviceConfigLegacyTest, StreamingConfigSuppliesEntryCredentials)
{
    const auto legacy = createLegacyConfig();
    const auto general = section(legacy, "General");
    general.setPropertyValue("Username", "user");
    general.setPropertyValue("Password", "pw");
    const auto entry = typeSection(legacy, "Streaming", OtherId);
    entry.setPropertyValue("Username", "streaminguser");
    entry.setPropertyValue("Password", "streamingpw");

    const auto config = fromLegacyAddStreamingConfig(legacy, createAuthenticatedStreamingType(OtherId));

    assertCredential(config.getAuthenticationConfig(), credential("streaminguser", "streamingpw"));
}

// The Username/Password properties of a standalone type config are translated as the UserNamePassword method's supplied credentials.
TEST_F(AddDeviceConfigLegacyTest, StreamingConfigSuppliesTypeConfigCredentials)
{
    const auto typeConfig = createTypeConfig();
    typeConfig.setPropertyValue("Username", "user");
    typeConfig.setPropertyValue("Password", "pw");

    const auto config = fromLegacyAddStreamingConfig(typeConfig, createAuthenticatedStreamingType(OtherId));

    assertCredential(config.getAuthenticationConfig(), credential("user", "pw"));
}

// Without a Username - a password alone, or a null config - no credential is supplied and the default method stays
// selected.
TEST_F(AddDeviceConfigLegacyTest, StreamingConfigWithoutUsernameSuppliesNoCredentials)
{
    const auto legacy = createLegacyConfig();
    section(legacy, "General").setPropertyValue("Password", "pw");

    assertNoCredential(fromLegacyAddStreamingConfig(legacy, createAuthenticatedStreamingType(OtherId)).getAuthenticationConfig());
    assertNoCredential(fromLegacyAddStreamingConfig(nullptr, createAuthenticatedStreamingType(OtherId)).getAuthenticationConfig());
}

// A Streaming type without the UserNamePassword method supported translates no credential from the legacy Username/Password properties.
TEST_F(AddDeviceConfigLegacyTest, StreamingConfigWithoutUserNamePasswordMethodSuppliesNoCredentials)
{
    const auto legacy = createLegacyConfig();
    const auto general = section(legacy, "General");
    general.setPropertyValue("Username", "user");
    general.setPropertyValue("Password", "pw");

    const auto config = fromLegacyAddStreamingConfig(legacy, createAuthenticatedStreamingType(OtherId, false));

    assertNoCredential(config.getAuthenticationConfig());
}

// Legacy device config translation supplies the credentials to the authentication configs of the device and of each
// streaming type - the Username/Password props of a type entry override the General ones, a type without them takes General.
TEST_F(AddDeviceConfigLegacyTest, DeviceConfigSuppliesLegacyCredentials)
{
    const auto legacy = createLegacyConfig();
    const auto general = section(legacy, "General");
    general.setPropertyValue("Username", "user");
    general.setPropertyValue("Password", "pw");
    const auto deviceEntry = typeSection(legacy, "Device", "dev");
    deviceEntry.setPropertyValue("Username", "devuser");
    deviceEntry.setPropertyValue("Password", "devpw");
    const auto streamingEntry = typeSection(legacy, "Streaming", OtherId);
    streamingEntry.setPropertyValue("Username", "streaminguser");
    streamingEntry.setPropertyValue("Password", "streamingpw");

    const auto authenticatedTypes = Dict<IString, IStreamingType>(
        {{NativeId, createAuthenticatedStreamingType(NativeId)}, {OtherId, createAuthenticatedStreamingType(OtherId)}});
    const auto config = fromLegacyAddDeviceConfig(legacy, createAuthenticatedDeviceType(), authenticatedTypes, True, loggerComponent);
    const auto streamingConfigs = config.getStreamingOptions().getAvailableStreamingConfigs();

    assertCredential(config.getAuthenticationConfig(), credential("devuser", "devpw"));
    assertCredential(streamingConfigs.get(NativeId).getAuthenticationConfig(), credential("user", "pw"));
    assertCredential(streamingConfigs.get(OtherId).getAuthenticationConfig(), credential("streaminguser", "streamingpw"));
}

// Legacy device config translation without a Username - Passwords alone, in General and the type entries - supplies no
// credential to the device or streaming authentication configs.
TEST_F(AddDeviceConfigLegacyTest, DeviceConfigWithoutUsernameSuppliesNoCredentials)
{
    const auto legacy = createLegacyConfig();
    section(legacy, "General").setPropertyValue("Password", "pw");
    typeSection(legacy, "Device", "dev").setPropertyValue("Password", "devpw");
    typeSection(legacy, "Streaming", OtherId).setPropertyValue("Password", "streamingpw");

    const auto authenticatedTypes = Dict<IString, IStreamingType>({{OtherId, createAuthenticatedStreamingType(OtherId)}});
    const auto config = fromLegacyAddDeviceConfig(legacy, createAuthenticatedDeviceType(), authenticatedTypes, True, loggerComponent);

    assertNoCredential(config.getAuthenticationConfig());
    assertNoCredential(config.getStreamingOptions().getAvailableStreamingConfigs().get(OtherId).getAuthenticationConfig());
}

TEST_F(AddDeviceConfigLegacyTest, NoStreamingOptionsDoesNotConnectStreaming)
{
    const auto connects = [](DeviceKind kind)
    {
        const auto legacy = toLegacyAddDeviceConfig(AddDeviceConfig(createDeviceType(kind), nullptr, nullptr, True));
        return static_cast<Bool>(section(legacy, "General").getPropertyValue("AutomaticallyConnectStreaming"));
    };

    ASSERT_FALSE(connects(DeviceKind::Mirrored));
    ASSERT_FALSE(connects(DeviceKind::Unspecified));
    ASSERT_TRUE(connects(DeviceKind::Adapter));
}

// New -> old translation writes the credential supplied for the selected UserNamePassword method of each authentication
// config into the deprecated Username/Password of its type entry - the device one into its Device entry, the streaming one
// into its Streaming entry - while General keeps its empty defaults.
TEST_F(AddDeviceConfigLegacyTest, ToLegacySetsCredentialProperties)
{
    const auto authenticatedTypes = Dict<IString, IStreamingType>({{OtherId, createAuthenticatedStreamingType(OtherId)}});
    const auto config = AddDeviceConfig(createAuthenticatedDeviceType(), authenticatedTypes, True);
    config.getAuthenticationConfig().setAuthenticationMethod(StandardUserNamePasswordId);
    config.getAuthenticationConfig().setSuppliedCredential(credential("user", "pw"));
    const auto streamingAuthentication = config.getStreamingOptions().getAvailableStreamingConfigs().get(OtherId).getAuthenticationConfig();
    streamingAuthentication.setAuthenticationMethod(StandardUserNamePasswordId);
    streamingAuthentication.setSuppliedCredential(credential("streaminguser", "streamingpw"));

    const auto legacy = toLegacyAddDeviceConfig(config);

    const auto deviceEntry = typeSection(legacy, "Device", "dev");
    ASSERT_EQ(deviceEntry.getPropertyValue("Username"), "user");
    ASSERT_EQ(deviceEntry.getPropertyValue("Password"), "pw");
    const auto streamingEntry = typeSection(legacy, "Streaming", OtherId);
    ASSERT_EQ(streamingEntry.getPropertyValue("Username"), "streaminguser");
    ASSERT_EQ(streamingEntry.getPropertyValue("Password"), "streamingpw");
    ASSERT_EQ(section(legacy, "General").getPropertyValue("Username"), "");
    ASSERT_EQ(section(legacy, "General").getPropertyValue("Password"), "");
}

// Bind drops the credentials supplied on the authentication configs and prunes the deprecated Username/Password properties
// (typically set by mistake) of the device and streaming type configs whose authentication config has the UserNamePassword
// method, as their credential is supplied through the authentication config.
TEST_F(AddDeviceConfigLegacyTest, BindClearsCredsAndPropsOfAuthenticatedTypes)
{
    const auto deviceType = createAuthenticatedDeviceType();
    const auto authenticatedTypes = Dict<IString, IStreamingType>({{OtherId, createAuthenticatedStreamingType(OtherId)}});
    const auto config = AddDeviceConfig(deviceType, authenticatedTypes, True);
    config.getTypeConfig().setPropertyValue("Username", "user");
    config.getTypeConfig().setPropertyValue("Password", "pw");
    config.getAuthenticationConfig().setAuthenticationMethod(StandardUserNamePasswordId);
    config.getAuthenticationConfig().setSuppliedCredential(credential("user", "pw"));
    const auto options = config.getStreamingOptions();
    options.setStreamingType(OtherId);
    const auto streamingConfig = options.getStreamingConfig();
    streamingConfig.getTypeConfig().setPropertyValue("Username", "streaminguser");
    streamingConfig.getTypeConfig().setPropertyValue("Password", "streamingpw");
    streamingConfig.getAuthenticationConfig().setAuthenticationMethod(StandardUserNamePasswordId);
    streamingConfig.getAuthenticationConfig().setSuppliedCredential(credential("streaminguser", "streamingpw"));

    const auto bound = config.asPtr<IAddDeviceConfigPrivate>().bind(Dict<IString, IDeviceType>({{"dev", deviceType}}), authenticatedTypes);

    const auto boundTypeConfig = bound.getTypeConfig();
    ASSERT_EQ(boundTypeConfig.getPropertyValue("Username"), "");
    ASSERT_EQ(boundTypeConfig.getPropertyValue("Password"), "");
    ASSERT_EQ(bound.getAuthenticationConfig().getSuppliedCredential().getCount(), 0u);
    const auto boundStreamingConfig = bound.getStreamingOptions().getStreamingConfig();
    ASSERT_EQ(boundStreamingConfig.getTypeConfig().getPropertyValue("Username"), "");
    ASSERT_EQ(boundStreamingConfig.getTypeConfig().getPropertyValue("Password"), "");
    ASSERT_EQ(boundStreamingConfig.getAuthenticationConfig().getSuppliedCredential().getCount(), 0u);
}

// Bind keeps the Username/Password properties of device and streaming types without authentication methods supported via modern API,
// as corresponding modules still read the credential from the type config properties.
TEST_F(AddDeviceConfigLegacyTest, BindKeepsCredPropsOfTypesWithoutModernAuth)
{
    const auto deviceType = createDeviceType();
    const auto config = AddDeviceConfig(deviceType, streamingTypes, True);
    config.getTypeConfig().setPropertyValue("Username", "user");
    config.getTypeConfig().setPropertyValue("Password", "pw");
    const auto options = config.getStreamingOptions();
    options.setStreamingType(OtherId);
    const auto streamingTypeConfig = options.getStreamingConfig().getTypeConfig();
    streamingTypeConfig.setPropertyValue("Username", "streaminguser");
    streamingTypeConfig.setPropertyValue("Password", "streamingpw");

    const auto bound = config.asPtr<IAddDeviceConfigPrivate>().bind(Dict<IString, IDeviceType>({{"dev", deviceType}}), streamingTypes);

    const auto boundTypeConfig = bound.getTypeConfig();
    ASSERT_EQ(boundTypeConfig.getPropertyValue("Username"), "user");
    ASSERT_EQ(boundTypeConfig.getPropertyValue("Password"), "pw");
    const auto boundStreamingTypeConfig = bound.getStreamingOptions().getStreamingConfig().getTypeConfig();
    ASSERT_EQ(boundStreamingTypeConfig.getPropertyValue("Username"), "streaminguser");
    ASSERT_EQ(boundStreamingTypeConfig.getPropertyValue("Password"), "streamingpw");
}

END_NAMESPACE_OPENDAQ
