#include <opendaq/instance_factory.h>
#include <opendaq/component_type_builder_factory.h>
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

END_NAMESPACE_OPENDAQ
