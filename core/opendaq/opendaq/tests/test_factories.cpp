#include <opendaq/instance_factory.h>
#include <opendaq/component_type_builder_factory.h>
#include <opendaq/component_type_private_ptr.h>
#include <opendaq/authentication_method_factory.h>
#include <gtest/gtest.h>
#include <coreobjects/property_object_factory.h>
#include <coreobjects/property_factory.h>
#include <opendaq/streaming_type_factory.h>
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
                               .build();

    ASSERT_EQ(type.getId(), "Id");
    ASSERT_EQ(type.getName(), "Name");
    ASSERT_EQ(type.getDescription(), "Desc");
    ASSERT_EQ(type.getConnectionStringPrefix(), "Prefix");
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

// Dict equality ignores order, so the order is checked through the key list
TEST_F(OpenDaqFactoriesTest, StreamingTypeAvailableTransports)
{
    const StreamingTypePtr type = StreamingTypeBuilder()
                                  .setId("Id")
                                  .setConnectionStringPrefix("Prefix")
                                  .setAvailableTransports(Dict<IString, IInteger>({{"tls", 7421}, {"plain", 7420}}))
                                  .build();

    ASSERT_EQ(type.getAvailableTransports().getKeyList(), List<IString>("tls", "plain"));
    ASSERT_EQ(type.getAvailableTransports().get("tls"), 7421);
    ASSERT_EQ(StreamingType("Id", "Name", "Desc", "Prefix").getAvailableTransports().getCount(), 0u);
}

TEST_F(OpenDaqFactoriesTest, StreamingTypeSerializeDeserialize)
{
    const StreamingTypePtr type = StreamingTypeBuilder()
                                  .setId("Id")
                                  .setName("Name")
                                  .setDescription("Desc")
                                  .setConnectionStringPrefix("Prefix")
                                  .setAvailableTransports(Dict<IString, IInteger>({{"wss", 7443}, {"tls", 7421}}))
                                  .build();

    const auto serializer = JsonSerializer();
    type.serialize(serializer);
    const auto serializedType = serializer.getOutput();

    const StreamingTypePtr newType = JsonDeserializer().deserialize(serializedType, nullptr, nullptr);

    ASSERT_EQ(newType.getId(), "Id");
    ASSERT_EQ(newType.getName(), "Name");
    ASSERT_EQ(newType.getDescription(), "Desc");
    ASSERT_EQ(newType.getConnectionStringPrefix(), "Prefix");
    ASSERT_EQ(newType.getAvailableTransports().getKeyList(), List<IString>("wss", "tls"));

    serializer.reset();
    newType.serialize(serializer);
    ASSERT_EQ(serializedType, serializer.getOutput());
}

TEST_F(OpenDaqFactoriesTest, ServerTypeAvailableTransports)
{
    const ServerTypePtr type = ServerTypeBuilder()
                               .setId("Id")
                               .setAvailableTransports(Dict<IString, IInteger>({{"tls", 7421}, {"plain", 7420}}))
                               .build();

    ASSERT_EQ(type.getAvailableTransports().getKeyList(), List<IString>("tls", "plain"));
    ASSERT_EQ(type.getAvailableTransports().get("plain"), 7420);
    ASSERT_EQ(ServerTypeBuilder().setId("Id").build().getAvailableTransports().getCount(), 0u);
}

TEST_F(OpenDaqFactoriesTest, ServerTypeSerializeDeserialize)
{
    const ServerTypePtr type = ServerTypeBuilder()
                               .setId("Id")
                               .setName("Name")
                               .setDescription("Desc")
                               .setAvailableTransports(Dict<IString, IInteger>({{"https", 443}, {"plain", 7420}}))
                               .build();

    const auto serializer = JsonSerializer();
    type.serialize(serializer);
    const auto serializedType = serializer.getOutput();

    const ServerTypePtr newType = JsonDeserializer().deserialize(serializedType, nullptr, nullptr);

    ASSERT_EQ(newType.getId(), "Id");
    ASSERT_EQ(newType.getName(), "Name");
    ASSERT_EQ(newType.getDescription(), "Desc");
    ASSERT_EQ(newType.getAvailableTransports().getKeyList(), List<IString>("https", "plain"));

    serializer.reset();
    newType.serialize(serializer);
    ASSERT_EQ(serializedType, serializer.getOutput());
}

TEST_F(OpenDaqFactoriesTest, FBTypeDoesNotSupportTransports)
{
    const auto transports = Dict<IString, IInteger>({{"plain", 7420}});

    ASSERT_THROW(FunctionBlockTypeBuilder().setAvailableTransports(transports), NotSupportedException);
    ASSERT_THROW(DeviceTypeBuilder().setTypeSort(ComponentTypeSort::Undefined).setAvailableTransports(transports), NotSupportedException);

    const FunctionBlockTypePtr fbType = FunctionBlockTypeBuilder().setId("Id").build();
    ASSERT_THROW(fbType.getAvailableTransports(), NotImplementedException);
}

END_NAMESPACE_OPENDAQ
