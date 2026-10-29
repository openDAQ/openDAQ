#include <opendaq/instance_factory.h>
#include <opendaq/component_type_builder_factory.h>
#include <opendaq/device_type_factory.h>
#include <opendaq/streaming_type_factory.h>
#include <opendaq/component_type_private_ptr.h>
#include <opendaq/authentication_method_factory.h>
#include <gtest/gtest.h>
#include <coreobjects/property_object_factory.h>
#include <coreobjects/property_factory.h>

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
    for (const auto kind : {DeviceKind::Root, DeviceKind::Mirrored, DeviceKind::External})
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

END_NAMESPACE_OPENDAQ
