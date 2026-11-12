#include <opendaq/instance_factory.h>
#include <opendaq/component_type_builder_factory.h>
#include <opendaq/device_type_factory.h>
#include <opendaq/streaming_type_factory.h>
#include <opendaq/component_type_private_ptr.h>
#include <opendaq/authentication_method_factory.h>
#include <gtest/gtest.h>
#include <coreobjects/property_object_factory.h>
#include <coreobjects/property_factory.h>
#include <opendaq/streaming_type_factory.h>
#include <opendaq/transport_option_impl.h>
#include <coreobjects/property_object_internal_ptr.h>
#include <coretypes/json_serializer_factory.h>
#include <coretypes/json_deserializer_factory.h>

using OpenDaqFactoriesTest = testing::Test;

BEGIN_NAMESPACE_OPENDAQ

TEST_F(OpenDaqFactoriesTest, ServerTypeBuilder)
{
    auto defConfig = PropertyObject();
    defConfig.addProperty(StringProperty("foo", "bar"));

    const ServerTypePtr type = ServerTypeBuilder()
                               .setId("Id")
                               .setName("Name")
                               .setDescription("Desc")
                               .setDefaultConfig(defConfig)
                               .build();

    ASSERT_EQ(type.getId(), "Id");
    ASSERT_EQ(type.getName(), "Name");
    ASSERT_EQ(type.getDescription(), "Desc");
    ASSERT_EQ(type.createDefaultConfig().getPropertyValue("foo"), "bar");
}

TEST_F(OpenDaqFactoriesTest, DeviceTypeBuilder)
{
    auto defConfig = PropertyObject();
    defConfig.addProperty(StringProperty("foo", "bar"));

    const DeviceTypePtr type = DeviceTypeBuilder()
                                   .setId("Id")
                                   .setName("Name")
                                   .setDescription("Desc")
                                   .setConnectionStringPrefix("Prefix")
                                   .setDefaultConfig(defConfig)
                                   .setDeviceKind(DeviceKind::Mirrored)
                                   .build();

    ASSERT_EQ(type.getId(), "Id");
    ASSERT_EQ(type.getName(), "Name");
    ASSERT_EQ(type.getDescription(), "Desc");
    ASSERT_EQ(type.getConnectionStringPrefix(), "Prefix");
    ASSERT_EQ(type.createDefaultConfig().getPropertyValue("foo"), "bar");
    ASSERT_EQ(type.getDeviceKind(), DeviceKind::Mirrored);
}

TEST_F(OpenDaqFactoriesTest, CreateDefaultConfigReturnsCopy)
{
    auto defConfig = PropertyObject();
    defConfig.addProperty(StringProperty("foo", "bar"));

    const DeviceTypePtr type = DeviceTypeBuilder().setId("Id").setConnectionStringPrefix("Prefix").setDefaultConfig(defConfig).build();

    type.createDefaultConfig().setPropertyValue("foo", "changed");
    ASSERT_EQ(type.createDefaultConfig().getPropertyValue("foo"), "bar");
}

TEST_F(OpenDaqFactoriesTest, FBTypeBuilder)
{
    auto defConfig = PropertyObject();
    defConfig.addProperty(StringProperty("foo", "bar"));

    const FunctionBlockTypePtr type = FunctionBlockTypeBuilder()
                               .setId("Id")
                               .setName("Name")
                               .setDescription("Desc")
                               .setDefaultConfig(defConfig)
                               .build();

    ASSERT_EQ(type.getId(), "Id");
    ASSERT_EQ(type.getName(), "Name");
    ASSERT_EQ(type.getDescription(), "Desc");
    ASSERT_EQ(type.createDefaultConfig().getPropertyValue("foo"), "bar");
}

TEST_F(OpenDaqFactoriesTest, StreamingTypeBuilder)
{
    auto defConfig = PropertyObject();
    defConfig.addProperty(StringProperty("foo", "bar"));

    const StreamingTypePtr type = StreamingTypeBuilder()
                                      .setId("Id")
                                      .setName("Name")
                                      .setDescription("Desc")
                                      .setConnectionStringPrefix("Prefix")
                                      .setDefaultConfig(defConfig)
                                      .build();

    ASSERT_EQ(type.getId(), "Id");
    ASSERT_EQ(type.getName(), "Name");
    ASSERT_EQ(type.getDescription(), "Desc");
    ASSERT_EQ(type.getConnectionStringPrefix(), "Prefix");
    ASSERT_EQ(type.createDefaultConfig().getPropertyValue("foo"), "bar");
    ASSERT_FALSE(type.getLinkedDeviceType().assigned());
}

TEST_F(OpenDaqFactoriesTest, BuilderDeviceKind)
{
    auto builder = DeviceTypeBuilder();
    ASSERT_EQ(builder.getDeviceKind(), DeviceKind::Unspecified);
    ASSERT_THROW(builder.setDeviceKind(DeviceKind::Unspecified), InvalidParameterException);
    for (const auto kind : {DeviceKind::Root, DeviceKind::Mirrored, DeviceKind::Adapter})
    {
        builder.setDeviceKind(kind);
        ASSERT_EQ(builder.getDeviceKind(), kind);
    }
}

TEST_F(OpenDaqFactoriesTest, StreamingTypeLinkNotInStructOrEquality)
{
    const auto deviceType = DeviceTypeBuilder().setId("DevType").build();
    const StreamingTypePtr unlinked =
        StreamingTypeBuilder().setId("Id").setName("Name").setDescription("Desc").setConnectionStringPrefix("Prefix").build();
    const StreamingTypePtr linked = StreamingTypeBuilder()
                                        .setId("Id")
                                        .setName("Name")
                                        .setDescription("Desc")
                                        .setConnectionStringPrefix("Prefix")
                                        .setLinkedDeviceType(deviceType)
                                        .build();
    const StreamingTypePtr factory = StreamingType("Id", "Name", "Desc", "Prefix");

    ASSERT_FALSE(factory.getLinkedDeviceType().assigned());
    ASSERT_EQ(unlinked, linked);
    ASSERT_EQ(unlinked, factory);
    ASSERT_EQ(linked.asPtr<IStruct>().getFieldNames(), StreamingTypeStructType().getFieldNames());
}

TEST_F(OpenDaqFactoriesTest, BuilderIgnoresPrefixOnOtherSorts)
{
    ASSERT_NO_THROW(ServerTypeBuilder().setId("Id").setConnectionStringPrefix("Prefix").build());
    ASSERT_NO_THROW(FunctionBlockTypeBuilder().setId("Id").setConnectionStringPrefix("Prefix").build());
    ASSERT_NO_THROW(ServerTypeBuilder().setId("Id").setConnectionStringPrefix("").build());
}

TEST_F(OpenDaqFactoriesTest, DeviceKindOnlyAppliesToDevice)
{
    for (const auto& builder : {StreamingTypeBuilder(), ServerTypeBuilder(), FunctionBlockTypeBuilder()})
    {
        ASSERT_THROW(builder.setDeviceKind(DeviceKind::Root), NotSupportedException);
        ASSERT_THROW(builder.getDeviceKind(), NotSupportedException);
    }
}

TEST_F(OpenDaqFactoriesTest, AddAuthenticationMethodOnlyAppliesToDeviceOrStreaming)
{
    const auto method = StandardAnonymousAuthenticationMethod();

    ASSERT_THROW(ServerTypeBuilder().addAuthenticationMethod(method), NotSupportedException);
    ASSERT_THROW(FunctionBlockTypeBuilder().addAuthenticationMethod(method), NotSupportedException);
    ASSERT_NO_THROW(DeviceTypeBuilder().addAuthenticationMethod(method));
    ASSERT_NO_THROW(StreamingTypeBuilder().addAuthenticationMethod(method));
}

TEST_F(OpenDaqFactoriesTest, SetLinkedDeviceTypeOnlyAppliesToStreaming)
{
    const auto deviceType = DeviceTypeBuilder().setId("LinkTargetDeviceType").setName("name").setDescription("description").setConnectionStringPrefix("daq.prefix").build();

    ASSERT_THROW(DeviceTypeBuilder().setLinkedDeviceType(deviceType), NotSupportedException);
    ASSERT_THROW(ServerTypeBuilder().setLinkedDeviceType(deviceType), NotSupportedException);
    ASSERT_THROW(FunctionBlockTypeBuilder().setLinkedDeviceType(deviceType), NotSupportedException);
    ASSERT_NO_THROW(StreamingTypeBuilder().setLinkedDeviceType(deviceType));

    ASSERT_THROW(DeviceTypeBuilder().getLinkedDeviceType(), NotSupportedException);
    ASSERT_THROW(ServerTypeBuilder().getLinkedDeviceType(), NotSupportedException);
    ASSERT_THROW(FunctionBlockTypeBuilder().getLinkedDeviceType(), NotSupportedException);
}

TEST_F(OpenDaqFactoriesTest, AddAuthenticationMethodAppendsInOrderAndDefaultsFirst)
{
    const auto anonymous = StandardAnonymousAuthenticationMethod();
    const auto userNamePassword = StandardUserNamePasswordAuthenticationMethod();

    const auto componentType = DeviceTypeBuilder()
                                    .setId("TestDeviceType")
                                    .setName("name")
                                    .setDescription("description")
                                    .setConnectionStringPrefix("daq.prefix")
                                    .addAuthenticationMethod(anonymous)
                                    .addAuthenticationMethod(userNamePassword)
                                    .build();

    const auto methods = componentType.asPtr<IComponentTypePrivate>().getSupportedAuthenticationMethods();
    ASSERT_EQ(methods.getKeyList(), List<IString>(StandardAnonymousId, StandardUserNamePasswordId));

    // The first one added is the one a built `IAuthenticationConfig` starts out selected on.
    const auto authConfig = componentType.createAuthenticationConfig();
    ASSERT_EQ(authConfig.getSelectedAuthenticationMethod().getId(), StandardAnonymousId);
}

TEST_F(OpenDaqFactoriesTest, AddAuthenticationMethodThrowsOnDuplicateId)
{
    auto builder = DeviceTypeBuilder()
                       .setId("TestDeviceType")
                       .setName("name")
                       .setDescription("description")
                       .setConnectionStringPrefix("daq.prefix")
                       .addAuthenticationMethod(StandardAnonymousAuthenticationMethod());

    ASSERT_THROW(builder.addAuthenticationMethod(StandardAnonymousAuthenticationMethod()), AlreadyExistsException);
}

TEST_F(OpenDaqFactoriesTest, SetLinkedDeviceTypeThrowsIfDefaultConfigAlreadySet)
{
    const auto deviceType = DeviceTypeBuilder().setId("LinkTargetDeviceType").setName("name").setDescription("description").setConnectionStringPrefix("daq.prefix").build();

    auto builder = StreamingTypeBuilder()
                       .setId("TestStreamingType")
                       .setName("name")
                       .setDescription("description")
                       .setConnectionStringPrefix("daq.prefix")
                       .setDefaultConfig(PropertyObject());

    ASSERT_THROW(builder.setLinkedDeviceType(deviceType), InvalidOperationException);
}

TEST_F(OpenDaqFactoriesTest, SetLinkedDeviceTypeThrowsIfAuthenticationMethodAlreadyAdded)
{
    const auto deviceType = DeviceTypeBuilder().setId("LinkTargetDeviceType").setName("name").setDescription("description").setConnectionStringPrefix("daq.prefix").build();

    auto builder = StreamingTypeBuilder()
                       .setId("TestStreamingType")
                       .setName("name")
                       .setDescription("description")
                       .setConnectionStringPrefix("daq.prefix")
                       .addAuthenticationMethod(StandardAnonymousAuthenticationMethod());

    ASSERT_THROW(builder.setLinkedDeviceType(deviceType), InvalidOperationException);
}

TEST_F(OpenDaqFactoriesTest, SettersThrowIfLinkedDeviceTypeAlreadySet)
{
    const auto deviceType = DeviceTypeBuilder().setId("LinkTargetDeviceType").setName("name").setDescription("description").setConnectionStringPrefix("daq.prefix").build();

    auto builder = StreamingTypeBuilder()
                       .setId("TestStreamingType")
                       .setName("name")
                       .setDescription("description")
                       .setConnectionStringPrefix("daq.prefix")
                       .setLinkedDeviceType(deviceType);

    ASSERT_THROW(builder.setDefaultConfig(PropertyObject()), InvalidOperationException);
    ASSERT_THROW(builder.addAuthenticationMethod(StandardAnonymousAuthenticationMethod()), InvalidOperationException);
}

TEST_F(OpenDaqFactoriesTest, SetLinkedDeviceTypeSucceedsOnFreshBuilder)
{
    const auto deviceType = DeviceTypeBuilder().setId("LinkTargetDeviceType").setName("name").setDescription("description").setConnectionStringPrefix("daq.prefix").build();

    const auto streamingType = StreamingTypeBuilder()
                                    .setId("TestStreamingType")
                                    .setName("name")
                                    .setDescription("description")
                                    .setConnectionStringPrefix("daq.prefix")
                                    .setLinkedDeviceType(deviceType)
                                    .build();

    ASSERT_TRUE(streamingType.assigned());
}

TEST_F(OpenDaqFactoriesTest, StreamingWithLinkedDeviceTypeRedirects)
{
    auto defConfig = PropertyObject();
    defConfig.addProperty(StringProperty("foo", "bar"));

    const auto deviceType = DeviceTypeBuilder()
                                 .setId("LinkTargetDeviceType")
                                 .setName("name")
                                 .setDescription("description")
                                 .setConnectionStringPrefix("daq.prefix")
                                 .setDefaultConfig(defConfig)
                                 .addAuthenticationMethod(StandardAnonymousAuthenticationMethod())
                                 .addAuthenticationMethod(StandardUserNamePasswordAuthenticationMethod())
                                 .build();

    const auto streamingType = StreamingTypeBuilder()
                                    .setId("TestStreamingType")
                                    .setName("name")
                                    .setDescription("description")
                                    .setConnectionStringPrefix("daq.prefix")
                                    .setLinkedDeviceType(deviceType)
                                    .build();

    const auto streamingMethods = streamingType.asPtr<IComponentTypePrivate>().getSupportedAuthenticationMethods();
    const auto deviceMethods = deviceType.asPtr<IComponentTypePrivate>().getSupportedAuthenticationMethods();
    ASSERT_EQ(streamingMethods.getKeyList(), deviceMethods.getKeyList());

    const auto streamingAuthConfig = streamingType.createAuthenticationConfig();
    ASSERT_EQ(streamingAuthConfig.getSelectedAuthenticationMethod().getId(), StandardAnonymousId);

    ASSERT_EQ(streamingType.createDefaultConfig().getPropertyValue("foo"), "bar");
}

TEST_F(OpenDaqFactoriesTest, ServerTypeDoesNotImplementAuthentication)
{
    const auto serverType = ServerTypeBuilder().setId("Id").setName("Name").setDescription("Desc").build();

    ASSERT_THROW(serverType.createAuthenticationConfig(), NotImplementedException);
    ASSERT_THROW(serverType.asPtr<IComponentTypePrivate>().getSupportedAuthenticationMethods(), NotImplementedException);
}

TEST_F(OpenDaqFactoriesTest, FunctionBlockTypeDoesNotImplementAuthentication)
{
    const auto fbType = FunctionBlockTypeBuilder().setId("Id").setName("Name").setDescription("Desc").build();

    ASSERT_THROW(fbType.createAuthenticationConfig(), NotImplementedException);
    ASSERT_THROW(fbType.asPtr<IComponentTypePrivate>().getSupportedAuthenticationMethods(), NotImplementedException);
}

TEST_F(OpenDaqFactoriesTest, DeviceTypeWithNoMethodsAddedDoesNotSupportAuthentication)
{
    const auto deviceType = DeviceTypeBuilder().setId("Id").setName("Name").setDescription("Desc").setConnectionStringPrefix("daq.prefix").build();

    ASSERT_THROW(deviceType.createAuthenticationConfig(), NotSupportedException);

    DictPtr<IString, IAuthenticationMethod> methods;
    ASSERT_NO_THROW(methods = deviceType.asPtr<IComponentTypePrivate>().getSupportedAuthenticationMethods());
    ASSERT_TRUE(methods.getCount() == 0);
}

TEST_F(OpenDaqFactoriesTest, StreamingTypeTransportOptions)
{
    const StreamingTypePtr type = StreamingTypeBuilder()
                                  .setId("Id")
                                  .setConnectionStringPrefix("Prefix")
                                  .setTransportOptions(List<ITransportOption>(TransportOption("tls", 7421, TransportProtection::Encrypted),
                                                                              TransportOption("plain", 7420)))
                                  .build();

    ASSERT_EQ(TransportOptionNames(type.getTransportOptions()), List<IString>("tls", "plain"));
    ASSERT_EQ(type.getTransportOptions()[0].getPort(), 7421);
    ASSERT_EQ(StreamingType("Id", "Name", "Desc", "Prefix").getTransportOptions().getCount(), 0u);
}

TEST_F(OpenDaqFactoriesTest, StreamingTypeSerializeDeserialize)
{
    const StreamingTypePtr type = StreamingTypeBuilder()
                                  .setId("Id")
                                  .setName("Name")
                                  .setDescription("Desc")
                                  .setConnectionStringPrefix("Prefix")
                                  .setTransportOptions(List<ITransportOption>(TransportOption("wss", 7443, TransportProtection::Encrypted),
                                                                              TransportOption("tls", 7421, TransportProtection::Encrypted)))
                                  .build();

    const auto serializer = JsonSerializer();
    type.serialize(serializer);
    const auto serializedType = serializer.getOutput();

    const StreamingTypePtr newType = JsonDeserializer().deserialize(serializedType, nullptr, nullptr);

    ASSERT_EQ(newType.getId(), "Id");
    ASSERT_EQ(newType.getName(), "Name");
    ASSERT_EQ(newType.getDescription(), "Desc");
    ASSERT_EQ(newType.getConnectionStringPrefix(), "Prefix");
    ASSERT_EQ(TransportOptionNames(newType.getTransportOptions()), List<IString>("wss", "tls"));
    ASSERT_EQ(newType.getTransportOptions()[0].getProtection(), TransportProtection::Encrypted);

    serializer.reset();
    newType.serialize(serializer);
    ASSERT_EQ(serializedType, serializer.getOutput());
}

TEST_F(OpenDaqFactoriesTest, ServerTypeTransportOptions)
{
    const ServerTypePtr type = ServerTypeBuilder()
                               .setId("Id")
                               .setTransportOptions(List<ITransportOption>(TransportOption("tls", 7421, TransportProtection::Encrypted),
                                                                           TransportOption("plain", 7420)))
                               .build();

    ASSERT_EQ(TransportOptionNames(type.getTransportOptions()), List<IString>("tls", "plain"));
    ASSERT_EQ(type.getTransportOptions()[1].getPort(), 7420);
    ASSERT_EQ(ServerTypeBuilder().setId("Id").build().getTransportOptions().getCount(), 0u);
}

TEST_F(OpenDaqFactoriesTest, ServerTypeSerializeDeserialize)
{
    const ServerTypePtr type = ServerTypeBuilder()
                               .setId("Id")
                               .setName("Name")
                               .setDescription("Desc")
                               .setTransportOptions(List<ITransportOption>(TransportOption("https", 443, TransportProtection::Encrypted),
                                                                           TransportOption("plain", 7420)))
                               .build();

    const auto serializer = JsonSerializer();
    type.serialize(serializer);
    const auto serializedType = serializer.getOutput();

    const ServerTypePtr newType = JsonDeserializer().deserialize(serializedType, nullptr, nullptr);

    ASSERT_EQ(newType.getId(), "Id");
    ASSERT_EQ(newType.getName(), "Name");
    ASSERT_EQ(newType.getDescription(), "Desc");
    ASSERT_EQ(TransportOptionNames(newType.getTransportOptions()), List<IString>("https", "plain"));

    serializer.reset();
    newType.serialize(serializer);
    ASSERT_EQ(serializedType, serializer.getOutput());
}

TEST_F(OpenDaqFactoriesTest, FBTypeHasNoTransportOptions)
{
    const auto options = List<ITransportOption>(TransportOption("plain", 7420));

    ASSERT_THROW(FunctionBlockTypeBuilder().setTransportOptions(options), NotSupportedException);
    ASSERT_THROW(DeviceTypeBuilder().setTypeSort(ComponentTypeSort::Undefined).setTransportOptions(options), NotSupportedException);

    const FunctionBlockTypePtr fbType = FunctionBlockTypeBuilder().setId("Id").build();
    ASSERT_THROW(fbType.getTransportOptions(), NotImplementedException);
}

TEST_F(OpenDaqFactoriesTest, StreamingTypeDefaultConfigSelectsFirstTransportOption)
{
    auto moduleConfig = PropertyObject();
    moduleConfig.addProperty(IntProperty("Port", 7000));

    const StreamingTypePtr type = StreamingTypeBuilder()
                                      .setId("Id")
                                      .setConnectionStringPrefix("Prefix")
                                      .setDefaultConfig(moduleConfig)
                                      .setTransportOptions(List<ITransportOption>(TransportOption("plain", 7420), TransportOption("tls", 7421)))
                                      .build();

    // The module's own port is kept
    const PropertyObjectPtr config = type.createDefaultConfig();
    ASSERT_EQ(config.getPropertyValue(TransportOptionConfigProperty), "plain");
    ASSERT_EQ(config.getPropertyValue(PortConfigProperty), 7000);

    const PropertyObjectPtr configWithoutModulePort = StreamingTypeBuilder()
                                                          .setId("Id")
                                                          .setConnectionStringPrefix("Prefix")
                                                          .setTransportOptions(List<ITransportOption>(TransportOption("tls", 7421)))
                                                          .build()
                                                          .createDefaultConfig();
    ASSERT_EQ(configWithoutModulePort.getPropertyValue(TransportOptionConfigProperty), "tls");
    ASSERT_EQ(configWithoutModulePort.getPropertyValue(PortConfigProperty), 7421);

    ASSERT_FALSE(StreamingType("Id", "Name", "Desc", "Prefix").createDefaultConfig().hasProperty(TransportOptionConfigProperty));
}

// A linked streaming type takes over the device's config, but selects a transport option of its own
TEST_F(OpenDaqFactoriesTest, LinkedStreamingTypeDefaultConfigSelectsOwnTransportOption)
{
    const DeviceTypePtr deviceType = DeviceTypeBuilder()
                                         .setId("DeviceId")
                                         .setConnectionStringPrefix("daq.device")
                                         .setTransportOptions(List<ITransportOption>(TransportOption("tls", 7421), TransportOption("plain", 7420)))
                                         .build();
    ASSERT_EQ(deviceType.createDefaultConfig().getPropertyValue(TransportOptionConfigProperty), "tls");

    const StreamingTypePtr type = StreamingTypeBuilder()
                                      .setId("Id")
                                      .setConnectionStringPrefix("Prefix")
                                      .setLinkedDeviceType(deviceType)
                                      .setTransportOptions(List<ITransportOption>(TransportOption("plain", 7420), TransportOption("tls", 7421)))
                                      .build();

    // The port the device type added stays: left at its default, it stands for the port of the selected option
    const PropertyObjectPtr config = type.createDefaultConfig();
    ASSERT_EQ(config.getPropertyValue(TransportOptionConfigProperty), "plain");
    ASSERT_EQ(config.getPropertyValue(PortConfigProperty), 7421);

    // Without options of its own the streaming selects nothing
    const PropertyObjectPtr configWithoutOptions =
        StreamingTypeBuilder().setId("Id").setConnectionStringPrefix("Prefix").setLinkedDeviceType(deviceType).build().createDefaultConfig();
    ASSERT_FALSE(configWithoutOptions.hasProperty(TransportOptionConfigProperty));
}

TEST_F(OpenDaqFactoriesTest, ServerTypeDefaultConfigHasNoTransportSelection)
{
    const ServerTypePtr type = ServerTypeBuilder()
                                   .setId("Id")
                                   .setTransportOptions(List<ITransportOption>(TransportOption("plain", 7420)))
                                   .build();

    const PropertyObjectPtr config = type.createDefaultConfig();
    ASSERT_FALSE(config.hasProperty(TransportOptionConfigProperty));
    ASSERT_FALSE(config.hasProperty(PortConfigProperty));
}

END_NAMESPACE_OPENDAQ
